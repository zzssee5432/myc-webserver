#pragma once
#include <memory>
#include <vector>
#include "eventloopthread.h"
#include "noncopyable.h"


class eventloopthreadpool : noncopyable {
 public:
  eventloopthreadpool(eventloop* baseLoop, int numThreads);

  ~eventloopthreadpool() {  }
  void start();

  eventloop* getNextLoop();

 private:
  eventloop* baseLoop_;
  bool started_;
  int numThreads_;
  int next_;
  std::vector<eventloop*> loops_;
  std::vector<std::shared_ptr<eventloopthread>> threads_;
};