#pragma once
#include <Arduino.h>

// Lista SCAN — "znajdzki" auto-skanu spoza QRG. Tylko w RAM (nie trwala).
#define SCAN_MAX 16

struct ScanHit {
    uint32_t hz;
    uint8_t  type;      // StypeCode lub ST_UNK gdy tylko nosna
    int      rssi;
    uint32_t seen_ms;   // ostatnie trafienie
};

int             scan_count();
const ScanHit*  scan_get(int i);
// Dodaj/odswiez trafienie. Pomija jesli freq jest juz w QRG (dedup robi wywolujacy).
// Scala z istniejacym wpisem SCAN w tolerancji tol_hz. Zwraca true gdy nowy wpis.
bool            scan_add(uint32_t hz, uint8_t type, int rssi, uint32_t tol_hz);
void            scan_clear();
// Usun wpisy starsze niz age_ms (sonda przeleciala / zniknela nosna).
void            scan_expire(uint32_t age_ms);
