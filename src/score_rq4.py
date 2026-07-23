#!/usr/bin/env python3
"""
score_rq4.py — wertet das Repo-Experiment (RQ4) aus:
Bedingung A (input_A_full*) vs. B (input_B_slice) gegen
repo_experiment/bugs/planted_bugs.yaml.

Heuristische Freitext-Suche wie score.py — stichprobenartig manuell
pruefen! Ausgabe: Markdown-Tabelle fuer docs/seminararbeit2.md 5.5.

    python src/score_rq4.py
"""
import os
import re
import json
import glob
import statistics

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(ROOT, "results", "raw")

# Muster fuer die zwei gepflanzten Bugs (DE+EN+CWE).
# Kalibriert am manuellen Durchgang 2026-07-05: 'freigegeben' allein ist
# Leak-Sprache (FP), nicht UAF; deutsche Formulierung "Pufferueberlauf ...
# 81 Zeichen" (generate_seed) zaehlt als Orts-Treffer fuer R2.
PATTERNS = {
    "R1-use-after-free": r"use.?after.?free|cwe-416|nach\s+(?:dem\s+)?free\w*\s+(?:noch|verwendet|benutzt|gelesen)|dangling",
    "R2-off-by-one":     r"off.?by.?one|malloc\(\s*81\s*\)|stream\[81\]|81\s+(?:statt|instead)|82\.?\s*(?:position|zeichen)|generate_seed.{0,200}(?:81|puffer|overflow)|81\s+zeichen.{0,120}(?:puffer|overflow|f.llt)|cwe-193",
}
CWE_RE = re.compile(r"cwe-\d+", re.IGNORECASE)


def load(cond_prefix):
    files = sorted(glob.glob(os.path.join(RAW, cond_prefix + "__*.json")))
    return [json.load(open(f, encoding="utf-8")) for f in files]


def analyze(records, label):
    if not records:
        print("| %s | 0 | (keine Laeufe gefunden) |  |  |  |  |" % label)
        return
    r1 = r2 = 0
    finding_counts, hashes, lens, lats = [], [], [], []
    for rec in records:
        text = rec["response_text"]
        low = text.lower()
        if re.search(PATTERNS["R1-use-after-free"], low):
            r1 += 1
        if re.search(PATTERNS["R2-off-by-one"], low, re.DOTALL):
            r2 += 1
        # nummerierte Top-Level-Findings ("1. **...")
        n = len(re.findall(r"(?m)^\s*\d+\.\s+\*\*", text))
        finding_counts.append(n)
        hashes.append(hash(text))
        lens.append(len(text))
        lats.append(rec.get("latency_s", 0))
    n = len(records)
    print("| %s | %d | %d/%d | %d/%d | %.1f (%d-%d) | %d von %d | %.1f s |" % (
        label, n, r1, n, r2, n,
        statistics.mean(finding_counts),
        min(finding_counts), max(finding_counts),
        len(set(hashes)), n,
        statistics.mean(lats)))


def main():
    print("| Bedingung | Laeufe | UAF (R1) | Off-by-one (R2, Ort) | "
          "Findings/Lauf (min-max) | eindeutige Outputs | Latenz |")
    print("|---|---|---|---|---|---|---|")
    for pref in ("input_A_full_xl", "input_A_full_s", "input_B_slice"):
        recs = load(pref)
        if recs or pref != "input_A_full_xl":
            analyze(recs, pref)

    print()
    print("Hinweis: heuristische Erstauswertung. R2 zaehlt ORTS-Treffer")
    print("(generate_seed/81-Puffer); exakte Off-by-one-Diagnose und alle")
    print("False Positives bitte manuell gegen planted_bugs.yaml pruefen.")


if __name__ == "__main__":
    main()
