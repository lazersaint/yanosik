// ═══════════════════════════════════════════════════════════════════════════
//  rdz_s3 — firmware plytki sond (ESP32-S3 SuperMini + RA-02)
//
//  Backbone Phase B:
//   • tryb MANUAL (lock na freq z QRG) / AUTO (skan pasma + dedup vs QRG → SCAN)
//   • dekoder RS41 (pozostale typy dochodza w nast. kroku)
//   • protokol UART do Yanosika: $SON/$STAT/$QRG/$SCAN  ⟵  $CMD,...
//   • BLE TrovaLaSonda (gdy SONDA_BLE)
//
//  Yanosik rysuje UI (4 ekrany) na podstawie tych linijek; mini liczy sonde.
// ═══════════════════════════════════════════════════════════════════════════
#include "config.h"
#include "sonde_types.h"
#include "qrg_store.h"
#include "scan_list.h"
#include "src/Sonde.h"
#include "src/SX1278FSK.h"
#include "src/RS41.h"
#include "src/sonde_bridge.h"     // struct SondeView
#include "src/sonde_ble.h"
#include "sonde_radio.h"
#include <Arduino.h>
#include <string.h>
#include <strings.h>   // strcasecmp
#include <stdlib.h>

extern void sonde_shim_init();    // z src/sonde_shim.cpp

#define YANO Serial0              // UART0 (GP43/44) do Yanosika

// ── Stan odbioru ─────────────────────────────────────────────────────────────
enum Mode { MODE_MANUAL = 0, MODE_AUTO = 1 };
static Mode     g_mode    = MODE_MANUAL;
static uint32_t g_lock_hz = LORA_SONDE_FREQ;   // aktywna freq w MANUAL
static uint8_t  g_lock_ty = ST_RS41;           // typ z wpisu QRG
static uint32_t g_cur_hz  = 0;                  // aktualnie nastrojona nosna
static uint8_t  g_rec_ty  = ST_UNK;            // typ realnie rozpoznany
static uint32_t g_scan_hz = LORA_SCAN_LO;      // kursor skanu
static int      g_floor   = -120;              // biezacy prog szumu w skanie

static SondeView _v;

// ── Suma kontrolna NMEA + wysylka linii ──────────────────────────────────────
static void emit_line(const char* body) {
    uint8_t ck = 0;
    for (const char* p = body; *p; ++p) ck ^= (uint8_t)*p;
    YANO.printf("$%s*%02X\r\n", body, ck);
}

// $SON,<typ>,<serial>,<lat>,<lon>,<alt>,<vs>,<hs>,<sats>,<rssi>,<frame>,<freqHz>
static void emit_son(uint8_t type, uint32_t freq) {
    char b[200];
    const char* ty = stype_name(type);
    if (_v.have_pos) {
        snprintf(b, sizeof b, "SON,%s,%s,%.6f,%.6f,%.0f,%.1f,%.1f,%d,%d,,%lu",
                 ty, _v.have_id ? _v.id : "", _v.lat, _v.lon, _v.alt,
                 _v.vs, _v.hs, _v.sats, _v.rssi_dbm, (unsigned long)freq);
    } else {
        snprintf(b, sizeof b, "SON,%s,%s,,,,,,,%d,,%lu",
                 ty, _v.have_id ? _v.id : "", _v.rssi_dbm, (unsigned long)freq);
    }
    emit_line(b);
}

// $STAT,<MANUAL|AUTO>,<lockHz>,<rozpoznanyTyp>,<rssi>,<rdzwx>,<trova>
static void emit_stat() {
    char b[96];
    int trova = 0;
#if SONDA_BLE
    trova = sonde_ble_connected() ? 1 : (sonde_ble_ready() ? 2 : 0);
#endif
    snprintf(b, sizeof b, "STAT,%s,%lu,%s,%d,%d,%d",
             g_mode == MODE_AUTO ? "AUTO" : "MANUAL",
             (unsigned long)g_lock_hz, stype_name(g_rec_ty), _v.rssi_dbm,
             0 /* rdzwx: dojdzie z WiFi */, trova);
    emit_line(b);
}

