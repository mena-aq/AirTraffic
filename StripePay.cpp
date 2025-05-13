#include <iostream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include <chrono>

#include "Challan.h"
#include "Types.h"

using namespace std;

int main(){

    int fd=-1;
    while (1){

        //request AVN By id
        int challanID;
        cout << "Enter challan ID: ";
        cin >> challanID;

        //send requesred id to generator
        fd = open(PAY_FIFO2,O_WRONLY);
        write(fd,&challanID,sizeof(challanID));
        close(fd);
        
        //receuve challan
        Challan challan;;
        fd = open(PAY_FIFO3,O_RDONLY);
        read(fd,(void*)&challan,sizeof(Challan));
        close(fd);

        if (challan.avnID==-1){
            cout << "Invalid AVN! Retry\n";
        }
        else{
            //read paid amount
            int amount;
            std::cout<<"Enter payment amount: ";
            cin>>amount;
            while(amount< challan.amountDue || challan.amountDue< amount){
                if(amount< challan.amountDue){
                    cout<<"The amount you entered is insufficient to clear the fee! Kindly enter again: ";
                    cin>> amount;
                }
                else{
                    cout<<"The amount you entered is more than required to clear the fee! Kindly enter again: ";
                    cin>> amount;
                }
            }

            challan.CalculatePayment(amount);
            //challan->printChallan();

            //send clearance back to generator
            int flightID = challan.flightID;
            fd = open(PAY_FIFO6,O_WRONLY);
            write(fd,&flightID,sizeof(flightID));
            close(fd);
            cout<<"sent clearance id to gen: "<<flightID<<endl;

            fd=open(PAY_FIFO5, O_WRONLY);
            write(fd, (void*)&challan, sizeof(Challan));
            close(fd);


           // delete challan;
        }           
    }
}