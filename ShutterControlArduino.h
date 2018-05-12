//
// Created by al on 17.03.2018.
//

#ifndef SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H
#define SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H

#include <Arduino.h>
#include <limits.h>
#include "Morse.h"

#define NUM_INPUTS 16
#define NUM_OUTPUTS 8
#define NUM_MAPPINGS 18
#define DEFAULT_INPT_STAT_CONF_THRS 1000
#define DEFAULT_INPT_INACT_CONF_THRS 15
#define DEFAULT_OUT_MAX_DURATION (1000UL * 5) // 60s
#define DEFAULT_OUT_MIN_STOPPING_DURATION 500 // 100ms
#define DEFAULT_MAP_ACTIV_THRS 100 // 200ms
#define OUTPUT_ON LOW
#define OUTPUT_OFF HIGH
#define INPUT_OFF LOW

//#define USE_LCD1
//#define USE_LCD2
#define USE_LCD3
//#define FLOOR_OG

class Input {

private:
    boolean isActive = false;
    unsigned long activationId = ULONG_MAX;

private:
    unsigned long activatedAtMs = ULONG_MAX;
    int activeConfidence = 0;
    int inactiveConfidence = 0;
    byte pin;

    boolean debounce(byte val) {
        if (val == INPUT_OFF) {
            if (inactiveConfidence <= DEFAULT_INPT_INACT_CONF_THRS) {
                inactiveConfidence++;
            } else {
              activeConfidence = 0;
            }
        } else {
            if (activeConfidence <= DEFAULT_INPT_STAT_CONF_THRS) {
                activeConfidence++;
            } else {
              inactiveConfidence = 0;
            }
        }

        return activeConfidence >= DEFAULT_INPT_STAT_CONF_THRS;
    }

public:
    unsigned long getActivationId() const {
        return activationId;
    }
    
    boolean getIsActive() const {
        return isActive;
    }

    void setPin(byte pin) {
        this->pin = pin;
    }

    byte getPin() const {
        return this->pin;
    }

    unsigned long getActiveDurationMs(unsigned long now) {
        return isActive ? now - activatedAtMs : 0;
    }

    void update(byte val, unsigned long now) {
        boolean activate = debounce(val);

        if (!activate && isActive) {
            activatedAtMs = ULONG_MAX;
            isActive = false;
            activationId = now;
        } else if (activate && !isActive) {
            activatedAtMs = now;
            isActive = true;
        }
    }
};

enum OutputStatus {
    IDLE, OPENING, CLOSING, STOPPING
};

typedef struct {
    OutputStatus status = IDLE;
    unsigned long activatedAtMs = ULONG_MAX;
    unsigned long activeDurationMs = 0;
    unsigned long maxActivationDurationMs = DEFAULT_OUT_MAX_DURATION;
    unsigned long activationId = 0;
    byte closePin;
    byte openPin;
} Output;

typedef struct {
    Input **inputs;
    byte numInputs;
    Output **outputs;
    byte numOutputs;
    OutputStatus statusToActivate;
    unsigned long activationThresholdMs = DEFAULT_MAP_ACTIV_THRS;
} InputToOutputMapping;

struct {
    Input inputs[NUM_INPUTS];
    Output outputs[NUM_OUTPUTS];
    InputToOutputMapping mappings[NUM_MAPPINGS];
} config;

class ShutterBuilder {
    byte closeInputPin{}, closeOutputPin{};
    byte openInputPin{}, openOutputPin{};
    byte mapNum;

public:
    void addMapping(byte num, Input *input, Output *output, OutputStatus targetStatus) const {
        InputToOutputMapping *mapping = &config.mappings[num];
        mapping->inputs = static_cast<Input **>(malloc(sizeof(Input *)));
        if (mapping->inputs == NULL) {
            morseError('I');
        }
        mapping->inputs[0] = input;
        mapping->numInputs = 1;
        mapping->outputs = static_cast<Output **>(malloc(sizeof(Output *)));
        if (mapping->outputs == NULL) {
            morseError('O');
        }
        mapping->outputs[0] = output;
        mapping->numOutputs = 1;

        mapping->statusToActivate = targetStatus;
    }

    Output *addOutput(byte num, byte closePin, byte openPin) const {
        Output *output = &config.outputs[num];
        output->closePin = closePin;
        output->openPin = openPin;
        pinMode(output->closePin, OUTPUT);
        pinMode(output->openPin, OUTPUT);
        digitalWrite(output->closePin, OUTPUT_OFF);
        digitalWrite(output->openPin, OUTPUT_OFF);
        return output;
    }

    Input *addInput(byte num, byte pin) const {
        Input *input = &config.inputs[num];
        input->setPin(pin);
        pinMode(input->getPin(), (INPUT_OFF == HIGH) ? INPUT_PULLUP : INPUT);
        return input;
    }

    explicit ShutterBuilder(byte num) {
        this->mapNum = num;
    }

    ShutterBuilder &close(byte in, byte out) {
        closeInputPin = in;
        closeOutputPin = out;
        return *this;
    }

    ShutterBuilder &open(byte in, byte out) {
        openInputPin = in;
        openOutputPin = out;
        return *this;
    }

    void end() {
        Input *input1 = addInput(mapNum * 2, closeInputPin);
        Input *input2 = addInput(mapNum * 2 + 1, openInputPin);
        Output *output = addOutput(mapNum, closeOutputPin, openOutputPin);

        addMapping(mapNum * 2, input1, output, OPENING);
        addMapping(mapNum * 2 + 1, input2, output, CLOSING);
    }
};

ShutterBuilder &mapping(byte i) {
    return *(new ShutterBuilder(i));
}

#endif //SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H
