#pragma once
#include "sonde_bridge.h"

// Nadajnik BLE zgodny z apka TrovaLaSonda/MySondy (te same UUID-y i format Packet).
// Dzieki temu telefon widzi nasze urzadzenie jak natywny odbiornik i dostaje pozycje.
// Wymaga biblioteki NimBLE-Arduino (gdy SONDA_BLE=1 w config.h).
void sonde_ble_begin();                       // init serwera BLE + reklama
void sonde_ble_update(const SondeView* v);    // notify Packet/RSSI z biezacych danych
void sonde_ble_stop();                        // stop reklamy
bool sonde_ble_connected();                   // czy apka jest polaczona
bool sonde_ble_ready();                        // czy serwer BLE wystartowal (reklamuje)
