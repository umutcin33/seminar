#!/usr/bin/env python3
"""Listet die fuer deinen Groq-API-Key verfuegbaren Modelle.
    python src/list_models.py
Danach ggf. MODEL in src/config.py anpassen.
"""
import os
import sys
import requests

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import config  # noqa: E402


def _decode(raw):
    for enc in ("utf-8-sig", "utf-16", "latin-1"):
        try:
            text = raw.decode(enc)
            if "\x00" not in text:
                return text
        except UnicodeDecodeError:
            continue
    return raw.decode("utf-8", "ignore")


def load_key():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    env = os.path.join(root, ".env")
    if os.path.exists(env):
        for line in _decode(open(env, "rb").read()).splitlines():
            line = line.strip()
            if line.startswith("GROQ_API_KEY") and "=" in line:
                return line.split("=", 1)[1].strip().strip('"').strip("'")
    return os.environ.get("GROQ_API_KEY")


key = load_key()
if not key or key == "your_key_here":
    sys.exit("FEHLER: GROQ_API_KEY fehlt in .env")

r = requests.get(config.API_BASE + "/models",
                 headers={"Authorization": "Bearer " + key}, timeout=30)
r.raise_for_status()
print("Verfuegbare Modelle:")
for m in r.json().get("data", []):
    print("  " + m.get("id", "?"))
