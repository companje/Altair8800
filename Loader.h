class Loader {
public:

  Loader() {
    if (!SD.begin(SD_SELECT)) {
      Serial.println("ERR:SD");
    }
  }

  int loadFile(const char filename[], int offset = 0) {
    File file = SD.open(filename);
    if (!file) {
      Serial.println("ERR:Load");
      return -2; 
    }
    while (file.available()) {
      RAM::writeByte(offset++, file.read());
//      panel.writeLEDs();
    }
    file.close();
    return 0;
  }
    
  void loadData(byte *program, int s, int offset = 0) {
    for (uint16_t i=0; i<s; i++) {
      RAM::writeByte(i+offset,program[i]);
      //panel.writeLEDs();
    }
  }
      
  void loadKillbits() {
    byte program[] = { //killbits
      0x21,0x00,0x00,
      0x16,0x80,
      0x01,0x00,0x20, //modified speed
      0x1a,
      0x1a,
      0x1a,
      0x1a,
      0x09,
      0xd2,0x08,0x00,
      0xdb,0xff,
      0xaa,
      0x0f,
      0x57,
      0xc3,0x08,0x00 
    };
    loadData(program,sizeof(program),0);
  }
} loader;
