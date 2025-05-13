#include "Types.h"
#include "ViolationInfo.h"
#include "ATCDashboard.h"
#include "Challan.h"


ATCDashboard dashboard;

pthread_mutex_t requestLock;

void checkOverdue(){
    //pthread_mutex_lock(&dashboard.lock);
    for (int i=0; i<dashboard.numViolations; i++){
    if (dashboard.AVNs[i]!=nullptr){
        if (dashboard.AVNs[i]->status == 0 && time(0)>dashboard.AVNs[i]->dueDate){
            dashboard.AVNs[i]->status = 2; //overdue
        }
    }
    }
    //pthread_mutex_unlock(&dashboard.lock);
}

void* generateAVNs(void* arg){
    int fd=-1;  
    while (1){
        //read any AVN request
        ViolationInfo violation;
        fd = open(AVN_FIFO1,O_RDONLY);
        read(fd,(void*)&violation,sizeof(ViolationInfo));
        close(fd);
        cout<<"read flight " << violation.violationTimestamp;
        violation.generateFee();
        //add to list
        pthread_mutex_lock(&dashboard.lock);
        dashboard.addAVN(violation);
        pthread_mutex_unlock(&dashboard.lock);
        printf("generate avn for flight %d",violation.flightID);

        //delete violation;
       
    }
    pthread_exit(NULL);
}

void* receivePayments(void* arg){
    //reads the confirmation or status apdate from stripe payment
    //send clearance to ATCS

    int fd = -1;
    int clearanceID = -1;
    while (1) {    
        // Read id of cleared flight
        fd = open(PAY_FIFO6, O_RDONLY);
        read(fd, &clearanceID, sizeof(clearanceID));
        close(fd);

        cout<< "Read clearance id from strip: "<<clearanceID<<endl;
        //inform atcs
        if (clearanceID!=-1){
            fd = open(AVN_FIFO2, O_WRONLY);
            write(fd, &clearanceID, sizeof(clearanceID));
            close(fd);
            dashboard.clearAVNByID(clearanceID); //by flightID
        }
        clearanceID = -1;
        
    }
    pthread_exit(NULL);

}

void* sendAdminChallans(void* arg){
    //when admin portal logs in
    //send all asscociated avns as challans

    while(1){
        //receive airline 
        int airline;
        int fd = open(PAY_FIFO4,O_RDONLY);
        read(fd,&airline,sizeof(airline)); 
        close(fd);
        std::cout << "Portal AIrline " << airline;

        //send all associated challans
        fd = open(PAY_FIFO1,O_WRONLY); 

        pthread_mutex_lock(&dashboard.lock);
        for (int i=0; i<dashboard.numViolations; i++){
            if (dashboard.AVNs[i]->airline == airline){
                AVN* thisAvn = dashboard.AVNs[i];
                Challan challan(thisAvn->avnID,thisAvn->flightID,static_cast<int>(thisAvn->airline),static_cast<int>(thisAvn->airlineType),thisAvn->amountDue,thisAvn->status, thisAvn->dueDate);
                write(fd,(void*)&challan, sizeof(challan));
                printf("sent challan %d\n",challan.avnID);
            }
        }
        close(fd);
        pthread_mutex_unlock(&dashboard.lock);
    }
    pthread_exit(NULL);
}
    
void* sendToPayment(void* arg){
    //stripe pay requesta a challn

    while(1){
        //receive id 
        int avnID = -1;
        int fd = open(PAY_FIFO2,O_RDONLY);
        read(fd,&avnID,sizeof(int)); 
        close(fd);
        std::cout << "SPay avn id " << avnID;

        Challan* challan;
        //send challan to pay
        if (avnID!=-1){
            AVN* thisAvn = dashboard.getAVNByID(avnID);
            if (thisAvn)
                challan = new Challan(thisAvn->avnID,thisAvn->flightID,static_cast<int>(thisAvn->airline),static_cast<int>(thisAvn->airlineType),thisAvn->amountDue,thisAvn->status, thisAvn->dueDate);
            else
                challan = new Challan;
            fd = open(PAY_FIFO3,O_WRONLY);
            write(fd,(void*)challan,sizeof(Challan));
            close(fd);
        }
        else{
            challan = new Challan;
            fd = open(PAY_FIFO3,O_WRONLY);
            write(fd,(void*)challan,sizeof(Challan));
            close(fd);
        }
    }
    pthread_exit(NULL);
}


int main(){

    loadFont();

    pthread_mutex_init(&requestLock,NULL);

    mkfifo(AVN_FIFO1,0666);
    mkfifo(AVN_FIFO2,0666);

    mkfifo(PAY_FIFO1,0666);
    mkfifo(PAY_FIFO2,0666);
    mkfifo(PAY_FIFO3,0666);
    mkfifo(PAY_FIFO4,0666);
    mkfifo(PAY_FIFO5,0666);
    mkfifo(PAY_FIFO6,0666);


    pthread_t AVNreceive; 
    pthread_create(&AVNreceive,NULL,generateAVNs,NULL);

    pthread_t paymentReceive;
    pthread_create(&paymentReceive,NULL,receivePayments,NULL);

    pthread_t adminSend;
    pthread_create(&adminSend,NULL,sendAdminChallans,NULL);

    pthread_t paymentSend;
    pthread_create(&paymentSend,NULL,sendToPayment,NULL);


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

        checkOverdue();

        window.clear();
        window.draw(bgSprite);
        dashboard.displayAVNs(window);
        window.display();
    }

    unlink(AVN_FIFO1);
    unlink(AVN_FIFO2);

    unlink(PAY_FIFO1);
    unlink(PAY_FIFO2);
    unlink(PAY_FIFO3);
    unlink(PAY_FIFO4);
    unlink(PAY_FIFO5);
    unlink(PAY_FIFO6);
    
}