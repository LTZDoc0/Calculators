def calcola_capacita():
    print(" Calcolo capacità per alimentatore lineare e switching PWM\n")

    try:
        I = float(input("Corrente (A): "))
        f = float(input("Frequenza (Hz): "))
        Vripple = float(input("Ripple ammesso (Vpp): "))
        ESR = float(input("ESR del condensatore (Ohm): "))

        if f > 100:
            tipo = 'pwm'
            print(" Frequenza > 100 Hz → Rilevato alimentatore PWM")
            D = float(input("Duty cycle (0–1): "))
        else:
            tipo = 'lineare'
            print("🔌 Frequenza ≤ 100 Hz → Rilevato alimentatore lineare")
            D = None

    except ValueError:
        print(" Valori non validi. Inserisci solo numeri.")
        return

    V_esr = I * ESR
    V_c = Vripple - V_esr

    if V_c <= 0:
        print(f"\n Errore: Ripple residuo sul condensatore negativo o nullo.")
        print(f"   Ripple totale: {Vripple} V — Caduta ESR: {V_esr:.4f} V")
        return

    if tipo == 'lineare':
        C = I / (2 * f * V_c)
    elif tipo == 'pwm':
        C = (I * D) / (f * V_c)

    C_uF = C * 1e6

    print("\n✅ Risultato:")
    print(f"→ Tipo: {tipo.upper()}")
    print(f"→ Capacità richiesta: {C:.6f} F ({C_uF:.2f} μF)")
    print(f"→ Ripple da ESR: {V_esr:.4f} V")
    print(f"→ Ripple da capacità: {V_c:.4f} V\n")


#  Ciclo principale con pausa finale altrimenti scompare tutto come una figa sotto alcool
if __name__ == "__main__":
    while True:
        calcola_capacita()
        ripeti = input("Vuoi fare un altro calcolo? (s/n): ").strip().lower()
        if ripeti != 's':
            input("\nPremi INVIO per uscire a cercare figa...")
            break
