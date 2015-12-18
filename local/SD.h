#define FILE_WRITE 0 //?
#define FILE_READ 1 //?

class SD {
public:

//ONLY ONE FILE CAN BE OPEN ACCORDING TO THE OFFICIAL SD LIBRARY.
//HOW CAN DISK1.DSK AND DISK2.DSK BE OPEN SIMULTANIOUSLY? OR IS THIS NOT CASE?

  File fp;

  File open(const char *filename, int mode = FILE_READ) {
    return fp; //makes a copy
  }

  bool begin(int a) {
    return true;
  }

} SD;