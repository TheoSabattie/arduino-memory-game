#include <Array.h>

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

float lerp(float from, float to, float ratio){
    return (to - from) * ratio + from;
}

float inverseLerp(float value, float from, float to){
    return (value - from) / (to - from);
}

float remap(float value, float fromRangeStart, float fromRangeEnd, float toRangeStart, float toRangeEnd){
    return lerp(toRangeStart, toRangeEnd, inverseLerp(value, fromRangeStart, fromRangeEnd));
}

struct Color {
    public:
        float r;
        float g;
        float b;

        Color() : r(0), g(0), b(0) {}
        Color(float r, float g, float b): r(r), g(g), b(b) {}

        inline const uint32_t toGRB() const {
            return (uint32_t(g * 255) << 16) | (uint32_t(r * 255) << 8) | uint32_t(b * 255); 
        }

        inline const uint32_t toRGB() const {
            return (uint32_t(r * 255) << 16) | (uint32_t(g * 255) << 8) | uint32_t(b * 255); 
        }

        inline static Color lerp(const Color& a, const Color& b, float ratio) {
            return Color(::lerp(a.r, b.r, ratio), ::lerp(a.g, b.g, ratio), ::lerp(a.r, b.r, ratio));
        }

        inline static Color moveTowards(const Color& a, const Color& b, float distance) {
            Color delta = b - a;
            float currentDistance = sqrtf(delta.r * delta.r + delta.g * delta.g + delta.b * delta.b);
            
            if (currentDistance < distance)
                return b;

            Color normalizedDelta = delta / currentDistance;
            return a + normalizedDelta * distance;
        }

        inline Color operator-(const Color rhs) const { return Color(r-rhs.r, g-rhs.g, b-rhs.b); }
        inline Color operator+(const Color rhs) const { return Color(r+rhs.r, g+rhs.g, b+rhs.b); }
        inline Color operator/(const float factor) const { return Color(r/factor, g/factor, b/factor); }
        inline Color operator*(const float factor) const { return Color(r*factor, g*factor, b*factor); }
        inline bool operator==(const Color& rhs) const { return r == rhs.r && g == rhs.g && b == rhs.b; }
        inline bool operator!=(const Color& rhs) const { return r != rhs.r || g != rhs.g || b != rhs.b; }
};

enum class ProgressBarState : byte { 
    None = 0,
    Animated = 1,
    Fail = 2,
};

enum class GameState : byte { 
    None = 0,
    ShowObjective = 1,
    RetreiveObjective = 2,
    GameOver = 3,
    Win = 4
};

struct ProgressBar {
    private:
        Adafruit_NeoPixel strip;
        uint16_t currentProgress = 0;
        uint16_t maxProgress = 10;
        const Color DONE_PROGRESS_COLOR = Color(0,.15,0);
        const Color PROGRESS_COLOR = Color(.15,.15,.15);
        const Color FAIL_COLOR = Color(.15,0,0);
        Color currentProgressColor = PROGRESS_COLOR;
        ProgressBarState state = ProgressBarState::None;

    public:
        ProgressBar(uint16_t ledAmount, int16_t ledInputPin = 6, neoPixelType ledType = NEO_GRB + NEO_KHZ800) : strip(ledAmount, ledInputPin, ledType) {
        }

        void resetStateAndProgress(uint16_t maxProgress) {
            this->currentProgress = 0;
            this->maxProgress = maxProgress;
            setState(ProgressBarState::None);
        }

        ProgressBarState getState() const {
            return this->state;
        }

        void setState(ProgressBarState state) {
            this->state = state;
            render();
        }

        void setProgressWithAnim(uint16_t progress = 1){
            currentProgress = min(progress, maxProgress);
            currentProgressColor = DONE_PROGRESS_COLOR;
            changeStepTime = millis() + CHANGE_STEP_TIME_FREQUENCY;
            currentProgressionOn = true;
        }

        const uint16_t getLedAmount() const {
            return this->strip.numPixels();
        }

        void initialize() {
            this->strip.begin();
            this->strip.setBrightness(50);
            this->strip.show();
        }

        const unsigned long CHANGE_STEP_TIME_FREQUENCY = 1000;
        unsigned long changeStepTime = 0;
        bool currentProgressionOn = false;

        void doAction(){
            if (state == ProgressBarState::Animated){
                unsigned long currentMillis = millis();

                if (currentMillis >= changeStepTime){
                    changeStepTime += CHANGE_STEP_TIME_FREQUENCY;
                    currentProgressionOn = !currentProgressionOn;
                    render();
                }

                if (currentProgressColor != PROGRESS_COLOR){
                    currentProgressColor = Color::moveTowards(currentProgressColor, PROGRESS_COLOR, 0.0025);

                    if (currentProgressionOn)
                        render();
                }
            }
        }

