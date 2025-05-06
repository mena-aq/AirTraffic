//#include "Phase.h"
//#include "Flight.h"
#include "Runway.h"
#include "QueueFlights.h"
#include "Radar.h"
#include "Timer.h"
#include "FlightPanel.h"

bool flightTurn[TOTAL_FLIGHTS]={false};
pthread_cond_t flightCond[TOTAL_FLIGHTS];


//make the 3 runways
Runway rwyA('A'); 
Runway rwyB('B'); 
Runway rwyC('C');


struct args{
    Flight* flight;
    char runway;
};


void* simulateWaitingAtGate(void* arg){

    args* flightArg = static_cast<args*>(arg);
    Flight* flight = flightArg->flight;
    const char runwayToAcquire = flightArg->runway;
    cout<<"at gate flight: "<< flight->id<<endl;

    int status = 0;
    while (1){
        //flight will wait at gate stationary until runway clears
        flight->phase = FlightPhase::GATE;
        //flight will wait at gate stationary until runway clears
        //if any fault, just tow and remove from queue???
        while (!flightTurn[(flight->id)-1]){
            flight->waitingTime++;
            sleep(1);
            if (int(flight->waitingTime)%100 == 0)
                flight->priority++; //up the priority every 100s
        }
        if (runwayToAcquire=='C'){
            rwyC.acquireRunway();
            status = flight->simulateEmergencyFlightDeparture(flightTurn);
        }
        else{
            rwyB.acquireRunway();
            if (flight->isInternational())
                status = flight->simulateInternationalFlightDeparture(flightTurn);
            else
                status = flight->simulateDomesticFlightDeparture(flightTurn);
        }

        if (runwayToAcquire=='C')
            rwyC.releaseRunway();
        else
            rwyB.releaseRunway();

        if (status == 1){ //finished
            break;
        }    
    }   
    flight->phase = FlightPhase::DONE;
    pthread_exit(NULL);
}

void* simulateWaitingInHolding(void* arg){

    args* flightArg = static_cast<args*>(arg);
    Flight* flight = flightArg->flight;
    const char runwayToAcquire = flightArg->runway;
    cout<<"IN holding flight: "<< flight->id<<endl;

    int status =0;
    while (1){
        //keep holding around airport until its your turn

        flight->phase = FlightPhase::HOLDING;
        while (!flightTurn[(flight->id)-1]){
            flight->speed += (10-rand()%20);//fluctuate by ~10
            flight->consumeFuel();
            flight->waitingTime++;
            if (int(flight->waitingTime)%100 == 0)
                flight->priority++; //up the priority every 100s
            sleep(1);
        }
            
        if (runwayToAcquire=='C'){
            rwyC.acquireRunway();
            status = flight->simulateEmergencyFlightArrival(flightTurn);
        }
        else{
            rwyA.acquireRunway();
            if (flight->isInternational())
                status = flight->simulateInternationalFlightArrival(flightTurn);
            else
                status = flight->simulateDomesticFlightArrival(flightTurn);
        }
    
        if (runwayToAcquire=='C')
            rwyC.releaseRunway();
        else
            rwyA.releaseRunway();

        if (status == 1) //it was finished so send dont back to holding
            break;

    }
    flight->phase = FlightPhase::DONE;
    pthread_exit(NULL);
}

class Dispatcher{
public:
    Radar radar;

    pthread_mutex_t turnLock;
    pthread_cond_t cond;
    char runway;

    static pthread_t flightTid[TOTAL_FLIGHTS]; //all flight thread ids
    static pthread_t radarTid[TOTAL_FLIGHTS]; //all radar thread ids
    
    //for all simulation flights 
    QueueFlights* internationalFlights;
    QueueFlights* domesticFlights;
    //for those whose arrival time has occured
    QueueFlights* internationalWaitingQueue; 
    QueueFlights* domesticWaitingQueue;
    //current
    Flight* currentFlight;
    //finished flights
    static vector<Flight*> finishedFlights;

    //graphics
    static FlightPanel flightPanel; 

