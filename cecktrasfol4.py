import numpy as np
import matplotlib.pyplot as plt

def main():
    print("Inserisci i parametri della linea e del sistema\n")

    # Input utente
    f_start_mhz = float(input("Frequenza iniziale (MHz): "))
    f_stop_mhz = float(input("Frequenza finale (MHz): "))
    vf = float(input("Fattore di velocità (es. 0.66): "))

    print("\n Inserisci l'impedenza del carico (complessa):")
    R = float(input("  Parte reale (Ohm): "))
    X = float(input("  Parte immaginaria (Ohm): "))
    ZL = complex(R, X)

    Z_gen = float(input("Impedenza del generatore (Ohm): "))
    Z0 = float(input("Impedenza caratteristica della linea (Ohm): "))
    l_cm = float(input("Lunghezza della linea (cm): "))
    l_m = l_cm / 100.0

    # Frequenze minime massime medie e di quanto si scopa
    f_start = f_start_mhz * 1e6
    f_stop = f_stop_mhz * 1e6
    f_range = np.linspace(f_start, f_stop, 500)
    f_range_mhz = f_range / 1e6
    f_central = (f_start + f_stop) / 2

    # Lunghezza d'onda e beta (costante di fase)  a f_media
    lambda_central = (3e8 * vf) / f_central
    beta_central = 2 * np.pi / lambda_central

    # Lunghezza della linea in λ
    l_lambda = l_m / lambda_central

    # Calcolo impedenza vista dal generatore a f_media
    def Zin(Z0, ZL, beta, l):
        return Z0 * (ZL + 1j * Z0 * np.tan(beta * l)) / (Z0 + 1j * ZL * np.tan(beta * l))

    Zin_central = Zin(Z0, ZL, beta_central, l_m)
    gamma_central = (Zin_central - Z_gen) / (Zin_central + Z_gen)
    vswr_central = (1 + np.abs(gamma_central)) / (1 - np.abs(gamma_central))

    # Output principali
    print("\n RISULTATI @ f_media:")
    print(f" Impedenza vista dal generatore: {Zin_central.real:.2f} + j{Zin_central.imag:.2f} Ohm")
    print(f" VSWR visto dal generatore: {vswr_central:.2f}")
    print(f" Lunghezza linea: {l_m:.4f} m ({l_cm:.2f} cm)")
    print(f" In termini di λ: {l_lambda:.4f} λ")

    # 🔹 GRAFICO 1: Impedenza vista dal generatore lungo la linea a f_media
    z_cm = np.linspace(0, l_cm, 500)
    z_m = z_cm / 100
    Zin_along_line = Zin(Z0, ZL, beta_central, z_m)

    plt.figure(figsize=(10, 6))
    plt.plot(z_cm, np.real(Zin_along_line), label='Parte Reale ℜ{Zin}')
    plt.plot(z_cm, np.imag(Zin_along_line), label='Parte Immaginaria ℑ{Zin}')
    plt.xlabel("Lunghezza linea (cm)")
    plt.ylabel("Impedenza vista (Ohm)")
    plt.title(f"Impedenza vista dal generatore @ {f_central/1e6:.1f} MHz")
    plt.grid(True)
    plt.legend()

    # 🔸 GRAFICO 2: VSWR vs frequenza
    lambda_range = (3e8 * vf) / f_range
    beta_range = 2 * np.pi / lambda_range
    Zin_f = Zin(Z0, ZL, beta_range, l_m)
    gamma_f = (Zin_f - Z_gen) / (Zin_f + Z_gen)
    vswr_f = (1 + np.abs(gamma_f)) / (1 - np.abs(gamma_f))

    plt.figure(figsize=(10, 5))
    plt.plot(f_range_mhz, vswr_f, label='VSWR')
    plt.xlabel("Frequenza (MHz)")
    plt.ylabel("VSWR")
    plt.title(f"VSWR vs Frequenza (Lunghezza linea = {l_lambda:.3f} λ)")
    plt.grid(True)
    plt.legend()

    # ✅ Mostra grafici
    plt.show()

    input("\nPremi INVIO per uscire...a trovare figa")

main()
