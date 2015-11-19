#include "Arduino.h"

#define DEST(x) (x >> 3 & 7)
#define SOURCE(x) (x & 7)
#define CONDITION(x) (x >> 3 & 7)
#define VECTOR(x) (x >> 3 & 7)
#define RP(x) (x >> 4 & 3)

class CPU {
public:
  boolean m1 = true; //Machine Cycle 1, fetching an opcode
  boolean memr = false; //Memory Read
  boolean stack = false; //on when using the stack (push, pop, ...)
  boolean wait = true;
  byte opcode = 0; // the last fetched opcode
  
  enum Flag { CARRY=1, PARITY=4, HALF=16, IF=32, ZERO=64, SIGN=128 };
  enum Condition { NZ,Z,NC,CC,PO,PE,P,M }; //NZ=!zero, Z=zero, NC=!carry, C=Carry, PO=parity odd, PE=parity even, P=!sign = positive, M=sign = minus/negative 
  enum Register {B,C,D,E,H,L,MEM,A };
  enum RegisterPair { BC,DE,HL,STP };

  union { uint16_t af; struct { byte flags, a; }; };  //a=accumulator,  //f=flags (S Z 0 A 0 P 1 C), S=sign, Z=zero, A=aux-carry, P=parity, C=carry 
  union { uint16_t bc; struct { byte b, c; }; }; 
  union { uint16_t de; struct { byte e, d; }; }; 
  union { uint16_t hl; struct { byte l, h; }; };
  
  uint16_t sp; //stack pointer
  uint16_t pc; //program counter
  
  byte read8(uint16_t addr) {
    byte val = RAM.read(addr);
//    LOG("%04x: %02x",addr,val);
    memr = true;
    return val;
  }
  
  uint16_t read16(uint16_t addr) {
    return read8(addr) | (read8(addr+1)<<8);
  }
  
  void write8(uint16_t addr, byte value) {
    RAM.write(addr, value);
  }
  
  void write16(uint16_t addr, uint16_t val) {
    write8(addr, val & 0xff);
    write8(addr+1, (val >> 8) & 0xff);
  }
  
  void setFlag(byte flag, byte value = true) {
//    LOG("setFlag: %02x = %02x",flag,value);
    if (value) flags |= flag;
    else clearFlag(flag);
  }
  
  byte getFlag(byte flag) {
    return flags & flag;
  }
  
  void clearFlag(byte flag) {
    flags &= ~flag;
  }
  
  boolean getParity(byte val) {
    val ^= val >> 4;
    val &= 0xf;
    return !((0x6996 >> val) & 1);
  }

  byte getRegister(byte reg) {
     switch (reg) {
       case B: return b;
       case C: return c;
       case D: return d;
       case E: return e;
       case H: return h;
       case L: return l;
       case MEM: return RAM.read(hl);
       case A: return a;
       default: LOG("getRegister: Unknown register %d",reg);
     }
  }
  
  void setRegister(byte reg, byte value) {
     switch (reg) {
       case B: b = value; break;
       case C: c = value; break;
       case D: d = value; break;
       case E: e = value; break;
       case H: h = value; break;
       case L: l = value; break;
       case MEM: RAM.write(hl,value); break;
       case A: a = value; break;
       default: LOG("setRegister: Unknown register %d",reg);
     }
  }
  
  uint16_t getRegisterPair(byte pair) {
    switch (pair) {
      case BC: return bc;
      case DE: return de;
      case HL: return hl;
      case STP: return sp;
      default: LOG("getRegisterPair: Unknown register pair %d",pair);
    }
  }
  
  void setRegisterPair(byte pair, uint16_t value) {
    switch (pair) {
      case BC: bc = value; break;
      case DE: de = value; break;
      case HL: hl = value; break;
      case STP: sp = value; break;
      default: LOG("setRegisterPair: Unknown register pair %d",pair);
    }
  }
  
