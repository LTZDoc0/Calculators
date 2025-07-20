import math

def calcola_capacita():
    print("🧮 Calcolo della capacità di un condensatore di livellamento. Era ora!\n")

    try:
        I = float(input("Corrente (A): "))
        f = float(input("Frequenza (Hz): "))
        Vripple = float(input("Ripple ammesso (Vpp): "))
        ESR = float(input("ESR del condensatore (Ohm): "))
    except ValueError:
        print("❌ Valori non validi. Inserisci solo numeri.\n")
        return

    V_esr = I * ESR
    if Vripple <= V_esr:
        print(f"\n❌ Errore: il ripple ammesso ({Vripple} V) è troppo basso rispetto alla caduta sull'ESR ({V_esr:.2f} V).")
        print("   ➜ Aumenta Vripple o riduci ESR.\n")
        return

    C = I / (2 * f * (Vripple - V_esr))  # Farad
    C_uF = C * 1e6

    print("\n✅ Risultato:")
    print(f"→ Capacità richiesta: {C:.6f} F  ({C_uF:.2f} μF)")
    print(f"→ Ripple da ESR: {V_esr:.4f} V")
    print(f"→ Ripple da capacità: {Vripple - V_esr:.4f} V\n")

if __name__ == "__main__":
    while True:
        calcola_capacita()
        risposta = input("Vuoi fare un altro calcolo? (s/n): ").strip().lower()
        if risposta != 's':
            input("\nPremi INVIO per uscire...a cercare figa")
            break
