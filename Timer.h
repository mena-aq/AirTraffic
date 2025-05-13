#ifndef TIMER_H
#define TIMER_H

class Timer{
public:
    static int currentTime;
    sf::Text timerText;

    Timer(){
        timerText.setFont(globalFont);
        timerText.setCharacterSize(20);
        timerText.setFillColor(sf::Color::White);
        timerText.setPosition(10, 570);
    }

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

    void drawTimer(sf::RenderWindow& window){
        timerText.setString("Simulation Time: " + std::to_string(Timer::currentTime) + " / 300s");
        window.draw(timerText);
    }

};
int Timer::currentTime = 0;

#endif