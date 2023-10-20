#include "../cpu/idt.h"
#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../drivers/display.h"
#include "../drivers/keyboard.h"
#include "../drivers/ports.h"
#include "../drivers/video.h"
#include "../stdlibs/string.h"
#include "../cpu/jmpbuf.h"

#include "kernel.h"
#include "util.h"
#include "time.h"

#define KERNEL_PROMPT_CHAR 0x10

static char kernel_console_key_buffer[1024];
jmp_buf kernel_env;

bool g_bKernelShouldStop = false;
bool g_bKernelInitialised = false;

typedef void (*FunctionPointer)();

unsigned char kernel_version_string[80];

int kernel_console_program();
void loaddisk();

void kernel_main() {
    setjmp(&kernel_env);
    set_color(WHITE_ON_BLACK);
    clear_screen();
    int revnum = REVISION_NUMBER;
    unsigned char* revdate = REVISION_DATE;
    sprintf(kernel_version_string, "DeuterOS 0.%d (%s)\n", (void*)&revnum, (void*)&revdate);
    printf("%s", kernel_version_string);

    if(! g_bKernelInitialised) {
        // print_string("Installing interrupt service routines (ISRs).\n");
        isr_install();

        //  print_string("Enabling external interrupts.\n");
        asm volatile("sti");

        //  print_string("Initializing keyboard (IRQ 1).\n");
        init_keyboard();

        init_memory();

        //   print_string("A20 Line was activated by the MBR.\n");
        //   enable_a20_line();

        //   print_string("Initializing timer.\n");
        init_timer(1000);

        //   print_string("Initializing PS/2 mouse interface\n");;
        // mouse_install();

        init_random();

        g_bKernelInitialised = true;
    }

    kernel_console_key_buffer[0] = '\0';
   // printlogo();
    loaddisk();
    printf("\n%c ", KERNEL_PROMPT_CHAR);

    while(!g_bKernelShouldStop) {
        kernel_console_program();
    }

end_of_kernel:
    print_nl();
    printf("Wow, we should get here...\nExiting...\nStopping the CPU...\nescape espace...\n");
    asm volatile("hlt");
    printf("P.S. Why is this still working when the CPU is officially stopped (hlt) ?\n");
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

void searchs(uint32_t address, uint32_t size, unsigned char* string) {
        printf("Searching %s\n", &string);
        void* result = search_string((void*)address, size, string);
        if (result != NULL) {
            char resultAddressStr[20];
            sprintf(resultAddressStr, "%p", &result);
            print_string("String found at address: ");
            print_string(resultAddressStr);
            print_string("\n");
        } else {
            print_string("String not found.\n");
        }
}

void uptime() {
    fmt_timespan(GetTicks(), &g_strUptime);
    printf("%s", g_strUptime);
}

void exit() {
    g_bKernelShouldStop = true;
}

void run(void* address) {
    FunctionPointer funcPtr = (FunctionPointer)address;
    funcPtr();
}

void random() {
    for (int i = 0; i < 10; i++) {
        printf("%d\n", rand_range(1, 100)); // Beispiel: Zahlen zwischen 1 und 100
    }
}  

void pf() {
    printframe_caption(30, 7, 20, 10, FG_WHITE | BG_LIGHT_BLUE, " Question ");
  //  printframe(2, 2, 74, 23, FG_BRIGHT_WHITE | BG_BLUE);
}

void dtmf() {
    playDTMF("*31#0461#");    
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



void loaddisk() {
    printf("Loading disk... to 0x100000\n");
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


int keycodes() {
    while (1) {
        uint8_t scancode = getkey();
        if (scancode < 128) {
            printf("%s\n", keyData[scancode].name);
            if (scancode == SC_ESC) {
                return 0;
            }
        }
    }
    sleep(33);
}

void execute_command(char *input) {
    int cursor = get_cursor();
    if (strcmp(input, "") == 0) { goto none; }

    CALL_FUNCTION(memtest)
    CALL_FUNCTION(killtimer)
    CALL_FUNCTION(bell)
    CALL_FUNCTION(snaketext)
    CALL_FUNCTION(ramdisk_test)
    CALL_FUNCTION(tinysql)
    CALL_FUNCTION(tinysql2)
    CALL_FUNCTION(restart)
    CALL_FUNCTION(main_c)
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
    CALL_FUNCTION(printascii)
    CALL_FUNCTION_WITH_ARG(cat)
    CALL_FUNCTION_WITH_ARG(hexviewer)
    CALL_FUNCTION_WITH_ARG(run)
    CALL_FUNCTION_WITH_2ARGS(beep)
    CALL_FUNCTION_WITH_2ARGS(hexdump)
    CALL_FUNCTION_WITH_2ARGS(memset)
    CALL_FUNCTION_WITH_3ARGS(memcpy)
    CALL_FUNCTION_WITH_3ARGS(searchb)
    CALL_FUNCTION_WITH_STR(printf)
    CALL_FUNCTION_WITH_2ARGS_AND_STR(searchs)

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
    printf("%c ", KERNEL_PROMPT_CHAR);
    return;
none:
    set_cursor(cursor - 156);
    return;
}

// Console program one
int kernel_console_program() {
    while (1) {
        uint8_t key = getkey();
        uint8_t chr = char_from_key(key);
        HandleKeypress(chr);

            if (key == SC_BACKSPACE) {
                if (backspace(kernel_console_key_buffer)) {
                    print_backspace();
                }
            } else if (key == SC_ENTER) {
                clear_cursor();
                print_nl();
                execute_command(kernel_console_key_buffer);
                kernel_console_key_buffer[0] = '\0';
            } 
            else if (key == SC_F1) {
                print_nl();
                ramdisk_test();
            }
            else {
                append(kernel_console_key_buffer, chr);
                char str[2] = {chr, '\0'};
                print_string(str);
            }
        }
}
