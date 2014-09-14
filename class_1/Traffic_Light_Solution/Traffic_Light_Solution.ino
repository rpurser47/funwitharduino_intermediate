// Fun with Arduino by Rob Purser is licensed under a
// Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// http://creativecommons.org/licenses/by-nc-sa/3.0/deed.en_US
// Based on a work at http://www.funwitharduino.net
// Copyright Rob Purser, 2013-2014

int signal1RedPin = 13;
int signal1YellowPin = 12;
int signal1GreenPin = 11;
int signal2RedPin = 10;
int signal2YellowPin = 9;
int signal2GreenPin = 8;
int arrivalSensorPin = 7;
int delayTimingPin = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(signal1RedPin,OUTPUT);
  pinMode(signal1YellowPin,OUTPUT);
  pinMode(signal1GreenPin,OUTPUT);
  pinMode(signal2RedPin,OUTPUT);
  pinMode(signal2YellowPin,OUTPUT);
  pinMode(signal2GreenPin,OUTPUT);
  digitalWrite(signal2RedPin,HIGH);
  
  //TO DO:  Insert the code to configure the digital input connected to the button
  pinMode(arrivalSensorPin,INPUT_PULLUP);
}

void loop()
{
  Serial.println("Green  Red -- waiting on arrival");
  digitalWrite(signal1RedPin,LOW);
  digitalWrite(signal1GreenPin,HIGH);
  // TO DO: Replace this delay with code to loop until the button is pressed.  Don't forget that the button will read LOW when pressed
  //delay(5000);
  while(digitalRead(arrivalSensorPin) == HIGH)
  {
    delay(100);
  }
  
  Serial.println("Yellow Red");
  digitalWrite(signal1GreenPin,LOW);
  digitalWrite(signal1YellowPin,HIGH);
  delay(1000);
  
  Serial.println("Red    Red");
  digitalWrite(signal1YellowPin,LOW);
  digitalWrite(signal1RedPin,HIGH);
  delay(500);
  
  //Serial.println("Red    Green");
  digitalWrite(signal2RedPin,LOW);
  digitalWrite(signal2GreenPin,HIGH);
  
  // TO DO:   Replace this delay with code to read the knob and map it to a delay.
  int delayTimingReading = analogRead(delayTimingPin);
  int delayInMs = map(delayTimingReading,0,1023,1000,15000);
  Serial.print("Red    Green - a reading of ");
  Serial.print(delayTimingReading);
  Serial.print(" creates a delay of ");
  Serial.print(delayInMs);
  Serial.println("ms.");
  delay(delayInMs);
  
  Serial.println("Red    Yellow");
  digitalWrite(signal2GreenPin,LOW);
  digitalWrite(signal2YellowPin,HIGH);
  delay(1000);
  
  Serial.println("Red    Red");
  digitalWrite(signal2YellowPin,LOW);
  digitalWrite(signal2RedPin,HIGH);
  delay(500);
  
}


