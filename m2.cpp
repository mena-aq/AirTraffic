#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include <chrono>
#include <cmath>
#include <algorithm>
using namespace std;

// ATC SYSTEM CONSTANTS 
#define NUM_FLIGHT_PHASES 8
#define NUM_AIRLINE 6
#define TOTAL_FLIGHTS 3
#define GRID 6 
#define SIMULATION_DURATION 300
//assume runways start at (0,0), so each quadrant is 3x3 km
#define RUNWAY_LEN 0.6
#define TERMINAL_TO_RUNWAY_LEN 0.03

//FIFO
#define AVN_FIFO1 "pipes/avnfifo_ATC"
#define AVN_FIFO2 "pipes/avnfifo_GEN"


// ENUMS
enum AirlineType {
    //highest priority
    MEDICAL, 
    MILITARY, 
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
    LANDING,
    DONE
};

bool flightTurn[TOTAL_FLIGHTS]={false};
pthread_cond_t flightCond[TOTAL_FLIGHTS];


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
    flightPhases[(int)FlightPhase::CLIMB] = Phase{250, 463, 0, 5000};
    flightPhases[(int)FlightPhase::CRUISE] = Phase{800, 900, 5000, 10000};
    flightPhases[(int)FlightPhase::HOLDING] = Phase{290, 600, 2000, 5000};
    flightPhases[(int)FlightPhase::APPROACH] = Phase{240, 290, 800, 1000};
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

    const FlightType flightType; //international/domestic arrival/departure
    const AirlineType airlineType; //commercial,cargo,med,military etc
    const AirlineName airlineName; // PIA etc
    char direction; // N, S, E, W

    FlightPhase phase; //current phase flight is in
    float speed; //current speed
    float altitude; //current altitude

    int priority; //priority of flight for runway access
    float fuelLevel; //in gallons
    float waitingTime; 
    float scheduledTime;

    bool faulty;

    Flight(int id, FlightType flightType, AirlineType airlineType, AirlineName name,float st, char dir ) :  id(id), direction(dir), scheduledTime(st), thread_id(-1), radar_id(-1), airlineName(name), airlineType(airlineType), flightType(flightType), waitingTime(0), faulty(false){    
        
        if (flightType == FlightType::DOMESTIC_ARRIVAL || flightType == FlightType::INTERNATIONAL_ARRIVAL) {
            phase = FlightPhase::HOLDING; // if its arriving, start phase is holding
            speed = 500;
            altitude = flightPhases[FlightPhase::HOLDING].altitudeLowerLimit+200;
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

    bool isArrival() const{
        return (this->flightType == FlightType::DOMESTIC_ARRIVAL || this->flightType == FlightType::INTERNATIONAL_ARRIVAL);
    }
    bool isDeparture() const{
        return (this->flightType == FlightType::DOMESTIC_DEPARTURE|| this->flightType == FlightType::INTERNATIONAL_DEPARTURE);
    }
    bool isDomestic(){
        return (this->flightType == FlightType::DOMESTIC_ARRIVAL || this->flightType == FlightType::DOMESTIC_DEPARTURE);
    }
    bool isInternational(){
        return (this->flightType == FlightType::INTERNATIONAL_ARRIVAL || this->flightType == FlightType::INTERNATIONAL_DEPARTURE);

    }

    //operator for priority 
    bool operator<(const Flight& other) const{
        if (this->priority!=other.priority)
            return (this->priority>other.priority); //priority sched //im doing fuel alr         
        return this->id < other.id;                       // FCFS    
    }

    int simulateFlightDeparture() {                                        
    
        printf("DEPARTURE: flight ID: %d\n",this->id);
    
        // ------------- SIMULATE --------------
    
        // START TAXI (0->(15-30))
        float distanceToRunway = TERMINAL_TO_RUNWAY_LEN;
        this->phase = FlightPhase::TAXI;
        while (this->speed<15 && distanceToRunway>0.01){
            this->speed += 2 + (rand()%50)/10.0; //speed up by 2 - 2.5km/s
            distanceToRunway -= this->speed/3600; //s=vt
            this->consumeFuel();
            printf("Flight : %d Distance to Runway= %f. Speed=%f\n", this->id, distanceToRunway,this->speed);
            if (!flightTurn[this->id-1]){ // somebody pre-empted it, go back to holding
                return 0; //exit w/ unfinished status
            }
            sleep(1);
            //usleep(1000);
        }
        // KEEP TAXIING UNTIL NEAR RUNWAY
        while (distanceToRunway>0.001){
            this->speed += (5-rand()%10)/10.0; //vary by ~0.5
            distanceToRunway -= this->speed/3600; //s=vt
            this->consumeFuel();
            printf("Flight : %d Distance to Runway= %f. Speed=%f\n", this->id, distanceToRunway,this->speed);
            if (!flightTurn[this->id-1]){ // somebody pre-empted it, go back to holding
                return 0; //exit w/ unfinished status
            }
            sleep(1);
            //usleep(1000);
        }
        // TAXI->STOP before takeoff pause
        while(this->speed>0){
            this->speed -= 5; //decelerate by 5km/s^2
            distanceToRunway -= this->speed/3600; //s=vt
            if (this->speed<0)
                this->speed=0;
            this->consumeFuel();
            printf("Flight : %d Distance to Runway= %f. Speed=%f\n",this->id,distanceToRunway, this->speed);
            sleep(1);
            //usleep(1000);
        }
        //printf("Flight : %d Stopped at runway dist=%f, prepare to takeoff\n",flight->id, distanceToRunway); //ye submit krna h??? its wirtten in milestone 2 and class to nhi h 
        // TAKEOFF from standstill
        this->phase = FlightPhase::TAKEOFF;
        float distanceAlongRunway = RUNWAY_LEN + distanceToRunway;
        while (distanceAlongRunway > 0.01 ){ //while runway left
            int a = 65050 + (1000-rand()%2000);
            this->speed += a/3600.0; //v=u+at
            distanceAlongRunway -= this->speed/3600.0 + 0.5*a*pow((1.0/3600),2); //ut+1/2at^2
            this->consumeFuel();
            printf("Flight : %d takeoff, speed=%f, altitude=%f, dist=%f\n",this->id, this->speed,this->altitude,distanceAlongRunway);
            sleep(1);
            //usleep(1000);
        }
        //printf("Flight : %d Takeoff at dist remaining=%f\n", flight->id, distanceAlongRunway);
        // CLIMB
        this->phase = FlightPhase::CLIMB;
        while (this->speed < 800){
            int a = 60000 + (500-rand()%1000);
            this->speed += a*(1.0/3600); //v=u+at
            this->altitude += 3280* (this->speed*sin(M_PI/12)*(1.0/3600) + 0.5*(a*sin(M_PI/12))*pow((1.0/3600),2));//ut+1/2at^2
            this->consumeFuel();
            printf("Flight : %d climb, speed=%f, altitude=%f\n", this->id,this->speed,this->altitude);
            sleep(1);
            //usleep(1000);
        }
        //CRUISE for a bit (bring to correct altitude) until out of airspace
        this->phase=FlightPhase::CRUISE;
        float cruisingAltitude = (flightPhases[FlightPhase::CRUISE].altitudeLowerLimit + flightPhases[FlightPhase::CRUISE].altitudeUpperLimit)/2.0;
        while (this->altitude < cruisingAltitude){
            int a = 25000 + (500-rand()%1000);
            this->speed += a/3600.0; //v=u+at
            this->altitude += 200;
            this->consumeFuel();
            sleep(1);
            //usleep(1000);
            //printf("Flight : %d cruise, speed=%f, altitude=%f\n", this->id, this->speed,this->altitude);
        }
        this->phase=FlightPhase::CRUISE;
        printf("Flight %d exiting..\n",this->id);
        return 1; //return w/ finished status

    }

    int simulateFlightArrival(){  
    
        printf(" ARRIVAL :flight ID: %d\n",this->id);
    
        // ------------- SIMULATE --------------
    
        float distanceToRunway = 3.5; //for a safe-ish descent (50ft/s)
        
        //bring flight from HOLDING->APPROACH to avoid violation
        while(this->speed > 292){
            this->speed -= 20 + (1-rand()%2); //~1.5-2.5
            distanceToRunway -= this->speed*(1.0/3600);
            this->altitude -= 40; //safe (if 1km away its 107ft/s which would kill everyone i fear)
            this->consumeFuel();
            printf("flight %d : holding->approach, speed=%f, altitude=%f, dist=%f\n", this->id, this->speed,this->altitude,distanceToRunway);
            if (!flightTurn[this->id-1]){ // somebody pre-empted it, go back to holding
                return 0; //exit w/ unfinished status
            }
            sleep(1);
            //usleep(1000);
        }
        //APPROACH
        this->phase = FlightPhase::APPROACH;
        while(this->speed>242.5){
            this->speed += -5000*(1.0/3600); //v=u+at
            distanceToRunway -= this->speed * (1.0/3600); 
            this->altitude -= 50;   
            this->consumeFuel();
            printf("flight %d : approach, speed=%f, altitude=%f, dist=%f\n",this->id, this->speed,this->altitude,distanceToRunway);
            if (!flightTurn[this->id-1]){ // somebody pre-empted it, go back to holding
                return 0;
            }
            sleep(1);
            //usleep(1000);
        }
        //LANDING
        this->phase = FlightPhase::LANDING;
        float distanceAlongRunway = distanceToRunway+RUNWAY_LEN; 
        while (this->speed > 30 || this->altitude>0){
            float a = -30750 + (50-rand()%100);
            this->speed += a*(1.0/3600); //v=u+at
            distanceAlongRunway -= this->speed/3600.0 + 0.5*a*pow((1.0/3600),2); //ut+1/2at^2
            if (this->altitude>0){
                this->altitude -= 30;
                if (this->altitude<0)
                    this->altitude=0;
            }
            this->consumeFuel();
            printf("flight %d : landing, speed=%f, altitude=%f, dist=%f\n",this->id, this->speed,this->altitude,distanceAlongRunway);
            sleep(1);
            //usleep(1000);
        }
            
        //TAXI
        this->phase = FlightPhase::TAXI;
        float distanceToGate = distanceAlongRunway+TERMINAL_TO_RUNWAY_LEN;
        while (distanceToGate>0.1){
            if (this->speed<28)
                this->speed += (50-rand()%100)/100.0;//fluctuate by ~0.5
            else
                this->speed -= 0.5;
            distanceToGate -= this->speed*(1.0/3600);
            this->consumeFuel();
            printf("flight %d : taxi, speed=%f, dist=%f\n",this->id, this->speed,distanceToGate);
            sleep(1);
            //usleep(1000);
        }
        //TAXI->GATE 
        while(this->speed>0){
            //flight->speed += -1500*(1.0/3600); //v=u+at
            this->speed -= 4;
            if (this->speed<0)
                this->speed=0;
            distanceToGate -= this->speed*(1.0/3600);
            this->consumeFuel();
            printf("flight %d : taxi->gate, speed=%f, dist=%f\n",this->id, this->speed,distanceToGate);
            sleep(1);
            //usleep(1000);
        }
        this->phase = FlightPhase::GATE;
                
                    
        printf("Flight %d exiting..\n",this->id);

        return 1; //exit w/ finished status
    }

};


class QueueFlights {
public:
    vector<Flight*> flightQueue;
    
    // Helper to sort a vector based on Comparator
    void sortQueue() {
        std::sort(flightQueue.begin(), flightQueue.end(), [](Flight* a, Flight* b) {
            if (a->priority != b->priority)
                return a->priority > b->priority; // Higher priority = comes first
            return a->id < b->id; // FCFS if same priority
        });
    }
    void sortQueue2() {
        std::sort(flightQueue.begin(), flightQueue.end(), [](Flight* a, Flight* b) {
            return a->scheduledTime < b->scheduledTime; // earlier time comes first
        });
    }

    void addFlight(Flight*& flight) {
        printf("added flight %d\n",flight->id);
        flightQueue.push_back(flight);
    }
    void addFlightAtFront(Flight*& flight){
        flightQueue.insert(flightQueue.begin(), flight);
    }
    Flight* getNextFlight(){
        if (!flightQueue.empty()){
            Flight* nextFlight = flightQueue.front();
            flightQueue.erase(flightQueue.begin());
            return nextFlight;
        }
        else 
            return nullptr;
    }
    bool isEmpty(){
        return flightQueue.empty();
    }
    int numInQueue(){
        return flightQueue.size();
    }
    void printQueues() {
        cout<<"Flights in Queue\n";
        for (const auto& flight : flightQueue) {
            flight->printStatus();
        }
    }
    Flight*& operator[](int i){
        return flightQueue[i];
    } 
};


class Runway {
private:
    pthread_mutex_t lock;
public:
    char runwayID;

    Runway(char id) : runwayID(id) {
        pthread_mutex_init(&lock, nullptr);
    }

    ~Runway() {
        pthread_mutex_destroy(&lock);
    }

    void acquireRunway(){
        //printf("try to acquire runway %c\n",this->runwayID);
        pthread_mutex_lock(&lock);
        //printf("acquired runway %c\n",this->runwayID);
    }
    void releaseRunway(){
        pthread_mutex_unlock(&lock);
    }

   
};

/*
struct ViolationInfo{
    int flightID;
    AirlineName airline;
    AirlineType airlineType;
    float speedRecorded;
    FlightPhase phaseViolated;

    ViolationInfo(){}
    ViolationInfo(int flightID,AirlineName airline,AirlineType airlineType, float speedRecorded,FlightPhase phaseViolated)
    : flightID(flightID),airline(airline),airlineType(airlineType),speedRecorded(speedRecorded),phaseViolated(phaseViolated) {}
};
*/

// AVN (Violation Notice)
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

    ViolationInfo(int flightID,int airline,int airlineType,float speedRecorded,int phaseViolation)
    : flightID(flightID), airline(airline), airlineType(airlineType), speedRecorded(speedRecorded), phaseViolation(phaseViolation){
        this->violationTimestamp = std::time(nullptr); //save current time
        auto currentTime = std::chrono::system_clock::now();
    }
};

