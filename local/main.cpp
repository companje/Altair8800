#ifndef ARDUINO

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include "keyboard.h"
#include "disk.h"

char term_in() {
  if (!kbhit()) return 0;
  unsigned char ch = fgetc(stdin);    //printf("char: %02xh\n",ch);
  if (ch==0x0a) ch=0x0d;  //replace line-feed by carriage-return
  return ch;
}

extern "C" {
  #include "i8080.h"
  #include "i8080_hal.h"

  int hal_io_input(int port) {
    static uint8_t character = 0;

    switch (port) {
      case 0x00:
        return 0;
      case 0x01: //serial read
        return term_in();
      case 0x08: 
        return disk_status();
      case 0x09:
        return disk_sector();
      case 0x0a: 
        return disk_read();
      case 0x10: //2SIO port 1 status
        if (!character) {
          // printf("2SIO port 1 status\n");
          character = term_in();
        }
        return (character ? 0b11 : 0b10); 
      case 0x11: //2SIO port 1, read
        if (character) {
          int tmp = character; 
          character = 0; 
          return tmp; 
        } else {
          return term_in();
        }
      case 0xff: //sense switches
        return 0;
      default:
        printf("%04x: in 0x%02x\n",i8080_pc(),port);
        exit(1);
        return 1;
    }
    return 1;
  }

  void hal_io_output(int port, int value) {
    switch (port) {
      case 0x01: 
        printf("%c",value & 0x7f);
        fflush(stdout);
        break;
      case 0x08:
        disk_select(value);
        break;
      case 0x09:
        disk_function(value);
        break;
      case 0x0a:
        disk_write(value);
        break;
      case 0x10: // 2SIO port 1 control
        //nothing
        break;
      case 0x11: // 2SIO port 1 write  
        printf("%c",value & 0x7f);
        fflush(stdout);
        break;
      case 0x12: // ????
        break;
      default:
        printf("%04x: out 0x%02x\n",i8080_pc(),port);
        exit(1);
        break;
    }
  }  
}



void loadRom() {
  const uint8_t rom[] = {
    #include "data/4kbas32.h"
  };
  unsigned char* mem = i8080_hal_memory();
  memset(mem, 0, 0x4000);
  memcpy(mem, rom, sizeof rom);
  i8080_jump(0x0000);
}

void load_mem_file(const char* filename, size_t offset) {
  size_t size;
  FILE* fp = fopen(filename, "rb");
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  unsigned char* mem = i8080_hal_memory();
  fread(mem+offset, 1, size, fp);
  fclose(fp);
}

int main() {
  printf("[altair8800]\n");

  i8080_init();

  // load_mem_file("data/4kbas32.bin", 0);
  // i8080_jump(0);

  load_mem_file("data/88dskrom.bin", 0xff00);
  disk_drive.disk1.fp = fopen("data/cpm63k.dsk", "r+b");
  disk_drive.disk2.fp = fopen("data/zork.dsk", "r+b");
  disk_drive.nodisk.status = 0xff;
  i8080_jump(0xff00);

  nonblock(NB_ENABLE); //keyboard

  while (1) {
    i8080_instruction();
    //printf("%04x\n",i8080_pc());
    //usleep(1000);
  }
}

#endif