    Dispatcher(char runway,QueueFlights*& domesticQueue, QueueFlights*& interQueue): runway(runway){
        this->internationalFlights= interQueue;
        this->domesticFlights= domesticQueue;
        internationalWaitingQueue= new QueueFlights;
        domesticWaitingQueue = new QueueFlights;
        this->currentFlight = nullptr;
    }
    
    static void* dispatchFlights(void* arg) {

        Dispatcher* dispatcher = (Dispatcher*)(arg);

        dispatcher->internationalFlights->sortQueue2();
        dispatcher->domesticFlights->sortQueue2();
        
        QueueFlights* internationalFlightsQueue = dispatcher->internationalFlights;
        QueueFlights* domesticFlightsQueue = dispatcher->domesticFlights;
        QueueFlights* internationalWaitingQueue = dispatcher->internationalWaitingQueue; 
        QueueFlights* domesticWaitingQueue = dispatcher->domesticWaitingQueue; 

        //first launch all the flights
        //international flights
        for (int i=0; i<internationalFlightsQueue->numInQueue(); i++){ 
            Flight* flight = (*internationalFlightsQueue)[i]; 
            int index = (flight->id) - 1;
            args* flightArg = new args;
            flightArg->flight = flight;
            flightArg->runway = dispatcher->runway;
            if (flight->isArrival())
                pthread_create(&flightTid[index],NULL,simulateWaitingInHolding,(void*)flightArg);
            else
                pthread_create(&flightTid[index],NULL,simulateWaitingAtGate,(void*)flightArg);
            flight->thread_id = flightTid[index]; 
            pthread_create(&(flight->radar_id), nullptr, dispatcher->radar.flightRadar, (void*)flight);
        }
        //domestic flights
        for (int i=0; i<domesticFlightsQueue->numInQueue(); i++){ 
            Flight* flight = (*domesticFlightsQueue)[i]; 
            int index = (flight->id) - 1;
            args* flightArg = new args;
            flightArg->flight = flight;
            flightArg->runway = dispatcher->runway;
            if (flight->isArrival())
                pthread_create(&flightTid[index],NULL,simulateWaitingInHolding,(void*)flightArg);
            else
                pthread_create(&flightTid[index],NULL,simulateWaitingAtGate,(void*)flightArg);
                flight->thread_id = flightTid[index]; 
            pthread_create(&(flight->radar_id), nullptr, dispatcher->radar.flightRadar, (void*)flight);
        }
        sleep(1); //this is the bandaid holding this tgt

        //Flight* dispatcher->currentFlight = nullptr;
        //Flight* dispatcher->currentFlight = dispatcher->dispatcher->currentFlight;
        int currentFlightID = -1;
        int x=1;

        // START AIR TRAFFIC CONTROL OF ALL FLIGHTS BY FCFS/PRIORITY
        while ( !internationalFlightsQueue->isEmpty() || !domesticFlightsQueue->isEmpty() || !domesticWaitingQueue->isEmpty() || !internationalWaitingQueue->isEmpty()/*&& Timer::currentTime < SIMULATION_DURATION*/) {

            //dispatch the front (FCFS) if time for it 
            Flight* readyInternational = nullptr;
            Flight* readyDomestic = nullptr;
            
            if (dispatcher->runway == 'A' || dispatcher->runway == 'C'){
                if (Timer::currentTime % 120 == 0 && !internationalFlightsQueue->isEmpty()){ //move an international arrival to waiting Q
                    readyInternational = internationalFlightsQueue->getNextFlight();
                    if (readyInternational->isArrival()){
                        internationalWaitingQueue->addFlight(readyInternational); //add to waiting
                        printf("time for flight %d\n",readyInternational->id);
                        dispatcher->flightPanel.addCard(readyInternational);
                    }
                    else
                        internationalFlightsQueue->addFlightAtFront(readyInternational); //add back (C)
                }
                if (Timer::currentTime % 180 == 0 && !domesticFlightsQueue->isEmpty()){ //move a domestic arrival to waiting Q
                    readyDomestic = domesticFlightsQueue->getNextFlight();
                    if (readyDomestic->isArrival()){
                        printf("time for flight %d\n",readyDomestic->id);
                        domesticWaitingQueue->addFlight(readyDomestic);
                        dispatcher->flightPanel.addCard(readyDomestic);
                    }
                    else
                        domesticFlightsQueue->addFlightAtFront(readyDomestic);
                }
            }
            if (dispatcher->runway == 'B' || dispatcher->runway == 'C'){
                if (Timer::currentTime % 150 == 0 && !internationalFlightsQueue->isEmpty()){ //move a international departure to waiting Q
                    readyInternational = internationalFlightsQueue->getNextFlight();
                    if (readyInternational->isDeparture()){
                        printf("time for flight %d\n",readyInternational->id);
                        internationalWaitingQueue->addFlight(readyInternational); //add to waiting
                        dispatcher->flightPanel.addCard(readyInternational);
                    }
                    else
                        internationalFlightsQueue->addFlightAtFront(readyInternational); //add back (C)
                }
                if (Timer::currentTime % 240 == 0 && !domesticFlightsQueue->isEmpty()){ //move a domestic departure to waiting Q
                    readyDomestic = domesticFlightsQueue->getNextFlight();
                    if (readyDomestic->isDeparture()){
                        domesticWaitingQueue->addFlight(readyDomestic);
                        printf("time for flight %d\n",readyDomestic->id);
                        dispatcher->flightPanel.addCard(readyDomestic);
                    }
                    else
                        domesticFlightsQueue->addFlightAtFront(readyDomestic);                
                }
            }

            //sort the waiting Qs to choose priority flights
            internationalWaitingQueue->sortQueue();
            domesticWaitingQueue->sortQueue();

            Flight* nextInternational = nullptr;
            Flight* nextDomestic = nullptr;

            if (!internationalWaitingQueue->isEmpty())
                nextInternational = internationalWaitingQueue->getNextFlight();
            if (!domesticWaitingQueue->isEmpty())
                nextDomestic = domesticWaitingQueue->getNextFlight();

            //if none do nothing
            //if both choose one
            Flight* nextFlight = nullptr;
            if (nextInternational && nextDomestic){
                nextFlight = ( nextDomestic->priority > nextInternational->priority? nextDomestic:nextInternational);
                if(nextFlight != nextDomestic){
                    domesticWaitingQueue->addFlightAtFront(nextDomestic);
                    nextDomestic = nullptr;
                }
                else if (nextFlight != nextInternational){
                    internationalWaitingQueue->addFlightAtFront(nextInternational);
                    nextInternational = nullptr;
                }
            }
            else if (nextInternational && !nextDomestic)
                nextFlight = nextInternational;
            else if (nextDomestic && !nextInternational)
                nextFlight = nextDomestic;

            // check if anyone currently running do we pre-empt
            if (nextFlight != nullptr) {
                if (dispatcher->currentFlight == nullptr){
                    //just assign
                    flightTurn [nextFlight->id-1] = true;
                    dispatcher->currentFlight = nextFlight;
                    currentFlightID = dispatcher->currentFlight->id;
                }
                else{
                    //check if can pre-empt
                    if (dispatcher->runway == 'A' && nextFlight->priority > dispatcher->currentFlight->priority && dispatcher->currentFlight->phase < FlightPhase::LANDING){
                        //pre-empt current
                        printf("flight %d pre=empting flight %d\n",nextFlight->id,dispatcher->currentFlight->id);
                        flightTurn[dispatcher->currentFlight->id - 1]=false;
                        sleep(1); //give it some time to go back to holding
                        flightTurn [nextFlight->id-1] = true;
                        dispatcher->currentFlight = nextFlight;
                        currentFlightID = dispatcher->currentFlight->id;
                    }
                    else if (dispatcher->runway == 'B' && nextFlight->priority > dispatcher->currentFlight->priority && dispatcher->currentFlight->phase < FlightPhase::TAKEOFF){
                        //pre-empt current
                        printf("flight %d pre=empting flight %d\n",nextFlight->id,dispatcher->currentFlight->id);
                        flightTurn[dispatcher->currentFlight->id - 1]=false;
                        sleep(1); //give it some time to go back to holding
                        flightTurn [nextFlight->id-1] = true;
                        dispatcher->currentFlight = nextFlight;
                        currentFlightID = dispatcher->currentFlight->id;
                    }
                    else if (dispatcher->runway == 'C' && nextFlight->priority > dispatcher->currentFlight->priority){
                        //if opposite types, if current flight is departure and < takeoff, preempt
                        // else dont preempt
                        // of same types so check for each type, and their phases and preempt accordingly
                        printf("C pre-empt\n");
                        if(dispatcher->currentFlight->isArrival() && dispatcher->currentFlight->phase<FlightPhase:: LANDING
                            &&nextFlight->isDeparture()){
                            flightTurn[dispatcher->currentFlight->id -1]=false;
                            sleep(1);
                            flightTurn[nextFlight->id -1]=true;
                            dispatcher->currentFlight = nextFlight;
                            currentFlightID = dispatcher->currentFlight->id;

                        }
                        else if( dispatcher->currentFlight->isDeparture() && dispatcher->currentFlight->phase<FlightPhase:: TAKEOFF
                            &&nextFlight->isArrival()){
                            flightTurn[dispatcher->currentFlight->id -1]=false;
                            sleep(1);
                            flightTurn[nextFlight->id -1]=true;
                            dispatcher->currentFlight = nextFlight;
                            currentFlightID = dispatcher->currentFlight->id;

                        }
                        else if(dispatcher->currentFlight->isArrival() &&dispatcher->currentFlight->phase<FlightPhase:: LANDING){
                            flightTurn[dispatcher->currentFlight->id -1]=false;
                            sleep(1);
                            flightTurn[nextFlight->id -1]=true;
                            dispatcher->currentFlight = nextFlight;
                            currentFlightID = dispatcher->currentFlight->id;

                        }
                        else if(dispatcher->currentFlight->isDeparture() &&dispatcher->currentFlight->phase<FlightPhase:: TAKEOFF){
                            flightTurn[dispatcher->currentFlight->id -1]=false;
                            sleep(1);
                            flightTurn[nextFlight->id -1]=true;
                            dispatcher->currentFlight = nextFlight;
                            currentFlightID = dispatcher->currentFlight->id;

                        }

                    }
                    else{ //cannot pre-empt
                        if(nextFlight->isDomestic())
                            domesticWaitingQueue->addFlightAtFront(nextFlight);
                        else if (nextFlight->isInternational()){
                            internationalWaitingQueue->addFlightAtFront(nextFlight);
                        }
                    }
                }
                nextFlight = nullptr;
            }
            if (dispatcher->currentFlight && dispatcher->currentFlight->phase == FlightPhase::DONE){ //its done
                flightTurn[currentFlightID-1]=false;
                flightPanel.removeCardById(dispatcher->currentFlight->id);
                currentFlightID = -1;
                finishedFlights.push_back(dispatcher->currentFlight);
                //free(dispatcher->currentFlight);
                dispatcher->currentFlight = nullptr;
                //printf("unset turn");
            }
            sleep(1);
        }
        printf("all flights done\n");
        pthread_exit(nullptr);
    }

