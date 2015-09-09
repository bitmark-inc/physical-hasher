// Copyright Bitmark Inc. 2015-2015

#include <stdbool.h>
#include <cyu3system.h>
#include <cyu3types.h>
#include <cyu3os.h>
#include <cyu3dma.h>
#include <cyu3error.h>
#include <cyu3uart.h>
#include <cyu3i2c.h>
#include <cyu3spi.h>
#include <cyu3gpio.h>
#include <cyu3utils.h>
#include <cyu3mipicsi.h>

#include "focus.h"
#include "gpio.h"

#define DEBUG_ENABLE 0

#if !defined(SIZE_OF_ARRAY)
#define SIZE_OF_ARRAY(a) (sizeof(a)/sizeof((a)[0]))
#endif


#define FOCUS_LINE_LENGTH 100
#define DELTA_CONTRAST    200

// pixels are 16 bit little endian and need 1 extra pixel at front and back
#define MAXIMUM_PIXEL_BYTES ((FOCUS_LINE_LENGTH + 2) * 2)

// Event flags
#define EVENT_HOME    (1 << 0)
#define EVENT_PIXELS  (1 << 1)
#define EVENT_FRAME   (1 << 2)
#define EVENT_TIMER   (1 << 3)

#define EVENT_MASK (EVENT_HOME | EVENT_PIXELS | EVENT_FRAME | EVENT_TIMER)


// focus motor calibration parameters
#define TOP_LIMIT    120
#define BOTTOM_LIMIT 120
#define N_STEPS       10

// focus motor normal running
#define STEP_MINIMUM   0
#define STEP_MAXIMUM  60

// timer values
#define INITIAL_TICKS    2500
#define RESCHEDULE_TICKS   15

// process control
#define THREAD_STACK     0x2000
#define THREAD_PRIORITY  8
static CyU3PThread focus_thread;
static CyU3PEvent focus_event;
static CyU3PTimer focus_timer;

// motor state
static int current_position = 0;
static int required_position = 0;

// pixel buffers
// to capture the middle line, and a one pixel border all around
static uint8_t pre_pixels[MAXIMUM_PIXEL_BYTES];
static uint8_t pixels[MAXIMUM_PIXEL_BYTES];
static uint8_t post_pixels[MAXIMUM_PIXEL_BYTES];
// to hold the converted greyscale
static uint16_t grey[FOCUS_LINE_LENGTH];

typedef struct {
	int x;  // byte offset of "border" pixel
	int y;  // line offset
	bool is_green;
} StartPoint_t;

static const StartPoint_t start_1080 = {
	.x = (2 * 1920 - MAXIMUM_PIXEL_BYTES) / 2,
	.y = 1080 / 2,
	.is_green = false
};

static const StartPoint_t *current_start = &start_1080;


// home position
typedef enum {
	HOME_START,
	HOME_WAIT_HIGH,
	HOME_WAIT_N,
	HOME_WAIT_LOW,
	HOME_FAILED,
	HOME_SUCCESS
} HomeState_t;

static HomeState_t home_state = HOME_START;


// focus stages
typedef enum {
	FOCUS_IDLE,
	FOCUS_HOME,
	FOCUS_OUT,
	FOCUS_HOLD
} FocusState_t;

static FocusState_t focus_state;

// prototypes
static void focus_process(uint32_t input);
static void timer_callback(uint32_t input);
static void pixel_compensation(void);
static uint32_t pixel_contrast(void);
static HomeState_t focus_home(void);
static bool focus_step(void);
static bool photo_switch(void);


// API
// ---

void Focus_SetLine(const int32_t line, const uint8_t *buffer, const size_t buffer_length) {

	if (line == current_start->y - 1) {
		CyU3PMemCopy(pre_pixels, (uint8_t *)&buffer[current_start->x], MAXIMUM_PIXEL_BYTES);
	} else if (line == current_start->y) {
		CyU3PMemCopy(pixels, (uint8_t *)&buffer[current_start->x], MAXIMUM_PIXEL_BYTES);
	} else if (line == current_start->y + 1) {
		CyU3PMemCopy(post_pixels, (uint8_t *)&buffer[current_start->x], MAXIMUM_PIXEL_BYTES);

		// signal capture complete
		uint32_t rc = CyU3PEventSet(&focus_event, EVENT_PIXELS, CYU3P_EVENT_OR);
		if (CY_U3P_SUCCESS != rc) {
			CyU3PDebugPrint(4, "Focus_Setline: EventSet error = %d 0x%x\r\n", rc, rc);
		}
	}
}


