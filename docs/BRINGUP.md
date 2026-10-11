---
geometry: margin=3cm
---

# Board Bringup Instructions

## [IMPORTANT] Errata
 
- When the debugger microcontroller is in reset, chip output 3 is held low rather than being high-impedance (as all other pins are).
    - This can be worked around by loading a firmware that manually sets all pins to high-impedance.
    - This is the result of a silicon errata in the ATSAMD21 microcontroller, see 1.13.2 in the [ATSAMD21 Silicon Errata](https://ww1.microchip.com/downloads/en/DeviceDoc/SAM-D21DA1-Family-Silicon-Errata-and-Data-Sheet-Clarification-DS80000760G.pdf)
    - When using the "switches board" - the small jumper on the bottom side (near the header) needs to be cut (using a knife) in order to not forcibly hold the microcontroller in reset
- USB C-C cables cannot be used to power the board due to an issue in the PD circuit. You will need to use a USB A-C cable.
- After changing the design, the clock must be stepped at least 5 times before the design is loaded
- F22 designs (TinyTapeout-style designs)
	- CLK is on IN[0] instead of CLK pin
	- Must still step the CLK pin to set the design
	- From Python, set the other inputs first, then raise IN[0] in a separate call (e.g. `set_all_inputs(v & ~1)` then `set_all_inputs(v | 1)`). This needs the current `serial_remote` firmware: older builds reset every input to 0 on each command, so an IN[0] edge always saw the other inputs as 0

## Kit Contents

(Please be cautious when opening the kit as small parts are easy to lose. The kits include spares for some—but not all—parts.)

- Debugger Board Kit
    - Debugger Board (assembled, with your ASIC)
    - 30-pin male header
    - 2-pin male header
    - 2-pin jumper

- Switches Kit
    - Buttons/Switches Board
    - 30-pin female header
    - 12x switches
    - 2x 3-pin male header
    - 2x 2-pin jumper

## Contact

If you run into any issues, feel free to email `ece224-bringup-help-s23 [at] lists.andrew.cmu.edu` for help! Additionally, if you get your design working, we'd love to hear about it, so please do let us know!

## Overview of the Debugger Board

To assist in bringup and testing of your tapeout projects, we have provided a "debugger" board that includes your chip, 1V8 and 3V3 power rails, and an ATSAMD21 microcontroller flashed with an Arduino bootloader for easy testing.

(image of the fully assembled board with annotations)

The board is powered via USB-C and can be powered from your computer. Please make sure you are using a USB-C cable with data lines (if you are using a "power-only" cable or the cable isn't firmly connected, it will appear to power up but not be detected by the serial port).

## Selecting a Project

Selecting your project can be done via the project-selection microswitches near the chip. When oriented with the "PROJECT SEL" text facing you, the 6 leftmost switches select the project (as a binary index, with the MSB on the left side), and the 7th (HOLD) switch can be used to disable unselected projects (unless doing highly precise power measurements, we recommend keeping the HOLD switch set at zero). The 8th switch is unused.

Use a small pin or similar tool to move the switches as they are too small to set by hand.

**IMPORTANT:** Regardless of what method you are using to interact with your design, you must always manually set the "select" switches.

(image of the switches and example of selecting a project)

You can find the index of your project in the [design list repo](https://github.com/asinghani/18224-s23-tapeout/tree/main/designs)

Due to a small bug in the June 2023 tapeout version of the multiplexer, the project-selection switches (as well as the reset input) are buffered, and require at least 5 clock pulses to register.

## Interacting with the Designs

There are four methods of interacting with the designs: The Python debugger interface, the Arduino debugger interface, the switches/buttons board, or a custom external circuit. Whichever method you choose, we recommend starting with one of the example designs (01-adder or 02-counter) first, to verify that your setup is working before starting to test your own design. **We also highly recommend reading the "Errata + Caveats" section before getting started.**

### Method (1): Python Debugger Interface

The easiest way to get started is using the Python debugger setup. This setup involves running code on the microcontroller which passes GPIO in/out to the host machine over the serial port, which is then bridged to a Python script from which you can run your tests. If you used the "debug interface" as part of your tapeout testing, it should translate almost exactly into the new Python debugger interface.

The main limitation with the Python interface is that the update rate is capped at \~500Hz due to serial bandwidth limits.

If you received a pre-assembled board, it should already be loaded with the debugger firmware. Otherwise, follow the instructions in the next section to flash `debugger_fw/serial_remote` to the Arduino. 

#### Writing Code

We recommend making a copy of one of the example projects and building your code from that.

The following functions are provided on the `Chip` object:

```
- chip.get_all_outputs()
	- Return a 12-bit integer representing `outputs[11:0]`

- chip.get_output(index)
	- Returns `outputs[index]`

- chip.set_input(index, value)
	- Sets `input[index] = value`

- chip.set_all_outputs(value)
	- Takes a 12-bit integer and sets `input[11:0] = value`

- chip.set_reset(value)
	- Sets the reset input of the chip to `value`

- chip.step_clock(n=1)
	- Step the chip's clock forward by N cycles

- chip.reset_sequence()
	- Resets the chip and steps the clock forward by 5 cycles to compensate for the synchronization delay in the reset.
```

Important note: if you are sensitive to speed, it is recommended to use a single invocation to `set_all_inputs` as it has better runtime performance than multiple invocations to `set_input`.

#### Example Code

See the example `.py` (Python scripts) and `.ipynb` (Jupyter notebooks - you will need to set up Jupyter locally) in [design_tests](design_tests/) for an example of how to use this interface.

You may need to `pip3 install pyserial` (important: it is `pyserial` and not `serial`) to get the dependency loaded.

### Method (2): Arduino Debugger Interface

If you need higher clock speed than the Python setup, you can also use the Arduino debugger, which can run at update rates upwards of 500kHz. In this case, you would write your code in Arduino C++ and flash it directly to the board. The board uses a bootloader based on that of the Adafruit Circuit Playground Express (SAMD21), so it will show up as such on the Arduino IDE.

#### Flashing the Arduino

You can flash the microcontroller using the Arduino IDE. It will appear to the IDE as a Circuit Playground. Avoid using the Arduino-style pin numbering - instead please use our `chip_interface` wrapper as it directly uses the microcontroller's I/O registers to feed inputs/outputs to your design.

#### Writing Code

We recommend making a copy of one of the example projects and building your code from that. Importantly, you need to make sure to `#include "chip_interface.h"` and call `init_portio(); step_clock(10);` at the start of your `setup()` function.

The following functions are provided:

```
- void init_portio()
     - configure the I/O config

- void set_status_led(bool val)
     - set the onboard status LED

- void set_input(int idx, bool val)
     - equivalent to `inputs[idx] = val`

- bool get_output(int idx)
     - equivalent to `return outputs[idx]`

- void set_inputs(uint32_t vals, int hi, int lo)
     - equivalent to `inputs[hi:lo] = vals` (hi/lo are both inclusive, like in Verilog)

- uint32_t get_outputs(int hi, int lo)
     - equivalent to `return outputs[hi:lo]` (hi/lo are both inclusive, like in Verilog)

- void set_reset(bool val)
     - set the chip's reset input

- void set_clock(bool val)
     - directly set the chip's clock input (don't use this function)

- void step_clock(bool val)
     - create one clock cycle with length 2us
 ``` 

#### Example Code

Some example programs are provided in [debugger_fw/d01_example_adder](debugger_fw/d01_example_adder), [debugger_fw/d02_example_counter](debugger_fw/d02_example_counter), and [debugger_fw/d05_meta_info](debugger_fw/d05_meta_info).

### Method (3): Switches/Buttons Board

#### Assembly

On the debugger board: Populate the 30-pin connector in the middle with a male header. Take care to ensure the header sits perpendicular to the board and is not skewed.

(image of header)

On the buttons/switches board: Populate two 3-pin male headers on the `IN[0] sel` and `IN[1] sel` footprints. Populate a 30-pin female header on the _bottom_ side, for connecting to the debugger board.

#### Disabling the debugger

**If you are driving the chip's inputs externally (i.e. not from the debugger), it is very important that the debugger be held in reset.** To do so, populate the 2-pin RESET header with a 2-pin male header. Insert a pin-jumper to keep the debugger in reset. You can verify it is disabled by making sure the USB device no longer enumerates on your computer (but it still receives power).

(image of reset header)

#### How to Use

- Set the jumper for in0 / in1 sel
- You must step the clock at least 5 times after powering on the chip so that the "project selection" register is updated.

### Method (4): Custom Circuit/Breadboard

#### Assembly

On the debugger board: Populate the 30-pin connector in the middle with a male header. Take care to ensure the header sits perpendicular to the board and is not skewed.

(image of header)

#### Disabling the debugger

**If you are driving the chip's inputs externally (i.e. not from the debugger), it is very important that the debugger be held in reset.** To do so, populate the 2-pin RESET header with a 2-pin male header. Insert a pin-jumper to keep the debugger in reset. You can verify it is disabled by making sure the USB device no longer enumerates on your computer (but it still receives power).

(image of reset header)

#### Usage

Varies based on your project. If you're using a single-stepped clock, you must step it at least 5 times after powering on the chip so that the "project selection" register is updated.


## Internal Documentation

[https://github.com/asinghani/18224-tapeout-s23-bringup](https://github.com/asinghani/18224-tapeout-s23-bringup)

[https://github.com/asinghani/18224-s23-tapeout](https://github.com/asinghani/18224-s23-tapeout)

[https://github.com/asinghani/18224-tapeout-s23-caravel](https://github.com/asinghani/18224-tapeout-s23-caravel)

<!--
- link to schematics
- link to kicad files
- link to multiplexer systemverilog
- link to assmebly guide
-->












