#ifndef RADAR_H
#define RADAR_H

#include <pthread.h>
#include "Flight.h"
#include "ATCDashboard.h"


class Radar{
public:
    //pthread_t thread_id;
    //static ATCDashboard dashboard; //shared
    static pthread_mutex_t lock;
    static vector<Flight*> flaggedFlights;
    static sf::Text list;

    Radar(){
        list.setFont(globalFont);
        list.setFillColor(sf::Color::Red); 
        list.setOutlineColor(sf::Color::White);
        list.setOutlineThickness(2); 
        list.setCharacterSize(13);
        list.setPosition(10,200);

        pthread_mutex_init(&lock,NULL);
    }

    //radar thread function (per flight)
    static void* flightRadar(void* arg){

        Flight* flight = (Flight*)arg; 
        bool violation[NUM_FLIGHT_PHASES]={false};

        printf("radarr for flight %d\n",flight->id);
        
        while(flight && flight->phase!=FlightPhase::DONE){ //flight ongoing

            FlightPhase phase = flight->phase;
            int phaseIndex = static_cast<int>(phase);
            if (flight->checkViolation() && violation[phaseIndex]==false) {//if first violation of phase

                printf("Phase Index: %d, Violation Flag Before: %d\n", phaseIndex, violation[phaseIndex]);
                violation[phaseIndex]=true;
                printf("Phase Index: %d, Violation Flag AFter %d\n", phaseIndex, violation[phaseIndex]);

                
                //send to AVN generator to generate AVN
                //send id, airline name,airline type, recorded speed, phase
                printf("Request AVN for flight %d\n",flight->id);
                ViolationInfo* violationDetails = new ViolationInfo(flight->id,static_cast<int>(flight->airlineName),static_cast<int>(flight->airlineType),flight->speed,static_cast<int>(flight->phase));
                pthread_mutex_lock(&lock);
                printf("sending violation for flight %d\n",violationDetails->flightID);
                int fd = open(AVN_FIFO1,O_WRONLY,0666);
                write(fd,(void*)violationDetails,sizeof(ViolationInfo));
                close(fd);
                delete violationDetails;
                pthread_mutex_unlock(&lock);
                flight->numViolations++;
                if (flight->numViolations==1)
                    flaggedFlights.push_back(flight);
                displayFlaggedFlights();

                printf("--AVN DELIVERED--\n");

            }
            sleep(1);
            //usleep(1000);
        }
        pthread_exit(nullptr);
    }

    static void* clearFlightViolations(void* arg){
        printf("started----\n");
        while (1){
            int flightID;
            //read cleared avn flight id
            int fd = open(AVN_FIFO2,O_RDONLY);
            read(fd,&flightID,sizeof(flightID));
            close(fd);
            pthread_mutex_lock(&lock);
            printf("read flight %d\n",flightID);
            for (int i=0; i<flaggedFlights.size(); i++){
                if (flaggedFlights[i]->id == flightID ){
                    flaggedFlights[i]->numViolations--;
                    cout << "flight " << flaggedFlights[i]->id << " :" << flaggedFlights[i]->numViolations << " violations\n";
                    if (flaggedFlights[i]->numViolations == 0 ){
                        flaggedFlights.erase(flaggedFlights.begin() + i);
                        displayFlaggedFlights();
                    }
                }
            }
            pthread_mutex_unlock(&lock);
        }
        pthread_exit(NULL);
    }

    static void displayFlaggedFlights(){
        std::ostringstream ss;
        for (int i=0; i<flaggedFlights.size(); i++){
            ss << "> Flight " << flaggedFlights[i]->id << std::endl;
            ss << flaggedFlights[i]->numViolations << " violations\n";
        }
        list.setString(ss.str());
    }
    void drawGraphic(sf::RenderWindow& window){
        window.draw(list);
    }

};
pthread_mutex_t Radar::lock;
vector<Flight*> Radar::flaggedFlights;
sf::Text Radar::list;

#endif