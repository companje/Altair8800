#include <stdio.h>
#include <unistd.h>
#include "Arduino.h"
#include "Altair8800.ino"

int main() {
  printf("Altair Emulator running on Arduino VM...\n");
  setup();
  while (1) {
    loop();
    
    fflush(stdout);
    // char ch = Serial.read();
    // if (ch!=0) Serial.print(ch);

    usleep(1000);
  }
}