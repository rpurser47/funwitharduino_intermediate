// Fun with Arduino by Rob Purser is licensed under a
// Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// http://creativecommons.org/licenses/by-nc-sa/3.0/deed.en_US
// Based on a work at http://www.funwitharduino.net
// Copyright Rob Purser, 2013-2014

#include <SPI.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <Wire.h>

// Tmp102 temperature sensor globals
PROGMEM int tmp102Address = 0x48;

// HH10D humidity sensor globals
PROGMEM const int hh10dAddress = 81;
PROGMEM const int hh10dInterrupt = 1; // On UNO, interrupt 0 is pin D2, interrupt 1 is pin D3
float hh10dOffset;
float hh10dSensitivity;

// MPL3115 barometric pressure sensor globals
PROGMEM const int MPL3115Address = 0x60;  

// RGB LED pins
PROGMEM const unsigned int pinRed = 9;
PROGMEM const unsigned int pinGreen = 6;
PROGMEM const unsigned int pinBlue = 5;

// Ethernet card globals
PROGMEM const unsigned int sdCardSelectPin = 4;
// !!! You need to fill in the last byte here
PROGMEM byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, ?? };
EthernetClient client;

// !!! You need to fill in the Write API Key for your ThingSpeak Channel
String writeAPIKey = ??;    

void setup()
{
  Serial.begin(9600);

  pinMode(pinRed,OUTPUT);
  pinMode(pinGreen,OUTPUT);
  pinMode(pinBlue,OUTPUT);

  setRgbLed(0,0,20);

  Serial.println("Running");
  int success = false;
  while(!success)
  {
    // Set up the sensors
    Wire.begin();
    configHh10d();
  
    // Calibrated for Natick, MA from Weather Underground
    if(!configMPL3115A1(493))
    {
      Serial.println("Cannot read MPL3115A1");
      setRgbLed(20,0,0);
      delay(60000);
      continue;
    }
  
    // Set up the Ethernet card
    // Explicitly deselect the SD card on SPI bus.  This is done by setting pin 4 high.
    // The ethernet interface is unreliable unless this is set high.
    pinMode(sdCardSelectPin, OUTPUT);
    digitalWrite(sdCardSelectPin, HIGH);
    
    // Connect the ethernet, contact DHCP server, and get IP address
    // !!! Replace the false in the next line with the code to initialize the ethernet system in DHCP mode
    if(false)
    {
      Serial.println("Ethernet initialization failed.");
      setRgbLed(20,0,0);
      delay(60000);
      continue;
    }
  
    success = true;
    Serial.print("Ethernet initialized and assigned address ");Serial.println(Ethernet.localIP());
    setRgbLed(0,20,0);
  }
}

