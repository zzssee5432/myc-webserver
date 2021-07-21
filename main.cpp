#include "server.h"
#include "eventloop.h"

int main(int argc, char *argv[]) {
  int threadNum = 4;
  int port =1234;

  int opt;
  const char *str = "t:l:p:";
  while ((opt = getopt(argc, argv, str)) != -1) {
    switch (opt) {
      case 't': {
        threadNum = atoi(optarg);
        break;
      }
      case 'p': {
        port = atoi(optarg);
        break;
      }
      default:
        break;
    }
  }


  eventloop mainLoop;
  server myserver(&mainLoop, threadNum, port);
  myserver.start();
  mainLoop.loop();
  return 0;
}
