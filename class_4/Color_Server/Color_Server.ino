// Fun with Arduino by Rob Purser is licensed under a
// Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// http://creativecommons.org/licenses/by-nc-sa/3.0/deed.en_US
// Based on a work at http://www.funwitharduino.net 
// Copyright Rob Purser, 2013-2014

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetServer.h>
#include <Wire.h>

// Tmp102 temperature sensor globals
int tmp102Address = 0x48;

// RGB LED pins
unsigned int pinRed = 9;
unsigned int pinGreen = 6;
unsigned int pinBlue = 5;

// Ethernet card globals
unsigned int sdCardSelectPin = 4;
// !!! You need to fill in the last byte here
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, ?? };
byte ipAddress[] = { 192, 168, 1, 2 };
EthernetServer server = EthernetServer(80); //port 80

void setup()
{
  Serial.begin(9600);

  pinMode(pinRed,OUTPUT);
  pinMode(pinGreen,OUTPUT);
  pinMode(pinBlue,OUTPUT);

  // Power on self test
  setColor("red");
  delay(500);
  setColor("green");
  delay(500);
  setColor("blue");
  delay(500);
  setColor("none");
 
  // Set up the Ethernet card
  // Explicitly deselect the SD card on SPI bus.  This is done by setting pin 4 high.
  // The ethernet interface is unreliable unless this is set high.
  pinMode(sdCardSelectPin, OUTPUT);
  digitalWrite(sdCardSelectPin, HIGH);
 
  // Connect the ethernet, set static IP address using Ethernet.begin
  Ethernet.begin(mac,ipAddress);
  Serial.print("Ethernet initialized and assigned address ");Serial.println(Ethernet.localIP());
  
  server.begin();
}
void loop()
{
  // Check if there is a connection
  EthernetClient client = server.available();

  if (!client) 
  {
    // No connection -- check again in 100ms
    client.stop();
    delay(100);
    return;
  }
  
  // an http request ends with a blank line
  boolean currentLineIsBlank = true;
  boolean sentHeader = false;
  boolean reading = false;

  unsigned int iColor = 0;
  char colorCommand[20];
  long abortReadAt = millis() + 5000;

  while (client.connected()) 
  {
    if(millis() > abortReadAt)
    {
      // If reading for more than 5 seconds, give up and try again.
      Serial.println("Timeout reading -- abandon attempt");
      delay(1000);
      client.stop();
      return;
    }

    if (client.available()) 
    {
      if(!sentHeader)
      {
        // send a standard http response header
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println();
        sentHeader = true;
        Serial.println("Request received -- sent acknowledgement");
      }

      char c = client.read();

      if(reading && c == ' ')
      {
        // End of the message
        reading = false;
        colorCommand[iColor] = 0;
        break;
      }
      
      if(c == '?')
      {
        reading = true; //found the ?, begin reading the info
        continue;
      }

      if(reading)
      {
        colorCommand[iColor] = c;
        iColor++;
        if(iColor >= 19)  // remember that C is 0 based, so 19 is the 20th character in the array
        {
          // The text is more than just a color (like an error message) -- stop reading and flush.
          Serial.println();
          Serial.println("Got more text than expected -- abandon attempt");

          client.println("<html>Error -- too much text in command</html>");
          client.println();

          delay(1000);
          client.stop();
          return;
        }

        if (c != '\r') 
        {
          // Ignore carriage returns  
          currentLineIsBlank = false;
        }
      }
    }
  }
  Serial.println();
  
  setColor(colorCommand);

  client.print("<html>Color set to ");
  client.print(colorCommand);
  client.print("<br>Tmp102 reads ");
  client.print(readTmp102());
  client.println("&degC.</html>");
  client.println();
  
  delay(1); // give the web browser time to receive the data
  client.stop(); // close the connection:
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

String colorName[] = {"none","red","green","blue","cyan","white","warmwhite","purple","magenta","yellow","orange"};
int colorRGB[][3] = {     0,  0,  0, // "none"
                        255,  0,  0, // "red"
                          0,255,  0, // "green"
                          0,  0,255, // "blue"
                          0, 255,255, // "cyan",
                        255, 50,100, // "white",
                        255, 30, 10, // "warmwhite",
                        128,  0, 25, // "purple",
                        255,  0, 50, // "magenta",
                        255, 100,  0, // "yellow",
                        255, 10,  0}; // "orange"};


void setColor(String color)
{
  for(int iColor = 0; iColor <= 10; iColor++)
  {
    if(color == colorName[iColor])
    {
      Serial.print("Set color to: "); Serial.println(colorName[iColor]);
      setRGB(colorRGB[iColor][0], colorRGB[iColor][1], colorRGB[iColor][2]);
      return;
    }
  }
  Serial.print("Could not match color: "); Serial.println(color);
}

void setRGB(int redValue, int greenValue, int blueValue)
{
  Serial.print("Setting to: ");Serial.print(redValue);Serial.print(" , ");Serial.print(greenValue);Serial.print(" , ");Serial.println(blueValue);
  analogWrite(pinRed,redValue);
  analogWrite(pinGreen,greenValue);
  analogWrite(pinBlue,blueValue);
}
