#ifndef CONTROLLER
#define CONTROLLER

#include "Consts.h"
#include "Vector2.h"

struct Controller {
    private: 
        char readAxis(byte thisAxis) const {
            // read the analog input:
            int reading = analogRead(thisAxis);

            // map the reading from the analog input range to the output range:
            return map(reading, 0, 1023, 0, MAP_SIZE - 1);
        }

        bool previousButtonDownState = false;

    public:
        Vector2 getPosition(){
            return {readAxis(X_OUT), readAxis(Y_OUT)};
        }

        bool isButtonDown(){
            return digitalRead(BUTTON_IN);
        }

        bool isButtonJustDown(){
            return !previousButtonDownState && digitalRead(BUTTON_IN);
        } 

        void doAction(){
            previousButtonDownState = digitalRead(BUTTON_IN);
        }
};
#endif