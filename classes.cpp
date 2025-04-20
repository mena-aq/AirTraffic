#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <string>
#include <vector>

#define AVN_ACTIVE 1

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

#define NUM_FLIGHT_PHASES 8
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

// STRUCTS/CLASSES
class Airline {
public:
    std::string name;
    AirlineType type;
    int numAircraft;
    int numFlights;

    Airline(std::string name = "", AirlineType type = AirlineType::COMMERCIAL, int numAircraft = 0, int numFlights = 0)
        : name(std::move(name)), type(type), numAircraft(numAircraft), numFlights(numFlights) {}
};


class Flight {
public:
    FlightType type;
    char direction; // N, S, E, W
    float entryTime ;
    FlightPhase phase;
    float speed;
    float altitude;
    int priority;
    float fuelLevel;
    
    Flight(FlightType flightType,char dir,AirlineType airlineType){
        this->type = flightType;
        this->direction = dir;

        this->entryTime=0; //when allocated runway, update
        if (this->type== FlightType::DOMESTIC_ARRIVAL||this->type==FlightType::INTERNATIONAL_ARRIVAL){
            this->phase= FlightPhase::HOLDING;
            this->speed= 400 + rand()%201;
            //this->altitude=;
            //this->fuelLevel=;
        }
        else{
            this->phase=FlightPhase::GATE;
            this->speed=0;
            //this->altitude=
            //this->fuelLevel=
        }
        this->priority=0;//based on airline type
    }

    void changePhase() {
        // TODO: Implement phase transitions
    }

    void updateAltitude() {
        // TODO: Implement based on speed
    }

    void consumeFuel() {
        // TODO: Decrease based on activity
    }

    void updatePriority() {
        // TODO: Based on fuel, emergencies, etc.
    }

    void controlFlightSpeed() {
        // TODO: Vary speed by phase
    }
};

class Aircraft {
public:
    std::string airline;
    int violationStatus;
    Flight* currentFlight;
    bool faulty;

    Aircraft(std::string airline){
        this->airline=airline;
        this->violationStatus=0;
        this->currentFlight=nullptr;
        this->faulty=0;
    }
};

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

/*
class AVN {
public:
    int flightID;
    float amountDue;
    bool status; // true = paid, false = unpaid
    float speedRecorded;
    FlightPhase phaseViolation;
    time_t violationTimestamp;
    time_t dueDate;

    AVN(int id, float sp, float amount, FlightPhase phase): flightID(id), amountDue(amount), status(false), speed(sp), FlightPhase(phase){
        violationTimestamp= std::time(nullptr);  // save current time
        
        //due date
        auto currentTime = std::chrono::system_clock::now();
         // Add 3 days to current time (3 * 24 * 60 * 60 seconds)
        auto futureTime = currentTime + std::chrono::hours(24 * 3);
        // Convert futureTime to time_t 
        dueDate= std::chrono::system_clock::to_time_t(futureTime);
    }
};
*/
/*
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
*/

class Phase {
public:
    float speedLowerLimit;
    float speedUpperLimit;
    float altitudeLowerLimit;
    float altitudeUpperLimit;
    Phase():speedLowerLimit(0.0), speedUpperLimit(0.0), altitudeLowerLimit(0.0), altitudeUpperLimit(0.0){}
    Phase(float sLower,float sUpper,float aLower,float aUpper):speedLowerLimit(sLower),speedUpperLimit(sUpper),altitudeLowerLimit(aLower),altitudeUpperLimit(aUpper){}
};




int main(){



}

/// ALTITUDES in ft

/// holding 14000 to 6000
// approach 6000 to 800
// landing 800 to 0

//takeoff 0-500
//climb 500-10000
//cruise 10000??

//SPEEDS in kmh

//holding 400-600
//approach 240-290
//landing 240->30
//taxi 15-30
//gate 0-5/10
// takeoff 0-290
// climb 250-463
// cruise 800-900
