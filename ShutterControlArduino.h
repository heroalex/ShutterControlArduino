//
// Created by al on 17.03.2018.
//

#ifndef SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H
#define SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H

#define NUM_INPUTS 16
#define NUM_OUTPUTS 8
#define NUM_MAPPINGS 16
#define DEFAULT_INPT_STAT_CONF_THRS 100
#define DEFAULT_OUT_PIN0 22
#define DEFAULT_OUT_MAX_DURATION (1000UL * 5) // 5s
#define DEFAULT_MAP_ACTIV_THRS 50 // 50ms
#define DIT 200

typedef struct {
    boolean isActive = false;
    unsigned long activatedAtMs = 0;
    unsigned long activeDurationMs = 0;
    byte activeConfidence = 0;
    byte inactiveConfidence = 0;
    byte pin;
} Input;

enum OutputStatus {
    OFF, PIN_1, PIN_2
};

typedef struct {
    OutputStatus status = OFF;
    unsigned long activatedAtMs = 0;
    unsigned long activeDurationMs = 0;
    unsigned long maxActivationDurationMs;
    byte pin1;
    byte pin2;
} Output;

typedef struct {
    Input **inputs;
    byte numInputs;
    Output **outputs;
    byte numOutputs;
    OutputStatus statusToActivate;
    unsigned long activatedAtMs = 0;
    unsigned long activeDurationMs = 0;
    unsigned long activationThresholdMs;
} InputToOutputMapping;

struct {
    Input inputs[NUM_INPUTS];
    Output outputs[NUM_OUTPUTS];
    InputToOutputMapping mappings[NUM_MAPPINGS];
    byte inputStateConfidenceThreshold = DEFAULT_INPT_STAT_CONF_THRS;
} config;
#endif //SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H
