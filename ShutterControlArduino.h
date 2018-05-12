//
// Created by al on 17.03.2018.
//

#ifndef SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H
#define SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H

#include <Arduino.h>
#include "Morse.h"

#define NUM_INPUTS 16
#define NUM_OUTPUTS 8
#define NUM_MAPPINGS 16 //18
#define DEFAULT_INPT_STAT_CONF_THRS 250
#define DEFAULT_INPT_INACT_CONF_THRS 10
#define DEFAULT_OUT_MAX_DURATION (1000UL * 60) // 60s
#define DEFAULT_OUT_MIN_STOPPING_DURATION 100 // 100ms
#define DEFAULT_MAP_ACTIV_THRS 200 // 200ms
#define OUTPUT_ON LOW
#define OUTPUT_OFF HIGH

//#define USE_LCD

class Input {

private:
    boolean isActive = false;
    unsigned long activationId = 0;

private:
    unsigned long activatedAtMs = 0;
    byte activeConfidence = 0;
    byte inactiveConfidence = 0;
    byte pin;

    boolean debounce(byte val) {
        if (val == LOW) {
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
    
    byte getActiveConf() {
      return activeConfidence;
    }
    
    byte getInactiveConf() {
      return inactiveConfidence;
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
            activatedAtMs = 0;
            isActive = false;
            activationId++;
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
    unsigned long activatedAtMs = 0;
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
    byte closeInputId, closeOutputId;
    byte openInputId, openOutputId;
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
        pinMode(input->getPin(), INPUT);
        return input;
    }

    ShutterBuilder(byte num) {
        this->mapNum = num;
    }

    ShutterBuilder &close(byte in, byte out) {
        closeInputId = in;
        closeOutputId = out;
        return *this;
    }

    ShutterBuilder &open(byte in, byte out) {
        openInputId = in;
        openOutputId = out;
        return *this;
    }

    void end() {
        Input *input1 = addInput(mapNum * 2, closeInputId);
        Input *input2 = addInput(mapNum * 2 + 1, openInputId);
        Output *output = addOutput(mapNum, closeOutputId, openOutputId);

        addMapping(mapNum * 2, input1, output, OPENING);
        addMapping(mapNum * 2 + 1, input2, output, CLOSING);
    }
};

ShutterBuilder &mapping(byte i) {
    return *(new ShutterBuilder(i));
}

#endif //SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H
