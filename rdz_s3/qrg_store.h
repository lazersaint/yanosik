#pragma once
#include <Arduino.h>

// Lista QRG (zdefiniowane czestotliwosci) — trzymana w NVS (Preferences).
// Edycja docelowo przez web mini; Yanosik po UART tylko czyta + wybiera + zapisuje znajdzki.
#define QRG_MAX 24

struct __attribute__((packed)) QrgEntry {
    uint32_t hz;        // czestotliwosc w Hz
    uint8_t  type;      // StypeCode
    char     name[20];  // nazwa stacji (opcjonalna)
};

void             qrg_begin();                                   // wczytaj z NVS, zasiej domyslne gdy puste
int              qrg_count();
const QrgEntry*  qrg_get(int i);                                // NULL gdy poza zakresem
bool             qrg_add(uint32_t hz, uint8_t type, const char* name);
bool             qrg_del(int i);
void             qrg_save();                                    // zapis calej listy do NVS
// Czy czestotliwosc jest juz w QRG (z tolerancja w Hz). idx wypelnia indeks gdy znaleziono.
bool             qrg_contains(uint32_t hz, uint32_t tol_hz, int* idx);
