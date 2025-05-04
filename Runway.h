
#ifndef RUNWAY_H
#define RUNWAY_H

#include <pthread.h>
#include <stdio.h>

class Runway {
private:
    pthread_mutex_t lock;
public:
    char runwayID;

    Runway(char id) : runwayID(id) {
        pthread_mutex_init(&lock, nullptr);
    }

    ~Runway() {
        pthread_mutex_destroy(&lock);
    }

    void acquireRunway(){
        printf("try to acquire runway %c\n",this->runwayID);
        pthread_mutex_lock(&lock);
        printf("acquired runway %c\n",this->runwayID);
    }
    void releaseRunway(){
        pthread_mutex_unlock(&lock);
    }

    
};
    
#endif