class AVN {
public:
    int flightID;
    AirlineName airline;
    AirlineType airlineType;
    float speedRecorded;
    FlightPhase phaseViolation;
    time_t violationTimestamp;
    //for AVN generator
    float amountDue;
    bool status;  // true= paid, false= unpaid
    time_t dueDate;

    AVN () {}

    AVN(int flightID,AirlineName airline,AirlineType airlineType,float speedRecorded,FlightPhase phaseViolation,time_t violationTimestamp,float amountDue)
    : flightID(flightID), airline(airline), airlineType(airlineType), speedRecorded(speedRecorded), phaseViolation(phaseViolation),violationTimestamp(violationTimestamp),amountDue(amountDue){
        //due date
        std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(violationTimestamp);
        auto futureTime = tp + std::chrono::hours(24 * 3); // add 3 days
        dueDate = std::chrono::system_clock::to_time_t(futureTime);

    }

    void printAVN() {
        std::cout<<"\n> AVN CHALLAN\n";
        std::cout << "> AVN Issued: Flight " << flightID<<std::endl;
        std::cout << "> Speed: " << speedRecorded<<" km/h\n";
        std::cout << "> Phase: ";
        printPhase(phaseViolation);
        std::cout << "\n> Timestamp: " << ctime(&violationTimestamp);
        std::cout << "> Amount: $" << amountDue << std::endl;
        std::cout << "> Due: " << ctime(&dueDate)<<std::endl;
    }
};

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

