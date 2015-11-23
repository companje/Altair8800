#include "Arduino.h"

#define DEST(x) (x >> 3 & 7)
#define SOURCE(x) (x & 7)
#define CONDITION(x) (x >> 3 & 7)
#define VECTOR(x) (x >> 3 & 7)
#define RP(x) (x >> 4 & 3)

const byte instruction_length[] = { 1,3,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,3,1,1,1,1,2,1,1,1,1,1,1,1,2,1,1,3,3,1,1,1,2,1,1,1,3,1,1,1,2,1,1,3,3,1,1,1,2,1,1,1,3,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,3,3,1,2,1,1,1,3,3,3,3,2,1,1,1,3,2,3,1,2,1,1,1,3,2,3,3,2,1,1,1,3,1,3,1,2,1,1,1,3,1,3,3,2,1,1,1,3,1,3,1,2,1,1,1,3,1,3,3,2,1 };

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
    return !!(flags & flag); //does not return (0 or 1) but (0 or flag)
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
    aa = a;
    bb = 0x100 - val;
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
    //LOG("%04x: compare: a=val (%02x - %02x) = %02x (ZERO=%d)",pc,tmp,val,a,getFlag(ZERO));
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
    //LOG("dcr: val=%02x",val + 0xff);
  } //Decrement register
  
  void dcx() { setRegisterPair(RP(opcode), getRegisterPair(RP(opcode)-1));  pc++; } //Decrement register pair
  void di() { pc++; clearFlag(IF); } //Disable interrupts
  void ei() { pc++; setFlag(IF); } //Enable interrupts
  void hlt() { wait = true; } //Halt processor
  
  void in() { //Read input port into A
    byte port = read8(pc+1);
    switch (port) {
      case 0x00: a = Serial.available()==0; break; //negative logic
      case 0x01: a = Serial.read(); break; //Serial.write(a); 
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
  void ret() { pc = read16(sp); sp+=2; }; //Unconditional return from subroutine
  void rlc() { uint16_t hi = a & 0x80; a <<= 1; setFlag(CARRY,hi); if (hi) a |= 1; else a &= ~1; pc++; } //Rotate A left
  void rrc() { uint16_t lo = a & 1; a >>= 1; setFlag(CARRY,lo); if (lo) a |= 0x80; else a &= ~0x80; pc++; } //Rotate A right
  void rst() { sp-=2; write16(sp,pc+1); pc = DEST(opcode)*8; } //Restart (Call n*8)
  void sbb() { uint16_t val = getRegister(SOURCE(opcode)); if (getFlag(CARRY)) val++; gensub(val); pc++; } //Subtract register from A with borrow
  void sbi() { uint16_t val = read8(pc+1); if (getFlag(CARRY)) val++; gensub(val); pc+=2; } //Subtract immediate from A with borrow
  void shld() { write16(read16(pc+1), hl); pc+=3; } //Store H:L to memory
  void sphl() { sp=hl; pc++; } //Set SP to content of H:L
  void sta() { write16(read16(pc+1),a); pc+=3; } //Store A to memory
  void stax() { write8(getRegisterPair(RP(opcode)),a); pc++; } //Store indirect through BC or DE // TODO: only BC and DE allowed for indirect
  void stc() { setFlag(CARRY); pc++; } //Set  flag
  void sub() { byte val = getRegister(SOURCE(opcode)); gensub(val); pc++; } //Subtract register from A
  void sui() { gensub(read8(pc+1)); pc+=2; } //Subtract immediate from A
  void xchg() { uint16_t tmp=hl; hl=de; de=tmp; pc++; } //Exchange DE and HL content
  void xra() { a ^= getRegister(SOURCE(opcode)); clearFlag(CARRY); clearFlag(HALF); updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF); pc++; } //XOR register with A
  void xri() { a ^= read8(pc+1); clearFlag(CARRY); clearFlag(HALF); updateFlags(A, ZERO | SIGN | PARITY | CARRY | HALF); pc+=2; } //XOR immediate with A
  void xthl() { uint16_t tmp=sp; write16(sp,hl); hl=tmp; pc++; } //Swap H:L with top word on stack

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
    
    LOG("Unknown opcode: %02x @ %04x",opcode,pc);
  }

  byte rp1() {
    byte rp[] = "bc,de,hl,sp";
    return rp[RP(opcode)*3];
  }

  byte rp2() {
    byte rp[] = "bc,de,hl,sp";
    return rp[RP(opcode)*3+1];
  }

  byte c1() {
    byte c[] = "nz,z ,nc,c ,po,pe,p ,m ";
    return c[CONDITION(opcode)*3];
  }

  byte c2() {
    byte c[] = "nz,z ,nc,c ,po,pe,p ,m ";
    return c[CONDITION(opcode)*3+1];
  }

  byte reg(byte r) {
    byte s[] = "b,c,d,e,h,l,m,a";
    return s[r*2]; 
  }

  byte src() {
    return reg(SOURCE(opcode));
  }

  byte dst() {
    return reg(DEST(opcode));
  }
  
  void printInstruction() {
    byte opcode = read8(pc);
    byte len = instruction_length[opcode];
    byte db,lb,hb;
    int addr;
    
    char buf[100]; 
    sprintf(buf,"%04X %02X",pc,opcode); 
    Serial.print(buf);
    
    if (len==1) {
      Serial.print("\t");
    } else if (len==2) {
      db = read8(pc+1);
      sprintf(buf,"%02X",db); 
      Serial.print(buf);
    } else if (len==3) {
      lb = read8(pc+1);
      hb = read8(pc+2);
      addr = hb<<8 | lb;
      sprintf(buf,"%02X%02X",lb,hb); 
      Serial.print(buf);
    }
    
    Serial.print("\t");

    switch (opcode) {
      case 0x00: LOG("nop",0);           return;
      case 0x07: LOG("rlc",0);           return;
      case 0x0F: LOG("rrc",0);           return;
      case 0x17: LOG("ral",0);           return;
      case 0x1F: LOG("rar",0);           return;
      case 0x22: LOG("shld %04Xh",addr); return;
      case 0x27: LOG("daa",0);           return;
      case 0x2A: LOG("lhld %04Xh",addr); return;
      case 0x2F: LOG("cma",0);           return;
      case 0x32: LOG("sta %04Xh",addr);  return;
      case 0x37: LOG("stc",0);           return;
      case 0x3A: LOG("lda %04Xh",addr);  return;
      case 0x3F: LOG("cmc",0);           return;
      case 0x76: LOG("hlt",0);           return;
      case 0xC3: LOG("jmp %04Xh",addr);  return;
      case 0xC6: LOG("adi %02Xh",db);    return;
      case 0xC9: LOG("ret",0);           return;
      case 0xCD: LOG("call %04Xh",a);    return;
      case 0xCE: LOG("aci %02Xh",db);    return;
      case 0xD3: LOG("out %02Xh",db);    return;
      case 0xD6: LOG("sui %02Xh",db);    return;
      case 0xDB: LOG("in %02Xh",db);     return;
      case 0xDE: LOG("sbi %02Xh",db);    return;
      case 0xE3: LOG("xthl",0);          return;
      case 0xE6: LOG("ani %02Xh",db);    return;
      case 0xE9: LOG("pchl",0);          return;
      case 0xEB: LOG("xchg",0);          return;
      case 0xEE: LOG("xri %02Xh",db);    return;
      case 0xF3: LOG("di",0);            return;
      case 0xF6: LOG("ori %02Xh",db);    return;
      case 0xF9: LOG("sphl",0);          return;
      case 0xFB: LOG("ei",0);            return;
      case 0xFE: LOG("cpi %02Xh",db);    return;
    }
    
    switch (opcode & 0xF8) {
      case 0x80: LOG("add %c",src());    return;
      case 0x88: LOG("adc %c",src());    return;
      case 0x90: LOG("sub %c",src());    return;
      case 0x98: LOG("sbb %c",src());    return;
      case 0xA0: LOG("ana %c",src());    return;
      case 0xA8: LOG("xra %c",src());    return;
      case 0xB0: LOG("ora %c",src());    return;
      case 0xB8: LOG("cmp %c",src());    return;
    }
    
    switch (opcode & 0xCF) {
      case 0x01: LOG("lxi %c%c,%04Xh",rp1(), rp2(), addr); return;
      case 0x02: LOG("stax %c%c",rp1(),rp2()); return;
      case 0x03: LOG("inx %c%c",rp1(),rp2());  return;
      case 0x09: LOG("dad %c%c",rp1(),rp2());  return;
      case 0x0A: LOG("ldax %c%c",rp1(),rp2()); return;
      case 0x0B: LOG("dcx %c%c",rp1(),rp2());  return;
      case 0xC1: LOG("pop %c%c",rp1(),rp2());  return;
      case 0xC5: LOG("push %c%c",rp1(),rp2()); return;
    }
    
    switch (opcode & 0xC7) {
      case 0x04: LOG("inr %c",dst());          return;
      case 0x05: LOG("dcr %c",dst());          return;
      case 0x06: LOG("mvi %c,%02Xh",dst(),db); return;
      case 0xC0: LOG("r%c%c",c1(),c2());       return;
      case 0xC2: LOG("j%c%c %04Xh",c1(),c2(),addr); return;
      case 0xC4: LOG("c%c%c %04Xh",c1(),c2(),addr); return;
      case 0xC7: LOG("rst %d",VECTOR(opcode)); return;
    }
    
    switch (opcode & 0xC0) {
      case 0x40: LOG("mov %c,%c",dst(),src()); return;
    }
    
  }

  void printRegisters() {
    LOG("registers a=%02X, b=%02X, c=%02X, d=%02X, e=%02X, h=%02X, l=%02X, pc=%04X, sp=%04X",a,b,c,d,e,h,l,pc,sp);
    LOG("flags: c=%d, z=%d, h=%d",getFlag(CARRY),getFlag(ZERO),getFlag(HALF));
  }

  
} CPU;
