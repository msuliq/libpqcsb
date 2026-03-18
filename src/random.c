/*
 * random.c — platform CSPRNG fill.
 *
 * Fills a byte buffer from the best available cryptographic random source:
 *
 *   1. arc4random_buf  (macOS, BSDs — always succeeds, no seeding needed)
 *   2. RtlGenRandom    (Windows — via advapi32, available XP+)
 *   3. getrandom       (Linux 3.17+ — blocks until CSPRNG seeded)
 *   4. getentropy      (glibc 2.25+, OpenBSD, macOS 10.12+ — 256 byte limit)
 *   5. /dev/urandom    (portable fallback — cached fd, never closed)
 *
 * All paths return PQCSB_OK on success or PQCSB_ERR_RANDOM on failure.
 * No Ruby/Python/Rust dependencies.
 */

#include "internal.h"
#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_SYS_RANDOM_H
#include <sys/random.h>
#endif

/* Cached /dev/urandom fd for the fallback path (opened once, never closed).
 * Protected with atomic CAS to avoid race on first initialization. */
#if !defined(HAVE_ARC4RANDOM_BUF) && !defined(HAVE_GETRANDOM) && \
    !defined(HAVE_GETENTROPY) && !defined(_WIN32)
static _Atomic int s_urandom_fd = -1;
#endif

PQCSB_API pqcsb_status_t
pqcsb_fill_random(uint8_t *dst, size_t len)
{
    if (!dst) return PQCSB_ERR_NULL_PARAM;
    if (len == 0) return PQCSB_OK;

#if defined(HAVE_ARC4RANDOM_BUF)
    arc4random_buf(dst, len);
    return PQCSB_OK;

#elif defined(_WIN32)
    if (!RtlGenRandom(dst, (ULONG)len))
        return PQCSB_ERR_RANDOM;
    return PQCSB_OK;

#elif defined(HAVE_GETRANDOM)
    size_t off = 0;
    while (off < len) {
        ssize_t r = getrandom(dst + off, len - off, 0);
        if (r < 0) {
            if (errno == EINTR) continue;
            return PQCSB_ERR_RANDOM;
        }
        off += (size_t)r;
    }
    return PQCSB_OK;

#elif defined(HAVE_GETENTROPY)
    size_t off = 0;
    while (off < len) {
        size_t chunk = len - off;
        if (chunk > 256) chunk = 256;
        if (getentropy(dst + off, chunk) != 0)
            return PQCSB_ERR_RANDOM;
        off += chunk;
    }
    return PQCSB_OK;

#else
    /* Fallback: cached /dev/urandom fd (opened once, never closed).
     * Use atomic CAS to ensure thread-safe initialization. */
    int fd = atomic_load(&s_urandom_fd);
    if (fd < 0) {
        fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
        if (fd < 0)
            return PQCSB_ERR_RANDOM;
        int expected = -1;
        if (!atomic_compare_exchange_strong(&s_urandom_fd, &expected, fd)) {
            /* Another thread already initialized it; close our duplicate. */
            close(fd);
            fd = atomic_load(&s_urandom_fd);
        }
    }
    size_t off = 0;
    while (off < len) {
        ssize_t r = read(fd, dst + off, len - off);
        if (r < 0) {
            if (errno == EINTR) continue;
            return PQCSB_ERR_RANDOM;
        }
        if (r == 0)
            return PQCSB_ERR_RANDOM;
        off += (size_t)r;
    }
    return PQCSB_OK;
#endif
}
