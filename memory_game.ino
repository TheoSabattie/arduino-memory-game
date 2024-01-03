#include <Array.h>

#include <Adafruit_NeoPixel.h>

//We always have to include the library
#include "LedControl.h"

#include "Color.h"
#include "Consts.h"
#include "Map.h"
#include "ProgressBar.h"
#include "Controller.h"
#include "Cursor.h"

enum class GameState : uint8_t { 
    None = 0,
    ShowObjective = 1,
    RetreiveObjective = 2,
    GameOver = 3,
    Win = 4
};

enum class ProgressionMode : uint8_t {
    Game = 0,
    Level = 1
};

struct ExplanationsManager {
    private:
        const int GAME_INDEX = 0;
        const int LEVEL_INDEX = 1;
        const int LOOK_AND_MEMORIZE_INDEX = 2;
        const int GAME_OVER_INDEX = 4;
        const int WIN_INDEX = 3;
        const int REPRODUCE_FIGURE_INDEX = 5;
        Adafruit_NeoPixel strip = {6, TUTO_IN, NEO_GRB + NEO_KHZ800};
        GameState gameState;
        ProgressionMode progressionMode;

        void render(){
            strip.setPixelColor(LOOK_AND_MEMORIZE_INDEX, (gameState == GameState::ShowObjective ? Color::white() : Color::black()).toGRB());
            strip.setPixelColor(REPRODUCE_FIGURE_INDEX, (gameState == GameState::RetreiveObjective ? Color::white() : Color::black()).toGRB());
            strip.setPixelColor(WIN_INDEX, (gameState == GameState::Win ? Color::green() : Color::black()).toGRB());
            strip.setPixelColor(GAME_OVER_INDEX, (gameState == GameState::GameOver ? Color::red() : Color::black()).toGRB());
            strip.setPixelColor(GAME_INDEX, (progressionMode == ProgressionMode::Game ? Color::white() : Color::black()).toGRB());
            strip.setPixelColor(LEVEL_INDEX, (progressionMode == ProgressionMode::Level ? Color::white() : Color::black()).toGRB());
            strip.show();
            // todo: Game vs Level (progression)
        }

    public:
        void initialize(){
            this->strip.begin();
            this->strip.setBrightness(5);
        }

        ProgressionMode getProgressionMode() const {
            return this->progressionMode;
        }

        GameState getGameState() const {
            return this->gameState;
        }

        void setProgressionMode(ProgressionMode progressionMode) {
            this->progressionMode = progressionMode;
            this->render();
        }

        void setGameState(GameState gameState){
            this->gameState = gameState;
            this->render();
        }
};

// parameters for reading the joystick:
const int responseDelay = 1000;   // response delay of the mouse, in ms

char textBuffer[40];

struct Game {
    private:
        Controller _controller;
        Map _objectiveMap;
        Map _currentMap;
        Cursor _cursor;
        ProgressBar _progressBar;
        ExplanationsManager _explanations;
        uint16_t _level = 1;
        LedControl lc;
    public:
        Game() : _cursor(_currentMap, _controller), _progressBar(10, PROGRESSION_IN), lc(SCREEN_IN_DIN, SCREEN_IN_CLK, SCREEN_IN_CS, 1) {
            
        }

        void initialize(){
            lc.shutdown(0,false);
            /* Set the brightness to a medium values */
            lc.setIntensity(0,0);
            lc.clearDisplay(0);
            
            _progressBar.initialize();
            _explanations.initialize();
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
            GameState state = _explanations.getGameState();

            if (state == GameState::None){ {
                _explanations.setGameState(GameState::ShowObjective);
                _progressBar.resetStateAndProgress(_objectiveMap.getOnAmount());
            }
            } else if (state == GameState::ShowObjective) {
                if (_controller.isButtonJustDown()) {
                    _explanations.setGameState(GameState::RetreiveObjective);
                    _progressBar.setState(ProgressBarState::AnimatedProgression);
                }
            } else if (state == GameState::RetreiveObjective) {
                _cursor.doAction();

                if (_controller.isButtonJustDown()){
                    if (!_objectiveMap.getAt(_cursor.getPosition())) {
                        _explanations.setGameState(GameState::GameOver);
                        _progressBar.setState(ProgressBarState::Fail);
                    } else {
                        _currentMap.setAt(_cursor.getPosition(), true);
                        _progressBar.setProgressWithAnim(_currentMap.getOnAmount());

                        if (_currentMap == _objectiveMap) {
                            _explanations.setGameState(GameState::Win);
                            _progressBar.setState(ProgressBarState::Win);
                        }
                    }
                }
            } else if (state == GameState::GameOver) {
                if (_controller.isButtonJustDown()) {
                    _currentMap.clear();
                    _progressBar.resetStateAndProgress(_objectiveMap.getOnAmount());
                    _explanations.setGameState(GameState::None);
                }
            } else if (state == GameState::Win) {
                if (_controller.isButtonJustDown()) {
                    _explanations.setGameState(GameState::ShowObjective);
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
            GameState state = _explanations.getGameState();

            if (state == GameState::RetreiveObjective) {
                for (char x = 0; x < MAP_SIZE; ++x) {
                    for (char y = 0; y < MAP_SIZE; ++y) {
                        if (!(x == _cursor.getPosition().x && y == _cursor.getPosition().y))
                            lc.setLed(0, y, x, _currentMap.getAt({x, y}));
                    }
                }

                lc.setLed(0, _cursor.getPosition().y, _cursor.getPosition().x, _cursor.getLedIsOn());
            } else if (state == GameState::ShowObjective) {
                for (char x = 0; x < MAP_SIZE; ++x) {
                    for (char y = 0; y < MAP_SIZE; ++y) {
                        lc.setLed(0, y, x, _objectiveMap.getAt({x, y}));
                    }
                }
            } else if (state == GameState::GameOver){
                for (char x = 0; x < MAP_SIZE; ++x) {
                    for (char y = 0; y < MAP_SIZE; ++y) {
                        lc.setLed(0, y, x, true);
                    }
                }
            } else if (state == GameState::Win){
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

