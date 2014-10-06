// Fun with Arduino by Rob Purser is licensed under a
// Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// http://creativecommons.org/licenses/by-nc-sa/3.0/deed.en_US
// Based on a work at http://www.funwitharduino.net
// Copyright Rob Purser, 2013-2014

#include <b64.h>
#include <SPI.h>
#include <HttpClient.h>
#include <Ethernet.h>

unsigned int sdCardSelectPin = 4;

// You need to fill in the last byte here
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, ?? };

// Constant I like for infinite loops
boolean discoSucks = true;

void setup()
{
  Serial.begin(9600); 

  // Explicitly deselect the SD card on SPI bus.  This is done by setting pin 4 high.
  // The ethernet interface is unreliable unless this is set high.
  pinMode(sdCardSelectPin, OUTPUT);
  digitalWrite(sdCardSelectPin, HIGH);

  // Connect the ethernet, contact DHCP server, and get IP address
  if (Ethernet.begin(mac) != 1)
  {
    Serial.println("Error getting IP address via DHCP");
    while(discoSucks);
  }  
  Serial.print("Ethernet initialized and assigned address ");Serial.println(Ethernet.localIP());
}

void loop()
{
  EthernetClient client;
  HttpClient http(client);
  
  // Request/GET the page we want
  http.get("www.timeapi.org", "/utc/now");
  int status = http.responseStatusCode();
  Serial.print("GET returned status of ");Serial.println(status);
  
  // Ignore the many, many headers, and jump right to the content
  http.skipResponseHeaders();

  // As long as there's data from the server, keep reading it, and mirroring to serial port.
  while (!http.endOfBodyReached())
  {
    Serial.print(char(http.read()));
  }

  // Server has disconnected
  http.stop();
  while(discoSucks);
}


