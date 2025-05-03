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
                
                //dashboard.requestAVN(flight);
                //printf("recived\n");
                //dashboard.printAVNs();

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