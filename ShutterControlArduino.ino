#include <Arduino.h>
#include <LiquidCrystal.h>
#include "ShutterControlArduino.h"
#include "Morse/Morse.h"

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int loopCnt = 0;

void initEG();

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
                    output->status = mapping->statusToActivate;
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
        config.mappings[0].outputs[0]->status = OUT1;
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
        Input *button = &config.inputs[1];
        lcd.print(button->pin);
        lcd.print(':');
        lcd.print(button->activeDurationMs);
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
    /*0*/ mapping(0).zu(23, 22).auf(25, 24).end(); // Bad EG
    /*1*/ mapping(1).zu(27, 26).auf(29, 28).end(); // Schlafz. EG
    /*2*/ mapping(2).zu(31, 30).auf(33, 32).end(); // WZ Tür EG
    /*3*/ mapping(3).zu(35, 34).auf(37, 36).end(); // WZ Fenster EG
    /*4*/ mapping(4).zu(39, 38).auf(41, 40).end(); // Küche Tür EG
    /*5*/ mapping(5).zu(43, 42).auf(45, 44).end(); // Küche Fenster EG
    /*6*/ mapping(6).zu(47, 46).auf(49, 48).end(); // Gäste WC EG
    /*7*/ mapping(7).zu(51, 50).auf(53, 52).end(); // HWR EG
}

void initOG() {
    /*0*/ mapping(0).zu(23, 22).auf(25, 24).end(); // Reserve OG
    /*1*/ mapping(1).zu(27, 26).auf(29, 28).end(); // Flur OG
    /*2*/ mapping(2).zu(31, 30).auf(33, 32).end(); // Finn
    /*3*/ mapping(3).zu(35, 34).auf(37, 36).end(); // Lenn
    /*4*/ mapping(4).zu(39, 38).auf(41, 40).end(); // Musik
    /*5*/ mapping(5).zu(43, 42).auf(45, 44).end(); // Mina
    /*6*/ mapping(6).zu(47, 46).auf(49, 48).end(); // Büro
    /*7*/ mapping(7).zu(51, 50).auf(53, 52).end(); // Bad OG
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

