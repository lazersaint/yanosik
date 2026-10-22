#pragma once
#include <Arduino.h>
#include <string.h>
#include <strings.h>   // strcasecmp

// Kody typów sond w protokole UART / QRG (niezalezne od wewnetrznych enumow dekodera).
enum StypeCode : uint8_t {
    ST_RS41 = 0, ST_DFM, ST_M10, ST_M20, ST_MP3H, ST_RS92,
    ST_UNK = 0xFF
};

static inline const char* stype_name(uint8_t t) {
    switch (t) {
        case ST_RS41: return "RS41";
        case ST_DFM:  return "DFM";
        case ST_M10:  return "M10";
        case ST_M20:  return "M20";
        case ST_MP3H: return "MP3H";
        case ST_RS92: return "RS92";
        default:      return "?";
    }
}

static inline uint8_t stype_from_name(const char* s) {
    if (!s) return ST_UNK;
    if (!strcasecmp(s, "RS41")) return ST_RS41;
    if (!strcasecmp(s, "DFM"))  return ST_DFM;
    if (!strcasecmp(s, "M10"))  return ST_M10;
    if (!strcasecmp(s, "M20"))  return ST_M20;
    if (!strcasecmp(s, "M10M20")|| !strcasecmp(s,"M10/M20")) return ST_M10;
    if (!strcasecmp(s, "MP3H")) return ST_MP3H;
    if (!strcasecmp(s, "RS92")) return ST_RS92;
    return ST_UNK;
}
