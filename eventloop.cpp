#include "eventloop.h"
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include "util.h"

using namespace std;

int createEventfd() {
  int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    abort();
  }
  return evtfd;
}

__thread eventloop* t_loopInThisThread = 0;

 eventloop::eventloop():looping_(false),
 quit_(false),
 eventHandling_(false),
 callingPendingFunctors_(false),
      poller_(new Epoll()),
      wakeupFd_(createEventfd()),
      threadId_(CurrentThread::tid()),
      pwakeupChannel_(new Channel(this, wakeupFd_)) 
 {
if (t_loopInThisThread) {
  } else {
    t_loopInThisThread = this;
  }
  pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
  pwakeupChannel_->setReadHandler(bind(&eventloop::handleRead, this));
  pwakeupChannel_->setConnHandler(bind(&eventloop::handleConn, this));
  poller_->epoll_add(pwakeupChannel_, 0);
 }

void eventloop::handleRead() {
  uint64_t one = 1;
  ssize_t n = readn(wakeupFd_, &one, sizeof one);
  pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
}


 void eventloop::handleConn() {
  updatePoller(pwakeupChannel_, 0);
}

void eventloop::wakeup() {
  uint64_t one = 1;
  ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof one);

}

void eventloop::runInLoop(Functor&& cb) {
  if (isInLoopThread())
    cb();
  else
    queueInLoop(std::move(cb));
}

eventloop::~eventloop()
{ 
    close(wakeupFd_);
  t_loopInThisThread = NULL;
}

void eventloop::queueInLoop(Functor&& cb) {
  {
    mutexlockguard lock(mutex_);
    pendingFunctors_.emplace_back(std::move(cb));
  }

  if (!isInLoopThread() || callingPendingFunctors_) wakeup();
}

void eventloop::loop() {
  assert(!looping_);
  assert(isInLoopThread());
  looping_ = true;
  quit_ = false;
  
  std::vector<SP_Channel> ret;
  while (!quit_) {
    ret.clear();
    ret = poller_->poll();
    eventHandling_ = true;
    
    for (auto& it : ret) it->handleEvents();
    
    eventHandling_ = false;

    doPendingFunctors();
    poller_->handleExpired();
  }
  looping_ = false;
}

void eventloop::doPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
    mutexlockguard lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (size_t i = 0; i < functors.size(); ++i) functors[i]();
  callingPendingFunctors_ = false;
}

void eventloop::quit() {
  quit_ = true;
  if (!isInLoopThread()) {
    wakeup();
  }
}

