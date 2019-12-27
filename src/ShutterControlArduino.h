//
// Created by al on 17.03.2018.
//

#ifndef SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H
#define SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H

#include <Arduino.h>
#include <limits.h>

#define NUM_INPUTS 16
#define NUM_OUTPUTS 8
#define NUM_MAPPINGS 16
#define OUTPUT_ON LOW
#define OUTPUT_OFF HIGH
#define INPUT_OFF LOW

#define INPUTS_CHECK_MSEC 5 // Read hardware every 5 msec

struct {
    uint16_t inputActivationThreshold = 10; // confidence count for activation, time for reaching this limit is ~ INPUTS_CHECK_MSEC * this
    uint16_t inputDeactivationThreshold = 2; // confidence count for deactivation
    uint16_t outputPowerMaxDuration = (1000UL * 60); // 60s
    uint16_t outputDirectionSwitchingMinDuration = 1500; // 1500ms
} config;

class Input {

private:
    bool isActive = false;
    bool changed = false;
    unsigned long activatedAtMs = ULONG_MAX;
    uint16_t activeConfidence = 0;
    uint16_t inactiveConfidence = 0;
    byte pin;
    bool rawValue = LOW;
    bool debouncedValue = LOW;

    void debounce() {
        if (rawValue == INPUT_OFF) {
            if (inactiveConfidence <= config.inputDeactivationThreshold) {
                inactiveConfidence++;
            } else {
              activeConfidence = 0;
            }
        } else {
            if (activeConfidence <= config.inputActivationThreshold) {
                activeConfidence++;
            } else {
              inactiveConfidence = 0;
            }
        }

        debouncedValue = activeConfidence >= config.inputActivationThreshold;
    }

public:

    boolean active() const {
        return isActive;
    }

    boolean hasChanged() const {
        return changed;
    }

    boolean getRawValue() const {
        return rawValue;
    }

    boolean getDebouncedValue() const {
        return debouncedValue;
    }

    void setPin(byte pin) {
        this->pin = pin;
    }

    byte getPin() const {
        return pin;
    }

    unsigned long activeDurationMs(unsigned long now) const {
        return isActive ? now - activatedAtMs : ULONG_MAX;
    }

    void update(unsigned long now) {
        pinMode(getPin(), OUTPUT);
        digitalWrite(getPin(), LOW);
        __asm__("nop\n\t");
        pinMode(getPin(), INPUT);
        __asm__("nop\n\t");
        rawValue = digitalRead(getPin());
        debounce();

        if (debouncedValue == LOW && isActive) {
            activatedAtMs = 0;
            isActive = false;
            changed = true;
        } else if (debouncedValue == HIGH && !isActive) {
            activatedAtMs = now;
            isActive = true;
            changed = true;
        } else {
            changed = false;
        }
    }
};

enum OutputStatus {
    IDLE, OPENING, CLOSING, STOPPING
};

typedef struct {
    OutputStatus status = IDLE;
    unsigned long activatedAtMs = ULONG_MAX;
    unsigned long maxActivationDurationMs = config.outputPowerMaxDuration;
    bool isSwitching = false;
    byte powerPin;
    byte dirPin;
} Output;

typedef struct {
    Input *input;
    Output *output;
    OutputStatus statusToActivate;
} InputToOutputMapping;

Input inputs[NUM_INPUTS];
Output outputs[NUM_OUTPUTS];
InputToOutputMapping mappings[NUM_MAPPINGS];

class ShutterBuilder {
    byte closeInputPin{}, powerOutputPin{};
    byte openInputPin{}, dirOutputPin{};
    byte mapNum;

public:
    void addMapping(byte num, Input *input, Output *output, OutputStatus targetStatus) const {
        mappings[num].input = input;
        mappings[num].output = output;
        mappings[num].statusToActivate = targetStatus;
    }

    Output *addOutput(byte num, byte powerPin, byte dirPin) const {
        Output* output = &outputs[num];
        output->powerPin = powerPin;
        output->dirPin = dirPin;
        pinMode(output->powerPin, OUTPUT);
        pinMode(output->dirPin, OUTPUT);
        digitalWrite(output->powerPin, OUTPUT_OFF);
        digitalWrite(output->dirPin, OUTPUT_OFF);
        return output;
    }

    Input *addInput(byte num, byte pin) const {
        Input* input = &inputs[num];
        input->setPin(pin);
        pinMode(input->getPin(), (INPUT_OFF == HIGH) ? INPUT_PULLUP : INPUT);
        return input;
    }

    explicit ShutterBuilder(byte num) {
        this->mapNum = num;
    }

    ShutterBuilder &closeIn(byte in) {
        closeInputPin = in;
        return *this;
    }

    ShutterBuilder &openIn(byte in) {
        openInputPin = in;
        return *this;
    }

    ShutterBuilder &powerOut(byte out) {
        powerOutputPin = out;
        return *this;
    }

    ShutterBuilder &dirOut(byte out) {
        dirOutputPin = out;
        return *this;
    }

    void end() {
        Input *closeInput = addInput(mapNum * 2, closeInputPin);
        Input *openInput = addInput(mapNum * 2 + 1, openInputPin);
        Output *output = addOutput(mapNum, powerOutputPin, dirOutputPin);

        addMapping(mapNum * 2, closeInput, output, CLOSING);
        addMapping(mapNum * 2 + 1, openInput, output, OPENING);
    }
};

ShutterBuilder &mapping(byte i) {
    return *(new ShutterBuilder(i));
}

#endif //SHUTTERCONTROLARDUINO_SHUTTERCONTROLARDUINO_H