  void gensub(byte val) { //generic subtract
    uint16_t aa,bb;
    a = a;
    b = 0x100 - val;
    setFlag(HALF, checkHalfCarry(a,b));
    setFlag(CARRY, checkCarry(a,b));
    a += b;
    a = a & 0xff;
    updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF);
  }
  
  void genadd(byte val) { //generic add
    setFlag(HALF,checkHalfCarry(a,val));
    setFlag(CARRY, (a,val));
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
    if (mask & ZERO) setFlag(ZERO,val);
    if (mask & SIGN) setFlag(SIGN,val & 0x80);
  }
  
  boolean checkHalfCarry(uint16_t a, uint16_t b) {
    a &= 0xf;
    b &= 0xf;
    return (a+b) > 0xf;
  }
  
  boolean checkCarry(uint16_t a, uint16_t b) {
    return a+b > 0xff; 
  }
  
  boolean checkCondition(byte condition) {
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
  
  void aci() { genadd(read8(pc+1) + getFlag(CARRY)); pc+=2; } //Add immediate to A with carry
  void adc() { genadd(getRegister(SOURCE(opcode)) + getFlag(CARRY)); pc++; } //Add register to A with carry
  void add() { genadd(getRegister(SOURCE(opcode))); pc++; } //Add register to A
  void adi() { genadd(read8(pc+1)); pc+=2; } //Add immediate to A
  void ana() { a &= getRegister(SOURCE(opcode)); clearFlag(CARRY); updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF); pc++; } //AND register with A
  void ani() { a &= getRegister(read8(pc+1)); clearFlag(CARRY); clearFlag(HALF); updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF); pc+=2; } //AND immediate with A
  void call() { sp-=2; write16(sp, pc+3); pc=read16(pc+1); } //Unconditional subroutine call
  void cccc() { if (checkCondition(CONDITION(opcode))) call(); else pc+=3; } //Conditional subroutine call
  void cma() { a=~a; pc++; } //Compliment A
  void cmc() { flags ^= 1; pc++; } //Compliment Carry flag
  void cmp() { compare(getRegister(SOURCE(opcode))); pc++; } //Compare register with A
  void cpi() { compare(read8(pc+1)); pc+=2; } //Compare immediate with A
 
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
//    uint32_t _hl = getRegisterPair(HL);
    val += hl;
    setFlag(CARRY, val > 0xffff);
