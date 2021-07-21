
#pragma once
#include <memory>
#include "channel.h"
#include "eventloop.h"
#include "eventloopthreadpool.h"

class server {
 public:
  server(eventloop *loop, int threadNum, int port);
  ~server() {}
  eventloop *getLoop() const { return loop_; }
  void start();
  void handNewConn();
  void handThisConn() { loop_->updatePoller(acceptChannel_); }

 private:
  eventloop *loop_;
  int threadNum_;
  std::unique_ptr<eventloopthreadpool> eventloopthreadpool_;
  bool started_;
  std::shared_ptr<Channel> acceptChannel_;
  int port_;
  int listenFd_;
  static const int MAXFDS = 100000;
};