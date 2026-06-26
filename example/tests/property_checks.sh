#!/usr/bin/env bash
# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

set -euo pipefail

LOB_BIN="${1:-./build/example/lobber}"
exit_code=0

check_monotonic_decreasing() {
  local field="$1"
  local file="$2"
  local label="$3"
  local prev
  prev=$(jq ".[0].$field" "$file")
  local len
  len=$(jq 'length' "$file")
  for i in $(seq 1 $((len - 1))); do
    local cur
    cur=$(jq ".[$i].$field" "$file")
    if [ "$(jq -n "if $cur > $prev then 1 else 0 end")" = "1" ]; then
      echo "FAIL: $label not monotonic decreasing at index $i: $prev -> $cur"
      exit_code=1
      break
    fi
    prev="$cur"
  done
}

check_monotonic_increasing() {
  local field="$1"
  local file="$2"
  local label="$3"
  local prev
  prev=$(jq ".[0].$field" "$file")
  local len
  len=$(jq 'length' "$file")
  for i in $(seq 1 $((len - 1))); do
    local cur
    cur=$(jq ".[$i].$field" "$file")
    if [ "$(jq -n "if $cur < $prev then 1 else 0 end")" = "1" ]; then
      echo "FAIL: $label not monotonic increasing at index $i: $prev -> $cur"
      exit_code=1
      break
    fi
    prev="$cur"
  done
}

run_props() {
  local fixture="$1"
  local tmp
  tmp=$(mktemp)
  "$LOB_BIN" --json < "$fixture" > "$tmp"

  check_monotonic_decreasing "velocity" "$tmp" "velocity ($fixture)"
  check_monotonic_decreasing "energy" "$tmp" "energy ($fixture)"
  check_monotonic_increasing "time_of_flight" "$tmp" "time_of_flight ($fixture)"

  local nan_count
  nan_count=$(jq '[.[] | select((.velocity | isnan) or (.energy | isnan) or (.elevation | isnan) or (.deflection | isnan) or (.time_of_flight | isnan))] | length' "$tmp")
  if [ "$nan_count" != "0" ]; then
    echo "FAIL: output contains NaN values ($fixture)"
    exit_code=1
  fi

  local code
  code=$("$LOB_BIN" --json < "$fixture" > "$tmp"; echo $?)
  if [ "$code" != "0" ]; then
    echo "FAIL: lobber exited with non-zero code $code for valid input ($fixture)"
    exit_code=1
  fi

  rm "$tmp"
}

for f in example/tests/fixtures/*.json; do
  run_props "$f"
done

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
