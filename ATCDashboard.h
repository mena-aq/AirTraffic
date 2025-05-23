#ifndef ATCDASHBOARD_H
#define ATCDASHBOARD_H

#include "AVN.h"
#include <vector>

class ATCDashboard {
public:
    PhaseRules rules;
    int numViolations;
    std::vector<AVN*> AVNs;
    float max_Y;
    float max_X;

    pthread_mutex_t lock;

    ATCDashboard(){
        this->numViolations=0;
        this->max_Y = 80;
        this->max_X = 10;

        pthread_mutex_init(&lock,NULL);
    }

    void addAVN(ViolationInfo violation){
        float lowerLim = rules[(int)violation.phaseViolation].speedLowerLimit;
        float upperLim = rules[(int)violation.phaseViolation].speedUpperLimit;
        AVN* avn = new AVN(numViolations,violation.flightID,static_cast<AirlineName>(violation.airline),static_cast<AirlineType>(violation.airlineType),violation.speedRecorded,static_cast<FlightPhase>(violation.phaseViolation),violation.violationTimestamp,violation.amountDue,lowerLim,upperLim);
        if (avn){
            this->numViolations++;
            avn->initGraphic(max_X,max_Y);
            max_Y += 130;
            if (max_Y  > 700){
                max_Y=80;
                max_X += 220;
            }

            AVNs.push_back(avn);
        }
        printAVNs();
    }
    
/*
    void requestAVN(Flight* requestingFlight){
        //send to AVN generator to generate AVN
        //send id, airline name,airline type, recorded speed, phase
        printf("Request AVN\n");
        ViolationInfo* violation = new ViolationInfo(requestingFlight->id,static_cast<int>(requestingFlight->airlineName),static_cast<int>(requestingFlight->airlineType),requestingFlight->speed,static_cast<int>(requestingFlight->phase));
        int fd = open(AVN_FIFO1,O_WRONLY,0666);
        write(fd,(void*)violation,sizeof(ViolationInfo));
        close(fd);

        printf("avn received\n");
        //get back fee info
        fd = open(AVN_FIFO2,O_RDONLY,0666);
        read(fd,(void*)violation,sizeof(ViolationInfo));
        close(fd);

        //write
        AVN avn(violation->flightID,static_cast<AirlineName>(violation->airline),static_cast<AirlineType>(violation->airlineType),violation->speedRecorded,static_cast<FlightPhase>(violation->phaseViolation),violation->violationTimestamp,violation->amountDue);
        avn.initGraphic(max_Y);
        AVNs.push_back(avn);
    }
*/

    void printAVNs(){
        for (int i=0; i<AVNs.size(); i++){
            AVNs[i]->printAVN();
        }
    }

    AVN* getAVNByID(int id){
        for (int i=0; i<numViolations; i++){
            if (AVNs[i]->avnID == id){
                return AVNs[i];
                break;
            }
        }
        return nullptr;
    }

    //clear AVN
    void clearAVNByID(int id){
        for (int i=0; i<numViolations; i++){
            if (AVNs[i]->flightID == id){
                pthread_mutex_lock(&lock);
                AVNs[i]->status=1;
                AVNs[i]->refreshGraphic(); //refresh
                printf("cleared\n");
                pthread_mutex_unlock(&lock);
                break;
            }
        }
    }

    //graphics
    void displayAVNs(sf::RenderWindow& window){
        for (int i=0; i<AVNs.size(); i++){
            AVNs[i]->drawGraphic(window);
        }
    }

    
};

#endif