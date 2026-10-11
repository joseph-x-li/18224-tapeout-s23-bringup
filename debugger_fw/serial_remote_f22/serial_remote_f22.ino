#include <Arduino.h>
#include "chip_interface.h"

// Same serial protocol as serial_remote, with two changes for designs clocked from IN[0]:
//  - the pins are initialized once, instead of being cleared to 0 on every command
//  - IN[0] is written last, so a rising edge on it sees the new values of IN[11:1]

uint8_t read_blocking() {
    int x = -1;
    while (x == -1) x = Serial.read();
    return x & 0xFF;
}

void setup() {
    Serial.begin(115200);
    bool initialized = false;

    Serial.setTimeout(0);

    bool led = false;
    while (true) {
        if (read_blocking() == 0x5B) {

            if (!initialized) {
                init_portio();
                initialized = true;
            }

            uint8_t hi  = read_blocking();
            uint8_t lo  = read_blocking();
            uint8_t chk = read_blocking();

            if (chk != (((hi+lo) ^ 0xAB) & 0xFF)) continue;

            set_inputs(hi & 0x0F, 11, 8);
            set_inputs(lo >> 1, 7, 1);
            set_reset((hi >> 4) & 0x1);
            set_input(0, lo & 0x1);

            if ((hi >> 5) & 0x1) { // step the clock
                step_clock();
            } else { // just set the clock
                set_clock((hi >> 6) & 0x1);
            }
            delayMicroseconds(10);

            hi = get_outputs(11, 8) | 0x40;
            lo = get_outputs(7, 0);
            Serial.write(hi);
            Serial.write(lo);
            Serial.write(((hi+lo) ^ 0xAB) & 0xFF);

            led = !led;
            set_status_led(led);
        }
    }
}

void loop() { }
