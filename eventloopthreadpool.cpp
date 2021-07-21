#include "eventloopthreadpool.h"
#include <assert.h>


eventloopthreadpool::eventloopthreadpool(eventloop* baseloop, int numThreads):
baseLoop_(baseloop),
numThreads_(numThreads),
started_(false),
next_(0)
{}

void eventloopthreadpool::start()
{
    baseLoop_->assertInLoopThread();
    started_ = true;
  for (int i = 0; i < numThreads_; ++i) {
    std::shared_ptr<eventloopthread> t(new eventloopthread());
    loops_.push_back(t->startLoop());
    threads_.push_back(t);
  }
}

eventloop* eventloopthreadpool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
  assert(started_);
  eventloop *loop = baseLoop_;
  if (!loops_.empty()) {
    loop = loops_[next_];
    next_ = (next_ + 1) % numThreads_;
  }
  return loop;
}