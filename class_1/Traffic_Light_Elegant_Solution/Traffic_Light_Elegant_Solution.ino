// Fun with Arduino by Rob Purser is licensed under a
// Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// http://creativecommons.org/licenses/by-nc-sa/3.0/deed.en_US
// Based on a work at http://www.funwitharduino.net 
// Copyright Rob Purser, 2013-2014

// Indexes into the pin arrays
int RED = 0;
int YELLOW = 1;
int GREEN = 2;
char* colorName[] = {"   Red","Yellow"," Green"};

// Pins for the traffic lights
int signal1Pin[] = {13,12,11};
int signal2Pin[] = {10,9,8};

// Indexes into the oclumns of the sequence array
int SIGNAL1 = 0;
int SIGNAL2 = 1;
int DELAY = 2;

// Lighting sequence. 
int sequence[][3] = {
  {RED,    RED,     500},
  {GREEN,  RED,    5000},
  {YELLOW, RED,    1000},
  {RED,    RED,     500},
  {RED,    GREEN,  3000},
  {RED,    YELLOW, 1000}};

//Keep track of where we are in the sequence
int iSequence = 0;

void setup()
{
  Serial.begin(9600);
  for(int iPin = 0;iPin < 3; iPin++)
  {
    pinMode(signal1Pin[iPin],OUTPUT);
    pinMode(signal2Pin[iPin],OUTPUT);
  }
  
  //Power On Self Test
  Serial.println("Power on Self Test");
  for(int i = 0;i < 5;i++)
  {
    for(int iPin = 0;iPin < 3;iPin++)
    {
      digitalWrite(signal1Pin[iPin],HIGH);
      digitalWrite(signal2Pin[iPin],HIGH);
      delay(500);
      digitalWrite(signal1Pin[iPin],LOW);
      digitalWrite(signal2Pin[iPin],LOW);
    }
  }
}

void loop()
{
  digitalWrite(signal1Pin[sequence[iSequence][SIGNAL1]],LOW);
  digitalWrite(signal2Pin[sequence[iSequence][SIGNAL2]],LOW);
  
  iSequence = (iSequence + 1) % 6;
  
  Serial.print(colorName[sequence[iSequence][SIGNAL1]]);
  Serial.print(" ");
  Serial.print(colorName[sequence[iSequence][SIGNAL2]]);
   
  digitalWrite(signal1Pin[sequence[iSequence][SIGNAL1]],HIGH);
  digitalWrite(signal2Pin[sequence[iSequence][SIGNAL2]],HIGH);

  int delayInMs = sequence[iSequence][DELAY];

  Serial.print(". Waiting "); Serial.print(delayInMs); Serial.println("ms");
  delay(delayInMs);

}
