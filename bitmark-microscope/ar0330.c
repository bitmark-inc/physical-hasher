// Copyright Bitmark Inc. 2015-2015

#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3dma.h>
#include <cyu3error.h>
#include <cyu3uart.h>
#include <cyu3i2c.h>
#include <cyu3types.h>
#include <cyu3gpio.h>
#include <cyu3utils.h>
#include <cyu3mipicsi.h>

#include "ar0330.h"
#include "sensor_i2c.h"

#define DEBUG_ENABLE 0


// set pixel resolution: 8, 10 or 12
// lanes: 2 or 4
#define USE_N_BITS 12
#define USE_N_LANES 2

static const sensor_data_t AR0330_PrimaryInitialisation[] = {
	// from datasheet power-up sequence
	{0x3152, 0xA114},               // ?
	{0x304a, 0x0070},               // ?
	{SENSOR_DELAY, 1000},           // should equate to 150,000 EXTCLK periods

	// reset
	{0x301A, 0x0059},               // reset_register = Reset
	{SENSOR_DELAY, 100},
	{0x301A, 0x0050},               // reset_register = Unlock
	{SENSOR_DELAY, 100},

	// VCO input = 24MHz
	//    8 bit => 392MHz
	//   10 bit => 490MHz
	//   12 bit => 588MHz
#if USE_N_BITS == 8
	{0x302E, 3},                    // pre_pll_clk_div ( 8 bit, 392MHz)
	{0x3030, 49},                   // pll_multiplier  ( 8 bit, 392MHz)
#elif USE_N_BITS == 10
	{0x302E, 12},                   // pre_pll_clk_div (10 bit, 490MHz)
	{0x3030, 245},                  // pll_multiplier  (10 bit, 490MHz)
#elif USE_N_BITS == 12
	{0x302E, 2},                    // pre_pll_clk_div (12 bit, 588MHz)
	{0x3030, 49},                   // pll_multiplier  (12 bit, 588MHz)
#else
#error "only 10 or 12 bit modes supported"
#endif

#if USE_N_LANES == 2
	{0x31AE,0x0202},                // two lanes
#elif USE_N_LANES == 4
	{0x31AE,0x0204},                // four lanes
#else
#error "only 2 or 4 lanes supported"
#endif


#if USE_N_BITS == 8
	// PLL
#if USE_N_LANES == 2
	{0x302C, 2},                    // vt_sys_clk_div
	{0x302A, 4},                    // vt_pix_clk_div
	{0x3038, 1},                    // op_sys_clk_div
	{0x3036, 8},                    // op_pix_clk_div
#elif USE_N_LANES == 4
	{0x302C, 1},                    // vt_sys_clk_div
	{0x302A, 4},                    // vt_pix_clk_div
	{0x3038, 1},                    // op_sys_clk_div
	{0x3036, 8},                    // op_pix_clk_div
#else
#error "only 2 or 4 lanes supported"
#endif

	// 8 bit MIPI config
	{0x31AC, 0x0808},               // data_format_bits
	{0x31B0, 40},                   // frame_preamble
	{0X31B2, 14},                   // line_preamble
	{0X31B4, 0x5f77},               // MIPI_timing_0
	{0X31B6, 0x5299},               // MIPI_timing_1
	{0X31B8, 0x408e},               // MIPI_timing_2
	{0X31BA, 0x030c},               // MIPI_timing_3
	{0X31BC, 0x800a},               // MIPI_timing_4
	{0x31BE, 0x2003},               // MIPI_Config_status
	{0x31D0, 0x0000},               // compression = off

#elif USE_N_BITS == 10
	// PLL
#if USE_N_LANES == 2
	{0x302C, 2},                    // vt_sys_clk_div
	{0x302A, 5},                    // vt_pix_clk_div
	{0x3038, 1},                    // op_sys_clk_div
	{0x3036, 10},                   // op_pix_clk_div
#elif USE_N_LANES == 4
	{0x302C, 1},                    // vt_sys_clk_div
	{0x302A, 5},                    // vt_pix_clk_div
	{0x3038, 1},                    // op_sys_clk_div
	{0x3036, 10},                   // op_pix_clk_div
#else
#error "only 2 or 4 lanes supported"
#endif

	// 10 bit MIPI config
	{0x31AC, 0x0A0A},               // data_format_bits
	{0x31B0, 40},                   // frame_preamble
	{0X31B2, 14},                   // line_preamble
	{0X31B4, 0x2743},               // MIPI_timing_0
	{0X31B6, 0x114e},               // MIPI_timing_1
	{0X31B8, 0x2049},               // MIPI_timing_2
	{0X31BA, 0x0186},               // MIPI_timing_3
	{0X31BC, 0x8005},               // MIPI_timing_4
	{0x31BE, 0x2003},               // MIPI_Config_status
	{0x31D0, 0x0000},               // compression = off

#elif USE_N_BITS == 12
	// PLL
#if USE_N_LANES == 2
	{0x302C, 2},                    // vt_sys_clk_div
	{0x302A, 6},                    // vt_pix_clk_div
	{0x3038, 1},                    // op_sys_clk_div
	{0x3036, 12},                   // op_pix_clk_div
#elif USE_N_LANES == 4
	{0x302C, 1},                    // vt_sys_clk_div
	{0x302A, 6},                    // vt_pix_clk_div
	{0x3038, 1},                    // op_sys_clk_div
	{0x3036, 12},                   // op_pix_clk_div
#else
#error "only 2 or 4 lanes supported"
#endif

	// 12 bit MIPI config
	{0x31AC, 0x0C0C},               // data_format_bits
	{0x31B0, 36},                   // frame_preamble
	{0X31B2, 12},                   // line_preamble
	{0X31B4, 0x2643},               // MIPI_timing_0
	{0X31B6, 0x114e},               // MIPI_timing_1
	{0X31B8, 0x2048},               // MIPI_timing_2
	{0X31BA, 0x0186},               // MIPI_timing_3
	{0X31BC, 0x8005},               // MIPI_timing_4
	{0x31BE, 0x2003},               // MIPI_Config_status
	{0x31D0, 0x0000},               // compression = off

#else
#error "only 10 or 12 bit modes supported"
#endif

	// Timing_settings
	{0x3002, 0x0216},               // y_addr_start = 534
	{0x3004, 0x0346},               // x_addr_start = 838
	{0x3006, 0x03F5},               // y_addr_end = 1013
	{0x3008, 0x05C5},               // x_addr_end = 1477
	{0x300A, 0x0508},               // frame_length_lines = 2579
	{0x300C, 0x04DA},               // line_length_pck = 1242
	{0x3012, 0x0304},               // coarse_integration_time = 1210
	{0x3014, 0x0000},               // fine_integration_time = 0
	{0x30A2, 0x0001},               // x_odd_inc = 1
	{0x30A6, 0x0001},               // y_odd_inc = 1
	{0x308C, 0x0216},               // y_addr_start_cb = 534
	{0x308A, 0x0346},               // x_addr_start_cb = 838
	{0x3090, 0x03F5},               // y_addr_end_cb = 1013
	{0x308E, 0x05C5},               // x_addr_end_cb = 1477
	{0x30AA, 0x0508},               // frame_length_lines_cb = 2579
	{0x303E, 0x04DA},               // line_length_pck_cb = 1242
	{0x3016, 0x0304},               // coarse_integration_time_cb = 1554
	{0x3018, 0x0000},               // fine_integration_time_cb = 0
	{0x30AE, 0x0001},               // x_odd_inc_cb = 1
	{0x30A8, 0x0001},               // y_odd_inc_cb = 1
	{0x3040, 0x0000},               // read_mode = 0
	{0x3042, 0x0130},               // extra_delay = 85

	{0x3028, 0x0010},               // row_speed
	{0x3064, 0x1902},               // smia_test

	// gains
	{0x3060, 0x0000},               // analog_gain = 1x
	{0x30B0, 0x8000},               // digital_test = context A
	{0x305E, 0x0080},               // global_gain    = 1x
	{0x30C4, 0x0080},               // global_gain_cb = 1x
	{0x30BA, 0x000C},               // digital_ctrl = no dither

	{0x301A, 0x0058},               // reset_register = lock
	{SENSOR_DELAY, 5},
};