void Focus_EndFrame(const int32_t line) {
	uint32_t rc = CyU3PEventSet(&focus_event, EVENT_FRAME, CYU3P_EVENT_OR);
	if (CY_U3P_SUCCESS != rc) {
		CyU3PDebugPrint(4, "Focus_EndFrame: EventSet error = %d 0x%x\r\n", rc, rc);
	}
}


bool Focus_Initialise(void) {
	current_position = 0;
	required_position = 0;

	CyU3PDebugPrint(4, "Focus_Initialise\r\n");

	CyU3PReturnStatus_t status = CyU3PSpiInit();
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "Focus_Initialise: SPI init error = %d 0x%x\r\n", status, status);
		return false;
	}

	// cpha:cpol  0:0=mode0 .. 1:1=mode3
	CyU3PSpiConfig_t SPI_config = {
		.isLsbFirst = CyFalse,
		.cpha = CyTrue,         // Slave samples: Low→high
		.cpol = CyTrue,         // SCK idle: high
		.ssnPol = CyFalse,      // SSN is active low
		.ssnCtrl = CY_U3P_SPI_SSN_CTRL_HW_EACH_WORD,
		.leadTime = CY_U3P_SPI_SSN_LAG_LEAD_HALF_CLK,  // time between SSN assertion and first SCLK edge
		.lagTime  = CY_U3P_SPI_SSN_LAG_LEAD_HALF_CLK,  // time between the last SCK edge to SSN de-assertion
		.clock = 1500000,       // clock frequency in Hz
		.wordLen = 8            // bits
	};
	status = CyU3PSpiSetConfig(&SPI_config, NULL);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "Focus_Initialise: SPI set config error = %d 0x%x\r\n", status, status);
		return false;
	}

	// enable the stepper driver chip
	status = CyU3PGpioSetValue(MOTOR_DRIVER_EN, CyTrue);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "Focus_Initialise: enable motor driver error = %d 0x%x\r\n", status, status);
		return false;
	}
	CyU3PThreadSleep(50);
	status = CyU3PGpioSetValue(MOTOR_DRIVER_EN, CyFalse);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "Focus_Initialise: enable motor driver error = %d 0x%x\r\n", status, status);
		return false;
	}
	CyU3PSpiDisableBlockXfer(CyTrue, CyTrue);
	CyU3PThreadSleep(5);


	// Allocate the memory for the thread and create the thread
	uint8_t *stack = CyU3PMemAlloc(THREAD_STACK);
	uint32_t rc = CyU3PThreadCreate(&focus_thread,        // UVC Thread structure
					"thread_30:focus",    // Thread Id and name
					focus_process,        // UVC Application Thread Entry function
					0,                    // No input parameter to thread
					stack,                // Pointer to the allocated thread stack
					THREAD_STACK,         // UVC Application Thread stack size
					THREAD_PRIORITY,      // UVC Application Thread priority
					THREAD_PRIORITY,      // Pre-emption threshold
					CYU3P_NO_TIME_SLICE,  // No time slice for the application thread
					CYU3P_AUTO_START      // Start the Thread immediately
		);

	if (CY_U3P_SUCCESS != rc) {
		CyU3PDebugPrint(4, "Focus_Initialise: create thread error = %d 0x%x\r\n", rc, rc);
		return false;
	}

	rc = CyU3PEventCreate(&focus_event);
	if (CY_U3P_SUCCESS != rc) {
		CyU3PDebugPrint(4, "Focus_Initialise: create event error = %d 0x%x\r\n", rc, rc);
		return false;
	}

	rc = CyU3PTimerCreate(&focus_timer, timer_callback, 0, INITIAL_TICKS, RESCHEDULE_TICKS, CYU3P_AUTO_ACTIVATE);
	if (CY_U3P_SUCCESS != rc) {
		CyU3PDebugPrint(4, "Focus_Initialise: create time error = %d 0x%x\r\n", rc, rc);
		return false;
	}

	return true;
}


