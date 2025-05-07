#ifndef CHALLAN_H
#define CHALLAN_H

#include <ctime>
#include <chrono>

struct Challan {
    int avnID;
    int flightID;
    int airline;
    int airlineType;
    float amountDue;
    bool status;  // true= paid, false= unpaid

    Challan():flightID(-1),airline(0),airlineType(0),amountDue(0),status(0){}

    Challan(int flightID,int airline,int airlineType,float amount, int status): status(status),flightID(flightID), airline(airline), airlineType(airlineType), amountDue(amount){}

    void CalculatePayment(int amount){
        if(status==1){
            std::cout<<"Challan Paid\n";
            return;
        }
        if(amount> 0){
            amountDue-=amount;
            if(amountDue==0){
                std::cout<<"Paid Successfully!\n";
            }
        }
    }

    void printChallan(){
        std::cout<<"Flight ID: "<<flightID<<std::endl;
        std::cout<<"Airline TYpe: "<<airlineType<<std::endl;
        std::cout<<"amount due: "<<amountDue<<std::endl;
        std::cout<<"status: ";
        if(status==0){
            std::cout<<"Unpaid"<<std::endl<<std::endl;
        }
        else if(status==1){
            std::cout<<"Paid"<<std::endl<<std::endl;
        }
    }
};

#endif