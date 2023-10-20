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

// Funktion zur Berechnung des Cosinuswertes (ohne externe Bibliotheken)
double cos(double x) {
    // Cosinus kann mithilfe des Sinus berechnet werden
    // cos(x) = sin(π/2 - x)
    return sin(PI / 2 - x);
}

// Funktion zur Berechnung des Tangenswertes (ohne externe Bibliotheken)
double tan(double x) {
    // Tangens kann mithilfe von Sinus und Cosinus berechnet werden
    // tan(x) = sin(x) / cos(x)
    return sin(x) / cos(x);
}

// Hilfsfunktion zur Berechnung der Fakultät
int factorial(int n) {
    if (n == 0) {
        return 1;
    }
    return n * factorial(n - 1);
}
