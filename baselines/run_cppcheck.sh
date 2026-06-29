#!/usr/bin/env bash
# Baseline: traditionelles statisches Analysewerkzeug (cppcheck) auf den
# C-Testfaellen. Misst Laufzeit -> Vergleich "Werkzeug vs. LLM".
#
# Installation, falls noetig:
#   Ubuntu/Debian:  sudo apt-get install cppcheck
#   macOS:          brew install cppcheck
#
# Hinweis: cppcheck deckt NUR C/C++ ab (nicht Java/Python). Diese
# Asymmetrie in der Arbeit erwaehnen. Fuer Java waere SpotBugs/PMD das
# Pendant (hier bewusst ausgelassen, um den Umfang klein zu halten).

set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/results"
mkdir -p "$OUT"

if ! command -v cppcheck >/dev/null 2>&1; then
  echo "cppcheck nicht installiert. Bitte installieren (siehe Kopf dieser Datei)."
  exit 1
fi

echo "cppcheck-Version: $(cppcheck --version)"
echo "==> Analysiere testcases/c/ ..."

START=$(date +%s.%N)
cppcheck --enable=all --inconclusive --std=c11 \
  --template='{file}:{line}: [{severity}/{id}] {message}' \
  "$ROOT/testcases/c/" 2> "$OUT/cppcheck_report.txt"
END=$(date +%s.%N)

echo "----- cppcheck Befunde -----"
cat "$OUT/cppcheck_report.txt"
echo "----------------------------"
printf "cppcheck Laufzeit: %.2f s\n" "$(echo "$END - $START" | bc)"
echo "Report gespeichert: results/cppcheck_report.txt"

# Sekundaerer Vergleich: Compiler-Warnungen (gcc) als zweite Baseline.
echo
echo "==> gcc -Wall -Wextra (zum Vergleich):"
for f in "$ROOT"/testcases/c/*.c; do
  echo "--- $(basename "$f") ---"
  gcc -Wall -Wextra -fsyntax-only "$f" 2>&1 || true
done
