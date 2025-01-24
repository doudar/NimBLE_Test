
/** NimBLE_Client Demo:
 *
 *  Demonstrates many of the available features of the NimBLE client library.
 *
 *  Created: on March 24 2020
 *      Author: H2zero
 */
#ifdef CLIENT_ONLY
#include <Arduino.h>
#include <NimBLEDevice.h>
#include <vector>

static const NimBLEAdvertisedDevice *advDevice;
static bool doConnect = false;
static uint32_t scanTimeMs = 5000; /** scan time in milliseconds, 0 = scan forever */

// Heart Service
#define HEARTSERVICE_UUID NimBLEUUID((uint16_t)0x180D)
#define HEARTCHARACTERISTIC_UUID NimBLEUUID((uint16_t)0x2A37)

// BatteryLevel Service
#define BATTERYSERVICE_UUID NimBLEUUID((uint16_t)0x180F) // heart rate sensor service uuid, as defined in gatt specifications
#define BATTERYCHARACTERISTIC_UUID NimBLEUUID((uint16_t)0x2A19)

// Cycling Speed And Cadence
#define CSCSERVICE_UUID NimBLEUUID((uint16_t)0x1816)
#define CSCMEASUREMENT_UUID NimBLEUUID((uint16_t)0x2A5B)
#define CSCFEATURE_UUID NimBLEUUID((uint16_t)0x2A5C)
#define SENSORLOCATION_UUID NimBLEUUID((uint16_t)0x2A5D)
#define CSCCONTROLPOINT_UUID NimBLEUUID((uint16_t)0x2A55)

// Cycling Power Service
#define CYCLINGPOWERSERVICE_UUID NimBLEUUID((uint16_t)0x1818)
#define CYCLINGPOWERMEASUREMENT_UUID NimBLEUUID((uint16_t)0x2A63)
#define CYCLINGPOWERFEATURE_UUID NimBLEUUID((uint16_t)0x2A65)

// Fitness Machine Service
#define FITNESSMACHINESERVICE_UUID NimBLEUUID((uint16_t)0x1826)
#define FITNESSMACHINEFEATURE_UUID NimBLEUUID((uint16_t)0x2ACC)
#define FITNESSMACHINECONTROLPOINT_UUID NimBLEUUID((uint16_t)0x2AD9)
#define FITNESSMACHINESTATUS_UUID NimBLEUUID((uint16_t)0x2ADA)
#define FITNESSMACHINEINDOORBIKEDATA_UUID NimBLEUUID((uint16_t)0x2AD2)
#define FITNESSMACHINETRAININGSTATUS_UUID NimBLEUUID((uint16_t)0x2AD3)
#define FITNESSMACHINERESISTANCELEVELRANGE_UUID NimBLEUUID((uint16_t)0x2AD6)
#define FITNESSMACHINEPOWERRANGE_UUID NimBLEUUID((uint16_t)0x2AD8)
#define FITNESSMACHINEINCLINATIONRANGE_UUID NimBLEUUID((uint16_t)0x2AD5)

// Wattbike Service
#define WATTBIKE_SERVICE_UUID NimBLEUUID("b4cc1223-bc02-4cae-adb9-1217ad2860d1")
#define WATTBIKE_READ_UUID NimBLEUUID("b4cc1224-bc02-4cae-adb9-1217ad2860d1")
#define WATTBIKE_WRITE_UUID NimBLEUUID("b4cc1225-bc02-4cae-adb9-1217ad2860d1")

// GATT service/characteristic UUIDs for Flywheel Bike from ptx2/gymnasticon/
#define FLYWHEEL_UART_SERVICE_UUID NimBLEUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e")
#define FLYWHEEL_UART_RX_UUID NimBLEUUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e")
#define FLYWHEEL_UART_TX_UUID NimBLEUUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e")
#define FLYWHEEL_BLE_NAME "Flywheel 1"

// The Echelon Services
#define ECHELON_DEVICE_UUID NimBLEUUID("0bf669f0-45f2-11e7-9598-0800200c9a66")
#define ECHELON_SERVICE_UUID NimBLEUUID("0bf669f1-45f2-11e7-9598-0800200c9a66")
#define ECHELON_WRITE_UUID NimBLEUUID("0bf669f2-45f2-11e7-9598-0800200c9a66")
#define ECHELON_DATA_UUID NimBLEUUID("0bf669f4-45f2-11e7-9598-0800200c9a66")

#define HID_SERVICE_UUID NimBLEUUID((uint16_t)0x1812)
#define HID_REPORT_DATA_UUID NimBLEUUID((uint16_t)0x2A4D)

// Vector of supported BLE services and their corresponding characteristic UUIDs
struct BLEServiceInfo
{
    BLEUUID serviceUUID;
    BLEUUID characteristicUUID;
    String name;
};

