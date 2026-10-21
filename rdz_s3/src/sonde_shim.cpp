// Slim orkiestrator rdz dla Yanosik SX — zastepuje caly Sonde.cpp.
// Dostarcza globalne obiekty (sonde, sx1278, rs41, rxtask) i te metody klasy
// Sonde, ktore dekodery realnie wolaja: si() i clearAllData(). Reszta metod
// Sonde jest zadeklarowana w Sonde.h, ale nigdzie nie wolana, wiec linker ich
// nie zada. Konfiguracje ustawiamy w sonde_shim_init() z naszych pinow LORA_*.
#include "Sonde.h"
#include "SX1278FSK.h"
#include "RS41.h"
#include "rsc.h"
#include "../config.h"
#include <string.h>

// ── Globalne obiekty ──
// UWAGA: sx1278 jest zdefiniowany w SX1278FSK.cpp, a rs41 w RS41.cpp — NIE dublowac!
// Tu definiujemy tylko te, ktore byly w Sonde.cpp (ktorego nie kopiujemy).
Sonde      sonde;
RXTask     rxtask;

// Jeden slot sondy (Phase A: jeden kanal RS41)
static SondeInfo _slot[1];

// ── Minimalne metody Sonde wolane przez dekodery ──
Sonde::Sonde() { }                       // pusty — init robi sonde_shim_init()

SondeInfo *Sonde::si() { return &sondeList[rxtask.currentSonde]; }

void Sonde::clearAllData(SondeInfo *si) {
    memset(&si->d, 0, sizeof(si->d));    // czysc tylko zdekodowane dane (nie extra/calib)
    si->d.batteryVoltage = -1;
}

// ── Inicjalizacja slim configu — wywolaj raz przed startem dekodera ──
void sonde_shim_init() {
    memset(_slot, 0, sizeof(_slot));     // extra=NULL, wszystko wyzerowane
    sonde.sondeList = _slot;
    rxtask.currentSonde  = 0;
    rxtask.receiveResult = 0xFFFF;

    memset(&sonde.config, 0, sizeof(sonde.config));   // RDZConfig jest POD
    sonde.config.sx1278_ss   = LORA_NSS;   // i tak nieuzywane (bit-bang), ale spojne
    sonde.config.sx1278_sck  = LORA_SCK;
    sonde.config.sx1278_miso = LORA_MISO;
    sonde.config.sx1278_mosi = LORA_MOSI;
    sonde.config.freqofs     = 0;
    sonde.config.rs41.agcbw  = 12500;      // domyslne rdz
    sonde.config.rs41.rxbw   = 6300;

    _slot[0].active = true;
    _slot[0].type   = STYPE_RS41;
    _slot[0].freq   = LORA_SONDE_FREQ / 1e6f;
    _slot[0].d.batteryVoltage = -1;
}
