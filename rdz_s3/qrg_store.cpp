#include "qrg_store.h"
#include "sonde_types.h"
#include <Preferences.h>
#include <string.h>
#include <stdlib.h>

static QrgEntry _list[QRG_MAX];
static int      _n = 0;
static Preferences _prefs;

static void seed_defaults() {
    // Sensowne startowe (PL / sasiedztwo) — user i tak edytuje przez web.
    struct { uint32_t hz; uint8_t t; const char* nm; } d[] = {
        { 404500000UL, ST_RS41, "Legionowo 00Z" },
        { 405300000UL, ST_RS41, "Legionowo 12Z" },
        { 403000000UL, ST_DFM,  "zapas" },
        { 404000000UL, ST_RS41, "" },
    };
    _n = 0;
    for (auto& e : d) {
        _list[_n].hz = e.hz; _list[_n].type = e.t;
        strncpy(_list[_n].name, e.nm, sizeof(_list[_n].name) - 1);
        _list[_n].name[sizeof(_list[_n].name) - 1] = 0;
        _n++;
    }
}

void qrg_begin() {
    _prefs.begin("rdzqrg", false);
    size_t len = _prefs.getBytesLength("list");
    if (len >= sizeof(QrgEntry) && len <= sizeof(_list)) {
        _prefs.getBytes("list", _list, len);
        _n = len / sizeof(QrgEntry);
    } else {
        seed_defaults();
        qrg_save();
    }
}

int qrg_count() { return _n; }

const QrgEntry* qrg_get(int i) { return (i >= 0 && i < _n) ? &_list[i] : nullptr; }

void qrg_save() {
    _prefs.putBytes("list", _list, _n * sizeof(QrgEntry));
}

bool qrg_add(uint32_t hz, uint8_t type, const char* name) {
    int idx;
    if (qrg_contains(hz, 5000, &idx)) return false;   // juz jest (±5 kHz)
    if (_n >= QRG_MAX) return false;
    _list[_n].hz = hz; _list[_n].type = type;
    strncpy(_list[_n].name, name ? name : "", sizeof(_list[_n].name) - 1);
    _list[_n].name[sizeof(_list[_n].name) - 1] = 0;
    _n++;
    qrg_save();
    return true;
}

bool qrg_del(int i) {
    if (i < 0 || i >= _n) return false;
    for (int k = i; k < _n - 1; k++) _list[k] = _list[k + 1];
    _n--;
    qrg_save();
    return true;
}

bool qrg_contains(uint32_t hz, uint32_t tol_hz, int* idx) {
    for (int i = 0; i < _n; i++) {
        uint32_t a = _list[i].hz, b = hz;
        uint32_t diff = (a > b) ? (a - b) : (b - a);
        if (diff <= tol_hz) { if (idx) *idx = i; return true; }
    }
    return false;
}
