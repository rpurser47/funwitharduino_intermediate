// Fun with Arduino by Rob Purser is licensed under a
// Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// http://creativecommons.org/licenses/by-nc-sa/3.0/deed.en_US
// Based on a work at http://www.funwitharduino.net
// Copyright Rob Purser, 2013-2014

#include <Wire.h>

// Tmp102 temperature sensor globals
int tmp102Address = 0x48;
int minTempObserved = 100;
int maxTempObserved = 0;

// HH10D humidity sensor globals
int hh10dAddress = 81;
int hh10dInterrupt = 1; // On UNO, interrupt 0 is pin D2, interrupt 1 is pin D3
float hh10dOffset;
float hh10dSensitivity;
int minRHObserved = 100;
int maxRHObserved = 0;

// MPL3115 barometric pressure sensor globals
int MPL3115Address = 0x60;  
int minPressureObserved = 200000;
int maxPressureObserved = 0;

// RGB LED pins
int pinRed = 9;
int pinGreen = 6;
int pinBlue = 5;

// Mode button
int pinModeButton = 8;
int currentMode = 0; // 0 is TMP102, 1 is HH10D, 2 is MPL3115

void setup(){
  Serial.begin(9600);
  Wire.begin();

  pinMode(pinRed,OUTPUT);
  pinMode(pinGreen,OUTPUT);
  pinMode(pinBlue,OUTPUT);
  
  pinMode(pinModeButton,INPUT_PULLUP);

  configHh10d();

  // Calibrated for Natick, MA from Weather Underground
  if(!configMPL3115A2(493))
  {
    Serial.println("Cannot read MPL3115A2");
  }
}


void loop()
{
  float tempInC = readTmp102();
  
  if(tempInC < minTempObserved)
  {
    minTempObserved = tempInC;
  }
  if(tempInC > maxTempObserved)
  {
    maxTempObserved = tempInC;
  }
  
  Serial.print("Temperature: ");
  Serial.print(tempInC);
  Serial.print("C (");
  Serial.print((1.8 * tempInC) + 32);
  Serial.println("F)");
  
  float RH;
  if(readHh10dRH(&RH))
  {
    Serial.print("Relative Humidity: ");Serial.print(RH); Serial.println("%");
  }
  if(RH < minRHObserved)
  {
    minRHObserved = RH;
  }
  if(RH > maxRHObserved)
  {
    maxRHObserved = RH;
  }

  float pressureInPa, temperatureInC;
  if(readMPL3115A2(&pressureInPa, &temperatureInC))
  {
    Serial.print("Pressure: ");
    Serial.print(pressureInPa / 1000.0);  // Display in kPa
    Serial.print("kPa (");
    Serial.print(pressureInPa / 3386);  // Display in inches of mercury
    Serial.println("inHg)");
    Serial.print("Temperature: ");
    Serial.print(temperatureInC);
    Serial.println("C");
  }
  else
  {
    Serial.println("Cannot read MPL3115A2");
  }
  if(pressureInPa < minPressureObserved)
  {
    minPressureObserved = pressureInPa;
  }
  if(pressureInPa > maxPressureObserved)
  {
    maxPressureObserved = pressureInPa;
  }
  
  Serial.println("-----------------------------------");
  Serial.println();

  analogWrite(pinGreen,0);
  switch(currentMode)
  {
    case 0: // Tmp102
      analogWrite(pinRed, map(tempInC,minTempObserved,maxTempObserved,0,255));
      analogWrite(pinBlue, map(tempInC,minTempObserved,maxTempObserved,255,0));
      break;
    case 1: // HH10D
      analogWrite(pinRed, map(RH,minRHObserved,maxRHObserved,0,255));
      analogWrite(pinBlue, map(RH,minRHObserved,maxRHObserved,255,0));
      break;
    case 2: // MPL3115A2
      analogWrite(pinRed, map(pressureInPa,minPressureObserved,maxPressureObserved,0,255));
      analogWrite(pinBlue, map(pressureInPa,minPressureObserved,maxPressureObserved,255,0));
      break;
  }

  // This code provides a mechanism to change between the three sensors
  long nextReadingAt = millis() + 1000;
  bool lastSwitchReading = HIGH;
  while (millis() < nextReadingAt)
  {
    bool readSwitch = digitalRead(pinModeButton);
    if (lastSwitchReading != readSwitch)
    {
      if(readSwitch == LOW)
      {
        // button was pressed -- increment the mode
        currentMode = (currentMode + 1) % 3;
        Serial.print("Button pressed.  Mode now is ");Serial.println(currentMode);
        // Acknowledge with a green light for 250ms
        analogWrite(pinRed, 0);
        analogWrite(pinGreen, 255);
        analogWrite(pinBlue, 0);
        delay(250);
        // break out of the loop
        break; 
      }
      lastSwitchReading = readSwitch;
    }
    delay(10);
  }
}

float readTmp102()
{
  // Sensor datasheet: http://www.sparkfun.com/datasheets/Sensors/Temperature/tmp102.pdf
  Wire.requestFrom(tmp102Address,2); 

  byte MSB = Wire.read();
  byte LSB = Wire.read();

  //Serial.print("Tmp102 Data MSB: ");Serial.print(MSB);Serial.print(" LSB: ");Serial.println(LSB);
  //it's a 12bit int, using two's compliment for negative
  int TemperatureSum = ((MSB << 8) | LSB) >> 4; 

  return TemperatureSum*0.0625;
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
    hh10dFirstInterruptAt = micros();
  }
  hh10dLatestInterruptAt = micros();
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

  float periodInUs;
 
  if (finish > start)
  {
    periodInUs = float(finish - start);
  }
  else
  {
    // See http://arduino.cc/en/Reference/Micros 
    // micros() will roll over every 70 minutes or so
    periodInUs = float(4294967295L - finish + start);
  }
  
  if(count < 1000 || periodInUs < 500000)
  {
    return false;
  }
  
  // Serial.print("hh10dFirstInterruptAt: ");Serial.println(start);
  // Serial.print("hh10dLatestInterruptAt: ");Serial.println(finish);
  // Serial.print("hh10dInterruptCount: ");Serial.println(count);
  // Serial.print("time period: ");Serial.println(periodInUs);
  
  // Calculate frequency
  float frequency = float(count) * 1000000.0 / periodInUs;
//  Serial.print("HH10D Frequency: ");Serial.println(frequency);
  *RH = (hh10dOffset-frequency)*hh10dSensitivity/4096.0;
//  Serial.print("HH10D RH: ");Serial.println(*RH);
  return true;
}

// Offset to use when returning barometric pressure readings (calibrated to sea level)
float localMeanBasePressureOffsetInPa;

bool configMPL3115A2(unsigned int localSeaLevelOffsetInPa)
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

bool readMPL3115A2(float * pressure, float * temperature)
{
  // If INT_SOURCE (0x12) register's DRDY flag is enabled, return
  byte readyByte = i2cReadByteRegister(0x12);
  if(readyByte != 0x80) // check INT_SOURCE register on new data ready (SRC_DRDY)
  {
    Serial.print("MPL3115A2 INT_SOURCE byte was: ");Serial.println(readyByte);
    return false;
  }
  
//  Serial.println("New data at MPL3115A2");

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
