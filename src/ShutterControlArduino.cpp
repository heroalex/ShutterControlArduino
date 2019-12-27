#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <ShutterControlArduino.h>
#include <Ticker.h>

#if defined(USE_LCD)
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
#endif

struct { 
    char data[200];
    byte len = 0;
} rxBuffer;

struct { 
    char data[200];
    byte len = 0;
} txBuffer;

void initEG();

void initOG();

void updateInputs() {
    memset(&txBuffer.data[0], 0, sizeof(txBuffer.data));
    txBuffer.len = 0;
    unsigned long now = millis();
    for (byte i = 0; i < NUM_INPUTS; i++) {
        Input* input = &inputs[i];
        input->update(now);
        if (input->hasChanged()) {
            if(txBuffer.len != 0) {
                txBuffer.data[txBuffer.len++] = '/';
            }
            if(input->active()) {
                txBuffer.data[txBuffer.len++] = 'I';
            } else {
                txBuffer.data[txBuffer.len++] = 'X';
            }
            itoa(input->getPin(), &txBuffer.data[txBuffer.len], 10);
            txBuffer.len = strlen(txBuffer.data);
        }
    }

    if(txBuffer.len != 0) {
        Serial.println(txBuffer.data);
    }
}

void updateMappings() {
    unsigned long now = millis();
    for (byte i = 0; i < NUM_MAPPINGS; i++) {
        InputToOutputMapping* mapping = &mappings[i];
        Input *input = mapping->input;
        Output *output = mapping->output;
        if (input->active() && output->status != STOPPING && output->status != mapping->statusToActivate) {
            output->activatedAtMs = now;
            if(output->status == IDLE) {
                output->status = mapping->statusToActivate;
            } else {
                output->status = STOPPING;
            }
        }
    }
}

void updateOutputs() {
    unsigned long now = millis();
    for (byte i = 0; i < NUM_OUTPUTS; i++) {
        Output* output = &outputs[i];
        if ((output->status == OPENING || output->status == CLOSING) &&
            ((now - output->activatedAtMs) > output->maxActivationDurationMs)) {
            output->status = STOPPING;
            digitalWrite(output->powerPin, OUTPUT_OFF);
        } else if (output->status == OPENING) {
            digitalWrite(output->dirPin, OUTPUT_ON);
            if(!output->isSwitching) {
                output->isSwitching = true;
            } else {
                digitalWrite(output->powerPin, OUTPUT_ON);
            }
        } else if (output->status == CLOSING) {
            digitalWrite(output->dirPin, OUTPUT_OFF);
            if(!output->isSwitching) {
                output->isSwitching = true;
            } else {
                digitalWrite(output->powerPin, OUTPUT_ON);
            }
        } else if (output->status == STOPPING) {
            if ((now - output->activatedAtMs) > config.outputDirectionSwitchingMinDuration) {
                output->status = IDLE;
                output->activatedAtMs = ULONG_MAX;
                 // reset the duration to the config value, probably it was changed via serial e.g. for a 50% close
                output->maxActivationDurationMs = config.outputPowerMaxDuration;
                output->isSwitching = false;
            }
            digitalWrite(output->powerPin, OUTPUT_OFF);
            digitalWrite(output->dirPin, OUTPUT_OFF);
        }
    }
}

