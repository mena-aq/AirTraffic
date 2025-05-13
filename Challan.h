#ifndef CHALLAN_H
#define CHALLAN_H

#include <ctime>
#include <chrono>
#include "Types.h"

struct Challan {
    int avnID;
    int flightID;
    int airline;
    int airlineType;
    float amountDue;
    bool status;  
    time_t dueDate;

    Challan():avnID(-1),flightID(-1),airline(0),airlineType(0),amountDue(0),status(0){}

    Challan(int avnId, int flightID,int airline,int airlineType,float amount, int status,time_t dueDate): dueDate(dueDate), avnID(avnId), status(status),flightID(flightID), airline(airline), airlineType(airlineType), amountDue(amount){}

    void CalculatePayment(int amount){
        if(status==1){
            std::cout<<"Challan Paid\n";
            return;
        }
        if(amount> 0){
            amountDue-=amount;
            if(amountDue==0){
                auto now = std::chrono::system_clock::now();

                //timePaid = std::chrono::system_clock::to_time_t(now);

                std::cout<<"Paid Successfully!\n";
            }
        }
    }

    void printChallan(){
        std::cout<<"Flight ID: "<<flightID<<std::endl;
        std::cout<<"Airline Type: "<< getAirlineType(airlineType) <<std::endl;
        std::cout<<"Amount due: "<<amountDue<<std::endl;
        std::cout<<"Status: ";
        if(status==0){
            std::cout<<"Unpaid"<<std::endl<<std::endl;
        }
        else if(status==1){
            std::cout<<"Paid"<<std::endl<<std::endl;
        }
        else{
            std::cout<<"Overdue"<<std::endl<<std::endl;
        }
    }
};

#endif