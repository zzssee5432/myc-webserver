#include "channel.h"
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <queue>
#include "epoll.h"
#include "eventloop.h"
#include "util.h"



Channel::Channel(eventloop *loop):loop_(loop),events_(0), lastEvents_(0), fd_(0)
{}

Channel::Channel(eventloop *loop, int fd):loop_(loop),fd_(fd),events_(0), lastEvents_(0)
{
    //cout<<"channel"<<endl;
 }

Channel::~Channel(){
    //cout<<"~channel"<<endl;
}

int Channel::getfd()
{
    return fd_;
}

void Channel::setfd(int fd)
{
    fd_=fd;
}

void Channel::handleRead() {
   
  if (readHandler_) 
    readHandler_();
}

void Channel::handleError() {
  if (errorHandler_) 
    errorHandler_();
}

void Channel::handleWrite()
{
    if(writeHandler_)
    writeHandler_();
}

void Channel::handleConn()
{
    if(connHandler_)
    connHandler_();
}
