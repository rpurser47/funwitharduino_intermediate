// Fun with Arduino by Rob Purser is licensed under a
// Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// http://creativecommons.org/licenses/by-nc-sa/3.0/deed.en_US
// Based on a work at http://www.funwitharduino.net
// Copyright Rob Purser, 2013-2014

#include <Wire.h>
#include "TimerOne.h"

// Tmp102 temperature sensor globals
int tmp102Address = 0x48;
int minCelsiusTempObserved = 100;
int maxCelsiusTempObserved = 0;

// LED pin
int pinGreen = 6;
int pinRed = 9;

void setup(){
  Serial.begin(9600);
  Wire.begin();
  
  // Set up a interrupt timer (100ms) to detect if the I2C call hangs because device is not powered.
  Timer1.initialize(100000); // microseconds
  Timer1.attachInterrupt(detectI2CTimeout);

  pinMode(pinGreen,OUTPUT);
  pinMode(pinRed,OUTPUT);
}

// The time we entered the I2C call
unsigned long enteredTest = 0; 

void loop()
{
  // Record the time we entered the test 
  enteredTest = micros();
  bool tmp102Working = testTmp102();
  // A value of 0 indicates that we're not in the test 
  enteredTest = 0;
 
  if(tmp102Working)
  {
    digitalWrite(pinGreen,HIGH);
    digitalWrite(pinRed,LOW);
  }
  else
  {
    digitalWrite(pinRed,HIGH);
    digitalWrite(pinGreen,LOW);
    Wire.begin();
  }
  
  delay(100);
}

// Called ever 100 ms as an interrupt
void detectI2CTimeout()
{
  // Check if we're in the test
  if(enteredTest != 0)
  {
    long elapsedTime = micros() - enteredTest;
    // If we've been in the function more than 1ms, then it's hung
    if(elapsedTime > 1000)
    {
      // Set the light red and reset the I2C
      digitalWrite(pinRed,HIGH);
      digitalWrite(pinGreen,LOW);
      Wire.begin();
    }
  }
}

bool testTmp102()
{
  // Sensor datasheet: http://www.sparkfun.com/datasheets/Sensors/Temperature/tmp102.pdf
  Wire.requestFrom(tmp102Address,2); 

  int bytesAvailable = Wire.available();
  if(bytesAvailable < 2)
  {
    Serial.print("Tmp102 Request returned: ");Serial.print(bytesAvailable);Serial.println(" bytes.");
    return false;
  }
  
  byte MSB = Wire.read();
  byte LSB = Wire.read();

  //it's a 12bit int, using two's compliment for negative
  int TemperatureSum = ((MSB << 8) | LSB) >> 4; 

  float celsius = TemperatureSum*0.0625;
  Serial.print("Tmp102 Data MSB: ");Serial.print(MSB);Serial.print(" LSB: ");Serial.print(LSB);Serial.print(" Celsius: ");Serial.println(celsius);
  
  if(celsius < 15 || celsius > 25)
    return false;
  else
    return true;
}


