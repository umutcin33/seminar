#!/usr/bin/env python3
"""
Baseline: klassisches statisches Analysewerkzeug (cppcheck) auf den C-Faellen.
Plattformunabhaengig (laeuft auch unter Windows mit `py`).

    py baselines/run_cppcheck.py
    py baselines/run_cppcheck.py --exe "C:\\Program Files\\Cppcheck\\cppcheck.exe"

Sucht cppcheck auf dem PATH und an den ueblichen Windows-Installationsorten
(winget legt cppcheck NICHT automatisch in den PATH).

Hinweis: cppcheck deckt nur C/C++ ab, nicht Java/Python. Diese Asymmetrie in
der Arbeit erwaehnen (fuer Java waere SpotBugs/PMD das Pendant).
"""
import os
import sys
import time
import shutil
import argparse
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CDIR = os.path.join(ROOT, "testcases", "c")
OUT = os.path.join(ROOT, "results")
os.makedirs(OUT, exist_ok=True)

ap = argparse.ArgumentParser()
ap.add_argument("--exe", help="Pfad zu cppcheck.exe, falls nicht auf PATH")
args = ap.parse_args()

CANDIDATES = [
    args.exe,
    shutil.which("cppcheck"),
    r"C:\Program Files\Cppcheck\cppcheck.exe",
    r"C:\Program Files (x86)\Cppcheck\cppcheck.exe",
    os.path.expanduser(r"~\AppData\Local\Programs\Cppcheck\cppcheck.exe"),
]
exe = next((c for c in CANDIDATES if c and os.path.exists(c) or (c and shutil.which(c))), None)
if not exe:
    sys.exit("cppcheck nicht gefunden.\n"
             "  1) Pruefe in PowerShell:  & \"C:\\Program Files\\Cppcheck\\cppcheck.exe\" --version\n"
             "  2) Falls dort vorhanden, einfach erneut ausfuehren (Script sucht dort automatisch).\n"
             "  3) Sonst Pfad angeben:  py baselines/run_cppcheck.py --exe \"<Pfad zu cppcheck.exe>\"")

print("cppcheck:", subprocess.run([exe, "--version"], capture_output=True,
                                   text=True).stdout.strip(), "(", exe, ")")
print("Analysiere", CDIR, "...\n")

cmd = [exe, "--enable=all", "--inconclusive", "--std=c11",
       "--template={file}:{line}: [{severity}/{id}] {message}", CDIR]
t0 = time.time()
res = subprocess.run(cmd, capture_output=True, text=True)
dauer = time.time() - t0

report = (res.stderr or "").strip()
print("----- cppcheck Befunde -----")
print(report if report else "(keine Befunde)")
print("----------------------------")
print("Laufzeit: %.2f s" % dauer)

with open(os.path.join(OUT, "cppcheck_report.txt"), "w", encoding="utf-8") as f:
    f.write("cppcheck-Lauf\nLaufzeit: %.2f s\n\n%s\n" % (dauer, report))
print("Gespeichert: results/cppcheck_report.txt")
