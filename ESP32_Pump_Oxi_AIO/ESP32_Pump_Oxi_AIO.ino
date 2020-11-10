#include "BLEDevice.h"
//#include "BLEScan.h"

#include "BluetoothSerial.h" //Header File for Serial Bluetooth, will be added by default into Arduino

BluetoothSerial SerialBT; //Object for Bluetooth

int incoming;
int LED_BUILTIN = 2;
String str;

// The remote service we wish to connect to.
static BLEUUID serviceUUID("cdeacb80-5235-4c07-8846-93a37ee6b86d");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("CDEACB81-5235-4C07-8846-93A37EE6B86D");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
static unsigned int connectionTimeMs = 0;

int bpm;
int spo2;
float pi;

float u1, u2, u3, u4, u5, u6, u7;


static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  if (pData[0] == 0x81) {
    bpm = pData[1];
    spo2 = pData[2];
    pi = pData[3];

    u1 = pData[4];
    u2 = pData[5];
    u3 = pData[6];
    u4 = pData[7];
    u5 = pData[8];
    u6 = pData[9];
    u7 = pData[10];
  }
}

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
    }
    void onDisconnect(BLEClient* pclient) {
      connected = false;
      Serial.println("onDisconnect");
    }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient*  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");


  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic.
  if (pRemoteCharacteristic->canRead()) {
    std::string value = pRemoteCharacteristic->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  connected = true;
  return true;
}

/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    /**
        Called for each advertising BLE server.
    */
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertisedDevice.toString().c_str());

      // We have found a device, let us now see if it contains the service we are looking for.
      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = true;

      } // Found our server
    } // onResult
}; // MyAdvertisedDeviceCallbacks

void setup() {
  BLEDevice::init("");
  Serial.begin(115200);

  connectionTimeMs = millis();

  Serial.println("Starting Arduino BLE Client application...");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

  if (!SerialBT.begin("ESP32_PUMP_Control")) {
    Serial.println("An error occurred initializing Bluetooth");
  } else {
    Serial.println("Bluetooth initialized");
  }

  //Serial.println("#W0,0\n"); //Disable the pump (!can't do that otherwise pump won't turn on forever)
  Serial.println("#W18,0\n"); //Use Set Val (register 23) as the input to the bang bang controller
  Serial.println("#W19,10\n"); //Set the lower threshold to 10
  Serial.println("#W20,100\n"); //Set the upper threshold to 100
  Serial.println("#W21,1000\n"); //Set the power at the lower threshold to 1 watt
  Serial.println("#W22,0\n"); //Turn the pump off when the upper threshold is reached
  delay(20);
  Serial.println("#W23,1000\n"); //Start by initially turned off pump by setting “Set Value” well above the threshold value
  SerialBT.println("Initial state: PUMP is OFF");

  pinMode (LED_BUILTIN, OUTPUT);//Specify that LED pin is output

} // End of setup.


// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    String newValue = "Time since boot: " + String(millis() / 1000);
    SerialBT.println(newValue);
    SerialBT.print("SPO2: " + String(spo2));
    SerialBT.print("     BPM: " + String(bpm));
    SerialBT.println("    PI: " + String(pi / 10.0));


    SerialBT.print("u1: " + String(u1));
    SerialBT.print("     u2: " + String(u2));
    SerialBT.println("    u3: " + String(u3));
    SerialBT.print("u4: " + String(u4));
    SerialBT.print("     u5: " + String(u5));
    SerialBT.println("    u6: " + String(u6));
    SerialBT.println("    u7: " + String(u7));

    // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  } else if (doScan) {
    //BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }

  SerialBT.register_callback(callback);

  while (SerialBT.available()) {
    delay(1);
    char c = SerialBT.read();
    str += c;
  }

  str.trim();
  //Serial.println(str);

  if (str.length() > 0) {
    if (str == "scan") {
      Serial.println("Scan started ... ");

      if (!connected) {
        if (doScan) {
          BLEDevice::getScan()->start(5);  // this is just an example to start scan after disconnect, most likely there is better way to do it in arduino
        }
        else { // enable connects if no device was found on first boot
          //if (millis() > connectionTimeMs + 6000) {
            Serial.println("Enabling scanning.");
            doScan = true;
          //}
        }
      }
    }

    else if (str == "reset") {
      SerialBT.println("Resetting device in");
      delay(500);
      SerialBT.println("3");
      delay(1000);
      SerialBT.println("2");
      delay(1000);
      SerialBT.println("1");
      delay(1000);      
      esp_restart();
    }
    
    else if (str == "on") {
      Serial.println("#W23,9");
    }

    else if (str == "off") {
      Serial.println("#W23,101");
    }

    Serial.println("\n");
    str = "";
  }


  delay(100); // Delay a second between loops.
} // End of loop


void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("Client Connected");
  }

  if (event == ESP_SPP_CLOSE_EVT ) {
    Serial.println("Client disconnected: PUMP is OFF");
    //Serial.println("#W0,0\n"); //Disables the pump
    Serial.println("#W23,1000\n"); //Turn off the pump by setting “Set Value” well above the threshold value
  }
}
