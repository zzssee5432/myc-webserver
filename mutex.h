#pragma once
#include <pthread.h>
#include <cstdio>
#include "noncopyable.h"
class mutexlock:noncopyable{
    public:
    mutexlock(){pthread_mutex_init(&mutex,NULL);}
    ~mutexlock()
    {pthread_mutex_lock(&mutex);
    pthread_mutex_destroy(&mutex);
    }
    void lock(){pthread_mutex_lock(&mutex);}
    void unlock(){pthread_mutex_unlock(&mutex);}
    pthread_mutex_t * get(){return &mutex;}
    private:
    pthread_mutex_t mutex;

    friend class condition;
};

class mutexlockguard:noncopyable{
    public:
    mutexlockguard(mutexlock &_mutex):mutex(_mutex){mutex.lock();}
    ~mutexlockguard(){mutex.unlock();}
  
    private:
    mutexlock &mutex;
};

