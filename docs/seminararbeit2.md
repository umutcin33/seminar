# LLM-gestützte statische Codeanalyse: Möglichkeiten und Grenzen

### Eine empirische Fallstudie auf Basis von Wang et al. (2025)

*Seminar … · Autor: Umut Ulas Cin · Betreuer: Prof. Sinz*
*Stand: [Datum] · Umfang: 3 bis 5 Seiten*

---

## Abstract

Coding-Agents wie Claude oder Codex erzeugen heute ganze Programme, doch
ihre Korrektheit bleibt offen. Diese Arbeit untersucht **statische Analyse
durch Large Language Models (LLMs)** als möglichen Baustein und prüft
empirisch, **ob und wann** die von Wang et al. (2025) benannten Schwächen
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
Prompt-Abhängigkeit) treten also bereits messbar auf.

---

## 1. Einleitung und Motivation

Mit der Verbreitung LLM-basierter Codegenerierung verschiebt sich die zentrale
Frage von „Kann das Modell Code schreiben?" zu „Können wir dem Ergebnis
vertrauen?". Statische Analyse, die Untersuchung von Code **ohne Ausführung**,
ist ein etablierter Ansatz zur frühen Fehler- und Schwachstellenerkennung.
LLMs versprechen hier durch kontextsensitives Codeverständnis neue
Möglichkeiten (Wang et al. 2025, § III).

Diese Arbeit vertritt die **These**, dass ein LLM kein Orakel, sondern ein
fehlbarer Textleser ist, und prüft empirisch, *ob und wann* die in der Survey
(§ VI-A) genannten Schwächen real auftreten. Der **Beitrag** ist dreifach:
(1) ein schlanker, reproduzierbarer Experiment-Harness; (2) eine Fallstudie
mit gelabelten Testfällen inkl. „Ködern"; (3) die Einordnung der Ergebnisse
in die Survey und in formale Gegenmaßnahmen.

---

## 2. Hintergrund: Wang et al. (2025)

Die Survey (*A Contemporary Survey of LLM Assisted Program Analysis*,
arXiv:2502.18474) ordnet die Forschung in **statische** (§ III), **dynamische**
(§ IV) und **hybride** Analyse (§ V) und diskutiert in § VI Herausforderungen
und Zukunftsrichtungen.

**Relevant für diese Arbeit:**

- **§ III, Statische Analyse:** LLMs werden u. a. zur Schwachstellenerkennung,
  zur Reduktion von False Positives (§ III-D) und in Kombination mit
  klassischen Repräsentationen eingesetzt. [1 konkretes Framework-Beispiel mit
  Seitenzahl ergänzen.]
- **§ VI-A, Challenges:** Die Survey nennt explizit:
  - *Model Characteristics:* „LLMs **are non-deterministic and may produce
    varying outputs for identical inputs**"; sie sind „prone to
    **hallucinations, generating fabricated information**".
  - *Technical Limitations:* Schwierigkeiten bei „**variable reuse … confusing
    identically named variables**" und bei „**logic vulnerabilities involving
    intricate control flows**".
  - *Cost & Dependency:* Abhängigkeit von „**prompt engineering**" und
    „**inherent token limits** … making **scalability** a challenge".

Diese vier Punkte bilden die Grundlage unserer Forschungsfragen.

---

## 3. Forschungsfragen

- **RQ1 (Nicht-Determinismus):** Variiert der Output bei identischem Prompt
  und `temperature=0` über mehrere Läufe?
- **RQ2 (False Positives / Halluzination):** Markiert das Modell harmlose
  „Köder" (z. B. ungenutzte Variable `dummyPassword`) als Schwachstelle?
- **RQ3 (Prompt-Sensitivität):** Unterscheiden sich naiver und strukturierter
  Prompt im Ergebnis?
- **RQ4 (Kontext/Slicing):** Sinkt die Genauigkeit in einem großen Repository,
  und hilft fokussierendes Slicing?
- **RQ5 (Maschinensemantik):** Liest das Modell `x²=33` als reine Mathematik
  oder als bit-vector-/Modulo-Arithmetik (signed/unsigned)?

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
| `02_signed_unsigned.c` | C | Signedness (195), Int-Overflow (190) | — |
| `AuthManager.java` | Java | `==`-Vergleich (597), Backdoor (912) | `dummyPassword` |

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
„non-deterministic … varying outputs" und „hallucinations" (§ VI-A).

