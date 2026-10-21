#pragma once
// ═══════════════════════════════════════════════════════════════════════════
//  rdz_s3 — firmware DEDYKOWANEJ plytki sond: ESP32-S3 SuperMini + RA-02 (SX1278)
//  Cala robota sondowa (dekoder RS41 + BLE do apki) siedzi TU. Yanosik tylko
//  czyta gotowa linijke $SON po UART i pokazuje strzalke + dystans.
//
//  Plytka w Arduino IDE:  "ESP32S3 Dev Module"
//  USB CDC On Boot: Enabled  (wtedy debug -> USB, a drut do Yanosika czysty)
// ═══════════════════════════════════════════════════════════════════════════

// ─── RA-02 (SX1278) na SPRZETOWYM SPI SuperMini ──────────────────────────────
// SuperMini ma wolne HW-SPI (brak LCD/SD jak w Yanosiku), wiec lecimy sprzetowo.
#define LORA_SCK    12
#define LORA_MISO   13
#define LORA_MOSI   11
#define LORA_NSS    10
#define LORA_RST     9
#define LORA_DIO0    8
#define LORA_DIO2   -1            // nieuzywane
#define LORA_DIO1   -1            // nieuzywane
#define LORA_SPI_HZ  8000000UL    // 8 MHz — SX127x wyrabia bez problemu

// ─── Skan nosnej (RS41 w PL zwykle 400-406 MHz) ──────────────────────────────
#define LORA_SCAN_LO    400000000UL
#define LORA_SCAN_HI    406000000UL
#define LORA_SCAN_STEP  100000UL

// ─── Dekoder ─────────────────────────────────────────────────────────────────
#define SONDA_DECODER     1
#define LORA_SONDE_FREQ   404500000UL   // czestotliwosc startowa RS41 do dekodowania

// ─── BLE do apki TrovaLaSonda/MySondy ────────────────────────────────────────
// Na dedykowanej plytce RAM jest wolny (brak LVGL+mapa), wiec BLE WCHODZI.
#define SONDA_BLE         1

// ─── Link UART do Yanosika ───────────────────────────────────────────────────
// Runtime: UART1 zmapowany na GP43/44 (Serial1). GP43/44 to te same fizyczne
// piny, ktorych uzywa ROM-bootloader przy flashu — jeden komplet drutow na obie
// role. Yanosik: jego pin16 -> SM GP44(RX), pin15 -> SM GP43(TX).
#define YANOSIK_UART_BAUD   115200
#define YANOSIK_UART_TX     43
#define YANOSIK_UART_RX     44

// ─── Dioda stanu (opcjonalnie, SuperMini ma LED na GP48 lub brak) ─────────────
#define STATUS_LED_PIN     -1
