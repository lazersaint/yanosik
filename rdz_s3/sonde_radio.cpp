#include "sonde_radio.h"
#include "config.h"
#include <SPI.h>

// ── Sprzetowy SPI na pinach RA-02 (SuperMini ma wolne HW-SPI) ────────────────
// SX127x: SPI MODE0 (CPOL=0, CPHA=0), MSB first. Ta sama warstwa API co bit-bang
// w Yanosiku — dzieki temu caly dekoder (SX1278FSK/RS41) jest NIEZMIENIONY.
static bool    _inited  = false;
static uint8_t _ver     = 0;
static bool    _present = false;

static SPIClass      _spi(FSPI);               // S3: FSPI dla dowolnych pinow
static SPISettings   _set(LORA_SPI_HZ, MSBFIRST, SPI_MODE0);

static void _pins_init() {
    if (_inited) return;
    pinMode(LORA_NSS, OUTPUT); digitalWrite(LORA_NSS, HIGH);
    pinMode(LORA_RST, OUTPUT); digitalWrite(LORA_RST, HIGH);
    pinMode(LORA_DIO0, INPUT);
    _spi.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    _inited = true;
}

uint8_t sonde_radio_reg_read(uint8_t addr) {
    _pins_init();
    _spi.beginTransaction(_set);
    digitalWrite(LORA_NSS, LOW);
    _spi.transfer(addr & 0x7F);            // bit7=0 -> odczyt
    uint8_t v = _spi.transfer(0x00);
    digitalWrite(LORA_NSS, HIGH);
    _spi.endTransaction();
    return v;
}

void sonde_radio_reg_write(uint8_t addr, uint8_t val) {
    _pins_init();
    _spi.beginTransaction(_set);
    digitalWrite(LORA_NSS, LOW);
    _spi.transfer(addr | 0x80);            // bit7=1 -> zapis
    _spi.transfer(val);
    digitalWrite(LORA_NSS, HIGH);
    _spi.endTransaction();
}

bool sonde_radio_probe() {
    _pins_init();
    digitalWrite(LORA_RST, LOW);  delay(2);
    digitalWrite(LORA_RST, HIGH); delay(6);
    _ver = sonde_radio_reg_read(0x42);     // RegVersion: SX1276/77/78 -> 0x12
    _present = (_ver == 0x12);
    Serial.printf("[LORA] RegVersion=0x%02X -> %s\n", _ver, _present ? "RA-02 OK" : "BRAK/zly");
    return _present;
}

uint8_t sonde_radio_version() { return _ver; }
bool    sonde_radio_present() { return _present; }

// ── FSK RX + skan (Fstep = 32 MHz / 2^19 = 61.035 Hz) ────────────────────────
static void _set_freq(uint32_t hz) {
    uint64_t frf = ((uint64_t)hz << 19) / 32000000ULL;
    sonde_radio_reg_write(0x06, (uint8_t)((frf >> 16) & 0xFF));
    sonde_radio_reg_write(0x07, (uint8_t)((frf >> 8)  & 0xFF));
    sonde_radio_reg_write(0x08, (uint8_t)( frf        & 0xFF));
}
void sonde_radio_set_freq(uint32_t hz) { _set_freq(hz); }

#define OP_FSK_SLEEP  0x08
#define OP_FSK_STDBY  0x09
#define OP_FSK_RX     0x0D

void sonde_radio_fsk_begin() {
    _pins_init();
    sonde_radio_reg_write(0x01, OP_FSK_SLEEP); delay(2);
    _set_freq(LORA_SCAN_LO);
    sonde_radio_reg_write(0x0C, 0x20);  // RegLna: max gain
    sonde_radio_reg_write(0x0D, 0x08);  // RegRxConfig: AgcAutoOn
    sonde_radio_reg_write(0x12, 0x02);  // RegRxBw ~125 kHz
    sonde_radio_reg_write(0x13, 0x02);  // RegAfcBw ~125 kHz
    sonde_radio_reg_write(0x01, OP_FSK_RX); delay(5);
    Serial.printf("[LORA] FSK RX start (skan %lu-%lu Hz)\n",
                  (unsigned long)LORA_SCAN_LO, (unsigned long)LORA_SCAN_HI);
}

int sonde_radio_rssi() {
    return -((int)sonde_radio_reg_read(0x11)) / 2;
}

int sonde_radio_scan(uint32_t lo, uint32_t hi, uint32_t step, uint32_t* peakHz, int* floorDbm) {
    int best = -250, flr = 50; uint32_t bf = lo;
    for (uint32_t f = lo; f <= hi; f += step) {
        _set_freq(f);
        delayMicroseconds(1200);
        int r = sonde_radio_rssi();
        if (r > best) { best = r; bf = f; }
        if (r < flr)  flr = r;
    }
    if (peakHz)   *peakHz = bf;
    if (floorDbm) *floorDbm = flr;
    return best;
}

void sonde_radio_sleep() { sonde_radio_reg_write(0x01, OP_FSK_SLEEP); }
