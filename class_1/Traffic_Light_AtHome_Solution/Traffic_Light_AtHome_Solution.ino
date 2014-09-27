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
  crossFadeTwoTrafficLights(signal1RedPin,signal1GreenPin);
  // TO DO: Replace this delay with code to loop until the button is pressed.  Don't forget that the button will read LOW when pressed
  //delay(5000);
  while(digitalRead(arrivalSensorPin) == HIGH)
  {
    delay(100);
  }
  
  Serial.println("Yellow Red");
  crossFadeTwoTrafficLights(signal1GreenPin,signal1YellowPin);
  delay(1000);
  
  Serial.println("Red    Red");
  crossFadeTwoTrafficLights(signal1YellowPin,signal1RedPin);
  delay(500);
  
  Serial.println("Red    Green");
  crossFadeTwoTrafficLights(signal2RedPin,signal2GreenPin);
  
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
  crossFadeTwoTrafficLights(signal2GreenPin,signal2YellowPin);
  delay(1000);
  
  Serial.println("Red    Red");
  crossFadeTwoTrafficLights(signal2YellowPin,signal2RedPin);
  delay(500);
  
}

void crossFadeTwoTrafficLights(int fromPin, int toPin)
{
  // To cross fade, we need to write progressively increasing values to the "to" light, and
  // progressively decreasing values to the "from" light
  for(int i = 0; i <=255; i++)
  {
    analogWrite(toPin, i);          // Increases from 0 to 255
    analogWrite(fromPin, 255 - i);  // Decreases from 255 to 0
    delay(4);  // Fade will take place over 256 * 4 -- 1024 millisecond, or about second.
  }
}


