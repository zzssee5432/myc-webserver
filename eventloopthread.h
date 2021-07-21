#pragma once
#include "eventloop.h"
#include "condition.h"
#include "mutex.h"
#include "thread.h"
#include "noncopyable.h"



class eventloopthread : noncopyable {
 public:
  eventloopthread(); 
  ~eventloopthread();
  eventloop* startLoop();

 private:
  void threadFunc();
  eventloop* loop_;
  bool exiting_;
  Thread thread_;
  mutexlock mutex_;
  Condition cond_;
};