//
// 720p setting
//
// 1280 * 720 @ 60 fps
static const sensor_data_t sensor_720p_regs[] = {
	{0x3004,    518},               // x address start
	{0x3008,   1797},               // x address end
	{0x3002,    418},               // y address start
	{0x3006,   1137},               // y address end

	{0x30A2,      1},               // x odd inc (No Skip)
	{0x30A6,      1},               // y odd inc (No Skip)

	{0x3040, 0x0000},               // read mode
	{0x300C,   1280 +  6},          // the number of pixel clock periods in one line time
	{0x300A,    720 + 12},          // the number of complete lines(rows) in the frame timing

	{0x3014,      0},               // fine integration time
	{0x3012,   1349},               // coarse integration time
	{0x3042,      0},               // extra delay

#if 0
	{SENSOR_DELAY, 10},
	{0x301A, 0x0058},               // reset_register = lock
	{SENSOR_DELAY, 10},
#endif
};

//
// 1080p settings
//
// 1920 * 1080 @ 30 fps EIS
static const sensor_data_t sensor_1080p_regs[] = {
	{0x3004,    198},               // x address start
	{0x3008,   2117},               // x address end
	{0x3002,    238},               // y address start
	{0x3006,   1317},               // y address end

	{0x30A2,      1},               // x odd inc (No Skip)
	{0x30A6,      1},               // y odd inc (No Skip)

	{0x3040, 0x0000},               // read mode
	{0x300C,   1920 +  6},          // the number of pixel clock periods in one line time
	{0x300A,   1080 + 12},          // the number of complete lines(rows) in the frame timing

	{0x3014,      0},               // fine integration time
	{0x3012,   1349},               // coarse integration time
	{0x3042,      0},               // extra delay

#if 0
	{SENSOR_DELAY, 10},
	{0x301A, 0x0058},               // reset_register = lock
	{SENSOR_DELAY, 10},
#endif
};