// local functions
// ---------------


// Entry function for the UVC application thread.
static void focus_process(uint32_t input) {

	CyU3PDebugPrint(4, "focus_process\r\n");
	//focus_state = FOCUS_IDLE;
	focus_state = FOCUS_HOME;
	home_state = HOME_START;
	//uint32_t maximum_contrast = 0;
	uint32_t current_contrast = 0;
	//uint32_t focus_position = 0;

	int nnn = 0;

	for (;;) {
		uint32_t event = 0;
		CyU3PEventGet(&focus_event, EVENT_MASK, CYU3P_EVENT_OR_CLEAR, &event, CYU3P_WAIT_FOREVER);

		if (0 != (event & EVENT_HOME)) {
			if (FOCUS_HOME != focus_state) {
				//maximum_contrast = 0;
				//current_contrast = 0;
				focus_state = FOCUS_HOME;
				home_state = HOME_START;
			}

		} else if (event & EVENT_PIXELS) {
			if (FOCUS_HOME != focus_state) {
				pixel_compensation();
				current_contrast = pixel_contrast();
				CyU3PDebugPrint(4, "focus contrast = %d\r\n", current_contrast);
			}

		} else if (event & EVENT_FRAME) {

		} else if (event & EVENT_TIMER) {
			focus_step();

			switch (focus_state) {
			case FOCUS_IDLE:
				break;

			case FOCUS_HOME:
				switch (focus_home()) {
				case HOME_FAILED:
					CyU3PDebugPrint(4, "focus_home failed\r\n");
					focus_state = FOCUS_IDLE;
					break;
				case HOME_SUCCESS:
					CyU3PDebugPrint(4, "focus_home success\r\n");
					focus_state = FOCUS_OUT;
					nnn = 0;
					break;
				default:
					break;
				}
				break;

			case FOCUS_OUT:
				++nnn;
				if (nnn > 250) {
					CyU3PDebugPrint(4, "focus_hold\r\n");
					focus_state = FOCUS_HOLD;
				}

#if 0
				++required_position;
				if (current_contrast > maximum_contrast) {
					maximum_contrast = current_contrast;
					focus_position = current_position;
				} else if (current_contrast + DELTA_CONTRAST < maximum_contrast) {
					focus_state = FOCUS_HOLD;
					required_position = focus_position;
				}
#endif
				break;

			case FOCUS_HOLD:
				// just for a test
				if (required_position == current_position) {
					if (0 == required_position) {
						required_position = 60;
					} else {
						required_position = 0;
					}
				}
				break;
			}
		}
	}
}


// Entry function for the UVC application thread.
static void timer_callback(uint32_t input) {
	//CyU3PDebugPrint(4, "timer_callback: input: %x\r\n", input);
	uint32_t rc = CyU3PEventSet(&focus_event, EVENT_TIMER, CYU3P_EVENT_OR);
	if (CY_U3P_SUCCESS != rc) {
		CyU3PDebugPrint(4, "timer_callback: EventSet  error = %d 0x%x\r\n", rc, rc);
	}
}

static void pixel_compensation(void) {

	bool is_green = current_start->is_green;

	for (int i = 0; i < SIZE_OF_ARRAY(grey); ++i) {
		int byte_offset = 2 * i + 2;
		if (is_green) {
			uint16_t n = pre_pixels[byte_offset] | (pre_pixels[byte_offset + 1] << 8);
			uint16_t w = pixels[byte_offset - 2] | (pixels[byte_offset - 1] << 8);
			uint16_t c = pixels[byte_offset] | (pixels[byte_offset + 1] << 8);
			uint16_t e = pixels[byte_offset + 2] | (pixels[byte_offset + 3] << 8);
			uint16_t s = post_pixels[byte_offset] | (post_pixels[byte_offset + 1] << 8);
			grey[i] = (n + s) / 2 + (w + e) / 2 + c;
		} else {
			uint16_t nw = pre_pixels[byte_offset - 2] | (pre_pixels[byte_offset - 1] << 8);
			uint16_t n  = pre_pixels[byte_offset] | (pre_pixels[byte_offset + 1] << 8);
			uint16_t ne = pre_pixels[byte_offset + 2] | (pre_pixels[byte_offset + 3] << 8);
			uint16_t w  = pixels[byte_offset - 2] | (pixels[byte_offset - 1] << 8);
			uint16_t c  = pixels[byte_offset] | (pixels[byte_offset + 1] << 8);
			uint16_t e  = pixels[byte_offset + 2] | (pixels[byte_offset + 3] << 8);
			uint16_t sw = post_pixels[byte_offset - 2] | (post_pixels[byte_offset - 1] << 8);
			uint16_t s  = post_pixels[byte_offset] | (post_pixels[byte_offset + 1] << 8);
			uint16_t se = post_pixels[byte_offset + 2] | (post_pixels[byte_offset + 3] << 8);
			grey[i] = (nw + ne + sw + se) / 4 + (n + w + e + s) / 4 + c;
		}
		is_green = !is_green;
	}
}


