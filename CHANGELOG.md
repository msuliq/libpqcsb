# Changelog

All notable changes to libpqcsb are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-03-19

### Added
- Initial release of libpqcsb: secure memory buffer for cryptographic key material
- Guard pages (PROT_NONE) before and after data region
- mlock to prevent swapping to disk
- mprotect-based access control (PROT_NONE when idle)
- Canary values for tamper/overflow detection
- Secure zeroing via best available platform primitive
- Constant-time equality comparison
- Platform CSPRNG for random buffer generation
- Thread-safe concurrent read access with atomic reference counting
- Cross-platform support:
  - Unix/Linux (mmap/mprotect/mlock backend)
  - macOS (mmap/mprotect/mlock backend)
  - Windows (VirtualAlloc/VirtualProtect backend)

### CI/CD & Supply Chain
- Reusable GitHub Actions workflow architecture eliminating duplicate configuration
- Multi-platform automated builds (Linux, macOS, Windows)
- Comprehensive test matrix (gcc, clang, MSVC; Debug, Release; with malloc fallback)
- Sanitizer support (AddressSanitizer, MemorySanitizer, UBSan, ThreadSan)
- Code coverage reporting via gcovr with HTML artifacts
- Keyless Cosign signing for all release artifacts (OIDC via Sigstore)
- SBOM generation in SPDX JSON format
- SLSA v3 provenance attestation for supply chain security
- Trivy vulnerability scanning on releases
- Security scanning (Semgrep, gitleaks) in CI pipeline

### Build & Install
- CMake build system with modern practices
- Static and shared library builds
- pkg-config support for easy integration
- Coverage instrumentation support (-DPQCSB_ENABLE_COVERAGE)
- Sanitizer support via compile flags
- Cross-compilation support with platform detection

### Documentation
- SECURITY.md with artifact verification guide (Cosign, SHA256, SLSA)
- Comprehensive CHANGELOG
- API documentation in header file
- Multiple test examples demonstrating usage

### Testing
- Comprehensive test suite covering:
  - Allocation and protection
  - Secure wiping and zeroing
  - Random generation
  - Canary detection
  - Slice operations
  - Constant-time equality
  - Concurrent access (optional)
  - Edge cases and malloc fallback