#if defined(USE_LCD)
void printInfos() {
    #define LCD2
    #ifdef LCD0
    lcd.setCursor(0, 0);
    for (byte i = 0; i < NUM_INPUTS; i++) {
        lcd.print(inputs[i].getRawValue());
    }

    lcd.setCursor(0, 1);
    for (byte i = 0; i < NUM_INPUTS; i++) {
        lcd.print(inputs[i].getDebouncedValue());
    }
    #endif

    #ifdef LCD1
    unsigned long now = millis();
    lcd.setCursor(0, 0);
    lcd.print(inputs[0].getRawValue());
    lcd.print(' ');
    lcd.print(inputs[0].getDebouncedValue());
    lcd.print(' ');
    lcd.print(inputs[0].active(now));

    lcd.setCursor(0, 1);
    lcd.print(inputs[0].getActivationId());
    lcd.print(' ');
    lcd.print(inputs[0].activeDurationMs(now));
    #endif

    #ifdef LCD2
    unsigned long now = millis();
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print(inputs[0].active());
    lcd.print(' ');
    lcd.print(inputs[0].activeDurationMs(now));

    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(outputs[0].status);
    lcd.print(' ');
    lcd.print(outputs[0].activatedAtMs);
    #endif
}
#endif

#if defined(BLINK_LED)
void blinkLed() {
    static bool state = true;
    digitalWrite(LED_BUILTIN, state);
    state = !state;
}
#endif

void handleMapping(char* mappingNumStr, char* closeInStr, char* openInStr, char* powerOutStr, char* dirOutStr) {
    byte mappingNum = atoi(&mappingNumStr[1]);
    byte closeIn = atoi(closeInStr);
    byte openIn = atoi(openInStr);
    byte powerOut = atoi(powerOutStr);
    byte dirOut = atoi(dirOutStr);
    mapping(mappingNum).closeIn(closeIn).openIn(openIn).powerOut(powerOut).dirOut(dirOut).end();
}

void handleMappings() {
    
    char *str1, *token, *mappingNum, *closeIn, *openIn, *powerOut, *dirOut;
    char *saveptr1, *saveptr2;

    for (str1 = rxBuffer.data;; str1 = NULL)
    {
        token = strtok_r(str1, "/", &saveptr1);
        if (token == NULL) {
            return;
        }
        mappingNum = strtok_r(token, ":", &saveptr2);
        if (mappingNum == NULL) {
            #ifdef DEBUG
            Serial.print("NOK MAPPING_NUM: ");
            Serial.println(token);
            #endif
            return;
        }
        closeIn = strtok_r(NULL, ":", &saveptr2);
        if (closeIn == NULL) {
            #ifdef DEBUG
            Serial.print("NOK CLOSE_IN: ");
            Serial.println(token);
            #endif
            return;
        }
        openIn = strtok_r(NULL, ":", &saveptr2);
        if (openIn == NULL) {
            #ifdef DEBUG
            Serial.print("NOK OPEN_IN: ");
            Serial.println(token);
            #endif
            return;
        }
        powerOut = strtok_r(NULL, ":", &saveptr2);
        if (powerOut == NULL) {
            #ifdef DEBUG
            Serial.print("NOK POWER_OUT: ");
            Serial.println(token);
            #endif
            return;
        }
        dirOut = strtok_r(NULL, ":", &saveptr2);
        if (dirOut == NULL) {
            #ifdef DEBUG
            Serial.print("NOK DIR_OUT: ");
            Serial.println(token);
            #endif
            return;
        }
        handleMapping(mappingNum, closeIn, openIn, powerOut, dirOut);
        
        #ifdef DEBUG
        Serial.print("OK: ");
        Serial.print(mappingNum);
        Serial.print(':');
        Serial.print(closeIn);
        Serial.print(':');
        Serial.print(openIn);
        Serial.print(':');
        Serial.println(powerOut);
        Serial.print(':');
        Serial.println(dirOut);
        #endif
    }
}

void handleParam(char* paramNumStr, char* paramValStr) {
    byte paramNum = atoi(&paramNumStr[1]);
    uint16_t paramVal = atoi(paramValStr);

    switch (paramNum) {
        case 0: config.inputActivationThreshold = paramVal; break;
        case 1: config.inputDeactivationThreshold = paramVal; break;
        case 2: config.outputPowerMaxDuration = paramVal; break;
        case 3: config.outputDirectionSwitchingMinDuration = paramVal; break;
    }
}

