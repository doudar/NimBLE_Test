
/**
 *  NimBLE_Server Demo:
 *
 *  Demonstrates many of the available features of the NimBLE server library.
 *
 *  Created: on March 22 2020
 *      Author: H2zero
 */
#ifdef SERVER_ONLY
#include <Arduino.h>
#include <NimBLEDevice.h>

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

// Cycling Power Service
#define CYCLINGPOWERSERVICE_UUID NimBLEUUID((uint16_t)0x1818)
#define CYCLINGPOWERMEASUREMENT_UUID NimBLEUUID((uint16_t)0x2A63)
#define CYCLINGPOWERFEATURE_UUID NimBLEUUID((uint16_t)0x2A65)

/* These Work */
NimBLEUUID serviceToTest1 = CYCLINGPOWERSERVICE_UUID;
NimBLEUUID charToTest1 = CYCLINGPOWERMEASUREMENT_UUID;

/* These Don't Work*/
NimBLEUUID serviceToTest2 = FITNESSMACHINESERVICE_UUID;
NimBLEUUID charToTest2 = FITNESSMACHINEINDOORBIKEDATA_UUID;

static NimBLEServer *pServer;

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
    {
        Serial.printf("Client address: %s\n", connInfo.getAddress().toString().c_str());
        pServer->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 180);
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
    {
        Serial.printf("Client disconnected - start advertising\n");
        NimBLEDevice::startAdvertising();
    }

    void onMTUChange(uint16_t MTU, NimBLEConnInfo &connInfo) override
    {
        Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, connInfo.getConnHandle());
    }
} serverCallbacks;

/** Handler class for characteristic actions */
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        Serial.printf("%s : onRead(), value: %s\n",
                      pCharacteristic->getUUID().toString().c_str(),
                      pCharacteristic->getValue().c_str());
    }

    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        Serial.printf("%s : onWrite(), value: %s\n",
                      pCharacteristic->getUUID().toString().c_str(),
                      pCharacteristic->getValue().c_str());
    }

    /**
     *  The value returned in code is the NimBLE host return code.
     */
    void onStatus(NimBLECharacteristic *pCharacteristic, int code) override
    {
        Serial.printf("Notification/Indication return code: %d, %s\n", code, NimBLEUtils::returnCodeToString(code));
    }

    /** Peer subscribed to notifications/indications */
    void onSubscribe(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo, uint16_t subValue) override
    {
        std::string str = "Client ID: ";
        str += connInfo.getConnHandle();
        str += " Address: ";
        str += connInfo.getAddress().toString();
        if (subValue == 0)
        {
            str += " Unsubscribed to ";
        }
        else if (subValue == 1)
        {
            str += " Subscribed to notifications for ";
        }
        else if (subValue == 2)
        {
            str += " Subscribed to indications for ";
        }
        else if (subValue == 3)
        {
            str += " Subscribed to notifications and indications for ";
        }
        str += std::string(pCharacteristic->getUUID());

        Serial.printf("%s\n", str.c_str());
    }
} chrCallbacks;

/** Handler class for descriptor actions */
class DescriptorCallbacks : public NimBLEDescriptorCallbacks
{
    void onWrite(NimBLEDescriptor *pDescriptor, NimBLEConnInfo &connInfo) override
    {
        std::string dscVal = pDescriptor->getValue();
        Serial.printf("Descriptor written value: %s\n", dscVal.c_str());
    }

    void onRead(NimBLEDescriptor *pDescriptor, NimBLEConnInfo &connInfo) override
    {
        Serial.printf("%s Descriptor read\n", pDescriptor->getUUID().toString().c_str());
    }
} dscCallbacks;

