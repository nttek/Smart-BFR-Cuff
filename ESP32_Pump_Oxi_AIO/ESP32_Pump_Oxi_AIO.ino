#include "BLEDevice.h"
//#include "BLEScan.h"

#include "BluetoothSerial.h" //Header File for Serial Bluetooth, will be added by default into Arduino
//
//#include <TimeLib.h>
//time_t startTime = 0;
//time_t timeElapsed = 0;

BluetoothSerial SerialBT; //Object for Bluetooth

int incoming;
int LED_BUILTIN = 2;
String str;
static unsigned int LOP = 105;
int currentPressure = 0;

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

int plethArray[15];
int mergedPlethArray[50];
int plethArrayLength = sizeof(plethArray) / sizeof(int);
boolean plethValid = true;
int plethSum = 0;
float plethAvg = 0;
int z = 1;

boolean lopAckFlag = false;

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {

  //  Serial.print("data: ");
  //  for (int i = 0; i < length; i++) {
  //    Serial.print(pData[i]);
  //    Serial.print(" ");
  //  }
  //
  //  Serial.println("");
  //
  if (pData[0] == 0x80) {
    //reset array contents
    for (int l = 0; l < plethArrayLength; l++) {
      plethArray[0];
    }

    //store incoming pleth data into array
    for (int i = 1; i < length; i++) {
      //Serial.print(pData[i]);
      //Serial.println("");;

      if (length < plethArrayLength) {
        plethArray[i - 1] = pData[i];
        plethValid = true;
      }
      else {
        plethValid = false;
        //Serial.println("Outbound");
      }
    }

    //h
    //    for (int l = 0; l < plethArrayLength; l++) {
    //      Serial.print(plethArray[l]);
    //      Serial.print(" ");
    //    }
    //
    //    Serial.println();


    int maxValue = plethArray[0];  // initialize the maximum as the first element

    for (int idx = 0; idx < plethArrayLength; ++idx)
    {
      if ( plethArray[ idx ] > maxValue)
      {
        maxValue = plethArray[idx];
      }
    }

    //Serial.println("MAX: " + String(maxValue));

    if (plethValid && z != 12) {
      plethSum += maxValue;
      z++;
    }

    else if (plethValid && z == 12) {
      plethSum += maxValue;
      plethAvg = plethSum / 12;
      z = 0;
      plethSum = 0;
      //Serial.println("AVG: " + String(plethAvg));
    }
    maxValue = 0;

  }




  //}

  //
  //  if (pData[0] == 0x81) {
  //    bpm = pData[1];
  //    spo2 = pData[2];
  //    pi = pData[3];
  //
  //    u1 = pData[4];
  //    u2 = pData[5];
  //    u3 = pData[6];
  //    u4 = pData[7];
  //    u5 = pData[8];
  //    u6 = pData[9];
  //    u7 = pData[10];
  //  }
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

  Serial.println("#W18,0\n"); //Use Set Val (register 23) as the input to the bang bang controller
  Serial.println("#W19,10\n"); //Set the lower threshold to 10
  Serial.println("#W20,100\n"); //Set the upper threshold to 100
  Serial.println("#W21,1000\n"); //Set the power at the lower threshold to 1 watt
  Serial.println("#W22,0\n"); //Turn the pump off when the upper threshold is reached
  delay(20);
  Serial.println("#W23,1000\n"); //Start by initially turned off pump by setting “Set Value” well above the threshold value
  Serial.println("Initial state: PUMP is OFF");

  pinMode (LED_BUILTIN, OUTPUT);//Specify that LED pin is output

} // End of setup.


