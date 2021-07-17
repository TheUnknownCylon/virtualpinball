/**
 * Arduino Leonardo LEDWIZ controller
 * ==================================
 * 
 * Program an Arduino Leonardo to acts as a LEDWIZ controller.
 * Actual LEDS (or other devices) are controlled by a TLC5940.
 * 
 * License
 * -------
 * 
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 *  
 * Instructions
 * ------------
 * This sketch depends on the following libraries:
 * 1. Tlc5940 0.15.0 by Paul Stoffregen
 * 2. HID-Project 2.6.1 by NicoHood
 * 
 * Please make sure that they are available in your development environment (e.g. install
 * in Arduino IDE)
 * 
 * To get the Leonardo detected as a real ledwiz device, the following
 * steps have to be taken as well:
 * 
 * 1. in boards.txt, set leonardo.build.vid=0xFAFA
 * 2. in boards.txt, set leonardo.build.pid=0x00F0
 *    (or increase this value with n-1 if you want this device to be detected as ledwiz n)
 * 4. patch HID-project, in src/SingleReport/RawHID.cpp replace RAWHID_TX_SIZE and RAWHID_RX_SIZE 
 *    by 0x08
 * 
 * (typical location for boards.txt is ~/.arduino15/packages/arduino/hardware/avr/yourversion)
 * (typical location of the HID-project code ~/Arduino15/libraries/HID-Project)
 * 
 * When the above steps are applied, the code is ready to be compiled and uploaded
 * to the Arduino Leonardo. This can be done, for example, by using the Arduino IDE.
 * 
 * Wire diagram
 * ------------
 * 
 * 
 * Credits
 * -------
 * This script is written by TheUC. However, this script is based on all the work
 * that mjr has done for the Pinscape controller. For reference, please have a look at
 * https://os.mbed.com/users/mjr/code/Pinscape_Controller_V2//file/310ac82cbbee/USBProtocol.h/
 * and
 * https://os.mbed.com/users/mjr/code/Pinscape_Controller_V2//file/310ac82cbbee/main.cpp/
 * 
 * 
 * License
 * -------
 * This software is licensed under 
 */

#include "HID-Project.h"
#include "Tlc5940.h"

void setup() {
  // some unfortunate startup delay is required
  // or else the board shows unexpected behaviour
  delay(1000);
  
  Tlc.init();
  Tlc.clear();
  Tlc.update();

  setupLW();
  pulse();
}

// ----------------
// - LedWiz (LW) protocol handling
// ----------------
uint8_t rawhidData[255];

#define PULSE_INTERVAL 8 //256 pulse frames per 2 seconds -> ~8 ms / frame

// true if there are devices that have a pulsing pattern
bool pulse_enabled = false;

// pulse 'animation' settings
uint8_t pulse_frame = 0;
uint8_t pulse_speed = 2;

// (32) bit map to indicate the state of a device (on/off)
long device_state = 0b111;

// array holding the value of each device
//  this value is only applied when the according device state is on
uint8_t device_pattern[32] = {
  129, 129, 129, 48, 48, 48, 48, 48,
  48, 48, 48, 48, 48, 48, 48, 48,
  48, 48, 48, 48, 48, 48, 48, 48,
  48, 48, 48, 48, 48, 48, 48, 48
};

// Lookup table to map LEDWIZ intensity values to 
//  so mapping of [0,48] to [0,4095]
// Added element 49 as a duplicate of 48 as per ledWiz.
static uint16_t pwm_table[50] = {
     0,   85,  171,  256,  341,  427,  512,  597,  682,  768,
   853,  938, 1024, 1109, 1194, 1280, 1365, 1450, 1536, 1621,
  1706, 1792, 1877, 1962, 2048, 2133, 2218, 2303, 2389, 2474,
  2559, 2645, 2730, 2815, 2901, 2986, 3071, 3157, 3242, 3327,
  3412, 3498, 3583, 3668, 3754, 3839, 3924, 4010, 4095, 4095
};

void setupLW() {
  RawHID.begin(rawhidData, sizeof(rawhidData));
}

// FUNCTION 
//   hansleLW()
int command_index;
uint8_t command[8];
int bytesAvailable = 0;
void handleLW() {
  bytesAvailable = RawHID.available();
  if(bytesAvailable > 7) {
    for(command_index = 0; command_index < 8; command_index++) {
      command[command_index] = RawHID.read();
    }
    parseCommand();
  }
}


// FUNCTION 
//   parseCommand()
uint8_t deviceBlockIndex;
uint8_t counter;
void parseCommand() {
  if(command[0] == 0x40) {
    device_state = 0 | command[4];
    device_state = device_state << 8 | command[3];
    device_state = device_state << 8 | command[2];
    device_state = device_state << 8 | command[1];
    pulse_speed = command[5];
    deviceBlockIndex = 0;

  } else if(deviceBlockIndex < 32) {
    //memcpy(&device_pattern[deviceBlockIndex], command, 8);
    for(counter = 0; counter < 8; counter++) {
      device_pattern[deviceBlockIndex] = command[counter];
      if(129 <= command[counter] && command[counter] <= 132) {
        pulse_enabled = true;
      }
      deviceBlockIndex+=1;
    }
    if(deviceBlockIndex == 32) {
      pulse();
    }
  } else {
    // unexpected message
  }
}


// FUNCTION 
//   pulse()
uint8_t pulseDevIndex;
void pulse() {
  pulse_frame += pulse_speed;
  for(pulseDevIndex = 0; pulseDevIndex < 32; pulseDevIndex++) {
    pulseDevice(pulseDevIndex);
  }
  Tlc.update();
}

void pulseDevice(uint8_t deviceId) {
  if(!(device_state & (1 << deviceId))) {
    Tlc.set(deviceId, 0);
    
  } else if(device_pattern[deviceId] <= 49) {
    Tlc.set(deviceId, pwm_table[device_pattern[deviceId]]);
    
  } else if(device_pattern[deviceId] == 129) {
    // ramp up (1st half of flashFrames), ramp down (2nd half of flash frames)
    Tlc.set(deviceId, (pulse_frame < 128 ? pulse_frame : 255-pulse_frame) << 5);

  } else if(device_pattern[deviceId] == 130) {
    // flash, 1st half on, 2nd half off
    Tlc.set(deviceId, pulse_frame < 128 ? 4095 : 0);
    
  } else if(device_pattern[deviceId] == 131) {
    // 1st half on, 2nd half ramp down
    Tlc.set(deviceId, pulse_frame < 128 ? 4095 : ((255-pulse_frame) << 5));
    
  } else if(device_pattern[deviceId] == 132) {
    // 1st half ramp up, 2nd half on
    Tlc.set(deviceId, pulse_frame < 128 ? (pulse_frame << 5) : 4095);    
  }
}


// ----------------
// - Main Loop
// ----------------
unsigned long previousPulseMillis = 0;
unsigned long currentMillis;

void loop() {
  handleLW();

  if(true) {
    currentMillis = millis();
    if(currentMillis - previousPulseMillis > PULSE_INTERVAL) {
      previousPulseMillis = currentMillis;
      pulse();
    }
  }
}
