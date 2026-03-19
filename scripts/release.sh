#!/bin/bash
set -euo pipefail

# Release CLI Tool for libpqcsb
# Usage: ./scripts/release.sh patch|minor|major
#
# Does:
# 1. Bumps version in CMakeLists.txt and include/pqcsb.h
# 2. Generates/updates CHANGELOG.md from git commits
# 3. Creates annotated git tag
# 4. Pushes tag to trigger GitHub Actions release

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 patch|minor|major"
    echo ""
    echo "Examples:"
    echo "  $0 patch   # Bump 0.1.1 → 0.1.2"
    echo "  $0 minor   # Bump 0.1.2 → 0.2.0"
    echo "  $0 major   # Bump 0.2.0 → 1.0.0"
    exit 1
fi

BUMP_TYPE="$1"
if [[ ! "$BUMP_TYPE" =~ ^(patch|minor|major)$ ]]; then
    echo "Error: Invalid bump type '$BUMP_TYPE'. Use patch, minor, or major."
    exit 1
fi

# Check we're on main branch
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [[ "$CURRENT_BRANCH" != "main" ]]; then
    echo "Error: Must be on 'main' branch, currently on '$CURRENT_BRANCH'"
    exit 1
fi

# Check working tree is clean
if [[ -n $(git status -s) ]]; then
    echo "Error: Working tree is not clean. Commit or stash changes first."
    git status
    exit 1
fi

# Get current version from CMakeLists.txt
CURRENT_VERSION=$(grep -A 3 "project(pqcsb" CMakeLists.txt | grep "VERSION" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
if [[ -z "$CURRENT_VERSION" ]]; then
    echo "Error: Could not extract current version from CMakeLists.txt"
    exit 1
fi

echo "Current version: $CURRENT_VERSION"

# Parse version components
IFS='.' read -r MAJOR MINOR PATCH <<< "$CURRENT_VERSION"

# Bump version based on type
case "$BUMP_TYPE" in
    patch)
        PATCH=$((PATCH + 1))
        ;;
    minor)
        MINOR=$((MINOR + 1))
        PATCH=0
        ;;
    major)
        MAJOR=$((MAJOR + 1))
        MINOR=0
        PATCH=0
        ;;
esac

NEW_VERSION="$MAJOR.$MINOR.$PATCH"
echo "New version: $NEW_VERSION"

# Update CMakeLists.txt
echo "Updating CMakeLists.txt..."
sed -i.bak "s/VERSION $CURRENT_VERSION/VERSION $NEW_VERSION/" CMakeLists.txt
rm -f CMakeLists.txt.bak

# Update include/pqcsb.h version constants
echo "Updating include/pqcsb.h..."
sed -i.bak "s/#define PQCSB_VERSION_MAJOR $MAJOR/#define PQCSB_VERSION_MAJOR $MAJOR/" include/pqcsb.h
sed -i.bak "s/#define PQCSB_VERSION_MINOR $MINOR/#define PQCSB_VERSION_MINOR $MINOR/" include/pqcsb.h
sed -i.bak "s/#define PQCSB_VERSION_PATCH [0-9]*/#define PQCSB_VERSION_PATCH $PATCH/" include/pqcsb.h
sed -i.bak "s/#define PQCSB_VERSION_STRING \"[0-9.]*\"/#define PQCSB_VERSION_STRING \"$NEW_VERSION\"/" include/pqcsb.h
rm -f include/pqcsb.h.bak

# Generate changelog entry from recent commits
echo "Generating changelog entry..."
CHANGELOG_TEMP=$(mktemp)

# Get commits since last tag (or all if no tags exist)
LAST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "")
if [[ -z "$LAST_TAG" ]]; then
    COMMITS=$(git log --pretty=format:"%h %s" --reverse)
else
    COMMITS=$(git log "${LAST_TAG}"..HEAD --pretty=format:"%h %s" --reverse)
fi

# Group commits by type
FEATURES=$(echo "$COMMITS" | grep -E "feat|feature" || true)
FIXES=$(echo "$COMMITS" | grep -E "fix|bug" || true)
OTHERS=$(echo "$COMMITS" | grep -v -E "feat|feature|fix|bug" || true)

# Create changelog entry
cat > "$CHANGELOG_TEMP" << EOF
## [$NEW_VERSION] - $(date +%Y-%m-%d)

### Added
$(echo "$FEATURES" | sed 's/^/- /' || echo "- (no new features)")

### Fixed
$(echo "$FIXES" | sed 's/^/- /' || echo "- (no bug fixes)")

### Changed
$(echo "$OTHERS" | sed 's/^/- /' || echo "- (no other changes)")

EOF

# Prepend to CHANGELOG.md if it exists, otherwise create it
if [[ -f CHANGELOG.md ]]; then
    tail -n +1 CHANGELOG.md >> "$CHANGELOG_TEMP"
    mv "$CHANGELOG_TEMP" CHANGELOG.md
else
    mv "$CHANGELOG_TEMP" CHANGELOG.md
fi

echo "Updated CHANGELOG.md"

# Stage changes
git add CMakeLists.txt include/pqcsb.h CHANGELOG.md

# Create commit
echo "Creating commit..."
git commit -m "Release: Bump version to $NEW_VERSION

- Updated CMakeLists.txt
- Updated include/pqcsb.h version constants
- Generated CHANGELOG.md entry"

# Create annotated tag
echo "Creating git tag..."
git tag -a "v$NEW_VERSION" -m "Release v$NEW_VERSION"

# Push commit and tag
echo "Pushing to GitHub..."
git push origin main
git push origin "v$NEW_VERSION"

echo ""
echo "✓ Release v$NEW_VERSION prepared and pushed!"
echo ""
echo "GitHub Actions will now:"
echo "  1. Build on Linux, macOS, Windows"
echo "  2. Sign with Cosign"
echo "  3. Generate SBOM"
echo "  4. Create SLSA provenance"
echo "  5. Scan for vulnerabilities"
echo "  6. Publish to GitHub Releases"
echo ""
echo "Watch progress at: https://github.com/msuliq/libpqcsb/actions"
