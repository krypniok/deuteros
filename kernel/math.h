

// Definition von Pi (π)
#define PI 3.14159265358979323846
// Makro zur Konvertierung der Frequenz in Hertz in die erforderlichen LSB und MSB Werte
#define FREQUENCY_TO_LSB_MSB(frequency) \
    ((uint16_t)(1193180 / (frequency)))


double sin(double x);



int factorial(int n);