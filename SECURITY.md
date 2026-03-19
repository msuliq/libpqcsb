# Security Policy

## Reporting Security Vulnerabilities

If you discover a security vulnerability in libpqcsb, please **do not** open a public GitHub issue. Instead:

1. Go to https://github.com/msuliq/libpqcsb/security/advisories
2. Click **"Report a vulnerability"**
3. Fill in the details of the vulnerability
4. Click **"Draft a security advisory"**

This ensures the vulnerability is handled responsibly and users are notified with patches before public disclosure.

Alternatively, you can email `security@[project-owner-domain]` with details.

## Verifying Release Artifacts

All releases are signed and can be verified for authenticity. libpqcsb uses three complementary verification methods:

### 1. Cosign Keyless Signing (Recommended)

Every release artifact is signed using **Cosign with keyless OIDC**, tied to this GitHub repository. This is the most user-friendly approach — no need to download or manage public keys.

**Prerequisites:**
```bash
cosign --version  # v2.0.0 or later
```

**Verify a release artifact:**

Replace `VERSION` with the actual version (e.g., `0.1.2`):

```bash
# Download the release files
wget https://github.com/msuliq/libpqcsb/releases/download/v{VERSION}/libpqcsb-v{VERSION}-linux-x86_64.tar.gz
wget https://github.com/msuliq/libpqcsb/releases/download/v{VERSION}/libpqcsb-v{VERSION}-linux-x86_64.tar.gz.sig

# Verify the signature
cosign verify-blob \
  --certificate-identity 'https://github.com/msuliq/libpqcsb/actions/runs/*' \
  --certificate-oidc-issuer 'https://token.actions.githubusercontent.com' \
  --signature libpqcsb-v{VERSION}-linux-x86_64.tar.gz.sig \
  libpqcsb-v{VERSION}-linux-x86_64.tar.gz
```

If verification succeeds, you'll see:
```
Verified OK
```

**Why keyless signing?**
- No key management burden on users
- Signature is bound to the GitHub Actions run that created it
- OIDC token proves the release came from this repository
- Works across all platforms (Linux, macOS, Windows)

### 2. SHA256 Checksum Verification

For a simpler verification method without external tools, use SHA256 checksums:

**Verify using checksums:**

```bash
# Download the HASHES file
wget https://github.com/msuliq/libpqcsb/releases/download/v{VERSION}/HASHES

# Verify all artifacts against the checksums file
sha256sum -c HASHES
```

Expected output:
```
libpqcsb-v{VERSION}-linux-x86_64.tar.gz: OK
libpqcsb-v{VERSION}-macos-x86_64.tar.gz: OK
libpqcsb-v{VERSION}-windows-x86_64.zip: OK
```

**Note:** This method does NOT verify that the checksums themselves are authentic. Use Cosign or SLSA provenance (below) for stronger guarantees.

### 3. SLSA Provenance

Every release includes a **SLSA v3 provenance attestation** in the `.intoto.jsonl` file. This provides a cryptographic link proving:
- The artifact was built by this GitHub Actions workflow
- The source code at the given git commit was used
- The build occurred in the official GitHub Actions environment

**Prerequisites:**
```bash
go install github.com/slsa-framework/slsa-verifier/cmd/slsa-verifier@latest
```

**Verify provenance:**

```bash
# Download the provenance file
wget https://github.com/msuliq/libpqcsb/releases/download/v{VERSION}/libpqcsb-v{VERSION}-linux-x86_64.tar.gz.intoto.jsonl

# Verify the provenance
slsa-verifier verify-artifact \
  --provenance-path libpqcsb-v{VERSION}-linux-x86_64.tar.gz.intoto.jsonl \
  --source-uri github.com/msuliq/libpqcsb \
  --source-tag v{VERSION} \
  libpqcsb-v{VERSION}-linux-x86_64.tar.gz
```

If verification succeeds:
```
Verification succeeded!
```

This proves the artifact came from the specific commit, branch, and build environment.

## Software Bill of Materials (SBOM)

Each release includes an **SPDX JSON SBOM** (`*.sbom.json` file) documenting the contents and dependencies.

libpqcsb is a C library with **no external dependencies** — only the C standard library. This minimizes supply-chain risk.

View the SBOM:
```bash
wget https://github.com/msuliq/libpqcsb/releases/download/v{VERSION}/libpqcsb-v{VERSION}-linux-x86_64.sbom.json
cat libpqcsb-v{VERSION}-linux-x86_64.sbom.json | jq .
```

## Vulnerability Scanning

Every release is scanned for known vulnerabilities using **Trivy**, a static analysis tool that checks for CVEs in artifacts and dependencies.

Scan results are uploaded to GitHub Security as SARIF reports and appear in the **Security** tab of the repository.

## CI/CD Security

- **Pull Request CI**: All code changes are tested with multiple compilers (gcc, clang, MSVC) on Linux, macOS, and Windows
- **Sanitizers**: AddressSanitizer, MemorySanitizer, UBSan, and ThreadSan detect memory and undefined behavior issues
- **Static Analysis**: Semgrep and cppcheck flag security issues (weekly + on all PRs)
- **Coverage**: Code coverage is measured to ensure tests exercise the library thoroughly
- **Signing**: All release artifacts and SBOMs are signed with Cosign keyless OIDC

## Supported Versions

Only the latest release version receives security updates. Older versions are not patched.

For critical security issues in older versions, users should upgrade to the latest release.

## Contact

For security-related questions or responsible disclosure:
- **GitHub Security Advisory**: https://github.com/msuliq/libpqcsb/security/advisories
- **Code Review & Bug Reports**: https://github.com/msuliq/libpqcsb/issues

## References

- [Cosign Documentation](https://docs.sigstore.dev/cosign/overview/)
- [SLSA Framework](https://slsa.dev/)
- [SPDX](https://spdx.dev/)
- [Trivy Vulnerability Scanner](https://github.com/aquasecurity/trivy)
