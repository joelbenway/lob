#!/usr/bin/env bash
# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

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
  if bash "$SCRIPT_DIR/compare_json.sh" "$expected" "$actual"; then
    echo "  PASS: $name"
  else
    echo "  FAIL: $name"
    exit_code=1
  fi
  rm "$actual"
done

echo ""
echo "=== Property checks ==="
if bash "$SCRIPT_DIR/property_checks.sh" "$LOB_BIN"; then
  echo "  PASS: properties"
else
  echo "  FAIL: properties"
  exit_code=1
fi

exit $exit_code

# This file is part of lob.
#
# lob is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# lob is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# lob. If not, see <https://www.gnu.org/licenses/>.
