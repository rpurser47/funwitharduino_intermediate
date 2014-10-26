// Fun with Arduino by Rob Purser is licensed under a
// Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// http://creativecommons.org/licenses/by-nc-sa/3.0/deed.en_US
// Based on a work at http://www.funwitharduino.net 
// Copyright Rob Purser, 2013-2014
// This file Adapted from the Webduino Web_HelloWorld example

#include <SPI.h>
#include <Ethernet.h>
// This sketch requires the webduino library from
// https://github.com/sirleech/Webduino
#include <WebServer.h>
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
static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, ?? };
static uint8_t ip[] = { 192, 168, 1, 2 };

/* This creates an instance of the webserver.  By specifying a prefix
 * of "", all pages will be at the root of the server. */
#define PREFIX ""
WebServer webserver(PREFIX, 80);

/* commands are functions that get called by the webserver framework
 * they can read any posted data from client, and they output to the
 * server to send data back to the web browser. */
 
void indexCmd(WebServer &server, WebServer::ConnectionType type, char *colorCommand, bool)
{
  /* this line sends the standard "we're all OK" headers back to the
     browser */
  server.httpSuccess();

  /* if we're handling a GET or POST, we can output our data here.
     For a HEAD request, we just stop after outputting headers. */
  if (type != WebServer::HEAD)
  {
    if(colorCommand[0] != 0)
    {
      Serial.println(colorCommand);
      setColor(colorCommand);
      server.print("Color set to ");
      server.print(colorCommand);
      server.print("<br>");
    }
    server.print("Tmp102 reads ");
    server.print(readTmp102());
    server.println("&degC.");

  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Started");
   
  // Set up the Ethernet card
  // Explicitly deselect the SD card on SPI bus.  This is done by setting pin 4 high.
  // The ethernet interface is unreliable unless this is set high.
  pinMode(sdCardSelectPin, OUTPUT);
  digitalWrite(sdCardSelectPin, HIGH);

  // Connect the ethernet, set static IP address using Ethernet.begin
  Ethernet.begin(mac, ip);
  Serial.print("Ethernet initialized and assigned address ");Serial.println(Ethernet.localIP());
  
  /* setup our default command that will be run when the user accesses
   * the root page on the server */
  webserver.setDefaultCommand(&indexCmd);

  /* run the same command if you try to load /index.html, a common
   * default page name */
  webserver.addCommand("index.html", &indexCmd);

  /* start the webserver */
  webserver.begin();
}

void loop()
{
  char buff[64];
  int len = 64;

  /* process incoming connections one at a time forever */
  webserver.processConnection(buff, &len);
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

