// Copyright Bitmark Inc. 2015-2015

#ifndef _GPIO_H_
#define _GPIO_H_ 1


// LED driver GPIO
#define LED_DRIVER_CLK 19
#define LED_DRIVER_SDI 20
#define LED_DRIVER_SDO 21
#define LED_DRIVER_ED1 25
#define LED_DRIVER_ED2 22

// Focus Motor GPIO
#define MOTOR_DRIVER_EN 26
#define FOCUS_POSITION  17

// GPIO configuration - include all GPIOS here
// with one of: &anOutputInitiallyLow, &anOutputInitiallyHigh, &anInput
#define GPIO_SETUP_BLOCK {                                      \
		{LED_DRIVER_CLK,  &anOutputInitiallyLow},       \
		{LED_DRIVER_SDI,  &anOutputInitiallyLow},       \
		{LED_DRIVER_SDO,  &anInput},                    \
		{LED_DRIVER_ED2,  &anOutputInitiallyHigh},      \
		{LED_DRIVER_ED1,  &anOutputInitiallyLow},       \
		{MOTOR_DRIVER_EN, &anOutputInitiallyHigh},      \
		{FOCUS_POSITION,  &anInput},                    \
		{ 0, NULL} /* end of list */                    \
	}

#endif