namespace BLEServices
{
    const std::vector<BLEServiceInfo> SUPPORTED_SERVICES = {{CYCLINGPOWERSERVICE_UUID, CYCLINGPOWERMEASUREMENT_UUID, "Cycling Power Service"},
                                                            {CSCSERVICE_UUID, CSCMEASUREMENT_UUID, "Cycling Speed And Cadence Service"},
                                                            {HEARTSERVICE_UUID, HEARTCHARACTERISTIC_UUID, "Heart Rate Service"},
                                                            {FITNESSMACHINESERVICE_UUID, FITNESSMACHINEINDOORBIKEDATA_UUID, "Fitness Machine Service"},
                                                            {HID_SERVICE_UUID, HID_REPORT_DATA_UUID, "HID Service"},
                                                            {ECHELON_SERVICE_UUID, ECHELON_DATA_UUID, "Echelon Service"},
                                                            {FLYWHEEL_UART_SERVICE_UUID, FLYWHEEL_UART_TX_UUID, "Flywheel UART Service"}};
}

using BLEServices::SUPPORTED_SERVICES;

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ClientCallbacks : public NimBLEClientCallbacks
{
    void onConnect(NimBLEClient *pClient) override { Serial.printf("Connected\n"); }

    void onDisconnect(NimBLEClient *pClient, int reason) override
    {
        Serial.printf("%s Disconnected, reason = %d - Starting scan\n", pClient->getPeerAddress().toString().c_str(), reason);
        NimBLEDevice::getScan()->start(scanTimeMs, false, true);
    }
} clientCallbacks;

/** Define a class to handle the callbacks when scan events are received */
class ScanCallbacks : public NimBLEScanCallbacks
{
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override
    {
        Serial.printf("Advertised Device found: %s\n", advertisedDevice->toString().c_str());
        for (const auto &service : SUPPORTED_SERVICES)
        {
            if (advertisedDevice->isAdvertisingService(service.serviceUUID))
            {
                Serial.printf("Found Supported Service: %s\n", service.name.c_str());
                /** stop scan before connecting */
                NimBLEDevice::getScan()->stop();
                /** Save the device reference in a global for the client to use*/
                advDevice = advertisedDevice;
                /** Ready to connect now */
                doConnect = true;
                break;
            }
        }
    }

    /** Callback to process the results of the completed scan or restart it */
    void onScanEnd(const NimBLEScanResults &results, int reason) override
    {
        Serial.printf("Scan Ended, reason: %d, device count: %d; Restarting scan\n", reason, results.getCount());
        NimBLEDevice::getScan()->start(scanTimeMs, false, true);
    }
} scanCallbacks;

/** Notification / Indication receiving handler callback */
void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    std::string str = (isNotify == true) ? "Notification" : "Indication";
    str += " from ";
    str += pRemoteCharacteristic->getClient()->getPeerAddress().toString();
    str += ": Service = " + pRemoteCharacteristic->getRemoteService()->getUUID().toString();
    str += ", Characteristic = " + pRemoteCharacteristic->getUUID().toString();
    str += ", Value = " + std::string((char *)pData, length);
    Serial.printf("%s\n", str.c_str());
}

// Loop through all Supported Services and subscribe to notifications
void subscribeToAllNotifications(NimBLEClient *pClient)
{
    if (!pClient || !pClient->isConnected())
    {
        Serial.printf("Client not connected for notifications\n");
        return;
    }
    for (const auto &service : BLEServices::SUPPORTED_SERVICES)
    {
        NimBLERemoteService *pSvc = pClient->getService(service.serviceUUID);
        if (pSvc)
        {
            Serial.printf("Found %s\n", service.name.c_str());
            for (const auto &pChr : pSvc->getCharacteristics(true))
            {
                if (pChr)
                {
                    Serial.printf("Found %s, %s\n", service.serviceUUID.toString().c_str(), pChr->getUUID().toString().c_str());
                    if ((pChr->canNotify() || pChr->canIndicate()))
                    {
                        if (pChr->canIndicate())
                        {
                            if (pChr->subscribe(false, notifyCB, false))
                            {
                                Serial.printf("Indications setup for %s %s handle: %d\n", service.name.c_str(), pChr->getUUID().toString().c_str(), pChr->getHandle());
                            }
                            else
                            {
                                Serial.printf("Indications Failed for %s %s handle: %d\n", service.name.c_str(), pChr->getUUID().toString().c_str(), pChr->getHandle());
                            }
                        }
                        else // Setup Notifications
                        {
                            if (pChr->subscribe(true, notifyCB, false))
                            {
                                Serial.printf("Subscribed to %s %s handle: %d\n", service.name.c_str(), pChr->getUUID().toString().c_str(), pChr->getHandle());
                            }
                            else
                            {
                                Serial.printf("Failed to subscribe to %s %s handle: %d\n", service.name.c_str(), pChr->getUUID().toString().c_str(), pChr->getHandle());
                            }
                        }
                    }
                }
            }
        }
    }
}

