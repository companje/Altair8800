#include "Arduino.h"

class RAM {
public:

  const byte SELECT = 10;

  RAM() {
    pinMode(SCK, OUTPUT);
    pinMode(MOSI, OUTPUT);
    pinMode(MISO, INPUT);
    pinMode(SELECT, OUTPUT);
    digitalWrite(SELECT, HIGH); //disable
  }
  
  byte read(uint16_t address) {
    digitalWrite(SELECT, LOW);
    shiftOut(MOSI, SCK, MSBFIRST, 3); //READ
    shiftOut(MOSI, SCK, MSBFIRST, (address >> 16) & 255);
    shiftOut(MOSI, SCK, MSBFIRST, (address >> 8) & 255);
    shiftOut(MOSI, SCK, MSBFIRST, address);
    byte b = shiftIn(MISO, SCK, MSBFIRST);
    digitalWrite(SELECT, HIGH);
    
    //updates LEDs during memory access
    LEDs.address = address;  
    LEDs.data = b;
    LEDs.write();
    
    return b;
  }

  void write(uint16_t address, byte data) {
    digitalWrite(SELECT, LOW);
    shiftOut(MOSI, SCK, MSBFIRST, 2); //WRITE
    shiftOut(MOSI, SCK, MSBFIRST, (address >> 16) & 255);
    shiftOut(MOSI, SCK, MSBFIRST, (address >> 8) & 255);
    shiftOut(MOSI, SCK, MSBFIRST, address);
    shiftOut(MOSI, SCK, MSBFIRST, data);
    digitalWrite(SELECT, HIGH);
    
    //updates LEDs during memory access
    LEDs.address = address;  
    LEDs.data = data;
    LEDs.write();
  }

} RAM;


