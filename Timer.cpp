#include "Timer.h"
#include <sys/time.h>
#include <unistd.h>
#include <queue>

Timer::Timer(std::shared_ptr<Connection> requestData, int timeout):deleted_(false), SP_Connection(requestData)
{
    struct timeval now;
  gettimeofday(&now, NULL);
  expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

Timer::~Timer() {
  if (SP_Connection) SP_Connection->handleClose();
}

Timer::Timer(Timer &t)
    : SP_Connection(t.SP_Connection), expiredTime_(0) {}


    void Timer::update(int timeout)
    {
    struct timeval now;
    gettimeofday(&now, NULL);
    expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
    }

    bool Timer::isValid()
    {
        struct timeval now;
       gettimeofday(&now, NULL);
       if(((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)< expiredTime_) 
       return true;
        else
        {
            this->setDeleted();
        }
        return false;
        
    }


    void Timer::clearReq() {
  SP_Connection.reset();
  this->setDeleted();
}




void TimerManager::addTimer(std::shared_ptr<Connection> SP_Connection, int timeout) {
  SP_Timer new_timer(new Timer(SP_Connection, timeout));
  timerQueue.push(new_timer);
  SP_Connection->linkTimer(new_timer);
}



void TimerManager::handleExpiredEvent() {
  while (!timerQueue.empty()) {
    SP_Timer sp_timer_now = timerQueue.top();
    if (sp_timer_now->isDeleted())
      timerQueue.pop();
    else if (sp_timer_now->isValid() == false)
      timerQueue.pop();
    else
      break;
  }
}