    private:
        void render() {
            int ledPerProgress = max(1, map(1, 0, maxProgress, 0, getLedAmount()));
            int doneProgress = map(currentProgress, 0, maxProgress, 0, getLedAmount());

            for (uint16_t i = 0; i < doneProgress; ++i){
                strip.setPixelColor(i, DONE_PROGRESS_COLOR.toGRB());
            }

            for (uint16_t i = doneProgress; i < doneProgress + ledPerProgress; ++i){
                if (state == ProgressBarState::Fail)
                    strip.setPixelColor(i, FAIL_COLOR.toGRB());
                else
                    strip.setPixelColor(i, currentProgressionOn && state == ProgressBarState::Animated ? currentProgressColor.toGRB() : 0);
            }

            for (uint16_t i = doneProgress + ledPerProgress; i < getLedAmount(); ++i){
                strip.setPixelColor(i, 0);
            }

            this->strip.show();
        }
};

struct Vector2 {
    public:
        char x;
        char y;

        const bool isZero() const {
            return x == 0 && y == 0; 
        }

        inline bool operator==(const Vector2& rhs) { return x == rhs.x && y == rhs.y; }
        inline bool operator!=(const Vector2& rhs) { return x != rhs.x || y != rhs.y; }
};

struct Map {
    private:
        bool _map[MAP_SIZE][MAP_SIZE];

    public:
        Array<Vector2, MAP_SIZE * MAP_SIZE> getOffPositions() const {
            Array<Vector2, MAP_SIZE * MAP_SIZE> offPositions;

            for (char x = 0; x < MAP_SIZE; ++x){
                for (char y = 0; y < MAP_SIZE; ++y){
                    if (!this->getAt({x, y})){
                        offPositions.push_back({x, y});
                    }
                }
            }

            return offPositions;
        }

        byte getOnAmount(){
            byte amount = 0;

            for (char x = 0; x < MAP_SIZE; ++x){
                for (char y = 0; y < MAP_SIZE; ++y){
                    if (this->getAt({x, y}))
                        ++amount;
                }
            }

            return amount;
        }

        bool getAt(Vector2 position) const {
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

        inline bool operator==(const Map& rhs) { 
            for (char x = 0; x < MAP_SIZE; ++x){
                for (char y = 0; y < MAP_SIZE; ++y){
                    if (this->getAt({x, y}) != rhs.getAt({x,y}))
                        return false;
                }
            }

            return true;
        }

        inline bool operator!=(const Map& rhs) { return !(*this == rhs); }
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

struct Game {
    private:
        GameState _state;
        Controller _controller;
        Map _objectiveMap;
        Map _currentMap;
        Cursor _cursor;
        Light _winLight;
        ProgressBar _progressBar;
        uint16_t _level = 1;

    public:
        Game() : _cursor(_currentMap, _controller), _winLight(WIN_LED_ID), _progressBar(10) {
            
        }

        void initialize(){
            _progressBar.initialize();
            setNewObjectiveMap();
        }

        void setNewObjectiveMap(){
            auto offPositions = _objectiveMap.getOffPositions();
            
            for (byte i = _objectiveMap.getOnAmount(); i < _level; ++i){
                long randomIndex = random(offPositions.size());
                _objectiveMap.setAt(offPositions[randomIndex], true);
                offPositions.remove(randomIndex);
            }
        }

        void doAction(){
            if (_state == GameState::None){
                _state = GameState::ShowObjective;
            } else if (_state == GameState::ShowObjective) {
                if (_controller.isButtonJustDown()) {
                    _state = GameState::RetreiveObjective;
                    _progressBar.setState(ProgressBarState::Animated);
                }
            } else if (_state == GameState::RetreiveObjective) {
                _cursor.doAction();

                if (_controller.isButtonJustDown()){
                    if (!_objectiveMap.getAt(_cursor.position)) {
                        _state = GameState::GameOver;
                        _progressBar.setState(ProgressBarState::Fail);
                    } else {
                        _currentMap.setAt(_cursor.position, true);
                        _progressBar.setProgressWithAnim(_currentMap.getOnAmount());
                        _winLight.switchToFor(true, 1000);

                        if (_currentMap == _objectiveMap)
                            _state = GameState::Win;
                    }
                }
            } else if (_state == GameState::GameOver) {
                if (_controller.isButtonJustDown()) {
                    _currentMap.clear();
                    _progressBar.resetStateAndProgress(_objectiveMap.getOnAmount());
                    _cursor.position = {0,0};
                    _state = GameState::None;
                }
            } else if (_state == GameState::Win) {
                if (_controller.isButtonJustDown()) {
                    _state = GameState::ShowObjective;
                    _currentMap.clear();
                    ++_level;
                    setNewObjectiveMap();
                    _progressBar.resetStateAndProgress(_objectiveMap.getOnAmount());
                }
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
            } else if (_state == GameState::Win){
                for (char x = 0; x < MAP_SIZE; ++x) {
                    for (char y = 0; y < MAP_SIZE; ++y) {
                        lc.setLed(0, y, x, _currentMap.getAt({x, y}));
                    }
                }

                // win feedback        
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
    game.initialize();
}

void loop() { 
    
    game.doAction();
    game.render();
    //sprintf(textBuffer, "_cursor x: %d, y: %d", _objectiveMap.getAt(0,0), 10);
    //Serial.println(textBuffer);
    //delay(1000);
}

