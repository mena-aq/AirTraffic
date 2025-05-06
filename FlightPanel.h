
#ifndef FLIGHTPANEL_H
#define FLIGHTPANEL_H

#include "FlightCard.h"
#include <vector>

class FlightPanel{
public:

    vector<FlightCard> flightCards;
    pthread_mutex_t l;
    float max_X;

    FlightPanel(){
        pthread_mutex_init(&l,NULL);
        max_X = 120;
    }
    
    void addCard(Flight* flight){
        float xPos = 120.f; 
        float yPos = 5.f; 
    
        pthread_mutex_lock(&l);
        if (!flightCards.empty()) {
            const FlightCard& lastCard = flightCards.back();
            max_X+=150;
            xPos = max_X;
        }
        FlightCard fc(flight, xPos, yPos);
        flightCards.push_back(fc);
        printf("add card at %f %f\n",xPos,yPos);
        pthread_mutex_unlock(&l);

    }
    
    void removeCardById(int id) {
        for (auto it = flightCards.begin(); it != flightCards.end(); ++it) {
            if (it->flight && it->flight->id == id) {
                flightCards.erase(it);
                break; 
            }
        }
        recalculateCardPositions();
    }

    void recalculateCardPositions() {
        float startY = 5.f;
        float startX = 120.f;

        float xPos = startX;
        for (auto& card : flightCards) {
            card.x = xPos;
            card.y = startY;
            card.setPosition(xPos,startY); 
            xPos += 150;
            max_X = xPos;
        }
    }
    
    
    void displayPanel(sf::RenderWindow& window){
        for (int i=0; i<flightCards.size(); i++){
            if (flightCards[i].flight != nullptr){
                flightCards[i].drawCard(window);
            }
        }
    }

};


#endif