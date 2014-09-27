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
  }

  // This code provides a mechanism to change between the two sensors
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
        currentMode = (currentMode + 1) % 2;
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
    hh10dFirstInterruptAt = millis();
  }
  hh10dLatestInterruptAt = millis();
  hh10dInterruptCount++;
}

bool readHh10dRH(float * RH)
{
  unsigned int count;
  unsigned int start;
  unsigned int finish;
  
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
//  Serial.print("HH10D RH: ");Serial.println(RH);
  return true;
}