//make the 3 runways
Runway rwyA('A'); 
Runway rwyB('B'); 
Runway rwyC('C');


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
                
                dashboard.requestAVN(flight);
                printf("recived\n");
                dashboard.printAVNs();

                violation[phaseIndex]=true;
            }
            sleep(1);
            //usleep(1000);
        }
        pthread_exit(nullptr);
    }
};
ATCDashboard Radar::dashboard;



struct args{
    Flight* flight;
    char runway;
};

void* simulateWaitingAtGate(void* arg){

    args* flightArg = static_cast<args*>(arg);
    Flight* flight = flightArg->flight;
    const char runwayToAcquire = flightArg->runway;

    while (1){
        //flight will wait at gate stationary until runway clears
        flight->phase = FlightPhase::GATE;
        //flight will wait at gate stationary until runway clears
        //if any fault, just tow and remove from queue???
        while (!flightTurn[(flight->id)-1]){
            flight->waitingTime++;
            sleep(1);
            if (int(flight->waitingTime)%100 == 0)
                flight->priority++; //up the priority every 100s
        }
        
        if (runwayToAcquire=='C')
            rwyC.acquireRunway();
        else
            rwyB.acquireRunway();

        int status = flight->simulateFlightDeparture();

        if (runwayToAcquire=='C')
            rwyC.releaseRunway();
        else
            rwyB.releaseRunway();

        if (status == 1){ //finished
            break;
        }    
    }   
    flight->phase = FlightPhase::DONE;
    pthread_exit(NULL);
}

