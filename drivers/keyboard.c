#include <stdbool.h>

#include "keyboard.h"
#include "ports.h"
#include "../cpu/isr.h"
#include "display.h"
#include "../kernel/util.h"
#include "../kernel/kernel.h"

#include "hidden_cmd.h"


#define BACKSPACE 0x0E
#define ENTER 0x1C

#define SC_MAX 57

#define KEY_STATUS_MAX 128

static bool key_status[KEY_STATUS_MAX];
static bool extended_scancode = false;

const char *sc_name[] = {"ERROR", "Esc", "1", "2", "3", "4", "5", "6",
                         "7", "8", "9", "0", "-", "=", "Backspace", "Tab", "Q", "W", "E",
                         "R", "T", "Z", "U", "I", "O", "P", "[", "]", "Enter", "Lctrl",
                         "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "`",
                         "LShift", "\\", "Y", "X", "C", "V", "B", "N", "M", ",", ".",
                         "/", "RShift", "Keypad *", "LAlt", "Spacebar", "CapsLock",
                         "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
                         "NumLock", "ScrollLock", "Keypad 7", "Keypad 8", "Keypad 9",
                         "Keypad -", "Keypad 4", "Keypad 5", "Keypad 6", "Keypad +",
                         "Keypad 1", "Keypad 2", "Keypad 3", "Keypad 0", "Keypad .",
                         "AltSysReq", "???", "???", "F11", "F12",
                         "UP_ARROW", "DOWN_ARROW", "LEFT_ARROW", "RIGHT_ARROW"};

const char sc_ascii[] = {'?', '?', '1', '2', '3', '4', '5', '6',
                         '7', '8', '9', '0', '-', '=', '\b', '?', 'q', 'w', 'e', 'r', 't', 'z',
                         'u', 'i', 'o', 'p', '[', ']', '\n', '?', 'a', 's', 'd', 'f', 'g',
                         'h', 'j', 'k', 'l', ';', '\'', '`', '?', '\\', 'y', 'x', 'c', 'v',
                         'b', 'n', 'm', ',', '.', '/', '?', '?', '?', ' ', '?', '?', '?',
                         '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
                         '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
                         '^', 'v', '<', '>'};

const char sc_ascii_upper[] = {'?', '?', '!', '"', 0x15, '$', '%', '&',
                         '/', '(', ')', '=', '_', '*', '?', '?', 'Q', 'W', 'E', 'R', 'T', 'Z',
                         'U', 'I', 'O', 'P', '[', ']', '?', '?', 'A', 'S', 'D', 'F', 'G',
                         'H', 'J', 'K', 'L', ';', '\'', '`', '?', '\\', 'Y', 'X', 'C', 'V',
                         'B', 'N', 'M', ',', '.', '/', '?', '?', '?', ' ', '?', '?', '?',
                         '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
                         '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
                         '^', 'v', '<', '>'};


static void keyboard_callback(registers_t *regs) {
    uint8_t scancode = port_byte_in(0x60);

    // Check for extended scancode prefix
    if (scancode == 0xE0) {
        extended_scancode = true;
        return;
    }

    // Handle key release event
    if (scancode >= 0x80) {
        scancode -= 0x80;
        key_status[scancode] = false;
        return;
    }

    // Handle key press event
    if (scancode < KEY_STATUS_MAX) {
        key_status[scancode] = true;

        // Check if the scancode is extended
        if (extended_scancode) {
            // Handle extended scancode here, if needed
            extended_scancode = false; // Reset extended scancode flag

        } else {
            // Handle regular scancode here
        }
    }
}


void init_keyboard() {
    register_interrupt_handler(IRQ1, keyboard_callback);
}

bool is_key_pressed(unsigned char scancode) {
    if (scancode < KEY_STATUS_MAX) {
        return key_status[scancode];
    }
    return false;
}

unsigned char getkey() {
    sleep(33);
    while ( !(read_keyboard_status() & 0x01)) { }
    uint8_t scancode = read_keyboard_data();
    return scancode;
}

// Warten auf einen Tastendruck und Rückgabe des ASCII-Werts
char getch() {
    while (1) {
        uint8_t scancode = getkey();
        
        // Überprüfen, ob die Taste gedrückt wurde (Bit 0 ist gesetzt)
        if (scancode & 0x80) {
            // Taste wurde losgelassen, ignorieren und weiter warten
        } else {
            // Taste wurde gedrückt, gibt den ASCII-Wert zurück
            if (scancode < sizeof(sc_ascii)) {
                return sc_ascii[scancode];
            } else {
                // Wenn der Scancode nicht im Array ist, gib ein Fragezeichen zurück
                return '?';
            }
        }
    }
}

// Lesen einer Zeichenfolge (bis Enter) von der Tastatur
void gets(char *buffer, int buffer_size) {
    int index = 0;
    
    while (1) {
        char c = getch();
        printf("%c", c);
        
        if (c == '\n') {
            // Enter-Taste wurde gedrückt, beende die Zeichenfolgeneingabe
            buffer[index] = '\0';
            return;
        } else if (c == '\b' && index > 0) {
            // Backspace-Taste wurde gedrückt, lösche das letzte Zeichen im Puffer
            index--;
            buffer[index] = '\0';
        } else if (c >= ' ' && index < (buffer_size - 1)) {
            // Füge das Zeichen dem Puffer hinzu, wenn es druckbar ist und der Puffer nicht voll ist
            buffer[index] = c;
            index++;
            buffer[index] = '\0';
        }
    }
}