// This is the Arduino main loop function.
void loop() {

  //  // If the flag "doConnect" is true then we have scanned for and found the desired
  //  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  //  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      SerialBT.println("We are now connected to the BLE Server.");
    } else {
      SerialBT.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  //  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  //  // with the current time since boot.
  //  if (connected) {
  //    String newValue = "Time since boot: " + String(millis() / 1000);
  //    SerialBT.println(newValue);
  //    //    SerialBT.print("SPO2: " + String(spo2));
  //    //    SerialBT.print("     BPM: " + String(bpm));
  //    //    SerialBT.println("    PI: " + String(pi / 10.0));
  //
  //    // Set the characteristic's value to be the array of bytes that is actually a string.
  //    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  //  }

  SerialBT.register_callback(callback);

  while (SerialBT.available()) {
    delay(1);
    char c = SerialBT.read();
    str += c;
  }

  if (str.length() > 0) {

    str.trim();
    str.toLowerCase();
    Serial.println(str);

    if (str.substring(0, 3) == "set") {
      //expecting pressure data in the form of set=90
      int x = str.substring(4).toInt();
      setPres(x);
    }

    else if (str.substring(0, 9) == "calibrate") {
      //expecting instruction to calibrate LOP
      lopAckFlag = false;
      lopSequence();
    }


    else if (str.substring(0, 7) == "ackflag") {
      //request for lop status
      if (lopAckFlag) {
        //check lop validty
        //SerialBT.println("LOP success" + String(LOP));
        SerialBT.println("success");
      }

      else {
        //SerialBT.println("LOP fail");
        SerialBT.println("fail");
      }
    }

    else if (str.substring(0, 6) == "getlop") {
      //asking data in the form of LOP=180
      SerialBT.println("LOP: " + String(LOP));
    }

    else if (str.substring(0, 7) == "percent") {
      //expecting data in the form of percent=0.6
      float x = str.substring(8).toFloat();

      //LOP percent validity should be done in the app
      //if (x >= 0.2 && x <= 0.8) {
      setThreshold(LOP, x);
      //}

      //else {
      //SerialBT.println("Invalid percent");
      //}
    }

    else if (str == "scan") {
      SerialBT.println("Enabling scanning...");

      if (!connected) {
        SerialBT.println("Scan started");
        doScan = true;
        BLEDevice::getScan()->start(5);
      }

      else {
        SerialBT.println("Device already connected.");
      }
    }

    else if (str == "reset") {
      SerialBT.println("Resetting device in");
      //      delay(500);
      //      SerialBT.println("3");
      //      delay(1000);
      //      SerialBT.println("2");
      //      delay(1000);
      //      SerialBT.println("1");
      //      delay(1000);
      esp_restart();
    }

    else if (str == "on") {
      Serial.println("#W23,1");
    }

    else if (str == "off") {
      Serial.println("#W23,1000");
    }

    Serial.println("");
    str = "";
  }

  //delay(2000); // Delay a ms between loops.
} // End of loop

//Pump should be turned off before bluetooth device is connected
void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("Client Connected: PUMP enabled");
    Serial.println("#W0,1\n"); //Enables the pump
  }

  if (event == ESP_SPP_CLOSE_EVT ) {
    Serial.println("Client disconnected: PUMP disabled");
    Serial.println("#W0,0\n"); //Disables the pump
  }
}

//Update pressure reading
void setPres(int x) {
  currentPressure = x;
  Serial.println("#W23," + String(currentPressure) + "\n");
  SerialBT.println("Pressure set to: " + String(x));
}

//Set the LOP upon valid input
void setLOP(int x) {
  if (x >= 120 && x <= 200) {
    LOP = x;
    SerialBT.println("LOP set to: " + String(LOP));
  }

  else {
    SerialBT.println("Invalid LOP");
  }
}

//Set the lower and upper threshold pressures if valid
//percentages are given against the LOP pressure
void setThreshold(unsigned int targetPressure, float offsetPercent) {
  int lowThreshold = (targetPressure * offsetPercent) - 5;
  int uppThreshold = (targetPressure * offsetPercent) + 2;
  Serial.println("#W19," + String(lowThreshold) + "\n");
  Serial.println("#W20," + String(uppThreshold) + "\n");
  //Serial.println("#W22,200\n"); //Turn the pump low when the upper threshold is reached
  SerialBT.println("Threshold set to range: " + String(lowThreshold) + " " + String(uppThreshold));
}

void updatePres() {
  while (SerialBT.available()) {
    delay(1);
    char c = SerialBT.read();
    str += c;
  }

  if (str.length() > 0) {

    str.trim();
    str.toLowerCase();
    //Serial.println(str);

    if (str.substring(0, 3) == "set") {
      //expecting pressure data in the form of set=90
      int x = str.substring(4).toInt();
      setPres(x);
    }
  }
  str = "";
}

int startTime = 0;
int timeElapsed = 0;
String errorMessage = "";

void resetTime() {
  startTime = millis() / 1000;
  timeElapsed = 0;
}

int plethHolder[36] = {0};
int holderPosition = 0;

