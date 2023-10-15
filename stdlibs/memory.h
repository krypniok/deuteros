// Header-Datei: memory.h

#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

// Hier definieren wir die Größe des Speichers für unsere einfache Speicherverwaltung.
#define MEMORY_SIZE 1024 * 1024 * 16 // 16 MB

// Funktionen zur Speicherverwaltung
void init_memory();
void* malloc(size_t size);
void free(void* ptr);

#endif

