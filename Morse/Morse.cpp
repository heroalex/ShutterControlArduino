//
// Created by al on 17.03.2018.
//

#include "Morse.h"

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
            case 'O': {
                dah(1);
                dah(1);
                dah(7);
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