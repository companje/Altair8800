extern "C" {
  
  //read/write byte
  int i8080_hal_memory_read_byte(int addr) {
    return RAM.readByte(addr);
  }
  
  void i8080_hal_memory_write_byte(int addr, int value) {
    RAM.writeByte(addr,value);
  }
  
  //read/write word
  int i8080_hal_memory_read_word(int addr) {
    return RAM.readWord(addr);
  }
  
  void i8080_hal_memory_write_word(int addr, int value) {
    RAM.writeWord(addr,value);
  }
  
  //input/output
  int i8080_hal_io_input(int port) {
    return IO.read(port);
  }
  
  void i8080_hal_io_output(int port, int value) {
    IO.write(port,value);
  }
  
  //interrupts
  void i8080_hal_iff(int on) {
    //nothing
  }
}
