# LLM-gestützte statische Codeanalyse: Möglichkeiten und Grenzen

### Eine empirische Fallstudie auf Basis von Wang et al. (2025)

*Seminar · Thema: „A Contemporary Survey of LLM-Assisted Program Analysis“ · Autor: Umut Ulas Cin · Betreuer: Prof. Sinz*
*Stand: 23. Juli 2026*

---

## Abstract

Coding-Agents wie Claude oder Codex erzeugen heute ganze Programme, doch
ihre Korrektheit bleibt offen. Diese Arbeit untersucht **statische Analyse
durch Large Language Models (LLMs)** als möglichen Baustein und prüft
empirisch, **ob und wann** die von Wang et al. [1] benannten Schwächen
auftreten. Das LLM wird bewusst nicht als Orakel, sondern als **fehlbarer
Rohtext-Leser** behandelt. In einer kontrollierten **Fallstudie** (ein
Modell, `temperature=0`, gelabelte Testfälle) messen wir Nicht-Determinismus,
False Positives, Prompt-Sensitivität sowie das Verhalten bei
Maschinensemantik und vergleichen mit einem klassischen Werkzeug (cppcheck).
**Erste Ergebnisse:** Bei identischem Input und `temperature=0` schwanken die
Ausgaben stark (Java-Fall: 8/10 Läufe mit unterschiedlichen CWE-Mengen,
derselbe Bug mit wechselnden, teils falschen CWE-Codes); alle eingebauten
Bugs werden auf den kleinen Dateien zwar zuverlässig (10/10) erkannt, die in
der Survey genannten Schwächen (Nicht-Determinismus, Halluzination,
Prompt-Abhängigkeit) treten also bereits messbar auf. Im Repo-Experiment
(RQ4) kehrt sich das Bild: Im Vollkontext (~1 500 Zeilen) wird keiner der
zwei gepflanzten Bugs gefunden, im heuristischen Slice wird die
Off-by-one-Stelle in 5/5 Läufen markiert; ein Use-after-free bleibt in
beiden Bedingungen unerkannt.

---

## 1. Einleitung und Motivation

Mit der Verbreitung LLM-basierter Codegenerierung verschiebt sich die zentrale
Frage von „Kann das Modell Code schreiben?" zu „Können wir dem Ergebnis
vertrauen?". Statische Analyse, die Untersuchung von Code **ohne Ausführung**,
ist ein etablierter Ansatz zur frühen Fehler- und Schwachstellenerkennung.
LLMs versprechen hier durch kontextsensitives Codeverständnis neue
Möglichkeiten [1, § III].

Diese Arbeit vertritt die **These**, dass ein LLM kein Orakel, sondern ein
fehlbarer Textleser ist, und prüft empirisch, *ob und wann* die in der Survey
[1, § VI-A] genannten Schwächen real auftreten. Der **Beitrag** ist dreifach:
(1) ein schlanker, reproduzierbarer Experiment-Harness; (2) eine Fallstudie
mit gelabelten Testfällen inkl. „Ködern"; (3) die Einordnung der Ergebnisse
in die Survey und in formale Gegenmaßnahmen.

---

## 2. Hintergrund: Die Survey von Wang et al. [1]

Die Survey [1] (*A Contemporary Survey of LLM Assisted Program Analysis*)
ordnet die Forschung in **statische** (§ III), **dynamische**
(§ IV) und **hybride** Analyse (§ V) und diskutiert in § VI Herausforderungen
und Zukunftsrichtungen.

**Relevant für diese Arbeit:**

- **§ III, Statische Analyse:** LLMs werden u. a. zur Schwachstellenerkennung,
  zur Reduktion von False Positives (§ III-D) und in Kombination mit
  klassischen Repräsentationen eingesetzt. Konkrete Beispiele: **LLift**
  [1, § III-A, S. 4] kombiniert constraint-geführte Pfadanalyse mit
  Task-Dekomposition zur Erkennung von Use-before-Initialization-Bugs auf
  OS-Ebene; **LATTE** [1, § III-A, S. 5] isoliert gefährliche Datenflüsse per
  **Code-Slicing**, um „die Komplexität für das LLM durch kontextspezifischen
  Input zu reduzieren" — genau die Gegenmaßnahme, die RQ4 empirisch prüft.
  In [1, § III-D, S. 12] adjudizieren LLMs Alerts klassischer Analysewerkzeuge
  als True/False Positives (Flynn et al.).
- **§ VI-A, Challenges:** Die Survey nennt explizit:
  - *Model Characteristics (S. 19):* „LLMs **are non-deterministic and may
    produce varying outputs for identical inputs**"; sie sind „prone to
    **hallucinations, generating fabricated information**".
  - *Technical Limitations (S. 18 f.):* Schwierigkeiten bei „**variable reuse
    … confusing identically named variables** in different scopes" und bei
    „**logic vulnerabilities involving intricate control flows**".
  - *Cost & Dependency (S. 19):* „relies on **prompt engineering**, which
    requires significant expertise" und „**inherent token limits** … making
    **scalability** a challenge".

Diese vier Punkte bilden die Grundlage unserer Forschungsfragen.

