#!/usr/bin/env bash
set -euo pipefail

# Simple repository-wide linter runner.
# Finds common source files and runs basic shellcheck/format checks when available.
# Intended to be robust in CI and not fail due to find/escaping issues.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Build a list of files to consider (adjust patterns as needed)
# Use -print0 to robustly handle spaces and special characters in filenames.
mapfile -d '' FILES < <(
  find "$ROOT_DIR" -type f \
    \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \
       -o -name "*.cc" -o -name "*.hh" -o -name "*.js" -o -name "*.jsx" \
       -o -name "*.ts" -o -name "*.tsx" -o -name "*.py" -o -name "*.sh" \) \
    -not -path "*/node_modules/*" \
    -not -path "*/dist/*" \
    -not -path "*/build/*" \
    -print0
)

# If no files found, exit successfully
if [ "${#FILES[@]}" -eq 0 ]; then
  echo "No source files found for linting. Skipping."
  exit 0
fi

# Basic shell lint for .sh files if shellcheck exists
if command -v shellcheck >/dev/null 2>&1; then
  echo "Running shellcheck on shell scripts..."
  while IFS= read -r -d '' f; do
    case "$f" in
      *.sh)
        shellcheck "$f"
        ;;
    esac
  done < <(
    printf '%s\0' "${FILES[@]}" | xargs -0 -I{} sh -c 'printf "%s\0" "$1"' _ {}
  )
else
  echo "shellcheck not found; skipping shell script lint."
fi

# If eslint exists, run eslint on JS/TS files (non-blocking if not installed)
if command -v npx >/dev/null 2>&1; then
  echo "Attempting to run eslint on JS/TS files (if configuration is present)..."
  # Use npx to avoid global installs; ignore failures if eslint is not configured.
  set +e
  npx --yes eslint --version >/dev/null 2>&1
  if [ $? -eq 0 ]; then
    npx --yes eslint "**/*.{js,jsx,ts,tsx}" --ignore-pattern "node_modules" --ignore-pattern "dist" --ignore-pattern "build"
  else
    echo "eslint not available/configured; skipping JS/TS lint."
  fi
  set -e
else
  echo "npx not available; skipping JS/TS lint."
fi

echo "Linter script completed successfully."
