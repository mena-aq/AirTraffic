#include <iostream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include <chrono>


//FIFO
#define AVN_FIFO1 "pipes/avnfifo_ATC"
#define AVN_FIFO2 "pipes/avnfifo_GEN"


// AVN (Violation Notice) //maybe we can make a .h
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

    ViolationInfo():flightID(-1),airline(0),airlineType(0),speedRecorded(0),phaseViolation(0),violationTimestamp(0),amountDue(0),status(0){}

    ViolationInfo(int flightID,int airline,int airlineType,float speedRecorded,int phaseViolation)
    : flightID(flightID), airline(airline), airlineType(airlineType), speedRecorded(speedRecorded), phaseViolation(phaseViolation){
        this->violationTimestamp = std::time(nullptr); //save current time
        auto currentTime = std::chrono::system_clock::now();
    }

    void generateFee(){
        //generate AVN
        if(this->airlineType == 3){
            this->amountDue= 500000;
            this->amountDue+= 0.15*this->amountDue;
        }
        else if (this->airlineType ==  2){
            this->amountDue= 700000;
            this->amountDue+= 0.15*this->amountDue;
        }
        else{
            this->status = 1; //if no fee consider paid??
        }

    }
};
    



int main(){

    mkfifo(AVN_FIFO1,0666);
    mkfifo(AVN_FIFO2,0666);

    int fd=-1;
    while (1){
        //read request
        ViolationInfo* violation = new ViolationInfo;
        fd = open(AVN_FIFO1,O_RDONLY);
        read(fd,(void*)violation,sizeof(ViolationInfo));
        close(fd);

        //return fee
        violation->generateFee();
        fd = open(AVN_FIFO2,O_WRONLY);
        write(fd,(void*)violation,sizeof(ViolationInfo));
        close(fd);
        //printf("returned fee: %f\n",violation->amountDue);

        free(violation);
    }
}