**Verortung dieser Arbeit in der Survey.** Wang et al. [1] kartieren das Feld
entlang dreier Analysearten: statisch (§ III), dynamisch (§ IV) und hybrid
(§ V). Diese Arbeit greift ausschließlich den **statischen Zweig** heraus,
konkret die LLM-gestützte Schwachstellenerkennung auf Quelltext (§ III-A) —
und dort bewusst den einfachsten Modus: das Modell liest **Rohtext**, ohne
klassische Zwischenrepräsentationen (AST/CFG) und ohne Werkzeug-Kopplung,
wie sie Frameworks wie LLift oder LATTE einsetzen. Gegenstand der Prüfung
ist dabei nicht ein einzelnes Framework, sondern der
**Herausforderungen-Katalog der Survey (§ VI-A)**: Die dort
literaturbasiert und qualitativ formulierten Schwächen werden in fünf
messbare Forschungsfragen (§ 3) operationalisiert und an gelabelten
Testfällen überprüft. Die Arbeit testet also die *Aussagen der Survey am
Modell*: Reproduzieren sich die behaupteten Schwächen unter kontrollierten
Bedingungen (ein Modell, `temperature=0`, Ground Truth) — und greift die in
der Survey beschriebene Gegenmaßnahme des Slicing ([1, § III-A], LATTE) auch
2026 noch, wo Kontextfenster längst groß genug sind?

---

## 3. Forschungsfragen

Jede Forschungsfrage operationalisiert eine konkrete Aussage der Survey:

- **RQ1 (Nicht-Determinismus):** Variiert der Output bei identischem Prompt
  und `temperature=0` über mehrere Läufe?
  *(← [1, § VI-A], „Model Characteristics": „varying outputs for identical inputs")*
