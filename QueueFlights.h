#ifndef QUEUEFLIGHTS_H
#define QUEUEFLIGHTS_H

#include "Flight.h"

class QueueFlights {
public:
    vector<Flight*> flightQueue;
    
    // Helper to sort a vector based on Comparator
    void sortQueue() {
        std::sort(flightQueue.begin(), flightQueue.end(), [](Flight* a, Flight* b) {
            if (a->priority != b->priority)
                return a->priority > b->priority; // Higher priority = comes first
            return a->id < b->id; // FCFS if same priority
        });
    }
    void sortQueue2() {
        std::sort(flightQueue.begin(), flightQueue.end(), [](Flight* a, Flight* b) {
            return a->scheduledTime < b->scheduledTime; // earlier time comes first
        });
    }

    void addFlight(Flight*& flight) {
        printf("added flight %d\n",flight->id);
        flightQueue.push_back(flight);
    }
    void addFlightAtFront(Flight*& flight){
        flightQueue.insert(flightQueue.begin(), flight);
    }
    Flight* getNextFlight(){
        if (!flightQueue.empty()){
            Flight* nextFlight = flightQueue.front();
            flightQueue.erase(flightQueue.begin());
            return nextFlight;
        }
        else 
            return nullptr;
    }
    Flight* peekNextFlight(){
        if (!flightQueue.empty()){
            Flight* nextFlight = flightQueue.front();
            return nextFlight;
        }
        else 
            return nullptr;
    }
    bool isEmpty(){
        return flightQueue.empty();
    }
    int numInQueue(){
        return flightQueue.size();
    }
    void printQueues() {
        cout<<"Flights in Queue\n";
        for (const auto& flight : flightQueue) {
            flight->printStatus();
        }
    }
    Flight*& operator[](int i){
        return flightQueue[i];
    } 
};

#endif