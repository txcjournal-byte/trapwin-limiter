#!/usr/bin/env bash
# Prints pluginval output and turns failures into GitHub annotations.
# Usage: report-pluginval.sh <log file> <pluginval exit code>
log="$1"; rc="$2"
cat "$log"
if [ "$rc" != "0" ]; then
  n=0
  grep -nE "FAILED|!!!|[Ee]rror|failed|Test:|Starting test|Unable|Crash|assert" "$log" | tail -40 | while IFS= read -r line; do
    n=$((n+1)); [ $n -gt 10 ] && break
    echo "::error title=pluginval::$(printf '%s' "$line" | cut -c1-900)"
  done
  echo "::error title=pluginval tail::$(tail -c 3000 "$log" | tr '\n' '|' )"
  exit "$rc"
fi
tests=$(grep -c "Starting tests in" "$log")
summary=$(grep -E "SUCCESS|ALL TESTS PASSED|Finished validating" "$log" | tr '\n' ' ' | cut -c1-500)
echo "::notice title=pluginval passed::$tests test groups run. $summary"
echo "pluginval passed"
