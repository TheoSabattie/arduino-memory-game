#ifndef CONTROLLER
#define CONTROLLER

#include "Consts.h"
#include "Vector2.h"

struct Controller {
    private: 
        char readAxis(byte thisAxis) const {
            // read the analog input:
            int reading = analogRead(thisAxis);

            // map the reading from the anazlog input range to the output range:
            return Mathf::remap(reading, -68, 960, 0, MAP_SIZE - 1); // note: offset to fix wrong value (I don't know why it's inacurate)
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