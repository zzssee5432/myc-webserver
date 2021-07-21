#pragma once
#include <sys/epoll.h>
#include <sys/epoll.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <iostream>
using namespace std;
class  Connection;
class eventloop;

class Channel{
   private:
    typedef std::function<void()> CallBack;
  eventloop *loop_;
  int fd_;
  __uint32_t events_;
  __uint32_t revents_;
  __uint32_t lastEvents_;

CallBack readHandler_;
CallBack writeHandler_;
CallBack errorHandler_;
CallBack connHandler_;

std::weak_ptr<Connection> holder_;

int parse_URI();
  int parse_Headers();
  int analysisRequest();

 public:
  Channel(eventloop *loop);
  Channel(eventloop *loop, int fd);
  ~Channel();
  int getfd();
  void setfd(int fd);

  void setHolder(std::shared_ptr<Connection> holder) { holder_ = holder; }
  std::shared_ptr<Connection> getHolder() {
    std::shared_ptr<Connection> ret(holder_.lock());
    return ret;
  }


    void setReadHandler(CallBack &&readHandler) { 
      readHandler_ = readHandler; }
  void setWriteHandler(CallBack &&writeHandler) {
    writeHandler_ = writeHandler;
  }
  void setErrorHandler(CallBack &&errorHandler) {
    errorHandler_ = errorHandler;
  }
  void setConnHandler(CallBack &&connHandler) { connHandler_ = connHandler; }

  void handleEvents() {
   
    events_ = 0;
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
      events_ = 0;
      return;
    }
    if (revents_ & EPOLLERR) {
      if (errorHandler_) errorHandler_();
      events_ = 0;
      return;
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
      handleRead();
  
    }
    if (revents_ & EPOLLOUT) {
      handleWrite();
    }

    handleConn();
  }
  void handleRead();
  void handleWrite();
  void handleError();
  void handleConn();

  void setRevents(__uint32_t ev) { revents_ = ev; }

  void setEvents(__uint32_t ev) { events_ = ev; }
  __uint32_t &getEvents() { return events_; }

  bool EqualAndUpdateLastEvents() {
    bool ret = (lastEvents_ == events_);
    lastEvents_ = events_;
    return ret;
  }

  __uint32_t getLastEvents() { return lastEvents_; }
};

typedef std::shared_ptr<Channel> SP_Channel;