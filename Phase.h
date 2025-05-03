#ifndef PHASE_H
#define PHASE_H

// Phase configuration class
class Phase {
    public:
        float speedLowerLimit;
        float speedUpperLimit;
        float altitudeLowerLimit;
        float altitudeUpperLimit;
    
        //default constructor
        Phase() : speedLowerLimit(0.0), speedUpperLimit(0.0), altitudeLowerLimit(0.0), altitudeUpperLimit(0.0) {}
        //parameterised constructor
        Phase(float sl, float su, float al, float au)
            : speedLowerLimit(sl), speedUpperLimit(su), altitudeLowerLimit(al), altitudeUpperLimit(au) {}
};
#endif