void handleParams() {
    char *str1, *token, *paramNum, *paramVal;
    char *saveptr1, *saveptr2;

    for (str1 = rxBuffer.data;; str1 = NULL)
    {
        token = strtok_r(str1, "/", &saveptr1);
        if (token == NULL) {
            break;
        }
        paramNum = strtok_r(token, ":", &saveptr2);
        if (paramNum == NULL) {
            #ifdef DEBUG
            Serial.print("NOK PARAM_NUM: ");
            Serial.println(token);
            #endif
            break;
        }
        paramVal = strtok_r(NULL, ":", &saveptr2);
        if (paramVal == NULL) {
            #ifdef DEBUG
            Serial.print("NOK PARAM_VAL: ");
            Serial.println(token);
            #endif
            break;
        }
        handleParam(paramNum, paramVal);
        
        #ifdef DEBUG
        Serial.print("OK: ");
        Serial.print(paramNum);
        Serial.print(':');
        Serial.println(paramVal);
        #endif
    }
    #ifdef UPDATE_EEPROM
    EEPROM.put(0, config);
    #endif
}

void handleOutput(char* outputNumStr, char* outputDirStr, char* outputDurationStr) {
    byte outputNum = atoi(&outputNumStr[1]);
    byte outputDir = atoi(outputDirStr);
    uint16_t outputDuration = config.outputPowerMaxDuration; // default
    if(outputDurationStr != NULL) {
        outputDuration = atoi(outputDurationStr);
    }
    unsigned long now = millis();
    Output* output = &outputs[outputNum];
    output->status = outputDir > 0 ? OPENING:CLOSING;
    output->activatedAtMs = now;
    output->maxActivationDurationMs = outputDuration;
}

void handleOutputs() {
    char *str1, *token, *outputNum, *outputDir, *outputDuration;
    char *saveptr1, *saveptr2;

    for (str1 = rxBuffer.data;; str1 = NULL)
    {
        token = strtok_r(str1, "/", &saveptr1);
        if (token == NULL) {
            return;
        }
        outputNum = strtok_r(token, ":", &saveptr2);
        if (outputNum == NULL) {
            #ifdef DEBUG
            Serial.print("NOK OUTPUT_NUM: ");
            Serial.println(token);
            #endif
            return;
        }
        outputDir = strtok_r(NULL, ":", &saveptr2);
        if (outputDir == NULL) {
            #ifdef DEBUG
            Serial.print("NOK OUTPUT_DIR: ");
            Serial.println(token);
            #endif
            return;
        }
        outputDuration = strtok_r(NULL, ":", &saveptr2);
        if (outputDir == NULL) {
            // do nothing, duration is optional. If not given, the default is used
        }
        handleOutput(outputNum, outputDir, outputDuration);
        
        #ifdef DEBUG
        Serial.print("OK: ");
        Serial.print(outputNum);
        Serial.print(':');
        Serial.print(outputDir);
        Serial.print(':');
        Serial.println(outputDuration);
        #endif
    }
}

void handleGetParams() {
    memset(&txBuffer.data[0], 0, sizeof(txBuffer.data));
    txBuffer.len = 0;
    sprintf(txBuffer.data, "p0(iat):%u p1(idt):%u p2(opmd): %u p3(odsmd):%u", config.inputActivationThreshold, config.inputDeactivationThreshold, config.outputPowerMaxDuration, config.outputDirectionSwitchingMinDuration);
    txBuffer.len = strlen(txBuffer.data);
    Serial.println(txBuffer.data);
}