//
// 5M pixel settings
//
// 2304 X 1296
static const sensor_data_t sensor_5MP_regs[] = {
	{0x3004,      6},               // x address start
	{0x3008,   2309},               // x address end
	{0x3002,    120},               // y address start
	{0x3006,   1415},               // y address end

	{0x30A2,      1},               // x odd inc (No Skip)
	{0x30A6,      1},               // y odd inc (No Skip)

	{0x3040, 0x0000},               // read mode
	{0x300c,   2304 +  6},          // the number of pixel clock periods in one line time
	{0x300A,   1296 + 12},          // the number of complete lines(rows) in the frame timing

	{0x3014,      0},               // fine integration time
	{0x3012,   1349},               // coarse integration time
	{0x3042,      0},               // extra delay

#if 0
	{SENSOR_DELAY, 10},
	{0x301A, 0x0058},               // reset_register = lock
	{SENSOR_DELAY, 10},
#endif
};


void AR0330_Debug(void)
{
#if DEBUG_ENABLE
	uint16_t tempaddr = 0x3000;
	int i;
	for (i = 0; i < 256; i += 4) {
		CyU3PDebugPrint (4, "DEBUG: 0x%x:  ", tempaddr);
		int j;
		for (j = 0; j < 4; ++j, tempaddr += 2) {
			uint16_t d;
			sensor_read(tempaddr, &d);
			CyU3PDebugPrint (4, "0x%x%x%x%x ",
					 (d >> 12) & 0x0f,
					 (d >>  8) & 0x0f,
					 (d >>  4) & 0x0f,
					 (d >>  0) & 0x0f);
		}
		CyU3PDebugPrint (4, "\r\n");
	}

	static const uint16_t addresses[] = {
		0x3780,
		0x301A  // reset status
//		0x3ED0,  0x3ED2,  0x3ED4,  0x3ED6,
//		0x3ED8,  0x3EDA,  0x3EDC,  0x3EDE,
//		0x3EE0,                    0x3EE6,
//		0x3EE8,  0x3EEA,
//		0x3F06,
	};

	for (i = 0; i < SIZE_OF_ARRAY(addresses); ++i) {
		tempaddr = addresses[i];
		uint16_t tempdata;
		sensor_read(tempaddr, &tempdata);
		CyU3PDebugPrint (4, "DEBUG: read address = 0x%x data = 0x%x\r\n", tempaddr, tempdata);
	}

	// report if MIPI is active
	CyU3PDebugPrint(4, "DEBUG: MIPI active = %d\r\n", CyU3PMipicsiCheckBlockActive());
#endif
}


void AR0330_Base_Config(void) {
	uint16_t tempdata;
	uint16_t tempaddr;

	CyU3PDebugPrint (4, "AR0330_Base_Config: starting...\r\n");

	//SensorReset(); // already done elsewhere

	SENSOR_WRITE_ARRAY(AR0330_PrimaryInitialisation);

	// display versions
	tempaddr = 0x3000;
	sensor_read(0x3000, &tempdata);
	CyU3PDebugPrint (4, "AR0330_Base_Config: chip version@0x%x = 0x%x\r\n", tempaddr, tempdata);

	tempaddr = 0x300E;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "AR0330_Base_Config: revision number@0x%x = 0x%x\r\n", tempaddr, tempdata);

