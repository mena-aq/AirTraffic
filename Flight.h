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

    int flagged;

    float xPos;
    float yPos;
    sf::Texture texture;
    sf::Sprite sprite;
    sf::Text label;


    Flight(int id, FlightType flightType, AirlineType airlineType, AirlineName name,float st, char dir ) :  id(id), direction(dir), scheduledTime(st), thread_id(-1), radar_id(-1), airlineName(name), airlineType(airlineType), flightType(flightType), waitingTime(0), faulty(false), flagged(false){    
        
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

        texture.loadFromFile("./flight.png");
        sprite.setTexture(texture);
        if (this->isDeparture())
            sprite.setScale(0.25f,0.25f);
        else
            sprite.setScale(0.35f,0.35f);
        sf::FloatRect bounds = sprite.getLocalBounds();
        sprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f);

        if (this->direction == 'E'){
            sprite.setRotation(-90.f);
            this->xPos = 220;
            this->yPos = 230;
            updatePosition();
        }
        else if (this->direction == 'W'){
            sprite.setRotation(90.f);
            this->xPos = 750;
            this->yPos = 200;
            updatePosition();
        }
        else if (this->direction == 'N'){
            sprite.setRotation(180.f);
            this->xPos = 900;
            this->yPos = -220;
            updatePosition();
        }
        else if (this->direction == 'S'){
            this->xPos = 900;
            this->yPos = 1080;
            updatePosition();
        }
        if (this->isArrival() && airlineType == AirlineType::CARGO){//special case of emergency arrival
            this->xPos = 1150;
            this->yPos = 450;
            updatePosition();
        }
        
        label.setFont(globalFont);
        string lbl = "ID: " + to_string(this->id);
        label.setString(lbl);
        label.setCharacterSize(15);
        label.setFillColor(getAirlineColorCode(this->airlineName));
        label.setOutlineColor(sf::Color::White);
        label.setOutlineThickness(2); 
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

    bool isEmergency(){
        return (this->airlineType==AirlineType::MEDICAL || this->airlineType==AirlineType::MILITARY);
    }

    void sendBackToGate(){
        if (this->direction == 'E'){
            sprite.setRotation(-90.f);
            this->xPos = 220;
            this->yPos = 230;
            updatePosition();
        }
        else if (this->direction == 'W'){
            sprite.setRotation(90.f);
            this->xPos = 750;
            this->yPos = 200;
            updatePosition();
        }
    }

    void sendBackToHolding(){
        if (this->direction == 'N'){
            sprite.setRotation(180.f);
            this->xPos = 900;
            this->yPos = -220;
            updatePosition();
        }
        else if (this->direction == 'S'){
            this->xPos = 900;
            this->yPos = 1080;
            updatePosition();
        }
        if (this->isArrival() && airlineType == AirlineType::CARGO){
            this->xPos = 1150;
            this->yPos = 450;
            updatePosition();
        }
    }

    void useArrivalEmergencyRoute(){ //rwyC arrival
        this->xPos = 1150; 
        this->yPos = 450;
        updatePosition();
    }

    void useDepartureEmergencyRoute(){ //rwyC departure
        sprite.setRotation(-90.f);
        this->xPos = 220;
        this->yPos = 230;
        updatePosition();
    }

    //operator for priority 
    bool operator<(const Flight& other) const{
        if (this->priority!=other.priority)
            return (this->priority>other.priority); //priority sched //im doing fuel alr         
        return this->id < other.id;                       // FCFS    
    }

    int simulateInternationalFlightDeparture(bool flightTurn[]) {   
        
        float step = 1; //-0.05 for domestic (runway dir wise)
    
        printf("DEPARTURE: flight ID: %d\n",this->id);
    
        // ------------- SIMULATE --------------
    
        // START TAXI (0->(15-30))
        float distanceToRunway = TERMINAL_TO_RUNWAY_LEN;
        this->phase = FlightPhase::TAXI;
        while (this->speed<15 && xPos>165){
            this->speed += 2 + (rand()%50)/10.0; //speed up by 2 - 2.5km/s
            distanceToRunway -= this->speed/3600; //s=vt
            this->xPos-= this->speed * step;
            this->updatePosition();
            this->consumeFuel();
            printf("Flight : %d Distance to Runway= %f. Speed=%f\n", this->id, distanceToRunway,this->speed);
            if (!flightTurn[this->id-1]){ // somebody pre-empted it, go back to holding
                return 0; //exit w/ unfinished status
            }
            sleep(1);
            //usleep(1000);
        }
        sprite.rotate(-90.f);
        // KEEP TAXIING UNTIL NEAR RUNWAY
        while (yPos<320){
            this->speed += (5-rand()%10)/10.0; //vary by ~0.5
            distanceToRunway -= this->speed/3600; //s=vt
            this->yPos += this->speed * step;
            this->updatePosition();
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
            this->yPos += this->speed * step;
            this->updatePosition();
            printf("Flight : %d Distance to Runway= %f. Speed=%f\n",this->id,distanceToRunway, this->speed);
            sleep(1);
            //usleep(1000);
        }
        sprite.rotate(-90.f);
        //printf("Flight : %d Stopped at runway dist=%f, prepare to takeoff\n",flight->id, distanceToRunway); //ye submit krna h??? its wirtten in milestone 2 and class to nhi h 
        // TAKEOFF from standstill
        step/=4;
        this->phase = FlightPhase::TAKEOFF;
        float distanceAlongRunway = RUNWAY_LEN + distanceToRunway;
        while (xPos < 760){ //while runway left
            int a = 65050 + (1000-rand()%2000);
            this->speed += a/3600.0; //v=u+at
            this->xPos += this->speed * step;
            this->updatePosition();
            this->spriteAltitudeUp();
            distanceAlongRunway -= this->speed/3600.0 + 0.5*a*pow((1.0/3600),2); //ut+1/2at^2
            this->consumeFuel();
            printf("Flight : %d takeoff, speed=%f, altitude=%f, dist=%f\n",this->id, this->speed,this->altitude,distanceAlongRunway);
            sleep(1);
            //usleep(1000);
        }
        //printf("Flight : %d Takeoff at dist remaining=%f\n", flight->id, distanceAlongRunway);
        // CLIMB
        this->phase = FlightPhase::CLIMB;
        while (this->speed < 780){
            int a = 60000 + (500-rand()%1000);
            this->speed += a*(1.0/3600); //v=u+at
            this->altitude += 3280* (this->speed*sin(M_PI/12)*(1.0/3600) + 0.5*(a*sin(M_PI/12))*pow((1.0/3600),2));//ut+1/2at^2
            this->xPos += this->speed * step;
            this->updatePosition();
            this->spriteAltitudeUp();
            this->consumeFuel();
            printf("Flight : %d climb, speed=%f, altitude=%f\n", this->id,this->speed,this->altitude);
            sleep(1);
            //usleep(1000);
        }
        //CRUISE for a bit (bring to correct altitude) until out of airspace
        this->phase=FlightPhase::CRUISE;
        float cruisingAltitude = (flightPhases[FlightPhase::CRUISE].altitudeLowerLimit + flightPhases[FlightPhase::CRUISE].altitudeUpperLimit)/2.0;
        while (this->altitude < cruisingAltitude){
            int a = 25000 + (100-rand()%200);
            this->speed += a/3600.0; //v=u+at
            this->altitude += 200;
            this->spriteAltitudeUp();
            this->consumeFuel();
            sleep(1);
            //usleep(1000);
            //printf("Flight : %d cruise, speed=%f, altitude=%f\n", this->id, this->speed,this->altitude);
        }
        this->phase=FlightPhase::CRUISE;
        printf("Flight %d exiting..\n",this->id);
        return 1; //return w/ finished status

    }

    int simulateDomesticFlightDeparture(bool flightTurn[]) {   
        
        float step = 1; 
    
        printf("DEPARTURE: flight ID: %d\n",this->id);
    
        // ------------- SIMULATE --------------
    
        // START TAXI (0->(15-30))
        float distanceToRunway = TERMINAL_TO_RUNWAY_LEN;
        this->phase = FlightPhase::TAXI;
        while (this->speed<15 && xPos<830){
            this->speed += 1.8 + (rand()%20)/10.0; //speed up by 1.8 - 2 km/s
            distanceToRunway -= this->speed/3600; //s=vt
            this->xPos += this->speed * step;
            this->updatePosition();
            this->consumeFuel();
            printf("Flight : %d Distance to Runway= %f. Speed=%f\n", this->id, distanceToRunway,this->speed);
            if (!flightTurn[this->id-1]){ // somebody pre-empted it, go back to holding
                return 0; //exit w/ unfinished status
            }
            sleep(1);
            //usleep(1000);
        }
        sprite.rotate(90.f);
        // KEEP TAXIING UNTIL NEAR RUNWAY
        while (yPos<320){
            this->speed += (5-rand()%10)/10.0; //vary by ~0.5
            distanceToRunway -= this->speed/3600; //s=vt
            this->yPos += this->speed * step;
            this->updatePosition();
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
            this->yPos += this->speed * step;
            this->updatePosition();
            printf("Flight : %d Distance to Runway= %f. Speed=%f\n",this->id,distanceToRunway, this->speed);
            sleep(1);
            //usleep(1000);
        }
        sprite.rotate(90.f);
        //printf("Flight : %d Stopped at runway dist=%f, prepare to takeoff\n",flight->id, distanceToRunway); //ye submit krna h??? its wirtten in milestone 2 and class to nhi h 
        // TAKEOFF from standstill
        step/=4;
        this->phase = FlightPhase::TAKEOFF;
        float distanceAlongRunway = RUNWAY_LEN + distanceToRunway;
        while (xPos > 250){ //while runway left
            int a = 65050 + (1000-rand()%2000);
            this->speed += a/3600.0; //v=u+at
            this->xPos -= this->speed * step;
            this->updatePosition();
            this->spriteAltitudeUp();
            distanceAlongRunway -= this->speed/3600.0 + 0.5*a*pow((1.0/3600),2); //ut+1/2at^2
            this->consumeFuel();
            printf("Flight : %d takeoff, speed=%f, altitude=%f, dist=%f\n",this->id, this->speed,this->altitude,distanceAlongRunway);
            sleep(1);
            //usleep(1000);
        }
        //printf("Flight : %d Takeoff at dist remaining=%f\n", flight->id, distanceAlongRunway);
        // CLIMB
        this->phase = FlightPhase::CLIMB;
        while (this->speed < 780){
            int a = 60000 + (500-rand()%1000);
            this->speed += a*(1.0/3600); //v=u+at
            this->altitude += 3280* (this->speed*sin(M_PI/12)*(1.0/3600) + 0.5*(a*sin(M_PI/12))*pow((1.0/3600),2));//ut+1/2at^2
            this->xPos -= this->speed * step;
            this->updatePosition();
            this->spriteAltitudeUp();
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

    int simulateDomesticFlightArrival(bool flightTurn[]){  
    
        printf(" ARRIVAL :flight ID: %d\n",this->id);
    
        // ------------- SIMULATE --------------
    
        float distanceToRunway = 3.5; //for a safe-ish descent (50ft/s)
        
        float step = 0.05;
        //bring flight from HOLDING->APPROACH to avoid violation
        while(this->speed > 300){
            this->speed -= 20 + (1-rand()%2); //~1.5-2.5
            distanceToRunway -= this->speed*(1.0/3600);
            this->yPos -= this->speed * step;
            this->altitude -= 40; 
            this->updatePosition();
            //this->spriteAltitudeDown();
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
            this->yPos -= this->speed * step;
            this->altitude -= 50;   
            this->updatePosition();
            this->spriteAltitudeDown();
            this->consumeFuel();
            printf("flight %d : approach, speed=%f, altitude=%f, dist=%f\n",this->id, this->speed,this->altitude,distanceToRunway);
            if (!flightTurn[this->id-1]){ // somebody pre-empted it, go back to holding
                return 0;
            }
            sleep(1);
            //usleep(1000);
        }
        step *= 1.2;
        //LANDING
        this->phase = FlightPhase::LANDING;
        float distanceAlongRunway = distanceToRunway+RUNWAY_LEN; 
        while (this->yPos > 220 || this->altitude>0){
            float a = -30750 + (50-rand()%100);
            if (this->speed > 32)
                this->speed += a*(1.0/3600); //v=u+at
            this->yPos -= this->speed * step;
            distanceAlongRunway -= this->speed/3600.0 + 0.5*a*pow((1.0/3600),2); //ut+1/2at^2
            if (this->altitude>0){
                this->altitude -= 30;
                if (this->altitude<0)
                    this->altitude=0;
            }
            this->updatePosition();
            this->spriteAltitudeDown();
            this->consumeFuel();
            printf("flight %d : landing, speed=%f, altitude=%f, dist=%f\n",this->id, this->speed,this->altitude,distanceAlongRunway);
            sleep(1);
            //usleep(1000);
        }
            
        //TAXI
        this->phase = FlightPhase::TAXI;
        float distanceToGate = distanceAlongRunway+TERMINAL_TO_RUNWAY_LEN;
        while (yPos>200){
            if (this->speed<28)
                this->speed += (50-rand()%100)/100.0;//fluctuate by ~0.5
            else
                this->speed -= 0.5;
            this->yPos -= this->speed * step;
            this->updatePosition();
            distanceToGate -= this->speed*(1.0/3600);
            this->consumeFuel();
            printf("flight %d : taxi, speed=%f, dist=%f\n",this->id, this->speed,distanceToGate);
            sleep(1);
            //usleep(1000);
        }
        sprite.rotate(-90.f);
        //TAXI->GATE 
        step *= 4;
        while(this->speed>0 && xPos>700){
            //flight->speed += -1500*(1.0/3600); //v=u+at
            this->speed -= 0.5;
            if (this->speed<0)
                this->speed=0;
            distanceToGate -= this->speed*(1.0/3600);
            this->xPos -= this->speed * step;
            this->updatePosition();
            this->consumeFuel();
            printf("flight %d : taxi->gate, speed=%f, dist=%f\n",this->id, this->speed,distanceToGate);
            sleep(1);
            //usleep(1000);
        }
        this->phase = FlightPhase::GATE;
                
                    
        printf("Flight %d exiting..\n",this->id);

        return 1; //exit w/ finished status
    }

    int simulateInternationalFlightArrival(bool flightTurn[]){  
    
        printf(" ARRIVAL :flight ID: %d\n",this->id);
    
        // ------------- SIMULATE --------------
    
        float distanceToRunway = 3.5; //for a safe-ish descent (50ft/s)
        
        float step = 0.05;
        //bring flight from HOLDING->APPROACH to avoid violation
        while(this->speed > 300){
            this->speed -= 20 + (1-rand()%2); //~1.5-2.5
            distanceToRunway -= this->speed*(1.0/3600);
            this->yPos += this->speed * step;
            this->altitude -= 40; 
            this->updatePosition();
            this->consumeFuel();
            printf("flight %d : holding->approach, speed=%f, altitude=%f, dist=%f\n", this->id, this->speed,this->altitude,distanceToRunway);
            if (!flightTurn[this->id-1]){ // somebody pre-empted it, go back to holding
                return 0; //exit w/ unfinished status
            }
            sleep(1);
            //usleep(100000);
        }
        //APPROACH
        this->phase = FlightPhase::APPROACH;
        while(this->speed>242.5){
            this->speed += -5000*(1.0/3600); //v=u+at
            distanceToRunway -= this->speed * (1.0/3600); 
            this->yPos += this->speed * step;
            this->altitude -= 50;   
            this->updatePosition();
            //this->spriteAltitudeDown();
            this->consumeFuel();
            printf("flight %d : approach, speed=%f, altitude=%f, dist=%f\n",this->id, this->speed,this->altitude,distanceToRunway);
            if (!flightTurn[this->id-1]){ // somebody pre-empted it, go back to holding
                return 0;
            }
            sleep(1);
            //usleep(100000);
        }
        step *= 1.2;
        //LANDING
        this->phase = FlightPhase::LANDING;
        float distanceAlongRunway = distanceToRunway+RUNWAY_LEN; 
        while (yPos<540 || this->altitude>0 ){
            float a = -30750 + (50-rand()%100);
            if (this->speed > 32)
                this->speed += a*(1.0/3600); //v=u+at
            this->yPos += this->speed * step;
            distanceAlongRunway -= this->speed/3600.0 + 0.5*a*pow((1.0/3600),2); //ut+1/2at^2
            if (this->altitude>0){
                this->altitude -= 30;
                if (this->altitude<0)
                    this->altitude=0;
            }
            this->updatePosition();
            this->spriteAltitudeDown();
            this->consumeFuel();
            printf("flight %d : landing, speed=%f, altitude=%f, dist=%f\n",this->id, this->speed,this->altitude,distanceAlongRunway);
            //sleep(1);
            //if (yPos>540)
                //break;
            sleep(1);
            //usleep(100000);
        }
        sprite.rotate(90.f);
    
        //TAXI
        step *= 5;
        this->phase = FlightPhase::TAXI;
        float distanceToGate = distanceAlongRunway+TERMINAL_TO_RUNWAY_LEN;
        while (xPos>200 || yPos>220){
            if (this->speed<28)
                this->speed += (50-rand()%100)/100.0;//fluctuate by ~0.5
            else
                this->speed -= 0.5;
            if (xPos>200)
                this->xPos -= this->speed * step;
            else {
                sprite.setRotation(0.f);
                this->yPos -= this->speed * step;
            }
            this->updatePosition();
            distanceToGate -= this->speed*(1.0/3600);
            this->consumeFuel();
            printf("flight %d : taxi, speed=%f, dist=%f\n",this->id, this->speed,distanceToGate);
            sleep(1);
            //usleep(100000);
        }
        sprite.rotate(90.f);

        // maybe release the runway here??

        //TAXI->GATE 
        step *= 4;
        while(this->speed>0 && xPos<210){
            //flight->speed += -1500*(1.0/3600); //v=u+at
            this->speed -= 0.5;
            if (this->speed<0)
                this->speed=0;
            distanceToGate -= this->speed*(1.0/3600);
            this->xPos += this->speed * step;
            this->updatePosition();
            this->consumeFuel();
            printf("flight %d : taxi->gate, speed=%f, dist=%f\n",this->id, this->speed,distanceToGate);
            sleep(1);
            //usleep(100000);
        }
        this->phase = FlightPhase::GATE;
                
                    
        printf("Flight %d exiting..\n",this->id);

        return 1; //exit w/ finished status
    }

    int simulateEmergencyFlightDeparture(bool flightTurn[]) {   
        
        float step = 1; //-0.05 for domestic (runway dir wise)
    
        printf("EMERGENCY DEPARTURE: flight ID: %d\n",this->id);
    
        // ------------- SIMULATE --------------
    
        // START TAXI (0->(15-30))
        float distanceToRunway = TERMINAL_TO_RUNWAY_LEN;
        this->phase = FlightPhase::TAXI;
        while (this->speed<15 || xPos>150){
            this->speed += 2 + (rand()%50)/10.0; //speed up by 2 - 2.5km/s
            distanceToRunway -= this->speed/3600; //s=vt
            this->xPos-= this->speed * step;
            this->updatePosition();
            this->consumeFuel();
            printf("Flight : %d Distance to Runway= %f. Speed=%f\n", this->id, distanceToRunway,this->speed);
            if (!flightTurn[this->id-1]){ // somebody pre-empted it, go back to holding
                return 0; //exit w/ unfinished status
            }
            sleep(1);
            //usleep(1000);
        }
        sprite.rotate(-90.f);
        // KEEP TAXIING UNTIL NEAR RUNWAY
        while (yPos<420){
            this->speed += (5-rand()%10)/10.0; //vary by ~0.5
            distanceToRunway -= this->speed/3600; //s=vt
            this->yPos += this->speed * step;
            this->updatePosition();
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
            this->yPos += this->speed * step;
            this->updatePosition();
            printf("Flight : %d Distance to Runway= %f. Speed=%f\n",this->id,distanceToRunway, this->speed);
            sleep(1);
            //usleep(1000);
        }
        sprite.rotate(-90.f);
        //printf("Flight : %d Stopped at runway dist=%f, prepare to takeoff\n",flight->id, distanceToRunway); //ye submit krna h??? its wirtten in milestone 2 and class to nhi h 
        // TAKEOFF from standstill
        step/=4;
        this->phase = FlightPhase::TAKEOFF;
        float distanceAlongRunway = RUNWAY_LEN + distanceToRunway;
        while (xPos < 760){ //while runway left
            int a = 65050 + (1000-rand()%2000);
            this->speed += a/3600.0; //v=u+at
            this->xPos += this->speed * step;
            this->updatePosition();
            this->spriteAltitudeUp();
            distanceAlongRunway -= this->speed/3600.0 + 0.5*a*pow((1.0/3600),2); //ut+1/2at^2
            this->consumeFuel();
            printf("Flight : %d takeoff, speed=%f, altitude=%f, dist=%f\n",this->id, this->speed,this->altitude,distanceAlongRunway);
            sleep(1);
            //usleep(1000);
        }
        //printf("Flight : %d Takeoff at dist remaining=%f\n", flight->id, distanceAlongRunway);
        // CLIMB
        this->phase = FlightPhase::CLIMB;
        while (this->speed < 780){
            int a = 60000 + (500-rand()%1000);
            this->speed += a*(1.0/3600); //v=u+at
            this->altitude += 3280* (this->speed*sin(M_PI/12)*(1.0/3600) + 0.5*(a*sin(M_PI/12))*pow((1.0/3600),2));//ut+1/2at^2
            this->xPos += this->speed * step;
            this->updatePosition();
            this->spriteAltitudeUp();
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
            this->spriteAltitudeUp();
            this->consumeFuel();
            sleep(1);
            //usleep(1000);
            //printf("Flight : %d cruise, speed=%f, altitude=%f\n", this->id, this->speed,this->altitude);
        }
        this->phase=FlightPhase::CRUISE;
        printf("Flight %d exiting..\n",this->id);
        return 1; //return w/ finished status

    }

    int simulateEmergencyFlightArrival(bool flightTurn[]){  
    
        printf(" ARRIVAL :flight ID: %d\n",this->id);
    
        // ------------- SIMULATE --------------
    
        float distanceToRunway = 3.5; //for a safe-ish descent (50ft/s)
        
        float step = 0.05;
        //bring flight from HOLDING->APPROACH to avoid violation
        while(this->speed > 292){
            this->speed -= 20 + (1-rand()%2); //~1.5-2.5
            distanceToRunway -= this->speed*(1.0/3600);
            this->xPos -= this->speed * step;
            this->altitude -= 40; 
            this->updatePosition();
            //this->spriteAltitudeDown();
            this->consumeFuel();
            printf("flight %d : holding->approach, speed=%f, altitude=%f, dist=%f\n", this->id, this->speed,this->altitude,distanceToRunway);
            if (!flightTurn[this->id-1]){ // somebody pre-empted it, go back to holding
                return 0; //exit w/ unfinished status
            }
            sleep(1);
            //usleep(100000);
        }
        //APPROACH
        this->phase = FlightPhase::APPROACH;
        while(this->speed>242.5){
            this->speed += -5000*(1.0/3600); //v=u+at
            distanceToRunway -= this->speed * (1.0/3600); 
            this->xPos -= this->speed * step;
            this->altitude -= 50;   
            this->updatePosition();
            //this->spriteAltitudeDown();
            this->consumeFuel();
            printf("flight %d : approach, speed=%f, altitude=%f, dist=%f\n",this->id, this->speed,this->altitude,distanceToRunway);
            if (!flightTurn[this->id-1]){ // somebody pre-empted it, go back to holding
                return 0;
            }
            sleep(1);
            //usleep(100000);
        }
        step *= 1.6;
        //LANDING
        this->phase = FlightPhase::LANDING;
        float distanceAlongRunway = distanceToRunway+RUNWAY_LEN; 
        while ( this->altitude>0 || xPos > 140){
            float a = -30250 + (50-rand()%100);
            if (this->speed > 32)
                this->speed += a*(1.0/3600); //v=u+at
            this->xPos -= this->speed * step;
            distanceAlongRunway -= this->speed/3600.0 + 0.5*a*pow((1.0/3600),2); //ut+1/2at^2
            if (this->altitude>0){
                this->altitude -= 30;
                if (this->altitude<0)
                    this->altitude=0;
            }
            this->updatePosition();
            this->consumeFuel();
            printf("flight %d : landing, speed=%f, altitude=%f, dist=%f\n",this->id, this->speed,this->altitude,distanceAlongRunway);
            sleep(1);
            //usleep(100000);
        }
        sprite.rotate(90.f);
        //TAXI
        this->phase = FlightPhase::TAXI;
        float distanceToGate = distanceAlongRunway+TERMINAL_TO_RUNWAY_LEN;
        while (yPos>220){
            if (this->speed<28)
                this->speed += (50-rand()%100)/100.0;//fluctuate by ~0.5
            else
                this->speed -= 0.5;
            this->yPos -= this->speed * step;
            this->updatePosition();
            distanceToGate -= this->speed*(1.0/3600);
            this->consumeFuel();
            printf("flight %d : taxi, speed=%f, dist=%f\n",this->id, this->speed,distanceToGate);
            sleep(1);
            //usleep(100000);
        }
        sprite.rotate(90.f);
        //TAXI->GATE 
        step *= 4;
        while(this->speed>0 && xPos<220){
            //flight->speed += -1500*(1.0/3600); //v=u+at
            this->speed -= 0.5;
            if (this->speed<0)
                this->speed=0;
            distanceToGate -= this->speed*(1.0/3600);
            this->xPos += this->speed * step;
            this->updatePosition();
            this->consumeFuel();
            printf("flight %d : taxi->gate, speed=%f, dist=%f\n",this->id, this->speed,distanceToGate);
            sleep(1);
            //usleep(1000);
        }
        this->phase = FlightPhase::GATE;
                
                    
        printf("Flight %d exiting..\n",this->id);

        return 1; //exit w/ finished status
    }

    //graphics functions
    void draw(sf::RenderWindow& window) {
        window.draw(sprite);
        window.draw(label);
    }

    void updatePosition(){
        sprite.setPosition(this->xPos,this->yPos);
        label.setPosition(this->xPos,this->yPos);
        printf("x=%f, y=%f\n",xPos,yPos); 
    }

    void spriteAltitudeUp(){
        if (this->altitude>0 && xPos>0 && xPos<1000){
            //scale up going up
            float scale = altitude/500.0; 
            sprite.scale(1+scale,1+scale);
        }
    }

    void spriteAltitudeDown() {
        if (altitude > 0 && xPos > 0 && xPos < 1000) {
            float scale = 0.35f - (1 - altitude / 500.f) * (0.35f - 0.25f);  // From 0.35 to 0.25
            // Ensure the scale doesn't go below 0.25
            if (scale < 0.25f) scale = 0.25f;
            sprite.setScale(scale, scale);  // Set the scale
        }
    }

};

#endif