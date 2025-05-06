#ifndef RADAR_H
#define RADAR_H

#include <pthread.h>
#include "Flight.h"
#include "ATCDashboard.h"

class Radar{
public:
    pthread_t thread_id;
    static ATCDashboard dashboard; //shared

    Radar():thread_id(-1){}

    //radar thread function (per flight)
    static void* flightRadar(void* arg){

        Flight* flight = (Flight*)arg; 
        bool violation[NUM_FLIGHT_PHASES]={false};

        
        while(flight && flight->phase!=FlightPhase::DONE){ //flight ongoing

            FlightPhase phase = flight->phase;
            int phaseIndex = static_cast<int>(phase);
            if (flight->checkViolation() && violation[phaseIndex]==false) {//if first violation of phase
                
                //send to AVN generator to generate AVN
                //send id, airline name,airline type, recorded speed, phase
                printf("Request AVN\n");
                ViolationInfo* violationDetails = new ViolationInfo(flight->id,static_cast<int>(flight->airlineName),static_cast<int>(flight->airlineType),flight->speed,static_cast<int>(flight->phase));
                int fd = open(AVN_FIFO1,O_WRONLY,0666);
                write(fd,(void*)violationDetails,sizeof(ViolationInfo));
                close(fd);
                flight->flagged = true;

                printf("--AVN DELIVERED--\n");
                //get back fee info
                /*
                fd = open(AVN_FIFO2,O_RDONLY,0666);
                read(fd,(void*)violation,sizeof(ViolationInfo));
                close(fd);
                */

                //write
                /*
                AVN avn(violation->flightID,static_cast<AirlineName>(violation->airline),static_cast<AirlineType>(violation->airlineType),violation->speedRecorded,static_cast<FlightPhase>(violation->phaseViolation),violation->violationTimestamp,violation->amountDue);
                avn.initGraphic(max_Y);
                AVNs.push_back(avn);
                printf("recived\n");
                dashboard.printAVNs();
                */

                violation[phaseIndex]=true;
            }
            sleep(1);
            //usleep(1000);
        }
        pthread_exit(nullptr);
    }

};
ATCDashboard Radar::dashboard;

#endif