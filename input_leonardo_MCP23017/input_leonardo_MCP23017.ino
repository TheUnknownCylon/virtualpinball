/**
   Arduino Leonardo LEDWIZ controller
   ==================================

   Program an Arduino Leonardo to acts as a simple Joystick input device.
   Up to 16 inputs can be connected by using a MCP23017.

   License
   -------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.


   Future work:
    - Plunger
    - Accelerometer

   Credits
   -------
   This script is written by TheUC.
*/

#include "Wire.h"
#include <Joystick.h> //https://github.com/MHeironimus/ArduinoJoystickLibrary

/**


    MCP23017: 16 bit i/o extender. Can be used to control 16 GPIO devices
    per unit. Multiple units can be used by changing the address.

    Datasheet: http://ww1.microchip.com/downloads/en/DeviceDoc/21952b.pdf

    To use this hardware device most efficiently use a single bank for input
    only, or for output only (e.g. bank A input devices only, and bank B output
    defices only).

    Pin layout:
                     ____   _____
       GPB0 <--> 01  |   \_/    |  28 <--> GPA7
       GPB1 <--> 02  |          |  27 <--> GPA6
       GPB2 <--> 03  |          |  26 <--> GPA5
       GPB3 <--> 04  |    M     |  25 <--> GPA4
       GPB4 <--> 05  |    C     |  24 <--> GPA3
       GPB5 <--> 06  |    P     |  23 <--> GPA2
       GPB6 <--> 07  |    2     |  22 <--> GPA1
       GPB7 <--> 08  |    3     |  21 <--> GPA0
        VDD <--> 09  |    0     |  20  --> INTA
        VSS  --> 10  |    1     |  19  --> INTB
         NC  --  11  |    7     |  18  --> RESET
        SCL  --> 12  |          |  17 <--  A2
        SDA <--> 13  |          |  16 <--  A1
         NC  --  14  |__________|  15 <--  A0


    Connections:
     GPB*: Bank B input/output, where * is the nth-LSB bit in the address mask
     GPA*: Bank A input/output,  "

     VDD: 3.3V in (5.5V should work as well)
     VSS: GND
     SCL: I2C clock
     SDA: I2C data
     RESET: always set to high, same voltage as VDD
     A1,A1,A2: GND --> I2C address 0x20. Refer to datasheet for other configuration options.
*/
#define MCP23017_I2C_PORT 0x20

inline void mcp23017_write(uint8_t address, uint8_t value) {
  Wire.beginTransmission(MCP23017_I2C_PORT);
  Wire.write(address);
  Wire.write(value);
  Wire.endTransmission();
}

inline uint8_t mcp23017_read(uint8_t address) {
  Wire.beginTransmission(MCP23017_I2C_PORT);
  Wire.write(address);
  Wire.endTransmission();
  Wire.requestFrom(MCP23017_I2C_PORT, 1);
  return Wire.read();
}

// Create the Joystick
Joystick_ Joystick;

void setup() {
  // Initialize I2C for MCP23017
  Wire.begin();
  setupButtons();

  // Initialize Joystick Library
  Joystick.begin(false);
}

// ~~ BEGIN READ PHYSICAL BUTTONS   ~~~~~~~~~~~~~~~~~~~

// keep state of buttons since last iteration in the loop
// so that we can detect a button-state change in a next loop iteration
uint8_t buttonsLastStateA = 0x00;
uint8_t buttonsLastStateB = 0x00;
boolean changed = false;
uint8_t buttonsA;
uint8_t buttonsB;

void setupButtons() {
  // All input buttons will be connected to the MCP23017
  // Configure MCP23017: simply set both banks as input, configured as pullup
  mcp23017_write(0x0C, 0xFF); // configure bank A for pullup
  mcp23017_write(0x0D, 0xFF); // configure bank B for pullup

  mcp23017_write(0x00, 0xFF); // configure bank A as input
  mcp23017_write(0x01, 0xFF); // configure bank B as input
}

void updateButtons() {
  buttonsA = mcp23017_read(0x12);
  buttonsB = mcp23017_read(0x13);

  if (buttonsA != buttonsLastStateA) {
    changed = true;
    updateButton(0, 0, buttonsA, buttonsLastStateA);
    updateButton(1, 1, buttonsA, buttonsLastStateA);
    updateButton(2, 2, buttonsA, buttonsLastStateA);
    updateButton(3, 3, buttonsA, buttonsLastStateA);
    updateButton(4, 4, buttonsA, buttonsLastStateA);
    updateButton(5, 5, buttonsA, buttonsLastStateA);
    updateButton(6, 6, buttonsA, buttonsLastStateA);
    updateButton(7, 7, buttonsA, buttonsLastStateA);
  }

  if (buttonsB != buttonsLastStateB) {
    changed = true;
    updateButton(8, 0, buttonsB, buttonsLastStateB);
    updateButton(9, 1, buttonsB, buttonsLastStateB);
    updateButton(10, 2, buttonsB, buttonsLastStateB);
    updateButton(11, 3, buttonsB, buttonsLastStateB);
    updateButton(12, 4, buttonsB, buttonsLastStateB);
    updateButton(13, 5, buttonsB, buttonsLastStateB);
    updateButton(14, 6, buttonsB, buttonsLastStateB);
    updateButton(15, 7, buttonsB, buttonsLastStateB);
  }

  // processing done, update state for next loop
  // and update external components
  buttonsLastStateA = buttonsA;
  buttonsLastStateB = buttonsB;
  if (changed) {
    changed = false;
    Joystick.sendState();
  }
}

inline void updateButton(uint8_t joystickButton, uint8_t index, uint8_t newValue, uint8_t oldValue) {
  if ((newValue & (1 << index)) != (oldValue & 1 << index)) {
    Joystick.setButton(joystickButton, !(newValue & (1 << index)));
  }
}

// ~~ END READ PHYSICAL BUTTONS

void loop() {
  updateButtons();
}
