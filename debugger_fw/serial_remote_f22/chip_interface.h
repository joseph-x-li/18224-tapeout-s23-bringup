#include <Arduino.h>

#define __ASSERT_USE_STDERR
#include <assert.h>

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
 *      - create one clock cycle with length 2uS
 */


// Status LED is connected to LED_BUILTIN
// Use digitalWrite for this port only
void set_status_led(bool st) {
    digitalWrite(LED_BUILTIN, st);
}

void init_portio() {
    pinMode(LED_BUILTIN, OUTPUT);

    // PA = port 0
    // PB = port 1

    // set chip inputs as debugger outputs
    // debugger -> chip [0 - 11]:
    // {PA23, PA22, PA21, PA20, PA19, PA18,
    //  PA16, PA15, PA14, PA13, PA12, PA11}
    REG_PORT_DIRSET0 = 0x00FDF800u;

    // set clk and reset pins as outputs
    // rst = PB08, clk = PB09
    REG_PORT_DIRSET1 = 0x00000300u;

    // set chip outputs as debugger inputs
    // chip -> debugger [0 - 11]:
    // {PA10, PA09, PA08, PA07, PA06, PA05,
    //  PA04, PA03, PA02, PB03, PB02, PA28}
    REG_PORT_DIRCLR0 = 0x100007FCu;
    REG_PORT_DIRCLR1 = 0x0000000Cu;

    // set direction and pull resistors
    *(((uint8_t*) &REG_PORT_PINCFG0) + 10) = 0x6u; // INEN, PULLEN
    *(((uint8_t*) &REG_PORT_PINCFG0) + 9)  = 0x6u; // INEN, PULLEN
    *(((uint8_t*) &REG_PORT_PINCFG0) + 8)  = 0x6u; // INEN, PULLEN
    *(((uint8_t*) &REG_PORT_PINCFG0) + 7)  = 0x6u; // INEN, PULLEN
    *(((uint8_t*) &REG_PORT_PINCFG0) + 6)  = 0x6u; // INEN, PULLEN
    *(((uint8_t*) &REG_PORT_PINCFG0) + 5)  = 0x6u; // INEN, PULLEN
    *(((uint8_t*) &REG_PORT_PINCFG0) + 4)  = 0x6u; // INEN, PULLEN
    *(((uint8_t*) &REG_PORT_PINCFG0) + 3)  = 0x6u; // INEN, PULLEN
    *(((uint8_t*) &REG_PORT_PINCFG0) + 2)  = 0x6u; // INEN, PULLEN
    *(((uint8_t*) &REG_PORT_PINCFG0) + 28) = 0x6u; // INEN, PULLEN
    *(((uint8_t*) &REG_PORT_PINCFG1) + 3)  = 0x6u; // INEN, PULLEN
    *(((uint8_t*) &REG_PORT_PINCFG1) + 2)  = 0x6u; // INEN, PULLEN

    REG_PORT_OUTCLR0 = 0x100007FCu;
    REG_PORT_OUTCLR1 = 0x0000000Cu;

    // set all chip inputs to zero
    REG_PORT_OUTCLR0 = 0x00FDF800u;

    // set clock and reset to zero
    REG_PORT_OUTCLR1 = 0x00000300u;
}

const int _input_idxs[]  = {23, 22, 21, 20, 19, 18, 16, 15, 14, 13, 12, 11};
const int _output_idxs[] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 3, 2, 28};

void set_input(int idx, bool st) {
    assert((idx >= 0) && (idx <= 11));

    if (st) REG_PORT_OUTSET0 = (1 << _input_idxs[idx]);
    else    REG_PORT_OUTCLR0 = (1 << _input_idxs[idx]);
}

bool get_output(int idx) {
    assert((idx >= 0) && (idx <= 11));

    if ((idx == 9) || (idx == 10)) { // These ones are on Port B
        return (REG_PORT_IN1 & (1 << _output_idxs[idx])) != 0;
    } else {
        return (REG_PORT_IN0 & (1 << _output_idxs[idx])) != 0;
    }
}

// Equivalent to SystemVerilog `inputs[hi:lo] = vals`
void set_inputs(uint32_t vals, int hi, int lo) {
    assert((hi <= 11) && (lo >= 0) && (hi >= lo));

    for (int i = lo; i <= hi; i++) {
        set_input(i, vals & 0x1);
        vals = vals >> 1;
    }
}

// Equivalent to SystemVerilog `outputs[hi:lo]`
uint32_t get_outputs(int hi, int lo) {
    assert((hi <= 11) && (lo >= 0) && (hi >= lo));

    uint32_t out = 0;
    for (int i = hi; i >= lo; i--) {
        out = out << 1;
        out |= (get_output(i) & 0x1);
    }
    return out;
}

void set_reset(bool st) {
    if (st) REG_PORT_OUTSET1 = 0x100u;
    else    REG_PORT_OUTCLR1 = 0x100u;
}

void set_clock(bool st) {
    if (st) REG_PORT_OUTSET1 = 0x200u;
    else    REG_PORT_OUTCLR1 = 0x200u;
}

void step_clock() {
    // This caps us at a 500kHz clock
    // It is likely possible to go faster
    // But this is known to work
    REG_PORT_OUTSET1 = 0x200u;
    delayMicroseconds(1);
    REG_PORT_OUTCLR1 = 0x200u;
    delayMicroseconds(1);
}

void step_clock(int n) {
    for (int i = 0; i < n; i++) step_clock();
}

// Assertions wrapper
void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp) {
    while (true) {
        Serial.println("**** ASSERTION FAILED ****");
        Serial.println(__func);
        Serial.println(__file);
        Serial.println(__lineno, DEC);
        Serial.println(__sexp);
        Serial.flush();
        delay(2000);
    }
}
