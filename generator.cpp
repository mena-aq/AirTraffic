#include "Types.h"
#include "ViolationInfo.h"
#include "ATCDashboard.h"

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
        //add to list
        dashboard.addAVN(violation);
        free(violation);

    }
    pthread_exit(NULL);
}

int main(){

    mkfifo(AVN_FIFO1,0666);
    mkfifo(AVN_FIFO2,0666);
    loadFont();

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
}