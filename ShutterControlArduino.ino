#include <Arduino.h>
#include <LiquidCrystal.h>
#include "ShutterControlArduino.h"
#include "Morse/Morse.h"

#if defined(USE_LCD1) || defined(USE_LCD2) || defined(USE_LCD3)
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int loopCnt = 0;
#endif

void initEG();

void initOG();

void initExtraMappingsEG();

void initExtraMappingsOG();

void updateInputs(unsigned long now) {
    boolean anyActive = false;
    for (byte i = 0; i < NUM_INPUTS; i++) {
        Input *input = &config.inputs[i];
        byte val = digitalRead(input->getPin());
        input->update(val, now);
        if (input->getIsActive() == true) {
            anyActive = true;
        }
    }

    if (anyActive) {
        digitalWrite(LED_BUILTIN, HIGH);
    } else {
        digitalWrite(LED_BUILTIN, LOW);
    }
}

void updateMappings(unsigned long now) {
    for (byte i = 0; i < NUM_MAPPINGS; i++) {
        InputToOutputMapping *mapping = &config.mappings[i];
        Input **inputs = mapping->inputs;
        byte numInputs = mapping->numInputs;
        for (byte j = 0; j < numInputs; j++) {
            Input *input = inputs[j];
            if (input->getIsActive()) {
                Output **outputs = mapping->outputs;
                byte numOutputs = mapping->numOutputs;
                for (byte k = 0; k < numOutputs; k++) {
                    Output *output = outputs[k];
                    if (output->status == IDLE && input->getActiveDurationMs(now) > mapping->activationThresholdMs) {
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
            digitalWrite(output->closePin, OUTPUT_OFF);
            digitalWrite(output->openPin, OUTPUT_OFF);
        } else if (output->status == OPENING) {
            digitalWrite(output->openPin, OUTPUT_OFF);
            digitalWrite(output->closePin, OUTPUT_ON);
        } else if (output->status == CLOSING) {
            digitalWrite(output->closePin, OUTPUT_OFF);
            digitalWrite(output->openPin, OUTPUT_ON);
        } else if (output->status == STOPPING) {
            if ((now - output->activatedAtMs) > DEFAULT_OUT_MIN_STOPPING_DURATION) {
                output->status = IDLE;
                output->activatedAtMs = ULONG_MAX;
                output->activationId = 0;
            }
            digitalWrite(output->closePin, OUTPUT_OFF);
            digitalWrite(output->openPin, OUTPUT_OFF);
        }
    }
}

#ifdef USE_LCD1
void printInfos1(unsigned long now) {
    if (now % 100UL == 0) {
        lcd.setCursor(0, 0);
        lcd.print(F("                "));
        lcd.setCursor(0, 0);
        Input *input1 = config.mappings[0].inputs[0];
        lcd.print(input1->getPin());
        lcd.print(':');
        lcd.print(input1->getActivationId());
        lcd.print(' ');
        lcd.print(input1->getActiveConf());
        lcd.setCursor(0, 1);
        lcd.print(F("                "));
        lcd.setCursor(0, 1);
        Input *input2 = config.mappings[1].inputs[0];
        lcd.print(input2->getPin());
        lcd.print(':');
        lcd.print(input2->getActivationId());
        lcd.print(' ');
        lcd.print(input2->getActiveConf());
        
        Output *output = config.mappings[0].outputs[0];
        lcd.print(' ');
        lcd.print('O');
        lcd.print(output->status);
    }
}
#endif

#ifdef USE_LCD2

void printInfos2(unsigned long now) {
    if (now % 100UL == 0) {

        lcd.setCursor(0, 0);
        for (byte i = 0; i < NUM_MAPPINGS - 2; i++) {
            InputToOutputMapping *mapping = &config.mappings[i];
            Input **inputs = mapping->inputs;
            byte numInputs = mapping->numInputs;
            for (byte j = 0; j < numInputs; j++) {
                Input *input = inputs[j];
                lcd.print(input->getIsActive());
            }
        }

        lcd.setCursor(0, 1);
        for (byte i = 0; i < NUM_MAPPINGS - 2; i++) {
            InputToOutputMapping *mapping = &config.mappings[i];
            Output **outputs = mapping->outputs;
            byte numOutputs = mapping->numOutputs;
            for (byte j = 0; j < numOutputs; j++) {
                Output *output = outputs[j];
                lcd.print(output->status);
            }

        }
    }
}

#endif

#ifdef USE_LCD3

void printInfos3(unsigned long now) {
    if (now % 100UL == 0) {

        lcd.setCursor(0, 0);
        for (byte i = 0; i < NUM_INPUTS; i++) {
            Input *input = &config.inputs[i];
            byte val = digitalRead(input->getPin());
            lcd.print(val);
        }

        lcd.setCursor(0, 1);
        for (byte i = 0; i < NUM_MAPPINGS - 2; i++) {
            InputToOutputMapping *mapping = &config.mappings[i];
            Input **inputs = mapping->inputs;
            byte numInputs = mapping->numInputs;
            for (byte j = 0; j < numInputs; j++) {
                Input *input = inputs[j];
                lcd.print(input->getIsActive());
            }
        }
    }
}

#endif

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

#ifdef FLOOR_OG
    initOG();
#else
    initEG();
#endif

#if defined(USE_LCD1) || defined(USE_LCD2) || defined(USE_LCD3)
    lcd.begin(16, 2);
#endif
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

    initExtraMappingsEG();
}

void initExtraMappingsEG() {
    /*8*/
    InputToOutputMapping *mappingCloseAllEG = &config.mappings[16];
    InputToOutputMapping *mappingOpenAllEG = &config.mappings[17];

    mappingCloseAllEG->inputs = static_cast<Input **>(malloc(6 * sizeof(Input *)));
    mappingOpenAllEG->inputs = static_cast<Input **>(malloc(6 * sizeof(Input *)));
    if (mappingCloseAllEG->inputs == NULL || mappingOpenAllEG->inputs == NULL) {
        morseError('I');
    }

    mappingCloseAllEG->outputs = static_cast<Output **>(malloc(NUM_OUTPUTS * sizeof(Output *)));
    mappingOpenAllEG->outputs = static_cast<Output **>(malloc(NUM_OUTPUTS * sizeof(Output *)));
    if (mappingCloseAllEG->outputs == NULL || mappingOpenAllEG->outputs == NULL) {
        morseError('O');
    }

    mappingCloseAllEG->numInputs = 6;
    mappingOpenAllEG->numInputs = 6;

    mappingCloseAllEG->numOutputs = NUM_OUTPUTS;
    mappingOpenAllEG->numOutputs = NUM_OUTPUTS;

    mappingCloseAllEG->statusToActivate = CLOSING;
    mappingCloseAllEG->activationThresholdMs = (1000UL * 3); // 3s
    mappingOpenAllEG->statusToActivate = OPENING;
    mappingOpenAllEG->activationThresholdMs = (1000UL * 3); // 3s

    mappingCloseAllEG->inputs[0] = config.mappings[2].inputs[0]; // Schlafz. EG
    mappingCloseAllEG->inputs[1] = config.mappings[4].inputs[0]; // WZ Tür EG
    mappingCloseAllEG->inputs[2] = config.mappings[6].inputs[0]; // WZ Fenster EG
    mappingCloseAllEG->inputs[3] = config.mappings[8].inputs[0]; // Küche Tür EG
    mappingCloseAllEG->inputs[4] = config.mappings[10].inputs[0]; // Küche Fenster EG
    mappingCloseAllEG->inputs[5] = config.mappings[14].inputs[0]; // HWR EG

    mappingOpenAllEG->inputs[0] = config.mappings[3].inputs[0]; // Schlafz. EG
    mappingOpenAllEG->inputs[1] = config.mappings[5].inputs[0]; // WZ Tür EG
    mappingOpenAllEG->inputs[2] = config.mappings[7].inputs[0]; // WZ Fenster EG
    mappingOpenAllEG->inputs[3] = config.mappings[9].inputs[0]; // Küche Tür EG
    mappingOpenAllEG->inputs[4] = config.mappings[11].inputs[0]; // Küche Fenster EG
    mappingOpenAllEG->inputs[5] = config.mappings[15].inputs[0]; // HWR EG

    for (int i = 0; i < NUM_OUTPUTS; ++i) {
        Output *output = &config.outputs[i];
        mappingCloseAllEG->outputs[i] = output;
        mappingOpenAllEG->outputs[i] = output;
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

    //initExtraMappings();
}

void loop() {
    unsigned long now = millis();
#if 0 //def USE_LCD
    loopCnt++;
    if (now % 1000UL == 0) {
        lcd.setCursor(0, 0);
        lcd.print(F("     "));
        lcd.print(loopCnt);
        loopCnt = 0;
    }
#endif

    updateInputs(now);

    updateMappings(now);

    updateOutputs(now);
#ifdef USE_LCD1
    printInfos1(now);
#elif defined(USE_LCD2)
    printInfos2(now);
#elif defined(USE_LCD3)
    printInfos3(now);
#endif
}

