#ifndef ATCDASHBOARD_H
#define ATCDASHBOARD_H

#include "AVN.h"
#include <vector>

class ATCDashboard {
public:
    int numViolations;
    std::vector<AVN> AVNs;

    ATCDashboard(){
        this->numViolations=0;
    }
    
    void requestAVN(Flight* requestingFlight){
        //send to AVN generator to generate AVN
        //send id, airline name,airline type, recorded speed, phase
        printf("Request AVN\n");
        ViolationInfo* violation = new ViolationInfo(requestingFlight->id,static_cast<int>(requestingFlight->airlineName),static_cast<int>(requestingFlight->airlineType),requestingFlight->speed,static_cast<int>(requestingFlight->phase));
        int fd = open(AVN_FIFO1,O_WRONLY,0666);
        write(fd,(void*)violation,sizeof(ViolationInfo));
        close(fd);

        //get back fee info
        fd = open(AVN_FIFO2,O_RDONLY,0666);
        read(fd,(void*)violation,sizeof(ViolationInfo));
        close(fd);

        //write
        AVNs.push_back(AVN(violation->flightID,static_cast<AirlineName>(violation->airline),static_cast<AirlineType>(violation->airlineType),violation->speedRecorded,static_cast<FlightPhase>(violation->phaseViolation),violation->violationTimestamp,violation->amountDue));
    }

    void printAVNs(){
        for (int i=0; i<AVNs.size(); i++){
            AVNs[i].printAVN();
        }
    }
};

#endif