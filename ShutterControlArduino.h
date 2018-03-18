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
#define DEFAULT_OUT_PIN0 22
#define DEFAULT_OUT_MAX_DURATION (1000UL * 5) // 5s
#define DEFAULT_MAP_ACTIV_THRS 50 // 50ms

typedef struct {
    boolean isActive = false;
    unsigned long activatedAtMs = 0;
    unsigned long activeDurationMs = 0;
    byte activeConfidence = 0;
    byte inactiveConfidence = 0;
    byte pin;
} Input, *pInput;

enum OutputStatus {
    OFF, OUT1, OUT2
};

typedef struct {
    OutputStatus status = OFF;
    unsigned long activatedAtMs = 0;
    unsigned long activeDurationMs = 0;
    unsigned long maxActivationDurationMs;
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
    byte inputStateConfidenceThreshold = DEFAULT_INPT_STAT_CONF_THRS;
} config;

class MappingBuilder {
    byte zuIn, zuOut;
    byte aufIn, aufOut;
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
        Input *button = &config.inputs[num];
        button->pin = pin;
        pinMode(button->pin, INPUT_PULLUP);
        return button;
    }

public:
    MappingBuilder(byte num) {
        this->mapNum = num;
    }

    MappingBuilder &zu(byte in, byte out) {
        zuIn = in;
        zuOut = out;
        return *this;
    }

    MappingBuilder &auf(byte in, byte out) {
        aufIn = in;
        aufOut = out;
        return *this;
    }

    void end() {
        Input *input1 = addInput(mapNum * 2, zuIn);
        Input *input2 = addInput(mapNum * 2 + 1, aufIn);
        Output *output = addOutput(mapNum, zuOut, aufOut);

        addMapping(mapNum, input1, output, OUT1);
        addMapping(mapNum + 1, input2, output, OUT2);
    }
};

MappingBuilder &mapping(byte i) {
    return *(new MappingBuilder(i));
}
#endif //SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H