#if 0
	tempaddr = 0x30F0;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "AR0330_Base_Config: read address = 0x%x data = 0x%x\r\n",tempaddr,tempdata);

	tempaddr = 0x3072;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "AR0330_Base_Config: read address = 0x%x data = 0x%x\r\n",tempaddr,tempdata);
#endif

	// report if MIPI is active
	CyU3PDebugPrint(4, "AR0330_Base_Config: MIPI active = %d\r\n", CyU3PMipicsiCheckBlockActive());
}


void AR0330_VGA_config(void) {
	CyU3PDebugPrint (4, "AR0330_VGA_config =============\r\n");
}


void AR0330_VGA_HS_config(void) {
	CyU3PDebugPrint (4, "AR0330_VGA_HS_config =============\r\n");

	AR0330_Debug();
}


void AR0330_720P_config(void) {
	CyU3PDebugPrint (4, "AR0330_720P_config =============\r\n");

	SENSOR_WRITE_ARRAY(AR0330_PrimaryInitialisation);
	SENSOR_WRITE_ARRAY(sensor_720p_regs);

	AR0330_Debug();
}

void AR0330_1080P_config(void) {
	CyU3PDebugPrint (4, "AR0330_1080P_config =============\r\n");

	SENSOR_WRITE_ARRAY(AR0330_PrimaryInitialisation);
	SENSOR_WRITE_ARRAY(sensor_1080p_regs);

	AR0330_Debug();
}


void AR0330_5MP_config(void) {
	CyU3PDebugPrint (4, "AR0330_5MP_config =============\r\n");

	SENSOR_WRITE_ARRAY(AR0330_PrimaryInitialisation);
	SENSOR_WRITE_ARRAY(sensor_5MP_regs);

	AR0330_Debug();
}


void AR0330_Power_Down(void) {
	CyU3PDebugPrint (4, "AR0330_Power_Down ================\r\n");

	sensor_write(0x301a, 0x0058);
}


void AR0330_Power_Up(void) {
	CyU3PDebugPrint (4, "AR0330_Power_Up =============\r\n");

	// set test mode
	sensor_write(0x3070, 0x0000);
	sensor_write(0x3072, 0x0180 + 'R'); // red
	sensor_write(0x3074, 0x0280 + 'G'); // green (in red row)
	sensor_write(0x3076, 0x0380 + 'b'); // blue
	sensor_write(0x3078, 0x0480 + 'g'); // green (in blue row)

	// strt streaming
	sensor_write(0x301a, 0x005c);

	CyU3PThreadSleep(10);

	AR0330_Debug();
}


void AR0330_Auto_Focus_Config(void) {
	CyU3PDebugPrint (4, "AR0330_Auto_Focus_Config ================\r\n");
}


void AR0330_SetBrightness(int8_t Brightness) {
	CyU3PDebugPrint (4, "AR0330_SetBrightness ================\r\n");
}


int8_t AR0330_GetBrightness(uint8_t option) {
	CyU3PDebugPrint (4, "AR0330_GetBrightness ================\r\n");
	return 0;
}


void AR0330_SetContrast(int8_t Contrast) {
	CyU3PDebugPrint (4, "AR0330_SetContrast ================\r\n");
}


int8_t AR0330_GetContrast(int8_t option) {

	CyU3PDebugPrint (4, "AR0330_GetContrast ================\r\n");
#if 0
	CyBool_t clr_error_count = CyFalse;
	CyU3PMipicsiErrorCounts_t error_count;

	CyU3PMipicsiGetErrors(clr_error_count, &error_count);
	CyU3PDebugPrint (4, "Error count %d\r\n", clr_error_count);

	CyU3PDebugPrint (4, "frmErrCnt =  %d\r\n", error_count.frmErrCnt);
	CyU3PDebugPrint (4, "crcErrCnt =  %d\r\n", error_count.crcErrCnt);
	CyU3PDebugPrint (4, "mdlErrCnt =  %d\r\n", error_count.mdlErrCnt);
	CyU3PDebugPrint (4, "ctlErrCnt =  %d\r\n", error_count.ctlErrCnt);

	CyU3PDebugPrint (4, "eidErrCnt =  %d\r\n", error_count.eidErrCnt);
	CyU3PDebugPrint (4, "recrErrCnt =  %d\r\n", error_count.recrErrCnt);
	CyU3PDebugPrint (4, "unrcErrCnt =  %d\r\n", error_count.unrcErrCnt);
	CyU3PDebugPrint (4, "recSyncErrCnt =  %d\r\n", error_count.recSyncErrCnt);
	CyU3PDebugPrint (4, "unrSyncErrCnt =  %d\r\n", error_count.unrSyncErrCnt);
#endif
	return 0;
}


