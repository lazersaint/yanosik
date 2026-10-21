#include "../config.h"
#include "sonde_ble.h"

#if SONDA_BLE

#include <NimBLEDevice.h>
#include <Arduino.h>
#include <string.h>

// UUID-y 1:1 z TrovaLaSondaFw/Ble.cpp — apka filtruje po SERVICE_UUID
#define SERVICE_UUID  "177fba78-7843-40a6-801b-a4cd8d7f5c11"
#define PACKET_UUID   "4dee4a71-2e7e-4018-9656-b60f1e562047"
#define BATT_UUID     "4578ee77-f50f-4584-b59c-46264c56d949"
#define RSSI_UUID     "e482dfeb-774f-4f8b-8eea-87a752326fbd"
#define TYPEFREQ_UUID "66bf4d7f-2b21-468d-8dce-b241c7447cc6"
#define MUTE_UUID     "a8b47819-eb1a-4b5c-8873-6258ddfe8055"
#define VERSION_UUID  "2bc3ed96-a00a-4c9a-84af-7e1283835d71"

// Format pakietu 1:1 z TrovaLaSondaFw.h (apka parsuje bajt w bajt) — packed!
struct __attribute__((packed)) TPacket {
    int      frame;
    double   lat, lng;
    float    alt, hVel, vVel;
    uint16_t bkTime;
    uint8_t  bkStatus, cpuTemp, radioTemp;
    bool     encrypted;
    char     serial[13];     // SERIAL_LENGTH(12)+1
};

static NimBLECharacteristic *pPacket = nullptr, *pBatt = nullptr, *pRSSI = nullptr,
                            *pTypeFreq = nullptr, *pMute = nullptr, *pVer = nullptr;
static volatile bool _connected = false;
static bool _inited = false;

class SrvCB : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer*, NimBLEConnInfo&) override    { _connected = true; }
    void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) override { _connected = false; }
};

void sonde_ble_begin() {
    if (_inited) { NimBLEDevice::getAdvertising()->start(); return; }

    Serial.printf("[BLE] free heap przed init: %u  largest=%u\n",
                  (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
    NimBLEDevice::init("");
    NimBLEDevice::setMTU(512);

    NimBLEServer* srv = NimBLEDevice::createServer();
    srv->setCallbacks(new SrvCB());
    srv->advertiseOnDisconnect(true);

    NimBLEService* s = srv->createService(SERVICE_UUID);
    pPacket   = s->createCharacteristic(PACKET_UUID,   NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pBatt     = s->createCharacteristic(BATT_UUID,     NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pRSSI     = s->createCharacteristic(RSSI_UUID,     NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pTypeFreq = s->createCharacteristic(TYPEFREQ_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
    pMute     = s->createCharacteristic(MUTE_UUID,     NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);
    pVer      = s->createCharacteristic(VERSION_UUID,  NIMBLE_PROPERTY::READ);
    pVer->setValue(std::string("YanosikSX/1.0"));
    pBatt->setValue((int)0);
    pMute->setValue((int)0);
    s->start();

    NimBLEAdvertising* adv = srv->getAdvertising();
    NimBLEAdvertisementData advData, scanData;
    advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
    advData.addServiceUUID(SERVICE_UUID);
    scanData.setName("TrovaLaSondaYSX");     // apka i tak filtruje po SERVICE_UUID
    adv->enableScanResponse(true);
    adv->setAdvertisementData(advData);
    adv->setScanResponseData(scanData);
    adv->start();

    _inited = true;
    Serial.println("[BLE] serwer gotowy (TrovaLaSonda compat)");
}

void sonde_ble_update(const SondeView* v) {
    if (!_inited || !_connected || !v) return;
    static TPacket pkt;
    memset(&pkt, 0, sizeof pkt);
    pkt.frame     = 0;
    pkt.lat       = v->lat;
    pkt.lng       = v->lon;
    pkt.alt       = v->alt;
    pkt.hVel      = v->hs;
    pkt.vVel      = v->vs;
    pkt.encrypted = false;
    strncpy(pkt.serial, v->id, sizeof(pkt.serial) - 1);

    pPacket->setValue((uint8_t*)&pkt, sizeof pkt);
    pPacket->notify();
    pRSSI->setValue((int)v->rssi_dbm);
    pRSSI->notify();
}

void sonde_ble_stop() {
    if (_inited) NimBLEDevice::getAdvertising()->stop();
}

bool sonde_ble_connected() { return _connected; }
bool sonde_ble_ready()     { return _inited; }

#else  // SONDA_BLE == 0 — puste stuby (bez zaleznosci NimBLE)
void sonde_ble_begin() {}
void sonde_ble_update(const SondeView*) {}
void sonde_ble_stop() {}
bool sonde_ble_connected() { return false; }
bool sonde_ble_ready()     { return false; }
#endif
