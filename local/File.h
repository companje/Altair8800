class File {
public:

  boolean seek(uint32_t pos) {
    return true;
  }

  void write(byte *a, int b) {

  }

  byte read() {
    return 0;
  }

  operator bool() {
    return true;
  // if (_file) 
  //   return  _file->isOpen();
  // return false;
  }

  bool available() {
    return true;
  }

  void close() {
    
  }

};