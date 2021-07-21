#include "eventloopthread.h"
#include <functional>

using namespace std;

eventloopthread::eventloopthread():
loop_(nullptr),
exiting_(false),
thread_(bind(&eventloopthread::threadFunc,this),"eventloopthread"),
mutex_(),
cond_(mutex_)
{}

eventloopthread::~eventloopthread()
{
    exiting_=true;
    if(loop_!=nullptr)
{  loop_->quit();
    thread_.join();
}
}

eventloop* eventloopthread::startLoop()
{
    assert(!thread_.started());
    thread_.start();
    {mutexlockguard lock(mutex_);
    while (loop_ == NULL) cond_.wait();
    }
    return loop_;
}

void eventloopthread::threadFunc() {
  eventloop loop;

  {
    mutexlockguard lock(mutex_);
    loop_ = &loop;
    cond_.notify();
  }

  loop.loop();
  loop_ = NULL;
}