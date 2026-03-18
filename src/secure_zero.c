/*
 * secure_zero.c — platform-aware secure memory zeroing.
 *
 * Guarantees that the compiler will not optimise away the zeroing, even
 * if the buffer is not read after the call.  Uses the best available
 * primitive on each platform:
 *
 *   1. memset_s      (C11 Annex K — Apple, some BSDs)
 *   2. explicit_bzero (glibc 2.25+, OpenBSD, FreeBSD)
 *   3. SecureZeroMemory (Windows)
 *   4. Volatile loop   (portable fallback)
 */

#include "internal.h"

PQCSB_API void
pqcsb_secure_zero(void *ptr, size_t len)
{
    if (!ptr || len == 0) return;

#if defined(HAVE_MEMSET_S)
    memset_s(ptr, len, 0, len);
#elif defined(HAVE_EXPLICIT_BZERO)
    explicit_bzero(ptr, len);
#elif defined(_WIN32)
    SecureZeroMemory(ptr, len);
#else
    volatile uint8_t *p = (volatile uint8_t *)ptr;
    size_t i;
    for (i = 0; i < len; i++) p[i] = 0;
#endif
}
