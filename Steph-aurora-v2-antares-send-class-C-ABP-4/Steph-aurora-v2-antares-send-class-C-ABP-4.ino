/************************************************************************************* 
 * Gabungan dari LoRa Uplink dan ESP32 Touch
 *  Diambil dari https://github.com/cosmic-id/cosmic-lora-aurora/blob/main/examples/aurora-v2-antares-send-class-C-ABP/aurora-v2-antares-send-class-C-ABP.ino
 *  Example of ABP device
 * Status
 *  2032/03/25. Not Working beceause ABP Credentials.
 *************************************************************************************/
//LORA
#include <lorawan.h>

//Hardware LoRa
#define PIN_CS 15
#define PIN_RESET 0
#define PIN_DIO0 27
#define PIN_DIO1 2
#define PIN_EN 32

//Activation By Personalization (ABP) Credentials 
const char *devAddr = "24d8f5c9";
const char *nwkSKey = "f4ee5ff221f8357d0000000000000000";
const char *appSKey = "00000000000000002bae55eb22e09978";
//const char *devAddr = "ec5711f1"; 

//Send Message's variables
const unsigned long interval = 1200000;     // 1/3 jam interval to send message
unsigned long previousMillis = 0;           // will store last time message sent
unsigned int counter = 0;                   // message counter
char myStr[50];
//byte outStr[255];
byte recvStatus = 0;
int port, channel, freq;
bool newmessage = false;

//One time uplink after reset
bool initial = true;                        //first data sent.

//Hardware LoRa (cont.)
const sRFM_pins RFM_pins = {
  .CS   = PIN_CS,
  .RST  = PIN_RESET,
  .DIO0 = PIN_DIO0,
  .DIO1 = PIN_DIO1,
};

//ESP32 Touch Sensor
int  threshold = 40;
bool touchActive = false;
bool lastTouchActive = false;
bool testingLower = true;

//BME280
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C
unsigned long delayTime;
    
//Touch Event
void gotTouchEvent(){
  if (lastTouchActive != testingLower) {
    touchActive = !touchActive;
    testingLower = !testingLower;
    
    // Touch ISR will be inverted: Lower <--> Higher than the Threshold after ISR event is noticed
    touchInterruptSetThresholdDirection(testingLower);
  }
}

// Uplink. Send message.
void Uplink(){
  char temperature_string[10];
  float temperature = bme.readTemperature();
  dtostrf(temperature,5,2,temperature_string);   
  
  float pressure = bme.readPressure()/ 100.0F;
  char pressure_string[10];
  dtostrf(pressure,7,2,pressure_string);   

  float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  char altitude_string[10];
  dtostrf(altitude,5,2,altitude_string);

  float humidity = bme.readHumidity();
  char humidity_string[15];
  dtostrf(humidity,5,2,humidity_string);   

  //sprintf(myStr, "Lora 1 Counter-%d.", counter++);  
  sprintf(myStr, "%s", temperature_string );    
  //                       1         2        4         5
  //              1234567890123456789012345678901234567890
  //sprintf(myStr, "%d. T: %sC. P: %shPa", counter++, temperature_string, pressure_string ); //[Ok]
  //sprintf(myStr, "%d. T: %sC. P: %shPa. A: %sm. H: %s%%", counter++, temperature_string, pressure_string, altitude_string, humidity_string );
  //sprintf(myStr, "Count-%d. T: %s °C. P: %s hPa. A: %s m", counter++, temperature_string, pressure_string, altitude_string );
  //sprintf(myStr, "Lora 1 Counter-%d. T: %s °C. P: %s hPa. H: %s  ", counter++, temperature_string, pressure_string, humidity_string );
  
  Serial.print("Sending: "); 
  Serial.println(myStr);
    
  lora.sendUplink(myStr, strlen(myStr), 0);
  port = lora.getFramePortTx();
  channel = lora.getChannel();
  freq = lora.getChannelFreq(channel);
  
  Serial.print(F("fport: "));
  Serial.print(port);   
  Serial.print(" ");
  Serial.print(F("Ch: "));       
  Serial.print(channel);
  Serial.print(" ");
  Serial.print(F("Freq: "));     
  Serial.print(freq);   
  Serial.println(" ");  
}

//Temperature Sensor values
void printValues() {  
    Serial.print("Temperature = ");
    Serial.print(bme.readTemperature());
    Serial.println(" °C");

    Serial.print("Pressure = ");
    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");

    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");
    Serial.println();
}

