#include <iostream>
#include <string>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <ctime>
#include <chrono>
#include <cmath>

#define NUM_FLIGHT_PHASES 8

#define GRID 2 //6!!
//assume runways start at (0,0), so each quadrant is 3x3 km
#define RUNWAY_LEN GRID/2

pthread_mutex_t flightMutex; 

// ENUMS
enum class AirlineType {
    COMMERCIAL,
    CARGO,
    MILITARY,
    MEDICAL
};

enum class FlightType {
    DOMESTIC_ARRIVAL,
    DOMESTIC_DEPARTURE,
    INTERNATIONAL_ARRIVAL,
    INTERNATIONAL_DEPARTURE
};

enum class FlightPhase {
    GATE,
    TAXI,
    TAKEOFF,
    CLIMB,
    CRUISE,
    HOLDING, 
    APPROACH,
    LANDING
};

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
    flightPhases[(int)FlightPhase::TAKEOFF] = Phase{0, 290, 0, 500};
    flightPhases[(int)FlightPhase::CLIMB] = Phase{250, 463, 500, 10000};
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

// Runway class
class Runway {
    private:
        pthread_mutex_t lock;
    public:
        char runwayID;
    
        Runway(char id = 'A') : runwayID(id) {
            pthread_mutex_init(&lock, nullptr);
        }
    
        ~Runway() {
            pthread_mutex_destroy(&lock);
        }
    
        void acquireRunway() {
            pthread_mutex_lock(&lock);
        }
    
        void releaseRunway() {
            pthread_mutex_unlock(&lock);
        }
};

// Flight structure
class Flight {
public:
    int id;
    FlightType type; //international/domestic arrival/departure
    AirlineType airlineType; //commercial,cargo,med,military etc
    char direction; // N, S, E, W
    float entryTime; 
    FlightPhase phase; //current phase flight is in
    float speed; //current speed
    float altitude; //current altitude
    int priority; //priority of flight for runway access
    float fuelLevel; 
    bool faulty;

    Flight(int id, FlightType flightType, AirlineType airlineType) : id(id), type(flightType), airlineType(airlineType), entryTime(0), faulty(false){
        
        if (type == FlightType::DOMESTIC_ARRIVAL || type == FlightType::INTERNATIONAL_ARRIVAL) {
            phase = FlightPhase::HOLDING; // if its arriving, start phase is holding
            speed = 400 + rand() % 201;  // 400 - 600 random time
            altitude = 6000; 
            fuelLevel = 100; //??
        }
        else {
            phase = FlightPhase::GATE;
            speed = 0;
            altitude = 0;
            fuelLevel = 100; // ??
        }

        if (airlineType == AirlineType::MEDICAL || airlineType == AirlineType::MILITARY){
            priority = 1;
        } 
        else {
            priority =0;
        }

        switch(flightType){
            case FlightType::INTERNATIONAL_ARRIVAL:
                this->direction = 'N';
                break;
            case FlightType::DOMESTIC_ARRIVAL:
                this->direction='S';
                break;
            case FlightType::INTERNATIONAL_DEPARTURE:
                this->direction='E';
                break;
            case FlightType::DOMESTIC_DEPARTURE:
                this->direction='W';
                break;     
        }

    }

    bool checkViolation() {
        Phase p = flightPhases[(int)phase];
        //printPhase(this->phase);
        return (speed < p.speedLowerLimit || speed > p.speedUpperLimit);
    }



    void simulateTick() {

        consumeFuel();
        // randomly make faulty on gate or taxi phase;
        int fault = rand() % 100;
        /*if (fault < 10 && (phase == FlightPhase::GATE || phase == FlightPhase::TAXI)) {
            faulty = true;
            cout << "> Flight " << id << " is faulty on ground. It is being towed and removed.\n";
        }*/
    }

    void consumeFuel() {
        // i have no idea rn
        fuelLevel -= 0.5;
        if (fuelLevel < 0)
            fuelLevel = 0;
    }

