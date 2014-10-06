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

unsigned int sdCardSelectPin = 4;

// !!! You need to fill in the last byte here
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, ?? };
EthernetClient client;

boolean discoSucks = true;

void setup()
{
  Serial.begin(9600);

  // Explicitly deselect the SD card on SPI bus.  This is done by setting pin 4 high.
  // The ethernet interface is unreliable unless this is set high.
  pinMode(sdCardSelectPin, OUTPUT);
  digitalWrite(sdCardSelectPin, HIGH);
  
  // Connect the ethernet, contact DHCP server, and get IP address
  Ethernet.begin(mac);
  Serial.print("Ethernet initialized and assigned address ");Serial.println(Ethernet.localIP());

  // Connect to the server on port 80 (HTTP)
  Serial.println("connecting...");
  if (client.connect("www.timeapi.org", 80))
  {
    Serial.println("connected to www.timeapi.org");

    // Request/GET the page we want
    client.println("GET /edt/now HTTP/1.1");
    client.println("Host: www.timeapi.org");
    client.println();
  }
  else
  {
    Serial.println("connection failed");
    while(discoSucks);
  }
}

void loop()
{
  // As long as there's data from the server, keep reading it, and mirroring to serial port.
  if (client.available()) 
  {
    char c = client.read();
    Serial.print(c);
  }

  if (!client.connected()) 
  {
    // Server has disconnected
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    while(discoSucks);
  }
  }
