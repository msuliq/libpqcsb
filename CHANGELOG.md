# Changelog

All notable changes to libpqcsb are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.2] - 2026-03-19

### Added
- Fully automated release pipeline with keyless signing (Cosign + Sigstore)
- Local CLI tool for version management (`./scripts/release.sh patch|minor|major`)
- Software Bill of Materials (SBOM) generation in SPDX JSON format
- SLSA provenance attestation for supply chain security
- Security vulnerability scanning with Trivy
- Artifact validation before publishing (integrity verification)
- Multi-platform release builds (Linux, macOS, Windows in parallel)
- Auto-generated changelogs from conventional commits
- Templates for Homebrew and Conan Center publishing
- Comprehensive release documentation (RELEASE_GUIDE.md)

### Changed
- Redesigned CI/CD to separate concerns:
  - CI runs only on PRs (no redundant testing on main push)
  - Release workflow triggers on version merge, auto-creates tag
  - Release workflow now builds, signs, and publishes all at once
- Release workflow now auto-detects version from CMakeLists.txt
- Removed dependency on manual git tag creation for releases
- Optimized release workflow to skip redundant test execution (CI already tested)

### Fixed
- Fixed version extraction in release script for multi-line CMakeLists.txt
- Added proper branch protection rule support (PR-only releases)
- Corrected Cosign action version (v3 from v4)

## [0.1.1] - 2026-03-19

### Added
- GitHub Actions CI pipeline for automated testing
- Multi-platform CI matrix testing (Linux/gcc, Linux/clang, macOS, Windows/MSVC)
- Multiple build configurations (Debug, Release)
- malloc fallback option testing
- Static and shared library builds
- pkg-config file generation and verification
- Comprehensive test coverage

### Changed
- Updated CI workflow to run on PRs and pushes to main/develop
- Improved build configuration with CMake options

### Fixed
- CI permissions and workflow dependencies

## [0.1.0] - 2026-03-16

### Added
- Initial release of libpqcsb
- Secure memory buffer for cryptographic key material
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
- Comprehensive test suite:
  - Allocation tests
  - Protection tests
  - Secure wiping tests
  - Random generation tests
  - Canary detection tests
  - Slice operations tests
  - Constant-time equality tests
- CMake build system
- Static and shared library builds
- Documentation and examples

---
