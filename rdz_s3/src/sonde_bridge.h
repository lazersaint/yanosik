#pragma once
#include <stdint.h>

// Pomost miedzy dekoderem rdz (task na rdzeniu 0) a ekranem SONDA (LVGL, rdzen 1).
// Czysty struct bez typow rdz, zeby ui_lvgl.cpp nie musial includowac Sonde.h.
struct SondeView {
    volatile bool running;
    volatile bool have_id;
    volatile bool have_pos;
    char     id[12];      // numer sondy (np. "R1234567")
    float    lat, lon, alt, vs, hs;
    int      sats;
    int      rssi_dbm;
    uint32_t last_ms;     // millis() ostatniej dobrej pozycji
};

void             sonde_bridge_start(uint32_t freq_hz);   // start task RX na danej czestotliwosci
void             sonde_bridge_stop();                    // zatrzymaj (uspi radio)
const SondeView* sonde_bridge_view();                    // biezacy stan do wyswietlenia
