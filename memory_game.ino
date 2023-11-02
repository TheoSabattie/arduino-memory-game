#include <Adafruit_NeoPixel.h>

//We always have to include the library
#include "LedControl.h"

#define MAP_SIZE 8

const int LED_CONTROL_DIN_ID = 12;
const int LED_CONTROL_CLK_ID = 11;
const int LED_CONTROL_CS_ID = 10;

const unsigned int WIN_LED_ID = 4;
const int BUTTON_ID = 2;
const int JOY_BUTTON_ID = 8;
const int JOY_X_ID = A0;
const int JOY_Y_ID = A1;


LedControl lc = LedControl(LED_CONTROL_DIN_ID, LED_CONTROL_CLK_ID, LED_CONTROL_CS_ID, 1);

struct Color {
    public:
        byte r;
        byte g;
        byte b;

        Color(byte r, byte g, byte b): r(r), g(g), b(b) {}

        uint32_t toGRB(){
            return (uint32_t(g) << 16) | (uint32_t(r) << 8) | b; 
        }

        uint32_t toRGB(){
            return (uint32_t(r) << 16) | (uint32_t(g) << 8) | b; 
        }
};

struct ProgressBar {
    private:
        Adafruit_NeoPixel strip;
        uint16_t currentProgress = 0;
        uint16_t maxProgress = 10;

    public:
        ProgressBar(uint16_t ledAmount, int16_t ledInputPin = 6, neoPixelType ledType = NEO_GRB + NEO_KHZ800) : strip(ledAmount, ledInputPin, ledType){
        }

        void begin() {
            strip.begin();
            strip.setBrightness(50);
            strip.show();
        }

        void doAction(){
            strip.setPixelColor(0, Color(255,0,0).toGRB());
            strip.show();
        }
};



struct Vector2 {
    public:
        char x;
        char y;

        bool isZero(){
            return x == 0 && y == 0; 
        }

        inline bool operator==(const Vector2& rhs) { return x == rhs.x && y == rhs.y; }
        inline bool operator!=(const Vector2& rhs) { return x != rhs.x || y != rhs.y; }
};

struct Map {
    private:
        bool _map[MAP_SIZE][MAP_SIZE];

    public:
        bool getAt(Vector2 position) {
        return this->_map[position.x][position.y];
        }

        void setAt(Vector2 position, bool isOn) {
        this->_map[position.x][position.y] = isOn;
        }

        void clear(){
            for (char x = 0; x < MAP_SIZE; ++x){
                for (char y = 0; y < MAP_SIZE; ++y){
                    this->setAt({x, y}, false);
                }
            }
        }
};

struct Light {
    private:
        bool _isOn;

    public:
        unsigned int id;
        unsigned long millisForSwitch;

        Light(unsigned int id) : id(id) {}

        bool isOn(){
            return this->_isOn;
        }

        void switchTo(bool isOn){
            this->_isOn = isOn;
            digitalWrite(id, isOn ? HIGH : LOW);
        }

        void switchToFor(bool isOn, unsigned long duration){
            this->switchTo(isOn);
            this->millisForSwitch = millis() + duration;
        }

        void doAction() {
            if (this->millisForSwitch > 0 && millis() > this->millisForSwitch){
                this->millisForSwitch = 0;
                this->switchTo(!this->_isOn);
            }
        }    
};

struct Controller {
    private: 
        char readAxis(byte thisAxis) {
            // read the analog input:
            int reading = analogRead(thisAxis);

            // map the reading from the analog input range to the output range:
            reading = map(reading, 0, 1023, 0, 3);
            return reading - 1;
        }

        bool previousButtonDownState = false;

    public:
        Vector2 getAxis(){
            return {readAxis(JOY_X_ID), readAxis(JOY_Y_ID)};
        }

        bool isButtonDown(){
            return digitalRead(BUTTON_ID);
        }

        bool isButtonJustDown(){
            return !previousButtonDownState && digitalRead(BUTTON_ID);
        } 