    void drawFlights(Dispatcher* dispatcher,sf::RenderWindow& window){
        //draw waiting flights
        for (int i=0; i<internationalWaitingQueue->numInQueue(); i++){
            (*internationalWaitingQueue)[i]->draw(window);
        }
        for (int i=0; i<domesticWaitingQueue->numInQueue(); i++){
            (*domesticWaitingQueue)[i]->draw(window);
        }
        //draw current flight
        if (dispatcher->currentFlight){
            dispatcher->currentFlight->draw(window);
        }
        //draw panel
        flightPanel.displayPanel(window);

        //draw AVNS
        //dispatcher->radar.dashboard.displayAVNs(aWindow);

    }

};
pthread_t Dispatcher::flightTid[TOTAL_FLIGHTS];
pthread_t Dispatcher::radarTid[TOTAL_FLIGHTS];
FlightPanel Dispatcher::flightPanel;
vector<Flight*> Dispatcher::finishedFlights;

// for input
FlightType getFlightType(char dir){
     dir = toupper(dir); // Normalize

    while(dir != 'N' && dir != 'S' && dir != 'E' && dir !='W'){
        cout << "Invalid Direction!\nEnter Again (N/S/E/W): ";
        cin.clear();                 // clear error flags
        cin.ignore(10000, '\n');     // discard bad input
        cin >> dir;
        dir = toupper(dir);          // normalize again
    }

    if (dir == 'N') return FlightType::INTERNATIONAL_ARRIVAL;
    if (dir == 'E') return FlightType::INTERNATIONAL_DEPARTURE;
    if (dir == 'S') return FlightType::DOMESTIC_ARRIVAL;
    return FlightType::DOMESTIC_DEPARTURE;  // must be 'W'
}
AirlineName getAirlineName(int name){
    while(name <1 || name >6){
        cout<<"Invalid Name!\nEnter Again: ";
        cin>> name;
    }
    if( name == 1)
            return AirlineName:: PIA;
    else if( name==2)
           return AirlineName:: AirBlue;
    else if( name==3)
            return AirlineName:: FedEx;
    else if( name==4)
            return AirlineName:: Pakistan_Airforce;
    else if( name==5)
            return AirlineName:: Blue_Dart;
    else 
        return AirlineName:: AghaKhan_Air_Ambulance;
}

