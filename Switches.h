#include "Arduino.h"

class Switches {
public:

  const byte SELECT = 6;
  const byte DATA = 7;
  uint16_t ax = 0, cx = 0, _cx = 0; //ax=address, cx=control, _cx=previous control value
  enum ControlKey { STOP, SINGLE_STEP, EXAMINE, DEPOSIT, RUN, SINGLE_STEP_, EXAMINE_NEXT, DEPOSIT_NEXT, 
    AUX2, AUX1, PROTECT, RESET, AUX2_, AUX1_, UNPROTECT, CLR };

  Switches() {
    pinMode(SCK, OUTPUT);
    pinMode(SELECT, OUTPUT);
    pinMode(DATA, INPUT);
    digitalWrite(SELECT, HIGH); //disable
  }
  
  void read() {
    digitalWrite(SELECT, LOW);
    digitalWrite(SCK, HIGH);
    digitalWrite(SELECT, HIGH);
    byte al = shiftIn(DATA, SCK, MSBFIRST);
    byte ah = shiftIn(DATA, SCK, MSBFIRST);
    byte cl = shiftIn(DATA, SCK, MSBFIRST);
    byte ch = shiftIn(DATA, SCK, MSBFIRST);
    _cx = cx; //remember previous value
    ax = (ah<<8) + al;
    cx = (ch<<8) + cl;
  }
    
  boolean onRelease(ControlKey c) {
    return (_cx & (1<<c)) > 0 && (cx & (1<<c)) == 0;
  }

  boolean isDown(ControlKey c) {
    return cx & (1<<c);
  } 
  
  boolean onRun() { return onRelease(RUN); }
  boolean onStop() { return onRelease(STOP); }
  boolean onSingleStep() { return onRelease(SINGLE_STEP) || onRelease(SINGLE_STEP_); }
  boolean onExamine() { return onRelease(EXAMINE); }
  boolean onExamineNext() { return onRelease(EXAMINE_NEXT); }
  boolean onDeposit() { return onRelease(DEPOSIT); }
  boolean onDepositNext() { return onRelease(DEPOSIT_NEXT); }
  boolean onReset() { return onRelease(RESET); }
  boolean onClear() { return onRelease(CLR); }  
  boolean onAux1() { return onRelease(AUX1) || onRelease(AUX1_); }
  boolean onAux2() { return onRelease(AUX2) || onRelease(AUX2_); }
  
} Switches;
