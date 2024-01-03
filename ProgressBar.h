#ifndef PROGRESSBAR
#define PROGRESSBAR

#include <Adafruit_NeoPixel.h>
#include "Color.h"

enum class ProgressBarState : byte { 
    None = 0,
    AnimatedProgression = 1,
    Fail = 2,
    Win,
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
        ProgressBar(uint16_t ledAmount, int16_t ledInputPin, neoPixelType ledType = NEO_GRB + NEO_KHZ800) : strip(ledAmount, ledInputPin, ledType) {
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
            this->strip.setBrightness(10);
            resetStateAndProgress(maxProgress);
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

#endif