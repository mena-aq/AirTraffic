#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>


//macros
#define AVN_ACTIVE 1

//global constants(enums)
typedef enum{
    PIA,
    AirBlue,
    FedEx,
    Pakistan Airforce,
} Airline;

typedef enum{
    COMMERCIAL,
    CARGO,
    MILITARY,
    MEDICAL
}AirlineType;

typedef enum{
    DOMESTIC_ARRIVAL,
    DOMESTIC_DEPARTURE,
    NTERNATIONAL_ARRIVAL,
    INTERNATIONAL_DEPARTURE
}FlightType;

typedef enum{
    GATE,
    TAXI,
    TAKEOFF,
    CLIMB,
    CRUISE,
    HOLDING,
    APPROACH,
    LANDING
}FlightPhase;


// CLASSES

//const structs made for the airlines
typedef struct{
    char airline[20];
    AirlineType airline_type;
    int num_aircraft;
    int num_flight;
} Airline;


typedef struct {
    AirlineType flight_type; // domestic, international
    //Airline airline;
    char direction; //N,S,E,W
    float flight_time;
    FlightPhase phase; //based on flight time 
    float speed; //self controlled
    float altitude; //depends on speed
    int priority; //Arrival priority shall be based on fuel status, emergency level, or type (e.g., Emergency > VIP > Cargo > Commercial).
    float fuel_level;
} Flight;

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


typedef struct{
    Airline airline;
    int violation_status; 
    Flight* current_flight;
    int faulty; //flag
} Aircraft;
void startFlight(){
    //calls flight methods in loop
}
//aircraft

//3 const runways GLOBAL AND SHARED
typedef struct{
    char runway_id; //A,B,C
    pthread_mutex_t lock; //one plane at a time
} Runway;
//constructor
void acquireRunway(){
}
void releaseRunway(){
}

typedef struct{
    int flight_id;
    float amount_due; //total of all AVNs
    int status; //1 paid 0 unpaid
    float speed_recorded;
    FlightPhase phase_violation;
    char* violation_timestamp;
    char* due_date; //ctime
} AVN;
//constructor


typedef struct{
    int num_violations;
    AVN** avns; //list of all AVNs
} ATCDashboard;
void addViolation(){
    //listens from pipe/fifo from avn generator

}


typedef struct{
    float speed_lower_limit;
    float speed_upper_limit;  
    float altitude_upper_limit;
    float altitude_lower_limit;   
} Phase;












	