static void emit_qrg_all() {
    int n = qrg_count();
    char b[96];
    if (n == 0) { emit_line("QRG,0,0,0,?,"); return; }
    for (int i = 0; i < n; i++) {
        const QrgEntry* e = qrg_get(i);
        snprintf(b, sizeof b, "QRG,%d,%d,%lu,%s,%s", i, n,
                 (unsigned long)e->hz, stype_name(e->type), e->name);
        emit_line(b);
    }
}

static void emit_scan_all() {
    int n = scan_count();
    char b[96];
    if (n == 0) { emit_line("SCAN,0,0,0,?,0"); return; }
    for (int i = 0; i < n; i++) {
        const ScanHit* h = scan_get(i);
        snprintf(b, sizeof b, "SCAN,%d,%d,%lu,%s,%d,1", i, n,
                 (unsigned long)h->hz, stype_name(h->type), h->rssi);
        emit_line(b);
    }
}

// ── Radio / dekoder ──────────────────────────────────────────────────────────
static void tune(uint32_t hz) {
    if (hz == g_cur_hz) return;
    rs41.setup((float)hz);     // RS41::setup chce Hz; ustawia nosna + FSK RX
    g_cur_hz = hz;
}

static int rssi_now() {
    return -((int)sonde_radio_reg_read(0x11)) / 2;   // RegRssiValue → dBm
}

// Odbierz jedna ramke na g_cur_hz, zaktualizuj _v. Zwraca true gdy mamy ID.
static bool decode_once() {
    rs41.receive();                                  // blokuje do ramki/timeoutu
    SondeInfo* si = sonde.si();
    _v.rssi_dbm = (si->rssi != 0) ? -si->rssi / 2 : rssi_now();
    bool got = false;
    if (si->d.validID) {
        strncpy(_v.id, si->d.id, sizeof(_v.id) - 1);
        _v.id[sizeof(_v.id) - 1] = 0;
        _v.have_id = true; got = true;
        g_rec_ty = ST_RS41;
    }
    if (si->d.validPos & 0x3F) {
        _v.lat = si->d.lat; _v.lon = si->d.lon; _v.alt = si->d.alt;
        _v.vs  = si->d.vs;  _v.hs  = si->d.hs;  _v.sats = si->d.sats;
        _v.have_pos = true; _v.last_ms = millis();
    }
    return got;
}

// ── Parser komend $CMD z Yanosika ────────────────────────────────────────────
static void handle_cmd(char* line) {               // line bez '$' i bez *CC
    char* tok[8] = {0}; int n = 0; char* p = line;
    while (n < 8 && p) { tok[n++] = p; char* c = strchr(p, ','); if (!c) break; *c = 0; p = c + 1; }
    if (n < 2 || strcmp(tok[0], "CMD")) return;
    const char* cmd = tok[1];

    if (!strcmp(cmd, "MODE") && n >= 3) {
        g_mode = (!strcasecmp(tok[2], "AUTO")) ? MODE_AUTO : MODE_MANUAL;
        if (g_mode == MODE_AUTO) { g_scan_hz = LORA_SCAN_LO; g_floor = -120; }
    } else if (!strcmp(cmd, "LOCK") && n >= 3) {
        g_lock_hz = strtoul(tok[2], nullptr, 10);
        g_lock_ty = (n >= 4) ? stype_from_name(tok[3]) : ST_RS41;
        g_mode = MODE_MANUAL;
        _v.have_id = _v.have_pos = false;           // nowa sonda — czysc
        g_rec_ty = g_lock_ty;
    } else if (!strcmp(cmd, "GETQRG")) {
        emit_qrg_all();
    } else if (!strcmp(cmd, "GETSCAN")) {
        emit_scan_all();
    } else if (!strcmp(cmd, "SAVEQRG") && n >= 3) {
        uint32_t hz = strtoul(tok[2], nullptr, 10);
        uint8_t  ty = (n >= 4) ? stype_from_name(tok[3]) : ST_UNK;
        qrg_add(hz, ty, (n >= 5) ? tok[4] : "");
        emit_qrg_all();
    }
}