void report(){
  char buffer[200];
  char temperature[10];
  float suhu = bme.readTemperature();
  dtostrf(suhu,7,2,temperature);
    /*
    float pressure = bme.readPressure()/ 100.0F;
    char pressure_string[10];
    dtostrf(pressure,7,2,pressure_string);   
    
    float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
    char altitude_string[10];
    dtostrf(altitude,7,2,altitude_string);   
    
    float humidity = bme.readHumidity();
    char humidity_string[10];
    dtostrf(humidity,7,2,humidity_string);   
    */
    //sprintf(buffer, "Temperature= %s °C, Pressure= %s hPa, Approx Altitude= %s m, Humidity= %s %.",suhu , pressure_string, altitude_string, humidity_string);  
    sprintf(buffer, "Temperature= %s °C", temperature);
    //Serial.println(buffer);
}

void setup() {
  // 1. Initiate the LoRa Enabled pin
  pinMode(PIN_EN, OUTPUT);
  // LoRa chip is Active High
  digitalWrite(PIN_EN, HIGH);
  
  // 2. Setup loraid access
  Serial.begin(115200);
  delay(2000);
  if (!lora.init()) {
    Serial.println("RFM95 not detected");
    delay(5000);
    return;
  }

  // 3. Set LoRaWAN Class change CLASS_A or CLASS_C
  lora.setDeviceClass(CLASS_C);

  // 4. Set Data Rate
  lora.setDataRate(SF12BW125); //GANTI

  // 5. Set FramePort Tx
  lora.setFramePortTx(5);

  // 6. Set Channel as random
  lora.setChannel(MULTI);

  // 7. Set Tx Power to 15 dBi (max)
  lora.setTxPower(15);

  // 8. Put ABP Key and DevAddress here
  lora.setNwkSKey(nwkSKey);
  lora.setAppSKey(appSKey);
  lora.setDevAddr(devAddr);

  // 1. ESP32 Touch Interrupt
  Serial.println("ESP32 Touch Interrupt Test");
  touchAttachInterrupt(T6, gotTouchEvent, threshold);

  // 2. Touch ISR will be activated when touchRead is lower than the Threshold
  touchInterruptSetThresholdDirection(testingLower);    

  unsigned status;
    
  // default settings
  status = bme.begin(0x76);  
  // You can also pass in a Wire library object like &Wire2
  // status = bme.begin(0x76, &Wire2)
  if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); 
        Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        while (1) 
          delay(10);
  }
  Serial.println("-- Default Test --");
}

void loop() {
  //1. One time sent
  if (initial){
    printValues();
    Uplink();
    initial= false;
  }
  
  //2. Check Interval Overflow to send a message again
  if (millis() - previousMillis > interval) {
    previousMillis = millis();
    Uplink();
  }

  if(lastTouchActive != touchActive){
    lastTouchActive = touchActive;
    if (touchActive) {
      Serial.println("  ---- Touch was Pressed");  
      printValues(); //original
      //report();      //using sprintf
      Uplink();            
    } else {
      Serial.println("  ---- Touch was Released");
    }
  }
  
  // Check Lora RX
  lora.update();

 // recvStatus = lora.readDataByte(outStr);
  /*
  if (recvStatus) {
    newmessage = true;
    int counter = 0;
    port = lora.getFramePortRx();
    channel = lora.getChannelRx();
    freq = lora.getChannelRxFreq(channel);

    for (int i = 0; i < recvStatus; i++)
    {
      if (((outStr[i] >= 32) && (outStr[i] <= 126)) || (outStr[i] == 10) || (outStr[i] == 13))
        counter++;
    }
    if (port != 0)
    {
      if (counter == recvStatus)
      {
        Serial.print(F("Received String : "));
        for (int i = 0; i < recvStatus; i++)
        {
          Serial.print(char(outStr[i]));
        }
      }
      else
      {
        Serial.print(F("Received Hex: "));
        for (int i = 0; i < recvStatus; i++)
        {
          Serial.print(outStr[i], HEX); Serial.print(" ");
        }
      }
      Serial.println();
      Serial.print(F("fport: "));    Serial.print(port);Serial.print(" ");
      Serial.print(F("Ch: "));    Serial.print(channel);Serial.print(" ");
      Serial.print(F("Freq: "));    Serial.println(freq);Serial.println(" ");
    }
    else
    {
      Serial.print(F("Received Mac Cmd : "));
      for (int i = 0; i < recvStatus; i++)
      {
        Serial.print(outStr[i], HEX); Serial.print(" ");
      }
      Serial.println();
      Serial.print(F("fport: "));    Serial.print(port);Serial.print(" ");
      Serial.print(F("Ch: "));    Serial.print(channel);Serial.print(" ");
      Serial.print(F("Freq: "));    Serial.println(freq);Serial.println(" ");
    }
  }
  */
}
