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
        Challan* challan = new Challan;

        fd = open(PAY_FIFO3,O_RDONLY);
        read(fd,(void*)challan,sizeof(Challan));
        close(fd);

        cout<<"Read from Generator\n";
        challan->printChallan();

        //read paid amount
        int amount;
        std::cout<<"Enter payment amount: ";
        cin>>amount;
        while(amount< challan->amountDue || challan->amountDue< amount){
            if(amount< challan->amountDue){
                cout<<"The amount you entered is insufficient to clear the fee! Kindly enter again: ";
                cin>> amount;
            }
            else{
                cout<<"The amount you entered is more than required to clear the fee! Kindly enter again: ";
                cin>> amount;
            }
        }
        
        challan->CalculatePayment(amount);
        //challan->printChallan();

        //send confirmation back to PORTal
        fd=open(PAY_FIFO4, O_WRONLY);
        write(fd, (void*)challan, sizeof(Challan));

        //calculate amount due and send back to generator
        fd = open(PAY_FIFO2,O_WRONLY);
        write(fd,(void*)challan,sizeof(Challan));
        close(fd);
        
        free(challan);
    }
}