# libpqcsb - Secure Memory Buffer for Cryptographic Key Material

[![Version](https://img.shields.io/badge/version-0.1.2-blue.svg)](https://github.com/msuliq/libpqcsb/releases/tag/v0.1.2)
[![C11 Standard](https://img.shields.io/badge/C-11-brightgreen.svg)]()
[![License: MIT OR Apache-2.0](https://img.shields.io/badge/License-MIT%20OR%20Apache--2.0-blue.svg)](./LICENSE)
[![Build Status](https://img.shields.io/github/actions/workflow/status/msuliq/libpqcsb/release.yml?label=release)](https://github.com/msuliq/libpqcsb/actions/workflows/release.yml)

A standalone C library providing **mmap/mprotect/mlock-backed secure memory buffers** for cryptographic keys. Zero external dependencies. Portable across Linux, macOS, FreeBSD, and Windows.

---

## What It Is

**libpqcsb** is a cryptographic-grade secure memory container that protects key material from:

- **Accidental memory disclosure** - Guard pages (PROT_NONE) before and after data
- **Buffer overflows** - Right-aligned canaries trigger segfault on overflow
- **Swap disclosure** - mlock prevents swapping to disk
- **Tamper detection** - Canary verification detects corruption
- **Timing attacks** - Constant-time equality comparison
- **DRAM dumps** - Secure zeroing on deallocation

---

## What It Is Not

- **Not an encryption library** - Use libsodium, OpenSSL, or WolfSSL for that
- **Not a replacement for OS protections** - Complements, not replaces, kernel security
- **Not immune to privileged attackers** - Root/kernel can inspect any memory
- **Not async-safe** - Caller must not signal-interrupt during read/write access
- **Not deterministic on all platforms** - mlock may fail silently on resource-limited systems

---

## Security Model

### Memory Layout

```
┌─────────────────────┐
│  Guard Page (PROT_NONE)  │
├─────────────────────┤
│  [PAD]            │
│  [CANARY_PRE]     │  Right-aligned to maximize
│  [USER_DATA] ──────→  overflow detection
│  [CANARY_POST]    │
├─────────────────────┤
│  Guard Page (PROT_NONE)  │
└─────────────────────┘
```

- **Guard pages**: Allocated via mmap with PROT_NONE. Reads/writes trigger SIGSEGV.
- **Right-alignment**: User data pointer is calculated so buffer overflows immediately hit rear guard, not just canary.
- **Canaries**: 16-byte (configurable) random markers before and after data. Verified on deallocation and canary checks.
- **Fallback**: On systems without mmap, uses malloc + canaries only (no guard pages, weaker protection).

### Access Control

```c
pqcsb_read_guard_t guard = pqcsb_begin_read(buf);  // mprotect(PROT_READ)
if (guard.status == PQCSB_OK) {
    const uint8_t *data = guard.data;
    // ... use data ...
}
pqcsb_end_read(&guard);  // mprotect(PROT_NONE)
```

- **PROT_READ access**: Guard pages readable (for comparisons), guard pages unreadable (for overflow detection)
- **PROT_NONE when idle**: Prevents accidental reads from untrusted code
- **Atomic reference counting**: Multiple readers can overlap; only last reader re-protects to PROT_NONE
- **PROT_READ|PROT_WRITE for mutations**: `pqcsb_begin_write` allows in-place modification

### Platform-Specific Features

| Feature | Linux | macOS | FreeBSD | Windows |
|---------|-------|-------|---------|---------|
| Guard pages | mmap + PROT_NONE | mmap + PROT_NONE | mmap + PROT_NONE | VirtualAlloc + PAGE_NOACCESS |
| Memory locking | mlock (RLIMIT_MEMLOCK budgeted) | mlock | mlock | VirtualLock |
| Access control | mprotect | mprotect | mprotect | VirtualProtect |
| Core dump exclusion | MADV_DONTDUMP | MAP_NOCORE | MAP_NOCORE | - |
| Fork protection | MADV_WIPEONFORK | - | - | - |
| Secure zero | explicit_bzero | memset_s | explicit_bzero | SecureZeroMemory |
| CSPRNG | getrandom (fallback: /dev/urandom) | arc4random_buf | arc4random_buf | RtlGenRandom |

---

## Installation

### From CMake

```bash
git clone https://github.com/pqcsb/libpqcsb
cd libpqcsb
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j4
sudo cmake --install .
```

### Via pkg-config

After installation:

```bash
pkg-config --cflags --libs libpqcsb
# -I/usr/local/include -L/usr/local/lib -lpqcsb
```

### Vendoring

For single-file distribution (Ruby/Python C extensions):

```bash
./scripts/concat.sh  # Produces dist/pqcsb.c
cp dist/pqcsb.c include/pqcsb.h src/internal.h pqcsb_config.h /vendor/path/
```

The single-file bundle includes all three `.c` files (secure_zero.c, random.c, pqcsb.c) plus headers.

---

## Build Options

```bash
cmake .. \
  -DPQCSB_BUILD_SHARED=ON         # Build shared library (default: ON)
  -DPQCSB_BUILD_STATIC=ON         # Build static library (default: ON)
  -DPQCSB_BUILD_TESTS=ON          # Build test suite (default: ON)
  -DPQCSB_ALLOW_MALLOC_FALLBACK=OFF  # Enable malloc-only fallback (default: OFF)
```

**Note:** `PQCSB_ALLOW_MALLOC_FALLBACK=ON` is required on systems without mmap/VirtualAlloc. The malloc fallback provides **no guard pages** - only canaries. Not recommended for production.

---

## Core API

### Lifecycle

#### pqcsb_create

```c
pqcsb_status_t pqcsb_create(const uint8_t *data, size_t len, pqcsb_buf_t **out);
```

Allocate a buffer, copy `data` into it, apply protections. On success, `*out` points to an opaque buffer; on failure, `*out == NULL` and status code is returned.

```c
uint8_t key[32] = {...};
pqcsb_buf_t *buf;
pqcsb_status_t rc = pqcsb_create(key, 32, &buf);
if (rc == PQCSB_OK) {
    // buf is ready; use pqcsb_begin_read/begin_write
}
```

#### pqcsb_create_ex

```c
pqcsb_status_t pqcsb_create_ex(const uint8_t *data, size_t len,
                                const pqcsb_config_t *config,
                                pqcsb_buf_t **out);
```

Create with custom options (mlock budget, canary checks, etc.).

```c
pqcsb_config_t config = { .use_mlock = 1, .check_canaries_on_read = 1 };
pqcsb_create_ex(key, 32, &config, &buf);
```

#### pqcsb_create_random

```c
pqcsb_status_t pqcsb_create_random(size_t len, pqcsb_buf_t **out);
```

Allocate and fill from platform CSPRNG (no arguments to fill).

```c
pqcsb_buf_t *ephemeral_key;
pqcsb_create_random(32, &ephemeral_key);
```

#### pqcsb_create_inplace

```c
pqcsb_status_t pqcsb_create_inplace(
    size_t len,
    pqcsb_status_t (*fill)(uint8_t *data, size_t len, void *ctx),
    void *ctx,
    pqcsb_buf_t **out);
```

Allocate and call a callback to populate data. Callback receives writable region and must return `PQCSB_OK`.

```c
pqcsb_status_t my_fill(uint8_t *data, size_t len, void *ctx) {
    // Fill data in-place
    return pqcsb_fill_random(data, len);
}
pqcsb_create_inplace(32, my_fill, NULL, &buf);
```

#### pqcsb_clone

```c
pqcsb_status_t pqcsb_clone(pqcsb_buf_t *src, pqcsb_buf_t **out);
```

Deep copy entire buffer into a new independent buffer.

#### pqcsb_destroy

```c
void pqcsb_destroy(pqcsb_buf_t **buf);
```

Securely wipe and deallocate. Safe to call on NULL. Sets `*buf = NULL` on return.

```c
pqcsb_destroy(&buf);
// buf is now NULL
```

### Access Control

#### pqcsb_begin_read / pqcsb_end_read

```c
pqcsb_read_guard_t pqcsb_begin_read(pqcsb_buf_t *buf);
void pqcsb_end_read(pqcsb_read_guard_t *guard);
```

Read-only access. Guard returned by value with status field.

```c
pqcsb_read_guard_t guard = pqcsb_begin_read(buf);
if (guard.status == PQCSB_OK) {
    const uint8_t *data = guard.data;  // read-only
    size_t len = guard.len;
    // ... use data ...
}
pqcsb_end_read(&guard);  // guard._priv set to NULL
```

**Thread-safe** via atomic reference counting. Multiple threads can hold concurrent read guards.

#### pqcsb_begin_write / pqcsb_end_write

```c
pqcsb_write_guard_t pqcsb_begin_write(pqcsb_buf_t *buf);
void pqcsb_end_write(pqcsb_write_guard_t *guard);
```

Read-write access. Canaries rewritten on `pqcsb_end_write`.

```c
pqcsb_write_guard_t guard = pqcsb_begin_write(buf);
if (guard.status == PQCSB_OK) {
    uint8_t *data = guard.data;  // writable
    // ... modify data ...
}
pqcsb_end_write(&guard);  // canaries rewritten, re-protected
```

**Not thread-safe** - external synchronization required if multiple writers.

### Queries

```c
size_t pqcsb_len(const pqcsb_buf_t *buf);            // User data length
int pqcsb_is_wiped(const pqcsb_buf_t *buf);         // 1 if wipe() called
int pqcsb_is_locked(const pqcsb_buf_t *buf);        // 1 if mlock succeeded
int pqcsb_is_readable(const pqcsb_buf_t *buf);      // 1 if readers active
int pqcsb_get_read_refcount(const pqcsb_buf_t *buf);  // Number of active readers
size_t pqcsb_get_allocation_size(const pqcsb_buf_t *buf);  // Total mmap'd bytes
```

### Operations

#### pqcsb_wipe

```c
pqcsb_status_t pqcsb_wipe(pqcsb_buf_t *buf);
```

Securely zero data + canaries, munlock. Buffer remains allocated but all operations return `PQCSB_ERR_WIPED` afterward. Returns `PQCSB_ERR_BUSY` if readers are active.

#### pqcsb_check_canary

```c
pqcsb_status_t pqcsb_check_canary(pqcsb_buf_t *buf);
```

Verify canary integrity. Returns `PQCSB_OK` or `PQCSB_ERR_CANARY`.

#### pqcsb_slice

```c
pqcsb_status_t pqcsb_slice(pqcsb_buf_t *buf, size_t offset, size_t length,
                            pqcsb_buf_t **out);
```

Extract sub-range into new buffer. Source temporarily unlocked for copy.

#### pqcsb_ct_equal

```c
int pqcsb_ct_equal(pqcsb_buf_t *a, const uint8_t *b, size_t b_len);
int pqcsb_ct_equal_bufs(pqcsb_buf_t *a, pqcsb_buf_t *b);
```

Constant-time equality (1 = equal, 0 = not). Resists timing attacks.

### Utilities

```c
void pqcsb_secure_zero(void *ptr, size_t len);                // Secure memset
pqcsb_status_t pqcsb_fill_random(uint8_t *dst, size_t len);   // CSPRNG fill
```

---

## Error Handling

### Status Codes

```c
typedef enum {
    PQCSB_OK              =  0,   // Success
    PQCSB_ERR_ALLOC       = -1,   // mmap/malloc failed
    PQCSB_ERR_MLOCK       = -2,   // mlock failed (non-fatal)
    PQCSB_ERR_MPROTECT    = -3,   // mprotect/VirtualProtect failed
    PQCSB_ERR_WIPED       = -4,   // Buffer was wiped
    PQCSB_ERR_CANARY      = -5,   // Canary corruption
    PQCSB_ERR_RANDOM      = -6,   // CSPRNG failure
    PQCSB_ERR_RANGE       = -7,   // Bounds / range error
    PQCSB_ERR_NULL_PARAM  = -8,   // NULL argument
    PQCSB_ERR_BUSY        = -9,   // Operation blocked (e.g., wipe while reading)
} pqcsb_status_t;
```

### Error Categorization

```c
pqcsb_error_info_t pqcsb_get_error_info(pqcsb_status_t code);

typedef struct {
    pqcsb_status_t code;
    int is_fatal;        // 1 = buffer permanently unusable
    int is_recoverable;  // 1 = operation may be retried
} pqcsb_error_info_t;
```

Bindings can use `is_fatal` and `is_recoverable` to decide error handling strategy.

---

## Thread Safety

- **Safe (no external lock needed)**:
  - `pqcsb_begin_read` / `pqcsb_end_read` - Atomic refcount allows concurrent readers
  - `pqcsb_len`, `pqcsb_is_wiped`, `pqcsb_is_locked` - Simple queries
  - `pqcsb_destroy` - Can be called from any thread (idempotent)
  - `pqcsb_is_readable`, `pqcsb_get_read_refcount` - Atomic load only

- **Unsafe (requires external synchronization)**:
  - `pqcsb_begin_write` + `pqcsb_end_write` - Not protected against concurrent writes
  - `pqcsb_wipe` - Fails with `PQCSB_ERR_BUSY` if readers active; otherwise safe
  - `pqcsb_check_canary` - Temporarily acquires read lock; safe if no concurrent wipes
  - `pqcsb_slice` - Temporarily acquires read lock; safe if no concurrent wipes

**Example: Shared key with multiple readers**

```c
// Thread 1-N
pqcsb_read_guard_t guard = pqcsb_begin_read(shared_key);
if (guard.status == PQCSB_OK) {
    // All threads can read concurrently
    verify_signature(guard.data, guard.len);
}
pqcsb_end_read(&guard);

// Thread main (serial wipe)
if (!pqcsb_is_readable(shared_key)) {  // Check no readers active
    pqcsb_wipe(shared_key);  // Now safe to wipe
}
```

---

## Language Bindings

### Ruby (C Extension)

```c
#define PQCSB_MALLOC ruby_xmalloc
#define PQCSB_FREE   ruby_xfree
#include "pqcsb.h"

// Wrap pqcsb_buf_t in TypedData
static const rb_data_type_t pqcsb_buf_type = { ... };

static VALUE
rb_pqcsb_create(VALUE self, VALUE data_str) {
    pqcsb_buf_t *buf;
    pqcsb_status_t rc = pqcsb_create(
        RSTRING_PTR(data_str),
        RSTRING_LEN(data_str),
        &buf
    );
    if (rc != PQCSB_OK)
        rb_raise(rb_eRuntimeError, "%s", pqcsb_error_message(rc));

    return TypedData_Make_Struct(cPqcsbBuffer, pqcsb_buf_t,
                                 &pqcsb_buf_type, buf, buf);
}

static VALUE
rb_pqcsb_use(VALUE self, VALUE obj) {
    pqcsb_buf_t *buf;
    TypedData_Get_Struct(obj, pqcsb_buf_t, &pqcsb_buf_type, buf);

    pqcsb_read_guard_t guard = pqcsb_begin_read(buf);
    if (guard.status != PQCSB_OK)
        rb_raise(rb_eRuntimeError, "%s", pqcsb_error_message(guard.status));

    VALUE result = rb_yield(rb_str_new(guard.data, guard.len));
    pqcsb_end_read(&guard);
    return result;
}
```

### Python (CFFI)

```python
from cffi import FFI

ffi = FFI()
ffi.cdef("""
    typedef struct pqcsb_buf pqcsb_buf_t;

    typedef struct {
        const uint8_t *data;
        size_t len;
        int status;
        pqcsb_buf_t *_priv;
    } pqcsb_read_guard_t;

    pqcsb_status_t pqcsb_create(const uint8_t *, size_t, pqcsb_buf_t **);
    pqcsb_read_guard_t pqcsb_begin_read(pqcsb_buf_t *);
    void pqcsb_end_read(pqcsb_read_guard_t *);
    void pqcsb_destroy(pqcsb_buf_t **);
    ...
""")

lib = ffi.dlopen("libpqcsb")

class SecureBuffer:
    def __init__(self, data):
        buf_ptr = ffi.new("pqcsb_buf_t**")
        rc = lib.pqcsb_create(data, len(data), buf_ptr)
        if rc != 0:
            raise RuntimeError(ffi.string(lib.pqcsb_error_message(rc)).decode())
        self.buf = buf_ptr[0]

    def __enter__(self):
        self.guard = lib.pqcsb_begin_read(self.buf)
        if self.guard.status != 0:
            raise RuntimeError(...)
        return ffi.buffer(self.guard.data, self.guard.len)[:]

    def __exit__(self, *args):
        lib.pqcsb_end_read(self.guard_ptr)

    def __del__(self):
        lib.pqcsb_destroy(self.buf)
```

### Rust (bindgen)

```rust
use std::ptr;

pub struct SecureBuffer {
    buf: *mut pqcsb_buf_t,
}

impl SecureBuffer {
    pub fn new(data: &[u8]) -> Result<Self, i32> {
        let mut buf_ptr: *mut pqcsb_buf_t = ptr::null_mut();
        let rc = unsafe {
            pqcsb_create(data.as_ptr(), data.len(), &mut buf_ptr)
        };
        if rc == 0 {
            Ok(SecureBuffer { buf: buf_ptr })
        } else {
            Err(rc)
        }
    }

    pub fn read<F, R>(&self, f: F) -> R
    where
        F: FnOnce(&[u8]) -> R,
    {
        let mut guard = unsafe { pqcsb_begin_read(self.buf) };
        if guard.status != 0 {
            panic!("begin_read failed: {}", guard.status);
        }
        let slice = unsafe { std::slice::from_raw_parts(guard.data, guard.len) };
        let result = f(slice);
        unsafe { pqcsb_end_read(&mut guard) };
        result
    }
}

impl Drop for SecureBuffer {
    fn drop(&mut self) {
        unsafe { pqcsb_destroy(&mut self.buf) };
    }
}
```

---

## ABI Stability

libpqcsb follows semantic versioning for ABI:

- **PQCSB_VERSION**: Release version (0.1.0, 0.2.0, 1.0.0)
- **PQCSB_ABI_VERSION**: Struct/calling-convention version (1.0)
  - Incremented only on breaking ABI changes
  - Minor bump allows forward-compatible additions

Language bindings should call `pqcsb_check_abi_version()` at initialization:

```c
pqcsb_status_t rc = pqcsb_check_abi_version(
    PQCSB_ABI_VERSION_MAJOR,
    PQCSB_ABI_VERSION_MINOR
);
if (rc != PQCSB_OK) {
    fprintf(stderr, "ABI mismatch; recompile required\n");
    return 1;
}
```

---

## Portability

**Tested on:**

- Linux: glibc 2.25+ (getrandom), musl
- macOS: 10.12+ (arc4random_buf, memset_s)
- FreeBSD: 11.0+ (arc4random_buf, explicit_bzero)
- Windows: XP+ (RtlGenRandom, VirtualAlloc)

**No external dependencies** except:
- POSIX pthreads (optional, only for concurrent test)
- C11 `<stdatomic.h>` (required)

---

## Configuration

### Configurable Canary Size

Default is 16 bytes. Override before including header:

```c
#define PQCSB_CANARY_SIZE 32
#include "pqcsb.h"
```

### Configurable Allocator

Override malloc/free for language bindings:

```c
#define PQCSB_MALLOC ruby_xmalloc
#define PQCSB_FREE   ruby_xfree
#include "pqcsb.h"
```

Only affects `pqcsb_buf_t` struct allocation; data region always uses mmap/VirtualAlloc/malloc canaries.

---

## License

Dual-licensed under **MIT** or **Apache 2.0** at your choice. See [LICENSE](./LICENSE).

---

## Contributing

Contributions welcome! Please:

1. Write tests for any changes
2. Run `cmake --build . && ctest --output-on-failure`
3. Keep zero external dependencies
4. Maintain C11 portability

---

## See Also

- **[libpqcasn1](https://github.com/msuliq/libpqcasn1)** - DER/PEM/Base64 codec for PQC algorithms
- **[libsodium](https://github.com/jedisct1/libsodium)** - General-purpose cryptography library (higher-level)
- **[WolfSSL](https://github.com/wolfSSL/wolfssl)** - Lightweight TLS library with secure memory options
