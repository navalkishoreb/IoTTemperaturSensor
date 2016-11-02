#include <BLE_API.h>
#include <stdio.h>
BLE & ble = BLE::Instance();
Ticker ticker1s;
const short MANUFACTURE_ID = 30721;
const uint8_t REQUIRED_DATA_LENGTH = 8;
const unsigned char bit_mask[8] = {
        1,
        2,
        4,
        8,
        16,
        32,
        64,
        128
};

void printPeerAddress(const BLEProtocol::AddressBytes_t peerAddress) {
        uint8_t index;
        Serial.print("Peer Address: ");
        for (index = 0; index < BLEProtocol::ADDR_LEN; index++) {
                Serial.print(peerAddress[index], HEX);
                Serial.print(" ");
        }
        Serial.println();
}

void printRssi(const int8_t rssi) {
        Serial.print("Rssi: ");
        Serial.println(rssi);
}

void printAdvertisingType(const GapAdvertisingParams::AdvertisingType_t advType) {
        Serial.print("Advertising Type: ");
        switch (advType) {
        case GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED:
                Serial.println("ADV_CONNECTABLE_UNDIRECTED");
                break;
        case GapAdvertisingParams::ADV_CONNECTABLE_DIRECTED:
                Serial.println("ADV_CONNECTABLE_DIRECTED");
                break;
        case GapAdvertisingParams::ADV_SCANNABLE_UNDIRECTED:
                Serial.println("ADV_SCANNABLE_UNDIRECTED");
                break;
        case GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED:
                Serial.println("ADV_NON_CONNECTABLE_UNDIRECTED");
                break;
        default:
                Serial.println("Unknown type");
        }
}

void printAdvertisedData(const uint8_t len,
        const uint8_t * data) {
        Serial.print("Data: ");
        for (int i = 0; i < len; i++) {
                Serial.print(data[i]);
                Serial.print("-");
        }
        Serial.println();
}

void printParticularFlag(int value) {
        switch (value) {
        case 0:
                Serial.println("LE Limited Discoverable Mode");
                break;
        case 1:
                Serial.println("LE General Discoverable Mode");
                break;
        case 2:
                Serial.println("BR/EDR Not Supported.");
                break;
        case 3:
                Serial.println("Simultaneous LE and BR/EDR to Same Device Capable (Controller)");
                break;
        case 4:
                Serial.println("Simultaneous LE and BR/EDR to Same Device Capable (Host)");
                break;
        case 5:
        case 6:
        case 7:
                Serial.println("Reserved");
                break;
        default:
                Serial.println("Flag is not implemented");
        }
}

void printFlags(const uint8_t len,
        const uint8_t * data) {
        uint8_t flag = * data;
        Serial.println("Printing Flags: ");
        for (int i = 0; i < 8; i++) {
                if (flag & bit_mask[i]) {
                        printParticularFlag(i);
                }
        }

}
void printManufactureData(const uint8_t len, const uint8_t * data) {
        
        if (len == 8) {
                short manufactureId = 0;
                long deviceId = 0;
                short temperature = 0;
                uint8_t i = 0;
                Serial.println("Manufacture Specific Data");
              //  manufactureId = (data[0] << 8) | data[1];
                while (i < 2) {
                        manufactureId = manufactureId << 8;
                        manufactureId = manufactureId | data[i++];
                }

                if (manufactureId) {
                        while (i >= 2 && i < 6) {
                                deviceId = deviceId << 8;
                                deviceId = deviceId | data[i++];
                        }
                        while (i >= 6 && i < 8) {
                                temperature = temperature << 8;
                                temperature = temperature | data[i++];
                        }
                } else {
                        Serial.println("Manufacture id does not match");
                }

                Serial.print("Manufacture id ");
                Serial.println(manufactureId, HEX);
                Serial.print("Device id ");
                Serial.println(deviceId, HEX);
                Serial.print("Temperature ");
                Serial.println(temperature);
        } else {
                Serial.println("Data length does not match");
        }
}

void sendManufactureData(const uint8_t len, const uint8_t * data){
   if (len == 8) {
                short manufactureId = 0;
                long deviceId = 0;
                short temperature = 0;
                uint8_t i = 0;                
                while (i < 2) {
                        manufactureId = manufactureId << 8;
                        manufactureId = manufactureId | data[i++];
                }

                if (manufactureId) {
                        while (i >= 2 && i < 6) {
                                deviceId = deviceId << 8;
                                deviceId = deviceId | data[i++];
                        }
                        while (i >= 6 && i < 8) {
                                temperature = temperature << 8;
                                temperature = temperature | data[i++];
                        }
                
                for(i=0;i < 8;i++){
                  if(data[i]<10){
                    Serial.print(0,HEX);
                  }
                  Serial.write(data[i]);    
                  Serial.print("-");
                }
                Serial.print("\n");
                }

        } 
  
}
void printDataField(uint8_t len,
        const uint8_t * data, uint8_t fieldType) {
          Serial.print("Field 0x");
          Serial.print(fieldType,HEX);
          Serial.print(" ");
        switch (fieldType) {
        case 0x01:
                    printFlags(len,data);
                break;
        case 0xFF:
                  printManufactureData(len, data);
                  //sendManufactureData(len,data);
                break;
        }
}

void printAdvertisedData2(const uint8_t len,
        const uint8_t * data) {
        //  Serial.println("Data: ");
        uint8_t dataSize = 0;
        uint8_t fieldType = 0;
        uint8_t i = 0;
        while (i < len) {
                dataSize = data[i] - 1;
                    Serial.print("Data Size: ");
                    Serial.println(dataSize);
                fieldType = data[++i];
                ++i;
                printDataField(dataSize, (data + i), fieldType);
                i = i + dataSize;

        }

}

void advertisementCallback(const Gap::AdvertisementCallbackParams_t * params) {
         printPeerAddress(params->peerAddr);
         printRssi(params->rssi);
         printAdvertisingType(params->type);
         Serial.print("Data length: ");
         Serial.println(params->advertisingDataLen);
         printAdvertisedData(params->advertisingDataLen, params->advertisingData);
        if (params-> type == GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED) {
                printAdvertisedData2(params-> advertisingDataLen, params-> advertisingData);
        } else {
                  Serial.println("Beacon is not of required advertising type");
        }
         Serial.println("---------------------------------------------------------------");
}

void bleInitComplete(BLE::InitializationCompleteCallbackContext * params) {

}

void setup() {
        // put your setup code here, to run once:
        Serial.begin(9600);
        Serial.println("setting up...");
        ble.init();
        ble.gap().setScanParams();
        ble.gap().startScan(advertisementCallback);
        ble.gap().setDeviceName((const uint8_t * )
                "Observer");

}

void loop() {
        // put your main code here, to run repeatedly:
        ble.waitForEvent();
}

