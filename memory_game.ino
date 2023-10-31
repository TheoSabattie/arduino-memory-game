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

struct Vector2 {
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
        unsigned long lastMillis;
        bool ledIsOn;
        Map& map;
        Controller& controller;

        long previousMoveMillis;
        Vector2 previousAxis;

    public:
        Vector2 position;

        Cursor(Map& map, Controller& controller) : map(map), controller(controller) {
            
        }

        bool getLedIsOn(){
            return this->ledIsOn;
        }

        void doAction(){
            unsigned long currentMillis = millis();

            Vector2 currentAxis = controller.getAxis();

            if (currentMillis > lastMillis + LED_BLINK_FREQUENCY) {
                lastMillis = currentMillis;
                ledIsOn = !ledIsOn;
            }

            if (currentAxis != previousAxis) {
                previousAxis = currentAxis;
                move(currentAxis, currentMillis);
            } else if (currentMillis > previousMoveMillis + MOVE_FREQUENCY && !currentAxis.isZero()){
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

      this->previousMoveMillis = currentMillis;
    }
};


// parameters for reading the joystick:
const int responseDelay = 1000;   // response delay of the mouse, in ms

char textBuffer[40];

void setup() {
  pinMode(WIN_LED_ID, OUTPUT);
  pinMode(JOY_BUTTON_ID, INPUT);
  pinMode(BUTTON_ID, INPUT);
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,0);
  lc.clearDisplay(0);
  Serial.begin(9600);
}

enum class GameState : unsigned char { 
  None = 0,
  ShowObjective = 1,
  RetreiveObjective = 2,
  GameOver = 3,
  Win = 4
};

struct Game {
    private:
        GameState state;
        Controller controller;
        Map objectiveMap;
        Map currentMap;
        Cursor cursor;
        Light winLight;

    public:
        Game() : cursor(currentMap, controller), winLight(WIN_LED_ID) {
            for (char x = 0; x < MAP_SIZE; ++x) {
            for (char y = 0; y < MAP_SIZE; ++y) {
                objectiveMap.setAt({x, y}, random(2) == 1);
            }
            }
        }

        void doAction(){
            if (state == GameState::None){
                state = GameState::ShowObjective;
            } else if (state == GameState::ShowObjective) {
                if (controller.isButtonJustDown()) {
                    state = GameState::RetreiveObjective;
                }
            } else if (state == GameState::RetreiveObjective) {
                cursor.doAction();

                if (controller.isButtonJustDown()){
                    if (!objectiveMap.getAt(cursor.position)) {
                        state = GameState::GameOver;
                    } else {
                        currentMap.setAt(cursor.position, true);
                        winLight.switchToFor(true, 1000);
                        // TODO: si plus rien à trouver: win feedback        
                    }
                }
            } else if (state == GameState::GameOver) {
            // feedback ?
            if (controller.isButtonJustDown()) {
                currentMap.clear();
                cursor.position = {0,0};
                state = GameState::None;
            }
            } else if (state == GameState::Win) {
            // feedback
            }

            controller.doAction();
            winLight.doAction();
        }

        void render(){
            if (state == GameState::RetreiveObjective) {
                for (char x = 0; x < MAP_SIZE; ++x) {
                    for (char y = 0; y < MAP_SIZE; ++y) {
                        if (!(x == cursor.position.x && y == cursor.position.y))
                            lc.setLed(0, y, x, currentMap.getAt({x, y}));
                    }
                }

                lc.setLed(0, cursor.position.y, cursor.position.x, cursor.getLedIsOn());
            } else if (state == GameState::ShowObjective) {
                for (char x = 0; x < MAP_SIZE; ++x) {
                    for (char y = 0; y < MAP_SIZE; ++y) {
                        lc.setLed(0, y, x, objectiveMap.getAt({x, y}));
                    }
                }
            } else if (state == GameState::GameOver){
                for (char x = 0; x < MAP_SIZE; ++x) {
                    for (char y = 0; y < MAP_SIZE; ++y) {
                        lc.setLed(0, y, x, true);
                    }
                }
            }
        }
};

Game game;

void loop() { 
  
  game.doAction();
  game.render();
  //sprintf(textBuffer, "cursor x: %d, y: %d", objectiveMap.getAt(0,0), 10);
  //Serial.println(textBuffer);
  //delay(1000);
}

