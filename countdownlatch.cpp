#include "countdownlatch.h"

CountDownLatch::CountDownLatch(int count)
    : mutex_(), condition_(mutex_), count_(count) {}

void CountDownLatch::wait() {
  mutexlockguard lock(mutex_);
  while (count_ > 0) condition_.wait();
}

void CountDownLatch::countDown() {
  mutexlockguard lock(mutex_);
  --count_;
  if (count_ == 0) condition_.notifyAll();
}