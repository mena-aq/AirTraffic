#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <ctime>
#include <chrono>
#include <cmath>
#include <algorithm>
using namespace std;

// ATC SYSTEM CONSTANTS 
#define NUM_FLIGHT_PHASES 8
#define NUM_AIRLINE 6
#define TOTAL_FLIGHTS 9
#define GRID 6 
#define SIMULATION_DURATION 300
//assume runways start at (0,0), so each quadrant is 3x3 km
#define RUNWAY_LEN GRID/2
#define TERMINAL_TO_RUNWAY_LEN 0.5

//MUTEXES
pthread_mutex_t flightMutex; 

// ENUMS
enum AirlineType {
    //highest priority
    MEDICAL, //EMERGENCY??
    MILITARY, //VIP??
    CARGO,
    COMMERCIAL
    //lowest priority
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
    flightPhases[(int)FlightPhase::TAKEOFF] = Phase{0, 290, 0, 500};                            
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

// Flight structure
class Flight {
public:
    int id;
    pthread_t thread_id;
    pthread_t radar_id;

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

    Flight(int id, FlightType flightType, AirlineType airlineType, AirlineName name) : airlineName(name), id(id), thread_id(-1), radar_id(-1), type(flightType), airlineType(airlineType), entryTime(0), faulty(false){    
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

    void printStatus() const{
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

    /*
    pthread_mutex_t threadMutex = PTHREAD_MUTEX_INITIALIZER;
    void rerouteFlight() {
        pthread_mutex_lock(&threadMutex);
        if (this->thread_id != -1) {
            int cancelResult = pthread_cancel(this->thread_id);
            if (cancelResult == 0) {
                pthread_join(this->thread_id, nullptr);
                printf("Re-routing flight %d\n", this->id);
            } else {
                printf("Failed to cancel thread for flight %d\n", this->id);
            }
            this->thread_id = -1;
        } else {
            printf("Thread for flight %d is not active or already joined.\n", this->id);
        }
        pthread_mutex_unlock(&threadMutex);
    }
    */

    //ADDED
    void rerouteFlight(){
        pthread_cancel(this->thread_id);
        pthread_join(this->thread_id, nullptr);
        this->thread_id = -1;
        printf("Re-routing flight %d\n", this->id);
    }

    bool isArrival() const{
        return (this->type == FlightType::DOMESTIC_ARRIVAL || this->type == FlightType::INTERNATIONAL_ARRIVAL);
    }
    bool isDeparture() const{
        return (this->type == FlightType::DOMESTIC_DEPARTURE|| this->type == FlightType::INTERNATIONAL_DEPARTURE);
    }

    //operator for priority 
    bool operator<(const Flight& other) const{
        if (this->airlineType != other.airlineType)
            return this->airlineType < other.airlineType; // smaller airlineType first
        if (this->fuelLevel != other.fuelLevel)
            return this->fuelLevel < other.fuelLevel;     // smaller fuel first
        //directions priority (emergency %)
        if (this->isArrival() &&  other.isArrival()){
            //north over south
            return (this->direction!=other.direction && this->direction=='N' && other.direction=='S');
        } 
        else if (this->isDeparture() && other.isDeparture()){ //departure
            //west over east
            return (this->direction!=other.direction && this->direction=='W' && other.direction=='E');
        }
        return this->id < other.id;                       // earlier arrival first (smaller id)       
    }

};

/*
struct Comparator {
    bool operator()(const Flight& a, const Flight& b) const {
        if (a.airlineType != b.airlineType)
            return a.airlineType < b.airlineType; // smaller airlineType first
        if (a.fuelLevel != b.fuelLevel)
            return a.fuelLevel < b.fuelLevel;     // smaller fuel first
        return a.id < b.id;                       // earlier arrival first (smaller id)
    }
};
*/

class QueueFlights {
public:
    vector<Flight*> incomingQueue;
    vector<Flight*> outgoingQueue;

    // Helper to sort a vector based on Comparator
    void sortQueue(std::vector<Flight*>& flights) {
        //std::sort(flights.begin(), flights.end(), Comparator());
        std::sort(flights.begin(), flights.end());
    }
    void addFlight(Flight*& flight) {
        if (flight->type == FlightType::INTERNATIONAL_ARRIVAL || flight->type == FlightType::DOMESTIC_ARRIVAL) {
            incomingQueue.push_back(flight);
            sortQueue(incomingQueue); // Keep it sorted after every insertion
        } else {
            outgoingQueue.push_back(flight);
            //cout<<"Flight: "<<flight->id<<"entered in out\n";
            sortQueue(outgoingQueue);
        }
    }

    /*void processArrivals() {
        while (!incomingQueue.empty()) {
            Flight currentFlight = incomingQueue.front();
            incomingQueue.erase(incomingQueue.begin());

            pthread_t thread;
            pthread_create(&thread, nullptr, simulateFlightArrival, (void*)&currentFlight);
            pthread_join(thread, nullptr);
        }
    }

    void processDepartures() {
        while (!outgoingQueue.empty()) {
            Flight currentFlight = outgoingQueue.front();
            outgoingQueue.erase(outgoingQueue.begin());

            pthread_t thread;
            pthread_create(&thread, nullptr, simulateFlightDeparture, (void*)&currentFlight);
            pthread_join(thread, nullptr);
        }
    }*/

    void printQueues() {
        std::cout << "\n--- Incoming Flights Queue ---\n";
        for (const auto& flight : incomingQueue) {
            flight->printStatus();
        }

        std::cout << "\n--- Outgoing Flights Queue ---\n";
        for (const auto& flight : outgoingQueue) {
            flight->printStatus();
        }
    }
};

struct args{
    Flight* flight;
    QueueFlights* qf;
};

class Runway {
private:
    pthread_mutex_t lock;
    pthread_cond_t cond;
public:
    char runwayID;
    char priorityDirection;
    //int waitingLowPriority;
    //int waitingHighPriority;
    //bool busy;
    Flight* currentFlight;

    Runway(char id, char priorityDirection) : runwayID(id), priorityDirection(priorityDirection) {
        pthread_mutex_init(&lock, nullptr);
        pthread_cond_init(&cond, nullptr);
        //busy = false;
        currentFlight = nullptr;
    }

    ~Runway() {
        pthread_mutex_destroy(&lock);
        pthread_cond_destroy(&cond);
    }

    void preemptRunway(Flight* flight,QueueFlights*& qf){
        //send current back to queue, cancel thread and acquire runway
        pthread_mutex_unlock(&lock);
        currentFlight->rerouteFlight();
        //terminateRadar() 
        qf->addFlight(currentFlight);
        printf("Flight %d pre-empt runway from flight %d\n",flight->id,currentFlight->id);
        pthread_mutex_lock(&lock);
        currentFlight = flight;
        return;
    }

    void acquireRunway(Flight* flight,QueueFlights*& qf){//for now its just for arrival
        printf("Flight %d wants runway access\n",flight->id);
        if (currentFlight==nullptr){ //if available, acquire
            pthread_mutex_lock(&lock);
            currentFlight = flight;
            printf("Flight %d acquire runway %c\n",flight->id, runwayID);
            return;
        }
        else{
            printf("can i pre-empt?\n");
            //compare priorities and if pre-emptable phase
            if (flight->isArrival() && currentFlight->isArrival()){ //arrivals 
                if (*flight<*currentFlight && currentFlight->phase<FlightPhase::LANDING){//higher priority and not on ground
                    preemptRunway(flight,qf);
                    return;
                }
            }
            else if (flight->isDeparture() && currentFlight->isDeparture()){//departures
                if (*flight<*currentFlight && currentFlight->phase<FlightPhase::TAKEOFF){//higher priority and not on ground
                    preemptRunway(flight,qf);
                    return;
                }
            }
        }
        //cannot acquire,wait
        printf("Flight %d waiting for runway\n",flight->id);
        pthread_mutex_lock(&lock);
        currentFlight = flight;
        printf("Flight %d acquire runway %c\n",flight->id, this->runwayID);
        return;
    }


    void releaseRunway(){
        printf("Flight %d released runway %c\n",currentFlight->id, this->runwayID);
        pthread_mutex_unlock(&lock);
        currentFlight = nullptr;
    }

    /*
    void acquireRunway(Flight* flight, QueueFlights*& qf) {
        
        pthread_mutex_lock(&lock);

        // If the runway is free, acquire it
        if (!busy) {
            currentFlight = flight;
            busy = true;
            std::cout << "Runway " << runwayID << " acquired by Flight ID: " << flight->id << "\n";
        }
        else {
            std::cout << "INSIDE CHECK\n";
            flight->printStatus();
            currentFlight->printStatus();

            // Preempt the current flight if the new flight has higher priority
            if (flight->airlineType < currentFlight->airlineType || flight->fuelLevel < currentFlight->fuelLevel || flight->id < currentFlight->id) {

                std::cout << "Runway " << runwayID << " preempted! Flight ID "
                        << currentFlight->id << " -> Flight ID " << flight->id << "\n";
                
                // Add the preempted flight back into the queue and block it
                qf->addFlight(*currentFlight);
                pthread_cond_broadcast(&cond);

                // Set the current flight to the new one and signal that it can proceed
                currentFlight = flight;
                busy = true;
            } else {
                // If the current flight can't be preempted, wait until it's done
                std::cout << "Flight ID: " << flight->id << " waiting for runway " << runwayID << "\n";
                while (busy) {
                    pthread_cond_wait(&cond, &lock);
                }

                // After being signaled, try to acquire the runway
                currentFlight = flight;
                busy = true;
                std::cout << "Runway " << runwayID << " acquired after wait by Flight ID: " << flight->id << "\n";
            }
        }

        pthread_mutex_unlock(&lock);
    }
    */
    /*
    void releaseRunway() {
        pthread_mutex_lock(&lock);

        // Release the runway and allow other waiting flights to proceed
        std::cout << "Runway " << runwayID << " released by Flight ID: " 
                    << (currentFlight ? currentFlight->id : -1) << "\n";
        
        busy = false;
        currentFlight = nullptr;

        // Wake up all waiting flights, because the runway is now available
        pthread_cond_broadcast(&cond);

        pthread_mutex_unlock(&lock);
    }*/
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
class ATCDashboard {
    public:
        int numViolations;
        std::vector<std::vector<int>> avns;
    
        ATCDashboard(){
            this->numViolations=0;
        }
    
        void addViolation() {
            //For milestone 2/3 graphics
            // Simulate FIFO/pipe listening
            // Add AVN violation
        }
};

//make the 3 runways
Runway rwyA('A','N'); //north>south
Runway rwyB('B','W'); //west>east
Runway rwyC('C','\0');

void* simulateFlightDeparture(void* arg) {                                        

    args* argument = static_cast<args*>(arg);
    Flight* flight= argument->flight;
    QueueFlights* qf= argument->qf;

    //printf("--DEPARTURE--\n");
    //printf("flight details:");
    //printf("flight ID: %d\n",flight->id);

    if (!flight->checkFaults()){ //if no faults, plane can fly

        //acquire runway
        if(flight->airlineType==CARGO){
            rwyC.acquireRunway(flight, qf);
        }
        else
            rwyB.acquireRunway(flight, qf); //if busy wait

        // ------------- SIMULATE --------------

        // GATE->TAXI
        float distanceToRunway = TERMINAL_TO_RUNWAY_LEN;
        printf("Flight : %d Gate to Taxi\n", flight->id);
        while(1){
            flight->speed += 10800/3600.0; //v=u+at
            distanceToRunway -= flight->speed/3600.0 + 0.5*10800*pow((1.0/3600),2); //ut+1/2at^2
            printf("Flight : %d Distance to Runway= %f. Speed=%f\n",flight->id, distanceToRunway,flight->speed);
            //flight->printStatus();
            if (flight->speed>=15)
                break;
            //sleep(1);
            usleep(1000);
            flight->consumeFuel();
        }
        flight->phase = FlightPhase::TAXI;
        // TAXI
        printf("Flight : %d Taxi and start approaching runway\n", flight->id);
        while (distanceToRunway>0.0104){
            //flight->speed += (rand()%10)/10.0; //fluctuate ~0.1
            distanceToRunway -= flight->speed/3600; //s=vt
            printf("Flight : %d Distance to Runway= %f. Speed=%f\n", flight->id, distanceToRunway,flight->speed);
            //sleep(1);
            usleep(1000);
            flight->consumeFuel();
        }
        // TAXI->STOP 
        while(flight->speed>=0){
            flight->speed -= 10800/3600.0;
            distanceToRunway -= flight->speed/3600.0 + 0.5*10800*pow((1.0/3600),2); //ut+1/2at^2
            printf("Flight : %d Distance to Runway= %f. Speed=%f\n",flight->id,distanceToRunway, flight->speed);
            //sleep(1);
            usleep(1000);
            flight->consumeFuel();
        }
        printf("Flight : %d Stopped at runway dist=%f, prepare to takeoff\n",flight->id, distanceToRunway); //ye submit krna h??? its wirtten in milestone 2 and class to nhi h 
        // TAKEOFF
        flight->phase = FlightPhase::TAKEOFF;
        float distanceAlongRunway = RUNWAY_LEN + distanceToRunway;
        while (flight->speed<250){
            flight->speed += 12000.0/3600; //v=u+at
            distanceAlongRunway -= flight->speed/3600.0 + 0.5*12000*pow((1.0/3600),2); //ut+1/2at^2
            printf("Flight : %d takeoff, speed=%f, altitude=%f, dist=%f\n",flight->id, flight->speed,flight->altitude,distanceAlongRunway);
            //sleep(1);
            usleep(1000);
            flight->consumeFuel();
        }
        printf("Flight : %d Takeoff at dist remaining=%f\n", flight->id, distanceAlongRunway);
        // CLIMB
        flight->phase = FlightPhase::CLIMB;
        while (flight->altitude < flightPhases[(int)FlightPhase::CRUISE].altitudeLowerLimit){
            flight->speed += 1637*(1.0/3600); //v=u+at
            flight->altitude += 3280*(flight->speed*sin(M_PI/12)*(1.0/3600) + 0.5*1637*pow((1.0/3600),2));//ut+1/2at^2
            printf("Flight : %d climb, speed=%f, altitude=%f\n", flight->id,flight->speed,flight->altitude);
            //sleep(1);
            usleep(1000);
            flight->consumeFuel();
        }
        //CLIMB->CRUISE
        printf("Flight : %d Bring to cruise\n", flight->id);
        while (flight->speed<800){
            flight->speed += 63568*(1.0/3600); //v=u+at
            flight->altitude += 3280 *(flight->speed*sin(M_PI/12)*(1.0/3600) + 0.5*63568*pow((1.0/3600),2));//ut+1/2at^2
            usleep(1000);
            flight->consumeFuel();
            printf("Flight : %d climb->cruise, speed=%f, altitude=%f\n", flight->id, flight->speed,flight->altitude);
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
    }

    //exit atomicaly
    pthread_mutex_lock(&flightMutex);
    //free(flight);
    pthread_mutex_unlock(&flightMutex);
    
    pthread_exit(nullptr);
}

void* simulateFlightArrival(void* arg){  
    
    //pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    args* argument = static_cast<args*>(arg);
    Flight* flight= argument->flight;
    QueueFlights* qf= argument->qf;

    cout<<"Entered\n";
    printf("Thread id = %ld\n",flight->thread_id);
    flight->printStatus();
    //printf("--ARRIVAL--\n");
    //printf("flight details:");
    //printf("flight ID: %d\n",flight->id);

    if (!flight->checkFaults()){       
        //acquire runway
        if(flight->airlineType==CARGO){
            rwyC.acquireRunway(flight, qf);
        }
        else{
            rwyA.acquireRunway(flight, qf); //if busy wait
        }

        // ------------- SIMULATE --------------

        float distanceToRunway = 20; //for a safe-ish descent (50ft/s)
        //HOLDING->APPROACH
        while(flight->speed>290){
            flight->speed += -4805*(1.0/3600); //v=u+at
            distanceToRunway -= flight->speed/3600.0 + 0.5*-4805*pow((1.0/3600),2); //ut+1/2at^2
            flight->altitude -= 30; //safe (if 1km away its 107ft/s which would kill everyone i fear)
            flight->consumeFuel();
            printf("flight %d : holding->approach, speed=%f, altitude=%f, dist=%f\n", flight->id, flight->speed,flight->altitude,distanceToRunway);
            //sleep(1);
            pthread_testcancel(); // safe cancellation point
            usleep(1000);
        }
        //APPROACH
        flight->phase = FlightPhase::APPROACH;
        while(flight->speed>240){
            flight->speed += -12500*(1.0/3600); //v=u+at
            distanceToRunway -= flight->speed/3600.0 + 0.5*-12500*pow((1.0/3600),2); //ut+1/2at^2
            flight->altitude -= 70;   
            flight->consumeFuel();
            printf("flight %d : approach, speed=%f, altitude=%f, dist=%f\n",flight->id, flight->speed,flight->altitude,distanceToRunway);
            //sleep(1);
            pthread_testcancel(); // safe cancellation point
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
            flight->consumeFuel();
            printf("flight %d : landing, speed=%f, altitude=%f, dist=%f\n",flight->id, flight->speed,flight->altitude,distanceAlongRunway);
            //sleep(1);
            usleep(1000);
        }
        //TAXI
        flight->phase = FlightPhase::TAXI;
        float distanceToGate = distanceAlongRunway+TERMINAL_TO_RUNWAY_LEN;
        while (distanceToGate>0.1){
            if (flight->speed>15){
                flight->speed += -1000*(1.0/3600); //v=u+at
            }
            distanceToGate -= flight->speed/3600.0 + 0.5*-1000*pow((1.0/3600),2); //ut+1/2at^2
            flight->consumeFuel();
            printf("flight %d : taxi, speed=%f, dist=%f\n",flight->id, flight->speed,distanceToGate);
            //sleep(1);
            usleep(1000);
        }
        //TAXI->GATE
        while(distanceToGate>0 && flight->speed>0){
            flight->speed += -1125*(1.0/3600); //v=u+at
            distanceToGate -= flight->speed/3600.0 + 0.5*-1125*pow((1.0/3600),2); //ut+1/2at^2
            flight->consumeFuel();
            printf("flight %d : taxi->gate, speed=%f, dist=%f\n",flight->id, flight->speed,distanceToGate);
            //sleep(1);
            usleep(1000);
        }
        flight->phase = FlightPhase::GATE;
            
        if(flight->airlineType==CARGO){
            rwyC.releaseRunway();
        }
        else
            rwyA.releaseRunway();
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

class Radar{
public:
    pthread_t thread_id;
//radar thread function (per flight)
    static void* flightRadar(void* arg){

        Flight* flight = (Flight*)arg; 
        bool violation[NUM_FLIGHT_PHASES] = {0}; //no phase violation yet
        
        while(flight){ //flight ongoing
            if (flight->checkViolation() && !violation[(int)flight->phase]) {//if first violation of phase
                FlightPhase phase = flight->phase;
                int amount=0;
                //generate AVN 
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
            //sleep(1);
            usleep(1000);
        }
        pthread_exit(nullptr);
    }
    void terminateRadar(){
        pthread_cancel(this->thread_id);
        this->thread_id = -1;
    }
};

class Dispatcher{
public:
    Radar radar;

    int numFlightsDispatched;
    static pthread_t flightTid[TOTAL_FLIGHTS]; //all flight thread ids
    //static pthread_t radarTid[TOTAL_FLIGHTS]; //all radar thread ids
    static int flightsRemainingPerAirline[NUM_AIRLINE];

    QueueFlights* queue; 

    Dispatcher(){
        numFlightsDispatched=0;
        queue= new QueueFlights;
    }

    void dispatchFlight(FlightType flightType) {
        int airline = rand() % NUM_AIRLINE;
        while (flightsRemainingPerAirline[airline] <= 0) {
            airline = rand() % NUM_AIRLINE;
        }

        int flightID = numFlightsDispatched + 1;
        int type;
        switch (airline) {
            case 0:
            case 1:
                type = 0;
                break;
            case 2:
            case 4:
                type = 1;
                break;
            case 3:
                type = 2;
                break;
            default:
                type = 3;
                break;
        }

        Flight* fl = new Flight(flightID, flightType, (AirlineType)type, (AirlineName)airline);
        queue->addFlight(fl);

        flightsRemainingPerAirline[airline]--;
    }

    
    static void* dispatchFlights(void* arg) {
        Dispatcher* dispatcher = (Dispatcher*)(arg);

        while ( (dispatcher->numFlightsDispatched < TOTAL_FLIGHTS || !dispatcher->queue->incomingQueue.empty() || !dispatcher->queue->outgoingQueue.empty())  && Timer::currentTime < SIMULATION_DURATION) {
            
            if (!dispatcher->queue->incomingQueue.empty() || !dispatcher->queue->outgoingQueue.empty()) {
                Flight* flight = nullptr;

                // Get flight from queues
                if (!dispatcher->queue->incomingQueue.empty()) {
                    flight = dispatcher->queue->incomingQueue.front();
                    dispatcher->queue->incomingQueue.erase(dispatcher->queue->incomingQueue.begin());
                }
                else if (!dispatcher->queue->outgoingQueue.empty()) {
                    flight = dispatcher->queue->outgoingQueue.front();
                    dispatcher->queue->outgoingQueue.erase(dispatcher->queue->outgoingQueue.begin());
                }

                if (flight) {
                    args* arg = new args;
                    arg->flight = flight;
                    arg->qf = dispatcher->queue;

                    if (flight->type == INTERNATIONAL_ARRIVAL || flight->type == DOMESTIC_ARRIVAL) {
                        pthread_create(&(flight->thread_id), nullptr, simulateFlightArrival, (void*)arg);
                        //pthread_create(&(flight->radar_id), nullptr, dispatcher->radar.flightRadar, (void*)flight);
                    }
                    else {
                        pthread_create(&(flight->thread_id), nullptr, simulateFlightDeparture, (void*)arg);
                        //pthread_create(&(flight->radar_id), nullptr, dispatcher->radar.flightRadar, (void*)flight);
                    }

                    dispatcher->numFlightsDispatched++;
                }
            }
            else {
                if (Timer::currentTime % 120 == 0) {
                    dispatcher->dispatchFlight((FlightType::DOMESTIC_ARRIVAL));
                } 
                else if (Timer::currentTime % 150 == 0) {
                    dispatcher->dispatchFlight(FlightType::INTERNATIONAL_DEPARTURE);
                }
                else if (Timer::currentTime % 180 == 0) {
                    dispatcher->dispatchFlight(FlightType::INTERNATIONAL_ARRIVAL);
                }
                else if (Timer::currentTime % 240 == 0) {
                    dispatcher->dispatchFlight(FlightType::DOMESTIC_DEPARTURE);
                }
            }

            sleep(1);
        }

        return nullptr;
    }


};
pthread_t Dispatcher::flightTid[TOTAL_FLIGHTS];
//pthread_t Dispatcher::radarTid[TOTAL_FLIGHTS];
int Dispatcher::flightsRemainingPerAirline[NUM_AIRLINE] = {4, 4, 2, 1, 2, 1};

int main() {
    srand(time(NULL));
    initializeFlightPhases();

    Dispatcher flightDispatcher;
    //Radar radar;
    //Timer timer;
    
    //pthread_t timertid;
    //pthread_create(&timertid, NULL, timer.simulationTimer, NULL );

    pthread_t dispatcher;
    pthread_create(&dispatcher, nullptr, Dispatcher::dispatchFlights, (void*)&flightDispatcher);  // Pass dispatcher to the thread

    //pthread_t tid;
    //pthread_create(&tid,nullptr,simulateFlightDeparture,(void*)test); //create flight thread

    //and create a radar to monitor it
    //pthread_t radartid;
    //pthread_create(&radartid,nullptr,radar.flightRadar,nullptr);

    //pthread_join(timertid, NULL);
    //pthread_join(tid,NULL);
    //pthread_join(radartid,NULL);
    pthread_join(dispatcher,NULL);
    
   /*QueueFlights* qf= new QueueFlights;

    //int id, FlightType flightType, AirlineType airlineType, AirlineName name
    Flight* fl1= new Flight(1, FlightType:: INTERNATIONAL_ARRIVAL, AirlineType:: COMMERCIAL, AirlineName:: PIA);
    Flight* fl2= new Flight(2, FlightType:: DOMESTIC_ARRIVAL, AirlineType:: MEDICAL, AirlineName:: AghaKhan_Air_Ambulance);
    Flight* fl3= new Flight(3, FlightType:: DOMESTIC_DEPARTURE, AirlineType:: COMMERCIAL, AirlineName:: PIA);
    Flight* fl4 = new Flight(4, FlightType:: DOMESTIC_DEPARTURE, AirlineType:: MILITARY, AirlineName:: AghaKhan_Air_Ambulance);
    Flight* fl5 = new Flight(5, FlightType:: DOMESTIC_ARRIVAL, AirlineType:: CARGO, AirlineName:: AghaKhan_Air_Ambulance);


    qf->addFlight(fl1);
    Flight* currentFlight = qf->incomingQueue.front();
    qf->incomingQueue.erase(qf->incomingQueue.begin());
    args* a= new args;
    a->flight=currentFlight;
    a->qf= qf;
    pthread_t thread;
    pthread_create(&(fl1->thread_id), nullptr, simulateFlightArrival, (void*)a);
    usleep(1000);

    qf->addFlight(fl2);
    Flight* currentFlight2 = qf->incomingQueue.front();
    qf->incomingQueue.erase(qf->incomingQueue.begin());
    args* a2= new args;
    a2->flight=currentFlight2;
    a2->qf= qf;
    pthread_t thread2;
    pthread_create(&(fl2->thread_id), nullptr, simulateFlightArrival, (void*)a2);
    usleep(1000);

    qf->addFlight(fl3);
    Flight* currentFlight3 = qf->outgoingQueue.front();
    qf->outgoingQueue.erase(qf->outgoingQueue.begin());
    args* a3= new args;
    a3->flight=currentFlight3;
    a3->qf= qf;
    pthread_create(&(fl3->thread_id), nullptr, simulateFlightDeparture, (void*)a3);
    usleep(1000);

    qf->addFlight(fl4);
    Flight* currentFlight4 = qf->outgoingQueue.front();
    qf->outgoingQueue.erase(qf->outgoingQueue.begin());
    args* a4= new args;
    a4->flight=currentFlight4;
    a4->qf= qf;
    pthread_create(&(fl4->thread_id), nullptr, simulateFlightDeparture, (void*)a4);
    usleep(1000);*/

    pthread_exit(NULL);
    return 0;
}