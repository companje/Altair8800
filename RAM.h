#ifndef RAM_H
#define RAM_H

namespace RAM {  
  
  void init();
  uint8_t readByte(uint16_t address);
  void writeByte(uint16_t address, uint8_t value);
  uint16_t readWord(uint16_t address);
  void writeWord(uint16_t address, uint16_t value);
}

#endif