boolean lopCheckAt(int pressure) {

  Serial.println("Entry to: " + String(pressure));
  //inflate to given pressure******************************
  setThreshold(pressure, 1);
  resetTime();
  while (currentPressure < pressure) {
    if (timeElapsed >= 20) {
      errorMessage = "Inflation timeout!";
      //Serial.println(errorMessage);
      return false;
    }
    updatePres();
    timeElapsed = millis() / 1000 - startTime;
  }
  resetTime();
  //wait for 10 secs while observing pleth data
  //delay(5000);
  while (timeElapsed < 10) {
    plethHolder[holderPosition] = plethAvg;
    //Serial.println("Inside " + String (pressure) + ": " + String(plethHolder[holderPosition]));
    holderPosition++;
    timeElapsed = millis() / 1000 - startTime;
    if (!connected) {
      errorMessage = "Disconnected!";
      //Serial.println(errorMessage);
      return false;
    }

    int tempTime = millis();
    int tempTimeElapsed = 0;

    while (tempTimeElapsed < 2000) {
      updatePres();
      delay(10);
      tempTimeElapsed = millis() - tempTime;
    }
  }

  //do something with the plethHolder
  //  for (int i = 0; i <= holderPosition; i++) {
  //    Serial.print(plethHolder[i]);
  //    Serial.print(" ");
  //  }
  //  Serial.println();
  Serial.println("Exit from: " + String(pressure));
  return true;
}

void lopSequence() {

  SerialBT.println("Starting LOP sequence...");
  Serial.println("#W22,200\n"); //Turn the pump low to 200 when the upper threshold is reached
  holderPosition = 0;
  if (lopCheckAt(60) && !lopAckFlag) {
    //normal readings, take mean
    int sum = 0;
    float normalAvg = 0;
    for (int i = 0; i < 6; i++) {
      sum = sum + plethHolder[i];
    }
    normalAvg = sum / 6;

    //*******************************
    if (lopCheckAt(90)) {
      //start point, observe changes
      sum = 0;
      float avg = 0;
      float tempAvg = 0;
      float delta = 0;
      boolean next = false;
      for (int i = 6; i < 12; i++) {
        sum = sum + plethHolder[i];
      }
      avg = sum / 6;
      delta = normalAvg - avg;
      Serial.println("90: " + String(delta));

      if (delta >= 5) {
        next = true;
        tempAvg = avg;
      }

      //*******************************
      if (!lopAckFlag && lopCheckAt(110)) {
        //check lop, if lop reached
        sum = 0;
        for (int i = 12; i < 18; i++) {
          sum = sum + plethHolder[i];
        }
        avg = sum / 6;
        delta = normalAvg - avg;
        Serial.println("110: " + String(delta));

        if (next) {
          if (delta > 15) {
            LOP = 100;
            lopAckFlag = true;
          }

          else {
            next = true;
            tempAvg = avg;
          }
        }

        else if (delta >= 5) {
          next = true;
          tempAvg = avg;
        }

        //*******************************
        if (!lopAckFlag && lopCheckAt(130)) {
          //check lop, if lop/post lop reached
          sum = 0;
          for (int i = 18; i < 24; i++) {
            sum = sum + plethHolder[i];
          }
          avg = sum / 6;
          delta = normalAvg - avg;
          Serial.println("130: " + String(delta));

          if (next) {
            if (delta > 15) {
              LOP = 120;
              lopAckFlag = true;
            }

            else {
              next = true;
              tempAvg = avg;
            }
          }

          else if (delta >= 5) {
            next = true;
            tempAvg = avg;
          }

          if (!lopAckFlag && lopCheckAt(140)) {
            //record lop, if lop/post lop reached
            sum = 0;
            for (int i = 24; i < 30; i++) {
              sum = sum + plethHolder[i];
            }
            avg = sum / 6;
            delta = normalAvg - avg;

            Serial.println("140: " + String(delta));

            if (next) {
              if (delta > 15) {
                LOP = 130;
                lopAckFlag = true;
              }
            }

            else {
              LOP = 140;
              lopAckFlag = true;
            }
          }
        }
      }
    }
  }

  for (int i = 0; i <= holderPosition; i++) {
    Serial.print(plethHolder[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println(lopAckFlag);
  if (lopAckFlag) {
    Serial.println("Calibration success!");
    Serial.println("Calibrated LOP: " + String(LOP));
  }
  else {
    Serial.println("Calibration error:" + errorMessage);
    Serial.println("Default LOP: " + String(LOP));
  }
  Serial.println("Exit calibrate");
}
