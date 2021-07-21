#include <assert.h>
#include <errno.h>
#include <linux/unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <memory>
#include "thread.h"
#include "CurrentThread.h"

using namespace std;



class ThreadData
{
  public:
  typedef Thread::ThreadFunc  ThreadFunc;
  ThreadFunc threadfunc_;
  string name_;
  pid_t * tid_;
  CountDownLatch * latch_;

ThreadData(ThreadFunc& func,const string& name,pid_t * tid,CountDownLatch * latch):
threadfunc_(func),name_(name),tid_(tid),latch_(latch){}

void runThread() {
    *tid_ = CurrentThread::tid();
    tid_ = NULL;
    latch_->countDown();
    latch_ = NULL;
    CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
    prctl(PR_SET_NAME, CurrentThread::t_threadName);
    threadfunc_();
    CurrentThread::t_threadName = "finished";
  }

};


void* startThread(void* obj) {
  ThreadData* data = static_cast<ThreadData*>(obj);
  data->runThread();
  delete data;
  return NULL;
}


namespace CurrentThread {
__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char* t_threadName = "default";
}

pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

void CurrentThread::cacheTid() {
  if (t_cachedTid == 0) {
    t_cachedTid = gettid();
    t_tidStringLength =
        snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
  }
}



Thread::Thread(const ThreadFunc& func, const string& name):started_(false),
      joined_(false),
      pthreadId_(0),
      tid_(0),
      func_(func),
      name_(name),
      latch_(1) {
  setDefaultName();
}

Thread::~Thread() {
  if (started_ && !joined_) pthread_detach(pthreadId_);
}


void Thread::setDefaultName() {
  if (name_.empty()) {
    char buf[32];
    snprintf(buf, sizeof buf, "Thread");
    name_ = buf;
  }
}


void Thread::start() {
  assert(!started_);
  started_ = true;
  ThreadData* data = new ThreadData(func_, name_, &tid_, &latch_);
  if (pthread_create(&pthreadId_, NULL, &startThread, data)) {
    started_ = false;
    delete data;
  } else {
    latch_.wait();
    assert(tid_ > 0);
  }
}


int Thread::join() {
  assert(started_);
  assert(!joined_);
  joined_ = true;
  return pthread_join(pthreadId_, NULL);
}