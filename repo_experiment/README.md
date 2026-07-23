# Experiment 2 — Großer Kontext vs. Slicing

## Frage
Wang et al. nennen Token-/Kontextgrenzen als LLM-Schwäche und Program
Slicing als Gegenmaßnahme. Aber 2026 haben Modelle riesige Kontextfenster
(Gemini ~1 Mio+ Token). Also ist die Frage **nicht mehr** „passt der Code
rein?", sondern:

> **Sinkt die Treffergenauigkeit, wenn der relevante Bug in einem großen,
> verrauschten Kontext „untergeht" („lost in the middle")? Und hilft
> Slicing (Aufmerksamkeit fokussieren) auch dann noch?**

## Aufbau
1. **Repo wählen:** ein kleines, eigenständiges C-Projekt (~2–5k Zeilen,
   10–20 Dateien). Gute Kandidaten: ein einfaches Terminal-Spiel
   (Snake/Tetris/2048 in C). In `repo_experiment/game/` ablegen.
2. **Bug pflanzen:** 1–2 bekannte Bugs einbauen (z. B. Off-by-one,
   Use-after-free) und in `bugs/planted_bugs.yaml` dokumentieren
   (Datei, Zeile, CWE, Beschreibung).
3. **Zwei Bedingungen laufen lassen** (gleiches Modell, gleiche Temperatur):
   - **(A) Voll:** gesamtes Repo (alle Dateien aneinandergehängt) als Kontext.
   - **(B) Slice:** nur die relevanten Funktionen via
     `python src/slicer.py --repo repo_experiment/game --target <funktion>`.
4. **Vergleichen:** Wird der gepflanzte Bug in A vs. B gefunden? Wie viele
   False Positives je Bedingung? Latenz/Token-Kosten?

## Erwartung / ehrliche Einordnung
- Es kann sein, dass das Modell den Bug auch im Vollkontext findet — dann
  ist *das* der Befund (große Kontextfenster mildern die in der Survey
  beschriebene Grenze ab).
- Wahrscheinlicher: mehr False Positives und/oder Auslassungen im
  Vollkontext; der Slice ist präziser. Das stützt die Slicing-These
  **in aktualisierter Form**.
- **Wichtig:** Unser Slicer ist eine *Heuristik* (Funktionsebene, naiver
  Aufrufgraph), KEINE echte Daten-/Kontrollfluss-Slice. In der Arbeit so
  benennen.

## Status
- [x] Repo ausgewählt und nach `game/` kopiert — **nudoku** (jubalh/nudoku,
      commit ff3507c, GPL-3.0), 8 Dateien, ~2000 Zeilen C
- [x] Bugs gepflanzt + `bugs/planted_bugs.yaml` (UAF in `new_puzzle`,
      Off-by-one in `generate_seed`)
- [x] Lauf A (Vollkontext) — Free-Tier-TPM-Limit erzwang Variante
      `A_full_s` (5 Dateien, ~11 k Token); XL/full → HTTP 413.
      5 Läufe in `results/raw/input_A_full_s__*`
- [x] Lauf B (Slice um `generate_puzzle`, 13 % des Repos) — 5 Läufe in
      `results/raw/input_B_slice__*`
- [x] Auswertung: `python src/score_rq4.py` + manueller Abgleich
- [x] Auswertung in `docs/seminararbeit2.md` §5.5 — Kernbefund:
      Off-by-one im Vollkontext 0/5, im Slice 5/5 (Ort); UAF 0/10 überall
