class Panel {
public:

  uint16_t ax = 0, cx = 0, _cx = 0; //ax=address, cx=control, _cx=previous control value
  enum ControlKey { STOP, SINGLE_STEP, EXAMINE, DEPOSIT, RUN, SINGLE_STEP_, EXAMINE_NEXT, DEPOSIT_NEXT, 
    AUX2_UP, AUX1_UP, PROTECT, RESET, AUX2_DOWN, AUX1_DOWN, UNPROTECT, CLR };
  
  Panel() {
    pinMode(LED_SELECT, OUTPUT);
    pinMode(LED_DATA, OUTPUT);
    pinMode(CLOCK, OUTPUT);
    digitalWrite(LED_SELECT, HIGH); //disable
    pinMode(SW_SELECT, OUTPUT);
    pinMode(SW_DATA, INPUT);
    digitalWrite(SW_SELECT, HIGH); //disable
  }

  //the LEDs for HLTA, STACK, WO and INT are not connected
  //it's better to remove INTE, PROT, WAIT and HLDA since these are not real STATUS bits of the processor.
  void writeLEDs() {
    digitalWrite(LED_SELECT, LOW);
    shiftOut(LED_DATA, CLOCK, MSBFIRST, bus.state);
    shiftOut(LED_DATA, CLOCK, MSBFIRST, bus.address >> 8);
    shiftOut(LED_DATA, CLOCK, MSBFIRST, bus.address);
    shiftOut(LED_DATA, CLOCK, MSBFIRST, bus.data);
    digitalWrite(LED_SELECT, HIGH);
  }
  
  void readSwitches() {
    digitalWrite(SW_SELECT, LOW);
    digitalWrite(CLOCK, HIGH);
    digitalWrite(SW_SELECT, HIGH);
    byte al = shiftIn(SW_DATA, CLOCK, MSBFIRST);
    byte ah = shiftIn(SW_DATA, CLOCK, MSBFIRST);
    byte cl = shiftIn(SW_DATA, CLOCK, MSBFIRST);
    byte ch = shiftIn(SW_DATA, CLOCK, MSBFIRST);
    _cx = cx; //remember previous value
    ax = (ah<<8) + al;
    cx = (ch<<8) + cl;
  }
  
  byte sense() {
    return ax >> 8;
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
  boolean onAux1Up() { return onRelease(AUX1_UP); }
  boolean onAux1Down() { return onRelease(AUX1_DOWN); }
  boolean onAux2Up() { return onRelease(AUX2_UP); }
  boolean onAux2Down() { return onRelease(AUX2_DOWN); }
  
} panel;
