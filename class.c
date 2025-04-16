#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define COMMERCIAL 1
#define CARGO 2
#define MILITARY 3
#define MEDICAL 4

#define DOMESTIC_ARRIVAL 1
#define INTERNATIONAL_DEPARTURE 2
#define DOMESTIC_DEPARTURE 3
#define INTERNATIONAL_ARRIVAL 4

#define AVN_ACTIVE 1

#define NUM_FLIGHT_PHASES 8
#define GATE 1
#define TAXI 2
//departure
#define TAKEOFF 3
#define CLIMB 
#define CRUISE 
//arrival
#define HOLDING 6
#define APPROACH 7
#define LANDING 8

//const structs made for the airlines
typedef struct{
    char airline[20];
    int airline_type; //from COMMERCIAL,CARGO,MILITARY,MEDICAL
    int num_aircraft;
    int num_flight;
} Airline;
void Airline(/*params*/){

}

typedef struct {
    int flight_type; // domestic, international
    //Airline airline;
    char direction; //N,S,E,W
    float flight_time;
    float speed; //self controlled
    float altitude; //depends on speed
    int phase; //based on flight time 
    int priority; //Arrival priority shall be based on fuel status, emergency level, or type (e.g., Emergency > VIP > Cargo > Commercial).
    float fuel_level;
} Flight;
void Flight(/*params*/){ //C++ assuming
    //priority of type
}
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
    //mutex to access runway ???(can keep outside idk)
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
    int phase_violation;
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
    //speeds for every phase
    float speed_lower_limit;
    float speed_upper_limit;  
    float altitude_upper_limit;
    float altitude_lower_limit;   
} Phase;
//consytcutor


// can remove class and just keep globals
typedef struct{
    //clock (to spawn flights)
    Phase phaseRules[NUM_FLIGHT_PHASES];
} ATCRadar;
//constructor
void ATCRadarCheckSpeed(Flight* flight){ //or flight*& in cpp
    int violation = 0; //flag
    switch(phase){
        case GATE:
            break;
        case TAXI:
            break;
        case TAKEOFF:
            break;
        case CLIMB:
            break;
        case CRUISE:
            break;
        case HOLDING:
            break;
        case APPROACH:
            break;
        case LANDING:
            break;
    }
}










	