void AR0330_SetHue(int32_t Hue) {
	CyU3PDebugPrint (4, "AR0330_SetHue ================\r\n");
}


int32_t AR0330_GetHue(uint8_t option) {
	CyU3PDebugPrint (4, "AR0330_SetHue ================\r\n");
	return 0;
}


void AR0330_SetSaturation(uint32_t Saturation)
{
	CyU3PDebugPrint (4, "AR0330_SetSaturation ================\r\n");
}

int8_t AR0330_GetSaturation(uint8_t option) {
	CyU3PDebugPrint (4, "AR0330_GetSaturation ================\r\n");

	return 0;
}


void AR0330_SetSharpness(uint8_t Sharp) {
	CyU3PDebugPrint (4, "AR0330_SetSharpness ================\r\n");
}


int8_t AR0330_GetSharpness(uint8_t option) {
	CyU3PDebugPrint (4, "AR0330_GetSharpness ================\r\n");

	return 0;
}


void AR0330_SetWhiteBalance(uint8_t WhiteBalance) {
	CyU3PDebugPrint (4, "AR0330_SetWhiteBalance ================\r\n");
}


uint8_t AR0330_GetWhiteBalance(uint8_t option) {
	CyU3PDebugPrint (4, "AR0330_GetWhiteBalance ================\r\n");

	return 0;
}


void AR0330_SetAutoWhiteBalance(uint8_t AutoWhiteBalance) {
	CyU3PDebugPrint (4, "AR0330_SetAutoWhiteBalance ================\r\n");
}


uint8_t AR0330_GetAutoWhiteBalance(uint8_t option) {
	CyU3PDebugPrint (4, "AR0330_GetAutoWhiteBalance ================\r\n");
	return 0;
}


void AR0330_SetExposure(int32_t Exposure) {
	CyU3PDebugPrint (4, "AR0330_SetExposure ================\r\n");
}


int32_t AR0330_GetExposure(uint8_t option) {
	CyU3PDebugPrint (4, "AR0330_GetExposure ================\r\n");
#if 0
	CyBool_t clr_error_count = CyFalse;
	CyU3PMipicsiErrorCounts_t error_count;

	CyU3PMipicsiGetErrors(clr_error_count, &error_count);
	CyU3PDebugPrint (4, "Error count %d\r\n", clr_error_count);

	CyU3PDebugPrint (4, "frmErrCnt =  %d\r\n", error_count.frmErrCnt);
	CyU3PDebugPrint (4, "crcErrCnt =  %d\r\n", error_count.crcErrCnt);
	CyU3PDebugPrint (4, "mdlErrCnt =  %d\r\n", error_count.mdlErrCnt);
	CyU3PDebugPrint (4, "ctlErrCnt =  %d\r\n", error_count.ctlErrCnt);

	CyU3PDebugPrint (4, "eidErrCnt =  %d\r\n", error_count.eidErrCnt);
	CyU3PDebugPrint (4, "recrErrCnt =  %d\r\n", error_count.recrErrCnt);
	CyU3PDebugPrint (4, "unrcErrCnt =  %d\r\n", error_count.unrcErrCnt);
	CyU3PDebugPrint (4, "recSyncErrCnt =  %d\r\n", error_count.recSyncErrCnt);
	CyU3PDebugPrint (4, "unrSyncErrCnt =  %d\r\n", error_count.unrSyncErrCnt);
#endif
	return 0;
}


void AR0330_SetAutofocus(uint8_t Is_Enable) {
	CyU3PDebugPrint (4, "AR0330_SetAutofocus ================\r\n");
}


uint8_t AR0330_GetAutofocus(uint8_t option) {
	CyU3PDebugPrint (4, "AR0330_GetAutofocus ================\r\n");
	return 1;
}


void AR0330_SetManualfocus(uint16_t manualfocus) {
	CyU3PDebugPrint (4, "AR0330_SetManualfocus ================\r\n");
}


uint16_t AR0330_GetManualfocus(uint8_t option) {
	CyU3PDebugPrint (4, "AR0330_GetManualfocus ================\r\n");
	return 0;
}


void AR0330_SetAutoExposure(uint8_t AutoExp) {
	CyU3PDebugPrint (4, "AR0330_SetAutoExposure ================\r\n");
}

uint8_t AR0330_GetAutoExposure(uint8_t option) {
	CyU3PDebugPrint (4, "AR0330_GetAutoExposure ================\r\n");
	return 0;
}
