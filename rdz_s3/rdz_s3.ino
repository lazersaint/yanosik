// ═══════════════════════════════════════════════════════════════════════════
//  rdz_s3 — firmware plytki sond (ESP32-S3 SuperMini + RA-02)
//
//  Co robi:
//   1. dekoduje RS41 (nasz port rdz, ten sam co zdekodowal X3852437),
//   2. nadaje pozycje po BLE do apki TrovaLaSonda/MySondy (RAM wolny -> wchodzi),
//   3. wypluwa gotowa linijke "$SON,..." po UART0 do Yanosika (strzalka+dystans).
//
//  Yanosik nic nie liczy — dostaje numer, lat, lon, alt i sam rysuje azymut.
// ═══════════════════════════════════════════════════════════════════════════
#include "config.h"
#include "src/Sonde.h"
#include "src/SX1278FSK.h"
#include "src/RS41.h"
#include "src/sonde_bridge.h"     // struct SondeView (dla BLE)
#include "src/sonde_ble.h"
#include "sonde_radio.h"
#include <Arduino.h>
#include <string.h>

extern void sonde_shim_init();    // z src/sonde_shim.cpp

// Link do Yanosika: UART1 zmapowany na GP43/44 (te same fizyczne druty, ktorych
// uzywa bootloader przy flashu). Przy USB-CDC-on-boot=Enabled 'Serial' to USB
// (debug), wiec drut do Yanosika niesie wylacznie linijki $SON — czysto.
#define YANO Serial1

static SondeView _v;
static uint32_t  _freq_hz = LORA_SONDE_FREQ;

// ── Linijka $SON z suma kontrolna NMEA (XOR miedzy '$' a '*') ────────────────
//   $SON,RS41,<serial>,<lat>,<lon>,<alt>,<vs>,<hs>,<sats>,<rssi>*HH
// Pola bez pozycji dostaja puste miejsce (Yanosik to obsluzy).
static void emit_son() {
    char body[160];
    if (_v.have_pos) {
        snprintf(body, sizeof body,
            "SON,RS41,%s,%.6f,%.6f,%.0f,%.1f,%.1f,%d,%d",
            _v.have_id ? _v.id : "", _v.lat, _v.lon, _v.alt,
            _v.vs, _v.hs, _v.sats, _v.rssi_dbm);
    } else {
        snprintf(body, sizeof body,
            "SON,RS41,%s,,,,,,,%d",
            _v.have_id ? _v.id : "", _v.rssi_dbm);
    }
    uint8_t ck = 0;
    for (const char* p = body; *p; ++p) ck ^= (uint8_t)*p;
    YANO.printf("$%s*%02X\r\n", body, ck);
}

void setup() {
    Serial.begin(115200);                                        // debug (USB)
    YANO.begin(YANOSIK_UART_BAUD, SERIAL_8N1, YANOSIK_UART_RX, YANOSIK_UART_TX);
    delay(200);
    Serial.println("\n[rdz_s3] start — dekoder RS41 + BLE + UART do Yanosika");

    if (STATUS_LED_PIN >= 0) { pinMode(STATUS_LED_PIN, OUTPUT); digitalWrite(STATUS_LED_PIN, LOW); }

    memset(&_v, 0, sizeof _v);
    sonde_shim_init();
    sx1278.setup(NULL);                 // HW-SPI init + reset SX127x (przez sonde_radio)
    rs41.setup((float)_freq_hz);        // UWAGA: RS41::setup chce Hz (nie MHz!)
    _v.running = true;

#if SONDA_BLE
    sonde_ble_begin();                  // serwer BLE (TrovaLaSonda compat)
    Serial.printf("[BLE] start, free heap=%u\n", (unsigned)ESP.getFreeHeap());
#endif
    Serial.printf("[rdz_s3] nasluch RS41 @ %.3f MHz\n", _freq_hz / 1e6);
}

void loop() {
    // Zywy RSSI (diagnostyka nosnej) przed odbiorem ramki
    sonde_radio_reg_write(0x01, 0x05);
    delayMicroseconds(400);
    int live_rssi = -((int)sonde_radio_reg_read(0x11)) / 2;

    rs41.receive();                     // blokuje do ramki lub timeoutu
    SondeInfo* si = sonde.si();
    _v.rssi_dbm = (si->rssi != 0) ? -si->rssi / 2 : live_rssi;

    if (si->d.validID) {
        strncpy(_v.id, si->d.id, sizeof(_v.id) - 1);
        _v.id[sizeof(_v.id) - 1] = 0;
        _v.have_id = true;
    }
    if (si->d.validPos & 0x3F) {        // lat/lon/alt/vs/hs/dir wazne
        _v.lat = si->d.lat; _v.lon = si->d.lon; _v.alt = si->d.alt;
        _v.vs  = si->d.vs;  _v.hs  = si->d.hs;  _v.sats = si->d.sats;
        _v.have_pos = true;
        _v.last_ms  = millis();
    }

#if SONDA_BLE
    if (_v.have_id) sonde_ble_update(&_v);
#endif

    // $SON do Yanosika: po kazdej ramce z ID; dodatkowo heartbeat co 3s
    static uint32_t t_hb = 0;
    bool fresh = _v.have_id && (millis() - _v.last_ms < 2000 || !_v.have_pos);
    if (fresh) emit_son();
    if (millis() - t_hb > 3000) {
        t_hb = millis();
        if (!fresh) emit_son();         // nawet bez swiezej ramki -> nr+RSSI
        Serial.printf("[SONDA] RSSI=%d dBm  id=%s  pos=%s  heap=%u\n",
                      _v.rssi_dbm, _v.have_id ? _v.id : "---",
                      _v.have_pos ? "TAK" : "nie", (unsigned)ESP.getFreeHeap());
        if (STATUS_LED_PIN >= 0) digitalWrite(STATUS_LED_PIN, _v.have_id ? HIGH : LOW);
    }
    delay(5);
}
