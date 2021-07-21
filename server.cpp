#include "server.h"
#include "util.h"
#include <functional>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <memory>
#include <string.h>


using namespace std;

server::server(eventloop *loop, int threadNum, int port)
:loop_(loop),
threadNum_(threadNum),
port_(port),
 eventloopthreadpool_(new eventloopthreadpool(loop_, threadNum)),
      started_(false),
      acceptChannel_(new Channel(loop_)),
      listenFd_(socket_bind_listen(port_))
{
acceptChannel_->setfd(listenFd_);
  handle_for_sigpipe();
  if (setSocketNonBlocking(listenFd_) < 0) {
    perror("set socket non block failed");
    abort();
  }
}

void server::start() {
  eventloopthreadpool_->start();
  acceptChannel_->setEvents(EPOLLIN | EPOLLET);
  acceptChannel_->setReadHandler(bind(&server::handNewConn, this));
  acceptChannel_->setConnHandler(bind(&server::handThisConn, this));
  loop_->addToPoller(acceptChannel_, 0);
  started_ = true;
}

void server::handNewConn() {
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(struct sockaddr_in));
  socklen_t client_addr_len = sizeof(client_addr);
  int accept_fd = 0;
  while ((accept_fd = accept(listenFd_, (struct sockaddr *)&client_addr,
                             &client_addr_len)) > 0) {
    eventloop *loop = eventloopthreadpool_->getNextLoop();
    if (accept_fd >= MAXFDS) {
      close(accept_fd);
      continue;
     }
    if (setSocketNonBlocking(accept_fd) < 0) {
      return;
    }

    setSocketNodelay(accept_fd);

    shared_ptr<Connection> req_info(new Connection(loop, accept_fd));
    req_info->getChannel()->setHolder(req_info);
    loop->queueInLoop(bind(&Connection::newEvent, req_info));
  }
  acceptChannel_->setEvents(EPOLLIN | EPOLLET);
}
