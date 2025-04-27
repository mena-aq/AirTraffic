#include <iostream>
#include <string>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <ctime>
#include <chrono>
#include <cmath>
using namespace std;

// ATC SYSTEM CONSTANTS 
#define NUM_FLIGHT_PHASES 8
#define NUM_AIRLINE 6
#define TOTAL_FLIGHTS 14
#define GRID 6 
#define SIMULATION_DURATION 300
//assume runways start at (0,0), so each quadrant is 3x3 km
#define RUNWAY_LEN GRID/2
#define TERMINAL_TO_RUNWAY_LEN 0.5

//MUTEXES
pthread_mutex_t flightMutex; 

// ENUMS
enum AirlineType {
    COMMERCIAL,
    CARGO,
    MILITARY,
    MEDICAL
};

enum AirlineName {
    PIA,
    AirBlue,
    FedEx,
    Pakistan_Airforce,
    Blue_Dart,
    AghaKhan_Air_Ambulance
};

enum FlightType {
    DOMESTIC_ARRIVAL,
    DOMESTIC_DEPARTURE,
    INTERNATIONAL_ARRIVAL,
    INTERNATIONAL_DEPARTURE
};

enum FlightPhase {
    GATE,
    TAXI,
    TAKEOFF,
    CLIMB,
    CRUISE,
    HOLDING, 
    APPROACH,
    LANDING
};


// CLASSES
// Phase configuration class
class Phase {
    public:
        float speedLowerLimit;
        float speedUpperLimit;
        float altitudeLowerLimit;
        float altitudeUpperLimit;
    
        //default constructor
        Phase() : speedLowerLimit(0.0), speedUpperLimit(0.0), altitudeLowerLimit(0.0), altitudeUpperLimit(0.0) {}
        //parameterised constructor
        Phase(float sl, float su, float al, float au)
            : speedLowerLimit(sl), speedUpperLimit(su), altitudeLowerLimit(al), altitudeUpperLimit(au) {}
};


// Initialize 8 flight phases globally
Phase flightPhases[NUM_FLIGHT_PHASES];
void initializeFlightPhases() {
    flightPhases[(int)FlightPhase::GATE] = Phase{0, 10, 0, 0};
    flightPhases[(int)FlightPhase::TAXI] = Phase{15, 30, 0, 0};
    flightPhases[(int)FlightPhase::TAKEOFF] = Phase{0, 290, 0, 500};                            // CHANGED
    flightPhases[(int)FlightPhase::CLIMB] = Phase{250, 463, 0, 10000};
    flightPhases[(int)FlightPhase::CRUISE] = Phase{800, 900, 10000, 14000};
    flightPhases[(int)FlightPhase::HOLDING] = Phase{400, 600, 6000, 14000};
    flightPhases[(int)FlightPhase::APPROACH] = Phase{240, 290, 800, 6000};
    flightPhases[(int)FlightPhase::LANDING] = Phase{30, 240, 0, 800};
}
void printPhase(FlightPhase phase) {
    switch (phase) {
        case FlightPhase::GATE: std::cout << "GATE"; break;
        case FlightPhase::TAXI: std::cout << "TAXI"; break;
        case FlightPhase::TAKEOFF: std::cout << "TAKEOFF"; break;
        case FlightPhase::CLIMB: std::cout << "CLIMB"; break;
        case FlightPhase::CRUISE: std::cout << "CRUISE"; break;
        case FlightPhase::HOLDING: std::cout << "HOLDING"; break;
        case FlightPhase::APPROACH: std::cout << "APPROACH"; break;
        case FlightPhase::LANDING: std::cout << "LANDING"; break;
    }
}
void printAirlineName(AirlineName airlineName) {
    switch (airlineName) {
        case PIA: std::cout << "PIA"; break;
        case AirBlue: std::cout << "AirBlue"; break;
        case FedEx: std::cout << "FedEx"; break;
        case Pakistan_Airforce: std::cout << "Pakistan Airforce"; break;
        case Blue_Dart: std::cout << "Blue Dart"; break;
        case AghaKhan_Air_Ambulance: std::cout << "AghaKhan Air Ambulance"; break;
    }
}
void printAirlineType(AirlineType airlineytype) {
    switch (airlineytype) {
        case COMMERCIAL: std::cout << "COMMERCIAL"; break;
        case CARGO: std::cout << "CARGO"; break;
        case MEDICAL: std::cout << "MEDICAL"; break;
        case MILITARY: std::cout << "MILITARY"; break;
    }
}


