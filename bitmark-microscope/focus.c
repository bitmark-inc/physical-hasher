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
#include "macros.h"


#define FOCUS_LINE_LENGTH 100
#define DELTA_CONTRAST    200

// pixels are 16 bit little endian and need 1 extra pixel at front and back
#define MAXIMUM_PIXEL_BYTES ((FOCUS_LINE_LENGTH + 2) * 2)

// Event flags
#define EVENT_ABORT   (1 << 0)
#define EVENT_HOME    (1 << 1)
#define EVENT_PIXELS  (1 << 2)
#define EVENT_FRAME   (1 << 3)

#define EVENT_MASK (EVENT_ABORT | EVENT_HOME | EVENT_PIXELS | EVENT_FRAME)


// focus motor calibration parameters
#define TOP_LIMIT    120
#define BOTTOM_LIMIT 120
#define N_STEPS       10

// focus motor normal running
#define STEP_MINIMUM   0
#define STEP_MAXIMUM  60
#define STEP_MATCH    required_position == current_position

// process control
#define THREAD_STACK     0x2000
#define THREAD_PRIORITY  8
static CyU3PThread focus_thread;
static CyU3PEvent focus_event;

// motor state
static int current_position = 0;
static int required_position = 0;

// pixel buffers
// to capture the middle line, and a one pixel border all around
//(aligned to RED so first usable pixel is GREEN(on red line) (same as first frame pixel)
static uint8_t pre_pixels[MAXIMUM_PIXEL_BYTES];   // gbgb...
static uint8_t pixels[MAXIMUM_PIXEL_BYTES];       // RGRG...
static uint8_t post_pixels[MAXIMUM_PIXEL_BYTES];  // gbgb...
// to hold the converted greyscale
static uint16_t grey[FOCUS_LINE_LENGTH];

typedef struct {
	struct {
		int begin;
		int end;
	} offsets[3];  // byte offset of "border" pixel
	bool is_green;
} StartPoint_t;

#define LINE_BYTES   (2 * 1920)
#define CENTRE_LINE   540
#define LINE_OFFSET  ((LINE_BYTES - MAXIMUM_PIXEL_BYTES) / 2)
#define BEGIN_OFFSET(offset) ((CENTRE_LINE - (offset)) * LINE_BYTES + LINE_OFFSET)
#define END_OFFSET(offset) (BEGIN_OFFSET((offset)) + MAXIMUM_PIXEL_BYTES)

static const StartPoint_t start_1080 = {
	.offsets = {
		{.begin = BEGIN_OFFSET(-1), .end = END_OFFSET(-1)},
		{.begin = BEGIN_OFFSET(0),  .end = END_OFFSET(0)},
		{.begin = BEGIN_OFFSET(1),  .end = END_OFFSET(1)}
	},
	.is_green = true // == value of "second" pixel in pixels
};

static const StartPoint_t *current_start = &start_1080;


