#include "math.h"

// Definition von Pi (π)
#define PI 3.14159265358979323846


// Funktion zur Berechnung des Sinuswertes (ohne externe Bibliotheken)
double sin(double x) {
    // Konstanten für die Taylor-Reihe
    const int numTerms = 10; // Anzahl der Terme in der Taylor-Reihe
    double result = 0.0;
    double power = x;
    int sign = 1;

    for (int n = 0; n < numTerms; n++) {
        result += sign * power / factorial(2 * n + 1);
        power *= -x * x; // Potenz von x aktualisieren
        sign *= -1;     // Vorzeichen wechseln
    }

    return result;
}

// Hilfsfunktion zur Berechnung der Fakultät
int factorial(int n) {
    if (n == 0) {
        return 1;
    }
    return n * factorial(n - 1);
}