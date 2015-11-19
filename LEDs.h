#include "Arduino.h"

class LEDs {
public:

  const byte SELECT = 8;
  const byte DATA = 9;
  uint16_t address = 0;
  byte data = 0;
  byte flags = 0;
  
  enum Flag { HLDA, WAIT, INTE, PROT, MEMR, INP, M1, OUT }; //HLTA, STACK, WO and INT are not connected

  LEDs() {
    pinMode(SCK, OUTPUT);
    pinMode(SELECT, OUTPUT);
    pinMode(DATA, OUTPUT);
    digitalWrite(SELECT, HIGH); //disable
  }

  void write() {
    digitalWrite(SELECT, LOW);
    shiftOut(DATA, SCK, MSBFIRST, flags);
    shiftOut(DATA, SCK, MSBFIRST, address >> 8);
    shiftOut(DATA, SCK, MSBFIRST, address);
    shiftOut(DATA, SCK, MSBFIRST, data);
    digitalWrite(SELECT, HIGH);
  }
  
  void setFlag(Flag flag, boolean value) {
    bitWrite(flags,flag,value);
  }

} LEDs;
