#!/usr/bin/env python3
"""
run_rq4.py — RQ4-Orchestrierung: grosser Kontext (A) vs. Slice (B).

Erzeugt die Eingaben und ruft analyzer.py auf:
  A: alle Repo-Dateien konkateniert (input_A_*.c)
  B: heuristischer Slice um generate_puzzle (input_B_slice.c)

Groq Free Tier hat ein Tokens-pro-Minute-Limit; ist die Anfrage GROESSER
als das Limit, kommt HTTP 413 und kein Retry hilft. Darum probiert das
Skript Kontextvarianten von gross nach klein und nimmt die erste, die
funktioniert (ehrlich in der Arbeit dokumentieren, welche lief!).

    py repo_experiment\\run_rq4.py            (Windows)
    python3 repo_experiment/run_rq4.py --runs 5
"""
import os
import re
import sys
import argparse
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME_SRC = os.path.join(ROOT, "repo_experiment", "game", "src")
EXP_DIR = os.path.join(ROOT, "repo_experiment")

# Varianten gross -> klein. XL = komplettes Repo inkl. gettext.h-Rauschen.
# EMPIRISCHER BEFUND (2026-07, Free Tier, TPM=12k): XL (~15k Token) und
# full (~12,5k) -> HTTP 413; nur A_full_s (~11k) passt als Einzelanfrage.
# Fuer die Arbeit: Bedingung A == A_full_s (5 Dateien, ~1,5k Zeilen).
VARIANTS = [
    ("A_full_s",  ["utils.h", "utils.c", "sudoku.h", "sudoku.c", "main.c"]),
]
# Ausgemustert (dokumentiert, damit nachvollziehbar):
VARIANTS_TOO_LARGE = [
    ("A_full_xl", ["gettext.h", "utils.h", "utils.c", "outp.h", "outp.c",
                   "sudoku.h", "sudoku.c", "main.c"]),
    ("A_full",    ["utils.h", "utils.c", "outp.h", "outp.c",
                   "sudoku.h", "sudoku.c", "main.c"]),
]


def build_variant(name, files):
    path = os.path.join(EXP_DIR, "input_%s.c" % name)
    with open(path, "w", encoding="utf-8") as w:
        for fn in files:
            w.write("// ==================== DATEI: src/%s "
                    "====================\n" % fn)
            w.write(open(os.path.join(GAME_SRC, fn),
                         encoding="utf-8", errors="ignore").read())
            w.write("\n")
    return path


def build_slice():
    path = os.path.join(EXP_DIR, "input_B_slice.c")
    out = subprocess.run(
        [sys.executable, os.path.join(ROOT, "src", "slicer.py"),
         "--repo", os.path.join("repo_experiment", "game"),
         "--target", "generate_puzzle"],
        cwd=ROOT, capture_output=True, text=True, check=True)
    open(path, "w", encoding="utf-8").write(out.stdout)
    return path


def run_analyzer(path, runs, sleep):
    """Ruft analyzer.py auf, gibt Anzahl erfolgreicher Laeufe zurueck."""
    cmd = [sys.executable, os.path.join(ROOT, "src", "analyzer.py"),
           "--file", os.path.relpath(path, ROOT),
           "--runs", str(runs), "--sleep", str(sleep)]
    p = subprocess.run(cmd, cwd=ROOT, text=True)
    # analyzer druckt "Fertig: X/Y ..." — wir lesen X aus einem 2. Aufruf
    # nicht; stattdessen zaehlen wir die neuen JSONs.
    return p.returncode


def count_results(base):
    raw = os.path.join(ROOT, "results", "raw")
    if not os.path.isdir(raw):
        return 0
    return len([f for f in os.listdir(raw) if f.startswith(base + "__")])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", type=int, default=5)
    ap.add_argument("--sleep-full", type=float, default=90,
                    help="Pause zw. Laeufen in Bedingung A (TPM schonen; "
                         "Befund 2026-07: 45s reichte NICHT, 413 trotz "
                         "passender Einzelanfrage)")
    ap.add_argument("--sleep-slice", type=float, default=10)
    args = ap.parse_args()

    # --- Bedingung A: von gross nach klein probieren --------------------
    used = None
    for name, files in VARIANTS:
        path = build_variant(name, files)
        size = os.path.getsize(path)
        print("\n### Bedingung A, Variante %s (%d Bytes, ~%d Token)"
              % (name, size, size // 3.3))
        before = count_results("input_" + name)
        run_analyzer(path, args.runs, args.sleep_full)
        got = count_results("input_" + name) - before
        if got >= args.runs:
            used = name
            break
        print("!! Variante %s: nur %d/%d Laeufe ok (vermutlich 413/Limit) "
              "-> naechstkleinere Variante." % (name, got, args.runs))
    print("\n=> Bedingung A gelaufen mit Variante: %s" % (used or "KEINE!"))

    # --- Bedingung B: Slice ---------------------------------------------
    path_b = build_slice()
    print("\n### Bedingung B, Slice (%d Bytes)" % os.path.getsize(path_b))
    run_analyzer(path_b, args.runs, args.sleep_slice)

    print("\nFertig. Roh-Outputs: results/raw/input_A_*  und  "
          "results/raw/input_B_slice__*")
    if used:
        print("WICHTIG fuer die Arbeit: Bedingung A lief mit '%s' — "
              "Dateiliste in run_rq4.py dokumentiert." % used)


if __name__ == "__main__":
    main()