// Runway class
class Runway {
    private:
        pthread_mutex_t lock;
        pthread_cond_t cond;
    public:
        char runwayID;
        char priorityDirection;
        int waitingLowPriority;
        int waitingHighPriority;
        bool busy;

        Runway(char id,char priorityDirection) : runwayID(id), priorityDirection(priorityDirection) {
            pthread_mutex_init(&lock, nullptr);
            busy=false;
        }
    
        ~Runway() {
            pthread_mutex_destroy(&lock);
        }
    
        void acquireRunway(char direction) {
            pthread_mutex_lock(&lock);
        
            bool highPriority = (direction == 'N' || direction == 'W' || runwayID=='C');
            
            // Update waiting counters
            if (highPriority) {
                waitingHighPriority++;
            } else {
                waitingLowPriority++;
            }
            
            // Wait if:
            // 1. Runway is already in use, OR
            // 2. This is a low priority thread and there are high priority threads waiting
            while (busy || (!highPriority && waitingHighPriority > 0)) {
                pthread_cond_wait(&cond, &lock);
            }
            
            busy = true;  

            if (highPriority) {
                waitingHighPriority--;
            } else {
                waitingLowPriority--;
            }
            
            pthread_mutex_unlock(&lock);
        }
    
        void releaseRunway() {
            pthread_mutex_unlock(&lock);
            busy = false;
            pthread_cond_broadcast(&cond);
            //signal
        }
};

// Flight structure
class Flight {
public:
    int id;
    FlightType type; //international/domestic arrival/departure
    AirlineType airlineType; //commercial,cargo,med,military etc
    AirlineName airlineName; // PIA etc
    char direction; // N, S, E, W
    float entryTime; 
    FlightPhase phase; //current phase flight is in
    float speed; //current speed
    float altitude; //current altitude
    int priority; //priority of flight for runway access
    float fuelLevel; //in gallons
    bool faulty;

    Flight(int id, FlightType flightType, AirlineType airlineType, AirlineName name) : airlineName(name), id(id), type(flightType), airlineType(airlineType), entryTime(0), faulty(false){    
        if (type == FlightType::DOMESTIC_ARRIVAL || type == FlightType::INTERNATIONAL_ARRIVAL) {
            phase = FlightPhase::HOLDING; // if its arriving, start phase is holding
            speed = 400 + rand() % 201;  // 400 - 600 random time
            altitude = 6000; 
            fuelLevel = 500; //assuming 1 gallon/s
        }
        else {
            phase = FlightPhase::GATE;
            speed = 0;
            altitude = 0;
            fuelLevel = 1000; 
        }

        if (airlineType == AirlineType::MEDICAL || airlineType == AirlineType::MILITARY){
            priority = 1;
        } 
        else {
            priority = 0;
        }

        switch(flightType){
            case FlightType::INTERNATIONAL_ARRIVAL:
                this->direction = 'N';
                this->priority += 10;
                break;
            case FlightType::DOMESTIC_ARRIVAL:
                this->direction='S';
                this->priority += 5;
                break;
            case FlightType::INTERNATIONAL_DEPARTURE:
                this->direction='E';
                this->priority += 15;
                break;
            case FlightType::DOMESTIC_DEPARTURE:
                this->direction='W';
                this->priority += 20;
                break;     
        }

    }
    bool checkViolation() {
        Phase p = flightPhases[(int)phase];
        //printPhase(this->phase);
        return (speed < p.speedLowerLimit || speed > p.speedUpperLimit);
    }

    bool checkFaults() {
        // randomly make faulty on gate or taxi phase;
        int fault = rand() % 100;
        if (fault == 0 && (phase == FlightPhase::GATE || phase == FlightPhase::TAXI)) {
            this->faulty = true;
            std::cout << "> Flight " << id << " is faulty on ground. It is being towed and removed.\n";    
        }
        return this->faulty;
    }

