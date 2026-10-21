#pragma once
#include <Arduino.h>

// RA-02 (SX1278) — warstwa najnizsza: software-SPI (bit-bang) + probe ukladu.
// Oba sprzetowe SPI S3 zajete (QSPI=display, HSPI=SD), wiec radio na bit-bang.
// Etap 1: tylko wykrycie ukladu (odczyt RegVersion). Dekoder rdz dojdzie pozniej.

bool    sonde_radio_probe();     // reset + odczyt RegVersion; true gdy ==0x12 (SX127x)
uint8_t sonde_radio_version();   // ostatnio odczytany bajt wersji
bool    sonde_radio_present();   // czy ostatni probe wykryl uklad

// Niskopoziomowy dostep (dla przyszlego dekodera)
uint8_t sonde_radio_reg_read(uint8_t addr);
void    sonde_radio_reg_write(uint8_t addr, uint8_t val);

// ── FSK RX + skan nosnej (etap "slysze nosna") ──
void sonde_radio_fsk_begin();                 // FSK RX ciagly, LNA max, AGC
void sonde_radio_set_freq(uint32_t hz);       // przestroj nosna
int  sonde_radio_rssi();                       // biezace RSSI w dBm (ujemne)
int  sonde_radio_scan(uint32_t lo, uint32_t hi, uint32_t step, uint32_t* peakHz, int* floorDbm); // peak RSSI + jego czestotliwosc + szum
void sonde_radio_sleep();                       // uspij radio
