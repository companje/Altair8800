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
    LEDs.address = address;  
    LEDs.data = b;
    LEDs.write(); //updates LEDs during memory access
    return b;
  }

  uint16_t read16(uint16_t address) {
    return read(address) | (read(address+1)<<8);
  }

  void write(uint16_t address, byte data) {
    digitalWrite(SELECT, LOW);
    shiftOut(MOSI, SCK, MSBFIRST, 2); //WRITE
    shiftOut(MOSI, SCK, MSBFIRST, (address >> 16) & 255);
    shiftOut(MOSI, SCK, MSBFIRST, (address >> 8) & 255);
    shiftOut(MOSI, SCK, MSBFIRST, address);
    shiftOut(MOSI, SCK, MSBFIRST, data);
    digitalWrite(SELECT, HIGH);
    LEDs.address = address;  
    LEDs.data = data;
    LEDs.write(); //updates LEDs during memory access
  }

  void write16(uint16_t address, uint16_t data) {
    write(address, data & 0xff);
    write(address+1, (data >> 8) & 0xff);
  }

} RAM;


