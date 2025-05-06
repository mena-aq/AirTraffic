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

        Challan* challan = new Challan(violation->flightID, violation->airline, violation->airlineType, violation->amountDue, violation->status);

        //write challan to the airline portal
        fd = open(PAY_FIFO1,O_WRONLY);
        write(fd,(void*)challan, sizeof(challan));
        close(fd);

        challan->printChallan();

        //write challan to stripPay
        fd = open(PAY_FIFO3,O_WRONLY);
        write(fd,(void*)challan, sizeof(Challan));
        close(fd);

        //reads the confirmation or status apdate from strip payment
        fd = open(PAY_FIFO2,O_RDONLY);
        read(fd,(void*)challan, sizeof(Challan));
        close(fd);

        if(challan->amountDue <= 0){
            violation->status=1;
            challan->status=1;
            //if challan cleared write back to controller
            fd = open(AVN_FIFO2,O_WRONLY);
            write(fd,(void*)violation,sizeof(ViolationInfo));
            close(fd);

        }
        //printf("returned fee: %f\n",violation->amountDue);
        //add to list
        dashboard.addAVN(violation);
        free(violation);
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

    //stripe pay admin portal
    pthread_create(&AVNreceive,NULL,generateAVNs,NULL);

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
    int fd=-1;
    while (1){
        //read request
        ViolationInfo* violation = new ViolationInfo;
        fd = open(AVN_FIFO1,O_RDONLY);
        read(fd,(void*)violation,sizeof(ViolationInfo));
        close(fd);

        //return fee
        std::cout<<"From test:\n";
        violation->generateFee();
        std::cout<<violation->flightID<<" "<<violation->airlineType<<" "<<violation->amountDue<<std::endl;

        //create a challan;
        Challan* challan = new Challan(violation->flightID, violation->airline, violation->airlineType, violation->amountDue, violation->status);

        //write challan to the airline portal
        fd = open(PAY_FIFO1,O_WRONLY);
        write(fd,(void*)challan, sizeof(challan));
        close(fd);

        challan->printChallan();

        //write challan to stripPay
        fd = open(PAY_FIFO3,O_WRONLY);
        write(fd,(void*)challan, sizeof(Challan));
        close(fd);

        //reads the confirmation or status apdate from strip payment
        fd = open(PAY_FIFO2,O_RDONLY);
        read(fd,(void*)challan, sizeof(Challan));
        close(fd);

        "Read from strip\n";
        challan->printChallan();

        if(challan->amountDue <= 0){
            violation->status=1;
            challan->status=1;
            //if challan cleared write back to controller
            fd = open(AVN_FIFO2,O_WRONLY);
            write(fd,(void*)violation,sizeof(ViolationInfo));
            close(fd);

        }
        //printf("returned fee: %f\n",violation->amountDue);

        free(violation);
    }
}