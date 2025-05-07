#include <vector>
#include <string>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

using namespace std;

#include "Challan.h"
#include "Types.h"

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
    //pthread_mutex_lock(&challanMutex);
    cout << "-------------Active--------------\n";
    for (int i = 0; i < challanList.size(); i++) {
        challanList[i].printChallan();
    }
    //pthread_mutex_unlock(&challanMutex);
}

//shows previous challans/avns
void viewHistory() {
    //pthread_mutex_lock(&challanMutex);
    cout << "-------------History--------------\n";
    for (int i = 0; i < history.size(); i++) {
        history[i].printChallan();
    }
    //pthread_mutex_unlock(&challanMutex);
}


void* updateChallan(void* arg) {
    int fd = -1;

    while (1) {
        Challan* challan = new Challan;

        //wait and read confirmation from strip pay
        fd = open(PAY_FIFO4, O_RDONLY);
        read(fd, challan, sizeof(Challan));
        close(fd);

        pthread_mutex_lock(&challanMutex);

        cout << "Challan Confirmation received from stripe pay:\n";

        bool found = false;
        //finds that challan in the list and updates it
        viewActive();
        viewHistory();
        for (int i = 0; i < challanList.size(); i++) {
            if (challanList[i].flightID == challan->flightID && challanList[i].status == 0) {
                found = true;

                challanList[i].status = challan->status;
                challanList[i].amountDue = challan->amountDue;

                if (challanList[i].amountDue <= 0) {
                    //if challan fully paid, add it to history and remove from list
                    Challan newChallan = challanList[i];
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

void* recieveChallan(void* arg) {
    int fd = -1;

    while (1) {
        Challan* challan = new Challan;

        fd = open(PAY_FIFO1, O_RDONLY);

        if (read(fd, challan, sizeof(Challan)) > 0) {
            pthread_mutex_lock(&challanMutex);
            challanList.push_back(*challan);
            pthread_mutex_unlock(&challanMutex);

            cout << "New Challan received:\n";
            challan->printChallan();
        }
        close(fd);
        delete challan;
    }

    pthread_exit(NULL);
}

int main() {
    //thread to update a challan when confirmation is recieved
    pthread_t updateChallanTID;
    // thread that recieves challan from generator
    pthread_t recieveChallanTID;

    pthread_create(&recieveChallanTID, NULL, recieveChallan, NULL);
    pthread_create(&updateChallanTID, NULL, updateChallan, NULL);

    pthread_join(updateChallanTID, NULL);
    pthread_join(recieveChallanTID, NULL);

    return 0;
}
