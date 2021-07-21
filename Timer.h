#pragma once
#include <unistd.h>
#include <deque>
#include <memory>
#include <queue>
#include "Connection.h"
#include "mutex.h"
#include "noncopyable.h"
#include "Connection.h"

class Connection;


class Timer{
    
  bool deleted_;
  size_t expiredTime_;
  std::shared_ptr<Connection> SP_Connection;

public:
  Timer(std::shared_ptr<Connection> requestData, int timeout);
  ~Timer();
  Timer(Timer &t);
  void update(int timeout);
  bool isValid();
  void clearReq();
  void setDeleted() { deleted_ = true; }
  bool isDeleted() const { return deleted_; }
  size_t getExpTime() const { return expiredTime_; }

};

class TimerCmp {
    public:
  bool operator()(std::shared_ptr<Timer> &a,
                  std::shared_ptr<Timer> &b) const {
    return a->getExpTime() > b->getExpTime();
  }
};



class TimerManager{
typedef std::shared_ptr<Timer> SP_Timer;
std::priority_queue<SP_Timer, std::deque<SP_Timer>, TimerCmp>
      timerQueue;
public:
TimerManager() {}
~TimerManager() {}
void addTimer(std::shared_ptr<Connection> SP_Connection, int timeout);
void handleExpiredEvent();
};



