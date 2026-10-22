#include "scan_list.h"

static ScanHit _hits[SCAN_MAX];
static int     _n = 0;

int scan_count() { return _n; }
const ScanHit* scan_get(int i) { return (i >= 0 && i < _n) ? &_hits[i] : nullptr; }
void scan_clear() { _n = 0; }

static int find(uint32_t hz, uint32_t tol_hz) {
    for (int i = 0; i < _n; i++) {
        uint32_t a = _hits[i].hz, b = hz;
        uint32_t d = (a > b) ? (a - b) : (b - a);
        if (d <= tol_hz) return i;
    }
    return -1;
}

bool scan_add(uint32_t hz, uint8_t type, int rssi, uint32_t tol_hz) {
    int i = find(hz, tol_hz);
    if (i >= 0) {                                   // odswiez istniejacy
        _hits[i].rssi = rssi; _hits[i].seen_ms = millis();
        if (type != 0xFF) _hits[i].type = type;     // znany typ nadpisuje "?"
        return false;
    }
    if (_n >= SCAN_MAX) {                            // pelna — wyrzuc najstarszy
        int old = 0; for (int k = 1; k < _n; k++) if (_hits[k].seen_ms < _hits[old].seen_ms) old = k;
        i = old;
    } else {
        i = _n++;
    }
    _hits[i].hz = hz; _hits[i].type = type; _hits[i].rssi = rssi; _hits[i].seen_ms = millis();
    return true;
}

void scan_expire(uint32_t age_ms) {
    uint32_t now = millis();
    int w = 0;
    for (int r = 0; r < _n; r++)
        if (now - _hits[r].seen_ms <= age_ms) _hits[w++] = _hits[r];
    _n = w;
}
