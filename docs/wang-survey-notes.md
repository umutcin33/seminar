# Notizen — Wang et al. (2025), arXiv:2502.18474

> Verankerung für Vortrag & Seminararbeit. Abschnittsnummern direkt zitierfähig.

## Aufbau des Papers
- **§ II** Hintergrund (statisch / dynamisch / hybrid; *context window* erklärt)
- **§ III** LLM für **statische Analyse** ← unser Fokus für die Methode
  - § III-D *Static Analysis Enhancement*; enthält *False Positive Filtering / Reduce False Positives*
- **§ IV** LLM für dynamische Analyse
- **§ V** LLM für hybride Ansätze
- **§ VI** **Discussion** ← unser Fokus für die *Schwächen*
  - **§ VI-A Challenges** (die Schwächen, die wir testen)
  - § VI-B Future Directions
- **§ VII** Conclusion

> Hinweis: „Abschnitt 3 / 6" aus unserer Planung = **§ III** (statische Analyse)
> und **§ VI** (Discussion/Challenges). Passt.

## § VI-A Challenges — wörtlich relevant (das ist unser Aufhänger)

**Model Characteristics and Limitations**
- „LLMs **are non-deterministic and may produce varying outputs for
  identical inputs**, complicating consistency in repeated vulnerability
  assessments." → **RQ1** (wir messen genau das mit `--runs`, temp=0).
- „LLMs are prone to **hallucinations, generating fabricated information**
  that misleads vulnerability detection." → **RQ2** (Köder `dummyPassword`,
  `unused_api_token`).

**Technical Limitations**
- „LLMs struggle with **variable reuse, often confusing identically named
  variables in different scopes**." → stützt unsere Köder-Idee; optional ein
  Testfall mit *shadowing*.
- „LLMs struggle to analyze **logic vulnerabilities involving intricate
  control flows, complex nesting, and time-based … conditions**." → stützt
  unseren **Auth-Bypass/Backdoor**-Fall (Logikfehler, den Linter kaum finden).

**Cost and Dependency Issues**
- „effectiveness … relies on **prompt engineering**, which requires
  significant expertise. Poorly designed prompts can lead to ineffective
  results." → **RQ3** (naiv vs. strukturiert).
- „**inherent token limits** … restrict their ability to handle extensive or
  complex programs, making **scalability** a challenge." → **RQ4**
  (großer Kontext vs. Slicing). *Program slicing* wird im Paper mehrfach als
  Gegenmaßnahme genannt (z. B. § III, Refs [36], [163-Bereich]).

## Nützliches Gegenargument (für Komplementarität)
§ VI-B / Future Directions: LLMs „**overcome the rule-based limitations of
traditional tools** by analyzing complex code contexts and identifying
nuanced vulnerabilities … missed by automated methods" (Project Naptime).
→ Genau hier kann das LLM einen klassischen Linter wie cppcheck schlagen
(semantische Logikfehler). Belegt unsere „wer wo gewinnt"-These.

## Takeaways (im Paper als „Takeaway N" markiert)
[Beim Lesen 2–3 passende Takeaways aus § III heraussuchen und mit Seitenzahl
notieren — zeigt dem Prof gründliche Lektüre.]

## Offene Zitate zum Nachtragen
- [ ] Seitenzahlen für die obigen Zitate
- [ ] 1 konkretes statisches-Analyse-Framework aus § III als Beispiel
