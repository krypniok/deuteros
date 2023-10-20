#include <stddef.h>

#include "kernel.h"
#include "time.h"

#include "../drivers/keyboard.h"
#include "../drivers/display.h"
#include "../cpu/jmpbuf.h"

extern bool g_bKernelShouldStop;
extern jmp_buf kernel_env;

void searchb(uint32_t address, uint32_t size, uint32_t byte) {
        printf("Searching %d\n", byte);
        void* result = (void*)search_byte((void*)address, size, byte);
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
        void* result = (void*)search_string((void*)address, size, string);
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

void kernel_console_clear() {
        clear_screen();
        set_cursor(0);
}
