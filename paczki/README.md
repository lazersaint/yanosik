# Paczki firmware — płytki zatwierdzone

Katalog z zatwierdzonymi paczkami firmware dla płytek projektu.

| Płytka | Paczka | Stan | Opis |
|--------|--------|------|------|
| **Płytka sond** (ESP32-S3 SuperMini + RA-02) | [`rdz_s3.zip`](rdz_s3.zip) | ✅ działa | Dekoder RS41 (port rdz), `$SON` po UART0 do Yanosika, BLE (TrovaLaSonda). Build: Flash 4MB, PSRAM Disabled, Partition „Huge APP (3MB/1MB SPIFFS)", USB CDC On Boot: Enabled, biblioteka NimBLE-Arduino. |
| **Yanosik SX** (ESP32-S3 JC3248W535C) | — | wstrzymane | Główne urządzenie (LCD, podgląd sondy, flasher mini). Niewrzucone — repo publiczne, a firmware niesie wrażliwe elementy. |

## Obraz do flashowania mini
Gotowy, przycięty obraz `rdz_s3.bin` leży w katalogu głównym repo i jest pobierany
przez Yanosika (Menu → SONDA → WGRAJ Z GITHUB) z:
`https://raw.githubusercontent.com/lazersaint/yanosik/main/rdz_s3.bin`

Źródło mini: katalog [`../rdz_s3/`](../rdz_s3/).
