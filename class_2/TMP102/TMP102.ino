// Fun with Arduino by Rob Purser is licensed under a
// Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// http://creativecommons.org/licenses/by-nc-sa/3.0/deed.en_US
// Based on a work at http://www.funwitharduino.net
// Copyright Rob Purser, 2013-2014

#include <Wire.h>

// Tmp102 temperature sensor globals
int tmp102Address = 0x48;
int minCelsiusTempObserved = 100;
int maxCelsiusTempObserved = 0;

// RGB LED pins
int pinRed = 9;
int pinGreen = 6;
int pinBlue = 5;

void setup(){
  Serial.begin(9600);
  Wire.begin();

  pinMode(pinRed,OUTPUT);
  pinMode(pinGreen,OUTPUT);
  pinMode(pinBlue,OUTPUT);
}


void loop()
{
  float celsius = readTmp102();
  
  if(celsius < minCelsiusTempObserved)
  {
    minCelsiusTempObserved = celsius;
  }
  if(celsius > maxCelsiusTempObserved)
  {
    maxCelsiusTempObserved = celsius;
  }
  
  analogWrite(pinRed, map(celsius,minCelsiusTempObserved,maxCelsiusTempObserved,0,255));
  analogWrite(pinBlue, map(celsius,minCelsiusTempObserved,maxCelsiusTempObserved,255,0));

  Serial.print("Temperature: ");
  Serial.print(celsius);
  Serial.print("C (");
  Serial.print((1.8 * celsius) + 32);
  Serial.println("F)");
  
  Serial.println("-----------------------------------");
  Serial.println();
  delay(1000);
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

  float celsius = TemperatureSum*0.0625;
  return celsius;
}


