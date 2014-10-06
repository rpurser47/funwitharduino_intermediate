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

int pinRed = 9;
int pinGreen = 6;
int pinBlue = 5;

unsigned int sdCardSelectPin = 4;

// !!! You need to fill in the last byte here
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, ?? };
EthernetClient client;

void setup()
{
  Serial.begin(9600);

  // Explicitly deselect the SD card on SPI bus.  This is done by setting pin 4 high.
  // The ethernet interface is unreliable unless this is set high.
  pinMode(sdCardSelectPin, OUTPUT);
  digitalWrite(sdCardSelectPin, HIGH);

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
  
  // Connect the ethernet, contact DHCP server, and get IP address using Ethernet.begin
  Ethernet.begin(mac);
  Serial.print("Ethernet initialized and assigned address ");Serial.println(Ethernet.localIP());
}

void loop()
{
  // Connect to api.thingspeak.com on port 80 (HTTP)
  Serial.println("connecting to api.thingspeak.com...");
  boolean connected;
  connected = client.connect("api.thingspeak.com", 80);
  
  if (!connected)
  {
    Serial.println("connection failed -- abandon attempt");
    delay(1000);
    return;
  }

  Serial.println("connected to api.thingspeak.com -- getting page...");

  // Request/GET the page we want using client.println  Rememeber that HTTP protocol is "GET <full page path>"
  client.println("GET /channels/1417/field/1/last.txt");
  client.println();
  
  // As long as there's data from the server, keep reading it, and mirroring to serial port.
  Serial.println("Reading Data...");

  unsigned int iColor = 0;
  char colorCommand[20];
  long abortReadAt = millis() + 1000;
  
  while (client.connected() )
  {
    while(client.available()) 
    {
      // Read the data from the ethernet port, one character at a time using client.read
      char c = client.read();
      if(c == -1)
      {
        break;
      }
      
      colorCommand[iColor] = c;
      Serial.print(c);
      iColor++;
      if(iColor >= 19)  // remember that C is 0 based, so 19 is the 20th character in the array
      {
        // The text is more than just a color (like an error message) -- stop reading and flush.
        Serial.println();
        Serial.println("Got more text than expected -- abandon attempt");
        delay(1000);
        client.stop();
        return;
      }
      colorCommand[iColor] = 0;
    }
    
    if(millis() > abortReadAt)
    {
      // If reading for more than a second, give up and try again.
      Serial.println("Timeout reading -- abandon attempt");
      delay(1000);
      client.stop();
      return;
    }
  }
  
  Serial.println();
  
  // Server has disconnected
  Serial.println("disconnectted from api.thingspeak.com");
  client.stop();

  Serial.print("Color was: ");  Serial.println(colorCommand);
  setColor(colorCommand);
  
  delay(5000);
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
      Serial.print("Current Color is: "); Serial.println(colorName[iColor]);
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
