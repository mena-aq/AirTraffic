#ifndef TIMER_H
#define TIMER_H

class Timer{
public:
    static int currentTime;

    static void* simulationTimer(void* arg) {
        time_t start = time(nullptr);

        while ( difftime(time(nullptr), start) < SIMULATION_DURATION) {
            currentTime = difftime(time(nullptr), start);
            std::cout << "\rSimulation Time: " << currentTime << "s / " << SIMULATION_DURATION << "s \n" << flush;
            sleep(1);
        }
        pthread_exit(nullptr);
    }
    int getCurrentTime(){
        return currentTime;
    }

};
int Timer::currentTime = 0;

#endif