/** Handles the provisioning of clients and connects / interfaces with the server */
bool connectToServer()
{
    NimBLEClient *pClient = nullptr;

    /** Check if we have a client we should reuse first **/
    if (NimBLEDevice::getCreatedClientCount())
    {
        /**
         *  Special case when we already know this device, we send false as the
         *  second argument in connect() to prevent refreshing the service database.
         *  This saves considerable time and power.
         */
        pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
        if (pClient)
        {
            if (!pClient->connect(advDevice, false))
            {
                Serial.printf("Reconnect failed\n");
                return false;
            }
            Serial.printf("Reconnected client\n");
        }
        else
        {
            /**
             *  We don't already have a client that knows this device,
             *  check for a client that is disconnected that we can use.
             */
            pClient = NimBLEDevice::getDisconnectedClient();
        }
    }

    /** No client to reuse? Create a new one. */
    if (!pClient)
    {
        if (NimBLEDevice::getCreatedClientCount() >= NIMBLE_MAX_CONNECTIONS)
        {
            Serial.printf("Max clients reached - no more connections available\n");
            return false;
        }

        pClient = NimBLEDevice::createClient();

        Serial.printf("New client created\n");

        pClient->setClientCallbacks(&clientCallbacks, false);
        /**
         *  Set initial connection parameters:
         *  These settings are safe for 3 clients to connect reliably, can go faster if you have less
         *  connections. Timeout should be a multiple of the interval, minimum is 100ms.
         *  Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 150 * 10ms = 1500ms timeout
         */
        pClient->setConnectionParams(12, 12, 0, 150);

        /** Set how long we are willing to wait for the connection to complete (milliseconds), default is 30000. */
        pClient->setConnectTimeout(5 * 1000);

        if (!pClient->connect(advDevice))
        {
            /** Created a client but failed to connect, don't need to keep it as it has no data */
            NimBLEDevice::deleteClient(pClient);
            Serial.printf("Failed to connect, deleted client\n");
            return false;
        }
    }

    if (!pClient->isConnected())
    {
        if (!pClient->connect(advDevice))
        {
            Serial.printf("Failed to connect\n");
            return false;
        }
    }

    Serial.printf("Connected to: %s RSSI: %d\n", pClient->getPeerAddress().toString().c_str(), pClient->getRssi());

    // subscribeToAllNotifications(pClient);

    Serial.printf("Done with this device!\n");
    return true;
}

void setup()
{
    Serial.begin(115200);
    Serial.printf("Starting NimBLE Client\n");

    /** Initialize NimBLE and set the device name */
    NimBLEDevice::init("NimBLE-Client");

    /**
     * Set the IO capabilities of the device, each option will trigger a different pairing method.
     *  BLE_HS_IO_KEYBOARD_ONLY   - Passkey pairing
     *  BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
     *  BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
     */
    // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY); // use passkey
    // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); //use numeric comparison

    /**
     * 2 different ways to set security - both calls achieve the same result.
     *  no bonding, no man in the middle protection, BLE secure connections.
     *  These are the default values, only shown here for demonstration.
     */
    // NimBLEDevice::setSecurityAuth(false, false, true);

    NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);

    /** Optional: set the transmit power */
    NimBLEDevice::setPower(3); /** 3dbm */
    NimBLEScan *pScan = NimBLEDevice::getScan();

    /** Set the callbacks to call when scan events occur, no duplicates */
    pScan->setScanCallbacks(&scanCallbacks, false);

    /** Set scan interval (how often) and window (how long) in milliseconds */
    pScan->setInterval(100);
    pScan->setWindow(100);

    /**
     * Active scan will gather scan response data from advertisers
     *  but will use more energy from both devices
     */
    pScan->setActiveScan(true);

    /** Start scanning for advertisers */
    pScan->start(scanTimeMs);
    Serial.printf("Scanning for peripherals\n");
}

void loop()
{
    /** Loop here until we find a device we want to connect to */
    delay(10);

    if (doConnect)
    {
        doConnect = false;
        /** Found a device we want to connect to, do it now */
        if (connectToServer())
        {
            vTaskDelay(5000);
            subscribeToAllNotifications(NimBLEDevice::getConnectedClients()[0]);
            Serial.printf("Success! we should now be getting notifications, scanning for more!\n");
        }
        else
        {
            Serial.printf("Failed to connect, starting scan\n");
        }
        if (!NimBLEDevice::getCreatedClientCount())
        {
            NimBLEDevice::getScan()->start(scanTimeMs, false, true);
        }
    }
}
#endif