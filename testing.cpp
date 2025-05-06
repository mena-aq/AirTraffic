#include "AVN.h"
#include <vector>

class ATCDashboard {
public:
    int numViolations;
    std::vector<AVN> AVNs;
     float max_Y;
    float max_X;

    ATCDashboard(){
        this->numViolations=0;
        this->max_Y = 80;
        this->max_X = 10;
    }
    void requestAVN(Flight* requestingFlight){
        //send to AVN generator to generate AVN
        //send id, airline name,airline type, recorded speed, phase
        printf("Request AVN\n");
        ViolationInfo* violation = new ViolationInfo(requestingFlight->id,static_cast<int>(requestingFlight->airlineName),static_cast<int>(requestingFlight->airlineType),requestingFlight->speed,static_cast<int>(requestingFlight->phase));
         std::cout<<violation->flightID<<" "<<int(violation->airlineType)<<" "<<violation->amountDue<<std::endl;
        int fd = open(AVN_FIFO1,O_WRONLY,0666);
        write(fd,(void*)violation,sizeof(ViolationInfo));
        close(fd);

        printf("avn received\n");
        //get back fee info
        fd = open(AVN_FIFO2,O_RDONLY,0666);
        read(fd,(void*)violation,sizeof(ViolationInfo));
        close(fd);

        //write
        AVN avn(violation->flightID,static_cast<AirlineName>(violation->airline),static_cast<AirlineType>(violation->airlineType),violation->speedRecorded,static_cast<FlightPhase>(violation->phaseViolation),violation->violationTimestamp,violation->amountDue);
        avn.initGraphic(max_X,max_Y);
        AVNs.push_back(avn);
    }

    void printAVNs(){
        for (int i=0; i<AVNs.size(); i++){
            AVNs[i].printAVN();
        }
    }

    //clear AVN

    //graphics
    void displayAVNs(sf::RenderWindow& window){
        for (int i=0; i<AVNs.size(); i++){
            AVNs[i].drawGraphic(window);
        }
    }

    //recalculate
    
};

int main(){
    Flight* f1 = new Flight(1, FlightType:: INTERNATIONAL_ARRIVAL, AirlineType:: COMMERCIAL, AirlineName:: PIA,12.0,'N');
    Flight* f2 = new Flight(2, FlightType:: DOMESTIC_ARRIVAL, AirlineType:: MILITARY, AirlineName::Pakistan_Airforce,12.30,'S');


    f1->printStatus();
    f2->printStatus();

    ATCDashboard atc;
    atc.requestAVN(f1);
    sleep(5);
    atc.requestAVN(f2);
    return 0;
}
