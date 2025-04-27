#include <iostream>
#include <string>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <ctime>
#include <chrono>

using namespace std;

#define AVN_ACTIVE 1
#define NUM_FLIGHT_PHASES 8
#define SIMULATION_DURATION 300
#define NUM_FLIGHTS 14  

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

enum class Direction{
    NORTH,
    SOUTH,
    EAST,
    WEST
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

/// ALTITUDES in ft

/// holding 14000 to 6000
// approach 6000 to 800
// landing 800 to 0

//takeoff 0-500
//climb 500-10,000
//cruise 10,000

//SPEEDS in kmh

//holding 400-600
//approach 240-290
//taxi 15-30
// takeoff 0-290
// climb 250-463
// cruise 800-900

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

// Airline structure
class Airline {
public:
    string name;
    AirlineType type;
    int numAircraft;
    int numFlights;

    Airline(string name = "", AirlineType type = AirlineType::COMMERCIAL, int numAircraft = 0, int numFlights = 0)
        : name(name), type(type), numAircraft(numAircraft), numFlights(numFlights) {}
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
        cout<<"\n> AVN CHALLAN\n";
        cout << "> AVN Issued: Flight " << flightID<<endl;
        cout << "> Speed: " << speedRecorded<<" km/h\n";
        cout << "> Phase: " << (int)phaseViolation<<endl;
        cout << "> Due: " << ctime(&dueDate)<<endl;
    }
};

// Flight structure
class Flight {
public:
    int id;
    FlightType type;
    AirlineType airlineType; 
    char direction; // N, S, E, W
    float entryTime;
    FlightPhase phase;
    float speed;
    float altitude;
    int priority;
    float fuelLevel;
    bool faulty;

    Flight(int id, FlightType flightType, AirlineType airlineType) : id(id), type(flightType), airlineType(airlineType), entryTime(0), faulty(false){
        if (type == FlightType::DOMESTIC_ARRIVAL || type == FlightType::INTERNATIONAL_ARRIVAL) {
            phase = FlightPhase::HOLDING; // if its arriving, start phase is holding
            speed = 400 + rand() % 201;  // 400 - 600 random time
            altitude = 6000; // 6000 starting
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

    /*
    bool checkViolation() {
        Phase p = flightPhases[(int)phase];
        return (speed < p.speedLowerLimit || speed > p.speedUpperLimit);
    }
    */

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
        cout << "> Flight " << id << " | Speed: " << speed
             << " km/h | Alt: " << altitude
             << " ft | Phase: " << (int)phase
             << " | Fuel: " << fuelLevel
             << " | Priority: " << priority << "\n";
    }
};


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

class ATCDashboard {
public:
    int numViolations;
    std::vector<std::vector<int>> avns;

    ATCDashboard(){
        this->numViolations=0;
    }

    void addViolation() {
        // Simulate FIFO/pipe listening
        // Add AVN violation
    }
};

//for dispatcher
struct Dispatch{
    Direction direction;
    FlightType flightType;
    int interval; // in seconds
    int emergencyProb; //% age
};

// Simulate flight and AVN check
void* simulateFlightDeparture(void* arg) {
    Flight* flight = static_cast<Flight*>(arg);

    //10kmh/s
    while(1){
        flight->speed+=10;
        
        flight->printStatus();
        sleep(1);
    }

    /*for (int i = 0; i < SIMULATION_DURATION && !flight->faulty; ++i) {

        //flight->simulateTick();

        //gate->taxi
        


        
        if (flight->checkViolation()) {
        
            int amount=0;

            if(flight->airlineType == AirlineType:: COMMERCIAL){
                amount= 500000;
                amount+= 0.15*amount;
            }
            else if (flight->airlineType == AirlineType:: CARGO){
                amount= 700000;
                amount+= 0.15*amount;
            }

            AVN avn(flight->id, flight->speed, amount, flight->phase);
            avn.printAVN();
        }
        
        flight->printStatus();
        sleep(1);
    }
    */

    pthread_exit(nullptr);
}

vector<Flight*> flights;
int currentFlight=0;
pthread_mutex_t flightMutex; // to protect flight vector, if radar and dispatcher access it at the same time
// idk if we NEED this??

/*
//interval is according to 5 min simulation as given in doc
vector<Dispatch> dispatchSchedule = {
    {Direction::NORTH, FlightType::INTERNATIONAL_ARRIVAL, 180, 10},
    {Direction::SOUTH, FlightType::DOMESTIC_ARRIVAL, 120, 5},
    {Direction::EAST, FlightType::INTERNATIONAL_DEPARTURE,150, 15},
    {Direction::WEST, FlightType::DOMESTIC_DEPARTURE, 240, 20}
};

vector<Airline> airlines = {
        {"PIA", AirlineType::COMMERCIAL, 6, 4},
        {"AirBlue", AirlineType::COMMERCIAL, 4, 4},
        {"FedEx", AirlineType::CARGO, 3, 2},
        {"Pakistan Airforce", AirlineType::MILITARY, 2, 1},
        {"Blue Dart", AirlineType::CARGO, 2, 2},
        {"AghaKhan Air Ambulance", AirlineType::MEDICAL, 2, 1}
 };

void* createInitialFlights(void * arg) {
    for (const Airline& a : airlines) {
        for (int i = 0; i < a.numFlights; i++) {
            FlightType type;
            if (a.type == AirlineType::COMMERCIAL){
                int t = i % 2;
                if(t==0)
                    type=FlightType::DOMESTIC_ARRIVAL;
                else
                    FlightType::DOMESTIC_DEPARTURE;
            }
            else if (a.type == AirlineType::CARGO){
                    type= FlightType::DOMESTIC_DEPARTURE;
            }
            else if (a.type == AirlineType::MILITARY)
                type = FlightType::INTERNATIONAL_DEPARTURE;
            else
                type = FlightType::INTERNATIONAL_ARRIVAL;

            Flight* f = new Flight(++currentFlight, type, 'N', a.type);  // set the direction

            pthread_mutex_lock(&flightMutex);
            flights.push_back(f);
            pthread_mutex_unlock(&flightMutex);
            cout << "Created Flight for " << a.name << " ID: " << f->id << "\n";
        }
    }
    pthread_exit(NULL);
}
*/

void* simulationTimer(void* arg) {
    time_t start = time(nullptr);

    while ( difftime(time(nullptr), start) < SIMULATION_DURATION) {

        int current = difftime(time(nullptr), start);
        cout << "\rSimulation Time: " << current << "s / " << SIMULATION_DURATION << "s" << flush;
        sleep(1);
    }
    pthread_exit(nullptr);
}

int main() {
    srand(time(nullptr));

    //pthread_t dispatcherThread;
    // create flights according to the schedule
    //pthread_create(&dispatcherThread, nullptr, createInitialFlights, nullptr);
    //pthread_join(dispatcherThread, NULL);

    //test with a single flight for now
    //Flight test(1,FlightType::DOMESTIC_DEPARTURE,AirlineType::COMMERCIAL);
    //start flight simulation
    // for now start the first flight
    pthread_t tid;
    pthread_create(&tid,nullptr,simulationTimer,NULL); //create flight thread
    pthread_join(tid,NULL);

    return 0;
}

