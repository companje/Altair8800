#include "Arduino.h"

#define DEST(x) (x >> 3 & 7)
#define SOURCE(x) (x & 7)
#define CONDITION(x) (x >> 3 & 7)
#define VECTOR(x) (x >> 3 & 7)
#define RP(x) (x >> 4 & 3)

#define read8(a) RAM.read(a)
#define read16(a) RAM.read16(a)
#define write8(a,v) RAM.write(a,v)
#define write16(a,v) RAM.write16(a,v)

class CPU {
public:
  boolean m1 = true; //Machine Cycle 1, fetching an opcode
  boolean memr = false; //Memory Read
  boolean stack = false; //on when using the stack (push, pop, ...)
  boolean wait = true;
  byte opcode = 0; // the last fetched opcode
  byte db,lb,hb;
 
  enum Flag { CARRY=1, PARITY=4, HALF=16, IF=32, ZERO=64, SIGN=128 };
  enum Condition { NZ,Z,NC,CC,PO,PE,P,M }; //NZ=!zero, Z=zero, NC=!carry, C=Carry, PO=parity odd, PE=parity even, P=!sign = positive, M=sign = minus/negative 
  enum Register {B,C,D,E,H,L,MEM,A };
  enum RegisterPair { BC,DE,HL,STP };

  union { uint16_t af; struct { byte flags, a; }; };  //a=accumulator,  //f=flags (S Z 0 A 0 P 1 C), af is also called PSW, S=sign, Z=zero, A=aux-carry, P=parity, C=carry 
  union { uint16_t bc; struct { byte b, c; }; }; 
  union { uint16_t de; struct { byte e, d; }; }; 
  union { uint16_t hl; struct { byte l, h; }; };   

  uint16_t sp; //stack pointer
  uint16_t pc; //program counter
  
  void setFlag(byte flag, byte value = true) {
    if (value) flags |= flag;
    else clearFlag(flag);
  }
  
  byte getFlag(byte flag) {
    return !!(flags & flag);
  }
  
  void clearFlag(byte flag) {
    flags &= ~flag;
  }
  
  bool getParity(byte val) {
    val ^= val >> 4;
    val &= 0xf;
    return !((0x6996 >> val) & 1);
  }

  byte getRegister(byte reg) {
     switch (reg) {
       case A: return a;
       case B: return b;
       case C: return c;
       case D: return d;
       case E: return e;
       case H: return h;
       case L: return l;
       case MEM: return RAM.read(hl);
       default: Serial.print(F("getRegister: Unknown register ")); LOG("%04x:%02x",pc,reg); stop();
     }
  }
  
  void setRegister(byte reg, byte value) {
     switch (reg) {
       case A: a = value; break;
       case B: b = value; break;
       case C: c = value; break;
       case D: d = value; break;
       case E: e = value; break;
       case H: h = value; break;
       case L: l = value; break;
       case MEM: RAM.write(hl,value); break;
       default: Serial.print(F("setRegister: Unknown register ")); LOG("%04x:%02x",pc,reg); stop();
     }
  }
  
  uint16_t getRegisterPair(byte pair) {
    switch (pair) {
      case BC: return bc;
      case DE: return de;
      case HL: return hl;
      case STP: return sp;
      default: Serial.print(F("getRegisterPair: Unknown register pair ")); LOG("%04x:%02x\n",pc,pair); stop();
    }
  }
  
  void setRegisterPair(byte pair, uint16_t value) {
    switch (pair) {
      case BC: bc = value; break;
      case DE: de = value; break;
      case HL: hl = value; break;
      case STP: sp = value; break;
      default: Serial.print(F("setRegisterPair: Unknown register pair ")); LOG("%04x:%02x\n",pc,pair); stop();
    }
  }
  
