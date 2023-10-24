#include "display.h"
#include "ports.h"
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#include "../kernel/util.h"

unsigned char g_ConsoleColor = WHITE_ON_BLACK;
unsigned char g_ConsoleX=0, g_ConsoleY=0;
unsigned char g_CursorShow = 1;
unsigned char g_CursorChar = '_';

int setpal() {
    // Initialisierung des VGA-Textmodus (In Assembler durchgeführt)

    // Farbpalette ändern
    // Farbendaten für Schwarz (Farbe 0)
    port_byte_out(0x03C8, 0); // Indexregister setzen
    port_byte_out(0x03C9, 0); // Rotwert
    port_byte_out(0x03C9, 0); // Grünwert
    port_byte_out(0x03C9, 127); // Blauwert

    // Farbendaten für Weiß (Farbe 15)
    port_byte_out(0x03C8, 15); // Indexregister setzen
    port_byte_out(0x03C9, 255); // Rotwert
    port_byte_out(0x03C9, 255); // Grünwert
    port_byte_out(0x03C9, 255); // Blauwert

    // Graduelle Blauabstufungen (Farben 1-14)
    for (int i = 0; i < 15; i++) {
        port_byte_out(0x03C8, i); // Indexregister setzen
        port_byte_out(0x03C9, 0); // Rotwert
        port_byte_out(0x03C9, i * 16); // Grünwert
        port_byte_out(0x03C9, 0); // Blauwert (graduell anpassen)
    }

    for(int i = 0; i < 16; i++) {
        port_byte_out(0x03C8, 7); // Indexregister setzen
        port_byte_out(0x03C9, i*16); // Rotwert
        port_byte_out(0x03C9, 0); // Grünwert
        port_byte_out(0x03C9, i*16); // Blauwert (graduell anpassen)
        sleep(1000);
    }

    return 0;
}

void set_color(unsigned char c) { g_ConsoleColor = c; }
//unsigned char get_color(unsigned char c) { return g_ConsoleColor; }
unsigned char get_color() { return g_ConsoleColor; }

void set_cursor_char(unsigned char c) { g_CursorChar = c; }
unsigned char get_cursor_char() { return g_CursorChar; }

unsigned char isCursorVisible() { return g_CursorShow; }
void setCursorVisible(unsigned char visible) { g_CursorShow = visible; }

void set_cursor(int offset) {
    offset /= 2;
    port_byte_out(REG_SCREEN_CTRL, 14);
    port_byte_out(REG_SCREEN_DATA, (unsigned char) (offset >> 8));
    port_byte_out(REG_SCREEN_CTRL, 15);
    port_byte_out(REG_SCREEN_DATA, (unsigned char) (offset & 0xff));
}

