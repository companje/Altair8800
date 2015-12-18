class DiskController {
public:

  enum Status { ENWD=1, MOVE_HEAD=2, HEAD=4, IE=32, TRACK_0=64, NRDA=128 };
  enum Control { STEP_IN=1,STEP_OUT=2,HEAD_LOAD=4,HEAD_UNLOAD=8,IE_CTRL=16,ID=32,HCS=64,WE=128 };

  #define SECTOR 137UL
  #define TRACK (32UL*SECTOR)

  struct Disk {
    File fp;
    byte track;
    byte sector;
    byte status;
    byte write_status;
  } disk1,disk2,nodisk,*selected;
  
  DiskController() {
    nodisk.status = 0xff;
  }
  
  void mount(Disk &disk, const char *dsk_file) {
    disk.fp = SD.open(dsk_file, FILE_WRITE);
  }

  void set_status(byte bit) {
    selected->status &= ~bit;
  }

  void clear_status(byte bit) {
    selected->status |= bit;
  }

  void select(byte b) {
    byte select = b & 0xf;
    //Serial.println(select,HEX);
    selected = select == 0 ? &disk1 : select == 1 ? &disk2 : &nodisk;
  }

  byte status() {
    return selected->status;
  }

  void function(byte b) {
    if (b & STEP_IN) {
      selected->track++;
      if (selected->track != 0) clear_status(TRACK_0);
      selected->fp.seek(TRACK * selected->track);
    }
    
    if (b & STEP_OUT) {
      if (selected->track > 0) selected->track--;
      if (selected->track == 0) set_status(TRACK_0);
      selected->fp.seek(TRACK * selected->track);
    }
    
    if (b & HEAD_LOAD) {
      set_status(HEAD);
      set_status(NRDA);
    }
    
    if (b & HEAD_UNLOAD) {
      clear_status(HEAD);
    }
    
    if(b & WE) {
      set_status(ENWD);
      selected->write_status = 0;
    }
  }

  byte sector() {
    if (selected->sector == 32) selected->sector = 0;
    selected->fp.seek(selected->track * TRACK + selected->sector * (SECTOR));
    byte ret_val = selected->sector << 1;
    selected->sector++;
    return ret_val;
  }

  void write(byte b) {
    selected->fp.write(&b, 1);
    if(selected->write_status == 137) {
      selected->write_status = 0;
      clear_status(ENWD);
    } else {
      selected->write_status++;
    }
  }

  byte read() {
    return selected->fp.read();
  }

} disk;

