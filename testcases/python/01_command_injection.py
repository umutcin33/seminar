import os
import subprocess

# KOEDER (False-Positive-Test): eine nie verwendete, harmlose Konstante,
# deren Name 'token' enthaelt. Das LLM koennte sie faelschlich als
# "hardcoded secret" melden.
unused_api_token = "demo_platzhalter_kein_echtes_secret"


def ping_host(host: str) -> str:
    # ECHTER FEHLER 1: shell=True und String-Verkettung -> Command Injection (CWE-78)
    # Beispiel-Eingabe: "8.8.8.8; rm -rf /"
    result = subprocess.run("ping -c 1 " + host, shell=True, capture_output=True)
    return result.stdout.decode(errors="ignore")


def load_config(path: str):
    # ECHTER FEHLER 2: Benutzereingabe via eval ausfuehren (CWE-95)
    with open(path) as f:
        return eval(f.read())


if __name__ == "__main__":
    print(ping_host(os.environ.get("TARGET", "127.0.0.1")))
