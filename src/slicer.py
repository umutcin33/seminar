#!/usr/bin/env python3
"""
slicer.py — LEICHTGEWICHTIGES, heuristisches "Slicing" (KEINE echte
Programm-Slice mit Daten-/Kontrollfluss!).

Zweck: Im Zeitalter grosser Kontextfenster geht es nicht mehr ums
"Reinpassen", sondern um die FOKUSSIERUNG der Aufmerksamkeit. Wir vergleichen
(a) ganzes Repo als Kontext vs. (b) nur die relevanten Funktionen.

Heuristik (bewusst simpel, in der Arbeit als Naeherung benennen):
  1. Kommentare entfernen, Funktionsdefinitionen per Regex + Klammer-Matching.
  2. Naiver Aufrufgraph: welche bekannten Funktionen nennt eine Funktion?
  3. Slice = Zielfunktion + Callees + Caller (1 Hop, --depth).

Beispiel:
    python src/slicer.py --repo repo_experiment/game --target update_player
"""
import os
import re
import argparse

COMMENT = re.compile(r'//.*?$|/\*.*?\*/', re.MULTILINE | re.DOTALL)
FUNC_DEF = re.compile(
    r'(?:^|[};])\s*[A-Za-z_][\w\s\*]*?\b([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{',
    re.MULTILINE,
)
EXTS = {".c", ".h", ".cpp", ".hpp"}
KEYWORDS = {"if", "for", "while", "switch", "sizeof", "return", "else"}


def extract_functions(repo):
    """name -> dict(file, body). Klammer-Matching fuer den Funktionsrumpf."""
    funcs = {}
    for root, _, files in os.walk(repo):
        for fn in files:
            if os.path.splitext(fn)[1] not in EXTS:
                continue
            path = os.path.join(root, fn)
            raw = open(path, encoding="utf-8", errors="ignore").read()
            src = COMMENT.sub(" ", raw)
            for m in FUNC_DEF.finditer(src):
                name = m.group(1)
                if name in KEYWORDS:
                    continue
                start = src.index("{", m.start())
                depth, i = 0, start
                while i < len(src):
                    if src[i] == "{":
                        depth += 1
                    elif src[i] == "}":
                        depth -= 1
                        if depth == 0:
                            break
                    i += 1
                body = src[m.start():i + 1].strip()
                funcs[name] = {"file": os.path.relpath(path, repo), "body": body}
    return funcs


def build_callgraph(funcs):
    """callee-Kanten: ruft Funktion X eine bekannte Funktion Y auf?"""
    names = set(funcs)
    calls = {}
    for name, info in funcs.items():
        body = info["body"]
        calls[name] = {
            other for other in names
            if other != name and re.search(r'\b' + re.escape(other) + r'\s*\(', body)
        }
    return calls


def slice_around(target, funcs, calls, depth=1):
    if target not in funcs:
        raise SystemExit("FEHLER: Funktion '%s' nicht gefunden. Vorhanden: %s"
                         % (target, ", ".join(sorted(funcs))))
    selected = {target}
    frontier = {target}
    for _ in range(depth):
        nxt = set()
        for f in frontier:
            nxt |= calls.get(f, set())
            nxt |= {c for c, cs in calls.items() if f in cs}
        nxt -= selected
        selected |= nxt
        frontier = nxt
    return selected


def main():
    ap = argparse.ArgumentParser(description="Heuristic function-level slicer")
    ap.add_argument("--repo", required=True)
    ap.add_argument("--target", required=True, help="Zielfunktion (Bug-Stelle)")
    ap.add_argument("--depth", type=int, default=1)
    args = ap.parse_args()

    funcs = extract_functions(args.repo)
    calls = build_callgraph(funcs)
    selected = slice_around(args.target, funcs, calls, args.depth)

    total = sum(len(f["body"]) for f in funcs.values())
    sliced = sum(len(funcs[n]["body"]) for n in selected)
    pct = 100 * sliced // max(total, 1)
    print("// SLICE um '%s' (depth=%d)" % (args.target, args.depth))
    print("// %d/%d Funktionen, ~%d/%d Zeichen (%d%% des Repos)\n"
          % (len(selected), len(funcs), sliced, total, pct))
    for n in sorted(selected):
        print("// --- %s  (%s) ---" % (n, funcs[n]["file"]))
        print(funcs[n]["body"])
        print()


if __name__ == "__main__":
    main()
