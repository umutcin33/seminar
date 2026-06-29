#!/usr/bin/env python3
"""
analyzer.py — der "Postbote".

Liest eine Quelldatei als ROHTEXT (kein AST, kein CFG) und schickt sie an EIN
LLM (Groq, OpenAI-kompatible API). Laeuft N-mal (Nicht-Determinismus) und
speichert jeden Roh-Output mit Metadaten -> Reproduzierbarkeit.

Behandelt Rate-Limits (HTTP 429) automatisch: wartet die vom Server genannte
Zeit ab und versucht es erneut (Groq Free Tier: Tokens-pro-Minute begrenzt).

Beispiele:
    python src/analyzer.py --file testcases/c/01_buffer_overflow_memleak.c --runs 10
    python src/analyzer.py --file testcases/java/AuthManager.java --prompt naive --runs 10
"""
import os
import re
import sys
import json
import time
import hashlib
import argparse
import datetime

import requests

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(SCRIPT_DIR)
sys.path.insert(0, SCRIPT_DIR)

import config                          # noqa: E402
from prompts.prompts_de import PROMPTS  # noqa: E402

EXT_LANG = {".c": "c", ".h": "c", ".java": "java", ".py": "python", ".cpp": "cpp"}


def _decode(raw):
    """Robuste Dekodierung (Windows-.env kann BOM/UTF-16 sein)."""
    for enc in ("utf-8-sig", "utf-16", "latin-1"):
        try:
            text = raw.decode(enc)
            if "\x00" not in text:
                return text
        except UnicodeDecodeError:
            continue
    return raw.decode("utf-8", "ignore")


def load_env():
    path = os.path.join(ROOT, ".env")
    if not os.path.exists(path):
        return
    for line in _decode(open(path, "rb").read()).splitlines():
        line = line.strip()
        if line and not line.startswith("#") and "=" in line:
            k, v = line.split("=", 1)
            os.environ.setdefault(k.strip(), v.strip().strip('"').strip("'"))


def _wait_seconds(resp):
    """Ermittelt die Wartezeit bei 429 (Header oder Fehlertext)."""
    ra = resp.headers.get("retry-after")
    if ra:
        try:
            return float(ra)
        except ValueError:
            pass
    m = re.search(r"try again in ([\d.]+)s", resp.text)
    return float(m.group(1)) if m else 15.0


def call_llm(prompt_text, model, temperature, api_key, max_retries=8):
    """Ein Chat-Completion-Aufruf mit automatischem 429-Retry.
    Gibt (text, latenz_s) zurueck."""
    url = config.API_BASE + "/chat/completions"
    headers = {"Authorization": "Bearer " + api_key}
    body = {
        "model": model,
        "messages": [{"role": "user", "content": prompt_text}],
        "temperature": temperature,
    }
    last = None
    for _ in range(max_retries + 1):
        t0 = time.time()
        resp = requests.post(url, json=body, headers=headers, timeout=120)
        latency = time.time() - t0
        if resp.status_code == 429:
            wait = min(_wait_seconds(resp) + 1.0, 75.0)
            print("    429 Rate-Limit -> warte %.0fs ..." % wait)
            time.sleep(wait)
            last = resp
            continue
        resp.raise_for_status()
        data = resp.json()
        try:
            return data["choices"][0]["message"]["content"], latency
        except (KeyError, IndexError):
            return json.dumps(data, ensure_ascii=False), latency
    last.raise_for_status()  # Retries erschoepft -> 429 werfen


def main():
    ap = argparse.ArgumentParser(description="LLM raw-text code analyzer (Groq)")
    ap.add_argument("--file", required=True, help="Pfad zur Quelldatei")
    ap.add_argument("--prompt", default="structured", choices=list(PROMPTS),
                    help="Prompt-Variante (default: structured)")
    ap.add_argument("--runs", type=int, default=config.RUNS)
    ap.add_argument("--temperature", type=float, default=config.TEMPERATURE)
    ap.add_argument("--model", default=config.MODEL)
    ap.add_argument("--sleep", type=float, default=1.0,
                    help="Pause zwischen Laeufen in s (Rate-Limit schonen)")
    ap.add_argument("--outdir", default=os.path.join(ROOT, config.RESULTS_DIR))
    args = ap.parse_args()

    load_env()
    api_key = os.environ.get("GROQ_API_KEY")
    if not api_key or api_key == "your_key_here":
        sys.exit("FEHLER: GROQ_API_KEY fehlt. Kopiere .env.example -> .env "
                 "und trage deinen Schluessel ein (console.groq.com/keys).")

    code = open(args.file, encoding="utf-8", errors="ignore").read()
    lang = EXT_LANG.get(os.path.splitext(args.file)[1], "")
    template = PROMPTS[args.prompt]
    prompt_text = template.format(lang=lang, code=code)
    prompt_hash = hashlib.sha256(prompt_text.encode()).hexdigest()[:12]

    os.makedirs(args.outdir, exist_ok=True)
    base = os.path.splitext(os.path.basename(args.file))[0]
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")

    print("Datei=%s  Modell=%s  prompt=%s  temp=%s  runs=%d"
          % (args.file, args.model, args.prompt, args.temperature, args.runs))
    print("-" * 60)

    ok = 0
    for i in range(1, args.runs + 1):
        try:
            text, latency = call_llm(prompt_text, args.model,
                                     args.temperature, api_key)
        except requests.HTTPError as e:
            print("  run %d: HTTP-Fehler %s -> %s"
                  % (i, e.response.status_code, e.response.text[:160]))
            continue
        except requests.RequestException as e:
            print("  run %d: Netzwerkfehler -> %s" % (i, e))
            continue

        record = {
            "timestamp": datetime.datetime.now().isoformat(),
            "source_file": args.file,
            "language": lang,
            "provider": "groq",
            "model": args.model,
            "temperature": args.temperature,
            "prompt_name": args.prompt,
            "prompt_sha256_12": prompt_hash,
            "run_index": i,
            "latency_s": round(latency, 2),
            "response_text": text,
        }
        out = os.path.join(args.outdir,
                           "%s__%s__t%s__run%02d__%s.json"
                           % (base, args.prompt, args.temperature, i, stamp))
        json.dump(record, open(out, "w", encoding="utf-8"),
                  ensure_ascii=False, indent=2)
        ok += 1
        print("  run %02d  %5.1fs  %5d Zeichen  -> %s"
              % (i, latency, len(text), os.path.basename(out)))
        if i < args.runs and args.sleep:
            time.sleep(args.sleep)

    print("-" * 60)
    print("Fertig: %d/%d Laeufe erfolgreich. Roh-Outputs in: %s"
          % (ok, args.runs, args.outdir))


if __name__ == "__main__":
    main()
