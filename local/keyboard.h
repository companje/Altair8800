#ifndef ARDUINO

//http://cc.byexamples.com/2007/04/08/non-blocking-user-input-in-loop-without-ncurses/

#define NB_ENABLE 0
#define NB_DISABLE 1

int kbhit()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

void nonblock(int state) {
  struct termios ttystate;
  //get the terminal state
  tcgetattr(STDIN_FILENO, &ttystate);
  if (state==NB_ENABLE) {
      //turn off canonical mode
      //ttystate.c_lflag &= ~ICANON;

      //turn off canonical mode AND ECHO
      //https://github.com/digidotcom/xbee_ansic_library/blob/master/src/posix/xbee_readline.c
      ttystate.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);

      //minimum of number input read.
      ttystate.c_cc[VMIN] = 1;
  }
  else if (state==NB_DISABLE) {
      //turn on canonical mode
      ttystate.c_lflag |= ICANON;
  }
  //set the terminal attributes.
  tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}

#endif