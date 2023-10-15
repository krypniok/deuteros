#define MAX_FILES 10
#define MAX_FILE_SIZE 1024

typedef struct {
    char filename[32];
    char content[MAX_FILE_SIZE];
    size_t size;
    int mode;
} RAMFILE;

RAMFILE* ramdisk_fopen(const char *filename, const char *mode);
size_t ramdisk_fread(void *ptr, size_t size, size_t count, RAMFILE* stream);
size_t ramdisk_fwrite(const void *ptr, size_t size, size_t count, RAMFILE* stream);

int ramdisk_fclose(RAMFILE* stream);

int ramdisk_test();