void* simulateWaitingInHolding(void* arg){

    args* flightArg = static_cast<args*>(arg);
    Flight* flight = flightArg->flight;
    const char runwayToAcquire = flightArg->runway;
    cout<<"IN holding flight: "<< flight->id<<endl;
    while (1){
        //keep holding around airport until its your turn

        flight->phase = FlightPhase::HOLDING;
        while (!flightTurn[(flight->id)-1]){
            flight->speed += (10-rand()%20);//fluctuate by ~10
            flight->consumeFuel();
            flight->waitingTime++;
            if (int(flight->waitingTime)%100 == 0)
                flight->priority++; //up the priority every 100s
            sleep(1);
        }
            
        if (runwayToAcquire=='C')
            rwyC.acquireRunway();
        else
            rwyA.acquireRunway();

        int status = flight->simulateFlightArrival();
    
        if (runwayToAcquire=='C')
            rwyC.releaseRunway();
        else
            rwyA.releaseRunway();

        if (status == 1) //it was finished so send dont back to holding
            break;

    }
    flight->phase = FlightPhase::DONE;
    pthread_exit(NULL);
}

class Dispatcher{
public:
    Radar radar;

    pthread_mutex_t turnLock;
    pthread_cond_t cond;
    char runway;

    static pthread_t flightTid[TOTAL_FLIGHTS]; //all flight thread ids
    static pthread_t radarTid[TOTAL_FLIGHTS]; //all radar thread ids
    
