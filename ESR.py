import pandas as pd
import numpy as np
from fpdf import FPDF

# Dati capacità standard
capacitance_uf = [1, 4.7, 10, 22, 47, 100, 220, 470, 1000, 2200, 4700, 10000, 22000, 47000]
voltage_levels = [16, 25, 50, 100, 160, 250, 300]
frequency = 100  # Hz

# Raccolta dati
data = []

for c in capacitance_uf:
    for v in voltage_levels:
        xc = 1 / (2 * np.pi * frequency * (c * 1e-6))

        if c <= 10:
            esr_ind = round(6 / c + v * 0.0005, 3)
            esr_std = round(10 / c + v * 0.001, 3)
            esr_china = round(20 / c + v * 0.002, 3)
        elif c <= 100:
            esr_ind = round(0.3 / np.sqrt(c) + v * 0.0002, 3)
            esr_std = round(0.6 / np.sqrt(c) + v * 0.0004, 3)
            esr_china = round(1.2 / np.sqrt(c) + v * 0.0008, 3)
        else:
            esr_ind = round(0.03 / np.sqrt(c) + v * 0.0001, 4)
            esr_std = round(0.06 / np.sqrt(c) + v * 0.0002, 4)
            esr_china = round(0.12 / np.sqrt(c) + v * 0.0004, 4)

        data.append([
            c, v, esr_ind, esr_std, esr_china, round(xc, 4)
        ])

# Creazione PDF
pdf = FPDF()
pdf.add_page()
pdf.set_font("Courier", size=8)

# Titolo
pdf.set_font("Courier", 'B', 10)
pdf.cell(200, 10, txt="Tabella ESR e Reattanza - 100 Hz", ln=True, align='C')
pdf.set_font("Courier", size=8)

# Intestazione
headers = ["Cap (uF)", "Volt", "ESR_Ind", "ESR_Std", "ESR_China", "Xc (Ohm)"]
header_line = " | ".join(f"{h:>10}" for h in headers)
pdf.cell(200, 8, header_line, ln=True)

# Riga separatrice
pdf.cell(200, 4, "-" * 75, ln=True)

# Contenuto
for row in data:
    line = " | ".join(f"{str(x):>10}" for x in row)
    pdf.cell(200, 4, txt=line, ln=True)

# Salvataggio
pdf.output("err.pdf")

print("✅ Tabella salvata in 'err.pdf' con successo.")
input("\nPremi INVIO per chiudere...")
