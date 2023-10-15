#include "../drivers/display.h"
#include "../drivers/keyboard.h"
#include "../drivers/ports.h"
// include "../drivers/graphics.h"
#include "../drivers/video.h"

#include "../stdlibs/string.h"
#include "../kernel/util.h"
#include "../kernel/mem.h"

#include "editor.h"

#define MAX_BUFFER_SIZE 1024

unsigned char text_buffer[MAX_BUFFER_SIZE];
int text_buffer_index = 0;


const unsigned char* sc_name3[] = {"VROOM", "Esc", "1", "2", "3", "4", "5", "6",
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

const unsigned char sc_name8[][12] = {"VROOOM", "Esc", "1", "2", "3", "4", "5", "6",
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


const unsigned char sc_ascii4[] = {'?', '?', '1', '2', '3', '4', '5', '6',
                         '7', '8', '9', '0', '-', '=',
                         0xE1, // 'ß',
                         '?', 'q', 'w', 'e', 'r', 't', 'z',
                         'u', 'i', 'o', 'p', '[', ']', '?', '?', 'a', 's', 'd', 'f', 'g',
                         'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'y', 'x', 'c', 'v',
                         'b', 'n', 'm', ',', '.', '/', '?', '?', '?', ' ', '?', '?', '?',
                         '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
                         '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
                         '^', 'v', '<', '>'};

const unsigned char sc_ascii5[] = {'?', '?', '!', '"',
                         0x15, // '§',
                         '$', '%', '&',
                         '/', '(', ')', '=', '_', '*', '?', '?', 'Q', 'W', 'E', 'R', 'T', 'Z',
                         'U', 'I', 'O', 'P', '[', ']', '?', '?', 'A', 'S', 'D', 'F', 'G',
                         'H', 'J', 'K', 'L', ';', '\'', '`', 0, '\\', 'Y', 'X', 'C', 'V',
                         'B', 'N', 'M', ',', '.', '/', '?', '?', '?', ' ', '?', '?', '?',
                         '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
                         '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
                         '^', 'v', '<', '>'};



void draw_status_bar() {
    set_color(FG_BLACK | BG_CYAN);
    for(unsigned char c=0; c<80; c++) {
        printf(" ");
    }
}

void editor_exit() {
    set_color(FG_WHITE | BG_BLACK);
    clear_screen();
}

void draw_text_buffer() {
    set_color(FG_WHITE | BG_LIGHT_BLUE);
    printf("%s", text_buffer);
}


void insert_character(unsigned char c) {
    if (text_buffer_index < MAX_BUFFER_SIZE - 1) {
        text_buffer[text_buffer_index] = c;
        text_buffer_index++;
        text_buffer[text_buffer_index] = '\0';
        draw_text_buffer();
    }
}

void delete_character() {
    if (text_buffer_index > 0) {
        text_buffer_index--;
        text_buffer[text_buffer_index] = '\0';
        draw_text_buffer();
    }
}

void clear_text_buffer() {
    text_buffer[0] = '\0';
    text_buffer_index = 0;
}


int keycodes() {
    while (1) {
        while (!(read_keyboard_status() & 0x01)) {}
        uint8_t scancode = read_keyboard_data();
        printf("%s\n", sc_name3[scancode]);
        if (scancode < 128) {
            if (scancode == SC_ESC) {

                return 0;
            }
        }
    }
    sleep(33);
}

int editor_main2() {
    while (1) {
        clear_screen();
        draw_status_bar();
        draw_text_buffer();

        while (!(read_keyboard_status() & 0x01)) {}
        uint8_t scancode = read_keyboard_data();

        // Überprüfe den Tastaturstatus
        if (scancode < 128) {
            
            if (scancode == SC_ESC) {
                editor_exit();
                return 0;
            } else if (scancode == SC_F1) {
                clear_text_buffer();
            } else if (scancode == SC_ENTER) {
                insert_character('\n');
            } else if (scancode == SC_BACKSPACE) {
                delete_character();
            } else {
                unsigned char letter;
                if (is_key_pressed(SC_LEFT_SHIFT)) {
                    if(scancode == SC_LEFT_SHIFT) continue;
                    letter = sc_ascii5[(int)scancode];
                    insert_character(letter);
                } else {
                    letter = sc_ascii4[(int)scancode];
                    insert_character(letter);
                }
                
            }
        }
    }
    sleep(100);
}



int hexviewer(uint32_t address) {
    int sector = 0;
    char run = 1;
    unsigned char cx=0;
    unsigned char cy=0;
    unsigned char px=0;
    unsigned char py=0;
    void* cursor_address = (void*)address;
    int cursor;
    while(run) {
        unsigned char key = getkey();
        if (key == 0 || key == 0xE0) continue; 
        switch(key) {
            case SC_PAGEUP : sector--; break; 
            case SC_PAGEDOWN : sector++; break;
            case 72 : if(cy > 0) cy--; break; 
            case 80 : if(cy < 22) cy++; break;
            case 75 : if(cx > 0) cx--; break; 
            case 77 : if(cx < 15) cx++; break;
            case 1: run = 0; break;
            default: {
                if(key < 97) {
                unsigned char letter = sc_ascii4[(int)key];
                key = letter;
                break;
                }
            }
        }

        clear_screen();
        draw_status_bar();
        cursor = get_cursor();
        set_cursor(0);
        cursor_address = ((void*)address+(368*sector))+(cy*16)+cx;
        printf("Cursor: %X, ", cursor_address);
        printf("Char: %c\n", key);
        set_cursor(cursor);

        hexdump(((void*)address+(368*sector)), 368);

        draw_status_bar();
        px = 10+(cx*3);
        py = 1+cy;
        set_cursor_xy(px, py);
    }
    set_color(FG_WHITE | BG_BLACK);
    clear_screen();
}

int editor_main() {
    int cursor_pos = 0;

    while (1) {
        clear_screen();
        draw_status_bar();
        hexdump((void*)0x0, (int)256);

        // Zeige den Cursor als "X" am aktuellen Position an
        set_cursor(cursor_pos * 2 + 1);
        printf("%c", 0x03);

        while (!(read_keyboard_status() & 0x01)) {}
        uint8_t scancode = read_keyboard_data();

        int ch = scancode;
        if (ch == 0 || ch == 0xE0) {
            // Erweiterte Tastaturtaste, z.B. Pfeiltasten
            ch = getkey();
            switch (ch) {
                case 72:  // Pfeiltaste nach oben (Keypad 8)
                    cursor_pos = (cursor_pos - 16) >= 0 ? cursor_pos - 16 : cursor_pos;
                    break;
                case 80:  // Pfeiltaste nach unten (Keypad 2)
                    cursor_pos = (cursor_pos + 16) < text_buffer_index ? cursor_pos + 16 : cursor_pos;
                    break;
                case 75:  // Pfeiltaste nach links (Keypad 4)
                    cursor_pos = (cursor_pos - 1) >= 0 ? cursor_pos - 1 : cursor_pos;
                    break;
                case 77:  // Pfeiltaste nach rechts (Keypad 6)
                    cursor_pos = (cursor_pos + 1) < text_buffer_index ? cursor_pos + 1 : cursor_pos;
                    break;
                // Füge weitere Tastensteuerungen hinzu, falls nötig
                default:
                    break;
            }
        } else if (ch == 27) {
            // ESC-Taste
            editor_exit();
            return 0;
        } else if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F')) {
            // Hexadezimale Eingabe
            if (cursor_pos < text_buffer_index) {
                text_buffer[cursor_pos] = ch;
                cursor_pos++;
                draw_text_buffer();
            }
        } else if (ch == '\b') {
            // Backspace-Taste
            if (cursor_pos > 0) {
                cursor_pos--;
                delete_character();
            }
        } else if (ch == '\r') {
            // Enter-Taste
            insert_character('\n');
            cursor_pos += 2; // Der Cursor wird nach dem Einfügen von '\n' um zwei Positionen verschoben
        } else {
            // Zeichen eingeben
            insert_character(ch);
            cursor_pos++; // Der Cursor wird nach dem Einfügen des Zeichens um eine Position verschoben
        }
    }
}