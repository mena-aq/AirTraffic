#ifndef VIOLATIONINFO_H
#define VIOLATIONINFO_H

#include <ctime>
#include <chrono>

struct ViolationInfo {
    int flightID;
    int airline;
    int airlineType;
    float speedRecorded;
    int phaseViolation;
    time_t violationTimestamp;
    //long violationTimestamp;
    //for AVN generator
    float amountDue;
    bool status;  // true= paid, false= unpaid

    ViolationInfo(int flightID,int airline,int airlineType,float speedRecorded,int phaseViolation)
    : flightID(flightID), airline(airline), airlineType(airlineType), speedRecorded(speedRecorded), phaseViolation(phaseViolation){
        this->violationTimestamp = std::time(nullptr); //save current time
        auto currentTime = std::chrono::system_clock::now();
    }
};

#endif