    void consumeFuel() {
        fuelLevel -= 1;
        if (fuelLevel < 0)
            fuelLevel = 0;
    }

    void printStatus() {
        std::cout << "> Flight " << id;
        std::cout << " | Airline: ";
        printAirlineName(airlineName);
        std::cout << " | Airline Type: ";
        printAirlineType(airlineType);
               cout << " | " << speed << " km/h | Alt: " << altitude
                << " ft | Phase: ";
        printPhase(phase);
        std::cout << " | Fuel: " << fuelLevel
                << " | Priority: " << priority << "\n";
    }
};

// AVN (Violation Notice)
class AVN {
public:
    int flightID;
    float amountDue;
    bool status;  // true= paid, false= unpaid
    float speedRecorded;
    FlightPhase phaseViolation;
    time_t violationTimestamp;
    time_t dueDate;

    AVN(int id, float sp, float amount, FlightPhase phase)
        : flightID(id), amountDue(amount), status(false), speedRecorded(sp), phaseViolation(phase)
    {
        violationTimestamp = std::time(nullptr); //save current time
        auto currentTime = std::chrono::system_clock::now();
        auto futureTime = currentTime + std::chrono::hours(24 * 3); // add 3 days
        dueDate = std::chrono::system_clock::to_time_t(futureTime);
    }

    void printAVN() {
        std::cout<<"\n> AVN CHALLAN\n";
        std::cout << "> AVN Issued: Flight " << flightID<<std::endl;
        std::cout << "> Speed: " << speedRecorded<<" km/h\n";
        std::cout << "> Phase: ";
        printPhase(phaseViolation);
        std::cout << "\n> Due: " << ctime(&dueDate)<<std::endl;
    }
};


//make the 3 runways
Runway rwyA('A','N'); //north>south
Runway rwyB('B','W'); //west>east
Runway rwyC('C','\0');

class Radar{
public:
//radar thread function
    static void* flightRadar(void* arg){
        Flight* flight = (Flight*)arg; //if stored in arr for now im just using the global var
        bool violation[NUM_FLIGHT_PHASES] = {0}; //no phase violation yet
        while(flight){ //flight ongoing
            if (flight->checkViolation() && !violation[(int)flight->phase]) {
                FlightPhase phase = flight->phase;
                int amount=0;

                if(flight->airlineType == AirlineType:: COMMERCIAL){
                    amount= 500000;
                    amount+= 0.15*amount;
                }
                else if (flight->airlineType == AirlineType:: CARGO){
                    amount= 700000;
                    amount+= 0.15*amount;
                }

                AVN avn(flight->id, flight->speed, amount, phase);
                avn.printAVN();

                violation[(int)flight->phase]=true;
            }
            
            //flight->printStatus();
            sleep(1);
        }
        pthread_exit(nullptr);
    }
};