        void doAction(){
            previousButtonDownState = digitalRead(BUTTON_ID);
        }
};

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
        Vector2 position;

        Cursor(Map& _map, Controller& _controller) : _map(_map), _controller(_controller) {
            
        }

        bool getLedIsOn(){
            return this->_ledIsOn;
        }

        void doAction(){
            unsigned long currentMillis = millis();

            Vector2 currentAxis = _controller.getAxis();

            if (currentMillis > _lastMillis + LED_BLINK_FREQUENCY) {
                _lastMillis = currentMillis;
                _ledIsOn = !_ledIsOn;
            }

            if (currentAxis != _previousAxis) {
                _previousAxis = currentAxis;
                move(currentAxis, currentMillis);
            } else if (currentMillis > _previousMoveMillis + MOVE_FREQUENCY && !currentAxis.isZero()){
                move(currentAxis, currentMillis);
            }
        }

  private:
    void move(const Vector2& velocity, unsigned long currentMillis) {
      this->position.x = (this->position.x + velocity.x) % MAP_SIZE;
      this->position.y = (this->position.y + velocity.y) % MAP_SIZE;

      if (this->position.x < 0)
        this->position.x += MAP_SIZE;

      if (this->position.y < 0)
        this->position.y += MAP_SIZE;

      this->_previousMoveMillis = currentMillis;
    }
};


// parameters for reading the joystick:
const int responseDelay = 1000;   // response delay of the mouse, in ms

char textBuffer[40];

enum class GameState : unsigned char { 
    None = 0,
    ShowObjective = 1,
    RetreiveObjective = 2,
    GameOver = 3,
    Win = 4
};

struct Game {
    private:
        GameState _state;
        Controller _controller;
        Map _objectiveMap;
        Map _currentMap;
        Cursor _cursor;
        Light _winLight;
        ProgressBar _progressBar;

    public:
        Game() : _cursor(_currentMap, _controller), _winLight(WIN_LED_ID), _progressBar(10) {
            for (char x = 0; x < MAP_SIZE; ++x) {
                for (char y = 0; y < MAP_SIZE; ++y) {
                    _objectiveMap.setAt({x, y}, random(2) == 1);
                }
            }
        }

        void begin(){
            _progressBar.begin();
        }

        void doAction(){
            if (_state == GameState::None){
                _state = GameState::ShowObjective;
            } else if (_state == GameState::ShowObjective) {
                if (_controller.isButtonJustDown()) {
                    _state = GameState::RetreiveObjective;
                }
            } else if (_state == GameState::RetreiveObjective) {
                _cursor.doAction();

                if (_controller.isButtonJustDown()){
                    if (!_objectiveMap.getAt(_cursor.position)) {
                        _state = GameState::GameOver;
                    } else {
                        _currentMap.setAt(_cursor.position, true);
                        _winLight.switchToFor(true, 1000);
                        // TODO: si plus rien à trouver: win feedback        
                    }
                }
            } else if (_state == GameState::GameOver) {
                // feedback ?
                if (_controller.isButtonJustDown()) {
                    _currentMap.clear();
                    _cursor.position = {0,0};
                    _state = GameState::None;
                }
            } else if (_state == GameState::Win) {
                // feedback
            }

            _controller.doAction();
            _winLight.doAction();
            _progressBar.doAction();
        }

        void render(){
            if (_state == GameState::RetreiveObjective) {
                for (char x = 0; x < MAP_SIZE; ++x) {
                    for (char y = 0; y < MAP_SIZE; ++y) {
                        if (!(x == _cursor.position.x && y == _cursor.position.y))
                            lc.setLed(0, y, x, _currentMap.getAt({x, y}));
                    }
                }

                lc.setLed(0, _cursor.position.y, _cursor.position.x, _cursor.getLedIsOn());
            } else if (_state == GameState::ShowObjective) {
                for (char x = 0; x < MAP_SIZE; ++x) {
                    for (char y = 0; y < MAP_SIZE; ++y) {
                        lc.setLed(0, y, x, _objectiveMap.getAt({x, y}));
                    }
                }
            } else if (_state == GameState::GameOver){
                for (char x = 0; x < MAP_SIZE; ++x) {
                    for (char y = 0; y < MAP_SIZE; ++y) {
                        lc.setLed(0, y, x, true);
                    }
                }
            }
        }
};

Game game;

void setup() {
    pinMode(WIN_LED_ID, OUTPUT);
    pinMode(JOY_BUTTON_ID, INPUT);
    pinMode(BUTTON_ID, INPUT);
    lc.shutdown(0,false);
    /* Set the brightness to a medium values */
    lc.setIntensity(0,0);
    lc.clearDisplay(0);
    Serial.begin(9600);
    game.begin();

     Serial.println(0xFF0000, HEX);
     Serial.println(Color(0,255,0).toGRB(), HEX);
     Serial.println(Color(0,255,0).r);
     Serial.println(Color(0,255,0).g);
     Serial.println(Color(0,255,0).b);
     Serial.println(uint32_t(255) << 16, HEX);
}

void loop() { 
    
    game.doAction();
    game.render();
    //sprintf(textBuffer, "_cursor x: %d, y: %d", _objectiveMap.getAt(0,0), 10);
    //Serial.println(textBuffer);
    //delay(1000);
}