//Serial.println(val);
    hl = val & 0xffff;
    pc++;
  } 
 
  void dcr() { 
    byte val = getRegister(DEST(opcode));
    setFlag(HALF, checkHalfCarry(val, 0xff));
    setRegister(DEST(opcode), val + 0xff);
    updateFlags(DEST(opcode), ZERO | SIGN | CARRY | HALF); //no parity check here?
    pc++;
  } //Decrement register
  
  void dcx() { setRegisterPair(RP(opcode), getRegisterPair(RP(opcode)-1));  pc++; } //Decrement register pair
  void di() { pc++; clearFlag(IF); } //Disable interrupts
  void ei() { pc++; setFlag(IF); } //Enable interrupts
  void hlt() { wait = true; } //Halt processor
  
  void inp() { //Read input port into A
    byte port = read8(pc+1);
    switch (port) {
      case 0x00: a = Serial.available()==0; break; //negative logic
      case 0x01: a = Serial.read(); break;
      case 0xff: a = Switches.ax >> 8; break;
      default: a = 0xff;
    }
    pc+=2;
  }
  
  void inr() { 
    byte val = getRegister(DEST(opcode)); 
    setFlag(HALF,checkHalfCarry(val,1));
    setRegister(DEST(opcode),val+1);
    updateFlags(DEST(opcode), ZERO | SIGN | PARITY | CARRY | HALF);
    pc++;
  } //Increment register
  
  void inx() { setRegisterPair(RP(opcode), getRegisterPair(RP(opcode)+1)); pc++; } //Increment register pair
  void jccc() { if (checkCondition(CONDITION(opcode))) jmp(); else pc+=3; } //Conditional jump
  void jmp() { pc = read16(pc+1); } //Unconditional jump
  void lda() { a = read16(pc+1); pc+=3; } //Load A from memory
  void ldax() { a = read8(getRegisterPair(RP(opcode))); pc++; } //Load indirect through BC or DE (TODO: only BC and DE allowed for indirect)
  void lhld() { hl = read16(read16(pc+1)); } //Load H:L from memory
  void lxi() { setRegisterPair(RP(opcode), read16(pc+1)); pc+=3; } //Load register pair immediate
  void mov() { setRegister(DEST(opcode), getRegister(SOURCE(opcode))); pc++; } //Move register to register
  void mvi() { setRegister(DEST(opcode), read8(pc+1)); pc+=2; } //Move immediate to register
  void nop() { pc++; } //No operation
  void ora() { a |= getRegister(SOURCE(opcode)); clearFlag(CARRY); clearFlag(HALF); updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF); pc++; } //OR register with A
  void ori() { a |= read8(pc+1); clearFlag(CARRY); clearFlag(HALF); updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF); pc+=2; } //OR immediate with A

  void out() { //Write A to output port
    byte port = read8(pc+1);
    switch (port) {
      case 0x00: break; //interrupts? //the echo example says: 'mvi a,1' 'out 0' before 'ei'...
      case 0x01: Serial.write(a); break;
    }
    pc+=2; 
  }
  
  void pchl() { pc = hl; } //Jump to address in H:L
  void pop() { /*FIXME: see note about SP*/ setRegisterPair(RP(opcode), read16(sp)); sp+=2; pc++; } //Pop register pair from the stack
  void push() { /*FIXME: see note about SP*/ sp-=2; write16(sp,getRegisterPair(RP(opcode))); pc++; } //Push register pair on the stack
  void ral() { uint16_t hi = a & 0x80; a <<= 1; if (getFlag(CARRY)) a |= 1; else a &= ~1; setFlag(CARRY,hi); pc++; } //Rotate A left through carry
  void rar() { uint16_t lo = a & 1; a >>= 1; if (getFlag(CARRY)) a |= 0x80; else a &= ~0x80; setFlag(CARRY,lo); pc++; } //Rotate A right through carry
  void rccc() { if (checkCondition(CONDITION(opcode))) ret(); else pc++; } //Conditional return from subroutine
  void ret() { pc = read16(sp); sp+=2; Serial.println(pc); }; //Unconditional return from subroutine
  void rlc() { uint16_t hi = a & 0x80; a <<= 1; setFlag(CARRY,hi); if (hi) a |= 1; else a &= ~1; pc++; } //Rotate A left
  void rrc() { uint16_t lo = a & 1; a >>= 1; setFlag(CARRY,lo); if (lo) a |= 0x80; else a &= ~0x80; pc++; } //Rotate A right
  //LOG("rrc:%02x",a);
  void rst() { sp-=2; write16(sp,pc); pc = DEST(opcode)*8; } //Restart (Call n*8) ////// CHECKME (this was ...,pc+1);
  void sbb() { Serial.println("TODO SBB"); } //Subtract register from A with borrow
  void sbi() { Serial.println("TODO SBI");} //Subtract immediate from A with borrow
  void shld() { write16(read16(pc+1), hl); pc+=3; } //Store H:L to memory
  void sphl() { sp=hl; pc++; } //Set SP to content of H:L
  void sta() { write16(read16(pc+1),a); pc+=3; } //Store A to memory
  void stax() { write8(getRegisterPair(RP(opcode)),a); pc++; } //Store indirect through BC or DE // TODO: only BC and DE allowed for indirect
  void stc() { setFlag(CARRY); pc++; } //Set  flag
  void sub() { Serial.println("TODO SUB"); } //Subtract register from A
  void sui() { Serial.println("TODO SUI"); } //Subtract immediate from A
  void xchg() { uint16_t tmp=hl; hl=de; de=tmp; pc++; } //Exchange DE and HL content
  void xra() { a ^= getRegister(SOURCE(opcode)); clearFlag(CARRY); clearFlag(HALF); updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF); pc++; } //XOR register with A
  void xri() { a ^= read8(pc+1); clearFlag(CARRY); clearFlag(HALF); updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF); pc+=2; } //XOR immediate with A
  void xthl() { uint16_t tmp=sp; write16(sp,hl); hl=tmp; pc++; } //Swap H:L with top word on stack

  void step() { //our step function does whole instruction, not just one byte
  
    if ((flags & IF) && Serial.available()) {
      clearFlag(IF); //disable interrupts
      opcode = 0xff;
      rst();
      return;
    } else {
      opcode = read8(pc);
    }
    
//    LOG("%04x: %02x",pc,opcode);

    switch (opcode) {
      case 0b11111110: cpi(); return; // cpi #              ZSPCA   Compare immediate with A
      case 0b11111011: ei(); return; // ei                 -       Enable interrupts
      case 0b11111001: sphl(); return; // sphl               -       Set SP to content of H:L
      case 0b11110110: ori(); return; // ori #              ZSPCA   OR  immediate with A
      case 0b11110011: di(); return; // di                 -       Disable interrupts
      case 0b11101110: xri(); return; // xri #     db       ZSPCA   ExclusiveOR immediate with A
      case 0b11101011: xchg(); return; // xchg               -       Exchange DE and HL content
      case 0b11101001: pchl(); return; // pchl               -       Jump to address in H:L
      case 0b11100110: ani(); return; // ani #     db       ZSPCA   AND immediate with A
      case 0b11100011: xthl(); return; // xthl               -       Swap H:L with top word on stack
      case 0b11011110: sbi(); return; // sbi #     db       ZSCPA   Subtract immediate from A with borrow
      case 0b11011011: inp(); return; // in p      pa       -       Read input port into A
      case 0b11010110: sui(); return; // sui #     db       ZSCPA   Subtract immediate from A
      case 0b11010011: out(); return; // out p     pa       -       Write A to output port
      case 0b11001110: aci(); return; // aci #     db       ZSCPA   Add immediate to A with carry
      case 0b11001101: call(); return; // call a    lb hb    -       Unconditional subroutine call
      case 0b11001001: ret(); return; // ret                -       Unconditional return from subroutine
      case 0b11000110: adi(); return; // adi #     db       ZSCPA   Add immediate to A
      case 0b11000011: jmp(); return; // jmp a     lb hb    -       Unconditional jump
      case 0b01110110: hlt(); return; // hlt                -       Halt processor
      case 0b00111111: cmc(); return; // cmc                C       Compliment Carry flag
      case 0b00111010: lda(); return; // lda a     lb hb    -       Load A from memory
      case 0b00110111: stc(); return; // stc                C       Set Carry flag
      case 0b00110010: sta(); return; // sta a     lb hb    -       Store A to memory
      case 0b00101111: cma(); return; // cma                -       Compliment A
      case 0b00101010: lhld(); return; // lhld a    lb hb    -       Load H:L from memory
      case 0b00100111: daa(); return; // daa                ZSPCA   Decimal Adjust accumulator
      case 0b00100010: shld(); return; // shld a    lb hb    -       Store H:L to memory
      case 0b00011111: rar(); return; // rar                C       Rotate A right through carry
      case 0b00010111: ral(); return; // ral                C       Rotate A left through carry
      case 0b00001111: rrc(); return; // rrc                C       Rotate A right
      case 0b00000111: rlc(); return; // rlc                C       Rotate A left
      case 0b00000000: nop(); return; // nop                -       No operation
    }
    
    switch (opcode & 0b11111000) {
      case 0b10111000: cmp(); return; // cmp s              ZSPCA   Compare register with A
      case 0b10110000: ora(); return; // ora s              ZSPCA   OR  register with A
      case 0b10101000: xra(); return; // xra s              ZSPCA   ExclusiveOR register with A
      case 0b10100000: ana(); return; // ana s              ZSCPA   AND register with A
      case 0b10011000: sbb(); return; // sbb s              ZSCPA   Subtract register from A with borrow
      case 0b10010000: sub(); return; // sub s              ZSCPA   Subtract register from A
      case 0b10001000: adc(); return; // adc s              ZSCPA   Add register to A with carry
      case 0b10000000: add(); return; // add s              ZSPCA   Add register to A
    }
    
    switch (opcode & 0b11001111) {
      case 0b11000101: push(); return; // push rp   *2       -       Push register pair on the stack
      case 0b11000001: pop(); return; // pop rp    *2       *2      Pop  register pair from the stack
      case 0b00001011: dcx(); return; // dcx rp             -       Decrement register pair
      case 0b00001010: ldax(); return; // ldax rp   *1       -       Load indirect through BC or DE
      case 0b00001001: dad(); return; // dad rp             C       Add register pair to HL (16 bit add)
      case 0b00000011: inx(); return; // inx rp             -       Increment register pair
      case 0b00000010: stax(); return; // stax rp   *1       -       Store indirect through BC or DE
      case 0b00000001: lxi(); return; // lxi rp,#  lb hb    -       Load register pair immediate
    }
    
    switch (opcode & 0b11000111) {
      case 0b11000111: rst(); return; // rst n              -       Restart (Call n*8)
      case 0b11000100: cccc(); return; // cccc a    lb hb    -       Conditional subroutine call
      case 0b11000010: jccc(); return; // jccc a    lb hb    -       Conditional jump
      case 0b11000000: rccc(); return; // rccc               -       Conditional return from subroutine
      case 0b00000110: mvi(); return; // mvi d,#   db       -       Move immediate to register
      case 0b00000101: dcr(); return; // dcr d              ZSPA    Decrement register
      case 0b00000100: inr(); return; // inr d              ZSPA    Increment register
    }
    
    switch (opcode & 0b11000000) {
      case 0b01000000: mov(); return; // mov d,s            -       Move register to register
    }
    
    LOG("Unknown opcode: %02x @ %04x",opcode,pc);
  }
  
} CPU;
