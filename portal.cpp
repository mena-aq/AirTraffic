#include <vector>
#include <string>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

#include "Challan.h"
#include "Types.h"

vector<Challan> challanList;
vector<Challan> history;

void printChallans() {
    for (int i = 0; i < challanList.size(); i++) {
        cout << "\n--- Challan " << i + 1 << " ---\n";
        challanList[i].printChallan();
    }
}

void updateChallan(Challan* updated) {
    bool found = false;
    for (int i=0; i<challanList.size(); i++) {
        if (challanList[i].flightID == updated->flightID && challanList[i].status == 0) {
            found = true;

            challanList[i].status = updated->status;
            challanList[i].amountDue = updated->amountDue;

            //if challan is fully paid, add it to history and remove from active avn
            if(challanList[i].amountDue<=0)
            {
                Challan newChallan= challanList[i];
                history.push_back(newChallan);
                challanList.erase(challanList.begin() + i);
            }

            cout << "Updated challan:\n";
            challanList[i].printChallan();

            free(updated);
            break;
        }
    }

} 

void viewActive(){
    cout<<"-------------Active--------------\n";
    for(int i=0; i<challanList.size(); i++){
        challanList[i].printChallan();
         //cout<"AVN "<< (i+1)<<endl;
    }
}

void viewHistory(){
    cout<<"-------------History--------------\n";
    for(int i=0; i<history.size(); i++){
        //cout<"AVN "<< (i+1)<<endl;
        history[i].printChallan();
    }
}

int main(){

    while(1){
        /*cout<<"V1) View history\n2) View active AVNs\n";
        int x;
        cin>>x;

        if(x==1){
            viewHistory();
        }
        if(x==2){
            viewActive();
        }*/

        Challan* challan = new Challan;

        //get challan from generator and store in vector
        int fd = open(PAY_FIFO1, O_RDONLY);

        if (read(fd, (void*) challan, sizeof(Challan)) > 0) {
            challanList.push_back(*challan);
            cout << "New Challan received:\n";

            challan->printChallan();
        }
        close(fd);

        challan = new Challan;

        fd = open(PAY_FIFO4, O_RDONLY);
        read(fd, challan, sizeof(Challan));
        close(fd);

        challanList.push_back(*challan);
        cout << "Challan Confirmation received from strip pay:\n";

        updateChallan(challan);
    }
    return 0;
}
