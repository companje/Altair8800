class Serial {
public:

  void begin(int baudrate) {

  }

  bool available() {
    return true;
  }

  byte read() {
    return getch();
    //return 0;
  }

  void print(int a, int mode=0) {
    printf("%d",a);
  }

  void print(char a) {
    printf("%c",a);
  }

  size_t print(const char str[]) {
    printf("%s",str);
    return 0;
  }

  void println(int a, int mode=0) {
    printf("%d\n",a);
  }

  size_t println(const char str[]) {
    printf("%s\n",str);
    return 0;
  }

} Serial;