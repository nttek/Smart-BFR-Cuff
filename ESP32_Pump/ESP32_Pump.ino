//Program to control disk pump with LED (ON/OFF) from ESP32 using Serial Bluetooth

#include "BluetoothSerial.h" //Header File for Serial Bluetooth, will be added by default into Arduino

BluetoothSerial SerialBT; //Object for Bluetooth

const int presPin = 34;

int incoming;
int presValue = 0;
int LED_BUILTIN = 2;

void setup() {
  Serial.begin(115200); //Start Serial monitor in 9600
  SerialBT.begin("ESP32_PUMP_Control"); //Name of your Bluetooth Signal
  Serial.println("Bluetooth Device is Ready to Pair");

  pinMode (LED_BUILTIN, OUTPUT);//Specify that LED pin is output
}

void loop() {
  
  if (SerialBT.available()) //Check if we receive anything from Bluetooth
  {
    incoming = SerialBT.read(); //Read what we recevive 
    //Serial.print("Received:"); Serial.println(incoming);

    if (incoming == 49)
        {
        digitalWrite(LED_BUILTIN, HIGH);
        SerialBT.println("PUMP turned ON");
        Serial.println("#W0,1\n");
        }
        
    if (incoming == 48)
        {
        digitalWrite(LED_BUILTIN, LOW);
        SerialBT.println("PUMP turned OFF");        
        Serial.println("#W0,0\n");
        }     
  }

  //Reading pressure sensor value
  presValue = analogRead(presPin);
  Serial.println(presValue);
  delay(200);
}