// home position
typedef enum {
	HOME_IDLE,
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
static void pixel_compensation(void);
static uint32_t pixel_contrast(void);
static HomeState_t seek_home(void);
static bool focus_step(void);
static bool photo_switch(void);
static void motor_on(void);
static void motor_off(void);


// API
// ---


// stop the focusing process immediately
void Focus_Stop(void) {
	uint32_t rc = CyU3PEventSet(&focus_event, EVENT_ABORT, CYU3P_EVENT_OR);
	if (CY_U3P_SUCCESS != rc) {
		CyU3PDebugPrint(4, "Focus_Stop: EventSet error: %d 0x%x\r\n", rc, rc);
	}
}


// start an autofocus cycle
void Focus_Start(void) {
	uint32_t rc = CyU3PEventSet(&focus_event, EVENT_HOME, CYU3P_EVENT_OR);
	if (CY_U3P_SUCCESS != rc) {
		CyU3PDebugPrint(4, "Focus_Home: EventSet error: %d 0x%x\r\n", rc, rc);
	}
}


// keep track of the current data range in the line buffer
// this buffer is not synchronised with the image lines
static int buffer_begin = 0;
static int buffer_end   = 0;
void Focus_SetLine(const int32_t buffer_number, const uint8_t *buffer, const size_t buffer_length) {

	if (0 == buffer_number) {
		buffer_begin = 0;
		buffer_end = buffer_length;
	} else {
		buffer_begin += buffer_length;
		buffer_end += buffer_length;
	}

	if (current_start->offsets[0].begin >= buffer_begin && current_start->offsets[0].begin < buffer_end) {
		// have some/all of the data
		size_t length = buffer_end - current_start->offsets[0].begin;
		if (length > MAXIMUM_PIXEL_BYTES) {
			length = MAXIMUM_PIXEL_BYTES;
		}
		size_t offset = current_start->offsets[0].begin - buffer_begin;
		CyU3PMemCopy(pre_pixels, (uint8_t *)&buffer[offset], length);

	} else if (current_start->offsets[0].end > buffer_begin && current_start->offsets[0].end <= buffer_end) {
		// have trailing bytes of data
		size_t length = current_start->offsets[0].end - buffer_begin;
		if (length > MAXIMUM_PIXEL_BYTES) {
			length = MAXIMUM_PIXEL_BYTES;
		}
		size_t offset = MAXIMUM_PIXEL_BYTES - length;
		CyU3PMemCopy(&pre_pixels[offset], (uint8_t *)buffer, length);

	}

	if (current_start->offsets[1].begin >= buffer_begin && current_start->offsets[1].begin < buffer_end) {
		// have some/all of the data
		size_t length = buffer_end - current_start->offsets[1].begin;
		if (length > MAXIMUM_PIXEL_BYTES) {
			length = MAXIMUM_PIXEL_BYTES;
		}
		size_t offset = current_start->offsets[1].begin - buffer_begin;
		CyU3PMemCopy(pixels, (uint8_t *)&buffer[offset], length);

	} else if (current_start->offsets[1].end > buffer_begin && current_start->offsets[1].end <= buffer_end) {
		// have trailing bytes of data
		size_t length = current_start->offsets[1].end - buffer_begin;
		if (length > MAXIMUM_PIXEL_BYTES) {
			length = MAXIMUM_PIXEL_BYTES;
		}
		size_t offset = MAXIMUM_PIXEL_BYTES - length;
		CyU3PMemCopy(&pixels[offset], (uint8_t *)buffer, length);

	}

	if (current_start->offsets[2].begin >= buffer_begin && current_start->offsets[2].begin < buffer_end) {
		// have some/all of the data
		size_t length = buffer_end - current_start->offsets[2].begin;
		if (length > MAXIMUM_PIXEL_BYTES) {
			length = MAXIMUM_PIXEL_BYTES;
		}
		size_t offset = current_start->offsets[2].begin - buffer_begin;
		CyU3PMemCopy(post_pixels, (uint8_t *)&buffer[offset], length);

		if (MAXIMUM_PIXEL_BYTES == length) {
			// signal capture complete
			uint32_t rc = CyU3PEventSet(&focus_event, EVENT_PIXELS, CYU3P_EVENT_OR);
			if (CY_U3P_SUCCESS != rc) {
				CyU3PDebugPrint(4, "Focus_Setline: EventSet error: %d 0x%x\r\n", rc, rc);
			}
		}

	} else if (current_start->offsets[2].end > buffer_begin && current_start->offsets[2].end <= buffer_end) {
		// have trailing bytes of data
		size_t length = current_start->offsets[2].end - buffer_begin;
		if (length > MAXIMUM_PIXEL_BYTES) {
			length = MAXIMUM_PIXEL_BYTES;
		}
		size_t offset = MAXIMUM_PIXEL_BYTES - length;
		CyU3PMemCopy(&post_pixels[offset], (uint8_t *)buffer, length);

		// signal capture complete
		uint32_t rc = CyU3PEventSet(&focus_event, EVENT_PIXELS, CYU3P_EVENT_OR);
		if (CY_U3P_SUCCESS != rc) {
			CyU3PDebugPrint(4, "Focus_Setline: EventSet error: %d 0x%x\r\n", rc, rc);
		}
	}
}


void Focus_EndFrame(const int32_t buffer_number) {
	uint32_t rc = CyU3PEventSet(&focus_event, EVENT_FRAME, CYU3P_EVENT_OR);
	if (CY_U3P_SUCCESS != rc) {
		CyU3PDebugPrint(4, "Focus_EndFrame: EventSet error: %d 0x%x\r\n", rc, rc);
	}
}


bool Focus_Initialise(void) {
	current_position = STEP_MINIMUM;
	required_position = STEP_MINIMUM;

	CyU3PDebugPrint(4, "Focus_Initialise\r\n");

	CyU3PReturnStatus_t status = CyU3PSpiInit();
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "Focus_Initialise: SPI init error: %d 0x%x\r\n", status, status);
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
		CyU3PDebugPrint(4, "Focus_Initialise: SPI set config error: %d 0x%x\r\n", status, status);
		return false;
	}

	CyU3PSpiDisableBlockXfer(CyTrue, CyTrue);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "Focus_Initialise: disable block transfer error: %d 0x%x\r\n", status, status);
		return false;
	}

	// Allocate the memory for the thread and create the thread
	uint8_t *stack = CyU3PMemAlloc(THREAD_STACK);
	uint32_t rc = CyU3PThreadCreate(&focus_thread,        // UVC Thread structure
					"40:focus",           // Thread Id and name
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
		CyU3PDebugPrint(4, "Focus_Initialise: create thread error: %d 0x%x\r\n", rc, rc);
		return false;
	}

	rc = CyU3PEventCreate(&focus_event);
	if (CY_U3P_SUCCESS != rc) {
		CyU3PDebugPrint(4, "Focus_Initialise: create event error: %d 0x%x\r\n", rc, rc);
		return false;
	}

	return true;
}


