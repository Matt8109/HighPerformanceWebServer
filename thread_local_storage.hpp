// Code copyright Alberto Lerner and Matthew Mancuso
// See git blame for details

// Cross Platform Thread Local Storage Class Using pthreads
// From: http://alex.tapmania.org/2011/03/simple-thread-local-storage-with-pthreads.html
// Accessed: Mar 26, 2012
#pragma once
#ifndef __TLS_H__
#define __TLS_H__
 
#include <pthread.h>
 
namespace base { 

template<typename T>
class ThreadLocalStorage
{
private:
    pthread_key_t   key_;
 
public:
    ThreadLocalStorage()
    {
        pthread_key_create(&key_, NULL);    
    }
   
    ~ThreadLocalStorage()
    {
        pthread_key_delete(key_);
    }
   
    ThreadLocalStorage& operator =(T* p)
    {
        pthread_setspecific(key_, p);
        return *this;
    }
   
    bool operator !()
    {
        return pthread_getspecific(key_)==NULL;
    }
   
    T* operator ->()
    {
        return static_cast<T*>(pthread_getspecific(key_));
    }
   
    T* get()
    {
        return static_cast<T*>(pthread_getspecific(key_));
    }
};

}
 
#endif // __TLS_H__