    void printStatus() {
        std::cout << "> Flight " << id << " | Speed: " << speed
                << " km/h | Alt: " << altitude
                << " ft | Phase: " << (int)phase
                << " | Fuel: " << fuelLevel
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
Runway rwyA('A');
Runway rwyB('B');
Runway rwyC('C');

Flight* test;

// Simulate flight 
void* simulateFlightDeparture(void* arg) {

    Flight* flight = static_cast<Flight*>(arg);

    //accelerate by 10km/s
    //gate->taxi
    printf("Gate to Taxi\n");
    while(1){
        flight->speed+=5; //acclelerate at 5km/s2
        flight->printStatus();
        if (flight->speed>=15)
            break;
        //sleep(1);
        usleep(1000);
    }
    flight->phase = FlightPhase::TAXI;
    printf("Taxi and start approaching runway\n");
    float distToRunway = GRID/2;
    while (distToRunway>0.5){
        //flight->speed = flight->speed + (1 - rand()%10); //fluctuate ~1
        distToRunway -= flight->speed/3600;
        //printf("Distance to Runway= %f. Speed=%f\n",distToRunway,flight->speed);
        //sleep(1);
        usleep(1000);
    }
    //slow to a stop
    while(flight->speed>=0){
        flight->speed -= 3;
        distToRunway -= flight->speed/3600;
        //sleep(1);
        usleep(1000);
    }
    printf("Stopped at runway dist=%f, prepare to takeoff\n",distToRunway);
    //departures use rwyB
    rwyB.acquireRunway(); //if busy wait
    printf("Runway B freed up, ready for takeoff\n");
    //takeoff
    flight->phase = FlightPhase::TAKEOFF;
    float distAlongRwy = RUNWAY_LEN + distToRunway;
    while (distAlongRwy>flight->speed/3600){
        flight->speed += 10; //7-14
        distAlongRwy -= flight->speed/3600;
        //printf("takeoff, speed=%f, altitude=%f, dist=%f\n",flight->speed,flight->altitude,distAlongRwy);
        //sleep(1);
        usleep(1000);
    }
    printf("Takeoff at dist=%f\n",distAlongRwy);
    //climb
    flight->phase = FlightPhase::CLIMB;
    while (flight->altitude< flightPhases[(int)FlightPhase::CLIMB].altitudeLowerLimit){
        flight->speed+=1.0/3600;
        flight->altitude += flight->speed/3600*sin(15*M_PI/180)* 3.28084;
        //printf("climb, speed=%f, altitude=%f\n",flight->speed,flight->altitude);
        //sleep(1);
        usleep(1000);
    }
    rwyB.releaseRunway();

    //bring to cruise
    printf("Bring to cruise\n");
    while (flight->speed<800){
        flight->speed+=3;
        flight->altitude += flight->speed/3600*sin(15);
        //printf("climb->cruise, speed=%f, altitude=%f\n",flight->speed,flight->altitude);
        //flight->printStatus();
    }
    flight->phase=FlightPhase::CRUISE;
    printf("Cruising\n");

    //exit atomicaly
    pthread_mutex_lock(&flightMutex);
    free(test);
    test=nullptr;
    pthread_mutex_unlock(&flightMutex);
    pthread_exit(nullptr);
}

void* flightRadar(void* arg){
    //int flightID = *(int*)arg; //if stored in arr for now im just using the global var
    bool violation[NUM_FLIGHT_PHASES] = {0}; //no phase violation yet
    while(test){ //flight ongoing
        if (test->checkViolation() && !violation[(int)test->phase]) {
            FlightPhase phase = test->phase;
            int amount=0;

            if(test->airlineType == AirlineType:: COMMERCIAL){
                amount= 500000;
                amount+= 0.15*amount;
            }
            else if (test->airlineType == AirlineType:: CARGO){
                amount= 700000;
                amount+= 0.15*amount;
            }

            AVN avn(test->id, test->speed, amount, phase);
            avn.printAVN();

            violation[(int)test->phase]=true;
        }
        
        test->printStatus();
        sleep(1);
    }
    pthread_exit(nullptr);
}




int main(){
    
    initializeFlightPhases();
    //test with a single flight for now
    test = new Flight(1,FlightType::DOMESTIC_DEPARTURE,AirlineType::COMMERCIAL);
    //start flight simulation
    // for now start the first flight
    pthread_t tid;
    pthread_create(&tid,nullptr,simulateFlightDeparture,(void*)test); //create flight thread

    //and create a radar to monitor it
    pthread_t radar;
    pthread_create(&radar,nullptr,flightRadar,nullptr);

    pthread_join(tid,NULL);
    pthread_join(radar,NULL);


}