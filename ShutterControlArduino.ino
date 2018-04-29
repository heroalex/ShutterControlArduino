#include <Arduino.h>
#include <LiquidCrystal.h>
#include "ShutterControlArduino.h"
#include "Morse/Morse.h"

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int loopCnt = 0;

void initEG();

void updateInputs(unsigned long now) {
    for (byte i = 0; i < NUM_INPUTS; i++) {
        Input *input = &config.inputs[i];
        byte val = digitalRead(input->getPin());
        input->update(val, now);
    }
}

void updateMappings(unsigned long now) {
    for (byte i = 0; i < NUM_MAPPINGS; i++) {
        InputToOutputMapping *mapping = &config.mappings[i];
        Input **inputs = mapping->inputs;
        byte numInputs = mapping->numInputs;
        for (byte j = 0; j < numInputs; j++) {
            Input *input = inputs[j];
            if (input->getIsActive() && input->getActiveDurationMs(now) > mapping->activationThresholdMs) {
                Output **outputs = mapping->outputs;
                byte numOutputs = mapping->numOutputs;
                for (byte k = 0; k < numOutputs; k++) {
                    Output *output = outputs[k];
                    if ((output->activationId != input->getActivationId() || output->activationId == 0) &&
                        output->status != mapping->statusToActivate) {
                        output->status = mapping->statusToActivate;
                        output->activationId = input->getActivationId();
                        output->activatedAtMs = now;
                    } else if (output->activationId == input->getActivationId() &&
                               output->status == mapping->statusToActivate) {
                        output->status = OutputStatus::OFF;
                        output->activatedAtMs = 0;
                    }
                }
                break;
            }
        }
    }
}

void updateOutputs(unsigned long now) {
    for (byte i = 0; i < NUM_OUTPUTS; i++) {
        Output *output = &config.outputs[i];
        if ((now - output->activatedAtMs) > output->maxActivationDurationMs) {
            output->status = OutputStatus::OFF;
            digitalWrite(output->pin1, 0);
            digitalWrite(output->pin2, 0);
        } else if (output->status == OutputStatus::OUT1) {
            digitalWrite(output->pin2, 0);
            // TODO: wait
            digitalWrite(output->pin1, 1);
        } else if (output->status == OutputStatus::OUT2) {
            digitalWrite(output->pin1, 0);
            // TODO: wait
            digitalWrite(output->pin2, 1);
        }
    }
}

void printInfos(unsigned long now) {
    if (now % 100UL == 0) {
        lcd.setCursor(0, 1);
        lcd.print(F("                "));
        lcd.setCursor(0, 1);
        Input *button = &config.inputs[1];
        lcd.print(button->getPin());
        lcd.print(':');
        lcd.print(button->getActiveDurationMs(now));
        lcd.print(' ');
    }
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

#if FLOOR_EG == 1 && FLOOR_OG == 0
    initEG();
#elif FLOOR_EG == 0 && FLOOR_OG == 1
    initOG();
#else
#error "set floor"
#endif

    lcd.begin(16, 2);
    lcd.print(NUM_INPUTS);
    lcd.print(F(" Buttons"));
}

void initEG() {
    /*0*/ mapping(0).close(23, 22).open(25, 24).end(); // Bad EG
    /*1*/ mapping(1).close(27, 26).open(29, 28).end(); // Schlafz. EG
    /*2*/ mapping(2).close(31, 30).open(33, 32).end(); // WZ Tür EG
    /*3*/ mapping(3).close(35, 34).open(37, 36).end(); // WZ Fenster EG
    /*4*/ mapping(4).close(39, 38).open(41, 40).end(); // Küche Tür EG
    /*5*/ mapping(5).close(43, 42).open(45, 44).end(); // Küche Fenster EG
    /*6*/ mapping(6).close(47, 46).open(49, 48).end(); // Gäste WC EG
    /*7*/ mapping(7).close(51, 50).open(53, 52).end(); // HWR EG
}

void initOG() {
    /*0*/ mapping(0).close(23, 22).open(25, 24).end(); // Reserve OG
    /*1*/ mapping(1).close(27, 26).open(29, 28).end(); // Flur OG
    /*2*/ mapping(2).close(31, 30).open(33, 32).end(); // Finn
    /*3*/ mapping(3).close(35, 34).open(37, 36).end(); // Lenn
    /*4*/ mapping(4).close(39, 38).open(41, 40).end(); // Musik
    /*5*/ mapping(5).close(43, 42).open(45, 44).end(); // Mina
    /*6*/ mapping(6).close(47, 46).open(49, 48).end(); // Büro
    /*7*/ mapping(7).close(51, 50).open(53, 52).end(); // Bad OG
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

