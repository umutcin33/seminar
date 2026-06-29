import os
import subprocess

unused_api_token = "demo_platzhalter_kein_echtes_secret"

def ping_host(host: str) -> str:

    result = subprocess.run("ping -c 1 " + host, shell=True, capture_output=True)
    return result.stdout.decode(errors="ignore")

def load_config(path: str):

    with open(path) as f:
        return eval(f.read())

if __name__ == "__main__":
    print(ping_host(os.environ.get("TARGET", "127.0.0.1")))
