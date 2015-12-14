class Debugger {
public:

  bool enabled;
  // char buf[20];
  char cmd;
  uint16_t hexdump_addr;
  uint16_t unassemble_addr;
  byte opcode;
  uint16_t breakpoint;

  Debugger() {
    breakpoint = -1;
  }

  byte rp1() {
    byte rp[] = "bcdehlsp";
    return rp[RP(opcode)*2];
  }

  byte rp2() {
    byte rp[] = "bcdehlsp";
    return rp[RP(opcode)*2+1];
  }

  byte c1() {
    byte c[] = "nzz ncc popep m ";
    return c[CONDITION(opcode)*2];
  }

  byte c2() {
    byte c[] = "nzz ncc popep m ";
    return c[CONDITION(opcode)*2+1];
  }

  byte reg(byte r) {
    byte s[] = "bcdehlma";
    return s[r*1];
  }

  byte src() {
    return reg(SOURCE(opcode));
  }

  byte dst() {
    return reg(DEST(opcode));
  }

  int getInstructionLength(byte opcode) {
    switch (opcode) {
      case 0x01: case 0x11: case 0x21: case 0x22: case 0x2A: case 0x31: case 0x32: case 0x3A: 
      case 0xC2: case 0xC3: case 0xC4: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xD2: 
      case 0xD4: case 0xDA: case 0xDC: case 0xDD: case 0xE2: case 0xE4: case 0xEA: case 0xEC: 
      case 0xED: case 0xF2: case 0xF4: case 0xFA: case 0xFC: case 0xFD: return 3;
      case 0x06: case 0x0E: case 0x16: case 0x1E: case 0x26: case 0x2E: case 0x36: case 0x3E: 
      case 0xC6: case 0xCE: case 0xD3: case 0xD6: case 0xDB: case 0xDE: case 0xE6: case 0xEE: 
      case 0xF6: case 0xFE: return 2;
      default: return 1;
    }
  }
  
  void unassemble() {
    // Serial.println(F("unassemble currently disabled"));
    opcode = RAM.read(unassemble_addr);
    byte len = getInstructionLength(opcode);
    byte db,lb,hb;
    uint16_t addr;
    
    // char buf[20]; 
    sprintf(printbuf,"%04X %02X",unassemble_addr,opcode); 
    Serial.print(printbuf);
    // softserial.print(printbuf);
    
    if (len==1) {
      Serial.print("\t");
      // softserial.print("     ");
    } else if (len==2) {
      db = RAM.read(unassemble_addr+1);
      sprintf(printbuf,"%02X",db); 
      Serial.print(printbuf);
      // softserial.print(printbuf);
      // softserial.print("   ");
    } else if (len==3) {
      lb = RAM.read(unassemble_addr+1);
      hb = RAM.read(unassemble_addr+2);
      addr = hb<<8 | lb;
      sprintf(printbuf,"%02X%02X",lb,hb); 
      Serial.print(printbuf);
      // softserial.print(printbuf);
      // softserial.print(" ");
    }

    unassemble_addr+=len;
    
    Serial.print("\t");

    switch (opcode) {
      case 0x00: Serial.print(F("nop\n" )); return;
      case 0x07: Serial.print(F("rlc\n" )); return;
      case 0x0F: Serial.print(F("rrc\n" )); return;
      case 0x17: Serial.print(F("ral\n" )); return;
      case 0x1F: Serial.print(F("rar\n" )); return;
      case 0x22: Serial.print(F("shld ")); LOG("%04X",addr); return;
      case 0x27: Serial.print(F("daa\n" )); return;
      case 0x2A: Serial.print(F("lhld ")); LOG("[%04X]",addr); return;
      case 0x2F: Serial.print(F("cma\n" )); return;
      case 0x32: Serial.print(F("sta " )); LOG("%04X",addr);  return;
      case 0x37: Serial.print(F("stc\n" )); return;
      case 0x3A: Serial.print(F("lda " )); LOG("%04X",addr);  return;
      case 0x3F: Serial.print(F("cmc\n" )); return;
      case 0x76: Serial.print(F("hlt\n" )); return;
      case 0xC3: Serial.print(F("jmp " )); LOG("%04X",addr);  return;
      case 0xC6: Serial.print(F("adi " )); LOG("%02X",db);    return;
      case 0xC9: Serial.print(F("ret\n" )); return;
      case 0xCD: Serial.print(F("call ")); LOG("%04X",addr); return;
      case 0xCE: Serial.print(F("aci ")); LOG("%02X",db);    return;
      case 0xD3: Serial.print(F("out ")); LOG("%02X",db);    return;
      case 0xD6: Serial.print(F("sui ")); LOG("%02X",db);    return;
      case 0xDB: Serial.print(F("in " )); LOG("%02X",db);     return;
      case 0xDE: Serial.print(F("sbi ")); LOG("%02X",db);    return;
      case 0xE3: Serial.print(F("xthl\n")); return;
      case 0xE6: Serial.print(F("ani " )); LOG("%02X",db);    return;
      case 0xE9: Serial.print(F("pchl\n")); return;
      case 0xEB: Serial.print(F("xchg\n")); return;
      case 0xEE: Serial.print(F("xri " )); LOG("%02X",db);    return;
      case 0xF3: Serial.print(F("di\n" )); return;
      case 0xF6: Serial.print(F("ori " )); LOG("%02X",db);    return;
      case 0xF9: Serial.print(F("sphl\n")); return;
      case 0xFB: Serial.print(F("ei\n"  )); return;
      case 0xFE: Serial.print(F("cpi " )); LOG("%02X",db);    return;
    }
    
    switch (opcode & 0xF8) {
      case 0x80: Serial.print(F("add ")); LOG("%c",src()); return;
      case 0x88: Serial.print(F("adc ")); LOG("%c",src()); return;
      case 0x90: Serial.print(F("sub ")); LOG("%c",src()); return;
      case 0x98: Serial.print(F("sbb ")); LOG("%c",src()); return;
      case 0xA0: Serial.print(F("ana ")); LOG("%c",src()); return;
      case 0xA8: Serial.print(F("xra ")); LOG("%c",src()); return;
      case 0xB0: Serial.print(F("ora ")); LOG("%c",src()); return;
      case 0xB8: Serial.print(F("cmp ")); LOG("%c",src()); return;
    }
    
    switch (opcode & 0xCF) {
      case 0x01: Serial.print(F("lxi ")); LOG("%c%c,%04X",rp1(), rp2(), addr); return;
      case 0x02: Serial.print(F("stax ")); LOG("%c%c",rp1(),rp2()); return;
      case 0x03: Serial.print(F("inx ")); LOG("%c%c",rp1(),rp2());  return;
      case 0x09: Serial.print(F("dad ")); LOG("%c%c",rp1(),rp2());  return;
      case 0x0A: Serial.print(F("ldax ")); LOG("%c%c",rp1(),rp2()); return;
      case 0x0B: Serial.print(F("dcx ")); LOG("%c%c",rp1(),rp2());  return;
      case 0xC1: Serial.print(F("pop ")); LOG("%c%c",rp1(),rp2());  return;
      case 0xC5: Serial.print(F("push ")); LOG("%c%c",rp1(),rp2()); return;
    }
    
    switch (opcode & 0xC7) {
      case 0x04: Serial.print(F("inr ")); LOG("%c",dst());          return;
      case 0x05: Serial.print(F("dcr ")); LOG("%c",dst());          return;
      case 0x06: Serial.print(F("mvi ")); LOG("%c,%02X",dst(),db); return;
      case 0xC0: Serial.print(F("r"  )); LOG("%c%c",c1(),c2());       return;
      case 0xC2: Serial.print(F("j"  )); LOG("%c%c %04X",c1(),c2(),addr); return;
      case 0xC4: Serial.print(F("c"  )); LOG("%c%c %04X",c1(),c2(),addr); return;
      case 0xC7: Serial.print(F("rst ")); LOG("%d",VECTOR(opcode)); return;
    }
    
    switch (opcode & 0xC0) {
      case 0x40: Serial.print(F("mov ")); LOG("%c,%c",dst(),src()); return;
    }
    
  }

  void registersAndFlags() {
    LOG("R: a=%02X b=%02X c=%02X d=%02X e=%02X h=%02X l=%02X pc=%04X sp=%04X",CPU.a,CPU.b,CPU.c,CPU.d,CPU.e,CPU.h,CPU.l,CPU.pc,CPU.sp);
    LOG("F: c=%d z=%d h=%d",CPU.getFlag(CPU.CARRY),CPU.getFlag(CPU.ZERO),CPU.getFlag(CPU.HALF));
  }

  void start() {
    CPU.wait = true;
    enabled = true;
    hexdump_addr = 0;
    unassemble_addr = 0;
    Serial.println(F("Debug start"));
    softserial.println(F("debug start"));
  }

  void stop() {
    enabled = false;
    Serial.println(F("debug stop"));
    softserial.println(F("debug stop"));
  }

  void toggle() {
    if (enabled) stop();
    else start();
  }

  bool is_digit(char c) {
    return c >= '0' && c <= '9';
  }

  bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
  }

  bool is_hex(char c) {
    return is_digit(c) || (c >= 'a' && c <= 'f');
  }

  bool readLine(char *buf) {
    static int index = 0;
    
    //HARDWARE SERIAL
    while (Serial.available()) {
      byte ch = Serial.read();
      buf[index] = ch;
      if (ch=='\n') {
        buf[index] = 0;
        index = 0;
        return true;
      } else {
        index++;
      }
    }

    //SOFTWARE SERIAL
    while (softserial.available()) {
      byte ch = softserial.read();
      buf[index] = ch;
      if (ch=='\r' || ch=='\n') {    // carriage return
        buf[index] = 0;
        index = 0;
        return true;
      } else {
        index++;
      }
    }

    return false;
  }

  void update() {
    static char keybuf[20];

    if (readLine(keybuf) && strlen(keybuf)>0) {
      char cmd = keybuf[0];
      
      if (cmd=='d') {
        if (strlen(keybuf)>1) {
          uint16_t val;
          sscanf(&keybuf[1],"%x",&val); 
          hexdump_addr = val;
        }
        hexdump();
      } else if (cmd=='r') {
        registersAndFlags();
      } else if (cmd=='u') {
        if (strlen(keybuf)>1) {
          uint16_t val;
          sscanf(&keybuf[1],"%x",&val); 
          unassemble_addr = val;
        }
        for (int i=0; i<16; i++) {
          unassemble();
        }
      } else if (cmd=='g') { //go
        if (strlen(keybuf)>1) {
          uint16_t val;
          sscanf(&keybuf[1],"%x",&val); 
          CPU.pc = val;
        }
        stop(); //stop debugger
        CPU.wait = false; //run
      } else if (cmd=='b') { //set breakpoint
        if (strlen(keybuf)>1) {
          uint16_t val;
          sscanf(&keybuf[1],"%x",&val); 
          breakpoint = val;
          Serial.print(F("set breakpoint at "));
          LOG("%04x",val);
        }
      }
      Serial.println();
      softserial.println();
    }  
  }

  void hexdump() {
    // char buf[10];
    char line[17];
    line[16] = '\0';
    for (uint16_t i=hexdump_addr; i<hexdump_addr+8*16; i+=16) {
      //prefix
      sprintf(printbuf,"%04X: ",i); 
      Serial.print(printbuf);
      softserial.print(printbuf);
      //read + print hex
      for (int j=0; j<16; j++) {
        byte b = RAM.read(i+j);
        sprintf(printbuf,"%02X ",b);
        Serial.print(printbuf);
        softserial.print(printbuf);
        line[j] = (b>=32 && b<127) ? b : '.';
      }
      Serial.println(line);
      softserial.println(line);
    }
    hexdump_addr+=8*16;
  }

} debug;