void* simulateFlightDeparture(void* arg) {                                        //CHANGED

    Flight* flight = static_cast<Flight*>(arg);
    //printf("--DEPARTURE--\n");
    //printf("flight details:");
    //printf("flight ID: %d\n",flight->id);

    if (!flight->checkFaults()){ //if no faults, plane can fly

        //acquire runway
        if(flight->airlineType==CARGO){
            rwyC.acquireRunway(flight->direction);
        }
        else
            rwyB.acquireRunway(flight->direction); //if busy wait
        printf("Runway B acquired by flight ID: %d\n",flight->id);

        // ------------- SIMULATE --------------

        // GATE->TAXI
        float distanceToRunway = TERMINAL_TO_RUNWAY_LEN;
        printf("Gate to Taxi\n");
        while(1){
            flight->speed += 10800/3600.0; //v=u+at
            distanceToRunway -= flight->speed/3600.0 + 0.5*10800*pow((1.0/3600),2); //ut+1/2at^2
            printf("Distance to Runway= %f. Speed=%f\n",distanceToRunway,flight->speed);
            //flight->printStatus();
            if (flight->speed>=15)
                break;
            //sleep(1);
            usleep(1000);
            flight->consumeFuel();
        }
        flight->phase = FlightPhase::TAXI;
        // TAXI
        printf("Taxi and start approaching runway\n");
        while (distanceToRunway>0.0104){
            //flight->speed += (rand()%10)/10.0; //fluctuate ~0.1
            distanceToRunway -= flight->speed/3600; //s=vt
            printf("Distance to Runway= %f. Speed=%f\n",distanceToRunway,flight->speed);
            //sleep(1);
            usleep(1000);
            flight->consumeFuel();
        }
        // TAXI->STOP 
        while(flight->speed>=0){
            flight->speed -= 10800/3600.0;
            distanceToRunway -= flight->speed/3600.0 + 0.5*10800*pow((1.0/3600),2); //ut+1/2at^2
            printf("Distance to Runway= %f. Speed=%f\n",distanceToRunway,flight->speed);
            //sleep(1);
            usleep(1000);
            flight->consumeFuel();
        }
        printf("Stopped at runway dist=%f, prepare to takeoff\n",distanceToRunway); //ye submit krna h??? its wirtten in milestone 2 and class to nhi h 
        // TAKEOFF
        flight->phase = FlightPhase::TAKEOFF;
        float distanceAlongRunway = RUNWAY_LEN + distanceToRunway;
        while (flight->speed<250){
            flight->speed += 12000.0/3600; //v=u+at
            distanceAlongRunway -= flight->speed/3600.0 + 0.5*12000*pow((1.0/3600),2); //ut+1/2at^2
            printf("takeoff, speed=%f, altitude=%f, dist=%f\n",flight->speed,flight->altitude,distanceAlongRunway);
            //sleep(1);
            usleep(1000);
            flight->consumeFuel();
        }
        printf("Takeoff at dist remaining=%f\n",distanceAlongRunway);
        // CLIMB
        flight->phase = FlightPhase::CLIMB;
        while (flight->altitude < flightPhases[(int)FlightPhase::CRUISE].altitudeLowerLimit){
            flight->speed += 1637*(1.0/3600); //v=u+at
            flight->altitude += 3280*(flight->speed*sin(M_PI/12)*(1.0/3600) + 0.5*1637*pow((1.0/3600),2));//ut+1/2at^2
            printf("climb, speed=%f, altitude=%f\n",flight->speed,flight->altitude);
            //sleep(1);
            usleep(1000);
            flight->consumeFuel();
        }
        //CLIMB->CRUISE
        printf("Bring to cruise\n");
        while (flight->speed<800){
            flight->speed += 63568*(1.0/3600); //v=u+at
            flight->altitude += 3280 *(flight->speed*sin(M_PI/12)*(1.0/3600) + 0.5*63568*pow((1.0/3600),2));//ut+1/2at^2
            usleep(1000);
            flight->consumeFuel();
            printf("climb->cruise, speed=%f, altitude=%f\n",flight->speed,flight->altitude);
            //flight->printStatus();
        }
        flight->phase=FlightPhase::CRUISE;
        printf("Cruising\n");
        //release runway
        if(flight->airlineType==CARGO){
            rwyC.releaseRunway();
        }
        else{
            rwyB.releaseRunway();
        }
        printf("Runway B released by flight ID: %d\n",flight->id);
    }

    //exit atomicaly
    pthread_mutex_lock(&flightMutex);
    free(flight);
    pthread_mutex_unlock(&flightMutex);
    
    pthread_exit(nullptr);
}


