# LLM-gestützte statische Analyse — Proof of Concept

Seminar · Grundlage: **Wang et al. (2025),
*A Contemporary Survey of Large Language Model Assisted Program Analysis*,
arXiv:2502.18474** → `papers/2502.18474v1.pdf`.

## Idee in einem Satz
Wir behandeln das LLM **nicht** als Zaubertool, sondern als fehlbaren
Rohtext-Leser, und prüfen empirisch (als **Fallstudie**, nicht als
statistische Validierung), **ob und wann** die in der Survey genannten
Schwächen — Nicht-Determinismus, False Positives durch Kontext-Fehldeutung,
Token-/Slicing-Grenzen — bei einem **einzigen Modell (Groq)** tatsächlich
auftreten, und wie sich das zu einem klassischen Werkzeug (cppcheck) verhält.

## Forschungsfragen
- **RQ1 (Nicht-Determinismus):** Variiert der Output bei *identischem*
  Prompt + `temperature=0` über mehrere Läufe? → `--runs N`
- **RQ2 (False Positives):** Erfindet das LLM Schwachstellen bei harmlosen
  „Ködern" (z. B. ungenutzte Variable namens `dummyPassword`)?
- **RQ3 (Prompt-Sensitivität):** Ändert sich das Ergebnis zwischen naivem
  und strukturiertem Prompt?
- **RQ4 (Kontext/Slicing):** Sinkt die Genauigkeit in einem großen Repo,
  und hilft Slicing? → `repo_experiment/`
- **RQ5 (Semantik):** Liest das LLM `x²=33` als reine Mathematik oder als
  Maschinensemantik (signed/unsigned, mod 2ⁿ)? → `testcases/c/02_*`

## Struktur
```
papers/             Quell-Paper (Wang et al.)
testcases/          Testdateien mit GEZIELTEN Bugs + Ködern
  c/                01 buffer overflow + memory leak · 02 signed/unsigned
  java/             AuthManager: '==' Bug, "test"-Backdoor, dummyPassword-Köder
  python/           command injection + eval (optional, Mehrsprachigkeit)
  ground_truth.yaml gelabelte Wahrheit (echte Bugs vs. Köder) -> TP/FP/FN
src/
  analyzer.py       Rohtext -> Groq, N Läufe, speichert results/raw/*.json
  slicer.py         leichtgewichtiges Funktions-Slicing (Heuristik!)
  prompts/          deutsche Prompts (naiv vs. strukturiert)
  config.py         Modell, Temperatur, Läufe
baselines/          run_cppcheck.sh  (traditionelles Werkzeug, Zeitmessung)
repo_experiment/    großer-Kontext-vs-Slice Experiment
results/raw/        rohe LLM-Outputs (committet -> reproduzierbar)
docs/               seminararbeit.md (3–5 S.) + Survey-Notizen
```

## Setup
```bash
pip install -r requirements.txt
cp .env.example .env          # GROQ_API_KEY eintragen (console.groq.com/keys)
```

## Ausführen
```bash
# Ein Testfall, 5 Läufe, strukturierter Prompt:
python src/analyzer.py --file testcases/c/01_buffer_overflow_memleak.c --runs 5

# Prompt-Sensitivität vergleichen:
python src/analyzer.py --file testcases/java/AuthManager.java --prompt naive
python src/analyzer.py --file testcases/java/AuthManager.java --prompt structured

# Baseline (klassisches Werkzeug):
bash baselines/run_cppcheck.sh
```

## Wichtige Ehrlichkeit (für die Verteidigung)
- Es ist eine **qualitative Fallstudie** (kleines n), keine statistische
  Validierung.
- `slicer.py` ist eine **Heuristik**, keine echte Daten-/Kontrollfluss-Slice.
- Modellverhalten driftet; daher Modellname, Temperatur, Zeitstempel und
  Roh-Output in `results/raw/` festgehalten.
