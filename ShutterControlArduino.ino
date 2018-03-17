#include <Arduino.h>
#include <LiquidCrystal.h>
#include "ShutterControlArduino.h"

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

byte ledState = LOW;
byte oldButtonState = LOW;

int loopCnt = 0;

void dit(byte d) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(DIT);
    digitalWrite(LED_BUILTIN, LOW);
    delay(d * DIT);
}

void dah(byte d) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(3 * DIT);
    digitalWrite(LED_BUILTIN, LOW);
    delay(d * DIT);
}

void morseError(char code) {
    for (;;) {
        switch (code) {
            case 'I': {
                dit(1);
                dit(7);
            }
                break;
            default: {
                // SOS
                dit(1);
                dit(1);
                dit(3);
                dah(1);
                dah(1);
                dah(3);
                dit(1);
                dit(1);
                dit(7);
            }
        }
    }
}

void debounce(Input *button, byte val) {
    if (val == HIGH) {
        if (button->inactiveConfidence <= config.inputStateConfidenceThreshold) {
            button->inactiveConfidence++;
        }
        button->activeConfidence = 0;
    } else {
        if (button->activeConfidence <= config.inputStateConfidenceThreshold) {
            button->activeConfidence++;
        }
        button->inactiveConfidence = 0;
    }

    button->isActive = button->activeConfidence > config.inputStateConfidenceThreshold;
}

void updateInputs(unsigned long now) {
    for (byte i = 0; i < NUM_INPUTS; i++) {
        Input *input = &config.inputs[i];
        byte val = digitalRead(input->pin);
        debounce(input, val);
        if (!input->isActive) {
            input->activatedAtMs = 0;
        } else {
            if (input->activatedAtMs == 0) {
                input->activatedAtMs = now;
                input->activeDurationMs = 0;
            } else {
                input->activeDurationMs = now - input->activatedAtMs;
            }
        }
    }
}

void updateMappings(unsigned long now) {
    for (byte i = 0; i < NUM_MAPPINGS; i++) {
        InputToOutputMapping *mapping = &config.mappings[i];
        Input **inputs = mapping->inputs;
        byte numInputs = mapping->numInputs;
        boolean isActive = false;
        for (byte j = 0; j < numInputs; j++) {
            Input *input = inputs[j];
            if (input->isActive && input->activeDurationMs > mapping->activationThresholdMs) {
                isActive = true;
                break;
            }
        }
        if (isActive) {
            if (mapping->activatedAtMs == 0) {
                mapping->activatedAtMs = now;
                mapping->activeDurationMs = 0;
            } else {
                mapping->activeDurationMs = now - mapping->activatedAtMs;
                Output **outputs = mapping->outputs;
                byte numOutputs = mapping->numOutputs;
                for (byte k = 0; k < numOutputs; k++) {
                    Output *output = outputs[k];
//                    output->
                }
            }
        } else {
            mapping->activatedAtMs = 0;
            mapping->activeDurationMs = 0;
        }
    }

    /*if( !config.mappings[8].inputs[0]->isActive ) {
        config.mappings[0].outputs[0]->status = OFF;
        } else if (config.mappings[0].outputs[0]->status == OFF) {
        ledState = !ledState;
        config.mappings[0].outputs[0]->status = PIN_1;
        digitalWrite(config.mappings[0].outputs[0]->pin1, ledState);
    }*/
}

void updateOutputs(unsigned long now) {

}

void printInfos(unsigned long now) {
    if (now % 100UL == 0) {
        lcd.setCursor(0, 1);
        lcd.print(F("                "));
        lcd.setCursor(0, 1);
        for (byte i = 8; i < 10; i++) {
            Input *button = &config.inputs[i];
            lcd.print(button->pin);
            lcd.print(':');
            lcd.print(button->activeDurationMs);
            lcd.print(' ');
        }
    }
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    byte i, j, k;
    for (i = 0; i < NUM_INPUTS; i++) {
        Input *button = &config.inputs[i];
        button->pin = A0 + i;
        pinMode(button->pin, INPUT_PULLUP);
    }

    for (i = 0; i < NUM_OUTPUTS; i++) {
        Output *output = &config.outputs[i];
        output->pin1 = DEFAULT_OUT_PIN0 + i * 2;
        output->pin2 = output->pin1 + 1;
        pinMode(output->pin1, OUTPUT);
        pinMode(output->pin2, OUTPUT);
        output->maxActivationDurationMs = DEFAULT_OUT_MAX_DURATION;
    }

    i = 0;
    j = 0;
    k = 0;
    do {
        InputToOutputMapping *mapping = &config.mappings[i];
        mapping->inputs = static_cast<Input **>(malloc(sizeof(Input *)));
        if (mapping->inputs == NULL) {
            morseError('I');
        }
        mapping->inputs[0] = &config.inputs[j];
        mapping->numInputs = 1;
        mapping->outputs = static_cast<Output **>(malloc(sizeof(Output *)));
        if (mapping->outputs == NULL) {
            morseError('O');
        }
        mapping->outputs[0] = &config.outputs[k];
        mapping->numOutputs = 1;

        mapping->statusToActivate = i % 2 == 0 ? PIN_1 : PIN_2;
        mapping->activationThresholdMs = DEFAULT_MAP_ACTIV_THRS;

        k += i % 2;
        i++;
        j++;
    } while (i < NUM_MAPPINGS && j < NUM_INPUTS && k < NUM_OUTPUTS);

    lcd.begin(16, 2);
    lcd.print(NUM_INPUTS);
    lcd.print(F(" Buttons"));
}

void loop() {
    unsigned long now = millis();
    loopCnt++;
    if (now % 1000UL == 0) {
        lcd.setCursor(10, 0);
        lcd.print(F("     "));
        lcd.setCursor(11, 0);
        lcd.print(loopCnt);
        loopCnt = 0;
    }

    updateInputs(now);

    updateMappings(now);

    updateOutputs(now);

    printInfos(now);
}
