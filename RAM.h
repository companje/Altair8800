class RAM {
public:

  RAM() {
    pinMode(RAM_SELECT, OUTPUT);
  }

  byte readByte(int address) {
    digitalWrite(RAM_SELECT, LOW);
    SPI.transfer(3); //read
    SPI.transfer((address >> 16) & 255);
    SPI.transfer((address >> 8) & 255);
    SPI.transfer((address) & 255);
    byte value = SPI.transfer(0x00);
    digitalWrite(RAM_SELECT, HIGH);
    bus.data = value;
    bus.address = address;
    bitSet(bus.state, MEMR);
    return value;
  }
  
  void writeByte(int address, byte value) {
    digitalWrite(RAM_SELECT, LOW);
    SPI.transfer(2); //write
    SPI.transfer((address >> 16) & 255);
    SPI.transfer((address >> 8) & 255);
    SPI.transfer((address) & 255);
    SPI.transfer(value);
    digitalWrite(RAM_SELECT, HIGH);
    bus.data = value;
    bus.address = address;
    //panel.write();
  }
  
  int readWord(int address) {
    return readByte(address) | (readByte(address + 1) << 8);
  }
  
  void writeWord(uint16_t address, int value) {
    writeByte(address, value & 0xff);
    writeByte(address + 1, (value >> 8) & 0xff);
  }
  
} RAM;

