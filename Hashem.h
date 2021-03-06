#ifndef KeyboardIO_H_
#define KeyboardIO_H_
#include <Arduino.h>
//add your includes for the project KeyboardIO here


//end of add your includes here
#ifdef __cplusplus
extern "C" {
#endif
void loop();
void setup();
#ifdef __cplusplus
} // extern "C"
#endif

//add your function definitions for the project KeyboardIO here


#include <stdio.h>
#include <math.h>
#include <avr/wdt.h>
#include "key_defs.h"
#include "KeyboardConfig.h"

#include "debouncing.h"
#include "digitalWriteFast.h"

//extern int usbMaxPower;
#define DEBUG_SERIAL 0


byte physicalState[KEYS];

#define KEYS_HELD_BUFFER KEYS

byte charsBeingReported[KEYS_HELD_BUFFER]; // A bit vector for the 256 keys we might conceivably be holding down
byte charsReportedLastTime[KEYS_HELD_BUFFER]; // A bit vector for the 256 keys we might conceivably be holding down


long reporting_counter = 0;


// hardware keymap interaction

void scan_keys();

// key matrix
void setup_matrix();
void reset_matrix();


// sending events to the computer
void record_key_being_pressed(byte character);
void release_keys_not_being_pressed();
void reset_key_report();
void send_key_events();

void press_key(Key mappedKey);
void release_key(Key mappedKey);

#ifndef VERSION
#define VERSION "locally-built"
#endif

//Do not add code below this line
#endif /* KeyboardIO_H_ */
