#include <Array.h>

#include <Adafruit_NeoPixel.h>

//We always have to include the library
#include "LedControl.h"

#define MAP_SIZE 8

const int SCREEN_IN_DIN = 12;
const int SCREEN_IN_CS = 11;
const int SCREEN_IN_CLK = 10;

const int TUTO_IN = 6;
const int PROGRESSION_IN = 5;

const int BUTTON_IN = 7;
const int X_OUT = A0;
const int Y_OUT = A1;

LedControl lc = LedControl(SCREEN_IN_DIN, SCREEN_IN_CLK, SCREEN_IN_CS, 1);

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
    AnimatedProgression = 1,
    Fail = 2,
    Win,
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
        ProgressBar(uint16_t ledAmount, int16_t ledInputPin = PROGRESSION_IN, neoPixelType ledType = NEO_GRB + NEO_KHZ800) : strip(ledAmount, ledInputPin, ledType) {
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
            changeStepTime = millis() + CHANGE_STEP_TIME_FREQUENCY;

            if (state == ProgressBarState::AnimatedProgression)
                currentProgressionOn = false;
            else if (state == ProgressBarState::Win)
                currentProgressionOn = true;

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
            if (state == ProgressBarState::AnimatedProgression){
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
            } else if (state == ProgressBarState::Win){
                unsigned long currentMillis = millis();

                if (currentMillis >= changeStepTime){
                    changeStepTime += CHANGE_STEP_TIME_FREQUENCY;
                    currentProgressionOn = !currentProgressionOn;
                    render();
                }
            }
        }

    private:
        void render() {
            int ledPerProgress = max(1, map(1, 0, maxProgress, 0, getLedAmount()));
            auto ledAmount = getLedAmount();
            int doneProgress = 0;
            
            if (state != ProgressBarState::None){
                if (state == ProgressBarState::Win){
                    doneProgress = ledAmount;
                } else {
                    doneProgress = map(currentProgress, 0, maxProgress, 0, ledAmount);
                    
                    if (currentProgress + 1 == maxProgress)
                        ledPerProgress = ledAmount - doneProgress; 
                }
            }

            for (uint16_t i = 0; i < doneProgress; ++i){
                if (state == ProgressBarState::Win){
                    strip.setPixelColor(i, currentProgressionOn ? DONE_PROGRESS_COLOR.toGRB() : 0);
                } else {
                    strip.setPixelColor(i, DONE_PROGRESS_COLOR.toGRB());
                }
            }

            for (uint16_t i = doneProgress; i < min(ledAmount, doneProgress + ledPerProgress); ++i){
                if (state == ProgressBarState::Fail)
                    strip.setPixelColor(i, FAIL_COLOR.toGRB());
                else
                    strip.setPixelColor(i, currentProgressionOn && state == ProgressBarState::AnimatedProgression ? currentProgressColor.toGRB() : 0);
            }

            for (uint16_t i = doneProgress + ledPerProgress; i < ledAmount; ++i){
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

struct Controller {
    private: 
        char readAxis(byte thisAxis) {
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

        bool getLedIsOn(){
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
        ProgressBar _progressBar;
        uint16_t _level = 1;

    public:
        Game() : _cursor(_currentMap, _controller), _progressBar(PROGRESSION_IN) {
            
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
            if (_state == GameState::None){ {
                _state = GameState::ShowObjective;
                _progressBar.resetStateAndProgress(_objectiveMap.getOnAmount());
            }
            } else if (_state == GameState::ShowObjective) {
                if (_controller.isButtonJustDown()) {
                    _state = GameState::RetreiveObjective;
                    _progressBar.setState(ProgressBarState::AnimatedProgression);
                }
            } else if (_state == GameState::RetreiveObjective) {
                _cursor.doAction();

                if (_controller.isButtonJustDown()){
                    if (!_objectiveMap.getAt(_cursor.getPosition())) {
                        _state = GameState::GameOver;
                        _progressBar.setState(ProgressBarState::Fail);
                    } else {
                        _currentMap.setAt(_cursor.getPosition(), true);
                        _progressBar.setProgressWithAnim(_currentMap.getOnAmount());

                        if (_currentMap == _objectiveMap) {
                            _state = GameState::Win;
                            _progressBar.setState(ProgressBarState::Win);
                        }
                    }
                }
            } else if (_state == GameState::GameOver) {
                if (_controller.isButtonJustDown()) {
                    _currentMap.clear();
                    _progressBar.resetStateAndProgress(_objectiveMap.getOnAmount());
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
            _progressBar.doAction();
        }

        void render(){
            if (_state == GameState::RetreiveObjective) {
                for (char x = 0; x < MAP_SIZE; ++x) {
                    for (char y = 0; y < MAP_SIZE; ++y) {
                        if (!(x == _cursor.getPosition().x && y == _cursor.getPosition().y))
                            lc.setLed(0, y, x, _currentMap.getAt({x, y}));
                    }
                }

                lc.setLed(0, _cursor.getPosition().y, _cursor.getPosition().x, _cursor.getLedIsOn());
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
    pinMode(BUTTON_IN, INPUT);
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

