#include "epoll.h"
#include <arpa/inet.h>
#include <iostream>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <deque>
#include <queue>
#include "util.h"

using namespace std;
const int EVENTSNUM = 4096;
const int EPOLLWAIT_TIME = 10000;

Epoll::Epoll() : epollFd_(epoll_create1(EPOLL_CLOEXEC)), events_(EVENTSNUM) {
  assert(epollFd_ > 0);
}
Epoll::~Epoll() {}

void Epoll::epoll_add(SP_Channel request, int timeout) {
  int fd = request->getfd();
  if (timeout > 0) {
    add_timer(request, timeout);
    fd2http_[fd] = request->getHolder();
  }
  struct epoll_event event;
  event.data.fd = fd;
  event.events = request->getEvents();

  request->EqualAndUpdateLastEvents();

  fd2chan_[fd] = request;
  if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0) {
    perror("epoll_add error");
    fd2chan_[fd].reset();
  }
}

void Epoll::epoll_mod(SP_Channel request, int timeout)
{
  int fd = request->getfd();
  if (timeout > 0) {
    add_timer(request, timeout);
  }
  if (!request->EqualAndUpdateLastEvents()) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0) {
      perror("epoll_mod error");
      fd2chan_[fd].reset();
    }
  }
  }


void Epoll::epoll_del(SP_Channel request) {
  int fd = request->getfd();
  struct epoll_event event;
  event.data.fd = fd;
  event.events = request->getLastEvents();
  // event.events = 0;
  // request->EqualAndUpdateLastEvents()
  if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0) {
    perror("epoll_del error");
  }
  fd2chan_[fd].reset();
  fd2http_[fd].reset();
}


std::vector<std::shared_ptr<Channel>> Epoll::poll()
{

   while (true) {
     cout<<"wait"<<endl;
    int event_count =
        epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
    if (event_count < 0) perror("epoll wait error");
    std::vector<SP_Channel> req_data = getEventsRequest(event_count);
    if (req_data.size() > 0) return req_data;
  }
}

std::vector<SP_Channel>  Epoll::getEventsRequest(int event_count)
{
  std::vector<SP_Channel>  req;
  for(int i=0;i<event_count;i++)
  {
    int fd=events_[i].data.fd;
    SP_Channel request_now=fd2chan_[fd];
    if(request_now)
    {  request_now->setRevents(events_[i].events);
      request_now->setEvents(0);
      req.push_back(request_now);
      }
     
  }
   return req;
}


void Epoll::add_timer(SP_Channel request_data, int timeout) {
  shared_ptr<Connection> t = request_data->getHolder();
  if (t)
    timerManager_.addTimer(t, timeout);
  
}

void Epoll::handleExpired()
{
  timerManager_.handleExpiredEvent();
  return;
}



