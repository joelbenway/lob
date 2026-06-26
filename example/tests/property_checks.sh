#!/usr/bin/env bash
set -euo pipefail

LOB_BIN="${1:-./build/example/lobber}"
exit_code=0

check_monotonic_decreasing() {
  local field="$1"
  local file="$2"
  local label="$3"
  local prev=$(jq ".[0].$field" "$file")
  local len=$(jq 'length' "$file")
  for i in $(seq 1 $((len - 1))); do
    local cur=$(jq ".[$i].$field" "$file")
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
  local prev=$(jq ".[0].$field" "$file")
  local len=$(jq 'length' "$file")
  for i in $(seq 1 $((len - 1))); do
    local cur=$(jq ".[$i].$field" "$file")
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
  local tmp=$(mktemp)
  "$LOB_BIN" --json < "$fixture" > "$tmp"

  check_monotonic_decreasing "velocity" "$tmp" "velocity ($fixture)"
  check_monotonic_decreasing "energy" "$tmp" "energy ($fixture)"
  check_monotonic_increasing "time_of_flight" "$tmp" "time_of_flight ($fixture)"

  local elev0=$(jq '.[0].elevation' "$tmp")
  local elev0_ok=$(jq -n "if ($elev0 | length) < 0.000001 then 1 else 0 end")
  if [ "$elev0_ok" != "1" ]; then
    echo "FAIL: elevation at range 0 is not zero: $elev0 ($fixture)"
    exit_code=1
  fi

  local nan_count=$(jq '[.[] | select((.velocity | isnan) or (.energy | isnan) or (.elevation | isnan) or (.deflection | isnan) or (.time_of_flight | isnan))] | length' "$tmp")
  if [ "$nan_count" != "0" ]; then
    echo "FAIL: output contains NaN values ($fixture)"
    exit_code=1
  fi

  rm "$tmp"
}

for f in example/tests/fixtures/*.json; do
  run_props "$f"
done

exit $exit_code