  void gensub(byte val) { //generic subtract
    uint16_t aa = a;
    uint16_t bb = 0x100 - val;
    setFlag(HALF, checkHalfCarry(aa,bb));
    setFlag(CARRY, checkCarry(aa,bb));
    aa += bb;
    a = aa & 0xff; //update accumulator
    updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF);
  }
  
  void genadd(byte val) { //generic add
    setFlag(HALF, checkHalfCarry(a,val));
    setFlag(CARRY, checkCarry(a,val));
    a += val;
    updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF);
  }
  
  void compare(byte val) {
    byte tmp = a;
    gensub(val);
    a = tmp; //restore a 
  }
  
  void updateFlags(byte reg, byte mask) {
    byte val = getRegister(reg);
    if (mask & PARITY) setFlag(PARITY,getParity(val));
    if (mask & HALF) { /*TODO?*/ }
    if (mask & IF) { /*TODO?*/ }
    if (mask & ZERO) setFlag(ZERO,val==0);
    if (mask & SIGN) setFlag(SIGN,val & 0x80);
  }
  
  bool checkHalfCarry(uint16_t a, uint16_t b) {
    a &= 0xf;
    b &= 0xf;
    return (a+b) > 0xf;
  }
  
  bool checkCarry(uint16_t a, uint16_t b) {
    return (a+b) > 0xff; 
  }
  
  bool checkCondition(byte condition) {
    switch ((Condition)condition) {
      case NZ: return !(flags & ZERO);
      case Z:  return (flags & ZERO);
      case NC: return !(flags & CARRY);
      case CC: return (flags & CARRY); //CC = Carry 'Condition'
      case PO: return !(flags & PARITY);
      case PE: return (flags & PARITY);
      case P:  return !(flags & SIGN);
      case M:  return (flags & SIGN);
    }
  }
  
  byte fetch() {
    return read8(pc);
  }

  void store(byte value) {
    write8(pc, value);
  }
  
  void daa() { //Decimal Adjust accumulator
    byte val=a, add=0;
    if ((val & 0xf) > 9 || getFlag(HALF)) add += 0x06;
    val += add;
    if (((val & 0xf0) >> 4) > 9 || getFlag(CARRY)) add += 0x60;
    genadd(add);
    pc++; 
  } 
  
  void dad() { //Add register pair to HL (16 bit add)
    uint32_t val = getRegisterPair(RP(opcode));
    val += hl;
    setFlag(CARRY, val > 0xffff);
    hl = val & 0xffff;
    pc++;
  } 
 
  void dcr() { 
    byte val = getRegister(DEST(opcode));
    setFlag(HALF, checkHalfCarry(val, 0xff));
    setRegister(DEST(opcode), val + 0xff);
    updateFlags(DEST(opcode), ZERO | PARITY | SIGN | HALF); //no carry check here? FIXED: CARRY was replaced by PARITY
    pc++;
  } //Decrement register 
  
  void inr() { 
    byte val = getRegister(DEST(opcode)); 
    setFlag(HALF,checkHalfCarry(val,1));
    setRegister(DEST(opcode),val+1);
    updateFlags(DEST(opcode), ZERO | PARITY | SIGN | HALF); //no carry check here? FIXED: CARRY was replaced by PARITY
    pc++;
  } //Increment register
  
  //math operations & compare
  void aci() { genadd(read8(pc+1) + getFlag(CARRY)); pc+=2; } //Add immediate to A with carry
  void adc() { genadd(getRegister(SOURCE(opcode)) + getFlag(CARRY)); pc++; } //Add register to A with carry
  void add() { genadd(getRegister(SOURCE(opcode))); pc++; } //Add register to A
  void adi() { genadd(read8(pc+1)); pc+=2; } //Add immediate to A
  void sbb() { uint16_t val = getRegister(SOURCE(opcode)); if (getFlag(CARRY)) val++; gensub(val); pc++; } //Subtract register from A with borrow
  void sbi() { uint16_t val = read8(pc+1); if (getFlag(CARRY)) val++; gensub(val); pc+=2; } //Subtract immediate from A with borrow
  void sub() { byte val = getRegister(SOURCE(opcode)); gensub(val); pc++; } //Subtract register from A
  void sui() { gensub(read8(pc+1)); pc+=2; } //Subtract immediate from A
  void cmp() { compare(getRegister(SOURCE(opcode))); pc++; } //Compare register with A
  void cpi() { compare(read8(pc+1)); pc+=2; } //Compare immediate with A
  void dcx() { setRegisterPair(RP(opcode), getRegisterPair(RP(opcode))-1);  pc++; } //Decrement register pair
  void inx() { setRegisterPair(RP(opcode), getRegisterPair(RP(opcode)+1)); pc++; } //Increment register pair

  //bitwise operations
  void ana() { a &= getRegister(SOURCE(opcode)); clearFlag(CARRY); updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF); pc++; } //AND register with A
  void ani() { a &= read8(pc+1); clearFlag(CARRY); clearFlag(HALF); updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF); pc+=2; } //AND immediate with A
  void ora() { a |= getRegister(SOURCE(opcode)); clearFlag(CARRY); clearFlag(HALF); updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF); pc++; } //OR register with A
  void ori() { a |= read8(pc+1); clearFlag(CARRY); clearFlag(HALF); updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF); pc+=2; } //OR immediate with A
  void xra() { a ^= getRegister(SOURCE(opcode)); clearFlag(CARRY); clearFlag(HALF); updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF); pc++; } //XOR register with A
  void xri() { a ^= read8(pc+1); clearFlag(CARRY); clearFlag(HALF); updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF); pc+=2; } //XOR immediate with A
  void ral() { byte hi = a & 0x80; a <<= 1; if (getFlag(CARRY)) a |= 1; else a &= ~1; setFlag(CARRY,hi); pc++; } //Rotate A left through carry
  void rar() { byte lo = a & 1; a >>= 1; if (getFlag(CARRY)) a |= 0x80; else a &= ~0x80; setFlag(CARRY,lo); pc++; } //Rotate A right through carry
  void rlc() { byte hi = a & 0x80; a <<= 1; setFlag(CARRY,hi); if (hi) a |= 1; else a &= ~1; pc++; } //Rotate A left
  void rrc() { byte lo = a & 1; a >>= 1; setFlag(CARRY,lo); if (lo) a |= 0x80; else a &= ~0x80; pc++; } //Rotate A right  
  void cma() { a=~a; pc++; } //Compliment A

  //jumps & subroutines
  void call() { sp-=2; write16(sp, pc+3); pc=read16(pc+1); } //Unconditional subroutine call
  void cccc() { if (checkCondition(CONDITION(opcode))) call(); else pc+=3; } //Conditional subroutine call
  void jccc() { if (checkCondition(CONDITION(opcode))) jmp(); else pc+=3; } //Conditional jump
  void jmp() { pc = read16(pc+1); } //Unconditional jump
  void rccc() { if (checkCondition(CONDITION(opcode))) ret(); else pc++; } //Conditional return from subroutine
  void ret() { pc = read16(sp); sp+=2; }; //Unconditional return from subroutine
  void rst() { sp-=2; write16(sp,pc+1); pc = DEST(opcode)*8; } //Restart (Call n*8)
  void pchl() { pc = hl; } //Jump to address in H:L

  //assignment
  void lda() { a = read16(pc+1); pc+=3; } //Load A from memory
  void ldax() { a = read8(getRegisterPair(RP(opcode))); pc++; } //Load indirect through BC or DE (TODO: only BC and DE allowed for indirect)
  void lhld() { hl = read16(read16(pc+1)); } //Load H:L from memory
  void lxi() { setRegisterPair(RP(opcode), read16(pc+1)); pc+=3; } //Load register pair immediate
  void mov() { setRegister(DEST(opcode), getRegister(SOURCE(opcode))); pc++; } //Move register to register
  void mvi() { setRegister(DEST(opcode), read8(pc+1)); pc+=2; } //Move immediate to register
  void shld() { write16(read16(pc+1), hl); pc+=3; } //Store H:L to memory
  void sta() { write16(read16(pc+1),a); pc+=3; } //Store A to memory
  void stax() { write8(getRegisterPair(RP(opcode)),a); pc++; } //Store indirect through BC or DE // TODO: only BC and DE allowed for indirect
  void xchg() { uint16_t tmp=hl; hl=de; de=tmp; pc++; } //Exchange DE and HL content
  void xthl() { uint16_t tmp=sp; write16(sp,hl); hl=tmp; pc++; } //Swap H:L with top word on stack

  //carry flag operations
  void cmc() { flags ^= CARRY; pc++; } //Compliment Carry flag
  void stc() { setFlag(CARRY); pc++; } //Set carry flag

  //interrupts & ...
  void di() { pc++; clearFlag(IF); } //Disable interrupts
  void ei() { pc++; setFlag(IF); } //Enable interrupts
  void hlt() { wait = true; } //Halt processor
  void nop() { pc++; } //No operation
  
  //stack
  void sphl() { sp=hl; pc++; } //Set SP to content of H:L
  void pop() { uint16_t val = read16(sp); sp+=2; if (RP(opcode)==STP) af = val; else setRegisterPair(RP(opcode), val); pc++; } //Pop register pair from the stack
  void push() { uint16_t val = (RP(opcode)==STP) ? af : getRegisterPair(RP(opcode)); sp-=2; write16(sp, val); pc++; } //Push register pair on the stack

  void in() { //Read input port into A
    byte port = read8(pc+1);

    switch (port) {
      case 0x00: a = Serial.available()==0; break; //negative LOGic
      case 0x01: a = Serial.read(); break; //Serial.write(a); 
      case 0x08: Serial.print(F("in(): disk status TODO")); stop(); break;
      case 0x09: Serial.print(F("in(): disk controller TODO")); stop(); break;
      case 0x0a: Serial.print(F("in(): disk controller TODO")); stop(); break;
      case 0x10: Serial.print(F("in(): 2SIO port 1, status TODO")); stop(); break;
      case 0x11: Serial.print(F("in(): 2SIO port 1, read TODO")); stop(); break;

      // case 0x8c: a = Serial.read(); break; //a = Serial.available()==0; break; //Serial.available()==0; break;
      // case 0x8d: a = Serial.read(); break; //Serial.available()==0; break;

      case 0xff: a = Switches.ax >> 8; Serial.print("sense: "); Serial.println(a,HEX); break;
      default: Serial.print(F("in(): Unknown port ")); LOG("%04x:%02x",pc,port); stop();
    }
    
    LOG("%04x: in %02x => %02x",pc,port,a);

    pc+=2;
  }
  
  void out() { //Write A to output port
    byte port = read8(pc+1);

    LOG("%04x: out %02x => %02x",pc,port,a);

    switch (port) {
      case 0x00: a=0; break; //???? shortcut for clearing accumulator?
      case 0x01: Serial.write(a); break;
      case 0x08: Serial.print(F("in(): disk select TODO")); stop(); break;
      case 0x09: Serial.print(F("in(): disk function TODO")); stop(); break;
      case 0x0a: Serial.print(F("in(): disk write TODO")); stop(); break;
      case 0x10: Serial.print(F("in(): 2SIO port 1 control TODO")); stop(); break;
      case 0x11: Serial.print(F("in(): 2SIO port 1 write TODO")); stop(); break;
      
      // case 0x8c: Serial.write(a); break;
      // case 0x8d: Serial.write(a); break;

      default: Serial.print(F("out(): Unknown port ")); LOG("%04x:%02x",pc,port); stop();
    }
    pc+=2; 
  }
  
  void step() { //our step function does whole instruction, not just one byte
  
    //if interrupts are enabled and there's serial available then call RST
    if ((flags & IF) && Serial.available()) {
      clearFlag(IF); //disable interrupts
      opcode = 0xff;
      pc--;
      rst();
      return;
    }
   
    opcode = read8(pc);

    switch (opcode) {
      case 0x00: return nop();
      case 0x07: return rlc();
      case 0x0F: return rrc();
      case 0x17: return ral();
      case 0x1F: return rar();
      case 0x22: return shld();
      case 0x27: return daa();
      case 0x2A: return lhld();
      case 0x2F: return cma();
      case 0x32: return sta();
      case 0x37: return stc();
      case 0x3A: return lda(); 
      case 0x3F: return cmc(); 
      case 0x76: return hlt(); 
      case 0xC3: return jmp(); 
      case 0xC6: return adi(); 
      case 0xC9: return ret(); 
      case 0xCD: return call();
      case 0xCE: return aci(); 
      case 0xD3: return out(); 
      case 0xD6: return sui(); 
      case 0xDB: return in();  
      case 0xDE: return sbi();    
      case 0xE3: return xthl();   
      case 0xE6: return ani();    
      case 0xE9: return pchl();   
      case 0xEB: return xchg();   
      case 0xEE: return xri();
      case 0xF3: return di();     
      case 0xF6: return ori();
      case 0xF9: return sphl();   
      case 0xFB: return ei();     
      case 0xFE: return cpi();    
    }    

    switch (opcode & 0xF8) {
      case 0x80: return add();
      case 0x88: return adc();
      case 0x90: return sub();    
      case 0x98: return sbb();   
      case 0xA0: return ana();  
      case 0xA8: return xra();
      case 0xB0: return ora();
      case 0xB8: return cmp();
    }

    switch (opcode & 0xCF) {
      case 0x01: return lxi(); 
      case 0x02: return stax();
      case 0x03: return inx(); 
      case 0x09: return dad();    
      case 0x0A: return ldax(); 
      case 0x0B: return dcx();  
      case 0xC1: return pop();  
      case 0xC5: return push();
    }    

    switch (opcode & 0xC7) {
      case 0x04: return inr(); 
      case 0x05: return dcr(); 
      case 0x06: return mvi(); 
      case 0xC0: return rccc();   
      case 0xC2: return jccc();   
      case 0xC4: return cccc();  
      case 0xC7: return rst();    
    }

    switch (opcode & 0xC0) {
      case 0x40: return mov();
    }
    
    Serial.print(F("Unknown opcode"));
    LOG("%02x @ %04x\n",opcode,pc);
  }

} CPU;
