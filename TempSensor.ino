#include <OneWire.h>

// Read in temperature from DS1820 1 wire device
OneWire g_tempSensor(7);  // read in temp on pin 7, needs a 4.7K pull up

const float g_targetTemperature = 96.0f;
const byte g_magneticOutput = 4;

const byte s_addressLength = 8;
const byte s_crcIndex = 7;
const byte s_dataLength = 12;
const byte s_numDataBytes = 9;

const byte DS18S20 = 0x10;
const byte DS18B20 = 0x28;
const byte DS1822 = 0x22;

const byte s_startConversion = 0x44;
const byte s_readData = 0xBE;

void setup() {
  pinMode(g_magneticOutput, OUTPUT);

  // start with door locked...
  digitalWrite(g_magneticOutput, LOW);

  Serial.begin(9600);
}

void loop() {
  byte deviceAddress[s_addressLength];
  byte deviceData[s_dataLength];
  
  // put your main code here, to run repeatedly:
  if (g_tempSensor.search(deviceAddress) != false) {
    if (OneWire::crc8(deviceAddress, s_addressLength - 1) == deviceAddress[s_crcIndex]) {
      g_tempSensor.reset();
      g_tempSensor.select(deviceAddress);
      g_tempSensor.write(s_startConversion, 0); // assumes pull up and not using parasitic power

      delay(1000);     // need at least 750ms so add some slop  
      byte present = g_tempSensor.reset();
      g_tempSensor.select(deviceAddress);
      g_tempSensor.write(s_readData);         // Read Scratchpad

      for (int index = 0; index < s_numDataBytes; index++) { // we need 9 bytes
        deviceData[index] = g_tempSensor.read();
      }
    
      // Convert the data to actual temperature
      // because the result is a 16 bit signed integer, it should
      // be stored to an "int16_t" type, which is always 16 bits
      // even when compiled on a 32 bit processor.
      int16_t raw = (deviceData[1] << 8) | deviceData[0];
      if (deviceAddress[0] == DS18S20) {
        raw = raw << 3; // 9 bit resolution default
        if (deviceData[7] == 0x10) {
          // "count remain" gives full 12 bit resolution
          raw = (raw & 0xFFF0) + 12 - deviceData[6];
        }
      } else {
        byte cfg = (deviceData[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
        //// default is 12 bit resolution, 750 ms conversion time
      }
      float celsius = (float)raw / 16.0;
      float fahrenheit = celsius * 1.8 + 32.0;
      Serial.print("  Temperature = ");
      Serial.print(celsius);
      Serial.print(" Celsius, ");
      Serial.print(fahrenheit);
      Serial.println(" Fahrenheit");

      if (fahrenheit > g_targetTemperature) {
        digitalWrite(g_magneticOutput, HIGH);
      }
    } else {
      Serial.print("Failed CRC check on address: ");
      for (int index = 0; index < s_addressLength; index++) {
          Serial.print(deviceAddress[index], HEX);
      }
      Serial.println("");
    }
  } else {
    g_tempSensor.reset_search();
    delay(100);
  }
}
