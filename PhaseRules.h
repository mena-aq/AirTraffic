#ifndef PHASERULES_H
#define PHASERULES_H

#include "Types.h"
#include "Phase.h"

class PhaseRules{
public:
    Phase flightPhases[NUM_FLIGHT_PHASES];

    PhaseRules(){
        flightPhases[(int)FlightPhase::GATE] = Phase{0, 10, 0, 0};
        flightPhases[(int)FlightPhase::TAXI] = Phase{15, 30, 0, 0};
        flightPhases[(int)FlightPhase::TAKEOFF] = Phase{0, 290, 0, 500};                            
        flightPhases[(int)FlightPhase::CLIMB] = Phase{250, 463, 0, 5000};
        flightPhases[(int)FlightPhase::CRUISE] = Phase{800, 900, 5000, 10000};
        flightPhases[(int)FlightPhase::HOLDING] = Phase{290, 600, 2000, 5000};
        flightPhases[(int)FlightPhase::APPROACH] = Phase{240, 290, 800, 1000};
        flightPhases[(int)FlightPhase::LANDING] = Phase{30, 240, 0, 800};        
    }
    Phase& operator[](int i){
        return flightPhases[i];
    }

};

#endif