int get_cursor() {
    port_byte_out(REG_SCREEN_CTRL, 14);
    int offset = port_byte_in(REG_SCREEN_DATA) << 8; /* High byte: << 8 */
    port_byte_out(REG_SCREEN_CTRL, 15);
    offset += port_byte_in(REG_SCREEN_DATA);
    return offset * 2;
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

int get_offset(int col, int row) {
    return 2 * (row * MAX_COLS + col);
}

int get_row_from_offset(int offset) {
    return offset / (2 * MAX_COLS);
}

int move_offset_to_new_line(int offset) {
    return get_offset(0, get_row_from_offset(offset) + 1);
}

void set_char_at_video_memory(char character, int offset) {
    uint8_t *vidmem = (uint8_t *) VIDEO_ADDRESS;
    vidmem[offset] = character;
    vidmem[offset + 1] = g_ConsoleColor;
}


int scroll_ln(int offset) {
    memory_copy(
            (uint8_t * )(get_offset(0, 1) + VIDEO_ADDRESS),
            (uint8_t * )(get_offset(0, 0) + VIDEO_ADDRESS),
            MAX_COLS * (MAX_ROWS - 1) * 2
    );

    for (int col = 0; col < MAX_COLS; col++) {
        set_char_at_video_memory(' ', get_offset(col, MAX_ROWS - 1));
    }

    return offset - 2 * MAX_COLS;
}

/*
 * TODO:
 * - handle illegal offset (print error message somewhere)
 */
void print_string(char *string) {
    int offset = get_cursor();
    int i = 0;
    while (string[i] != 0) {
        if (offset >= MAX_ROWS * MAX_COLS * 2) {
            offset = scroll_ln(offset);
        }
        if (string[i] == '\n') {
            offset = move_offset_to_new_line(offset);
        }
        /*
        else if (string[i] == '\r') {
            // Überspringe das Carriage Return (\r)
            offset += 2;
        } else if (string[i] == '\t') {
            // Tabulatorzeichen
            int spaces = 4; // Anzahl der Leerzeichen pro Tab
            for (int j = 0; j < spaces; j++) {
                set_char_at_video_memory(' ', offset);
               // offset += 2;
            }
        } 
        */
       else {
            set_char_at_video_memory(string[i], offset);
            offset += 2;
        }
        i++;
    }
    set_cursor(offset);
}

void print_string_vertical(unsigned char x, unsigned char y, unsigned char* str) {
    for(int i=0; i<strlen(str); i++) {
        printChar(x, y+i, 0x0F, str[i]);
    }
}

void print_nl() {
    int newOffset = move_offset_to_new_line(get_cursor());
    if (newOffset >= MAX_ROWS * MAX_COLS * 2) {
        newOffset = scroll_ln(newOffset);
    }
    set_cursor(newOffset);
}

void clear_screen() {
    int screen_size = MAX_COLS * MAX_ROWS;
    for (int i = 0; i < screen_size; ++i) {
        set_char_at_video_memory(' ', i * 2);
    }
    set_cursor(get_offset(0, 0));
}

void clear_cursor() {
    int newCursor = get_cursor();
    set_char_at_video_memory(' ', newCursor);
}

void print_backspace() {
    int newCursor = get_cursor() - 2;
    set_char_at_video_memory(' ', newCursor);
    set_char_at_video_memory(' ', newCursor+2);
    set_cursor(newCursor);
}

// Function to print a colored character to the screen
void printChar(unsigned char x, unsigned char y, unsigned char cl, char c)
{
    unsigned char *videomemory = (unsigned char *)0xb8000 + (y << 7) + (y << 5) + (x << 1);
    *videomemory = c;
    videomemory++;
    *videomemory = cl;
    videomemory++;
}

// Function to print a null-terminated string to the screen
void printString(unsigned char *str)
{
    unsigned char *sp = str;
    // console cursor position
    static unsigned char x = 0, y = 0;
    while (*sp != '\0')
    {
        if (*sp == '\n')
        {
            y++;
            x = 0;
            sp++;
        }
        printChar(x, y, 0x0F, *sp);
        x++;
        sp++;
    }
}

// Helper function to convert an integer to a string
void intToString(int value, char *buffer)
{
    if (value == 0)
    {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    int numDigits = 0;
    int temp = value;

    // Count the number of digits
    while (temp != 0)
    {
        numDigits++;
        temp /= 10;
    }

    // Convert the integer to a string in reverse order
    for (int i = numDigits - 1; i >= 0; i--)
    {
        buffer[i] = '0' + (value % 10);
        value /= 10;
    }

    buffer[numDigits] = '\0';
}

// Helper function to convert a pointer to a string
void pointerToString(void *ptr, char *buffer)
{
    unsigned long value = (unsigned long)ptr;

    // Convert the pointer value to a string in hexadecimal format
    for (int i = 0; i < sizeof(void *) * 2; i++)
    {
        int digit = (value >> (sizeof(void *) * 8 - 4)) & 0xF;
        buffer[i] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        value <<= 4;
    }

    buffer[sizeof(void *) * 2] = '\0';
}

void charToString(unsigned char value, char *buffer)
{
    int digit1 = value >> 4;
    int digit2 = value & 0x0F;

    buffer[0] = (digit1 < 10) ? ('0' + digit1) : ('A' + digit1 - 10);
    buffer[1] = (digit2 < 10) ? ('0' + digit2) : ('A' + digit2 - 10);
    buffer[2] = '\0';
}

void intToStringHex(int value, char *buffer, int numDigits)
{
    const char hexDigits[] = "0123456789ABCDEF";

    for (int i = numDigits - 1; i >= 0; i--)
    {
        int digit = (value >> (i * 4)) & 0xF;
        buffer[numDigits - 1 - i] = hexDigits[digit];
    }

    buffer[numDigits] = '\0';
}

int getHexDigitsCount(unsigned int value)
{
    int count = 0;
    while (value != 0)
    {
        value >>= 4; // Shift right by 4 bits
        count++;
    }
    return count;
}

void appendHexValue(unsigned int value, int digits, char* buffer)
{
    int index = digits - 1;
    while (index >= 0)
    {
        int hexDigit = value & 0xF; // Get the lowest 4 bits
        if (hexDigit < 10)
        {
            buffer[index] = hexDigit + '0';
        }
        else
        {
            buffer[index] = hexDigit - 10 + 'A';
        }
        value >>= 4; // Shift right by 4 bits
        index--;
    }
}

void handleHexFormat(unsigned int value, int digits, char *buffer, int *charsWritten)
{
    // Calculate the actual number of hexadecimal digits in the value
    int valueDigits = getHexDigitsCount(value);

    // Calculate the number of leading zeros required
    int leadingZeros = digits - valueDigits;
    if (leadingZeros < 0)
    {
        leadingZeros = 0; // No leading zeros if the value has more digits than the specified format
    }

    // Copy the leading zeros to the buffer
    for (int i = 0; i < leadingZeros; i++)
    {
        buffer[(*charsWritten)++] = '0';
    }

    // Append the hex value to the buffer
    appendHexValue(value, valueDigits, &buffer[(*charsWritten)]);
    (*charsWritten) += valueDigits;
}

void nibbleToChar(unsigned char nibble, char *buffer)
{
    *buffer++ = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
}

int sprintf( char *buffer, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    int charsWritten = 0;

    while (*format != '\0')
    {
        if (*format == '%')
        {
            format++; // Skip the '%'

            // Handle format specifiers
            switch (*format)
            {
            case 'c':
            {
                // character argument
                const char* c = va_arg(args, const char*);
                buffer[charsWritten++] = *c;
                break;
            }
            case 's':
            {
                // String argument
                const unsigned char* str = *va_arg(args, const unsigned char**);
                // Copy the string to the buffer
                while (*str != '\0')
                {
                    buffer[charsWritten++] = *str++;
                    //break;
                }
                break;
            }
            case 'd':
            {
                // Integer argument
                int* value = *va_arg(args, int**);

                // Convert the integer to a string
                char intBuffer[12]; // Assumes a maximum of 12 digits for an integer
                intToString((int)value, intBuffer);
                // Copy the string to the buffer
                int i = 0;
                //if(*value < 10) buffer[charsWritten++] = '0';
                while (intBuffer[i] != '\0')
                {
                    buffer[charsWritten++] = intBuffer[i++];
                }
                break;
            }
            case 'p':
            {
                // Pointer argument
                void* ptr = *va_arg(args, void**);

                // Convert the pointer to a string
                char ptrBuffer[sizeof(void*) * 2 + 1]; // Assumes a maximum of sizeof(void*) * 2 digits for a pointer
                pointerToString(ptr, ptrBuffer);

                // Copy the string to the buffer
                int i = 0;
                while (ptrBuffer[i] != '\0')
                {
                    buffer[charsWritten++] = ptrBuffer[i++];
                }
                break;
            }

            case 'X':
            {
                    // Hexadecimal argument with leading zeros
                    unsigned int value = *va_arg(args, unsigned int*);
                    int digits = 0;

                    // Check for the number of digits in the format specifier
                    format++;
                    if (*format >= '0' && *format <= '9')
                    {
                        digits = *format - '0';
                    }
                    else
                    {
                        digits = 8; // Default to 8 digits if no specifier is provided
                    }

                    // Calculate the actual number of hexadecimal digits in the value
                    int valueDigits = getHexDigitsCount(value);

                    // Calculate the number of leading zeros required
                    int leadingZeros = digits - valueDigits;
                    if (leadingZeros < 0)
                    {
                        leadingZeros = 0; // No leading zeros if the value has more digits than the specified format
                    }

                    // Copy the leading zeros to the buffer
                    for (int i = 0; i < leadingZeros; i++)
                    {
                        buffer[charsWritten++] = '0';
                    }

                    // Append the hex value to the buffer
                    for (int i = valueDigits - 1; i >= 0; i--)
                    {
                        unsigned char nibble = (value >> (i * 4)) & 0xF;
                        nibbleToChar(nibble, (char *)(buffer + charsWritten));
                        charsWritten += 1;
                    }
                    break;
                }

            default:
                // Unsupported format specifier, ignore it
                break;
            }
        }
    /*
        else if (*format == '\t') {
            // Tabulatorzeichen
            int spaces = 4; // Anzahl der Leerzeichen pro Tab
            for (int i = 0; i < spaces; i++) {
                buffer[charsWritten++] = ' ';
            }
        }
        else if (*format == '\r') {
            // Überspringe das Carriage Return (\13)
            format++;
            continue;
        }
    */
        else
        {
            // Regular character, copy it to the buffer
            buffer[charsWritten++] = *format;
        }

        format++;
    }

    // Null-terminate the buffer
    buffer[charsWritten] = '\0';

    va_end(args);

    return charsWritten;
}

// Custom sprintf function
void printf(const char *format, ...)
{
    va_list args;
    g_CursorShow = 0;
    va_start(args, format);
	unsigned char buffer[80];
	sprintf(buffer, format, args);
	print_string(buffer);
    va_end(args);
    g_CursorShow = 1;
    return;
}

void printHexByte(unsigned char c) {
    unsigned char buffer[1];
    charToString(c, buffer);
    print_string(buffer);
    print_string(" ");
}

void printPointer2(void *ptr)
{
    char buffer[sizeof(void *) * 2 + 1]; // Assumes a maximum of sizeof(void *) * 2 digits for a pointer
    pointerToString(ptr, buffer);
    print_string(buffer);
}

void hexdump(void *ptr, size_t size)
{
    unsigned char *data = (unsigned char *)ptr;
    const int bytesPerLine = 16;

    for (size_t i = 0; i < size; i += bytesPerLine)
    {
        // Print the memory address
        printPointer2((void *)((uintptr_t)ptr + i));
        print_string(": ");
        //printf("%p: ", (void *)((uintptr_t)ptr + i));

        // Print the hexadecimal values
        for (int j = 0; j < bytesPerLine; j++)
        {
            if (i + j < size)
            {
                printHexByte(data[i + j]);
//                printf("%02X ", data[i + j]);
            }
            else
            {
                printf("   "); // Print empty spaces
            }
        }

        printf("  ");

        // Print the ASCII representation
        for (int j = 0; j < bytesPerLine; j++)
        {
            if (i + j < size)
            {
                unsigned char value = data[i + j];
                printf("%c", (value >= 32 && value <= 126) ? value : '.');
            }
            else
            {
                printf(" ");
            }
        }

        printf("\n");
    }
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
