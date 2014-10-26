// Fun with Arduino by Rob Purser is licensed under a
// Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// http://creativecommons.org/licenses/by-nc-sa/3.0/deed.en_US
// Based on a work at http://www.funwitharduino.net 
// Copyright Rob Purser, 2013-2014
// This file adapted from Arduino Ethernet Shield Web Server Tutorial 
// http://startingelectronics.com/tutorials/arduino/ethernet-shield-web-server-tutorial/web-server-read-switch-using-AJAX/

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
 
void indexCmd(WebServer &server, WebServer::ConnectionType type, char *command, bool)
{
  /* this line sends the standard "we're all OK" headers back to the
     browser */
  server.httpSuccess();

  /* if we're handling a GET or POST, we can output our data here.
     For a HEAD request, we just stop after outputting headers. */
  if (type == WebServer::HEAD)
  {
    return;
  }

    // send web page - contains JavaScript with AJAX calls
    sendStringInSRAM(server,PSTR("<!DOCTYPE html>"));
    sendStringInSRAM(server,PSTR("<html>"));
    sendStringInSRAM(server,PSTR("<head>"));
    sendStringInSRAM(server,PSTR("<title>Arduino Web Page</title>"));
    sendStringInSRAM(server,PSTR("<script type=\"text/javascript\">"));
    sendStringInSRAM(server,PSTR("function getTemperature() {"));
    sendStringInSRAM(server,PSTR("nocache = \"?nocache=\" + Math.random() * 1000000;"));
    sendStringInSRAM(server,PSTR("var request = new XMLHttpRequest();"));
    sendStringInSRAM(server,PSTR("request.onreadystatechange = function() {"));
    sendStringInSRAM(server,PSTR("if (this.readyState == 4) {"));
    sendStringInSRAM(server,PSTR("if (this.status == 200) {"));
    sendStringInSRAM(server,PSTR("if (this.responseText != null) {"));
    sendStringInSRAM(server,PSTR("document.getElementById(\"status_txt\").innerHTML = this.responseText;"));
    sendStringInSRAM(server,PSTR("}}}}"));
    sendStringInSRAM(server,PSTR("request.open(\"GET\", \"ajax_temp\" + nocache, true);"));
    sendStringInSRAM(server,PSTR("request.send(null);"));
    // This line was added to cause an update every second.
    sendStringInSRAM(server,PSTR("setTimeout('getTemperature()', 1000);"));    
    sendStringInSRAM(server,PSTR("}"));
    sendStringInSRAM(server,PSTR("</script>"));
    sendStringInSRAM(server,PSTR("</head>"));
    // This line is modified to run the script once on load to "prime" the pump
    sendStringInSRAM(server,PSTR("<body onload=\"getTemperature()\">"));
    sendStringInSRAM(server,PSTR("<h1>Arduino AJAX Temperature Monitor</h1>"));
    sendStringInSRAM(server,PSTR("<p id=\"status_txt\">Temperature: Not requested...</p>"));
//    sendStringInSRAM(server,PSTR("<button type=\"button\" onclick=\"getTemperature()\">Get Temperature</button>"));
    sendStringInSRAM(server,PSTR("</body>"));
    sendStringInSRAM(server,PSTR("</html>"));
}

void getTempCmd(WebServer &server, WebServer::ConnectionType type, char *command, bool)
{
  Serial.print("got request for ajax_temp.  command was: ");
  Serial.println(command);
  /* this line sends the standard "we're all OK" headers back to the
     browser */
  server.httpSuccess();
  server.print("Tmp102 reads ");
  server.print(readTmp102());
  server.println("&degC.");
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
  Ethernet.begin(mac,ip);
  Serial.print("Ethernet initialized and assigned address ");Serial.println(Ethernet.localIP());
  
  /* setup our default command that will be run when the user accesses
   * the root page on the server */
  webserver.setDefaultCommand(&indexCmd);

  /* run the same command if you try to load /index.html, a common
   * default page name */
  webserver.addCommand("index.html", &indexCmd);
  webserver.addCommand("ajax_temp", &getTempCmd);

  /* start the webserver */
  webserver.begin();
}

void sendStringInSRAM (WebServer &server, PGM_P s) {
    char c;
    while ((c = pgm_read_byte(s++)) != 0)
        server.print(c);
    server.println("");
}

void loop()
{
  char buff[64];
  int len = 64;

  /* process incoming connections one at a time forever */
  webserver.processConnection(buff, &len);
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


