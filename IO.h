class IO {
public:

  int read(int port) {
    bitSet(bus.state,INP);
    switch (port) {
      case 0x00: return 0; //clear A register?
      case 0x01: // serial port read
      case 0x11: 
      case 0x22:
      case 0x23:
        if (Serial.available()) return Serial.read(); //read from HardwareSerial (USB Serial)
//        else if (softserial.available()) return softserial.read(); //read from SoftwareSerial (DB25 connector)
        else return 0; // 2SIO port 1, read
      case 0x08: return disk.status();
      case 0x09: return disk.sector();
      case 0x0a: return disk.read();
      case 0x10: return 0; // 2SIO port 1, status
//      case 0xff: return panel.sense();
      default:
        Serial.print(i8080_pc() & 0xffff,HEX);
        Serial.print(": inp 0x");
        Serial.println(port,HEX);
        return 0;
    }
  }
  
  void write(int port, byte value) {
    bitSet(bus.state,OUT);
    switch (port) {
      case 0x01: // serial port write
      case 0x11: 
      case 0x22:
      case 0x23:
        Serial.print((char)(value & 0x7f));
//        softserial.print((char)(value & 0x7f));
        break; // 2sio port 1 write
      case 0x08: disk.select(value); break;
      case 0x09: disk.function(value); break;
      case 0x0a: disk.write(value); break;
      case 0x10: /*nothing?*/ break; // 2SIO port 1 control
      default:
//        Serial.print(i8080_pc() & 0xffff,HEX);
        Serial.print("out ");
        Serial.print(port,HEX);
//        Serial.print(",0x");
//        Serial.println(value);
        break;
    }
  }
        
} IO;