AirlineType getAirtype(int type){
    while(type <1 || type >4){
        cout<<"Invalid Type!\nEnter Again: ";
        cin>> type;
    }
    if( type == 1)
            return AirlineType:: MEDICAL;
    else if( type==2)
           return AirlineType:: MILITARY;
    else if( type==3)
            return AirlineType:: CARGO;
    else 
        return AirlineType:: COMMERCIAL;
}

int checkTime(float time){
    while(time <0.00 || time >=24.00){
        cout<<"Invalid Time!\nEnter again: ";
        cin>> time;
    }
    return time;
}



int main() {

    srand(time(NULL));
    loadFont();

    // INPUT FLIGHTS
    QueueFlights* rwyAInternationalFlights = new QueueFlights;
    QueueFlights* rwyADomesticFlights = new QueueFlights;

    QueueFlights* rwyBInternationalFlights = new QueueFlights;
    QueueFlights* rwyBDomesticFlights = new QueueFlights;

    QueueFlights* rwyCInternationalFlights = new QueueFlights;
    QueueFlights* rwyCDomesticFlights = new QueueFlights;

    //make dispatchers for 3 runways (A is for arrival, B for departure and C for cargo (dept and arr))
    Dispatcher rwyADispatcher('A',rwyADomesticFlights,rwyAInternationalFlights);
    Dispatcher rwyBDispatcher('B',rwyBDomesticFlights,rwyBInternationalFlights);
    Dispatcher rwyCDispatcher('C',rwyCDomesticFlights,rwyCInternationalFlights);
    
    /*
    cout<<"        Airline          |    Type        | Flights\n";
    cout<<"---------------------------------------------------\n";
    cout<<"PIA                      | Commercial     | 4\n";
    cout<<"AirBlue                  | Commercial     | 4\n";
    cout<<"FedEx                    | Cargo          | 2\n";
    cout<<"Pakistan Airforce        | Military       | 1\n";
    cout<<"Blue Dart                | Cargo          | 2\n";
    cout<<"AghaKhan Air Ambulance   | Medical        | 1\n\n";

    cout<<"==================================================\n";
    cout<<"1)PIA\n";
    cout<<"2)AirBlue\n";
    cout<<"3)FedEx\n";
    cout<<"4)Pakistan Airforce\n";
    cout<<"5)Blue Dart\n";
    cout<<"6)AghaKhan Air Ambulance\n\n";

    cout<<"==================================================\n";
    cout<<"1)MEDICAL\n";
    cout<<"2)MILITARY\n";
    cout<<"3)CARGO\n";
    cout<<"4)COMMERCIAL\n\n";

    printf("Enter data for %d flights\n\n", TOTAL_FLIGHTS);

    FlightType Ftype; AirlineType airtype; AirlineName name;
    int id; char dir; int priority; int type; int airname; float scheduledTime;

    Flight* f;

    for(int i=0; i<TOTAL_FLIGHTS; i++){
        cout<<"Flight Number | Airline Name (1-6) | Aircraft Type (1-4) | Direction  (N/S/E/W)| Priority | Scheduled Time (24 hr clock)\n";
        cin>>id;
        cin>> airname;
        cin>> type;
        cin>>dir;
        cin>> priority;
        cin>>scheduledTime;
        scheduledTime= checkTime(scheduledTime);
        Ftype= getFlightType(dir);
        name= getAirlineName(airname);
        airtype= getAirtype(type);
        
        f= new Flight(id, Ftype, airtype, name, scheduledTime, dir);

        if(airtype == AirlineType:: CARGO){
            if(Ftype== INTERNATIONAL_ARRIVAL || Ftype== INTERNATIONAL_DEPARTURE)
                rwyCDispatcher.internationalFlights->addFlight(f);
            else
                rwyCDispatcher.domesticFlights->addFlight(f);
        }
        else if(f->isArrival()){
            if(Ftype== INTERNATIONAL_ARRIVAL)
                rwyADispatcher.internationalFlights->addFlight(f);
            else
                rwyADispatcher.domesticFlights->addFlight(f);
        }
        else if(f->isDeparture()){
           if(Ftype== INTERNATIONAL_DEPARTURE)
                rwyBDispatcher.internationalFlights->addFlight(f);
            else
                rwyBDispatcher.domesticFlights->addFlight(f);
        }
        f->printStatus();
    }
    */


    pthread_t dispatcherA;
    pthread_t dispatcherB;
    pthread_t dispatcherC;

    Timer timer;
    pthread_t timerThread;

    Flight* f1 = new Flight(1, FlightType:: INTERNATIONAL_ARRIVAL, AirlineType:: COMMERCIAL, AirlineName:: PIA,12.0,'N');
    Flight* f2 = new Flight(2, FlightType:: DOMESTIC_ARRIVAL, AirlineType:: MILITARY, AirlineName::Pakistan_Airforce,12.30,'S');
    //rwyADispatcher.internationalFlights->addFlight(f1);
    rwyADispatcher.domesticFlights->addFlight(f2);
     
    Flight* f3 = new Flight(3, FlightType:: DOMESTIC_DEPARTURE, AirlineType:: COMMERCIAL, AirlineName:: PIA,12.0,'W');
    Flight* f4 = new Flight(4, FlightType:: INTERNATIONAL_DEPARTURE, AirlineType:: MEDICAL, AirlineName:: AghaKhan_Air_Ambulance,12.01,'E');
    rwyBDispatcher.internationalFlights->addFlight(f4);
    rwyBDispatcher.domesticFlights->addFlight(f3);

    //Flight* f5 = new Flight(5, FlightType:: DOMESTIC_ARRIVAL, AirlineType:: CARGO, AirlineName:: FedEx);
    //Flight* f6 = new Flight(6, FlightType:: INTERNATIONAL_DEPARTURE, AirlineType:: CARGO, AirlineName:: Blue_Dart);
    //rwyCDispatcher.domesticFlights->addFlight(f5);
    //rwyCDispatcher.internationalFlights->addFlight(f6);

    pthread_create(&dispatcherA, nullptr, Dispatcher::dispatchFlights, (void*)&rwyADispatcher);   
    pthread_create(&dispatcherB, nullptr, Dispatcher::dispatchFlights, (void*)&rwyBDispatcher);
    //pthread_create(&dispatcherC, nullptr, Dispatcher::dispatchFlights, (void*)&rwyCDispatcher);

    sleep(1);
    pthread_create(&timerThread,NULL,timer.simulationTimer,NULL);

    //start graphics window
    sf::RenderWindow window(sf::VideoMode(960, 600), "ATCS");
    sf::Texture backgroundTexture;
    if (!backgroundTexture.loadFromFile("background.png")) {
        printf("cant load texture\n");
    }

    sf::Sprite backgroundSprite(backgroundTexture);
    while (window.isOpen() )
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(backgroundSprite);
        rwyADispatcher.drawFlights(&rwyADispatcher,window);
        rwyBDispatcher.drawFlights(&rwyBDispatcher,window);
        //rwyCDispatcher.drawFlights(window);
        window.display();
    }


    pthread_join(dispatcherA, NULL);
    pthread_join(dispatcherB, NULL);
    //pthread_join(dispatcherC, NULL);
    pthread_join(timerThread, NULL);
    return 0;
}