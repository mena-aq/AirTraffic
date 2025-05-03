#ifndef FLIGHT_H
#define FLIGHT_H

#include "PhaseRules.h"


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
    PhaseRules flightPhases;


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

    int simulateFlightDeparture(bool flightTurn[]) {                                        
    
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

    int simulateFlightArrival(bool flightTurn[]){  
    
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

#endif