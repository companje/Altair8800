class SoftwareSerial {
public:
  
  SoftwareSerial(int a, int b) {
  }

  void begin(int baudrate) {

  }

  bool available() {
    return true;
  }

  byte read() {
    return 0;
  }

  void print(int a, int mode=0) {
    printf("%d",a);
  }

  size_t print(const char str[]) {
    printf("%s",str);
    return 0;
  }


};
