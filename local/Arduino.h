typedef unsigned char byte;
// typedef unsigned short uint16_t;
typedef bool boolean;

#include <unistd.h>
#include <termios.h>

char getch() {
  char buf = 0;
  struct termios old = {0};
  if (tcgetattr(0, &old) < 0)
          perror("tcsetattr()");
  old.c_lflag &= ~ICANON;
  old.c_lflag &= ~ECHO;
  old.c_cc[VMIN] = 1;
  old.c_cc[VTIME] = 0;
  if (tcsetattr(0, TCSANOW, &old) < 0)
          perror("tcsetattr ICANON");
  if (read(0, &buf, 1) < 0)
          perror ("read()");
  old.c_lflag |= ICANON;
  old.c_lflag |= ECHO;
  if (tcsetattr(0, TCSADRAIN, &old) < 0)
          perror ("tcsetattr ~ICANON");
  return (buf);
}

#include <stdint.h>
#include "File.h"
#include "Serial.h"
#include "wiring_constants.h"

void pinMode(int a, int b) { 

}

void digitalWrite(int a, int b) {

}

void shiftOut(int a, int b, int c, int d) {

}

int shiftIn(int a, int b, int c) {
  return 0;
}
