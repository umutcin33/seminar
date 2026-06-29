#!/usr/bin/env python3
"""
score.py — wertet die Roh-Outputs in results/raw/ gegen ground_truth.yaml aus.

ACHTUNG: heuristische Schluesselwort-/Regex-Suche im Freitext, KEINE
perfekte Bewertung. Dient als erste Naeherung; bitte stichprobenartig
manuell pruefen (besonders die Koeder-Treffer -> wurde der Koeder als Luecke
GEMELDET oder korrekt VERWORFEN?).

Ausgabe: Markdown-Tabellen (direkt in die Arbeit kopierbar) +
Nicht-Determinismus-Signale (Laengen-Streuung, variierende CWE-Mengen).

    python src/score.py
    python src/score.py --file AuthManager
"""
import os
import re
import sys
import json
import glob
import argparse
import statistics

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import config  # noqa: E402

try:
    import yaml
except ImportError:
    sys.exit("Bitte 'pip install pyyaml' (bzw. py -m pip install pyyaml).")

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Heuristische Muster je Bug-/Koeder-ID (case-insensitive, DE+EN+CWE).
PATTERNS = {
    # echte Bugs
    "C1-buffer-overflow": r"buffer.?overflow|puffer|arabellek|\bstrcpy\b|cwe-12[01]",
    "C1-memory-leak":     r"memory.?leak|speicherleck|leck|nicht freigegeben|kein(e)? *free|\bfree\b|cwe-401",
    "C2-signedness":      r"signed|unsigned|vorzeichen|size_t|negativ|cwe-19[5-7]",
    "C2-int-overflow":    r"integer.?overflow|ganzzahl|.berlauf|overflow|wrap|modulo|mod *256|cwe-190",
    "J1-string-equality": r"\.equals|cwe-59[57]|referenz|reference|identit|gleichheits|==",
    "J1-backdoor":        r"backdoor|hintert|umgeh|\bcontains\b|\"test\"|test.{0,20}enth|cwe-(912|489|284|304)|unbefugt|unerlaubt|als admin|bypass",
    "P1-command-injection": r"command.?inject|befehl|kommando|shell\s*=\s*true|os.?command|cwe-78|inject",
    "P1-eval":            r"\beval\b|cwe-9[45]|code.?inject|beliebige.{0,12}code|arbitrary.?code",
    # Koeder (Treffer = potenzieller False Positive -> manuell pruefen!)
    "J1-trap-dummyPassword": r"\bdummyPassword\b",
    "P1-trap-token":         r"\bunused_api_token\b",
}


def load_runs(name_filter=None):
    groups = {}
    for path in sorted(glob.glob(os.path.join(ROOT, config.RESULTS_DIR, "*.json"))):
        try:
            d = json.load(open(path, encoding="utf-8"))
        except Exception:
            continue
        src = d.get("source_file", "")
        if name_filter and name_filter.lower() not in src.lower():
            continue
        key = (src, d.get("prompt_name", "?"))
        groups.setdefault(key, []).append(d)
    return groups


def hit(pattern, text):
    return re.search(pattern, text, re.IGNORECASE | re.DOTALL) is not None


def snippet(token_pat, text, width=90):
    m = re.search(token_pat, text, re.IGNORECASE)
    if not m:
        return ""
    s = max(0, m.start() - width)
    return text[s:m.end() + width].replace("\n", " ").strip()


def main():
    ap = argparse.ArgumentParser(description="Score LLM outputs vs ground truth")
    ap.add_argument("--file", default=None, help="nur Dateien mit diesem Namen")
    args = ap.parse_args()

    gt = yaml.safe_load(open(os.path.join(ROOT, config.GROUND_TRUTH), encoding="utf-8"))
    gt_by_file = {os.path.basename(tc["file"]): tc for tc in gt["testcases"]}

    groups = load_runs(args.file)
    if not groups:
        sys.exit("Keine Outputs in results/raw/ gefunden. Erst analyzer.py laufen lassen.")

    print("# Auswertung (heuristisch — bitte manuell gegenpruefen)\n")

    for (src, prompt), runs in sorted(groups.items()):
        tc = gt_by_file.get(os.path.basename(src))
        n = len(runs)
        texts = [r.get("response_text", "") for r in runs]
        lengths = [len(t) for t in texts]
        cwe_sets = [frozenset(re.findall(r"cwe-\d+", t, re.IGNORECASE)) for t in texts]
        distinct_cwe = len(set(cwe_sets))

        print("## %s  —  prompt=%s  (n=%d)\n" % (os.path.basename(src), prompt, n))
        # RQ1-Signale
        if lengths:
            print("- Laenge (Zeichen): min %d / max %d / Ø %d  "
                  "→ Streuung trotz temp=0 = Nicht-Determinismus-Signal"
                  % (min(lengths), max(lengths), int(statistics.mean(lengths))))
        print("- Verschiedene CWE-Mengen ueber Laeufe: %d von %d "
              "(1 = stabil, >1 = variiert)\n" % (distinct_cwe, n))

        if not tc:
            print("_(keine ground_truth fuer %s)_\n" % src)
            continue

        # Echte Bugs
        print("**Echte Bugs (TP-Rate):**\n")
        print("| Bug | CWE | gefunden k/N |")
        print("|---|---|---|")
        for bug in tc.get("real_bugs", []):
            pat = PATTERNS.get(bug["id"])
            k = sum(hit(pat, t) for t in texts) if pat else 0
            mark = "" if pat else " ⚠kein Muster"
            print("| %s | %s | %d/%d%s |" % (bug["id"], bug.get("cwe", "-"), k, n, mark))
        print()

        # Koeder
        traps = tc.get("traps", [])
        if traps:
            print("**Koeder (Treffer = moeglicher False Positive — MANUELL pruefen):**\n")
            print("| Koeder | erwaehnt k/N | Beispiel-Snippet |")
            print("|---|---|---|")
            for tr in traps:
                pat = PATTERNS.get(tr["id"], re.escape(tr["id"]))
                k = sum(hit(pat, t) for t in texts)
                ex = ""
                for t in texts:
                    ex = snippet(pat, t)
                    if ex:
                        break
                ex = (ex[:110] + "…") if len(ex) > 110 else ex
                print("| %s | %d/%d | %s |" % (tr["id"], k, n, ex.replace("|", "/")))
            print()

    print("\n---\n_Hinweis: Muster koennen falsch-positiv/negativ matchen. "
          "Vor allem Koeder-Zeilen lesen: hat das Modell die Stelle als Luecke "
          "GEMELDET oder korrekt als harmlos VERWORFEN?_")


if __name__ == "__main__":
    main()
