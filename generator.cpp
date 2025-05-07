#include "Types.h"
#include "ViolationInfo.h"
#include "ATCDashboard.h"
#include "Challan.h"

//FIFO
#define AVN_FIFO1 "pipes/avnfifo_ATC"
#define AVN_FIFO2 "pipes/avnfifo_GEN"

ATCDashboard dashboard;

void* generateAVNs(void* arg){
    int fd=-1;
    while (1){

        //read any AVN request
        ViolationInfo* violation = new ViolationInfo;
        fd = open(AVN_FIFO1,O_RDONLY);
        read(fd,(void*)violation,sizeof(ViolationInfo));
        close(fd);
        violation->generateFee();
//printf("returned fee: %f\n",violation->amountDue);
        //add to list
        Challan* challan = new Challan(violation->flightID, violation->airline, violation->airlineType, violation->amountDue, violation->status);

        //write challan to the airline portal
        fd = open(PAY_FIFO1,O_WRONLY);
        write(fd,(void*)challan, sizeof(Challan));
        close(fd);

        challan->printChallan();

        //write challan to stripPay
        fd = open(PAY_FIFO3,O_WRONLY);
        write(fd,(void*)challan, sizeof(Challan));
        close(fd);

        dashboard.addAVN(violation);
        free(violation);
        free(challan);
        
    }
    pthread_exit(NULL);
}

    
void* readPayment(void* arg){
    int fd = -1;
    while (1) {
        Challan* challan = new Challan;

        // Read updated challan from StripePay
        fd = open(PAY_FIFO2, O_RDONLY);
        read(fd, (void*)challan, sizeof(Challan));
        close(fd);

        // If fully paid, inform controller
        if (challan->amountDue <= 0) {
            ViolationInfo *violation = new ViolationInfo;
            violation->flightID = challan->flightID;
            violation->airline = challan->airline;
            violation->airlineType = challan->airlineType;
            violation->amountDue = 0;
            violation->status = 1;

            fd = open(AVN_FIFO2, O_WRONLY);
            write(fd, (void*)violation, sizeof(ViolationInfo));
            close(fd);
        }

        free(challan);
    }

    pthread_exit(NULL);
}


int main(){

    mkfifo(AVN_FIFO1,0666);
    mkfifo(AVN_FIFO2,0666);

    mkfifo(PAY_FIFO1,0666);
    mkfifo(PAY_FIFO2,0666);
    mkfifo(PAY_FIFO3,0666);
    mkfifo(PAY_FIFO4,0666);

    pthread_t AVNreceive; //with ATCS process to receive and make AVN
    pthread_t AVNpay;

    //stripe pay admin portal
    pthread_create(&AVNreceive,NULL,generateAVNs,NULL);
    pthread_create(&AVNpay, NULL, readPayment, NULL);

    //main thread prints avn window
    sf::RenderWindow window(sf::VideoMode(600,720),"AVN Dashboard");
    sf::Texture avnBG;
    if (!avnBG.loadFromFile("AVN.png")) {
        printf("cant load texture\n");
    }
    sf::Sprite bgSprite(avnBG);
    while (window.isOpen() )
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
        window.draw(bgSprite);
        dashboard.displayAVNs(window);
        window.display();
    }

    pthread_join(AVNreceive,NULL);
    pthread_join(AVNpay, NULL);
}