#ifdef FLOOR_OG
void initOG() {
    /*0*/ mapping(0).closeIn(23).openIn(25).powerOut(22).dirOut(24).end(); // Reserve OG
    /*1*/ mapping(1).closeIn(27).openIn(29).powerOut(26).dirOut(28).end(); // Flur OG
    /*2*/ mapping(2).closeIn(31).openIn(33).powerOut(30).dirOut(32).end(); // Finn
    /*3*/ mapping(3).closeIn(35).openIn(37).powerOut(34).dirOut(36).end(); // Lenn
    /*4*/ mapping(4).closeIn(39).openIn(41).powerOut(38).dirOut(40).end(); // Musik
    /*5*/ mapping(5).closeIn(43).openIn(45).powerOut(42).dirOut(44).end(); // Mina
    /*6*/ mapping(6).closeIn(47).openIn(49).powerOut(46).dirOut(48).end(); // Büro
    /*7*/ mapping(7).closeIn(51).openIn(53).powerOut(50).dirOut(52).end(); // Bad OG
}
#else
void initEG() {
    /*0*/ mapping(0).closeIn(23).openIn(25).powerOut(22).dirOut(24).end(); // Bad EG
    /*1*/ mapping(1).closeIn(27).openIn(29).powerOut(26).dirOut(28).end(); // Schlafz. EG
    /*2*/ mapping(2).closeIn(31).openIn(33).powerOut(30).dirOut(32).end(); // WZ Tür EG
    /*3*/ mapping(3).closeIn(35).openIn(37).powerOut(34).dirOut(36).end(); // WZ Fenster EG
    /*4*/ mapping(4).closeIn(39).openIn(41).powerOut(38).dirOut(40).end(); // Küche Tür EG
    /*5*/ mapping(5).closeIn(43).openIn(45).powerOut(42).dirOut(44).end(); // Küche Fenster EG
    /*6*/ mapping(6).closeIn(47).openIn(49).powerOut(46).dirOut(48).end(); // Gäste WC EG
    /*7*/ mapping(7).closeIn(51).openIn(53).powerOut(50).dirOut(52).end(); // HWR EG
}
#endif

Ticker inputsTimer(updateInputs, INPUTS_CHECK_MSEC, 0, MILLIS);
Ticker mappingsTimer(updateMappings, 50, 0, MILLIS);
Ticker outputsTimer(updateOutputs, 50, 0, MILLIS);
#if defined(BLINK_LED)
Ticker blinkTimer(blinkLed, 500, 0, MILLIS);
#endif
#if defined(USE_LCD)
    Ticker infoLcdTimer(printInfos, 100, 0, MILLIS);
#endif

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

#ifdef SETUP_EEPROM
    EEPROM.put(0, config);
#endif
#ifdef READ_EEPROM
    EEPROM.get(0, config);
#endif

    Serial.begin(9600);
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

#ifdef FLOOR_OG
    initOG();
#else
    initEG();
#endif

#if defined(USE_LCD)
    lcd.begin(16, 2);
    infoLcdTimer.start();
#endif
#if defined(BLINK_LED)
    blinkTimer.start();
#endif
    inputsTimer.start();
    mappingsTimer.start();
    outputsTimer.start();
}

void loop() {
#if defined(BLINK_LED)
    blinkTimer.update();
#endif
    inputsTimer.update();
    mappingsTimer.update();
    outputsTimer.update();

#if defined(USE_LCD)
    infoLcdTimer.update();
#endif
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    #ifdef SERIAL_ECHO
    Serial.print(inChar);
    #endif
    if (inChar == '\n') {
        // terminate the buffered string with a null
        rxBuffer.data[rxBuffer.len++] = '\0';
        char cmd = rxBuffer.data[0];
        switch(cmd) {
            case 'M':
                handleMappings();
                break;
            case 'P':
                handleParams();
                break;
            case 'O':
                handleOutputs();
                break;
            case 'G':
                //handleGetMappings();
                break;
            case 'C':
                handleGetParams();
                break;
        }
        // clear the string:
        memset(&rxBuffer.data[0], 0, sizeof(rxBuffer.data));
        rxBuffer.len = 0;
        // allow the main loop to run
        return;
    } else {
        // add it to the buffer:
        rxBuffer.data[rxBuffer.len++] = inChar;
    }
  }
}