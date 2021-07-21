#pragma once
#include "noncopyable.h"
#include <assert.h>
#include "CurrentThread.h"
#include "epoll.h"
#include "mutex.h"
#include <functional>
#include <memory>
#include <vector>
#include "channel.h"
#include "util.h"
#include <iostream>
using namespace std;

class eventloop
{
public:
  typedef std::function<void()> Functor;
  eventloop();
  ~eventloop();
  void loop();
  void quit();
  void runInLoop(Functor&& cb);
  void queueInLoop(Functor&& cb);
  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
  void assertInLoopThread() { assert(isInLoopThread()); }
  void shutdown(shared_ptr<Channel> channel) { shutDownWR(channel->getfd()); }
  void removeFromPoller(shared_ptr<Channel> channel) {

    poller_->epoll_del(channel);
  }
  void updatePoller(shared_ptr<Channel> channel, int timeout = 0) {
    poller_->epoll_mod(channel, timeout);
  }
  void addToPoller(shared_ptr<Channel> channel, int timeout = 0) {
    poller_->epoll_add(channel, timeout);
  }

 private:
  bool looping_;
  shared_ptr<Epoll> poller_;
  int wakeupFd_;
  bool quit_;
  bool eventHandling_;
  mutable mutexlock mutex_;
  std::vector<Functor> pendingFunctors_;
  bool callingPendingFunctors_;
  const pid_t threadId_;
  shared_ptr<Channel> pwakeupChannel_;

  void wakeup();
  void handleRead();
  void doPendingFunctors();
  void handleConn();
};