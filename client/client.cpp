#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <iostream>


using namespace std;

#define MAXSIZE 1024
#define IPADDRESS "127.0.0.1"
#define SERV_PORT 1234
#define FDSIZE 1024
#define EPOLLEVENTS 20


int setSocketNonBlocking1(int fd) {
  int flag = fcntl(fd, F_GETFL, 0);
  if (flag == -1) return -1;

  flag |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flag) == -1) return -1;
  return 0;
}
int main(int argc, char *argv[]) {
  int sockfd;
  struct sockaddr_in servaddr;
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(SERV_PORT);
  inet_pton(AF_INET, IPADDRESS, &servaddr.sin_addr);
  char buff[4096];
  buff[0] = '\0';
  
 const char* p = "POST 127.0.0.1:1234/hello?arg1=1&arg2=2 HTTP/1.1\r\nUser-Agent: test\r\nContent-length: 15\r\nHost: 127.0.0.1\r\nConnection: Keep-Alive\r\n\r\nreceive content";
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == 0) {
    ssize_t n = write(sockfd, p, strlen(p));
    cout << "strlen(p) = " << strlen(p) << endl;
    sleep(10);
    n = read(sockfd, buff, 4096);
    cout << "n=" << n << endl;
    printf("%s", buff);
  } else {
    perror("err3");
  }

 p = "POST 127.0.0.1:1234/hello?arg3=3&arg4=4 HTTP/1.1\r\nUser-Agent: test\r\nContent-length: 19\r\nHost: 127.0.0.1\r\nConnection: Keep-Alive\r\n\r\nnew receive content";
 ssize_t n = write(sockfd, p, strlen(p));
    cout << "strlen(p) = " << strlen(p) << endl;
    sleep(10);
    n = read(sockfd, buff, 4096);
    cout << "n=" << n << endl;
    printf("%s", buff);

 close(sockfd);
  return 0;
}
