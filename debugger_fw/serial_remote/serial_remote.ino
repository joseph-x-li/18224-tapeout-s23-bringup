#include <Arduino.h>
#include "chip_interface.h"

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
                // Delayed init to give the user a chance to unplug
                // any extra circuits before starting the script
                init_portio();
                initialized = true;
            }

            uint8_t hi  = read_blocking();
            uint8_t lo  = read_blocking();
            uint8_t chk = read_blocking();

            if (chk != (((hi+lo) ^ 0xAB) & 0xFF)) continue;
            if (hi & 0x80 != 0x80) continue;

            // IN[0] is written last so that designs clocked from IN[0]
            // (the F22 designs) see the new values of IN[11:1] on its edge
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

/**
 * Summary of use:
 * - void init_portio()
 *      - configure the I/O config
 *
 * - void set_status_led(bool val)
 *      - set the onboard status LED
 *
 * - void set_input(int idx, bool val)
 *      - equivalent to `inputs[idx] = val`
 *
 * - bool get_output(int idx)
 *      - equivalent to `return outputs[idx]`
 *
 * - void set_inputs(uint32_t vals, int hi, int lo)
 *      - equivalent to `inputs[hi:lo] = vals`
 *
 * - uint32_t get_outputs(int hi, int lo)
 *      - equivalent to `return outputs[hi:lo]`
 *
 * - void set_reset(bool val)
 *      - set the chip's reset input
 *
 * - void set_clock(bool val)
 *      - set the chip's clock input (unsupported: use step_clock instead)
 *
 * - void step_clock(bool val)
 *      - create one clock cycle with length 2us
 */

