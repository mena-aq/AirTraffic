#ifndef AVN_H
#define AVN_H

#include "Types.h"
#include "ViolationInfo.h"
#include "Flight.h"
#include <sstream>


class AVN {
public:

    int avnID;

    int flightID;
    AirlineName airline;
    AirlineType airlineType;
    float speedRecorded;
    float speedAllowedLower;
    float speedAllowedUpper;
    FlightPhase phaseViolation;
    time_t violationTimestamp;
    //for AVN generator
    float amountDue;
    int status;  // 0 unpaid, 1 paid, 2 overdue
    time_t dueDate;

    sf::RectangleShape graphic;
    sf::Text graphicText;
    float graphicX,graphicY;

    AVN () {}

    AVN(int avnID,int flightID,AirlineName airline,AirlineType airlineType,float speedRecorded,FlightPhase phaseViolation,time_t violationTimestamp,float amountDue,float lowerLim,float upperLim)
    : avnID(avnID), flightID(flightID), airline(airline), airlineType(airlineType), speedRecorded(speedRecorded), phaseViolation(phaseViolation),violationTimestamp(violationTimestamp),amountDue(amountDue),status(0),dueDate(dueDate){

        //speed allowed
        this->speedAllowedLower = lowerLim;
        this->speedAllowedUpper = upperLim;

        //due date
        std::chrono::system_clock::time_point tp = std::chrono::system_clock::from_time_t(violationTimestamp);
        auto futureTime = tp + std::chrono::hours(24 * 3); // add 3 days
        dueDate = std::chrono::system_clock::to_time_t(futureTime);

        graphic.setSize(sf::Vector2f(200.f, 120.f)); 
        graphic.setFillColor(sf::Color::Yellow);
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

    std::string getStatus(){
        if(status==0){
            return "Unpaid";
        }
        else if(status==1){
            return "Paid";
        }
        else{
            return "Overdue";
        }
    }

    void initGraphic(float x,float y){
        std::ostringstream ss;
        ss << "AVN " << this->avnID << std::endl;
        ss << "AVN Issued: Flight " << flightID <<std::endl;
        ss << getAirlineName(airline) << ", " << getAirlineType(airlineType) << std::endl;
        ss << "Speed: " << speedRecorded<<" km/h\n";
        ss << "Phase: " << getPhase(phaseViolation)  << std::endl;
        ss << "allows " << speedAllowedLower << " - " << speedAllowedUpper << " kmh\n";
        ss << "Timestamp: " << ctime(&violationTimestamp);
        //ss << "Amount: $" << amountDue << std::endl;
        //ss << "Due: " << ctime(&dueDate);
        ss << "Status: " << getStatus();
        
        graphicText.setFont(globalFont);
        graphicText.setFillColor(sf::Color::Black);  
        graphicText.setCharacterSize(13);
        graphicText.setString(ss.str());

        //position
        graphic.setPosition(x,y);
        graphicText.setPosition(x,y);

    }
    void drawGraphic(sf::RenderWindow& window){
        //printf("draw avn\n");
        window.draw(graphic);
        window.draw(graphicText);
    }
};


#endif