// local functions
// ---------------


// Entry function for the UVC application thread.
static void focus_process(uint32_t input) {

	CyU3PDebugPrint(4, "focus_process\r\n");
	focus_state = FOCUS_IDLE;
	home_state = HOME_IDLE;
	uint32_t maximum_contrast = 0;
	uint32_t current_contrast = 0;
	uint32_t focus_position = STEP_MINIMUM;

#define FRAME_DIV_1 0x00
#define FRAME_DIV_2 0x01
#define FRAME_DIV_4 0x03
#define FRAME_DIV_8 0x07

	// this is used to step on a multiple of frame_count
	uint8_t frame_modulus_count = 0;
	uint8_t frame_modulus_mask = FRAME_DIV_1;

	for (;;) {
		uint32_t event = 0;
		CyU3PEventGet(&focus_event, EVENT_MASK, CYU3P_EVENT_OR_CLEAR, &event, CYU3P_WAIT_FOREVER);

		if (0 != (event & EVENT_ABORT)) {
			maximum_contrast = 0;
			current_contrast = 0;
			focus_state = FOCUS_IDLE;
			home_state = HOME_IDLE;
			motor_off();

		} else if (0 != (event & EVENT_HOME)) {
			if (FOCUS_HOME != focus_state) {
				maximum_contrast = 0;
				current_contrast = 0;
				current_position = STEP_MINIMUM;
				required_position = STEP_MINIMUM;
				frame_modulus_count = 0;
				frame_modulus_mask = FRAME_DIV_1;
				focus_state = FOCUS_HOME;
				home_state = HOME_START;
			}

		} else if (event & EVENT_PIXELS) {
			if (FOCUS_HOME != focus_state) {
				pixel_compensation();
				uint32_t c = pixel_contrast();
				if (c != current_contrast) {
					current_contrast = c;
					CyU3PDebugPrint(4, "focus contrast = %d\r\n", current_contrast);
				}
			}

		} else if (event & EVENT_FRAME) {

#if 0
			// debug
			CyU3PDebugPrint(4, "FS-: %x %x %x %x\r\n", pre_pixels[0], pre_pixels[1], pre_pixels[2], pre_pixels[3]);
			CyU3PDebugPrint(4, "FS0: %x %x %x %x\r\n", pixels[0], pixels[1], pixels[2], pixels[3]);
			CyU3PDebugPrint(4, "FS+: %x %x %x %x\r\n", post_pixels[0], post_pixels[1], post_pixels[2], post_pixels[3]);
			pixel_compensation();
			uint32_t c = pixel_contrast();
			CyU3PDebugPrint(4, "contrast = %d\r\n", c);
#endif


			++frame_modulus_count;
			if (0 != (frame_modulus_count & frame_modulus_mask)) {
				continue;
			}

			focus_step();

			switch (focus_state) {
			case FOCUS_IDLE:
				break;

			case FOCUS_HOME:
				switch (seek_home()) {
				case HOME_FAILED:
					CyU3PDebugPrint(4, "focus_home failed\r\n");
					focus_state = FOCUS_IDLE;
					motor_off();
					break;

				case HOME_SUCCESS:
					CyU3PDebugPrint(4, "focus_home success\r\n");
					focus_state = FOCUS_OUT;
					frame_modulus_mask = FRAME_DIV_4;
					required_position = STEP_MAXIMUM;
					focus_position = STEP_MINIMUM;
				break;

				default:
					break;
				}
				break;

			case FOCUS_OUT:
				if (STEP_MATCH) {
					required_position = focus_position;
					focus_state = FOCUS_HOLD;
					CyU3PDebugPrint(4, "focus_hold: %d\r\n", required_position);
				} else {
					if (current_contrast > maximum_contrast) {
						maximum_contrast = current_contrast;
						focus_position = current_position;
					}
				}
				break;

			case FOCUS_HOLD:
#if 0
				// just for a test
				if (STEP_MATCH) {
					if (STEP_MINIMUM == required_position) {
						required_position = STEP_MAXIMUM;
					} else {
						required_position = STEP_MINIMUM;
					}
					CyU3PDebugPrint(4, "focus_hold: %d\r\n", required_position);

				}
#endif
				break;
			}
		}
	}
}


