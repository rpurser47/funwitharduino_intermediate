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

// HH10D humidity sensor globals
PROGMEM const int hh10dAddress = 81;
PROGMEM const int hh10dInterrupt = 1; // On UNO, interrupt 0 is pin D2, interrupt 1 is pin D3
float hh10dOffset;
float hh10dSensitivity;

// MPL3115 barometric pressure sensor globals
PROGMEM const int MPL3115Address = 0x60;  

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
    sendStringInSRAM(server,PSTR("request.open(\"GET\", \"ajax_data\" + nocache, true);"));
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

void getDataCmd(WebServer &server, WebServer::ConnectionType type, char *command, bool)
{
  Serial.print("got request for ajax_data.  command was: ");
  Serial.println(command);
  /* this line sends the standard "we're all OK" headers back to the
     browser */
  server.httpSuccess();
  server.print("Tmp102 reads ");
  server.print(readTmp102());
  server.println("&degC.<br />");

  float RH;
  if(readHh10dRH(&RH))
  {
    server.print("HH10D reads ");
    server.print(RH);
    server.println("%<br />");
  }

  float pressureInPa, altTempInC;
  if(readMPL3115A1(&pressureInPa, &altTempInC))
  {
    server.print("MPL3115A1 reads: ");
    server.print(pressureInPa / 1000.0);  // Display in kPa
    server.print("kPa and ");
    server.print(altTempInC);
    server.println("&degC.<br />");
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
  webserver.addCommand("ajax_data", &getDataCmd);

  /* start the webserver */
  webserver.begin();

  Wire.begin();
  
  configHh10d();
  configMPL3115A1(493);
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

void configHh10d()
{
  // Sensor Datasheet: http://www.sparkfun.com/datasheets/Sensors/Temperature/HH10D.pdf
   hh10dSensitivity   =  float(i2cReadIntRegister(hh10dAddress, 10)); //Read sensitivity from HH10D EEPROM
   hh10dOffset =  float(i2cReadIntRegister(hh10dAddress, 12)); //Same for offset

//   Serial.print("HH10D sensitivity: "); Serial.println(hh10dSensitivity); 
//   Serial.print("HH10D offset: "); Serial.println(hh10dOffset); 

  // http://arduino.cc/en/Reference/AttachInterrupt
  attachInterrupt(hh10dInterrupt,hh10dInterruptHandler,RISING);
}

int i2cReadIntRegister(int deviceaddress, byte address)  
{
   // SET ADDRESS
   Wire.beginTransmission(deviceaddress);
   Wire.write(address); // address for sensitivity
   Wire.endTransmission();
   
   // REQUEST RETURN VALUE
   Wire.requestFrom(deviceaddress, 2);
   return Wire.read() << 8 | Wire.read();
}

// globals set by the hh10d interrupt handler
unsigned long hh10dInterruptCount = 0;
unsigned long hh10dFirstInterruptAt;
unsigned long hh10dLatestInterruptAt;

void hh10dInterruptHandler()
{
  if (hh10dInterruptCount == 0)
  {
    hh10dFirstInterruptAt = micros();
  }
  hh10dLatestInterruptAt = micros();
  hh10dInterruptCount++;
}

bool readHh10dRH(float * RH)
{
  unsigned long count;
  unsigned long start;
  unsigned long finish;
  
  // Capture the data from the ISR
  noInterrupts(); // Turn off interrupts momentarily to transfer data from ISR http://arduino.cc/en/Reference/noInterrupts
                  // Remember that these values change several thousand times per second!
  count = hh10dInterruptCount;
  start = hh10dFirstInterruptAt;
  finish = hh10dLatestInterruptAt;
  
  // Reset the ISR
  hh10dInterruptCount = 0;

  // Turn the interrupts back on 
  interrupts();

  float periodInUs;
 
  if (finish > start)
  {
    periodInUs = float(finish - start);
  }
  else
  {
    // See http://arduino.cc/en/Reference/Micros 
    // micros() will roll over every 70 minutes or so
    periodInUs = float(4294967295L - finish + start);
  }

  if(count < 1000 || periodInUs < 500000)
  {
    return false;
  }
  
//  Serial.print("hh10dFirstInterruptAt: ");Serial.println(start);
//  Serial.print("hh10dLatestInterruptAt: ");Serial.println(finish);
//  Serial.print("hh10dInterruptCount: ");Serial.println(count);
//  Serial.print("time period: ");Serial.println(periodInMs);
  
  // Calculate frequency
  float frequency = float(count) * 1000000.0 / periodInUs;
//  Serial.print("HH10D Frequency: ");Serial.println(frequency);
  *RH = (hh10dOffset-frequency)*hh10dSensitivity/4096.0;
//  Serial.print("HH10D RH: ");Serial.println(*RH);

  if (*RH < 0 || *RH > 100)
  {
    Serial.print("HH10D error: "); Serial.print(*RH); Serial.print(" ( "); Serial.print(count); Serial.print(", "); Serial.print(start); Serial.print(", "); Serial.print(finish); Serial.println(")");
    return false;
  }

  return true;
}

// Offset to use when returning barometric pressure readings (calibrated to sea level)
float localMeanBasePressureOffsetInPa;

bool configMPL3115A1(unsigned int localSeaLevelOffsetInPa)
{
  // Sensor datasheet: http://dlnmh9ip6v2uc.cloudfront.net/datasheets/Sensors/Pressure/MPL3115A2.pdf
  // To configure the sensor, find the register hex value and 
  // enter it into the first field of i2cWriteByteRegister (see below). Then
  // figure out the hex value of the data you want to send.
  // 
  // For example:
  // For CTRL_REG1, the address is 0x26 and the data is 0xB9
  // 0xB9 corresponds to binary 1011 1001. Each place holder 
   // represents a data field in CTRL_REG1. 
 
   // This is a basic II2 communication check. If the sensor doesn't
  // return decicmal number 196 (see 0x0C register in datasheet), 
  // error is printed. 
  if(i2cReadByteRegister(0x0C) != 196)
  {
    return false;
  }
 
  // CTRL_REG1 (0x26): put sensor in standby for programming, max oversampling, pressure mode
  // ALT:0, RAW:0, OST0-2:111 - 2^7, RST:0, OST:0, SYSB:0
  i2cWriteByteRegister(0x26, 0b00111000);
  
  // CTRL_REG4 (0x29): Data ready interrupt enabled
  // INT_EN_DRDY:1, INT_EN_FIFO:0, INT_EN_PW:0, INT_EN_TW:0,
  // INT_EN_PTH:0, INT_EN_TTH:0, INT_EN_PCHG:0, INT_EN_TCHG:0
  i2cWriteByteRegister(0x29, 0x80); 
  
  // PT_DATA_CFG (0x13): enable both pressure and temp event flags 
  // DREM:1, PDEFE:1, TDEFE:1
  i2cWriteByteRegister(0x13, 0x07);
  
  // This coniguration option calibrates the sensor according to 
  // the sea level pressure for the measurement location
  // 101326Pa -- standard mean sea level pressure
  localMeanBasePressureOffsetInPa = localSeaLevelOffsetInPa;
  // Note that you have to enter a value that is ONE HALF of your target value.
  unsigned int BAR_IN = (101326 + localSeaLevelOffsetInPa) / 2;
  
  // BAR_IN_MSB (0x14):
  byte BAR_IN_MSB = (BAR_IN & 0xFF00) >> 8;
  i2cWriteByteRegister(0x14, BAR_IN_MSB);
//  Serial.print("BAR_IN_MSB: "); Serial.print(BAR_IN_MSB, HEX);
  
  // BAR_IN_LSB (0x15):
  byte BAR_IN_LSB = BAR_IN & 0x00FF;
  i2cWriteByteRegister(0x15, BAR_IN_LSB);
//  Serial.print(" LSB: "); Serial.println(BAR_IN_LSB, HEX);

  // CTRL_REG1 (0x26): put sensor in run mode, max oversampling, pressure mode
  // ALT:0, RAW:0, OST0-2:111 - 2^7, RST:0, OST:0, SYSB:1
  i2cWriteByteRegister(0x26, 0b00111001);

  return true;  
}

bool readMPL3115A1(float * pressure, float * temperature)
{
  // If INT_SOURCE (0x12) register's DRDY flag is enabled, return
  byte readyByte = i2cReadByteRegister(0x12);
  if(readyByte != 0x80) // check INT_SOURCE register on new data ready (SRC_DRDY)
  {
    Serial.print("MPL3115A1 INT_SOURCE byte was: ");Serial.println(readyByte);
    return false;
  }
  
//  Serial.println("New data at MPL3115A1");

  // Information on data format from data sheet, section 7.1.3:
  // http://dlnmh9ip6v2uc.cloudfront.net/datasheets/Sensors/Pressure/MPL3115A2.pdf

  // variables for the calculations
  unsigned int OUT_P_MSB, OUT_P_CSB, OUT_P_LSB; 
  unsigned int OUT_T_MSB, OUT_T_LSB; 
  
  OUT_P_MSB = i2cReadByteRegister(0x01);
  OUT_P_CSB = i2cReadByteRegister(0x02);
  OUT_P_LSB = i2cReadByteRegister(0x03);

  //The Pressure data is arranged as 20-bit unsigned data in Pascals. The first 18 bits are located in OUT_P_MSB, OUT_P_CSB 
  //and bits 7-6 of OUT_P_LSB. The 2 bits in position 5-4 of OUT_P_LSB represent the fractional component. 
  //When RAW bit is set (CTRL_REG1), then the RAW value is stored in all 24 bits of OUT_P_MSB, OUT_P_CSB and 
  //OUT_P_LSB. 
  //Shift MSB over by 10 bits (since it's an 18 bit value, the shift needs to be done in a long)
  *pressure = float(long(OUT_P_MSB)<<10);
  //Shift CSB over by 2 bits and add to the total
  *pressure = *pressure + (float)(OUT_P_MSB<<2);
  //Mask out bits 6&7 in CSB and shift over by 6 bits and add to the total
  *pressure = *pressure + (float)((OUT_P_LSB & 0b11000000)>>6);
  // The least significant bytes OUT_P_LSB are 2-bit,
  // fractional values in bits 4&5, so you must mask them out, cast the calulation in (float),
  // shift the value over 4 bits to the right and divide by 4 (since 
  // there are 4 values in 2-bits). 
  *pressure = *pressure + float((OUT_P_LSB & 0b00110000)>>4)/4.0;
  
  // Apply an offset to calibrate results to sea level (the way weather stations work)
  *pressure = *pressure + localMeanBasePressureOffsetInPa;

  //The Altitude data is arranged as 20-bit 2’s complement value in meters. The data is stored as meters with the 16 bits of 
  //OUT_P_MSB and OUT_P_CSB and with fractions of a meter stored in bits 7-4 of OUT_P_LSB. Be aware that the fractional bits 
  //are not signed, therefore, they are not represented in 2’s complement. 
//  *altitude = (float)((OUT_P_MSB << 8)|OUT_P_CSB);
  // For altutude, the least significant bytes OUT_P_LSB are 4-bit,
  // fractional values, so you must cast the calulation in (float),
  // shift the value over 4 bits to the right and divide by 16 (since 
  // there are 16 values in 4-bits). 
//  *altitude = *altitude + float(OUT_P_LSB>>4)/16.0;

  //The Temperature data is arranged as 12-bit 2’s complement value in degrees C. The 8 bits of OUT_T_MSB representing degrees and
  //with fractions of a degree are stored in 4 bits in position 7-4 of OUT_T_LSB. Be aware that the fractional bits are not 
  //signed, therefore, they are not represented in 2’s complement. When RAW is selected then the RAW value is stored in all 16 bits
  //of OUT_T_MSB and OUT_T_LSB.
  OUT_T_MSB = i2cReadByteRegister(0x04); 
  OUT_T_LSB = i2cReadByteRegister(0x05); 
  *temperature = float(OUT_T_MSB) + float(OUT_T_LSB>>4)/16.0; //temp, fraction of a degree

  if (*pressure < 50000 || *pressure > 150000 ||
      *temperature < -20 || *temperature > 50)
  {
    Serial.println("MPL3115A1 error: "); 
    Serial.print(*pressure); Serial.print(" ( "); Serial.print(OUT_P_MSB); Serial.print(", "); Serial.print(OUT_P_CSB); Serial.print(", "); Serial.print(OUT_P_LSB); Serial.println(")");
    Serial.print(*temperature); Serial.print(" ( "); Serial.print(OUT_T_MSB); Serial.print(", "); Serial.print(OUT_T_LSB); Serial.println(")");
    return false;
  }
  
  return true;
}

byte i2cReadByteRegister(byte regAddr)
{
  // This function reads one byte over IIC
  Wire.beginTransmission(MPL3115Address);
  Wire.write(regAddr);  // Address of CTRL_REG1
  Wire.endTransmission(false); // Send data to I2C dev with option
                               //  for a repeated start. THIS IS
                               //  NECESSARY and not supported before
                               //  Arduino V1.0.1!!!!!!!!!
  Wire.requestFrom(MPL3115Address, 1); // Request the data...
  return Wire.read();
}

void i2cWriteByteRegister(byte regAddr, byte value)
{
  // This function writes one byto over IIC  
  Wire.beginTransmission(MPL3115Address);
  Wire.write(regAddr);
  Wire.write(value);
  Wire.endTransmission(true);
}  

