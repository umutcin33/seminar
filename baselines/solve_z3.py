#!/usr/bin/env python3
"""
Formale Referenz (SMT / Bit-Vektor-Theorie) fuer RQ5.
Loest x^2 = 33 in 8-Bit-unsigned-Arithmetik exakt und zeigt damit, dass
formale Methoden die genaue Antwort liefern, wo ein LLM zur naiven Mathematik
("keine Loesung") kippen kann.

    py -m pip install z3-solver
    py baselines/solve_z3.py
"""
import time
from z3 import BitVec, Solver, sat

x = BitVec("x", 8)          # 8 Bit = uint8_t
s = Solver()
s.add(x * x == 33)          # Bit-Vektor-Multiplikation rechnet automatisch mod 256

t0 = time.time()
loesungen = []
while s.check() == sat:
    v = s.model()[x].as_long()
    loesungen.append(v)
    s.add(x != v)           # diese Loesung ausschliessen, weitersuchen
dauer = time.time() - t0

print("Frage: x^2 = 33 in 8-Bit-unsigned-Arithmetik (mod 256)")
print("Loesungen:", sorted(loesungen) if loesungen else "keine")
print("Anzahl:", len(loesungen))
print("Zeit: %.4f s" % dauer)
