#ifndef CURSOR
#define CURSOR

#include "Map.h"
#include "Controller.h"
#include "Vector2.h"

struct Cursor {
    private:
        const long LED_BLINK_FREQUENCY = 300;
        const long MOVE_FREQUENCY = 500;
        unsigned long _lastMillis;
        bool _ledIsOn;
        Map& _map;
        Controller& _controller;

        long _previousMoveMillis;
        Vector2 _previousAxis;

    public:
        Vector2 getPosition(){
            return this->_controller.getPosition();
        }

        Cursor(Map& _map, Controller& _controller) : _map(_map), _controller(_controller) {
            
        }

        bool getLedIsOn() const {
            return this->_ledIsOn;
        }

        void doAction(){
            unsigned long currentMillis = millis();

            Vector2 currentAxis = _controller.getPosition();

            if (currentMillis > _lastMillis + LED_BLINK_FREQUENCY) {
                _lastMillis = currentMillis;
                _ledIsOn = !_ledIsOn;
            }
        }
};
#endif