void setup(void)
{
    Serial.begin(115200);
    Serial.printf("Starting NimBLE Server\n");

    /** Initialize NimBLE and set the device name */
    NimBLEDevice::init("NimBLE");

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);

    // Create first service
    NimBLEService *pBaadService = pServer->createService(serviceToTest1);
    NimBLECharacteristic *pFoodCharacteristic =
        pBaadService->createCharacteristic(charToTest1, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);

    pFoodCharacteristic->setValue("Fries");
    pFoodCharacteristic->setCallbacks(&chrCallbacks);

    // Create second service
    NimBLEService *pFitnessService = pServer->createService(serviceToTest2);

    // Indoor Bike Data
    NimBLECharacteristic *pBikeCharacteristic =
        pFitnessService->createCharacteristic(charToTest2, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
    pBikeCharacteristic->setValue("Doesn't Always Show");
    pBikeCharacteristic->setCallbacks(&chrCallbacks);

    // Fitness Machine Feature
    NimBLECharacteristic *pFeatureCharacteristic =
        pFitnessService->createCharacteristic(FITNESSMACHINEFEATURE_UUID, NIMBLE_PROPERTY::READ);
    pFeatureCharacteristic->setValue("0000"); // Default features
    pFeatureCharacteristic->setCallbacks(&chrCallbacks);

    // Fitness Machine Control Point
    NimBLECharacteristic *pControlPointCharacteristic =
        pFitnessService->createCharacteristic(FITNESSMACHINECONTROLPOINT_UUID, NIMBLE_PROPERTY::WRITE, NIMBLE_PROPERTY::INDICATE);
    pControlPointCharacteristic->setCallbacks(&chrCallbacks);

    /* Fitness Machine Status
    NimBLECharacteristic *pStatusCharacteristic =
        pFitnessService->createCharacteristic(FITNESSMACHINESTATUS_UUID, NIMBLE_PROPERTY::NOTIFY);
    pStatusCharacteristic->setCallbacks(&chrCallbacks);
   */
    // Training Status
    NimBLECharacteristic *pTrainingStatusCharacteristic =
        pFitnessService->createCharacteristic(FITNESSMACHINETRAININGSTATUS_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    pTrainingStatusCharacteristic->setValue("0"); // Default status
    pTrainingStatusCharacteristic->setCallbacks(&chrCallbacks);
    /*
 // Resistance Level Range
 NimBLECharacteristic *pResistanceRangeCharacteristic =
     pFitnessService->createCharacteristic(FITNESSMACHINERESISTANCELEVELRANGE_UUID, NIMBLE_PROPERTY::READ);
 pResistanceRangeCharacteristic->setValue("0-100"); // Example range
 pResistanceRangeCharacteristic->setCallbacks(&chrCallbacks);

 // Power Range
 NimBLECharacteristic *pPowerRangeCharacteristic =
     pFitnessService->createCharacteristic(FITNESSMACHINEPOWERRANGE_UUID, NIMBLE_PROPERTY::READ);
 pPowerRangeCharacteristic->setValue("0-1000"); // Example range in watts
 pPowerRangeCharacteristic->setCallbacks(&chrCallbacks);

 // Inclination Range
 NimBLECharacteristic *pInclinationRangeCharacteristic =
     pFitnessService->createCharacteristic(FITNESSMACHINEINCLINATIONRANGE_UUID, NIMBLE_PROPERTY::READ);
 pInclinationRangeCharacteristic->setValue("-10-10"); // Example range in degrees
 pInclinationRangeCharacteristic->setCallbacks(&chrCallbacks);*/

    /** Start the services when finished creating all Characteristics and Descriptors */
    pBaadService->start();
    pFitnessService->start();

    /** Create an advertising instance and add the services to the advertised data */
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName("NimBLE-Server");
    pAdvertising->addServiceUUID(pBaadService->getUUID());
    pAdvertising->addServiceUUID(pFitnessService->getUUID());
    /**
     *  If your device is battery powered you may consider setting scan response
     *  to false as it will extend battery life at the expense of less data sent.
     */
    pAdvertising->enableScanResponse(true);
    pAdvertising->start();

    Serial.printf("Advertising Started\n");
}

void loop()
{
    /** Loop here and send notifications to connected peers */
    delay(2000);

    if (pServer->getConnectedCount())
    {
        // Notify for first service
        NimBLEService *pSvc1 = pServer->getServiceByUUID(serviceToTest1);
        if (pSvc1)
        {
            NimBLECharacteristic *pChr1 = pSvc1->getCharacteristic(charToTest1);
            if (pChr1)
            {
                pChr1->notify();
            }
        }

        // Notify for second service
        NimBLEService *pSvc2 = pServer->getServiceByUUID(serviceToTest2);
        if (pSvc2)
        {
            NimBLECharacteristic *pChr2 = pSvc2->getCharacteristic(charToTest2);
            if (pChr2)
            {
                pChr2->notify();
            }
        }
    }
}
#endif