#ifndef TYPE_H
#define TYPE_H

#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cmath>
using namespace std;

//for graphics
#include <SFML/Graphics.hpp>


// ATC SYSTEM CONSTANTS 
#define NUM_FLIGHT_PHASES 8
#define NUM_AIRLINE 6
#define TOTAL_FLIGHTS 3
#define GRID 6 
#define SIMULATION_DURATION 300
//assume runways start at (0,0), so each quadrant is 3x3 km
#define RUNWAY_LEN 0.6
#define TERMINAL_TO_RUNWAY_LEN 0.045

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

std::string getPhase(FlightPhase phase) {
    switch (phase) {
        case FlightPhase::GATE: return "GATE"; break;
        case FlightPhase::TAXI: return "TAXI"; break;
        case FlightPhase::TAKEOFF: return "TAKEOFF"; break;
        case FlightPhase::CLIMB: return "CLIMB"; break;
        case FlightPhase::CRUISE: return "CRUISE"; break;
        case FlightPhase::HOLDING: return "HOLDING"; break;
        case FlightPhase::APPROACH: return "APPROACH"; break;
        case FlightPhase::LANDING: return "LANDING"; break;
    }
    return "";
}
std::string getAirlineName(AirlineName airlineName){
    switch (airlineName) {
        case PIA: return "PIA"; break;
        case AirBlue: return "AirBlue"; break;
        case FedEx: return "FedEx"; break;
        case Pakistan_Airforce: return "Pakistan Airforce"; break;
        case Blue_Dart: return "Blue Dart"; break;
        case AghaKhan_Air_Ambulance: return "AK Air Ambulance"; break;
    }
    return "";
}
std::string getAirlineType(AirlineType airlineType){
    switch (airlineType) {
        case COMMERCIAL: return "COMMERCIAL"; break;
        case CARGO: return "CARGO"; break;
        case MEDICAL: return "MEDICAL"; break;
        case MILITARY: return "MILITARY"; break;
    }
    return "";
}
sf::Color getAirlineColorCode(AirlineName airline){
    switch (airline) {
        case PIA: return sf::Color(0,100,0); break; //dark green
        case AirBlue: return sf::Color::Blue ; break;
        case FedEx: return sf::Color(128, 0, 128); break; //purple
        case Pakistan_Airforce: return sf::Color(101, 67, 33) ; break; //dark brown
        case Blue_Dart: return sf::Color(135, 206, 235); break; //sky blue
        case AghaKhan_Air_Ambulance: return sf::Color::Red; break;
    }
    return sf::Color::White;
}

//font
sf::Font globalFont;
inline void loadFont() {
    if (!globalFont.loadFromFile("VT323.ttf")) {
        std::cerr << "Error loading font VT323.ttf" << std::endl;
    }
}


#endif
