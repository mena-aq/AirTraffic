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

//should we make these objects so they have type also 
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
    flightPhases[(int)FlightPhase::TAXI] = Phase{0, 30, 0, 0};
    flightPhases[(int)FlightPhase::TAKEOFF] = Phase{0, 290, 0, 500};                          
    flightPhases[(int)FlightPhase::CLIMB] = Phase{250, 802, 0, 10050}; 
    flightPhases[(int)FlightPhase::CRUISE] = Phase{800, 900, 30000, 42000};
    flightPhases[(int)FlightPhase::HOLDING] = Phase{288, 600, 5950, 14000};
    flightPhases[(int)FlightPhase::APPROACH] = Phase{240, 290, 800, 6050};
    flightPhases[(int)FlightPhase::LANDING] = Phase{29, 245, 0, 800};
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

    const FlightType flightType; //international/domestic arrival/departure
    const AirlineType airlineType; //commercial,cargo,med,military etc
    const AirlineName airlineName; // PIA etc
    const char direction; // N, S, E, W

    FlightPhase phase; //current phase flight is in
    float speed; //current speed
    float altitude; //current altitude

    int priority; //priority of flight for runway access
    float fuelLevel; //in gallons
    float waitingTime; 

    bool faulty;

    Flight(int id, FlightType flightType, AirlineType airlineType, AirlineName name) :  id(id), thread_id(-1), radar_id(-1), airlineName(name), flightType(flightType), waitingTime(0), faulty(false){    
        
        if (flightType == FlightType::DOMESTIC_ARRIVAL || flightType == FlightType::INTERNATIONAL_ARRIVAL) {
            phase = FlightPhase::HOLDING; // if its arriving, start phase is holding
            speed = 400 + rand() % 201;  // 400 - 600 random time
            altitude = 6000; 
            fuelLevel = 1000; //assuming 1 gallon/s
        }
        else {
            phase = FlightPhase::GATE;
            speed = 0;
            altitude = 0;
            fuelLevel = 1000; 
        }

        if (airlineType == AirlineType::MEDICAL || airlineType == AirlineType::MILITARY){
            priority = 20;
        } 
        else if (airlineType == AirlineType::CARGO){
            priority = 15;
        }
        else {
            priority = 10;
        }

        switch(flightType){
            case FlightType::INTERNATIONAL_ARRIVAL:
                this->direction = 'N';
                //this->priority += 10;
                break;
            case FlightType::DOMESTIC_ARRIVAL:
                this->direction='S';
                //this->priority += 5;
                break;
            case FlightType::INTERNATIONAL_DEPARTURE:
                this->direction='E';
                //this->priority += 15;
                break;
            case FlightType::DOMESTIC_DEPARTURE:
                this->direction='W';
                //this->priority += 20;
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

    //ADDED
    void rerouteFlight(){
        pthread_cancel(this->thread_id);
        pthread_join(this->thread_id, nullptr);
        this->thread_id = -1;
        printf("Re-routing flight %d\n", this->id);
    }

    bool isArrival() const{
        return (this->flightType == FlightType::DOMESTIC_ARRIVAL || this->flightType == FlightType::INTERNATIONAL_ARRIVAL);
    }
    bool isDeparture() const{
        return (this->flightType == FlightType::DOMESTIC_DEPARTURE|| this->flightType == FlightType::INTERNATIONAL_DEPARTURE);
    }

    //operator for priority 
    bool operator<(const Flight& other) const{
        if (this->priority!=other.priority)
            return (this->priority>other.priority);        //priority sched
        return this->id < other.id;                       // FCFS    
    }

    // THE FLIGHT SIMULATION FUNCTIONS
    void simulateFlightDeparture() {                                        
    
        printf("DEPARTURE: flight ID: %d\n",flight->id);
    
        // ------------- SIMULATE --------------
    
        // START TAXI (0->(15-30))
        float distanceToRunway = TERMINAL_TO_RUNWAY_LEN;
        this->phase = FlightPhase::TAXI;
        while (this->speed<15 && distanceToRunway>0.05){
            this->speed += 2 + (rand()%50)/10.0; //speed up by 2 - 2.5km/s
            distanceToRunway -= this->speed/3600; //s=vt
            this->consumeFuel();
            //printf("Flight : %d Distance to Runway= %f. Speed=%f\n", flight->id, distanceToRunway,flight->speed);
            //sleep(1);
            pthread_testcancel(); // safe cancellation point
            usleep(1000);
        }
        // KEEP TAXIING UNTIL NEAR RUNWAY
        while (distanceToRunway>0.05){
            this->speed += (5-rand()%10)/10.0; //vary by ~0.5
            distanceToRunway -= this->speed/3600; //s=vt
            this->consumeFuel();
            //printf("Flight : %d Distance to Runway= %f. Speed=%f\n", flight->id, distanceToRunway,flight->speed);
            usleep(1000);
        }
        // TAXI->STOP before takeoff pause
        while(this->speed>0){
            //int a = -2500 + (500-rand()%1000);
            //flight->speed += a/3600.0; //v=u+at
            this->speed -= 5; //decelerate by 5km/s^2
            distanceToRunway -= this->speed/3600; //s=vt
            if (this->speed<0)
                this->speed=0;
            //distanceToRunway -= flight->speed/3600.0 + 0.5*a*pow((1.0/3600),2); //ut+1/2at^2
            this->consumeFuel();
            //printf("Flight : %d Distance to Runway= %f. Speed=%f\n",flight->id,distanceToRunway, flight->speed);
            //sleep(1);
            pthread_testcancel(); // safe cancellation point
            usleep(1000);
        }
        //printf("Flight : %d Stopped at runway dist=%f, prepare to takeoff\n",flight->id, distanceToRunway); //ye submit krna h??? its wirtten in milestone 2 and class to nhi h 
        // TAKEOFF from standstill
        this->phase = FlightPhase::TAKEOFF;
        float distanceAlongRunway = RUNWAY_LEN + distanceToRunway;
        while (distanceAlongRunway > 0.01 ){ //while runway left
            int a = 14050 + (1000-rand()%2000);
            this->speed += a/3600.0; //v=u+at
            distanceAlongRunway -= this->speed/3600.0 + 0.5*a*pow((1.0/3600),2); //ut+1/2at^2
            this->consumeFuel();
            //printf("Flight : %d takeoff, speed=%f, altitude=%f, dist=%f\n",flight->id, flight->speed,flight->altitude,distanceAlongRunway);
            //sleep(1);
            usleep(1000);
        }
        //printf("Flight : %d Takeoff at dist remaining=%f\n", flight->id, distanceAlongRunway);
        // CLIMB
        this->phase = FlightPhase::CLIMB;
        while (this->speed < 800){
            int a = 9000 + (500-rand()%1000);
            this->speed += a*(1.0/3600); //v=u+at
            this->altitude += 3280* (this->speed*sin(M_PI/12)*(1.0/3600) + 0.5*(a*sin(M_PI/12))*pow((1.0/3600),2));//ut+1/2at^2
            this->consumeFuel();
            //printf("Flight : %d climb, speed=%f, altitude=%f\n", flight->id,flight->speed,flight->altitude);
            //sleep(1);
            usleep(1000);
        }
        //CRUISE for a bit (bring to correct altitude) until out of airspace
        this->phase=FlightPhase::CRUISE;
        float cruisingAltitude = (flightPhases[FlightPhase::CRUISE].altitudeLowerLimit + flightPhases[FlightPhase::CRUISE].altitudeUpperLimit)/2.0;
        while (this->altitude < cruisingAltitude){
            int a = 5000 + (500-rand()%1000);
            this->speed += a/3600.0; //v=u+at
            this->altitude += 3280 *(this->speed*sin(M_PI/12)*(1.0/3600) + 0.5*(a*sin(M_PI/12))*pow((1.0/3600),2));//ut+1/2at^2
            usleep(1000);
            this->consumeFuel();
            //printf("Flight : %d cruise, speed=%f, altitude=%f\n", flight->id, flight->speed,flight->altitude);
            //flight->printStatus();
        }
        this->phase=FlightPhase::CRUISE;
        printf("Exiting Airspace...\n");
    
        //rwyB.releaseRunway();
    
        //exit atomicaly
        //pthread_mutex_lock(&flightMutex);
        //free(flight);
        //pthread_mutex_unlock(&flightMutex);
        
        pthread_exit(nullptr);
    }

    void simulateFlightArrival(){  
    
        printf(" ARRIVAL :flight ID: %d\n",flight->id);
    
        //rwyA.acquireRunway(flight,qf);
    
        // ------------- SIMULATE --------------
    
        float distanceToRunway = 15; //for a safe-ish descent (50ft/s)
    
        //bring flight from HOLDING->APPROACH to avoid violation
        while(this->speed > 292){
            //int a = -4250 + (100-rand()%200);
            //flight->speed += a*(1.0/3600); //v=u+at
            this->speed -= 2 + (1-rand()%2); //~1.5-2.5
            distanceToRunway -= this->speed*(1.0/3600);
            //distanceToRunway -= flight->speed/3600.0 + 0.5*a*pow((1.0/3600),2); //ut+1/2at^2
            this->altitude -= 30; //safe (if 1km away its 107ft/s which would kill everyone i fear)
            this->consumeFuel();
            //printf("flight %d : holding->approach, speed=%f, altitude=%f, dist=%f\n", flight->id, flight->speed,flight->altitude,distanceToRunway);
            //sleep(1);
            pthread_testcancel(); // safe cancellation point
            usleep(1000);
        }
        //APPROACH
        this->phase = FlightPhase::APPROACH;
        while(this->speed>242.5){
            this->speed += -8000*(1.0/3600); //v=u+at
            distanceToRunway -= this->speed * (1.0/3600); 
            //flight->speed -= 4 
            //distanceToRunway -= flight->speed*(1.0/3600);
            this->altitude -= 40;   
            this->consumeFuel();
            //printf("flight %d : approach, speed=%f, altitude=%f, dist=%f\n",flight->id, flight->speed,flight->altitude,distanceToRunway);
            //sleep(1);
            pthread_testcancel(); // safe cancellation point
            usleep(1000);
        }
        //LANDING
        this->phase = FlightPhase::LANDING;
        float distanceAlongRunway = distanceToRunway+RUNWAY_LEN; 
        while (this->speed > 30 || this->altitude>0){
            float a = -9750 + (50-rand()%100);
            this->speed += a*(1.0/3600); //v=u+at
            distanceAlongRunway -= this->speed/3600.0 + 0.5*a*pow((1.0/3600),2); //ut+1/2at^2
            if (this->altitude>0){
                this->altitude -= 60;
                if (this->altitude<0)
                    this->altitude=0;
            }
            this->consumeFuel();
            //printf("flight %d : landing, speed=%f, altitude=%f, dist=%f\n",flight->id, flight->speed,flight->altitude,distanceAlongRunway);
            //sleep(1);
            usleep(1000);
        }
            
        //TAXI
        this->phase = FlightPhase::TAXI;
        float distanceToGate = distanceAlongRunway+TERMINAL_TO_RUNWAY_LEN;
        while (distanceToGate>0.05){
            if (this->speed<28)
                this->speed += (50-rand()%100)/100.0;//fluctuate by ~0.5
            else
                this->speed -= 0.5;
            //distanceToGate -= flight->speed/3600.0 + 0.5*-1000*pow((1.0/3600),2); //ut+1/2at^2
            distanceToGate -= this->speed*(1.0/3600);
            this->consumeFuel();
            //printf("flight %d : taxi, speed=%f, dist=%f\n",flight->id, flight->speed,distanceToGate);
            //sleep(1);
            usleep(1000);
        }
        //TAXI->GATE 
        while(this->speed>0){
            //flight->speed += -1500*(1.0/3600); //v=u+at
            this->speed -= 4;
            if (this->speed<0)
                this->speed=0;
            distanceToGate -= this->speed*(1.0/3600);
            //distanceToGate -= flight->speed/3600.0 + 0.5*-1125*pow((1.0/3600),2); //ut+1/2at^2
            this->consumeFuel();
            //printf("flight %d : taxi->gate, speed=%f, dist=%f\n",this->id, this->speed,distanceToGate);
            //sleep(1);
            usleep(1000);
        }
        this->phase = FlightPhase::GATE;
            
        //rwyA.releaseRunway();
    
        //exit atomicaly
        //pthread_mutex_lock(&flightMutex);
        //free(flight);
        //pthread_mutex_unlock(&flightMutex);
            
        pthread_exit(nullptr);
    }
    

};


class QueueFlights {
public:
    vector<Flight*> flightQueue;
    
    // Helper to sort a vector based on Comparator
    void sortQueue(std::vector<Flight*>& flights) {
        //std::sort(flights.begin(), flights.end(), Comparator());
        std::sort(flights.begin(), flights.end());
    }
    void addFlight(Flight*& flight) {
        flightQueue.push_back(flight);
        //sortQueue(flightQueue); // Keep it sorted after every insertion
        printf("Flight %d added to Q\n",flight->id);
    }
    Flight* getNextFlight(){
        Flight* nextFlight = flightQueue.front();
        flightQueue.erase(flightQueue.begin());
        return nextFlight;
    }
    bool empty(){
        return flightQueue.empty();
    }
    void printQueues() {
        cout<<"Flights in Queue\n";
        for (const auto& flight : flightQueue) {
            flight->printStatus();
        }
    }
};


class Runway {
private:
    pthread_mutex_t lock;
    //pthread_cond_t cond;
public:
    char runwayID;
    //char priorityDirection;
    //int waitingLowPriority;
    //int waitingHighPriority;
    //bool busy;
    //Flight* currentFlight;

    Runway(char id, char priorityDirection) : runwayID(id), priorityDirection(priorityDirection) {
        pthread_mutex_init(&lock, nullptr);
        //pthread_cond_init(&cond, nullptr);
        //busy = false;
        //currentFlight = nullptr;
    }

    ~Runway() {
        pthread_mutex_destroy(&lock);
        //pthread_cond_destroy(&cond);
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
    void acquireRunway(){
        pthread_mutex_lock(&lock);
    }
/*
    void acquireRunway2(Flight* flight,QueueFlights*& qf){//for now its just for arrival
        printf("Flight %d wants runway access\n",flight->id);
        if (currentFlight==nullptr){ //if available, acquire
            pthread_mutex_lock(&lock);
            currentFlight = flight;
            printf("Flight %d acquire runway %c\n",flight->id, runwayID);
            return;
        }
        else{
            //printf("can i pre-empt?\n");
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
*/

    void releaseRunway(){
        //printf("Flight %d released runway %c\n",currentFlight->id, this->runwayID);
        pthread_mutex_unlock(&lock);
        //currentFlight = nullptr;
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

    Radar():thread_id(-1){}
    //radar thread function (per flight)
    static void* flightRadar(void* arg){

        Flight* flight = (Flight*)arg; 
        bool violation[NUM_FLIGHT_PHASES]={false};

        
       while(flight && flight->thread_id!=-1){ //flight ongoing
            FlightPhase phase = flight->phase;
            int phaseIndex = static_cast<int>(phase);
           //pthread_mutex_lock(&radarLock);
            if (flight->checkViolation() && violation[phaseIndex]==false) {//if first violation of phase
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

                printf("Before update violation[%d]=%d\n",phaseIndex,violation[phaseIndex]);
                violation[phaseIndex]=true;
                printf("After update violation[%d]=%d\n",phaseIndex,violation[phaseIndex]);
            }

            pthread_testcancel(); // safe cancellation point
            //flight->printStatus();
            //sleep(1);
            usleep(1000);
        }
        pthread_exit(nullptr);
    }
    void terminateRadar(){
        pthread_cancel(this->thread_id);
        pthread_join(this->thread_id, nullptr);
        this->thread_id = -1;
        //printf("Cancelling radar for flight %d\n",flight->id);
        //flight=nullptr;
    }
};


struct args{
    Flight* flight;
    char runway;
};

void* simulateWaitingAtGate(void* arg){

    args* flightArg = static_cast<arg*>(arg);
    Flight* flight = flightArg->flight;
    const char runwayToAcquire = arg->runway;

    flight->phase = FlightPhase::GATE;
    //flight will wait at gate stationary until runway clears
    //if any fault, just tow and remove from queue
    while (!flightTurn[flight->id-1]){
        flight->waitingTime++;
        sleep(1);
        if (flight->waitingTime%100 == 0)
            this->priority++; //up the priority every 100s
    }
    
    if (runwayToAcquire=='C')
        rwyC.acquireRunway();
    else
        rwyB.acquireRunway();

    flight->simulateFlightDeparture();

    if (runwayToAcquire=='C')
        rwyC.releaseRunway();
    else
        rywB.releaseRunway();

    pthread_exit(NULL);
}

void* simulateWaitingInHolding(void* arg){

    args* flightArg = static_cast<arg*>(arg);
    Flight* flight = flightArg->flight;
    const char runwayToAcquire = arg->runway;

    flight->phase = FlightPhase::HOLDING;
    //keep holding around airport until its your turn
    while (flightTurn[flight->id-1]){
        flight->speed += (10-rand()%20);//fluctuate by ~10
        flight->consumeFuel();
        flight->waitingTime++;
        if (flight->waitingTime%100 == 0)
            this->priority++; //up the priority every 100s
        sleep(1);
    }
        
    if (runwayToAcquire=='C')
        rwyC.acquireRunway();
    else
        rwyA.acquireRunway();

    flight->simulateFlightArrival();
  
    if (runwayToAcquire=='C')
        rwyC.acquireRunway();
    else
        rwyA.acquireRunway();

    pthread_exit(NULL);
}

class Dispatcher{
public:
    Radar radar;

    //int numFlightsDispatched;
    pthread_mutex_t turnLock;
    pthread_cond_t cond;
    char runway;

    static pthread_t flightTid[TOTAL_FLIGHTS]; //all flight thread ids
    static pthread_t radarTid[TOTAL_FLIGHTS]; //all radar thread ids
    //static int flightsRemainingPerAirline[NUM_AIRLINE];
    
    //vector<flight*> flights;

    QueueFlights* flights; 

    Dispatcher(char runway): runway(runway){
       flights= new QueueFlights;
    }
    
    static void* dispatchFlights(void* arg) {

        Dispatcher* dispatcher = (Dispatcher*)(arg);

        //first launch all the flights
        for (int i=0; i<flights.size(); i++){ 
            int index = flights[i]->id-1;
            if (flight[i].flightType==FlightType::INTERNATIONAL_ARRIVAL||flight[i].flightType==FlightType::DOMESTIC_ARRIVAL) 
                pthread_create(&flightTid[index],NULL,simulateWaitingInHolding,(void*)flights[i]);
            else
                pthread_create(&flightTid[index],NULL,simulateWaitingAtGate,(void*)flights[i]);
            pthread_create(&(flights[i]->radar_id), nullptr, dispatcher->radar.flightRadar, (void*)flights[i]);
        }

        // START AIR TRAFFIC CONTROL OF ALL FLIGHTS BY FCFS/PRIORITY
        while ( !dispatcher->flights.empty() && Timer::currentTime < SIMULATION_DURATION) {

            //sort (if fuel running out age them etc)
            flights.sortQueue();
            //dispatch the front (highest priority flight)
            Flight* currentFlight = flights.getNextFlight();

            //set its turn
            pthread_mutex_lock(&turnLock);
            flightTurn[currentFlight->id-1]=true;
            pthread_mutex_unlock(&turnLock);

            pthread_join(currentFlight->thread_id,NULL);
            free(currentFlight);
            
            // menahil, im just confused that how are we going to bringthreads from holding function back  
            // you said k holding me loop exit krkay call krlay, im ok w that
            //thats what you meant right
            //pthread_mutex_lock(&rwyA.lock);
            //pthread_mutex_lock(&rwyA.lock);

            //unset turn            //reaching here means rwy freed, so dispatch next flight
            pthread_mutex_lock(&turnLock);
            flightTurn[currentFlight->id-1]=false;
            pthread_mutex_unlock(&turnLock);

        /*
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

                    if (flight->flightType == INTERNATIONAL_ARRIVAL || flight->flightType == DOMESTIC_ARRIVAL) {
                        pthread_create(&(flight->thread_id), nullptr, simulateFlightArrival, (void*)arg);
                        pthread_create(&(flight->radar_id), nullptr, dispatcher->radar.flightRadar, (void*)flight);
                    }
                    else {
                        pthread_create(&(flight->thread_id), nullptr, simulateFlightDeparture, (void*)arg);
                        pthread_create(&(flight->radar_id), nullptr, dispatcher->radar.flightRadar, (void*)flight);
                    }

                    //dispatcher->numFlightsDispatched++;
                }
            }
            else {
                if (Timer::currentTime % 120 == 0) {
                    dispatcher->dispatchFlight(FlightType::DOMESTIC_ARRIVAL);
                } 
                if (Timer::currentTime % 150 == 0) {
                    dispatcher->dispatchFlight(FlightType::INTERNATIONAL_DEPARTURE);
                }
                if (Timer::currentTime % 180 == 0) {
                    dispatcher->dispatchFlight(FlightType::INTERNATIONAL_ARRIVAL);
                }
                if (Timer::currentTime % 240 == 0) {
                    dispatcher->dispatchFlight(FlightType::DOMESTIC_DEPARTURE);
                }
            }
        */
            sleep(1);
        }

        return nullptr;
    }


};
/*
pthread_t Dispatcher::flightTid[TOTAL_FLIGHTS];
pthread_t Dispatcher::radarTid[TOTAL_FLIGHTS];
in

bool flightTurn[TOTAL_FLIGHTS]={false};t Dispatcher::flightsRemainingPerAirline[NUM_AIRLINE] = {4, 4, 2, 1, 2, 1};
*/

int main() {

    srand(time(NULL));
    initializeFlightPhases();

    // INPUT FLIGHTS
    Dispatcher rwyADispatcher('A');
    Dispatcher rwyBDispatcher('B');
    Dispatcher rwyCDispatcher('C');
    
    //should we have two ids? one we give(for indexing) and one they choose, or they just dont get to input that Dispatcher rwyCDispatcher('C');
    
    //input validation of airline?? military not PIA etc (later)
    rwyADispatcher.flights.addFlight(new Flight(1, FlightType:: INTERNATIONAL_ARRIVAL, AirlineType:: COMMERCIAL, AirlineName:: PIA));
    rwyADispatcher.flights.addFlight(new Flight(2, FlightType:: DOMESTIC_ARRIVAL, AirlineType:: MILITARY, AirlineName::Pakistan_Airforce));
     
    //rwyBDispatcher.flights.addFlight(new Flight(3, FlightType:: DOMESTIC_DEPARTURE, AirlineType:: COMMERCIAL, AirlineName:: PIA));
    //rwyBDispatcher.flights.addFlight(new Flight(4, FlightType:: INTERNATIONAL_DEPARTURE, AirlineType:: MEDICAL, AirlineName:: AghaKhan_Air_Ambulance));

    //rwyCDispatcher.flights.addFlight(new Flight(5, FlightType:: DOMESTIC_ARRIVAL, AirlineType:: CARGO, AirlineName:: FedEx));
    //rwyCDispatcher.flights.addFlight(new Flight(6, FlightType:: INTERNATIONAL_ARRIVAL, AirlineType:: CARGO, AirlineName:: Blue_Dart));

    pthread_t dispatcherA;
    pthread_t dispatcherB;
    pthread_t dispatcherC;

    pthread_create(&dispatcherA, nullptr, Dispatcher::dispatchFlights, (void*)&rwyADispatcher);  // Pass dispatcher to the thread
    //pthread_create(&dispatcherB, nullptr, Dispatcher::dispatchFlights, (void*)&rwyBDispatcher);  // Pass dispatcher to the thread
    //pthread_create(&dispatcherC, nullptr, Dispatcher::dispatchFlights, (void*)&rwyCDispatcher);  // Pass dispatcher to the thread

    pthread_join(dispatcherA,NULL);
    //pthread_join(dispatcherB,NULL);
    //pthread_join(dispatcherC,NULL);
    pthread_exit(NULL);
    return 0;
}