    //for all simulation flights 
    QueueFlights* internationalFlights;
    QueueFlights* domesticFlights;
    //for those whose arrival time has occured
    QueueFlights* internationalWaitingQueue; 
    QueueFlights* domesticWaitingQueue;

    Dispatcher(char runway,QueueFlights*& domesticQueue, QueueFlights*& interQueue): runway(runway){
       this->internationalFlights= interQueue;
       this->domesticFlights= domesticQueue;
       internationalWaitingQueue= new QueueFlights;
       domesticWaitingQueue = new QueueFlights;
    }
    
    static void* dispatchFlights(void* arg) {

        Dispatcher* dispatcher = (Dispatcher*)(arg);

        dispatcher->internationalFlights->sortQueue2();
        dispatcher->domesticFlights->sortQueue2();
        
        QueueFlights* internationalFlightsQueue = dispatcher->internationalFlights;
        QueueFlights* domesticFlightsQueue = dispatcher->domesticFlights;
        QueueFlights* internationalWaitingQueue = dispatcher->internationalWaitingQueue; 
        QueueFlights* domesticWaitingQueue = dispatcher->domesticWaitingQueue; 

        //first launch all the flights
        //international flights
        for (int i=0; i<internationalFlightsQueue->numInQueue(); i++){ 
            Flight* flight = (*internationalFlightsQueue)[i]; 
            int index = (flight->id) - 1;
            args* flightArg = new args;
            flightArg->flight = flight;
            flightArg->runway = dispatcher->runway;
            if (flight->isArrival())
                pthread_create(&flightTid[index],NULL,simulateWaitingInHolding,(void*)flightArg);
            else
                pthread_create(&flightTid[index],NULL,simulateWaitingAtGate,(void*)flightArg);
            flight->thread_id = flightTid[index]; 
            pthread_create(&(flight->radar_id), nullptr, dispatcher->radar.flightRadar, (void*)flight);
        }
        //domestic flights
        for (int i=0; i<domesticFlightsQueue->numInQueue(); i++){ 
            Flight* flight = (*domesticFlightsQueue)[i]; 
            int index = (flight->id) - 1;
            args* flightArg = new args;
            flightArg->flight = flight;
            flightArg->runway = dispatcher->runway;
           if (flight->isArrival())
                pthread_create(&flightTid[index],NULL,simulateWaitingInHolding,(void*)flightArg);
            else
                pthread_create(&flightTid[index],NULL,simulateWaitingAtGate,(void*)flightArg);
                flight->thread_id = flightTid[index]; 
            pthread_create(&(flight->radar_id), nullptr, dispatcher->radar.flightRadar, (void*)flight);
        }
        sleep(1); //this is the bandaid holding this tgt

        Flight* currentFlight = nullptr;
        int currentFlightID = -1;
        int x=1;

        // START AIR TRAFFIC CONTROL OF ALL FLIGHTS BY FCFS/PRIORITY
        while ( !internationalFlightsQueue->isEmpty() || !domesticFlightsQueue->isEmpty() || !domesticWaitingQueue->isEmpty() || !internationalWaitingQueue->isEmpty()/*&& Timer::currentTime < SIMULATION_DURATION*/) {

            //dispatch the front (FCFS) if time for it 
            Flight* readyInternational = nullptr;
            Flight* readyDomestic = nullptr;
            
            if (dispatcher->runway == 'A' || dispatcher->runway == 'C'){
                if (Timer::currentTime % 120 == 0 && !internationalFlightsQueue->isEmpty()){ //move an international arrival to waiting Q
                    readyInternational = internationalFlightsQueue->getNextFlight();
                    if (readyInternational->isArrival()){
                        internationalWaitingQueue->addFlight(readyInternational); //add to waiting
                        printf("time for flight %d\n",readyInternational->id);
                    }
                    else
                        internationalFlightsQueue->addFlightAtFront(readyInternational); //add back (C)
                }
                if (Timer::currentTime % 180 == 0 && !domesticFlightsQueue->isEmpty()){ //move a domestic arrival to waiting Q
                    readyDomestic = domesticFlightsQueue->getNextFlight();
                    if (readyDomestic->isArrival()){
                        printf("time for flight %d\n",readyDomestic->id);
                        domesticWaitingQueue->addFlight(readyDomestic);
                    }
                    else
                        domesticFlightsQueue->addFlightAtFront(readyDomestic);
                }
            }
            else if (dispatcher->runway == 'B'){
                if (Timer::currentTime % 150 == 0 && !internationalFlightsQueue->isEmpty()){ //move a international departure to waiting Q
                    readyInternational = internationalFlightsQueue->getNextFlight();
                    if (readyInternational->isDeparture()){
                        printf("time for flight %d\n",readyInternational->id);
                        internationalWaitingQueue->addFlight(readyInternational); //add to waiting
                    }
                    else
                        internationalFlightsQueue->addFlightAtFront(readyInternational); //add back (C)
                }
                if (Timer::currentTime % 240 == 0 && !domesticFlightsQueue->isEmpty()){ //move a domestic departure to waiting Q
                    readyDomestic = domesticFlightsQueue->getNextFlight();
                    if (readyDomestic->isDeparture()){
                        domesticWaitingQueue->addFlight(readyDomestic);
                        printf("time for flight %d\n",readyInternational->id);
                    }
                    else
                        domesticFlightsQueue->addFlightAtFront(readyDomestic);                
                }
            }

            //sort the waiting Qs to choose priority flights
            internationalWaitingQueue->sortQueue();
            domesticWaitingQueue->sortQueue();

            Flight* nextInternational = nullptr;
            Flight* nextDomestic = nullptr;

            if (!internationalWaitingQueue->isEmpty())
                nextInternational = internationalWaitingQueue->getNextFlight();
            if (!domesticWaitingQueue->isEmpty())
                nextDomestic = domesticWaitingQueue->getNextFlight();

            //if none do nothing
            //if both choose one
            Flight* nextFlight = nullptr;
            if (nextInternational && nextDomestic){
                nextFlight = ( nextDomestic->priority > nextInternational->priority? nextDomestic:nextInternational);
                if(nextFlight != nextDomestic){
                    domesticWaitingQueue->addFlightAtFront(nextDomestic);
                    nextDomestic = nullptr;
                }
                else if (nextFlight != nextInternational){
                    internationalWaitingQueue->addFlightAtFront(nextInternational);
                    nextInternational = nullptr;
                }
            }
            else if (nextInternational && !nextDomestic)
                nextFlight = nextInternational;
            else if (nextDomestic && !nextInternational)
                nextFlight = nextDomestic;

            // check if anyone currently running do we pre-empt
            if (nextFlight != nullptr) {
                if (currentFlight == nullptr){
                    //just assign
                    flightTurn [nextFlight->id-1] = true;
                    currentFlight = nextFlight;
                    currentFlightID = currentFlight->id;
                }
                else{
                    //check if can pre-empt
                    if (dispatcher->runway == 'A' && nextFlight->priority > currentFlight->priority && currentFlight->phase < FlightPhase::LANDING){
                        //pre-empt current
                        printf("flight %d pre=empting flight %d\n",nextFlight->id,currentFlight->id);
                        flightTurn[currentFlight->id - 1]=false;
                        sleep(1); //give it some time to go back to holding
                        flightTurn [nextFlight->id-1] = true;
                        currentFlight = nextFlight;
                        currentFlightID = currentFlight->id;
                    }
                    else if (dispatcher->runway == 'B' && nextFlight->priority > currentFlight->priority && currentFlight->phase < FlightPhase::TAKEOFF){
                        //pre-empt current
                        printf("flight %d pre=empting flight %d\n",nextFlight->id,currentFlight->id);
                        flightTurn[currentFlight->id - 1]=false;
                        sleep(1); //give it some time to go back to holding
                        flightTurn [nextFlight->id-1] = true;
                        currentFlight = nextFlight;
                        currentFlightID = currentFlight->id;
                    }
                    else if (dispatcher->runway == 'C' && nextFlight->priority > currentFlight->priority){
                        //if opposite types, if current flight is departure and < takeoff, preempt
                        // else dont preempt
                        // of same types so check for each type, and their phases and preempt accordingly
                        printf("C pre-empt\n");
                        if(currentFlight->isArrival() && currentFlight->phase<FlightPhase:: LANDING
                            &&nextFlight->isDeparture()){
                            flightTurn[currentFlight->id -1]=false;
                            sleep(1);
                            flightTurn[nextFlight->id -1]=true;
                            currentFlight = nextFlight;
                            currentFlightID = currentFlight->id;

                        }
                        else if( currentFlight->isDeparture() && currentFlight->phase<FlightPhase:: TAKEOFF
                            &&nextFlight->isArrival()){
                            flightTurn[currentFlight->id -1]=false;
                            sleep(1);
                            flightTurn[nextFlight->id -1]=true;
                            currentFlight = nextFlight;
                            currentFlightID = currentFlight->id;

                        }
                        else if(currentFlight->isArrival() &&currentFlight->phase<FlightPhase:: LANDING){
                            flightTurn[currentFlight->id -1]=false;
                            sleep(1);
                            flightTurn[nextFlight->id -1]=true;
                            currentFlight = nextFlight;
                            currentFlightID = currentFlight->id;

                        }
                        else if(currentFlight->isDeparture() &&currentFlight->phase<FlightPhase:: TAKEOFF){
                            flightTurn[currentFlight->id -1]=false;
                            sleep(1);
                            flightTurn[nextFlight->id -1]=true;
                            currentFlight = nextFlight;
                            currentFlightID = currentFlight->id;

                        }

                    }
                    else{ //cannot pre-empt
                        if(nextFlight->isDomestic())
                            domesticWaitingQueue->addFlightAtFront(nextFlight);
                        else if (nextFlight->isInternational()){
                            internationalWaitingQueue->addFlightAtFront(nextFlight);
                        }
                    }
                }
                nextFlight = nullptr;
            }
            if (currentFlight && currentFlight->phase == FlightPhase::DONE){ //its done
                flightTurn[currentFlightID-1]=false;
                currentFlightID = -1;
                free(currentFlight);
                currentFlight = nullptr;
                //printf("unset turn");
            }
            else{
                //printf("current flight: %d\n",currentFlight->id);
            }

            sleep(1);
        }
        printf("all flights done\n");
        pthread_exit(nullptr);
    }

};
pthread_t Dispatcher::flightTid[TOTAL_FLIGHTS];
pthread_t Dispatcher::radarTid[TOTAL_FLIGHTS];

