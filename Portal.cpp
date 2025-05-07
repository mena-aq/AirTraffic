#include <vector>
#include <string>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

using namespace std;

#include "Challan.h"
#include "Types.h"

#define PAY_FIFO1 "pipes/payfifo_PORTAL"
#define PAY_FIFO2 "pipes/payfifo_GEN"
#define PAY_FIFO3 "pipes/payfifo_STRIP"
#define PAY_FIFO4 "pipes/payfifo_PORTAL-STRIP"
#define PAY_FIFO5 "pipes/payfifo_STRIP-PORTAL"

vector<Challan> challanList;
vector<Challan> history;

pthread_mutex_t challanMutex = PTHREAD_MUTEX_INITIALIZER;


//to print all challans
void printChallans() {
    for (int i = 0; i < challanList.size(); i++) {
        pthread_mutex_lock(&challanMutex);
        cout << "\n--- Challan " << i + 1 << " ---\n";
        challanList[i].printChallan();
        pthread_mutex_unlock(&challanMutex);
    }
}

//shows active challans/avns
void viewActive() {
    pthread_mutex_lock(&challanMutex);
    cout << "-------------Active--------------\n";
    for (int i = 0; i < challanList.size(); i++) {
        challanList[i].printChallan();
    }
    pthread_mutex_unlock(&challanMutex);
}

//shows previous challans/avns
void viewHistory() {
    pthread_mutex_lock(&challanMutex);
    cout << "-------------History--------------\n";
    for (int i = 0; i < history.size(); i++) {
        history[i].printChallan();
    }
    pthread_mutex_unlock(&challanMutex);
}


void* updateChallan(void* arg) {
    int fd = -1;

    while (1) {
        Challan* challan = new Challan;

        //wait and read confirmation from strip pay
        fd = open(PAY_FIFO4, O_RDONLY);
        read(fd, (void*)challan, sizeof(Challan));
        close(fd);

        pthread_mutex_lock(&challanMutex);

        cout << "Challan Confirmation received from stripe pay:\n";

        bool found = false;
        //finds that challan in the list and updates it
        //viewActive();
        //viewHistory();
        for (int i = 0; i < challanList.size(); i++) {
            if (challanList[i].flightID == challan->flightID && challanList[i].status == 0) {
                found = true;

                challanList[i].status = challan->status;
                challanList[i].amountDue = challan->amountDue;

                if (challanList[i].amountDue <= 0) {
                    //if challan fully paid, add it to history and remove from list
                    Challan newChallan = challanList[i];

                    //update status to paid;
                     newChallan.status=1;

                    history.push_back(newChallan);
                    challanList.erase(challanList.begin() + i);
                    cout << "Updated challan:\n";
                    newChallan.printChallan();
                }
                else{
                    cout << "Updated challan:\n";
                    challanList[i].printChallan();
                }
                break;
            }
        }

        pthread_mutex_unlock(&challanMutex);

        free(challan);
    }

    pthread_exit(NULL);
}

void receiveChallan(int airline) {   
    //send airline
    int fd = open(PAY_FIFO3,O_WRONLY);
    write(fd,(void*)&airline,sizeof(airline));;
    close(fd);
    
    //read all challans for airline into challn list/history
    Challan* challan = new Challan;

    fd = open(PAY_FIFO1, O_RDONLY);
    while (read(fd, (void*)challan, sizeof(Challan)) > 0) {
        pthread_mutex_lock(&challanMutex);
        if (challan->status == 1)
            history.push_back(*challan);
        else
            challanList.push_back(*challan);
        pthread_mutex_unlock(&challanMutex);

        cout << "New Challan received:\n";
        challan->printChallan();
    }
    close(fd);
    delete challan;

    pthread_exit(NULL);
}


int inputAirline(){
    int airline;

    std::cout<<"AIRLINE PORTAL\n";
    std::cout<<"1)PIA\n";
    std::cout<<"2)AirBlue\n";
    std::cout<<"3)FedEx\n";
    std::cout<<"4)Pakistan Airforce\n";
    std::cout<<"5)Blue Dart\n";
    std::cout<<"6)AghaKhan Air Ambulance\n\n";
    std::cout<<"Enter airline: ";

    std::cin>>airline;
    while(int(airline)<1 || int(airline) > 6){
        cout<<"Invalid airline!\n Enter again: ";
        cin>>airline;
    }
    return airline;
}

int main() {

    pthread_mutex_init(&challanMutex,NULL);

    int airline = inputAirline();

    //eveytime the process starts read from geerator all relavant challans
    receiveChallan(airline); //read once 

    //thread to update a challan when confirmation is recieved
    pthread_t updateChallanTID;
    //(void*)&airline    
    pthread_create(&updateChallanTID, NULL, updateChallan, NULL);
        
    int op;
    while (1){
        
        cout << "\n\n > Refresh? : 1\n";
        cout << "> LogOut? : 2\n";
        cin >> op;
        if (op == 1){
            pthread_mutex_lock(&challanMutex);
            challanList.clear();
            history.clear();
            pthread_mutex_unlock(&challanMutex);
            receiveChallan(airline);
        }
        else if (op==2){
            break;
        }
    }
    
    cout << "logging out...";
    sleep(1);
    pthread_join(updateChallanTID, NULL);

    return 0;
}