static void pixel_compensation(void) {

	bool is_green = current_start->is_green;

	for (int i = 0; i < SIZE_OF_ARRAY(grey); ++i) {
		int byte_offset = 2 * i + 2;  // to start at second pixel
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
static HomeState_t seek_home(void) {

	static HomeState_t olds = HOME_IDLE;
	if (olds != home_state) {
		CyU3PDebugPrint(4, "focus_home: new state: %d\r\n", home_state);
		olds = home_state;
	}
	switch (home_state) {
	case HOME_IDLE:
		break;

	case HOME_START:
		current_position = BOTTOM_LIMIT;
		required_position = STEP_MINIMUM;
		home_state = HOME_WAIT_HIGH;
		motor_on();
		break;

	case HOME_WAIT_HIGH:
		if (photo_switch()) {
			home_state = HOME_WAIT_N;
			required_position = STEP_MINIMUM;
			current_position = N_STEPS;
		} else if (STEP_MATCH) {
			home_state = HOME_FAILED;
		}
		break;

	case HOME_WAIT_N:
		if (STEP_MATCH) {
			current_position = STEP_MINIMUM;
			required_position = TOP_LIMIT;
			home_state = HOME_WAIT_LOW;
		}
		break;

	case HOME_WAIT_LOW:
		if (!photo_switch()) {
			current_position = STEP_MINIMUM;
			required_position = STEP_MINIMUM;
			home_state = HOME_SUCCESS;
		} else if (STEP_MATCH) {
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

	// set new position demand, only if last position was reached
	if (STEP_MATCH) {
		return true;
	}

	// stepper patterns
	const uint8_t full_step[] = {
		//0354, 0344, 0345, 0355  // low
		0323, 0322, 0332, 0333  // medium
		//0310, 0300, 0301, 0311  // high
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

static void motor_on(void) {
	CyU3PReturnStatus_t status = CyU3PGpioSetValue(MOTOR_DRIVER_EN, CyFalse);
	if (status != CY_U3P_SUCCESS) {
		CyU3PDebugPrint (4, "motor_on: error: %d 0x%x\r\n", status, status);
	}
	CyU3PThreadSleep(5);
}

static void motor_off(void) {
	CyU3PReturnStatus_t status = CyU3PGpioSetValue(MOTOR_DRIVER_EN, CyTrue);
	if (status != CY_U3P_SUCCESS) {
		CyU3PDebugPrint (4, "motor_off: error: %d 0x%x\r\n", status, status);
	}
	CyU3PThreadSleep(1);
}
