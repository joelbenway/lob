#!/usr/bin/env bash
set -euo pipefail

LOB_BIN="${1:-./build/example/lobber}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exit_code=0

echo "=== Golden output tests ==="
for fixture in "$SCRIPT_DIR"/fixtures/*.json; do
  name=$(basename "$fixture" .json)
  expected="$SCRIPT_DIR/expected/${name}.expected.json"
  actual=$(mktemp)
  "$LOB_BIN" --json < "$fixture" > "$actual"
  if "$SCRIPT_DIR/compare_json.sh" "$expected" "$actual"; then
    echo "  PASS: $name"
  else
    echo "  FAIL: $name"
    exit_code=1
  fi
  rm "$actual"
done

echo ""
echo "=== Property checks ==="
if "$SCRIPT_DIR/property_checks.sh" "$LOB_BIN"; then
  echo "  PASS: properties"
else
  echo "  FAIL: properties"
  exit_code=1
fi

exit $exit_code
