#include "../cpu/idt.h"
#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../drivers/display.h"
#include "../drivers/keyboard.h"
#include "../drivers/ports.h"
#include "../drivers/video.h"
#include "../stdlibs/string.h"

#include "util.h"
#include "mem.h"




#define SHIFT 0x2A
#define RELEASED_SHIFT 0xAA

#define BACKSPACE 0x0E
#define ENTER 0x1C

#define SC_MAX 128

static char key_buffer2[256];

bool g_bShiftPressed = false;
bool g_bKernelShouldStop = false;
bool g_bKernelInitialised = false;


// Funktion zum Lesen des Tastaturstatus
uint8_t read_keyboard_status() {
    return port_byte_in(0x64);
}

// Funktion zum Lesen des Tastaturdatenports
uint8_t read_keyboard_data() {
    return port_byte_in(0x60);
}

int kernel_console_program();



const unsigned char *sc_name2[] = {"ERROR", "Esc", "1", "2", "3", "4", "5", "6",
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


const unsigned char sc_ascii2[] = {'?', '?', '1', '2', '3', '4', '5', '6',
                         '7', '8', '9', '0', '-', '=',
                         0xE1, // 'ß',
                         '?', 'q', 'w', 'e', 'r', 't', 'z',
                         'u', 'i', 'o', 'p', '[', ']', '?', '?', 'a', 's', 'd', 'f', 'g',
                         'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'y', 'x', 'c', 'v',
                         'b', 'n', 'm', ',', '.', '/', '?', '?', '?', ' ', '?', '?', '?',
                         '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
                         '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
                         '^', 'v', '<', '>'};

const unsigned char sc_ascii3[] = {'?', '?', '!', '"',
                         0x15, // '§',
                         '$', '%', '&',
                         '/', '(', ')', '=', '_', '*', '?', '?', 'Q', 'W', 'E', 'R', 'T', 'Z',
                         'U', 'I', 'O', 'P', '[', ']', '?', '?', 'A', 'S', 'D', 'F', 'G',
                         'H', 'J', 'K', 'L', ';', '\'', '`', 0, '\\', 'Y', 'X', 'C', 'V',
                         'B', 'N', 'M', ',', '.', '/', '?', '?', '?', ' ', '?', '?', '?',
                         '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
                         '?', '?', '?', '?', '?', '?', '?', '?', '?', '?',
                         '^', 'v', '<', '>'};



#include "../cpu/jmpbuf.h"
jmp_buf kernel_env;

void start_kernel() {
    setjmp(&kernel_env);
    set_color(WHITE_ON_BLACK);
    clear_screen();
    printf("start_kernel @ 0x%p\n", &start_kernel);
    unsigned char str[80];
    int revnum = REVISION_NUMBER;
    unsigned char* revdate = REVISION_DATE;
    sprintf(str, "DeuterOS 0.%d (%s)\n", (void*)&revnum, (void*)&revdate);
    printf("DeuterOS 0.%d (", REVISION_NUMBER);
    print_string(REVISION_DATE);
    printf(")\n");

    if(! g_bKernelInitialised) {
       // print_string("Installing interrupt service routines (ISRs).\n");
        isr_install();

      //  print_string("Enabling external interrupts.\n");
        asm volatile("sti");

      //  print_string("Initializing keyboard (IRQ 1).\n");
        init_keyboard();

      //  print_string("Initializing dynamic memory.\n");
        init_dynamic_mem();

        init_memory();

     //   print_string("A20 Line was activated by the MBR.\n");
        // enable_a20_line();

     //   print_string("Initializing timer.\n");
        init_timer(1000);

     //   print_string("Initializing PS/2 mouse interface\n");;
        //mouse_install();

        g_bKernelInitialised = true;
    }

   // printlogo();

    printf("\n%c ", 0x10);

    while(!g_bKernelShouldStop) {
        kernel_console_program();
    }

end_of_kernel:
    print_nl();
    printf("Wow, we should get here...\nExiting...\nStopping the CPU...\nescape espace...\n");
    asm volatile("hlt");
    printf("P.S. Why is this still working when the CPU is officially stopped (hlt) ?\n");
}

typedef void (*FilePointer)();


// Helper function to search for a byte within a memory range
void* search_byte(void* start_address, size_t size, unsigned char byteToFind) {
    for (size_t i = 0; i < size; i++) {
        unsigned char byte = *((unsigned char*)(start_address + i));
        if (byte == byteToFind) {
            // Byte found, return the address
            return (start_address + i);
        }
    }

    // Byte not found in the given range
    return NULL;
}

void execute_search_command_str(char *input) {
    char* addressStr = strtok(input + strlen("searchs") + 1, " ");
    char* sizeStr = strtok(NULL, " ");
    char* searchString = strtok(NULL, " ");

    if (addressStr != NULL && sizeStr != NULL && searchString != NULL) {
        uint32_t address = strtoul(addressStr, NULL, 0);
        uint32_t size = strtoul(sizeStr, NULL, 0);

        // Call the search_string function to find the string
        void* result = search_string((void*)address, size, searchString);

        if (result != NULL) {
            // String found, print the address
            char resultAddressStr[20];
            sprintf(resultAddressStr, "%p", &result);
            print_string("String found at address: ");
            print_string(resultAddressStr);
            print_string("\n");
        } else {
            print_string("String not found.\n");
        }
    } else {
        print_string("Invalid parameters for 'search address size string' command.\n");
    }
}

void searchb(uint32_t address, uint32_t size, uint32_t byte) {
        printf("Searching %d\n", byte);
        void* result = search_byte((void*)address, size, byte);
        if (result != NULL) {
            char resultAddressStr[20];
            sprintf(resultAddressStr, "%p", &result);
            print_string("Byte found at address: ");
            print_string(resultAddressStr);
            print_string("\n");
        } else {
            print_string("Byte not found.\n");
        }
}


void printascii() {
    int rows = 16;
    int columns = 16;
    printf("%c", 0xC9);
    printf("%c", 0xCD);
    printf("%c", 0xCD);
    printf("%c", 0xCD);
    printf("%c", 0xCD);
    printf("%c", 0xD1);

    for (int col = 0; col < columns<<2; col++) {
        printf("%c", 0xCD);
    }
    printf("%c\n", 0xBB);

    printf("%c", 0xBA);
    printf("    ");
    printf("%c", 0xB3);
    
    for (int col = 0; col < columns; col++) {
        printf(" ");
        printHexByte(col);
    }
    printf("%c\n", 0xBA);

    printf("%c", 0xC7);
    printf("%c", 0xC4);
    printf("%c", 0xC4);
    printf("%c", 0xC4);
    printf("%c", 0xC4);
    printf("%c", 0xC5);

    for (int col = 0; col < columns<<2; col++) {
        printf("%c", 0xC4);
    }
    printf("%c\n", 0xB6);

    for (int row = 0; row < rows; row++) {
        printf("%c ", 0xBA);
        printHexByte(row<<4);
        printf("%c", 0xB3);

        for (int col = 0; col < columns; col++) {
            unsigned char c = row * columns + col;
            if (c == 0 || c == 10) // Skip the newline character '\n'
                printf("    ");
            else
                printf("  %c ", c);
        }
        printf("%c\n", 0xBA);
    }

    printf("%c", 0xC8);
    printf("%c", 0xCD);
    printf("%c", 0xCD);
    printf("%c", 0xCD);
    printf("%c", 0xCD);
    printf("%c", 0xCF);

    for (int col = 0; col < columns<<2; col++) {
        printf("%c", 0xCD);
    }
    printf("%c\n", 0xBC);
}

void formatTimestamp(unsigned int timestamp_ms) {
    unsigned int ms_per_day = 86400000; // Millisekunden pro Tag
    unsigned int ms_per_hour = 3600000; // Millisekunden pro Stunde
    unsigned int ms_per_minute = 60000; // Millisekunden pro Minute
    unsigned int ms_per_second = 1000;  // Millisekunden pro Sekunde

    unsigned int days = timestamp_ms / ms_per_day;
    timestamp_ms %= ms_per_day;

    unsigned int hours = timestamp_ms / ms_per_hour;
    timestamp_ms %= ms_per_hour;

    unsigned int minutes = timestamp_ms / ms_per_minute;
    timestamp_ms %= ms_per_minute;

    unsigned int seconds = timestamp_ms / ms_per_second;
    unsigned int milliseconds = timestamp_ms % ms_per_second;

    // Ausgabe der Längenzeit
    printf("%d Tage\n", days);
    printf("%d Stunden\n", hours);
    printf("%d Minuten\n", minutes);
    printf("%d Sekunden\n", seconds);
    printf("%d Millisekunden\n", milliseconds);
}

unsigned char strUptime[80];
void formatTimestampHHMMSS(unsigned int timestamp_ms) {
    unsigned int ms_per_day = 86400000; // Millisekunden pro Tag
    unsigned int ms_per_hour = 3600000; // Millisekunden pro Stunde
    unsigned int ms_per_minute = 60000; // Millisekunden pro Minute
    unsigned int ms_per_second = 1000;  // Millisekunden pro Sekunde

    unsigned int days = timestamp_ms / ms_per_day;
    timestamp_ms %= ms_per_day;

    unsigned int hours = timestamp_ms / ms_per_hour;
    timestamp_ms %= ms_per_hour;

    unsigned int minutes = timestamp_ms / ms_per_minute;
    timestamp_ms %= ms_per_minute;

    unsigned int seconds = timestamp_ms / ms_per_second;
    unsigned int milliseconds = timestamp_ms % ms_per_second;

    int revnum = REVISION_NUMBER;
    unsigned char* revdate = REVISION_DATE;
    sprintf(strUptime, "%d:%d:%d\n", &hours, &minutes, &seconds);

    print_string(strUptime);
}

// Funktion zum Verstecken des Cursors
void hidecursor() {
    // Index 0x0A entspricht dem Cursor-Form Control Register
    port_byte_out(0x3D4, 0x0A);
    unsigned char cursorControl = port_byte_in(0x3D5);

    // Bit 5 auf 1 setzen, um den Cursor zu verstecken
    cursorControl |= 0x20;

    // Neuen Wert an das CRTC Data Register senden
    port_byte_out(0x3D5, cursorControl);
}

// Funktion zum Anzeigen des Cursors
void showcursor() {
    // Index 0x0A entspricht dem Cursor-Form Control Register
    port_byte_out(0x3D4, 0x0A);
    unsigned char cursorControl = port_byte_in(0x3D5);

    // Bit 5 auf 0 setzen, um den Cursor anzuzeigen
    cursorControl &= 0xDF;

    // Neuen Wert an das CRTC Data Register senden
    port_byte_out(0x3D5, cursorControl);
}


int memtest() {
    void* mem = (void*)malloc(256);
    void* mem2 = (void*)malloc(128);
    printf("Allocated blocks:\n");
    list_allocated_blocks();
    free(mem);
    printf("\nAllocated blocks after freeing mem:\n");
    list_allocated_blocks();
    free(mem2);
    printf("\nAllocated blocks after freeing mem2:\n");
    list_allocated_blocks();
    return 0;
}




// Makro, um eine Funktion nach Namen aufzurufen (ohne Parameter)
#define CALL_FUNCTION(funcName) \
    else if (strcmp(input, #funcName) == 0) { \
        funcName(); \
    }

// Makro, um eine Funktion nach Namen aufzurufen (mit einem Parameter)
#define CALL_FUNCTION_WITH_STR(funcName) \
    else if (strstr(input, #funcName) == input) { \
        char* str1 = strtok(input + strlen(#funcName) + 1, " "); \
        if (str1 != NULL) { \
            funcName(str1); \
        } else { printf("Invalid parameters for %s\n", #funcName); } \
    }

// Makro, um eine Funktion nach Namen aufzurufen (mit einem Parameter)
#define CALL_FUNCTION_WITH_ARG(funcName) \
    else if (strstr(input, #funcName) == input) { \
        char* str1 = strtok(input + strlen(#funcName) + 1, " "); \
        if (str1 != NULL) { \
            uint32_t param1 = (uint32_t)strtoul(str1, NULL, 0); \
            funcName((void*)param1); \
        } else { printf("Invalid parameters for %s\n", #funcName); } \
    }

// Makro, um eine Funktion nach Namen aufzurufen (mit drei Parametern)
#define CALL_FUNCTION_WITH_2ARGS(funcName) \
    else if (strstr(input, #funcName) == input) { \
        char* str1 = strtok(input + strlen(#funcName) + 1, " "); \
        char* str2 = strtok(NULL, " "); \
        if (str1 != NULL && str2 != NULL) { \
            uint32_t param1 = (uint32_t)strtoul(str1, NULL, 0); \
            uint32_t param2 = (uint32_t)strtoul(str2, NULL, 0); \
            funcName((void*)param1, param2); \
        } else { printf("Invalid parameters for %s\n", #funcName); } \
    }


// Makro, um eine Funktion nach Namen aufzurufen (mit drei Parametern)
#define CALL_FUNCTION_WITH_3ARGS(funcName) \
    else if (strstr(input, #funcName) == input) { \
        char* str1 = strtok(input + strlen(#funcName) + 1, " "); \
        char* str2 = strtok(NULL, " "); \
        char* str3 = strtok(NULL, " "); \
        if (str1 != NULL && str2 != NULL && str3 != NULL) { \
            uint32_t param1 = (uint32_t)strtoul(str1, NULL, 0); \
            uint32_t param2 = (uint32_t)strtoul(str2, NULL, 0); \
            uint32_t param3 = (uint32_t)strtoul(str3, NULL, 0); \
            funcName(param1, param2, param3); \
        } else { printf("Invalid parameters for %s\n", #funcName); } \
    }

void random() {
    //init_random();
    for (int i = 0; i < 10; i++) {
        printf("%d\n", rand_range(1, 100)); // Beispiel: Zahlen zwischen 1 und 100
    }
}  

void uptime() {
    formatTimestampHHMMSS(GetTicks());
}

void exit() {
    g_bKernelShouldStop = true;
}

void run(void* address) {
    FilePointer funcPtr = (FilePointer)address;
    funcPtr();
}

void* search_signature(void* start_address, size_t size, uint32_t signature) {
    for (size_t i = 0; i < size - sizeof(uint32_t) + 1; i++) {
        uint32_t value = *((uint32_t*)(start_address + i));
        if (value == signature) {
            // Signature found, return the address
            return (start_address + i);
        }
    }

    // Signature not found in the given range
    return NULL;
}


void printframe(int x, int y, int w, int h, unsigned char color) {
    unsigned int old_color = get_color();
    set_color(color);
    set_cursor_xy(x, y);
    printf("%c", 0xC9); // ecke links oben
    for (int col = 0; col < w; col++) { printf("%c", 0xCD); } // balken oben
    printf("%c\n", 0xBB); // ecke rechts oben
    y++;
    for(int row=0; row<h; row++) {
        set_cursor_xy(x, y);
        printf("%c", 0xBA); // balken links
        for (int col = 0; col < w; col++) { printf("%c", ' '); } // leerzeichen
        printf("%c\n", 0xBA); // balken rechts
        y++;
    }
    set_cursor_xy(x, y);
    printf("%c", 0xC8); // ecke links unten
    for (int col = 0; col < w; col++) { printf("%c", 0xCD); } // balken unten
    printf("%c\n", 0xBC); // ecke rechts unten
    set_color(old_color);
}

void printframe_caption(int x, int y, int w, int h, unsigned char color, unsigned char* caption) {
    unsigned int old_color = get_color();
    unsigned char len = strlen(caption);
    unsigned char offset = (w - len) / 2;

    set_color(color);
    set_cursor_xy(x, y);
    printf("%c", 0xC9); // ecke links oben
 
    for (int col = 0; col < (w-offset-len/2)-5; col++) { printf("%c", 0xCD); } // balken oben
    printf("%s", caption); // balken oben
    for (int col = 0; col < (w-offset-len/2)-5; col++) { printf("%c", 0xCD); } // balken oben

    printf("%c\n", 0xBB); // ecke rechts oben
    y++;
    for(int row=0; row<h-2; row++) {
        set_cursor_xy(x, y);
        printf("%c", 0xBA); // balken links
        for (int col = 0; col < w; col++) { printf("%c", ' '); } // leerzeichen
        printf("%c\n", 0xBA); // balken rechts
        y++;
    }
    set_cursor_xy(x, y);
    printf("%c", 0xC8); // ecke links unten
    for (int col = 0; col < w; col++) { printf("%c", 0xCD); } // balken unten
    printf("%c\n", 0xBC); // ecke rechts unten
    set_color(old_color);
}

void pf() {
    printframe_caption(30, 7, 20, 10, FG_WHITE | BG_LIGHT_BLUE, " Question ");
  //  printframe(2, 2, 74, 23, FG_BRIGHT_WHITE | BG_BLUE);
}

void dtmf() {
    playDTMF("*31#0461#");    
}


void sub_timer_snaketext_callback();
int id=0;
void snt() {
   id = add_sub_timer(50, sub_timer_snaketext_callback);
}
void sne() {
    playDTMF("*31#0461#");
    remove_sub_timer(id);
}

void process_input(const char *input) {
    int len = strlen(input);
    int i = 0;
    char token[256]; // Annahme: Maximale Länge eines Tokens ist 255 Zeichen

    while (i < len) {
        int token_index = 0;

        // Token sammeln, bis \a oder das Ende des Strings erreicht ist
        while (i < len && input[i] != '\a') {
            token[token_index++] = input[i++];
        }

        // Null-Terminator hinzufügen
        token[token_index] = '\0';

        // Prüfen, ob das Token "freq:len" ist
        char *colon = strchr(token, ':');
        if (colon != NULL) {
            // Interpretieren als Frequenz und Länge
            int freq, len;
            if (sscanf(token, "%d:%d", &freq, &len) == 2) {
                printf("Bell: Frequency=%d ", freq);
                printf("Length=%d\n", len);
                beep(freq, len);
            }
        } else if (token_index > 0) { // Nur wenn Token nicht leer ist
            // Interpretieren als Text für eSpeak
            printf("eSpeak: %s\n", token);
            playDTMF(token);
        }

        i++; // Zum nächsten Zeichen gehen (Überspringen von \a)
    }
}

int bell() {
    const char *input = "\a0123456789\a\a440:500\a\a*31#9876543210#\a";
    process_input(input);
    return 0;
}

void killtimer() {
    remove_sub_timer(0);
}


int restart() {
    longjmp(&kernel_env, 0);
    return 0;
}

#define BYTE 1
#define WORD 2
#define DWORD 4
#define KILOBYTE 1024
#define MEGABYTE 1048576
#define GIGABYTE 1073741824

void loaddisk() {
    for(unsigned int i=0; i<2880; i++) {
        read_from_disk(i, (void*)0x100000+(512*i), 512);
    }
}

void cat(const char *addr) {
    while (*addr != '\0') {
        printf("%c", *addr);
        addr++;
    }
    printf("\n");
}

void execute_command(char *input) {
    int cursor = get_cursor();
    if (strcmp(input, "") == 0) { goto none; }

    CALL_FUNCTION_WITH_ARG(cat)
    CALL_FUNCTION(memtest)
    CALL_FUNCTION(killtimer)
    CALL_FUNCTION(bell)
    CALL_FUNCTION(snt)
    CALL_FUNCTION(sne)
    CALL_FUNCTION(ramdisk_test)
    CALL_FUNCTION(tinysql)
    CALL_FUNCTION(pip)
    CALL_FUNCTION(restart)
    CALL_FUNCTION(main_c)
    CALL_FUNCTION_WITH_2ARGS(beep)
    CALL_FUNCTION(dtmf)
    CALL_FUNCTION(editor_main)
    CALL_FUNCTION(editor_main2)
    CALL_FUNCTION(printlogo)
    CALL_FUNCTION(ll_main)
    CALL_FUNCTION(setpal)
    CALL_FUNCTION(snake_main)
    CALL_FUNCTION(random)
    CALL_FUNCTION(uptime)
    CALL_FUNCTION(hidecursor)
    CALL_FUNCTION(showcursor)
    CALL_FUNCTION(print_registers)
    CALL_FUNCTION(keycodes)
    CALL_FUNCTION(exit)
    CALL_FUNCTION(loaddisk)
    CALL_FUNCTION(pf)
    CALL_FUNCTION_WITH_2ARGS(hexdump)
    CALL_FUNCTION_WITH_ARG(hexviewer)
    CALL_FUNCTION_WITH_3ARGS(memcpy)
    CALL_FUNCTION_WITH_2ARGS(memset)
    CALL_FUNCTION_WITH_ARG(run)
    CALL_FUNCTION(printascii)
    CALL_FUNCTION_WITH_3ARGS(searchb)

    else if (strstr(input, "searchs") == input)
    {
        execute_search_command_str(input);
    }
    else if (strcmp(input, "clr") == 0 || strcmp(input, "rst") == 0)
    {
        clear_screen();
        set_cursor(0);
    }
    else
    {
        print_string("Unknown command: ");
        print_string(input);
        print_nl();
    }
    // print_string("> ");
    printf("%c ", 0x10);
    return;
none:
    set_cursor(cursor - 156);
    return;
}

void clear_cursor() {
    int newCursor = get_cursor();
    set_char_at_video_memory(' ', newCursor);
}

// Console program one
int kernel_console_program() {
    while (1) {
        // Überprüfe den Tastaturstatus
        if (read_keyboard_status() & 0x01) {
            // Tastatureingabe verfügbar, lese den Scan-Code
            uint8_t scancode = read_keyboard_data();

            if (scancode > SC_MAX) return 1;

            HandleKeypress(sc_ascii2[(int)scancode]);

            if (scancode == BACKSPACE) {
                if (backspace(key_buffer2)) {
                    print_backspace();
                }
            } else if (scancode == ENTER) {
                clear_cursor();
                print_nl();
                execute_command(key_buffer2);
                key_buffer2[0] = '\0';
            } else if (scancode == SC_F1) {
                ramdisk_test();
            } else {
                char letter;
                if (is_key_pressed(SHIFT)) {
                    letter = sc_ascii3[(int)scancode];
                } else {
                    letter = sc_ascii2[(int)scancode];
                }
                append(key_buffer2, letter);
                char str[2] = {letter, '\0'};
                print_string(str);
            }
        }
    }
}
