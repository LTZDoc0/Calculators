import numpy as np
import matplotlib.pyplot as plt

def main():
    print(" Inserisci i parametri\n")

    # Input utente
    f_start_mhz = float(input("Frequenza iniziale (MHz): "))
    f_stop_mhz = float(input("Frequenza finale (MHz): "))
    vf = float(input("Fattore di velocità (es. 0.66): "))

    print("\n Inserisci l'impedenza del carico (complessa):")
    R = float(input("  Parte reale (Ohm): "))
    X = float(input("  Parte immaginaria (Ohm): "))
    ZL = complex(R, X)

    Z_gen = float(input("Impedenza del generatore o TX (Ohm): "))

    # Frequenze
    f_start = f_start_mhz * 1e6
    f_stop = f_stop_mhz * 1e6
    f_range = np.linspace(f_start, f_stop, 500)
    f_range_mhz = f_range / 1e6
    f_central = (f_start + f_stop) / 2

    # Calcolo Z0 ideale per adattamento λ/4
    Z0_match = np.sqrt(ZL * Z_gen)

    if np.imag(Z0_match) != 0 or np.real(Z0_match) <= 0:
        print("\n Impossibile adattare con linea λ/4: impedenza trasformata non reale o negativa.")
        return

    Z0_match = np.real(Z0_match)

    # Calcolo lunghezza λ/4 alla frequenza centrale
    lambda_central = (3e8 * vf) / f_central
    l_line = lambda_central / 4
    beta = 2 * np.pi / lambda_central

    # ✅ Stampa risultati
    print(f"\n📏 Lunghezza linea λ/4 @ {f_central/1e6:.1f} MHz: {l_line:.4f} m ({l_line*100:.2f} cm)")
    print(f"🔁 Impedenza caratteristica richiesta per adattamento: {Z0_match:.2f} Ohm")

    # Posizione lungo la linea
    z = np.linspace(0, l_line, 500)
    z_eff = l_line - z  # distanza dal carico

    def Zin(Z0, ZL, beta, l):
        return Z0 * (ZL + 1j * Z0 * np.tan(beta * l)) / (Z0 + 1j * ZL * np.tan(beta * l))

    # Zin lungo la linea
    Zin_z = Zin(Z0_match, ZL, beta, z_eff)

    # 🔹 GRAFICO 1: Parte reale e immaginaria dell’impedenza vista dal generatore
    plt.figure(figsize=(10, 6))
    plt.plot(z * 100, np.real(Zin_z), label='Parte Reale ℜ{Zin}')
    plt.plot(z * 100, np.imag(Zin_z), label='Parte Immaginaria ℑ{Zin}')
    plt.xlabel("Distanza dal generatore (cm)")
    plt.ylabel("Impedenza (Ohm)")
    plt.title(f"Impedenza lungo la linea @ {f_central/1e6:.1f} MHz")
    plt.grid(True)
    plt.legend()

    # 🔸 GRAFICO 2: VSWR vs frequenza
    lambda_range = (3e8 * vf) / f_range
    beta_range = 2 * np.pi / lambda_range
    Zin_f = Z0_match * (ZL + 1j * Z0_match * np.tan(beta_range * l_line)) / (Z0_match + 1j * ZL * np.tan(beta_range * l_line))
    gamma_f = (Zin_f - Z_gen) / (Zin_f + Z_gen)
    vswr_f = (1 + np.abs(gamma_f)) / (1 - np.abs(gamma_f))

    plt.figure(figsize=(10, 5))
    plt.plot(f_range_mhz, vswr_f, label='VSWR')
    plt.xlabel("Frequenza (MHz)")
    plt.ylabel("VSWR visto dal generatore")
    plt.title(f"VSWR vs Frequenza (linea = λ/4 @ {f_central/1e6:.1f} MHz)")
    plt.grid(True)
    plt.legend()

    # ✅ Mostra grafici
    plt.show()
    input("\nPremi INVIO per uscire...anzi meglio che resti a casa")

main()
