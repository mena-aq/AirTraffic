#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

//macros
#define AVN_ACTIVE 1

//global constants(enums)
enum AirlineType{
    COMMERCIAL,
    CARGO,
    MILITARY,
    MEDICAL
};

enum FlightType{
    DOMESTIC_ARRIVAL,
    DOMESTIC_DEPARTURE,
    NTERNATIONAL_ARRIVAL,
    INTERNATIONAL_DEPARTURE
};

enum FlightPhase{
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
struct Airline{
    char airline[20];
    AirlineType airline_type; //from COMMERCIAL,CARGO,MILITARY,MEDICAL
    int num_aircraft;
    int num_flight;
};

struct Flight{
    FlightType flightType; // domestic, international
    char direction; //N,S,E,W
    float flightTime; //delta since runway acquired
    FlightPhase phase; //based on flight time 
    float speed; //self controlled
    float altitude; //depends on speed
    int priority; //Arrival priority shall be based on fuel status, emergency level, or type (e.g., Emergency > VIP > Cargo > Commercial).
    float fuelLevel;

    void changePhase(){
        //change phase & see restrictions
    }
    void updateAltitude(){ //according to speed
    
    }
    void consumeFuel(){
    
    }
    void updatePriority(){
        //fuel
        //emergency level
    }
    void controlFlightSpeed(){
        //the plane flying in its diff phases... speed
    }
};

struct Aircraft{
    Airline airline;
    int violationStatus; 
    Flight* currentFlight;
    int faulty; //flag

    void startFlight(){
        //calls flight methods in loop
    }
};

struct Runway{
private:
    pthread_mutex_t lock; //one plane at a time
public:
    char runwayID; //A,B,C

    void acquireRunway(){
        pthread_mutex_lock(&lock);
    }
    void releaseRunway(){
        pthread_mutex_unlock(&lock);
    }
};

struct AVN{
    int flightID;
    float amountDue; //total of all AVNs
    int status; //1 paid 0 unpaid
    float speedRecorded;
    FlightPhase phaseViolation;
    char* violationTimestamp;
    char* dueDate; 
};

struct ATCDashboard{
    int num_violations;
    int** avns; //list of all aircrafts w/ violations

    void addViolation(){
        //listens from pipe/fifo from avn generator
    
    }
};

struct Phase{
    float speed_lower_limit;
    float speed_upper_limit;  
    float altitude_upper_limit;
    float altitude_lower_limit;   
};











	