static uint32_t pixel_contrast(void) {
	uint32_t contrast = 0;
	for (int i = 1; i < SIZE_OF_ARRAY(grey); ++i) {
		if (grey[i -1] < grey[i]) {
			contrast += grey[i] - grey[i - 1];
		} else {
			contrast += grey[i - 1] - grey[i];
		}
	}
	return contrast;
}


// process to locate the home position
static HomeState_t focus_home(void) {

	static HomeState_t olds = -1;
	if (olds != home_state) {
		CyU3PDebugPrint(4, "focus_home: new state: %d\r\n", home_state);
		olds = home_state;
	}
	switch (home_state) {
	case HOME_START:
		current_position = BOTTOM_LIMIT;
		required_position = 0;
		home_state = HOME_WAIT_HIGH;
		// enable the stepper driver chip
		CyU3PReturnStatus_t status = CyU3PGpioSetValue(MOTOR_DRIVER_EN, CyFalse);
		if (status != CY_U3P_SUCCESS) {
			CyU3PDebugPrint (4, "focus_home: enable motor driver error = %d 0x%x\r\n", status, status);
		}
		CyU3PThreadSleep(5);
		break;

	case HOME_WAIT_HIGH:
		if (photo_switch()) {
			home_state = HOME_WAIT_N;
			required_position = 0;
			current_position = N_STEPS;
		} else if (required_position == current_position) {
			home_state = HOME_FAILED;
		}
		break;

	case HOME_WAIT_N:
		if (required_position == current_position) {
			current_position = 0;
			required_position = TOP_LIMIT;
			home_state = HOME_WAIT_LOW;
		}
		break;

	case HOME_WAIT_LOW:
		if (!photo_switch()) {
			current_position = 0;
			required_position = 0;
			home_state = HOME_SUCCESS;
		} else if (required_position == current_position) {
			home_state = HOME_FAILED;
		}
		break;

	default:
		break;
	}
	return home_state;
}


// returns true at hold_position matched
static bool focus_step(void) {
	//CyU3PDebugPrint(4, "focus_step: current_position: %d\r\n", current_position);

#if 0
	const uint8_t half_step[] = {
		037, 037, 037,
		033, 073, 023, 027,  022, 062, 032, 036,
		033, 073, 023, 027,  022, 062, 032, 036
	};
#endif

	// set new position demand, only if last position was reached
	if (current_position == required_position) {
		return true;
	}

	// stepper patterns
	const uint8_t full_step[] = {
#if 1
		//0354, 0344, 0345, 0355  // low
		0323, 0322, 0332, 0333  // medium
		//0310, 0300, 0301, 0311  // high
#else
		//0355, 0345, 0344, 0354  // low
		0333, 0332, 0322, 0323  // medium
		//0311, 0301, 0300, 0310,  // high
#endif
	};

	// the current position only changes when the state value transitions to 1
	if (current_position < required_position) {
		++current_position;
	} else {
		--current_position;
	}
	int step_state = current_position % sizeof(full_step);
	CyU3PSpiTransmitWords((uint8_t *)&full_step[step_state], 1);

	return false;
}

// this is to detect if the motor is in its home position
static bool photo_switch(void) {
	CyBool_t value = CyFalse;
	CyU3PReturnStatus_t rc = CyU3PGpioSimpleGetValue(FOCUS_POSITION, &value);
	return (CY_U3P_SUCCESS == rc) && (CyTrue == value);
}