FlightType getFlightType(char dir){
     dir = toupper(dir); // Normalize

    while(dir != 'N' && dir != 'S' && dir != 'E' && dir !='W'){
        cout << "Invalid Direction!\nEnter Again (N/S/E/W): ";
        cin.clear();                 // clear error flags
        cin.ignore(10000, '\n');     // discard bad input
        cin >> dir;
        dir = toupper(dir);          // normalize again
    }

    if (dir == 'N') return FlightType::INTERNATIONAL_ARRIVAL;
    if (dir == 'E') return FlightType::INTERNATIONAL_DEPARTURE;
    if (dir == 'S') return FlightType::DOMESTIC_ARRIVAL;
    return FlightType::DOMESTIC_DEPARTURE;  // must be 'W'
}
AirlineName getAirlineName(int name){
    while(name <1 || name >6){
        cout<<"Invalid Name!\nEnter Again: ";
        cin>> name;
    }
    if( name == 1)
            return AirlineName:: PIA;
    else if( name==2)
           return AirlineName:: AirBlue;
    else if( name==3)
            return AirlineName:: FedEx;
    else if( name==4)
            return AirlineName:: Pakistan_Airforce;
    else if( name==5)
            return AirlineName:: Blue_Dart;
    else 
        return AirlineName:: AghaKhan_Air_Ambulance;
}

