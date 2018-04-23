//
// Created by al on 17.03.2018.
//

#ifndef SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H
#define SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H

#include <Arduino.h>
#include "Morse.h"

#define NUM_INPUTS 16
#define NUM_OUTPUTS 8
#define NUM_MAPPINGS 16
#define DEFAULT_INPT_STAT_CONF_THRS 100
#define DEFAULT_OUT_MAX_DURATION (1000UL * 5) // 5s
#define DEFAULT_MAP_ACTIV_THRS 50 // 50ms

class Input {

private:
    boolean isActive = false;
    unsigned long activationId = 0;
    unsigned long activatedAtMs = 0;
    unsigned long activeDurationMs = 0;
    byte activeConfidence = 0;
    byte inactiveConfidence = 0;
    byte pin;

    void debounce(byte val) {
        if (val == HIGH) {
            if (inactiveConfidence <= DEFAULT_INPT_STAT_CONF_THRS) {
                inactiveConfidence++;
            }
            activeConfidence = 0;
        } else {
            if (activeConfidence <= DEFAULT_INPT_STAT_CONF_THRS) {
                activeConfidence++;
            }
            inactiveConfidence = 0;
        }

        isActive = activeConfidence > DEFAULT_INPT_STAT_CONF_THRS;
    }

public:
    boolean getIsActive() const {
        return isActive;
    }

    void setPin(byte pin) {
        this->pin = pin;
    }

    byte getPin() {
        return this->pin;
    }

    void activate(unsigned long now) {
        activatedAtMs = now;
        isActive = true;
    }


    void update(byte val, unsigned long now) {
        debounce(val);

        if (!isActive) {
            activatedAtMs = 0;
        } else {
            if (activatedAtMs == 0) {
                activatedAtMs = now;
                activeDurationMs = 0;
            } else {
                activeDurationMs = now - activatedAtMs;
            }
        }
    }
};

enum OutputStatus {
    OFF, OUT1, OUT2
};

typedef struct {
    OutputStatus status = OFF;
    unsigned long activatedAtMs = 0;
    unsigned long activeDurationMs = 0;
    unsigned long maxActivationDurationMs;
    unsigned long activationId1 = 0;
    unsigned long activationId2 = 0;
    byte pin1;
    byte pin2;
} Output, *pOutput;

typedef struct {
    Input **inputs;
    byte numInputs;
    Output **outputs;
    byte numOutputs;
    OutputStatus statusToActivate;
    unsigned long activatedAtMs = 0;
    unsigned long activeDurationMs = 0;
    unsigned long activationThresholdMs;
} InputToOutputMapping, *pInputToOutputMapping;

struct {
    Input inputs[NUM_INPUTS];
    Output outputs[NUM_OUTPUTS];
    InputToOutputMapping mappings[NUM_MAPPINGS];
} config;

class ShutterBuilder {
    byte closeInputId, closeOutputId;
    byte openInputId, openOutputId;
    byte mapNum;

    void addMapping(byte num, Input *input, Output *output, OutputStatus targetStatus) {
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
        mapping->activationThresholdMs = DEFAULT_MAP_ACTIV_THRS;
    }

    Output *addOutput(byte num, byte o1, byte o2) {
        Output *output = &config.outputs[num];
        output->pin1 = o1;
        output->pin2 = o2;
        pinMode(output->pin1, OUTPUT);
        pinMode(output->pin2, OUTPUT);
        output->maxActivationDurationMs = DEFAULT_OUT_MAX_DURATION;
        return output;
    }

    Input *addInput(byte num, byte pin) {
        Input *input = &config.inputs[num];
        input->setPin(pin);
        pinMode(input->getPin(), INPUT_PULLUP);
        return input;
    }

public:
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

        addMapping(mapNum, input1, output, OUT1);
        addMapping(mapNum + 1, input2, output, OUT2);
    }
};

ShutterBuilder &mapping(byte i) {
    return *(new ShutterBuilder(i));
}

#endif //SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H
