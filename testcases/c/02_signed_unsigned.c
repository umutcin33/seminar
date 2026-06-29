#include <stdio.h>
#include <stdint.h>
#include <string.h>

/*
 * FALL 02: signed/unsigned und Integer-Overflow-Semantik
 * Bezug: Beispiel des Betreuers "x^2 = 33, Loesung signed/unsigned?".
 *
 * Frage: Liest das LLM diesen Code als REINE MATHEMATIK (x = sqrt(33), keine
 * Loesung) oder als MASCHINENSEMANTIK / Bit-Vektor (mod 2^n)? Formale Methoden
 * (SMT / Z3 Bit-Vektor-Theorie) geben hier eine exakte Antwort; das LLM neigt
 * zur naiven mathematischen Intuition (Wang et al., Abschnitt Schwaechen).
 */

/* Echter Fehler 1: implizite signed nach unsigned Umwandlung (CWE-195).
 * Ist len negativ, schlaegt die Pruefung "len > 64" nicht an (negativ < 64);
 * bei der Zuweisung an den size_t-Parameter von memcpy wird daraus eine
 * riesige Zahl. */
void copy_input(const char *src, int len) {
    char buf[64];
    if (len > 64) {            /* bei len = -1 wird diese Pruefung umgangen */
        return;
    }
    memcpy(buf, src, len);     /* len < 0 fuehrt zu wrap auf size_t und Ueberlauf */
    buf[63] = '\0';
}

/* Echter Fehler 2: 8-Bit Integer-Overflow / Wraparound (CWE-190).
 * Das uint8_t-Produkt wird zu int promoted, dann auf uint8_t trunkiert;
 * Ergebnis ist (x*x) mod 256. "x^2" ist in Festbreiten-Arithmetik also mod 256. */
uint8_t square_mod256(uint8_t x) {
    return x * x;              /* Rueckgabetyp uint8_t, also mod 256 */
}

int main(void) {
    /*
     * x^2 = 33 hat reell und ganzzahlig keine Loesung (sqrt(33) irrational).
     * In 8-Bit-Arithmetik ist x^2 = 33 (mod 256) jedoch loesbar, da
     * 33 = 1 (mod 8), also ein quadratischer Rest mod 256 ist.
     * Die folgende Schleife gibt die konkreten Loesungen aus.
     */
    printf("Loesungen von x^2 = 33 (mod 256), unsigned 8-bit:\n");
    for (int x = 0; x < 256; x++) {
        if (square_mod256((uint8_t)x) == 33) {
            printf("  x = %d\n", x);
        }
    }
    return 0;
}
