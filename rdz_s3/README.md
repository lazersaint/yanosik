# rdz_s3 — firmware plytki sond (ESP32-S3 SuperMini + RA-02)

Dedykowana plytka: cala robota sondowa tutaj, Yanosik tylko pokazuje wynik.

## Co robi
1. Dekoduje RS41 (nasz port rdz — ten sam, co zdekodowal X3852437).
2. Nadaje pozycje po **BLE** do apki TrovaLaSonda / MySondy (tu RAM jest wolny, wiec BLE dziala).
3. Wypluwa gotowa linijke **`$SON,...`** po UART do Yanosika.

## Polaczenia RA-02 (SPRZETOWE SPI)
| RA-02 | SuperMini |
|-------|-----------|
| SCK   | GP12 |
| MISO  | GP13 |
| MOSI  | GP11 |
| NSS   | GP10 |
| RST   | GP9  |
| DIO0  | GP8  |
| GND   | GND  |
| 3V3   | 3V3  |

## Link do Yanosika (UART1 na GP43/44)
| SuperMini | Yanosik |
|-----------|---------|
| GP43 (TX) | pin 15 (RX) |
| GP44 (RX) | pin 16 (TX) |
| GND       | GND |

To **te same druty**, ktorych uzywa bootloader przy wgrywaniu z Yanosika
(EN=GP45? -> patrz flasher; RST/BOOT osobno). Dane lecą po tych samych liniach.

## Ustawienia w Arduino IDE
- Plytka: **ESP32S3 Dev Module**
- **Flash Size: 4MB (32Mb)**  ← SuperMini ma 4 MB! 16 MB = boot-loop (spi_flash probe failed)
- **PSRAM: Disabled**          ← SuperMini bez PSRAM; wlaczony = init fail
- **USB CDC On Boot: Enabled** (debug na USB; $SON i tak idzie po Serial0/UART0 43/44)
- **Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"** ← MUSI miescic sie w 4MB!
  (schemat >4MB = blad "partition invalid ... exceeds flash chip size 0x400000")
- Biblioteka: **NimBLE-Arduino** (Library Manager)

## Format linijki UART (`$SON`)
```
$SON,RS41,<serial>,<lat>,<lon>,<alt_m>,<vs>,<hs>,<sats>,<rssi_dbm>*HH
```
- `HH` = suma XOR wszystkich znakow miedzy `$` a `*` (styl NMEA).
- Bez pozycji (tylko nr + RSSI): puste pola, np. `$SON,RS41,X3852437,,,,,,,-44*..`
- Heartbeat co 3 s nawet bez swiezej ramki.

## Jak zrobic rdz.bin na SD (dla flashera w Yanosiku)
Arduino IDE → **Sketch → Export Compiled Binary**. Powstanie `rdz_s3.ino.bin`
(w podfolderze `build/...`). To jest obraz do wgrania; skopiuj na karte jako `rdz.bin`.
```