AirlineType getAirtype(int type){
    while(type <1 || type >4){
        cout<<"Invalid Type!\nEnter Again: ";
        cin>> type;
    }
    if( type == 1)
            return AirlineType:: MEDICAL;
    else if( type==2)
           return AirlineType:: MILITARY;
    else if( type==3)
            return AirlineType:: CARGO;
    else 
        return AirlineType:: COMMERCIAL;
}

int checkTime(float time){
    while(time <0.00 || time >=24.00){
        cout<<"Invalid Time!\nEnter again: ";
        cin>> time;
    }
    return time;
}
int main() {

    srand(time(NULL));
    initializeFlightPhases();
 

    // INPUT FLIGHTS
    QueueFlights* rwyAInternationalFlights = new QueueFlights;
    QueueFlights* rwyADomesticFlights = new QueueFlights;

    QueueFlights* rwyBInternationalFlights = new QueueFlights;
    QueueFlights* rwyBDomesticFlights = new QueueFlights;

    QueueFlights* rwyCInternationalFlights = new QueueFlights;
    QueueFlights* rwyCDomesticFlights = new QueueFlights;

    //make dispatchers for 3 runways (A is for arrival, B for departure and C for cargo (dept and arr))
    Dispatcher rwyADispatcher('A',rwyADomesticFlights,rwyAInternationalFlights);
    Dispatcher rwyBDispatcher('B',rwyBDomesticFlights,rwyBInternationalFlights);
    Dispatcher rwyCDispatcher('C',rwyCDomesticFlights,rwyCInternationalFlights);
    
    /*
    cout<<"        Airline          |    Type        | Flights\n";
    cout<<"---------------------------------------------------\n";
    cout<<"PIA                      | Commercial     | 4\n";
    cout<<"AirBlue                  | Commercial     | 4\n";
    cout<<"FedEx                    | Cargo          | 2\n";
    cout<<"Pakistan Airforce        | Military       | 1\n";
    cout<<"Blue Dart                | Cargo          | 2\n";
    cout<<"AghaKhan Air Ambulance   | Medical        | 1\n\n";

    cout<<"==================================================\n";
    cout<<"1)PIA\n";
    cout<<"2)AirBlue\n";
    cout<<"3)FedEx\n";
    cout<<"4)Pakistan Airforce\n";
    cout<<"5)Blue Dart\n";
    cout<<"6)AghaKhan Air Ambulance\n\n";

    cout<<"==================================================\n";
    cout<<"1)MEDICAL\n";
    cout<<"2)MILITARY\n";
    cout<<"3)CARGO\n";
    cout<<"4)COMMERCIAL\n\n";

    printf("Enter data for %d flights\n\n", TOTAL_FLIGHTS);

    FlightType Ftype; AirlineType airtype; AirlineName name;
    int id; char dir; int priority; int type; int airname; float scheduledTime;

    Flight* f;

    for(int i=0; i<TOTAL_FLIGHTS; i++){
        cout<<"Flight Number | Airline Name (1-6) | Aircraft Type (1-4) | Direction  (N/S/E/W)| Priority | Scheduled Time (24 hr clock)\n";
        cin>>id;
        cin>> airname;
        cin>> type;
        cin>>dir;
        cin>> priority;
        cin>>scheduledTime;
        scheduledTime= checkTime(scheduledTime);
        Ftype= getFlightType(dir);
        name= getAirlineName(airname);
        airtype= getAirtype(type);
        
        f= new Flight(id, Ftype, airtype, name, scheduledTime, dir);

        if(airtype == AirlineType:: CARGO){
            if(Ftype== INTERNATIONAL_ARRIVAL || Ftype== INTERNATIONAL_DEPARTURE)
                rwyCDispatcher.internationalFlights->addFlight(f);
            else
                rwyCDispatcher.domesticFlights->addFlight(f);
        }
        else if(f->isArrival()){
            if(Ftype== INTERNATIONAL_ARRIVAL)
                rwyADispatcher.internationalFlights->addFlight(f);
            else
                rwyADispatcher.domesticFlights->addFlight(f);
        }
        else if(f->isDeparture()){
           if(Ftype== INTERNATIONAL_DEPARTURE)
                rwyBDispatcher.internationalFlights->addFlight(f);
            else
                rwyBDispatcher.domesticFlights->addFlight(f);
        }
        f->printStatus();
    }
    */

    pthread_t dispatcherA;
    pthread_t dispatcherB;
    pthread_t dispatcherC;

    Timer timer;
    pthread_t timerThread;

    Flight* f1 = new Flight(1, FlightType:: INTERNATIONAL_ARRIVAL, AirlineType:: COMMERCIAL, AirlineName:: PIA,12.0,'N');
    rwyADispatcher.internationalFlights->addFlight(f1);

    pthread_create(&dispatcherA, nullptr, Dispatcher::dispatchFlights, (void*)&rwyADispatcher);   
    //pthread_create(&dispatcherB, nullptr, Dispatcher::dispatchFlights, (void*)&rwyBDispatcher);
    //pthread_create(&dispatcherC, nullptr, Dispatcher::dispatchFlights, (void*)&rwyCDispatcher);

    sleep(1);
    pthread_create(&timerThread,NULL,timer.simulationTimer,NULL);

    pthread_join(dispatcherA, NULL);
    //pthread_join(dispatcherB, NULL);
    //pthread_join(dispatcherC, NULL);
    pthread_join(timerThread, NULL);
    return 0;
}