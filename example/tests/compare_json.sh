#!/usr/bin/env bash
# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

set -euo pipefail

expected_file="$1"
actual_file="$2"
exit_code=0

compare_field() {
  local field="$1"
  local tolerance="$2"
  local expected="$3"
  local actual="$4"
  local diff
  diff=$(jq -n "$expected - $actual")
  local abs_diff
  abs_diff=$(jq -n "if $diff < 0 then -$diff else $diff end")
  local within
  within=$(jq -n "if $abs_diff <= $tolerance then 1 else 0 end")
  if [ "$within" != "1" ]; then
    echo "FAIL: $field: expected $expected, got $actual (diff=$diff, tolerance=$tolerance)"
    exit_code=1
  fi
}

length=$(jq 'length' "$expected_file")
if [ "$length" != "$(jq 'length' "$actual_file")" ]; then
  echo "FAIL: array length mismatch"
  exit_code=1
fi

for i in $(seq 0 $((length - 1))); do
  range_exp=$(jq ".[$i].range" "$expected_file")
  range_act=$(jq ".[$i].range" "$actual_file")
  if [ "$range_exp" != "$range_act" ]; then
    echo "FAIL: [$i].range: expected $range_exp, got $range_act"
    exit_code=1
  fi

  for field in velocity energy elevation deflection time_of_flight; do
    exp=$(jq ".[$i].$field" "$expected_file")
    act=$(jq ".[$i].$field" "$actual_file")
    compare_field "$field" 0.000001 "$exp" "$act"
  done
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