void loop()
{
  setRgbLed(0,0,20);

  double tempInC;

  if(!readTmp102(&tempInC))
  {
    Serial.println("Cannot read Tmp102");
    setRgbLed(20,0,0);
    delay(5000);
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(tempInC);
  Serial.print("C (");
  Serial.print((1.8 * tempInC) + 32);
  Serial.println("F)");
    
  float RH;
  if(!readHh10dRH(&RH))
  {
    Serial.println("Cannot read HH10D");
    setRgbLed(20,0,0);
    delay(5000);
    return;
  }
  Serial.print("Relative Humidity: ");Serial.print(RH); Serial.println("%");

  float pressureInPa, altTempInC;
  if(!readMPL3115A1(&pressureInPa, &altTempInC))
  {
    Serial.println("Cannot read MPL3115A1");
    setRgbLed(20,0,0);
    delay(5000);
    return;
  }
  Serial.print("Pressure: ");
  Serial.print(pressureInPa / 1000.0);  // Display in kPa
  Serial.print("kPa (");
  Serial.print(pressureInPa / 3386);  // Display in inches of mercury
  Serial.println("inHg)");
  Serial.print("Temperature: ");
  Serial.print(altTempInC);
  Serial.println("C");
  Serial.println();
  
  // Connect to the server on port 80 (HTTP)
  Serial.println("Connecting to ThingSpeak...");
  // !!! Replace the false in the next line with the code to connect to api.thingspeak.com on port 80
  if (false)
  { 
    Serial.println("Failed to connect to ThingSpeak...");
    setRgbLed(20,0,0);
    delay(5000);
    return;
  }
//  Serial.println("Connected to ThingSpeak...");

  // Construct the ThingSpeak POST content
  char celsiusStr[20];
  // dtostrf is an undocumented funciton in Arduino that converts a double to a string
  // dtostrf(floatVar, minStringWidthIncDecimalPoint, numVarsAfterDecimal, charBuf);
  dtostrf(tempInC, 4, 2, celsiusStr);
  char rhStr[20];
  dtostrf(RH, 4, 2, rhStr);
  char pressureStr[20];
  dtostrf(pressureInPa, 4, 2, pressureStr);
  char altTempStr[20];
  dtostrf(altTempInC, 4, 2, altTempStr);
  
  String postMessage;
  postMessage = String("field1=") + String(celsiusStr) +
                String("&field2=") + String(rhStr) +
                String("&field3=") + String(pressureStr) +
                String("&field4=") + String(altTempStr) +
                String("\n");
  
  // Post data to thingspeak
  client.print("POST /update HTTP/1.1\n");
  client.print("Host: api.thingspeak.com\n");
  client.print("Connection: close\n");
  client.print("X-THINGSPEAKAPIKEY: "+writeAPIKey+"\n");
  client.print("Content-Type: application/x-www-form-urlencoded\n");
  client.print("Content-Length: ");
  client.print(postMessage.length());
  client.print("\n\n");

  Serial.print("POST to ThingSpeak: ");Serial.println(postMessage);
  client.print(postMessage);client.print("\n");
 
  // As long as there's data from the server, keep reading it, and mirroring to serial port.
  while (client.connected())
  {
    // !!! Replace the false in the next line with the code to check to see if there are bytes avaliable
    while (false) 
    {
      // !!! Replace the 'X' in the next line with the code to read a character from the ethernet interface
      char c = 'X';
      Serial.print(c);
    }
  }

  // Server has disconnected
  Serial.println();
  Serial.println("disconnected from ThingSpeak.");
  // !!! Insert a command to stop the ethernet connection

  setRgbLed(0,20,0);

  Serial.println();
  Serial.println();

  // !!! You may want to shorten this until you get it working.
  delay(60000);
}

bool readTmp102(double * temperature)
{
  // Sensor datasheet: http://www.sparkfun.com/datasheets/Sensors/Temperature/tmp102.pdf
  Wire.requestFrom(tmp102Address,2); 

  byte MSB = Wire.read();
  byte LSB = Wire.read();

  //Serial.print("Tmp102 Data MSB: ");Serial.print(MSB);Serial.print(" LSB: ");Serial.println(LSB);
  //it's a 12bit int, using two's compliment for negative
  *temperature = double(((MSB << 8) | LSB) >> 4) * 0.0625; 

  if (*temperature < -20 || *temperature > 50)
  {
    Serial.print("Tmp102 error: "); Serial.print(*temperature); Serial.print(" ( "); Serial.print(MSB); Serial.print(", "); Serial.print(LSB); Serial.println(")");
    return false;
  }

  return true;
}

void configHh10d()
{
  // Sensor Datasheet: http://www.sparkfun.com/datasheets/Sensors/Temperature/HH10D.pdf
   hh10dSensitivity   =  float(i2cReadIntRegister(hh10dAddress, 10)); //Read sensitivity from HH10D EEPROM
   hh10dOffset =  float(i2cReadIntRegister(hh10dAddress, 12)); //Same for offset

//   Serial.print("HH10D sensitivity: "); Serial.println(hh10dSensitivity); 
//   Serial.print("HH10D offset: "); Serial.println(hh10dOffset); 

  // http://arduino.cc/en/Reference/AttachInterrupt
  attachInterrupt(hh10dInterrupt,hh10dInterruptHandler,RISING);
}

int i2cReadIntRegister(int deviceaddress, byte address)  
{
   // SET ADDRESS
   Wire.beginTransmission(deviceaddress);
   Wire.write(address); // address for sensitivity
   Wire.endTransmission();
   
   // REQUEST RETURN VALUE
   Wire.requestFrom(deviceaddress, 2);
   return Wire.read() << 8 | Wire.read();
}

// globals set by the hh10d interrupt handler
unsigned long hh10dInterruptCount = 0;
unsigned long hh10dFirstInterruptAt;
unsigned long hh10dLatestInterruptAt;

void hh10dInterruptHandler()
{
  if (hh10dInterruptCount == 0)
  {
    hh10dFirstInterruptAt = millis();
  }
  hh10dLatestInterruptAt = millis();
  hh10dInterruptCount++;
}

bool readHh10dRH(float * RH)
{
  unsigned long count;
  unsigned long start;
  unsigned long finish;
  
  // Capture the data from the ISR
  noInterrupts(); // Turn off interrupts momentarily to transfer data from ISR http://arduino.cc/en/Reference/noInterrupts
                  // Remember that these values change several thousand times per second!
  count = hh10dInterruptCount;
  start = hh10dFirstInterruptAt;
  finish = hh10dLatestInterruptAt;
  
  // Reset the ISR
  hh10dInterruptCount = 0;

  // Turn the interrupts back on 
  interrupts();

  float periodInMs = float(finish - start);
  if(count < 1000 || periodInMs < 500)
  {
    return false;
  }
  
//  Serial.print("hh10dFirstInterruptAt: ");Serial.println(start);
//  Serial.print("hh10dLatestInterruptAt: ");Serial.println(finish);
//  Serial.print("hh10dInterruptCount: ");Serial.println(count);
//  Serial.print("time period: ");Serial.println(periodInMs);
  
  // Calculate frequency
  float frequency = float(count) * 1000.0 / periodInMs;
//  Serial.print("HH10D Frequency: ");Serial.println(frequency);
  *RH = (hh10dOffset-frequency)*hh10dSensitivity/4096.0;
//  Serial.print("HH10D RH: ");Serial.println(*RH);

  if (*RH < 0 || *RH > 100)
  {
    Serial.print("HH10D error: "); Serial.print(*RH); Serial.print(" ( "); Serial.print(count); Serial.print(", "); Serial.print(start); Serial.print(", "); Serial.print(finish); Serial.println(")");
    return false;
  }

  return true;
}

// Offset to use when returning barometric pressure readings (calibrated to sea level)
float localMeanBasePressureOffsetInPa;

bool configMPL3115A1(unsigned int localSeaLevelOffsetInPa)
{
  // Sensor datasheet: http://dlnmh9ip6v2uc.cloudfront.net/datasheets/Sensors/Pressure/MPL3115A2.pdf
  // To configure the sensor, find the register hex value and 
  // enter it into the first field of i2cWriteByteRegister (see below). Then
  // figure out the hex value of the data you want to send.
  // 
  // For example:
  // For CTRL_REG1, the address is 0x26 and the data is 0xB9
  // 0xB9 corresponds to binary 1011 1001. Each place holder 
   // represents a data field in CTRL_REG1. 
 
   // This is a basic II2 communication check. If the sensor doesn't
  // return decicmal number 196 (see 0x0C register in datasheet), 
  // error is printed. 
  if(i2cReadByteRegister(0x0C) != 196)
  {
    return false;
  }
 
  // CTRL_REG1 (0x26): put sensor in standby for programming, max oversampling, pressure mode
  // ALT:0, RAW:0, OST0-2:111 - 2^7, RST:0, OST:0, SYSB:0
  i2cWriteByteRegister(0x26, 0b00111000);
  
  // CTRL_REG4 (0x29): Data ready interrupt enabled
  // INT_EN_DRDY:1, INT_EN_FIFO:0, INT_EN_PW:0, INT_EN_TW:0,
  // INT_EN_PTH:0, INT_EN_TTH:0, INT_EN_PCHG:0, INT_EN_TCHG:0
  i2cWriteByteRegister(0x29, 0x80); 
  
  // PT_DATA_CFG (0x13): enable both pressure and temp event flags 
  // DREM:1, PDEFE:1, TDEFE:1
  i2cWriteByteRegister(0x13, 0x07);
  
  // This coniguration option calibrates the sensor according to 
  // the sea level pressure for the measurement location
  // 101326Pa -- standard mean sea level pressure
  localMeanBasePressureOffsetInPa = localSeaLevelOffsetInPa;
  // Note that you have to enter a value that is ONE HALF of your target value.
  unsigned int BAR_IN = (101326 + localSeaLevelOffsetInPa) / 2;
  
  // BAR_IN_MSB (0x14):
  byte BAR_IN_MSB = (BAR_IN & 0xFF00) >> 8;
  i2cWriteByteRegister(0x14, BAR_IN_MSB);
//  Serial.print("BAR_IN_MSB: "); Serial.print(BAR_IN_MSB, HEX);
  
  // BAR_IN_LSB (0x15):
  byte BAR_IN_LSB = BAR_IN & 0x00FF;
  i2cWriteByteRegister(0x15, BAR_IN_LSB);
//  Serial.print(" LSB: "); Serial.println(BAR_IN_LSB, HEX);

  // CTRL_REG1 (0x26): put sensor in run mode, max oversampling, pressure mode
  // ALT:0, RAW:0, OST0-2:111 - 2^7, RST:0, OST:0, SYSB:1
  i2cWriteByteRegister(0x26, 0b00111001);

  return true;  
}

bool readMPL3115A1(float * pressure, float * temperature)
{
  // If INT_SOURCE (0x12) register's DRDY flag is enabled, return
  byte readyByte = i2cReadByteRegister(0x12);
  if(readyByte != 0x80) // check INT_SOURCE register on new data ready (SRC_DRDY)
  {
    Serial.print("MPL3115A1 INT_SOURCE byte was: ");Serial.println(readyByte);
    return false;
  }
  
//  Serial.println("New data at MPL3115A1");

  // Information on data format from data sheet, section 7.1.3:
  // http://dlnmh9ip6v2uc.cloudfront.net/datasheets/Sensors/Pressure/MPL3115A2.pdf

  // variables for the calculations
  unsigned int OUT_P_MSB, OUT_P_CSB, OUT_P_LSB; 
  unsigned int OUT_T_MSB, OUT_T_LSB; 
  
  OUT_P_MSB = i2cReadByteRegister(0x01);
  OUT_P_CSB = i2cReadByteRegister(0x02);
  OUT_P_LSB = i2cReadByteRegister(0x03);

  //The Pressure data is arranged as 20-bit unsigned data in Pascals. The first 18 bits are located in OUT_P_MSB, OUT_P_CSB 
  //and bits 7-6 of OUT_P_LSB. The 2 bits in position 5-4 of OUT_P_LSB represent the fractional component. 
  //When RAW bit is set (CTRL_REG1), then the RAW value is stored in all 24 bits of OUT_P_MSB, OUT_P_CSB and 
  //OUT_P_LSB. 
  //Shift MSB over by 10 bits (since it's an 18 bit value, the shift needs to be done in a long)
  *pressure = float(long(OUT_P_MSB)<<10);
  //Shift CSB over by 2 bits and add to the total
  *pressure = *pressure + (float)(OUT_P_MSB<<2);
  //Mask out bits 6&7 in CSB and shift over by 6 bits and add to the total
  *pressure = *pressure + (float)((OUT_P_LSB & 0b11000000)>>6);
  // The least significant bytes OUT_P_LSB are 2-bit,
  // fractional values in bits 4&5, so you must mask them out, cast the calulation in (float),
  // shift the value over 4 bits to the right and divide by 4 (since 
  // there are 4 values in 2-bits). 
  *pressure = *pressure + float((OUT_P_LSB & 0b00110000)>>4)/4.0;
  
  // Apply an offset to calibrate results to sea level (the way weather stations work)
  *pressure = *pressure + localMeanBasePressureOffsetInPa;

  //The Altitude data is arranged as 20-bit 2’s complement value in meters. The data is stored as meters with the 16 bits of 
  //OUT_P_MSB and OUT_P_CSB and with fractions of a meter stored in bits 7-4 of OUT_P_LSB. Be aware that the fractional bits 
  //are not signed, therefore, they are not represented in 2’s complement. 
//  *altitude = (float)((OUT_P_MSB << 8)|OUT_P_CSB);
  // For altutude, the least significant bytes OUT_P_LSB are 4-bit,
  // fractional values, so you must cast the calulation in (float),
  // shift the value over 4 bits to the right and divide by 16 (since 
  // there are 16 values in 4-bits). 
//  *altitude = *altitude + float(OUT_P_LSB>>4)/16.0;

  //The Temperature data is arranged as 12-bit 2’s complement value in degrees C. The 8 bits of OUT_T_MSB representing degrees and
  //with fractions of a degree are stored in 4 bits in position 7-4 of OUT_T_LSB. Be aware that the fractional bits are not 
  //signed, therefore, they are not represented in 2’s complement. When RAW is selected then the RAW value is stored in all 16 bits
  //of OUT_T_MSB and OUT_T_LSB.
  OUT_T_MSB = i2cReadByteRegister(0x04); 
  OUT_T_LSB = i2cReadByteRegister(0x05); 
  *temperature = float(OUT_T_MSB) + float(OUT_T_LSB>>4)/16.0; //temp, fraction of a degree

  if (*pressure < 50000 || *pressure > 150000 ||
      *temperature < -20 || *temperature > 50)
  {
    Serial.println("MPL3115A1 error: "); 
    Serial.print(*pressure); Serial.print(" ( "); Serial.print(OUT_P_MSB); Serial.print(", "); Serial.print(OUT_P_CSB); Serial.print(", "); Serial.print(OUT_P_LSB); Serial.println(")");
    Serial.print(*temperature); Serial.print(" ( "); Serial.print(OUT_T_MSB); Serial.print(", "); Serial.print(OUT_T_LSB); Serial.println(")");
    return false;
  }
  
  return true;
}

byte i2cReadByteRegister(byte regAddr)
{
  // This function reads one byte over IIC
  Wire.beginTransmission(MPL3115Address);
  Wire.write(regAddr);  // Address of CTRL_REG1
  Wire.endTransmission(false); // Send data to I2C dev with option
                               //  for a repeated start. THIS IS
                               //  NECESSARY and not supported before
                               //  Arduino V1.0.1!!!!!!!!!
  Wire.requestFrom(MPL3115Address, 1); // Request the data...
  return Wire.read();
}

void i2cWriteByteRegister(byte regAddr, byte value)
{
  // This function writes one byto over IIC  
  Wire.beginTransmission(MPL3115Address);
  Wire.write(regAddr);
  Wire.write(value);
  Wire.endTransmission(true);
}  

void setRgbLed(unsigned int red, unsigned int green, unsigned int blue)
{
  digitalWrite(pinRed, red);
  digitalWrite(pinGreen, green);
  digitalWrite(pinBlue, blue);
}
