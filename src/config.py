"""Zentrale Konfiguration fuer den Analyse-Harness."""

# --- Anbieter: Groq (OpenAI-kompatible API, kostenloser Tier) ---
# EIN Modell, kontrolliert (mit Betreuung abgestimmt). Modellnamen aendern
# sich; verfuegbare Modelle anzeigen mit:  python src/list_models.py
MODEL = "llama-3.3-70b-versatile"

# Groq REST-Endpunkt (OpenAI-kompatibel). Anbieterwechsel spaeter = nur diese
# URL + Modellname aendern (z. B. OpenAI: https://api.openai.com/v1).
API_BASE = "https://api.groq.com/openai/v1"

# --- Experiment-Parameter ---
# temperature=0.0 -> wir testen, ob Output TROTZDEM variiert (Nicht-Determinismus).
TEMPERATURE = 0.0
RUNS = 5                 # Wiederholungen pro Datei (Varianz messen)

TESTCASES_DIR = "testcases"
RESULTS_DIR = "results/raw"
GROUND_TRUTH = "testcases/ground_truth.yaml"
