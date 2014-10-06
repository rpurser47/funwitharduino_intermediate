// Fun with Arduino by Rob Purser is licensed under a
// Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// http://creativecommons.org/licenses/by-nc-sa/3.0/deed.en_US
// Based on a work at http://www.funwitharduino.net 
// Copyright Rob Purser, 2013-2014

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetServer.h>
#include <SD.h>

// Ethernet card globals
unsigned int sdCardSelectPin = 4;
// !!! You need to fill in the last byte here
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, ?? };
byte ipAddress[] = { 192, 168, 1, 2 };
EthernetServer server = EthernetServer(80); //port 80

void setup()
{
  Serial.begin(9600);

  SD.begin(sdCardSelectPin);

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

      if(c == ' ')
      {
        // End of the message
        break;
      }
    }
  }

  File myFile = SD.open("testpage.htm", FILE_READ);
  if(!myFile)
  {
    client.println("Could not open testpage.htm");
    delay(1); // give the web browser time to receive the data
    client.stop(); // close the connection:
    return;
  }
  while (myFile.available()) 
  {
    byte c = myFile.read();
    if(c != -1)
    {
      client.write((char)c);
    }
  }
  myFile.close();

  client.println();
  
  delay(1); // give the web browser time to receive the data
  client.stop(); // close the connection:
}  

