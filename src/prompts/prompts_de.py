"""
Deutsche Prompts fuer das Experiment.

naive vs structured: misst Prompt-Sensitivitaet (RQ3).
semantic: dedizierte RQ5-Frage zur Maschinensemantik (x^2 = 33).
"""

PROMPT_NAIVE = """Analysiere den folgenden Quellcode und finde alle Fehler \
und Sicherheitsluecken.

```{lang}
{code}
```
"""

PROMPT_STRUCTURED = """Du bist ein statisches Analysewerkzeug. Untersuche den \
Quellcode ausschliesslich anhand des gezeigten Textes (kein Ausfuehren).

Gib eine Liste der gefundenen Schwachstellen aus, je Eintrag:
- Zeilennummer
- CWE-Kennung (falls zutreffend)
- Kurzbeschreibung
- Schweregrad (niedrig/mittel/hoch)

Wichtig: Wenn eine Stelle KEINE echte Schwachstelle ist (z. B. eine \
ungenutzte, harmlose Variable), markiere sie NICHT als Schwachstelle. \
Wenn der Code fehlerfrei ist, sage das ausdruecklich.

```{lang}
{code}
```
"""

# RQ5: Maschinensemantik. Fragt direkt nach den Loesungen, ohne "mod 256" zu
# verraten. Korrekte (maschinennahe) Antwort: x in {17, 111, 145, 239}.
# Naive (rein mathematische) Fehlantwort: "keine Loesung" (sqrt(33) irrational).
PROMPT_SEMANTIC = """Betrachte die folgende C-Funktion:

```{lang}
{code}
```

Frage: Kann die Funktion `square_mod256` jemals den Wert 33 zurueckgeben?
Falls ja, fuer welche Eingaben x im Bereich 0..255? Falls nein, begruende warum.
Nenne alle Loesungen und erklaere deine Begruendung Schritt fuer Schritt.
"""

PROMPTS = {
    "naive": PROMPT_NAIVE,
    "structured": PROMPT_STRUCTURED,
    "semantic": PROMPT_SEMANTIC,
}
