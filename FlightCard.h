
#ifndef FLIGHTCARD_H
#define FLIGHTCARD_H

#include <sstream>
#include "Flight.h"

class FlightCard{
public:
    float x,y;
    sf::Text details;
    sf::Font font;
    Flight* flight;

    sf::RectangleShape bar;

    FlightCard(Flight* flight,float x,float y){
        details.setFont(globalFont);
        details.setFillColor(sf::Color::Yellow);  
        details.setCharacterSize(12);
        bar.setSize(sf::Vector2f(3,80));
        bar.setFillColor(getAirlineColorCode(flight->airlineName));
        setPosition(x,y);
        this->flight = flight;
    }

    void setPosition(float x,float y){
        this->x = x;
        this->y = y;
        details.setPosition(this->x,this->y);
        bar.setPosition((this->x)-5,(this->y)+5);
    }

    void drawCard(sf::RenderWindow& window){
        if (flight){
            std::ostringstream ss;
            ss << "Flight " << flight->id << "\n";
            ss << "Airline: " << getAirlineName(flight->airlineName) << "\n";
            ss << "Airline Type: " << getAirlineType(flight->airlineType) << "\n";
            ss << std::round(flight->speed) << " km/h";
            ss << " | Alt: " << std::round(flight->altitude) << " ft" << "\n";
            ss << "Phase: " << getPhase(flight->phase) << "\n";
            ss << "Fuel: " << flight->fuelLevel << "gal. \n";
            ss << "Priority: " << flight->priority << "\n";
            details.setString(ss.str());
        }
        window.draw(details);
        window.draw(bar);
    }

};

#endif