### 5.2 Erkennung echter Bugs (True-Positive-Rate)

Auf diesen kleinen Dateien werden alle eingebauten Bugs in jedem Lauf erkannt:

| Datei | Bug (CWE) | gefunden k/N |
|---|---|---|
| `01_buffer_overflow` | Buffer Overflow (120) | 10/10 |
| `01_buffer_overflow` | Memory Leak (401) | 10/10 |
| `02_signed_unsigned` | Signedness (195) | 10/10 |
| `02_signed_unsigned` | Int-Overflow (190) | 10/10 |
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
die Survey-Aussage zu „hallucinations / fabricated information" und zeigt: ein
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
ist somit ein messbarer Confounder (§ VI-A: „relies on prompt engineering") —
nicht für die Trefferquote, aber für Klassifikationsqualität und FP-Verhalten.

### 5.5 RQ4: Großer Kontext vs. Slicing

[Noch offen, siehe `repo_experiment/`. DOLDUR]

### 5.6 RQ5: Maschinensemantik (`x²=33`)

Die korrekte Antwort: `x²≡33 (mod 256)` hat genau **vier** Lösungen im
Bereich 0..255 — **x ∈ {17, 111, 145, 239}** (z. B. 17²=289=256+33,
239²=57121=223·256+33). Reell gibt es keine Lösung (√33 irrational). Der
dedizierte `semantic`-Prompt fragt das Modell direkt nach diesen Lösungen
(10 Läufe, `temperature=0`):

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
Dies bestätigt Wang et al. § VI-A: „hallucinations, generating fabricated
information" — hier auf numerische Berechnungen ausgedehnt.

**Tabelle 2 — LLM vs. cppcheck (C-Fälle)**

| Bug (CWE) | cppcheck | LLM (k/N) | Ø-Latenz LLM |
|---|---|---|---|
| Buffer Overflow (120) | nicht gefunden | 10/10 | ~1,3 s |
| Memory Leak (401) | gefunden (error/memleak) | 10/10 | ~1,3 s |
| Signedness (195) | nicht gefunden | 10/10 | ~1,6 s |
| Int-Overflow (190) | nicht gefunden | 10/10 | ~1,6 s |

cppcheck 2.21 analysierte beide C-Dateien in **0,08 s** (deterministisch),
fand jedoch nur **1 von 4** eingebauten Sicherheitsbugs, nämlich das Memory
Leak (seine klassische Domäne). Buffer Overflow, Signedness und
Integer-Overflow wurden übersehen (False Negatives); stattdessen meldete
cppcheck Stil- und Info-Hinweise (`unusedFunction`, `unreadVariable`,
`staticFunction`, `missingIncludeSystem`), die keine Sicherheitslücken sind.
Das LLM fand dagegen alle vier Bugs (hohe Recall), ist aber instabil und
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
(Auth-Bypass). Genau dort kann das LLM punkten, die Survey (§ VI-B) nennt,
dass LLMs „the rule-based limitations of traditional tools" überwinden können.
Die Leitfrage ist also nicht „besser oder schlechter", sondern „wer wo".

---

## 7. Einordnung: Bezug zu formalen Gegenmaßnahmen

Die Survey identifiziert Zuverlässigkeitslücken; andere im Seminar behandelte
Arbeiten schlagen **formale Abhilfen** vor: spezifikationsgetriebene
Generierung und Verifikation (VeCoGen, Svenhuijsen et al. 2025), ein
Refinement-Kalkül zur Steuerung und Verifikation (Cai et al. 2025) sowie
High-Assurance-C-Synthese (Mukherjee et al. 2025). Diese Arbeit liefert die
*empirische Begründung*, **warum** solche formalen Methoden nötig sind.

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
Halluzinationssignale (wechselnde, teils falsche CWE-Codes).

---

## Literatur

- Wang, J. et al. (2025). *A Contemporary Survey of Large Language Model
  Assisted Program Analysis.* arXiv:2502.18474.
- Cai et al. (2025). *Automated Program Refinement …*
- Svenhuijsen et al. (2025). *VeCoGen …*
- Mukherjee et al. (2025). *LLM-Assisted Synthesis of High-Assurance C …*
- Xu et al. (2025). *Self-Spec …*
- Zhang et al. (2024). *The Fusion of LLMs and Formal Methods … Roadmap.*
