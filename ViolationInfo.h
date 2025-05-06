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

    float amountDue;
    bool status;  // true= paid, false= unpaid
    //time_t dueDate;

    ViolationInfo(){}

    ViolationInfo(int flightID,int airline,int airlineType,float speedRecorded,int phaseViolation)
    : flightID(flightID), airline(airline), airlineType(airlineType), speedRecorded(speedRecorded), phaseViolation(phaseViolation){
        this->violationTimestamp = std::time(nullptr); //save current time
        auto currentTime = std::chrono::system_clock::now();
    }

    void generateFee(){
        //generate AVN
        if(this->airlineType == AirlineType::COMMERCIAL){
            this->amountDue= 500000;
        }
        else if (this->airlineType ==  AirlineType::CARGO){
            this->amountDue= 700000;
        }
        else{
            this->amountDue = 10000; //less for military/medical
        }
        this->amountDue+= 0.15*this->amountDue; //admin fee
    }
};

#endif