static void proto_poll() {
    static char line[128]; static uint8_t len = 0;
    while (YANO.available()) {
        char c = (char)YANO.read();
        if (c == '\n' || c == '\r') {
            if (len > 0) {
                line[len] = 0;
                if (line[0] == '$') {
                    char* star = strchr(line, '*'); if (star) *star = 0;   // odetnij *CC (nie wymagamy)
                    handle_cmd(line + 1);
                }
                len = 0;
            }
        } else if (len < sizeof(line) - 1) line[len++] = c;
        else len = 0;
    }
}

// ── Skan (AUTO): krok po pasmie, na nosnej dekoduj, dedup vs QRG → SCAN ───────
static void scan_step() {
    tune(g_scan_hz);
    delay(3);
    int r = rssi_now();
    if (r < g_floor) g_floor = r;                        // sledz prog szumu
    bool carrier = (r - g_floor) > 12;                   // ~12 dB nad szumem

    if (carrier) {
        _v.have_id = _v.have_pos = false;
        bool got = decode_once();                        // sprobuj zdekodowac (RS41)
        uint8_t ty = got ? ST_RS41 : ST_UNK;
        int idx;
        if (!qrg_contains(g_scan_hz, 6000, &idx)) {      // tylko spoza QRG
            scan_add(g_scan_hz, ty, _v.rssi_dbm, 6000);
        }
        if (got) { g_rec_ty = ST_RS41; emit_son(ST_RS41, g_scan_hz); }
    }

    g_scan_hz += LORA_SCAN_STEP;
    if (g_scan_hz > LORA_SCAN_HI) { g_scan_hz = LORA_SCAN_LO; g_floor = -120; scan_expire(60000); }
}

// ═════════════════════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    YANO.begin(YANOSIK_UART_BAUD, SERIAL_8N1, YANOSIK_UART_RX, YANOSIK_UART_TX);
    delay(200);
    Serial.println("\n[rdz_s3] start — MANUAL/AUTO + protokol UART");

    memset(&_v, 0, sizeof _v);
    qrg_begin();
    const QrgEntry* e0 = qrg_get(0);
    if (e0) { g_lock_hz = e0->hz; g_lock_ty = e0->type; g_rec_ty = e0->type; }

    sonde_shim_init();
    sx1278.setup(NULL);
    tune(g_lock_hz);
    _v.running = true;

#if SONDA_BLE
    sonde_ble_begin();
    Serial.printf("[BLE] start, free heap=%u\n", (unsigned)ESP.getFreeHeap());
#endif
    Serial.printf("[rdz_s3] MANUAL @ %.3f MHz (QRG: %d wpisow)\n", g_lock_hz / 1e6, qrg_count());
}

void loop() {
    proto_poll();

    if (g_mode == MODE_MANUAL) {
        tune(g_lock_hz);
        bool fresh = decode_once();
        // $SON po kazdej ramce z ID; inaczej heartbeat co 3s
        static uint32_t t_hb = 0;
        if (fresh) emit_son(g_rec_ty, g_lock_hz);
        if (millis() - t_hb > 3000) { t_hb = millis(); if (!fresh) emit_son(g_rec_ty, g_lock_hz); }
#if SONDA_BLE
        if (_v.have_id) sonde_ble_update(&_v);
#endif
    } else {
        scan_step();
    }

    // $STAT co 1s (tryb/lock/rssi/apki) — Yanosik aktualizuje belki/ikony
    static uint32_t t_stat = 0;
    if (millis() - t_stat > 1000) { t_stat = millis(); emit_stat(); }

    delay(3);
}
