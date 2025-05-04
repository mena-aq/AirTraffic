#ifndef AVN_H
#define AVN_H

#include "Types.h"
#include "ViolationInfo.h"
#include "Flight.h"
#include <sstream>


class AVN {
public:
    int flightID;
    AirlineName airline;
    AirlineType airlineType;
    float speedRecorded;
    FlightPhase phaseViolation;
    time_t violationTimestamp;
    //for AVN generator
    float amountDue;
    bool status;  // true= paid, false= unpaid
    time_t dueDate;

    sf::Text graphic;
    float graphicX,graphicY;

    AVN () {}

    AVN(int flightID,AirlineName airline,AirlineType airlineType,float speedRecorded,FlightPhase phaseViolation,time_t violationTimestamp,float amountDue)
    : flightID(flightID), airline(airline), airlineType(airlineType), speedRecorded(speedRecorded), phaseViolation(phaseViolation),violationTimestamp(violationTimestamp),amountDue(amountDue){
        //due date
        std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(violationTimestamp);
        auto futureTime = tp + std::chrono::hours(24 * 3); // add 3 days
        dueDate = std::chrono::system_clock::to_time_t(futureTime);

        graphicX=0;
        graphicY=0;
    }

    void printAVN() {
        std::cout<<"\n> AVN CHALLAN\n";
        std::cout << "> AVN Issued: Flight " << flightID<<std::endl;
        std::cout << "> Speed: " << speedRecorded<<" km/h\n";
        std::cout << "> Phase: ";
        printPhase(phaseViolation);
        std::cout << "\n> Timestamp: " << ctime(&violationTimestamp);
        std::cout << "> Amount: $" << amountDue << std::endl;
        std::cout << "> Due: " << ctime(&dueDate)<<std::endl;
    }

    void initGraphic(float& maxY){
        std::ostringstream ss;
        ss << "----------------\n";
        ss << "AVN CHALLAN\n";
        ss << "AVN Issued: Flight " << flightID<<std::endl;
        ss << "Speed: " << speedRecorded<<" km/h\n";
        ss << "Phase: ";
        //printPhase(phaseViolation);
        ss << "Timestamp: " << ctime(&violationTimestamp);
        ss << "Amount: $" << amountDue << std::endl;
        ss << "Due: " << ctime(&dueDate)<<std::endl;
    
        graphic.setFont(globalFont);
        graphic.setFillColor(sf::Color::White);  
        graphic.setCharacterSize(16);
        graphic.setString(ss.str());

        //position
        graphic.setPosition(0,maxY);
        maxY+=120;

    }
    void drawGraphic(sf::RenderWindow& window){
        //printf("draw avn\n");
        window.draw(graphic);
    }
};


#endif