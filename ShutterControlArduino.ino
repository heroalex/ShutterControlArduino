#include <Arduino.h>
#include <LiquidCrystal.h>
#include "ShutterControlArduino.h"
#include "Morse/Morse.h"

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int loopCnt = 0;

void initEG();

void initExtraMappings();

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
                    if (output->status == IDLE) {
                        output->status = mapping->statusToActivate;
                        output->activatedAtMs = now;
                        output->activationId = input->getActivationId();
                    } else if ((output->status == OPENING || output->status == CLOSING) &&
                               output->activationId != input->getActivationId()) {
                        output->status = STOPPING;
                        output->activatedAtMs = now;
                    }
                }
            }
        }
    }
}

void updateOutputs(unsigned long now) {
    for (byte i = 0; i < NUM_OUTPUTS; i++) {
        Output *output = &config.outputs[i];
        if ((output->status == OPENING || output->status == CLOSING) &&
            ((now - output->activatedAtMs) > output->maxActivationDurationMs)) {
            output->status = STOPPING;
            digitalWrite(output->closePin, LOW);
            digitalWrite(output->openPin, LOW);
        } else if (output->status == OPENING) {
            digitalWrite(output->openPin, LOW);
            digitalWrite(output->closePin, HIGH);
        } else if (output->status == CLOSING) {
            digitalWrite(output->closePin, LOW);
            digitalWrite(output->openPin, HIGH);
        } else if (output->status == STOPPING) {
            if ((now - output->activatedAtMs) > DEFAULT_OUT_MIN_STOPPING_DURATION) {
                output->status = IDLE;
                output->activatedAtMs = 0;
                output->activationId = 0;
            }
            digitalWrite(output->closePin, LOW);
            digitalWrite(output->openPin, LOW);
        }
    }
}

void printInfos(unsigned long now) {
    if (now % 100UL == 0) {
        lcd.setCursor(0, 1);
        lcd.print(F("                "));
        lcd.setCursor(0, 1);
        Input *input1 = config.mappings[0].inputs[0];
        lcd.print(input1->getPin());
        lcd.print(':');
        lcd.print(input1->getActivationId());
        lcd.print(' ');
        Input *input2 = config.mappings[1].inputs[0];
        lcd.print(input2->getPin());
        lcd.print(':');
        lcd.print(input2->getActivationId());
        Output *output = config.mappings[0].outputs[0];
        lcd.print(' ');
        lcd.print(output->status);
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

    initExtraMappings();
}

void initExtraMappings() {
    /*8 ALL */
    byte numOpeningInputs = 0;
    byte numClosingInputs = 0;
    byte numOpeningOutputs = 0;
    byte numClosingOutputs = 0;
    byte idxOpeningInputs = 0;
    byte idxClosingInputs = 0;
    byte idxOpeningOutputs = 0;
    byte idxClosingOutputs = 0;

    // count numbers of closing and opening inputs and outputs
    for (byte i = 0; i < NUM_MAPPINGS - 2; i++) {
        InputToOutputMapping *mapping = &config.mappings[i];
        byte numInputs = mapping->numInputs;
        byte numOutputs = mapping->numOutputs;
        if (mapping->statusToActivate == CLOSING) {
            numClosingInputs += numInputs;
            numClosingOutputs += numOutputs;
        } else if (mapping->statusToActivate == OPENING) {
            numOpeningInputs += numInputs;
            numOpeningOutputs += numOutputs;
        }
    }

    InputToOutputMapping *mappingCloseAll = &config.mappings[16];
    InputToOutputMapping *mappingOpenAll = &config.mappings[17];

    mappingCloseAll->inputs = static_cast<Input **>(malloc(numClosingInputs * sizeof(Input *)));
    mappingOpenAll->inputs = static_cast<Input **>(malloc(numOpeningInputs * sizeof(Input *)));
    if (mappingCloseAll->inputs == NULL || mappingOpenAll->inputs == NULL) {
        morseError('I');
    }

    mappingCloseAll->outputs = static_cast<Output **>(malloc(numClosingOutputs * sizeof(Output *)));
    mappingOpenAll->outputs = static_cast<Output **>(malloc(numOpeningOutputs * sizeof(Output *)));
    if (mappingCloseAll->outputs == NULL || mappingOpenAll->outputs == NULL) {
        morseError('O');
    }

    mappingCloseAll->numInputs = numClosingInputs;
    mappingOpenAll->numInputs = numOpeningInputs;

    mappingCloseAll->numOutputs = numClosingOutputs;
    mappingOpenAll->numOutputs = numOpeningOutputs;

    mappingCloseAll->statusToActivate = CLOSING;
    mappingCloseAll->activationThresholdMs = (1000UL * 3); // 3s
    mappingOpenAll->statusToActivate = OPENING;
    mappingOpenAll->activationThresholdMs = (1000UL * 3); // 3s

    for (byte i = 0; i < NUM_MAPPINGS - 2; i++) {
        InputToOutputMapping *mapping = &config.mappings[i];
        Input **inputs = mapping->inputs;
        byte numInputs = mapping->numInputs;
        for (byte j = 0; j < numInputs; j++) {
            Input *input = inputs[j];
            if (mapping->statusToActivate == CLOSING) {
                mappingCloseAll->inputs[idxClosingInputs++] = input;
            } else if (mapping->statusToActivate == OPENING) {
                mappingOpenAll->inputs[idxOpeningInputs++] = input;
            }
        }
        Output **outputs = mapping->outputs;
        byte numOutputs = mapping->numOutputs;
        for (byte k = 0; k < numOutputs; k++) {
            Output *output = outputs[k];
            if (mapping->statusToActivate == CLOSING) {
                mappingCloseAll->outputs[idxClosingOutputs++] = output;
            } else if (mapping->statusToActivate == OPENING) {
                mappingOpenAll->outputs[idxOpeningOutputs++] = output;
            }
        }
    }
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

    initExtraMappings();
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