void* simulateFlightArrival(void* arg){                                         //CHANGED

    Flight* flight = static_cast<Flight*>(arg);
    //printf("--ARRIVAL--\n");
    //printf("flight details:");
    //printf("flight ID: %d\n",flight->id);

    if (!flight->checkFaults()){       
        //acquire runway
        if(flight->airlineType==CARGO){
            rwyC.acquireRunway(flight->direction);
        }
        else
            rwyA.acquireRunway(flight->direction); //if busy wait
        printf("Runway A acquired by flight ID: %d\n",flight->id);

        // ------------- SIMULATE --------------

        float distanceToRunway = 10.6; //for a safe-ish descent (50ft/s)
        //HOLDING->APPROACH
        while(flight->speed>290){
            flight->speed += -4805*(1.0/3600); //v=u+at
            distanceToRunway -= flight->speed/3600.0 + 0.5*-4805*pow((1.0/3600),2); //ut+1/2at^2
            flight->altitude -= 50; //safe (if 1km away its 107ft/s which would kill everyone i fear)
            printf("holding->approach, speed=%f, altitude=%f, dist=%f\n",flight->speed,flight->altitude,distanceToRunway);
            //sleep(1);
            usleep(1000);
        }
        //APPROACH
        flight->phase = FlightPhase::APPROACH;
        while(flight->speed>240){
            flight->speed += -12500*(1.0/3600); //v=u+at
            distanceToRunway -= flight->speed/3600.0 + 0.5*-12500*pow((1.0/3600),2); //ut+1/2at^2
            flight->altitude -= 70;   
            printf("approach, speed=%f, altitude=%f, dist=%f\n",flight->speed,flight->altitude,distanceToRunway);
            //sleep(1);
            usleep(1000);
        }
        //LANDING
        flight->phase = FlightPhase::LANDING;
        float distanceAlongRunway = distanceToRunway+RUNWAY_LEN; 
        while (flight->speed>30 || flight->altitude>0){
            flight->speed += -9450*(1.0/3600); //v=u+at
            distanceAlongRunway -= flight->speed/3600.0 + 0.5*-9450*pow((1.0/3600),2); //ut+1/2at^2
            if (flight->altitude>0){
                flight->altitude -= 60;
                if (flight->altitude<0)
                    flight->altitude=0;
            }
            printf("landing, speed=%f, altitude=%f, dist=%f\n",flight->speed,flight->altitude,distanceAlongRunway);
            sleep(1);
            usleep(1000);
        }
        //TAXI
        flight->phase = FlightPhase::TAXI;
        float distanceToGate = distanceAlongRunway+TERMINAL_TO_RUNWAY_LEN;
        while (distanceToGate>0.1){
            if (flight->speed>20){
                flight->speed += -844*(1.0/3600); //v=u+at
            }
            distanceToGate -= flight->speed/3600.0 + 0.5*-844*pow((1.0/3600),2); //ut+1/2at^2
            printf("taxi, speed=%f, dist=%f\n",flight->speed,distanceToGate);
            //sleep(1);
            usleep(1000);
        }
        //TAXI->GATE
        while(distanceToGate>0.01 || flight->speed>0){
            flight->speed += -1125.75*(1.0/3600); //v=u+at
            distanceToGate -= flight->speed/3600.0 + 0.5*-1125*pow((1.0/3600),2); //ut+1/2at^2
            printf("taxi->gate, speed=%f, dist=%f\n",flight->speed,distanceToGate);
            sleep(1);
            usleep(1000);
        }
        flight->phase = FlightPhase::GATE;
            
        if(flight->airlineType==CARGO){
            rwyC.releaseRunway();
        }
        else
            rwyA.releaseRunway();

        printf("Runway A released by flight ID: %d\n",flight->id);
    }

    //exit atomicaly
    pthread_mutex_lock(&flightMutex);
    free(flight);
    pthread_mutex_unlock(&flightMutex);
        
    pthread_exit(nullptr);
}

class Timer{
public:
    static int currentTime;

    static void* simulationTimer(void* arg) {
        time_t start = time(nullptr);

        while ( difftime(time(nullptr), start) < SIMULATION_DURATION) {
            currentTime = difftime(time(nullptr), start);
            std::cout << "\rSimulation Time: " << currentTime << "s / " << SIMULATION_DURATION << "s \n" << flush;
            sleep(1);
        }
        pthread_exit(nullptr);
    }
    int getCurrentTime(){
        return currentTime;
    }

};
int Timer::currentTime = 0;



int main(){
    
    initializeFlightPhases();
    pthread_t tid;

    Flight* flight = new Flight(0,FlightType::DOMESTIC_ARRIVAL,AirlineType::COMMERCIAL,AirlineName::AirBlue);
    pthread_create(&tid,NULL,simulateFlightArrival,(void*)flight);
    pthread_join(tid,NULL);
    pthread_exit(NULL);


}