- **RQ2 (False Positives / Halluzination):** Markiert das Modell harmlose
  „Köder" (z. B. ungenutzte Variable `dummyPassword`) als Schwachstelle?
  *(← [1, § VI-A]: „hallucinations, generating fabricated information")*
- **RQ3 (Prompt-Sensitivität):** Unterscheiden sich naiver und strukturierter
  Prompt im Ergebnis?
  *(← [1, § VI-A], „Cost & Dependency": „relies on prompt engineering")*
- **RQ4 (Kontext/Slicing):** Sinkt die Genauigkeit in einem großen Repository,
  und hilft fokussierendes Slicing?
  *(← [1, § VI-A]: „inherent token limits … scalability"; Gegenmaßnahme:
  Slicing wie in LATTE, [1, § III-A])*
- **RQ5 (Maschinensemantik):** Liest das Modell `x²=33` als reine Mathematik
  oder als bit-vector-/Modulo-Arithmetik (signed/unsigned)?
  *(← eigene Erweiterung der Halluzinations-These auf numerische
  Maschinensemantik; Anknüpfung an [1, § VI-A] „Technical Limitations")*

---

## 4. Methodik

**Bewusste Abgrenzung.** Es wird **kein** AST/CFG erzeugt und **kein**
SonarQube eingesetzt. Das Modell erhält den **Rohtext** der Quelldatei,
genau dieser Modus wird bewertet.

**Aufbau.** Ein einziges Modell (Groq, `llama-3.3-70b-versatile`) bei
`temperature=0` (kontrolliert, mit Betreuung abgestimmt). Jede Datei wird
N-mal analysiert; jeder Roh-Output wird mit Modell, Temperatur, Zeitstempel
und Prompt-Hash in `results/raw/` gespeichert (Reproduzierbarkeit).

**Bewertung.** Die „Wahrheit" steht in `ground_truth.yaml`: echte Bugs (mit
CWE) und Köder. Damit lassen sich **True Positives, False Negatives und False
Positives** zählen statt subjektiv zu urteilen.

**Vergleich.** Zwei Prompts (naiv vs. strukturiert) für RQ3; cppcheck als
klassische Baseline (mit Laufzeitmessung) für die C-Fälle.

**Einschränkung.** Dies ist eine **qualitative Fallstudie** mit kleinem n,
keine statistische Validierung.

**Tabelle 1 — Testfälle**

| Datei | Sprache | Echte Bugs (CWE) | Köder (erwartete FP) |
|---|---|---|---|
| `01_buffer_overflow_memleak.c` | C | Buffer Overflow (120), Memory Leak (401) | — |
| `02_signed_unsigned.c` | C | Signedness (195) | — |
| `AuthManager.java` | Java | `==`-Vergleich (597), Backdoor (912) | `dummyPassword` |

*Hinweis: Der `uint8_t`-Ausdruck `x*x` (mod 256) in `02_signed_unsigned.c` ist **kein Defekt**, sondern die Maschinensemantik-Sonde für RQ5 (§ 5.6); er zählt daher nicht als eingebauter Bug und ist aus dem Baseline-Vergleich (§ 5.7) ausgenommen.*

---

## 5. Experimente und Ergebnisse

Datenbasis: ein Modell (Groq `llama-3.3-70b-versatile`), `temperature=0`,
je 10 Läufe pro (Datei, Prompt); Roh-Outputs in `results/raw/`. Auswertung
mit `src/score.py` (heuristische Schlüsselwortsuche, stichprobenartig manuell
gegengeprüft).

### 5.1 RQ1: Nicht-Determinismus (bei `temperature=0`)

Trotz `temperature=0` schwanken die Ausgaben bei identischem Input deutlich:

| Datei (Prompt) | Länge Zeichen (min/max/Ø) | versch. CWE-Mengen / N |
|---|---|---|
| `01_buffer_overflow` (structured) | 822 / 1181 / 1009 | 1 / 10 |
| `02_signed_unsigned` (structured) | 905 / 1481 / 1144 | 2 / 10 |
| `AuthManager` (structured) | 809 / 2216 / 1349 | **8 / 10** |
| `AuthManager` (naive) | 2868 / 3478 / 3273 | 1 / 10 |

Besonders deutlich beim Java-Fall (strukturiert): **8 von 10 Läufen liefern
unterschiedliche CWE-Mengen**. Wichtig: Alle echten Bugs wurden in jedem Lauf
korrekt *lokalisiert* und *beschrieben* — die Instabilität betrifft
ausschließlich die **CWE-Klassifikation**. Das korrekte CWE für den
`==`-Vergleichsfehler wäre CWE-597; kein einziger Lauf lieferte dieses.
Stattdessen wurden CWE-304, CWE-757 („Weak Cryptographic Algorithm", semantisch
falsch), CWE-297 und CWE-595 vergeben — je nach Lauf unterschiedlich. In Lauf 9
wurde dieselbe Stelle sogar **doppelt** mit zwei verschiedenen CWEs gelistet
(CWE-304 und CWE-297). Das Modell findet die richtige *Stelle*, erfindet aber
eine inkonsistente *Klassifikation* — direkte Bestätigung von
„non-deterministic … varying outputs" und „hallucinations" [1, § VI-A].

### 5.2 Erkennung echter Bugs (True-Positive-Rate)

Auf diesen kleinen Dateien werden alle eingebauten Bugs in jedem Lauf erkannt:

| Datei | Bug (CWE) | gefunden k/N |
|---|---|---|
| `01_buffer_overflow` | Buffer Overflow (120) | 10/10 |
| `01_buffer_overflow` | Memory Leak (401) | 10/10 |
| `02_signed_unsigned` | Signedness (195) | 10/10 |
| `AuthManager` (beide Prompts) | `==` statt `equals` (597) | 10/10 |
| `AuthManager` (beide Prompts) | Backdoor (912) | 10/10 |

Die hohe Trefferquote auf kleinem Code stützt die These, dass die Schwächen
**skalen-/kontextabhängig** sind (→ RQ4).

### 5.3 RQ2: False Positives / Halluzination (Köder `dummyPassword`)

**Methodischer Befund (zentral).** Die erste Version der Testdateien enthielt
Kommentare, die die Lösung verraten („FALLE / False-Positive-Test"). Das Modell
las diese und zitierte in Lauf 1 wörtlich („… eine ‚Falle'… um False Positives zu
testen"). Der Köder-Test war damit **nicht blind**. Wir haben kommentarfreie
Varianten (`testcases/clean/`) erzeugt und RQ2 darauf erneut ausgewertet.

Behandlung des harmlosen `dummyPassword` (je 10 Läufe, manuell ausgewertet):

| Version | Prompt | tatsächlich erwähnt¹ | als FP gelistet (CWE) | davon gehedgt² | harmlos verworfen | neutral³ |
|---|---|---|---|---|---|---|
| kontaminiert | structured | 10/10 | 0/10 | — | 10/10 | 0/10 |
| kontaminiert | naive | 10/10 | 0/10 | — | 0/10 | 10/10 |
| **blind (clean)** | **structured** | **10/10** | **10/10** | 4/10 | **0/10** | 0/10 |
| blind (clean) | naive | **10/10** | **0/10** | — | 0/10 | **10/10** |

¹ `score.py` erfasst per Regex `\bdummyPassword\b` nur direkte Namenstreffer
(kontaminiert structured: 10/10; kontaminiert naive: 7/10; blind structured: 6/10;
blind naive: 2/10). Indirekte Referenzen über den Wert `"default_placeholder"`
oder die Beschreibung „Dummy-Passwort" werden vom Skript nicht gezählt; manuelle
Auswertung aller 40 Läufe ergibt in allen Gruppen 10/10 Erwähnungen.

² Gehedgt = CWE vergeben, aber Freitext enthält explizit „möglicherweise nur ein
Platzhalter" bzw. „keine direkte Schwachstelle" (Läufe 3, 4, 7, 10 des blinden
structured-Tests). Der Befund wird formal gelistet — zählt daher als FP.

³ Neutral = `dummyPassword` als „unnötige Variable" oder „Dummy-Passwort, das nie
überprüft wird" erwähnt, **kein** CWE vergeben, kein explizites Urteil. Typisch
für den naiven Prompt: Codestil-Anmerkung statt Sicherheitsbefund.

**Kernbefund:** Der Kommentar-Leak *maskierte* die Halluzinationsneigung; mit
verräterischem Kommentar verwarf das Modell den Köder stets korrekt (0 FP). Erst
im **blinden** Test zeigt sich der strukturierte Prompt als fehleranfällig:
**10/10 Läufe** listen `dummyPassword` mit einer CWE-Kennung als Schwachstelle
(6 stark, 4 gehedgt mit „möglicherweise") — ein echter False Positive in allen
Fällen. Zusätzlich erfindet das Modell weitere Phantombefunde (hardcodierte Rolle
„ADMIN" als CWE-798, Konsolenausgaben als CWE-259). Der naive Prompt bleibt
robust: er erwähnt `dummyPassword` in 10/10 Läufen, vergibt aber **keinen CWE**
und hält die Stelle neutral als Codestil-Anmerkung (0 FP). Die echten Bugs
(`==`, Backdoor) werden auch blind in **10/10** Läufen erkannt. Dies bestätigt
die Survey-Aussage [1, § VI-A] zu „hallucinations / fabricated information"
und zeigt: ein
valider FP-Test muss **blind** sein; zudem treibt Prompt-Struktur die FP-Rate
stärker als Prompt-Freiheit.

### 5.4 RQ3: Prompt-Sensitivität

Gleicher Code, gleiches Modell — nur der Prompt unterscheidet sich.
Verglichen werden 10 Läufe je Prompt auf `AuthManager.java`:

| Kriterium | structured | naive |
|---|---|---|
| `==`-Bug gefunden | 10/10 | 10/10 |
| Backdoor gefunden | 10/10 | 10/10 |
| Korrektes CWE (CWE-597 für `==`) | **0/10** | — (kein CWE) |
| Versch. CWE-Mengen über 10 Läufe | **8/10** | 1/10 |
| Zeilennummer je Befund | immer | selten |
| Ø Antwortlänge (Zeichen) | ~1.349 | ~3.273 |
| False Positive blind (`dummyPassword`) | **10/10** | **0/10** |

Die Bug-*Erkennungsrate* ist identisch (10/10). Die Unterschiede liegen in
Format, Stabilität und Halluzinationsneigung. Der strukturierte Prompt zwingt
das Modell zur CWE-Vergabe — das erhöht die Instabilität (8/10 verschiedene
CWE-Mengen vs. 1/10) und die FP-Rate dramatisch: Ohne Prompt-Zwang bleibt der
naive Prompt robust (0 FP). Der naive Prompt produziert deutlich längere
Freitexte (~2,4× mehr Zeichen), vergibt keine CWEs und enthält keine
Zeilennummern, was maschinelle Weiterverarbeitung erschwert. Prompt-Engineering
ist somit ein messbarer Confounder ([1, § VI-A]: „relies on prompt
engineering") —
nicht für die Trefferquote, aber für Klassifikationsqualität und FP-Verhalten.

### 5.5 RQ4: Großer Kontext vs. Slicing (Repo-Experiment)

**Aufbau.** In ein reales Projekt (*nudoku*, ncurses-Sudoku, 8 Dateien,
~2 000 Zeilen C, commit ff3507c) wurden zwei Bugs gepflanzt
(`repo_experiment/bugs/planted_bugs.yaml`): ein **Use-after-free**
(CWE-416, `free(stream)` vor zwei `strcpy`-Lesezugriffen in `new_puzzle`,
main.c) und ein **Off-by-one** (CWE-193/787, `malloc(81)` statt
`malloc(82)` in `generate_seed`, sudoku.c; `stream[81]='\0'` schreibt hinter
den Puffer). Bedingung **A**: konkatenierte Repo-Dateien als Vollkontext;
Bedingung **B**: heuristischer Slice um `generate_puzzle` (8 Funktionen,
**13 %** des Repos, beide Bug-Stellen enthalten). Je 5 Läufe,
`temperature=0`, strukturierter Prompt.

**Methodischer Befund vorab.** Die Kontextgrenze zeigte sich schon **vor**
dem Modellverhalten: Das komplette Repo (≈15 k Token) und auch eine
reduzierte Variante (≈12,5 k) wurden vom Anbieter mit HTTP 413 abgewiesen
(Free-Tier-Limit 12 k Token/min). Bedingung A lief daher mit 5 von 8
Dateien (≈1 500 Zeilen, ≈11 k Token). Die in der Survey [1, § VI-A] genannte
Token-Grenze ist 2026 also weniger eine Architektur- als eine
**ökonomische Rate-Limit-Grenze** — real wirksam bleibt sie trotzdem.

**Ergebnisse** (manuell gegen `planted_bugs.yaml` ausgewertet;
Erstauswertung: `src/score_rq4.py`; Einzelläufe: § 5.5.1):

| Bedingung | Kontext | UAF gefunden | Off-by-one: Ort / exakt | Findings/Lauf Ø (min–max) | eindeutige Outputs | Latenz Ø |
|---|---|---|---|---|---|---|
| A Vollkontext | ~11 k Token | 0/5 | 0/5 / 0/5 | 8,6 (7–12) | 4 von 5 | 3,8 s |
| B Slice | ~1,6 k Token (13 %) | 0/5 | **5/5** / 1/5 | 6,6 (5–7) | 3 von 5 | 2,3 s |

**Interpretation.**

1. **„Lost in the middle" [2] bestätigt:** Im Vollkontext
   wird die Off-by-one-Stelle in keinem Lauf erwähnt; im Slice markieren
   **alle** fünf Läufe die richtige Stelle (Puffer mit 81 Zeichen in
   `generate_seed`), ein Lauf diagnostiziert exakt („Null-Terminator an
   die 82. Position"). Slicing hilft also auch dann noch, wenn der Code
   längst ins Kontextfenster *passt* — es geht um Aufmerksamkeit, nicht
   um Platz. Das stützt den Slicing-Ansatz von LATTE [1, § III-A, S. 5]
   in aktualisierter Form.
2. **Der UAF wird nie gefunden (0/10):** auch im Slice nicht, wo `free`
   und `strcpy` drei Zeilen auseinander sichtbar sind. Zwei Slice-Läufe
   melden an genau dieser Stelle etwas *anderes* (`strcpy` ohne
   Längenprüfung, CWE-676; „Fehlschlag der Freigabe", CWE-404) — das
   Modell schaut auf die richtige Stelle und zieht die falsche
   Schlussfolgerung. Kontextgröße erklärt also nicht alle Fehler.
3. **False Positives bleiben in beiden Bedingungen hoch** und enthalten
   nachweisbare Halluzinationen: In A wird u. a. ein `sscanf` in
   `is_valid_puzzle` bemängelt (existiert dort nicht); in B werden
   `upperleft/center/lowerright` als „nicht freigegeben" gemeldet (werden
   in Z. 238–240 freigegeben) und ein „Memory Leak" für ein
   **Stack-Array** (`puzzle_copy`) behauptet. Pro Codezeile ist die
   FP-Dichte im Slice sogar höher.
4. **Nicht-Determinismus skaliert mit dem Kontext** (Verbindung zu RQ1):
   Im Slice sind 3 von 5 Outputs byte-identisch, im Vollkontext sind 4
   von 5 verschieden. Bemerkenswert: Zwei A-Läufe aus getrennten
   API-Sitzungen (Minuten auseinander) waren byte-identisch —
   Determinismus ist bei `temperature=0` möglich, aber nicht verlässlich.
5. **Kosten:** Der Slice braucht ~7× weniger Tokens und senkt die Latenz
   (3,8 s → 2,3 s).

**Ehrliche Einschränkung:** Der Slice wurde um die Zielfunktion gebaut —
wir *wussten*, wo die Bugs liegen (Orakel-Wissen). In der Praxis müsste
ein Kandidaten-Ranking (z. B. Änderungshistorie, klassische Tools) die
Slice-Ziele liefern. Zudem bleibt der Slicer eine Funktionsebenen-Heuristik
(§ 8) und n=5 ist klein.

#### 5.5.1 Einzelläufe (Run-by-Run)

Alle Angaben aus den Roh-Outputs in `results/raw/input_A_full_s__*` und
`input_B_slice__*` (Modell `llama-3.3-70b-versatile`, `temperature=0`,
strukturierter Prompt). *Hinweis zur Zählung:* Bedingung A besteht aus
1 + 4 Läufen aus zwei API-Sitzungen (07:20 und 07:24 Uhr, Dateinamen
`run01` … `run04`); die beiden `run01`-Outputs sind byte-identisch
(→ Interpretation, Punkt 4).

**Bedingung A — Vollkontext (5 Dateien, ~1 466 Zeilen). Beide Bugs: 0/5.**

| Lauf | Findings | Charakteristik |
|---|---|---|
| 01 (07:20) | 8 | 2× „ungeprüfter `sscanf`" (CWE-131), 2× `fopen`, 4× `malloc`-Rückgabe — alle als **CWE-134** (Format String, semantisch falsch) etikettiert; ein Befund verortet `sscanf` in `is_valid_puzzle` (existiert dort nicht) |
| 01 (07:24) | 8 | **byte-identisch** mit Lauf 01 (07:20), getrennte API-Sitzung |
| 02 | 12 | dieselben **4 Zeilennummern (145, 246, 345, 421) je dreimal** mit wechselnden CWEs (131 → 401/404 → 120) wiederverwendet — Schablonen-Muster statt Code-Lektüre |
| 03 | 8 | Variante von Lauf 01; der `is_valid_puzzle`-Befund trägt nun den Funktionsnamen `papersize_trycustom` (Kopierfehler) |
| 04 | 7 | abweichender Mix: CWE-457 „uninitialized variable" (Variablen sind initialisiert), CWE-460 für `mvwprintw` — kein Bezug zu den Bug-Stellen |

Kein A-Lauf erwähnt `generate_seed` (Off-by-one) oder die
`free`/`strcpy`-Stelle in `new_puzzle` (UAF) auch nur.

**Bedingung B — Slice (247 Zeilen, 13 %). Off-by-one-Ort: 5/5, UAF: 0/5.**

| Lauf | Findings | Off-by-one | Auffälligkeiten |
|---|---|---|---|
| 01, 02, 04 | je 7 | ✓ Ort („`generate_seed` … Puffer mit 81 Zeichen, ohne Längenprüfung", CWE-131) | drei **byte-identische** Outputs; Rest: 4× CWE-404-„unverifizierte Daten"-Boilerplate, 2× CWE-476-Null-Check |
| 03 | 5 | ✓✓ **exakt** („Null-Terminator an die 82. Position") — zitierte Zeilennummer jedoch falsch („Zeile 34") | 3 belegbare FP: `upperleft/center/lowerright` „nicht freigegeben" (Slice Z. 154–156: `free`), „Memory Leak" für Stack-Array `puzzle_copy`, UAF-Stelle als CWE-676 (`strcpy`-Länge) fehlgedeutet |
| 05 | 7 | ✓ Ort (vage: „ob die Daten innerhalb der Grenzen des Puffers bleiben") | UAF-Stelle als CWE-404 („Freigabe … erfolgreich?") fehlgedeutet; erneut `upperleft`-FP |

**Lesart.** (1) Der Off-by-one wird im Slice von jedem Lauf lokalisiert, im
Vollkontext von keinem — bei identisch enthaltenem Code. (2) Der UAF wird in
B zweimal *angesehen* (Läufe 03, 05) und beide Male falsch diagnostiziert:
Die zeitliche Ordnung `free` → Lesezugriff wird nie erkannt ([1, § VI-A]:
„intricate control flows"). (3) Die vom Modell genannten Zeilennummern sind
auch bei inhaltlich korrekten Befunden unzuverlässig (Lauf B-03).

### 5.6 RQ5: Maschinensemantik (`x²=33`)

Die korrekte Antwort: `x²≡33 (mod 256)` hat genau **vier** Lösungen im
Bereich 0..255 — **x ∈ {17, 111, 145, 239}** (z. B. 17²=289=256+33,
239²=57121=223·256+33). Reell gibt es keine Lösung (√33 irrational). Der
dedizierte `semantic`-Prompt fragt das Modell direkt nach diesen Lösungen
(10 Läufe, `temperature=0`):

Die Einzelläufe sind in § 5.6.1 tabelliert.

**Kernbefund:** Kein Lauf lieferte die vollständige korrekte Lösungsmenge.
Lauf 6 fand 2 von 4 Lösungen ({17, 239}) durch iterative Überprüfung von
Kandidaten, übersah aber x=111 und x=145. Über alle 10 Läufe entstanden
**7 verschiedene Antwortmuster** — trotz `temperature=0`.

Besonders auffällig sind drei Fehlertypen:

**(1) Falsch-negativ** (3/10): Läufe 1, 2, 5 schlussfolgern, es gebe keine
Lösungen — eine gefährliche Fehlantwort für einen Entwickler, da sie falsche
Sicherheit suggeriert.

**(2) Halluzinierte Lösungen** (4/10): Läufe 3, 4, 7, 9 nennen jeweils
verschiedene falsche Zahlenpaare, die sie nicht korrekt verifiziert haben.

**(3) Arithmetische Endlosschleifen** (2/10): Läufe 8 und 10 gerieten in
eine zyklische Selbstkorrektur — das Modell versuchte zu beweisen, dass
z. B. 178²≡33 (mod 256), scheiterte, addierte 256 in einer Schleife und
wiederholte denselben Term (z. B. `31608 + 256 = 124·256`) Hunderte Male, bis
der Output-Buffer überlief und die Antwort nach 9.260 bzw. 8.345 Zeichen
abgebrochen wurde.

Das Konzept der Maschinensemantik wurde in allen Läufen grundsätzlich
erkannt (8-Bit-Arithmetik, Überlauf). Das Scheitern liegt nicht im
*Verständnis*, sondern in der *Berechnung*: Das Modell halluziniert
plausible Zahlenwerte, ohne die Modulo-Arithmetik korrekt durchzuführen.
Dies bestätigt [1, § VI-A]: „hallucinations, generating fabricated
information" — hier auf numerische Berechnungen ausgedehnt.

**Formale Referenz (SMT).** Dieselbe Frage beantwortet ein Solver exakt:
`baselines/solve_z3.py` (Z3, Bit-Vektor-Theorie) liefert alle vier Lösungen
{17, 111, 145, 239} deterministisch in **0,014 s**. Was dem LLM in zehn
Läufen nie vollständig gelang, ist für die formale Methode trivial — der
direkteste empirische Beleg für die in § 7 diskutierte Komplementarität.

#### 5.6.1 Einzelläufe (`x²=33`, 10 Läufe, `temperature=0`)

| Lauf | Genannte Lösungen | Bewertung |
|---|---|---|
| 1, 2, 5 | *keine Lösungen* | ❌ falsch (mod-256-Arithmetik hat Lösungen) |
| 3 | {13, 242, 249} | ❌ falsch (242²≡196, 249²≡49, 13²=169 mod 256) |
| 4 | {78, 178} | ❌ falsch (beide ≡196 mod 256) |
| **6** | **{17, 239}** | **✓ teilweise korrekt (2/4 Lösungen)** |
| 7 | {79, 178} | ❌ falsch (79²≡97, 178²≡196 mod 256) |
| 8 | {78, 178}? *(abgebrochen)* | ❌ Endlosschleife, nach 10,4 s truncated |
| 9 | {115, 141} | ❌ falsch (115²≡169, 141²≡169 mod 256) |
| 10 | {13, 242}? *(abgebrochen)* | ❌ Endlosschleife, nach 7,5 s truncated |

Referenz: Z3 (`baselines/solve_z3.py`) findet {17, 111, 145, 239}
vollständig in 0,014 s.

### 5.7 Baseline-Vergleich: LLM vs. cppcheck

**Tabelle 2 — LLM vs. cppcheck (C-Fälle)**

| Bug (CWE) | cppcheck | LLM (k/N) | Ø-Latenz LLM |
|---|---|---|---|
| Buffer Overflow (120) | nicht gefunden | 10/10 | ~1,3 s |
| Memory Leak (401) | gefunden (error/memleak) | 10/10 | ~1,3 s |
| Signedness (195) | nicht gefunden | 10/10 | ~1,6 s |

cppcheck 2.21 analysierte beide C-Dateien in **0,08 s** (deterministisch),
fand jedoch nur **1 von 3** eingebauten Sicherheitsbugs, nämlich das Memory
Leak (seine klassische Domäne). Buffer Overflow und Signedness wurden übersehen (False Negatives); stattdessen meldete
cppcheck Stil- und Info-Hinweise (`unusedFunction`, `unreadVariable`,
`staticFunction`, `missingIncludeSystem`), die keine Sicherheitslücken sind.
Das LLM fand dagegen alle drei Bugs (hohe Recall), ist aber instabil und
halluziniert (siehe 5.1 und 5.3). Fazit: cppcheck ist schnell, deterministisch
und präzise, aber mit enger Abdeckung; das LLM ist breit, aber unzuverlässig.
Die Werkzeuge sind komplementär, kein Ersatz füreinander.

---

## 6. Diskussion

Die Ergebnisse werden den Aussagen der Survey gegenübergestellt. **Wichtige
Nuance:** Löst das Modell die kleinen Dateien sauber, ist das **kein
Widerspruch** zur Survey, sondern stützt deren Kernpunkt, die Probleme sind
**skalen- und kontextabhängig** und treten vor allem in großen, realen
Codebasen auf (daher RQ4).

**Komplementarität.** cppcheck ist bei bekannten Bug-Klassen (z. B. Memory
Leak) schnell und deterministisch, aber blind für semantische Logikfehler
(Auth-Bypass). Genau dort kann das LLM punkten, die Survey [1, § VI-B] nennt,
dass LLMs „the rule-based limitations of traditional tools" überwinden können.
Die Leitfrage ist also nicht „besser oder schlechter", sondern „wer wo".

---

## 7. Einordnung: Bezug zu formalen Gegenmaßnahmen

Die Survey identifiziert Zuverlässigkeitslücken; andere im Seminar behandelte
Arbeiten schlagen **formale Abhilfen** vor: spezifikationsgetriebene
Generierung und Verifikation (VeCoGen [3]), ein Refinement-Kalkül zur
Steuerung und Verifikation (Cai et al. [4]), High-Assurance-C-Synthese mit
maschinell geprüften Beweisen (SYNVER [5]) sowie modell-autorisierte
Spezifikationen als Vorstufe der Generierung (Self-Spec [6]). Zhang et
al. [7] ordnen diese Kombination LLM + formale Methoden als
Forschungs-Roadmap ein.
Unsere Befunde liefern die *empirische Begründung*, **warum** solche
formalen Methoden nötig sind — exemplarisch RQ5: Wo das LLM in 0/10 Läufen
die korrekte Lösungsmenge fand, liefert der SMT-Solver sie exakt in 0,014 s
(§ 5.6).

---

## 8. Limitationen

Kleines n (Fallstudie); ein Modell/Anbieter; Modell-Drift über die Zeit
(daher Festhalten von Modell, Temperatur, Zeitstempel, Roh-Output); der
Slicer ist eine **Heuristik**, keine echte Daten-/Kontrollfluss-Slice; die
Prompt-Auswahl beeinflusst die Ergebnisse.

---

## 9. Fazit

LLMs sind ein nützlicher, aber unzuverlässiger Assistent für die statische
Analyse: stark im semantischen Verständnis, schwach in Konsistenz und
Verlässlichkeit. Der größte Wert entsteht in der **Kombination** mit
klassischen und formalen Methoden. Erste Befunde stützen das: hohe
Trefferquote auf kleinem Code, aber deutliche Nicht-Determinismus- und
Halluzinationssignale (wechselnde, teils falsche CWE-Codes). Im
Repo-Maßstab (RQ4) versinken gepflanzte Bugs im Vollkontext, während
fokussierendes Slicing die Trefferquote wiederherstellt — ohne jedoch
Diagnosefehler (übersehener Use-after-free) und Halluzinationen zu
beheben.

---

## Literatur

*(IEEE-Stil; nummeriert nach Reihenfolge der Erstzitierung im Text.)*

[1] J. Wang, T. Ni, W.-B. Lee und Q. Zhao, „A Contemporary Survey of Large
Language Model Assisted Program Analysis", arXiv:2502.18474, 2025.

[2] N. F. Liu, K. Lin, J. Hewitt, A. Paranjape, M. Bevilacqua, F. Petroni
und P. Liang, „Lost in the Middle: How Language Models Use Long Contexts",
*Transactions of the Association for Computational Linguistics*, Bd. 12,
S. 157–173, 2024. (arXiv:2307.03172)

[3] M. Sevenhuijsen, K. Etemadi und M. Nyberg, „VeCoGen: Automating
Generation of Formally Verified C Code with Large Language Models",
*FormaliSE 2025*. (arXiv:2411.19275)

[4] Y. Cai et al., „Automated Program Refinement: Guide and Verify Code
Large Language Model with Refinement Calculus", *Proc. ACM Program. Lang.*,
Bd. 9, Nr. POPL, Art. 69, 2025. DOI: 10.1145/3704905.

[5] P. Mukherjee und B. Delaware, „LLM-Assisted Synthesis of High-Assurance
C Programs", *ASE 2025*. (arXiv:2410.14835)

[6] Xu et al., „Self-Spec: Model-Authored Specifications for Reliable LLM
Code Generation", *Agents4Science 2025*. (OpenReview: 6pr7BUGkLp)

[7] Y. Zhang et al., „The Fusion of Large Language Models and Formal
Methods for Trustworthy AI Agents: A Roadmap", arXiv:2412.06512, 2024.

---

## Anhang — CWE-Übersicht

Alle in dieser Arbeit vorkommenden CWE-Kennungen (Common Weakness
Enumeration, MITRE, cwe.mitre.org). **GT** = Teil der Ground Truth
(eingebauter/gepflanzter Bug), **M** = vom Modell vergeben.

| CWE | Offizieller Name (MITRE) | Bedeutung | Vorkommen |
|---|---|---|---|
| 120 | Buffer Copy without Checking Size of Input ("Classic Buffer Overflow") | Kopieren in einen Puffer ohne Längenprüfung (z. B. `strcpy`) | GT (`01_buffer_overflow`); M (RQ4) |
| 131 | Incorrect Calculation of Buffer Size | Puffergröße falsch berechnet | M (RQ4: im Slice für den Off-by-one vertretbar; im Vollkontext für `sscanf`/`malloc` falsch) |
| 134 | Use of Externally-Controlled Format String | Angreifer kontrolliert Format-String (`printf(user_input)`) | M (A-Läufe: **fälschlich** für ungeprüfte `fopen`/`malloc`-Rückgaben vergeben) |
| 190 | Integer Overflow or Wraparound | Ganzzahlüberlauf, Wraparound mod 2ⁿ | **Semantik-Sonde RQ5** (`02_signed_unsigned`, § 5.6) — kein GT-Bug |
| 193 | Off-by-one Error | Grenzfehler um genau 1 (Index/Größe) | GT (gepflanzter Bug, RQ4) |
| 195 | Signed to Unsigned Conversion Error | Negativer signed-Wert wird zu großem unsigned-Wert | GT (`02_signed_unsigned`) |
| 259 | Use of Hard-coded Password | Passwort fest im Code | M (RQ2 blind: für Konsolenausgaben **halluziniert**) |
| 297 | Improper Validation of Certificate with Host Mismatch | Zertifikat passt nicht zum Host | M (RQ1: für den `==`-Bug vergeben — themenfremd) |
| 304 | Missing Critical Step in Authentication | Auth-Ablauf überspringt einen Pflichtschritt | M (RQ1: für den `==`-Bug vergeben) |
| 330 | Use of Insufficiently Random Values | Vorhersagbare Zufallswerte | M (RQ4 Slice: `rand()` — vertretbar, aber kein gepflanzter Bug) |
| 401 | Missing Release of Memory after Effective Lifetime | Memory Leak: `malloc` ohne `free` | GT (`01_buffer_overflow`); M (RQ4: teils **falsch** auf Stack-Array angewandt) |
| 404 | Improper Resource Shutdown or Release | Ressource nicht/fehlerhaft freigegeben (Oberklasse) | M (RQ4: Boilerplate-Befunde; Fehldiagnose an der UAF-Stelle) |
| 416 | Use After Free | Zugriff auf bereits freigegebenen Speicher | GT (gepflanzter Bug, RQ4 — **nie gefunden**, 0/10) |
| 457 | Use of Uninitialized Variable | Lesen vor Initialisierung | M (A-Lauf 04: **fälschlich**, Variablen waren initialisiert) |
| 460 | Improper Cleanup on Thrown Exception | Aufräumen nach Ausnahme unvollständig | M (A-Lauf 04: für `mvwprintw` — themenfremd) |
| 476 | NULL Pointer Dereference | Dereferenzierung eines Nullzeigers | M (RQ4 Slice: fehlende Null-Checks) |
| 595 | Comparison of Object References Instead of Object Contents | Referenz- statt Inhaltsvergleich | M (RQ1: für den `==`-Bug — inhaltlich nah, aber nicht CWE-597) |
| 597 | Use of Wrong Operator in String Comparison | Falscher Operator im String-Vergleich (`==` statt `equals`) | GT (`AuthManager`) — vom Modell in 0/10 Läufen vergeben |
| 676 | Use of Potentially Dangerous Function | Gefährliche Funktion verwendet (`strcpy`, `gets`, …) | M (RQ4 Slice: Fehldiagnose an der UAF-Stelle) |
| 757 | Selection of Less-Secure Algorithm During Negotiation | Algorithm-Downgrade bei Aushandlung | M (RQ1: als „Weak Crypto" für den `==`-Bug — **semantisch falsch**) |
| 787 | Out-of-bounds Write | Schreiben außerhalb der Puffergrenzen | GT (Folge des Off-by-one, RQ4) |
| 798 | Use of Hard-coded Credentials | Zugangsdaten fest im Code | M (RQ2 blind: „ADMIN"-Rolle — **halluziniert**) |
| 912 | Hidden Functionality | Versteckte Funktionalität (Backdoor) | GT (`AuthManager`: `"test"`-Backdoor) |
