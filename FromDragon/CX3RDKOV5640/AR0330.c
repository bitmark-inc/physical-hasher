//
// FILENAME.
//      Camera_API.c - Camera general API header
//
//      $PATH:
//
// FUNCTIONAL DESCRIPTION.
//
//
//
// MODIFICATION HISTORY.
//      2015/06/07      inital
//
// NOTICE.
//      Copyright (C) 2015-2025 Dragon All Rights Reserved.
//

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

#include "AR0330.h"

#define  SEQUNSER_A
#define regval_list reg_list_a16_d16
#define REG_DLY  0xffff



// from TI driver
static struct regval_list AR0330_OTPMVsettingsV5[] =
{
    {0x3ED2, 0x0146},
    {0x3EDA, 0x88BC},
    {0x3EDC, 0xAA63},
    {0x305E, 0x00A0},
};


static struct regval_list AR0330_SetOtherRegs[] =
{

		//PLL_settings (9)
		{0x301A, 0x0059},		//RESET_REGISTER = 88
		{0x301A, 0x0058},		//RESET_REGISTER = 88
		{0x302A, 0x0004},		//VT_PIX_CLK_DIV = 4
		{0x302C, 0x0002},		//VT_SYS_CLK_DIV = 2
		{0x302E, 0x0001},		//PRE_PLL_CLK_DIV = 1
		{0x3030, 0x0020},		//PLL_MULTIPLIER = 32
		{0x3036, 0x0008},		//OP_PIX_CLK_DIV = 8
		{0x3038, 0x0001},		//OP_SYS_CLK_DIV = 1
		{0x31AC, 0x0808},		//DATA_FORMAT_BITS = 2056
		{0x31AE, 0x0202},		//SERIAL_FORMAT = 514

		//MIPI Port Timing (7)
//		{0x31B0, 0x0049},     // FRAME_PREAMBLE = 73
//		{0x31B2, 0x001C},     //LINE_PREAMBLE = 28
		{0x31B0, 0x0028},     // FRAME_PREAMBLE = 73
		{0x31B2, 0x000E},     //LINE_PREAMBLE = 28
		{0x31B4, 0x5F77},     //MIPI_TIMING_0 = 24439
		{0x31B6, 0x5299},     //MIPI_TIMING_1 = 21145
		{0x31B8, 0x408E},     //MIPI_TIMING_2 = 16526
		{0x31BA, 0x030C},     //MIPI_TIMING_3 = 780
		{0x31BC, 0x800A},     //MIPI_TIMING_4 = 32778

		//Timing_settings (23)
		{0x3002, 0x0216},		//Y_ADDR_START = 534
		{0x3004, 0x0346},		//X_ADDR_START = 838
		{0x3006, 0x03F5},		//Y_ADDR_END = 1013
		{0x3008, 0x05C5},		//X_ADDR_END = 1477
		{0x300A, 0x0508},		//FRAME_LENGTH_LINES = 2579
		{0x300C, 0x04DA},		//LINE_LENGTH_PCK = 1242
		{0x3012, 0x0304},		//COARSE_INTEGRATION_TIME = 1210
		{0x3014, 0x0000},		//FINE_INTEGRATION_TIME = 0
		{0x30A2, 0x0001},		//X_ODD_INC = 1
		{0x30A6, 0x0001},		//Y_ODD_INC = 1
		{0x308C, 0x0216},		//Y_ADDR_START_CB = 534
		{0x308A, 0x0346},		//X_ADDR_START_CB = 838
		{0x3090, 0x03F5},		//Y_ADDR_END_CB = 1013
		{0x308E, 0x05C5},		//X_ADDR_END_CB = 1477
		{0x30AA, 0x0508},		//FRAME_LENGTH_LINES_CB = 2579
		{0x303E, 0x04DA},		//LINE_LENGTH_PCK_CB = 1242
		{0x3016, 0x0304},		//COARSE_INTEGRATION_TIME_CB = 1554
		{0x3018, 0x0000},		//FINE_INTEGRATION_TIME_CB = 0
		{0x30AE, 0x0001},		//X_ODD_INC_CB = 1
		{0x30A8, 0x0001},		//Y_ODD_INC_CB = 1
		{0x3040, 0x0000},		//READ_MODE = 0
		{0x3042, 0x0130},		//EXTRA_DELAY = 85
		{0x30BA, 0x002C},		//DIGITAL_CTRL = 44


		//Recommended Configuration (9)
		{0x31E0, 0x0303},
		{0x3064, 0x1802},
		{0x3ED2, 0x0146},
		{0x3ED4, 0x8F6C},
		{0x3ED6, 0x66CC},
		{0x3ED8, 0x8C42},
		{0x3EDA, 0x88BC},
		{0x3EDC, 0xAA63},
		{0x305E, 0x00A0},
};

/*
static struct regval_list AR0330_SetOtherRegs[] =
{

		//PLL_settings (9)
		{0x301A, 0x0059},		//RESET_REGISTER = 88
		{0x301A, 0x0058},		//RESET_REGISTER = 88
		{0x302A, 0x0006},		//VT_PIX_CLK_DIV = 4
		{0x302C, 0x0002},		//VT_SYS_CLK_DIV = 2
		{0x302E, 0x0004},		//PRE_PLL_CLK_DIV = 1
		{0x3030, 0x0062},		//PLL_MULTIPLIER = 32
		{0x3036, 0x000C},		//OP_PIX_CLK_DIV = 8
		{0x3038, 0x0001},		//OP_SYS_CLK_DIV = 1
		{0x31AC, 0x0C0C},		//DATA_FORMAT_BITS = 2056
		{0x31AE, 0x0202},		//SERIAL_FORMAT = 514

		//MIPI Port Timing (7)
		{0x31B0, 0x0028},     // FRAME_PREAMBLE = 73
		{0x31B2, 0x000E},     //LINE_PREAMBLE = 28
		{0x31B4, 0x2743},     //MIPI_TIMING_0 = 24439
		{0x31B6, 0x114E},     //MIPI_TIMING_1 = 21145
		{0x31B8, 0x2049},     //MIPI_TIMING_2 = 16526
		{0x31BA, 0x0186},     //MIPI_TIMING_3 = 780
		{0x31BC, 0x8005},     //MIPI_TIMING_4 = 32778

		//Timing_settings (23)
		{0x3002, 0x0204},		//Y_ADDR_START = 534
		{0x3004, 0x00C6},		//X_ADDR_START = 838
		{0x3006, 0x0521},		//Y_ADDR_END = 1013
		{0x3008, 0x0845},		//X_ADDR_END = 1477
		{0x300A, 0x0508},		//FRAME_LENGTH_LINES = 2579
		{0x300C, 0x04DA},		//LINE_LENGTH_PCK = 1242
		{0x3012, 0x0284},		//COARSE_INTEGRATION_TIME = 1210
		{0x3014, 0x0000},		//FINE_INTEGRATION_TIME = 0
		{0x30A2, 0x0001},		//X_ODD_INC = 1
		{0x30A6, 0x0001},		//Y_ODD_INC = 1
		{0x308C, 0x0006},		//Y_ADDR_START_CB = 534
		{0x308A, 0x0006},		//X_ADDR_START_CB = 838
		{0x3090, 0x0605},		//Y_ADDR_END_CB = 1013
		{0x308E, 0x0905},		//X_ADDR_END_CB = 1477
		{0x30AA, 0x0A04},		//FRAME_LENGTH_LINES_CB = 2579
		{0x303E, 0x04E0},		//LINE_LENGTH_PCK_CB = 1242
		{0x3016, 0x0A03},		//COARSE_INTEGRATION_TIME_CB = 1554
		{0x3018, 0x0000},		//FINE_INTEGRATION_TIME_CB = 0
		{0x30AE, 0x0001},		//X_ODD_INC_CB = 1
		{0x30A8, 0x0001},		//Y_ODD_INC_CB = 1
		{0x3040, 0x0000},		//READ_MODE = 0
		{0x3042, 0x0130},		//EXTRA_DELAY = 85
		{0x30BA, 0x002C},		//DIGITAL_CTRL = 44


		//Recommended Configuration (9)
		{0x31E0, 0x0303},
		{0x3064, 0x1802},
		{0x3ED2, 0x0146},
		{0x3ED4, 0x8F6C},
		{0x3ED6, 0x66CC},
		{0x3ED8, 0x8C42},
		{0x3EDA, 0x88BC},
		{0x3EDC, 0xAA63},
		{0x305E, 0x00A0},
};
*/

// PLL for 1 lane MIPI:
// 1 x CLK_OP = 2 x CLK_PIX = Pixel Rate
// (max: 65 Mpixels/se for 12 bits and 76 Mpixel/s for 10 bits)
static struct regval_list AR0330_setPLLRges1Lane[] =
{
    {0x31AE, 0x0201},     // Serial interface, 1 lane MIPI
//	{0x302C, 0x0004},     // VT_PIX_CLK_DIV
	{0x302C, 0x0002},     // VT_PIX_CLK_DIV
//    {0x302E, 0x0001},     // PRE_PLL_CLK_DIV
	{0x302E, 0x0002},     // PRE_PLL_CLK_DIV
//    {0x3030, 0x0020},     // PLL_MULTIPLIER
    {0x3030, 49},     // PLL_MULTIPLIER
};



// PLL for 2 lane MIPI:
// 2 x CLK_OP = 2 x CLK_PIX = Pixel Rate (max: 98 Mpixel/s)
static struct regval_list AR0330_setPLLRges2Lane[] =
{
	{0x31AE, 0x0202},     //SERIAL_FORMAT = 514
	{0x302C, 0x0002},     // VT_PIX_CLK_DIV
    {0x302E, 0x0004},     // PRE_PLL_CLK_DIV
    {0x3030, 0x0062},     // PLL_MULTIPLIER
};

// PLL for 4 lane MIPI:
// 4 x CLK_OP = 4 x CLK_PIX = Pixel Rate (max: 98 Mpixel/s)
static struct regval_list AR0330_setPLLRges4Lane[] =
{
    {0x302C, 0x0001},     // VT_PIX_CLK_DIV
    {0x302E, 0x0004},     // PRE_PLL_CLK_DIV
    {0x3030, 0x0062},     // PLL_MULTIPLIER
};

//
// for customer setting
//
static struct regval_list test_pattern_setting[] =
{
    {0x3070, 0x0001},     // test pattern mode  (Solid color test pattern)

};

//
// Default Register Setting / PLL Setting
//

static struct regval_list sensor_default_regs_OTPM_V5[] =
{
	// Reset Sensor
	{0x301A, 0x0059},
	{REG_DLY,0x0064},       // just a  delay (no meaning)

	{0x3052, 0xA114},
	{0x304A, 0x0070},
	{REG_DLY,0x0032},       // just a delay (no meaning)

	{0x31AE, 0x202	},      //Output Interface Configured to 2lane MIPI
	{0x301A, 0x005c	},      //Disable Streaming
	{REG_DLY,0x0032},       // just a delay (no meaning)

	{0x3064, 0x1802	},
	{0x3078,0x0001	},	    //Marker to say that 'Defaults' have been run

	//{//Toggle Flash, }on Each Frame
	{0x3046, 0x4038	},	    // Enable Flash Pin
	{0x3048, 0x8480	},	    // Flash Pulse Length
	{0x31E0,0x0203  },

	//
	// Default Register (OTPM V5)
	//
	{0x3ED2, 0x0146},
	{0x3EDA, 0x88BC},
	{0x3EDC, 0xAA63},

//	{0x3F06, 0x046A},

	{0x305E, 0x00A0},

	//
	// PLL Setting  (TBD)
	//
	{0x301A, 0x10C0},        // TBD ?

	{0x302A, 0x0005},        // vt_pix_clk_div
	{0x302C, 0x0002},        // vt_sys_clk_div
	{0x302E, 0x0004},        // pre_pll_clk_div
	{0x3030, 0x0062},        // pll_multiplier
	{0x3036, 0x0010},        // op_pix_clk_div
	{0x3038, 0x0001},        // op_sys_clk_div
	{0x31AC, 0x0A0A},        // data_format is 10-bit

	{0x31B0, 0x0024},        //
	{0x31B2, 0x000C},        //
	{0x31B4, 0x2643},        //
	{0x31B6, 0x114E},        //
	{0x31B8, 0x2048},        //
	{0x31BA, 0x0186},        //
	{0x31BC, 0x8005},        //
	{0x31BE, 0x2003},        //



	{0x301a, 0x10d0},        // TBD ?
};


//
// 1080p setting
//

static struct regval_list sensor_720p_regs[] = {	//720: 1280*720@60fps

	{0x301A,0x0058},
	{REG_DLY,0x0022},

	//{0x31AE,0x0301},
	{0x31AE,0x0202},      // dual-lane MIPI

	{0x3004,0x0200},      // x address start
	{0x3008,0x06ff},      // x address end
	{0x3002,0x019c},      // y address start
	{0x3006,0x046b},      // y address end
	//{0x301A,0x10dc},

	{0x30A2,0x0001},      //  x odd inc (No Skip)
	{0x30A6,0x0001},      //  y odd inc (No Skip)

	{0x3040,0x0000},      //  read mode
	{0x300C,0x04E0},      //  the number of pixel clock periods in one line time
	{0x300A,0x0356},      //  the number of complete lines(rows) in the frame timing
	//{0x300A,0x028e},
	{0x3014,0x0000},      //  fine integration time
	{0x3012,0x0311},      //  coarse integration time
	{0x3042,0x0000},      //  extra delay
	{0x30BA,0x002C},      //  (?__?) digital ctrl
	// {0x3ED4,0x8F6C},   //  (?__?)
	{0x3ED6,0x66CC},      //  (?__?)

	// {0x3088,0x80BA},   // ?___?
	// {0x3086,0x0253},   // ?___?
	//{0x3086,0x0253},

	//{0x301A,0x0058}
	{0x301A,0x005C}
};


static struct regval_list sensor_1080p_regs[] = {    //1080: 1920*1080@30fps EIS

	// 2304 X 1296

	{0x301A,0x0059},
	{REG_DLY,0x0022},

	//{0x31AE,0x0301},

	{0x31AE,0x0202},      // dual-lane MIPI

	{0x3004,0x0006},      // x address start
	{0x3008,0x0905},      // x address end
	{0x3002,0x0078},      // y address start
	{0x3006,0x0587},      // y address end
	//{0x301A,0x10dc},

	{0x30A2,0x0001},      //  x odd inc (No Skip)
	{0x30A6,0x0001},      //  y odd inc (No Skip)

	{0x3040,0x0000},      //  read mode
	{0x300c,0x04E0},      //  the number of pixel clock periods in one line time
	{0x300A,0x06AD},      //  the number of complete lines(rows) in the frame timing
	//{0x300A,0x028e},

	{0x3014,0x0000},      //  fine integration time
	{0x3012,0x0545},      //  coarse integration time
	{0x3042,0x0000},      //  extra delay
	{0x30BA,0x002c},      //  (?__?) digital ctrl

	//{0x3ed4,0x8f6c},
	//{0x3ed6,0x66cc},

	//{0x3088,0x80BA},
	//{0x3086,0xE653},

	{0x301A,0x0058}
};


//
// 720p configuration
//

//
//  VGA Configuration
//

static struct regval_list sensor_vga_regs[] = { //VGA:  640*480
	//DVP_VGA_60fps

	{0x301A, 0x0058},
	{REG_DLY,0x0022},     // delay

	{0x31AE,0x0201},      // one lane MIPI

	{0x3004,0x00c0},      // x address start
//	{0x3008,0x083f},      // x address end
 	{0x3008,0x033F},      // x address end
	{0x3002,0x0030},      // y address start
//	{0x3006,0x05cf},      // y address end
  	{0x3006,0x020f},      // y address end
	//{0x301A,0x10dc},

	{0x30A2,0x0001},      //  x odd inc
	{0x30A6,0x0001},      //  y odd inc

	{0x3040,0x0000},      //  read mode
	{0x300c,0x04da},      //  the number of pixel clock periods in one line time
	{0x300A,0x0291},      //  the number of complete lines(rows) in the frame timing
	//{0x300A,0x028e},

	{0x3014,0x0000},      //  fine integration time
	{0x3012,0x0290},      //  coarse integration time
	{0x3042,0x02a1},      //  extra delay
	{0x30BA,0x002c},      //  (?__?) digital ctrl

	{0x301A,0x001C}

};


//
// This function inserts a delay between successful I2C transfers to prevent
// false errors due to the slave being busy.
//

//
// FUNCTION NAME.
//      SensorI2CAccessDelay
//
// FUNCTIONAL DESCRIPTION.
//      This function inserts a delay between successful I2C transfers to prevent
//      false errors due to the slave being busy.
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//

static void
SensorI2CAccessDelay(
    CyU3PReturnStatus_t status
    )
{
    //
    // Add a 10us delay if the I2C operation that preceded this call was successful.
    //

    if (status == CY_U3P_SUCCESS)
    CyU3PBusyWait(10);
}

//
// FUNCTION NAME.
//      SensorWrite2B
//
// FUNCTIONAL DESCRIPTION.
//      Write to an I2C slave with two bytes of data.
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//

CyU3PReturnStatus_t
SensorWrite2B(
    uint8_t slaveAddr,
    uint8_t highAddr,
    uint8_t lowAddr,
    uint8_t highData,
    uint8_t lowData
    )
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PI2cPreamble_t preamble;
    uint8_t buf[2];


    // Validate the I2C slave address.
    //
//    if ((slaveAddr != SENSOR_ADDR_WR) && (slaveAddr != I2C_MEMORY_ADDR_WR))
    if (slaveAddr != SENSOR_ADDR_WR)
    {
        CyU3PDebugPrint(4, "\rI2C Slave address is not valid!\n");
        return 1;
    }

    //
    // Set the parameters for the I2C API access and then call the write API.
    //
    preamble.buffer[0] = slaveAddr;
    preamble.buffer[1] = highAddr;
    preamble.buffer[2] = lowAddr;
    preamble.length = 3;            //  Three byte preamble.
    preamble.ctrlMask = 0x0000;     //  No additional start and stop bits.

    buf[0] = highData;
    buf[1] = lowData;

    apiRetStatus = CyU3PI2cTransmitBytes(&preamble, buf, 2, 0);
    SensorI2CAccessDelay(apiRetStatus);

    if (apiRetStatus == CY_U3P_SUCCESS)
    		CyU3PDebugPrint (4, "\rWrite Camera REG address =  0x%x%x data = 0x%x%x   \r\n", highAddr, lowAddr, highData, lowData);
    	else
    		CyU3PDebugPrint (4, "\rFailed to write . Error Code = 0x%x  \r\n",apiRetStatus);

    return apiRetStatus;
}

//
// FUNCTION NAME.
//     SensorWrite
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//

CyU3PReturnStatus_t
SensorWrite(
    uint8_t slaveAddr,
    uint8_t highAddr,
    uint8_t lowAddr,
    uint8_t count,
    uint8_t *buf
    )
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PI2cPreamble_t preamble;

    //
    // Validate the I2C slave address.
    //
//    if ((slaveAddr != SENSOR_ADDR_WR) && (slaveAddr != I2C_MEMORY_ADDR_WR))
	if (slaveAddr != SENSOR_ADDR_WR)
    {
        CyU3PDebugPrint(4, "I2C Slave address is not valid!\n");
        return 1;
    }

    if (count > 64)
    {
        CyU3PDebugPrint(4, "ERROR: SensorWrite count > 64\n");
        return 1;
    }

    //
    // Set up the I2C control parameters and invoke the write API.
    //
    preamble.buffer[0] = slaveAddr;
    preamble.buffer[1] = highAddr;
    preamble.buffer[2] = lowAddr;
    preamble.length = 3;
    preamble.ctrlMask = 0x0000;

    apiRetStatus = CyU3PI2cTransmitBytes(&preamble, buf, count, 0);
    SensorI2CAccessDelay(apiRetStatus);

    return apiRetStatus;
}

//
// FUNCTION NAME.
//     SensorRead2B
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//

CyU3PReturnStatus_t
SensorRead2B(
    uint8_t slaveAddr,
    uint8_t highAddr,
    uint8_t lowAddr,
    uint8_t *buf
    )
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PI2cPreamble_t preamble;

    preamble.buffer[0] = slaveAddr & I2C_SLAVEADDR_MASK;    //  Mask out the transfer type bit.
    preamble.buffer[1] = (uint8_t) highAddr;
    preamble.buffer[2] = (uint8_t) lowAddr;
    preamble.buffer[3] =  (slaveAddr | 0x01);
    preamble.length = 4;
    preamble.ctrlMask = 0x0004;     //  Send start bit after third byte of preamble.

    apiRetStatus = CyU3PI2cReceiveBytes(&preamble, buf, 2, 0);
    SensorI2CAccessDelay(apiRetStatus);

    return apiRetStatus;
}

//
// FUNCTION NAME.
//     SensorRead
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//

CyU3PReturnStatus_t SensorRead(
    uint8_t slaveAddr,
    uint8_t highAddr,
    uint8_t lowAddr,
    uint8_t count,
    uint8_t *buf
    )
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PI2cPreamble_t preamble;

    //
    // Validate the parameters.
    //
    if (slaveAddr != SENSOR_ADDR_RD)
    {
        CyU3PDebugPrint(4, "I2C Slave address is not valid!\n");
        return 1;
    }

    if (count > 64) {
        CyU3PDebugPrint(4, "ERROR: SensorWrite count > 64\n");
        return 1;
    }

    preamble.buffer[0] = slaveAddr & I2C_SLAVEADDR_MASK;    //  Mask out the transfer type bit.
    preamble.buffer[1] = highAddr;
    preamble.buffer[2] = lowAddr;
    preamble.buffer[3] = slaveAddr;
    preamble.length = 4;
    preamble.ctrlMask = 0x0004; //Send start bit after third byte of preamble.

    apiRetStatus = CyU3PI2cReceiveBytes(&preamble, buf, count, 0);
    SensorI2CAccessDelay(apiRetStatus);

    return apiRetStatus;
}


static CyU3PReturnStatus_t sensor_write(
	unsigned short reg,
	unsigned short value
	)
{
//	int ret=0;
	int cnt=0;

	uint8_t highAddr;
    uint8_t lowAddr;
    uint8_t highData;
    uint8_t lowData;

    CyU3PReturnStatus_t retCyU3;

	highAddr  = (uint8_t) ((reg & 0xFF00) >> 8);
	lowAddr   = (uint8_t) (reg & 0x00FF);
	highData  = (uint8_t) ((value & 0xFF00) >> 8);
	lowData   = (uint8_t) (value & 0x00FF);

	retCyU3 = SensorWrite2B(SENSOR_ADDR_WR,highAddr,lowAddr,highData,lowData);

	while((retCyU3 != CY_U3P_SUCCESS) && (cnt < 2))
	{
		retCyU3 = SensorWrite2B(SENSOR_ADDR_WR,highAddr,lowAddr,highData,lowData);
		cnt++;
	}
	if(cnt>0)
	{
		CyU3PDebugPrint(4, "\rSensor read retry = %d\n",cnt);
	}
	return retCyU3;
}

static CyU3PReturnStatus_t sensor_write_array(
	struct regval_list *regs,
	int array_size
	)
{
	int i = 0;
	CyU3PReturnStatus_t retCyU3;

	if(!regs)
		return -EINVAL;

	while(i < array_size)
	{
		if(regs->addr == REG_DLY)
		{
			CyU3PThreadSleep(regs->data);          // delay
		}
		else
		{
    		CyU3PDebugPrint (4, "\rWrite 0x%x=0x%x \r\n",regs->addr,regs->data);
    		retCyU3 = sensor_write(regs->addr, regs->data);
    	}
		i++;
    	regs++;
	}
  return retCyU3;
}



static CyU3PReturnStatus_t sensor_read(
	unsigned short reg,
	unsigned short *value
	)
{
	int cnt=0;

	uint8_t highAddr;
    uint8_t lowAddr;
    uint8_t buf[2];


    CyU3PReturnStatus_t retCyU3;

	highAddr  = (uint8_t) ((reg & 0xFF00) >> 8);
	lowAddr   = (uint8_t) (reg & 0x00FF);


	retCyU3 = SensorRead2B(SENSOR_ADDR_RD, highAddr, lowAddr, buf);
	while((retCyU3 != CY_U3P_SUCCESS) && (cnt<3))
	{
		retCyU3 = SensorRead2B(SENSOR_ADDR_RD, highAddr, lowAddr, buf);
		cnt++;
	}

	*value = (unsigned short) ((buf[0] << 8) + buf[1]);

	if(cnt>0)
	{
		CyU3PDebugPrint(4, "\rSensor read retry = %d\n",cnt);
	}
	return retCyU3;
}

static CyU3PReturnStatus_t sensor_read_array(
	struct regval_list *regs,
	int array_size
	)
{
	unsigned short temp=0;
	int i=0;
	CyU3PReturnStatus_t retCyU3;

	for(i=0;i<array_size;i++,regs++)
	{
		if(regs->addr == REG_DLY)
		{
			CyU3PThreadSleep(regs->data);
		}
		retCyU3 = sensor_read(regs->addr,&temp);
		CyU3PDebugPrint (4, "\rRead 0x%x=0x%2x \r\n",regs->addr, temp);
  	}
  return retCyU3;
}


//
// FUNCTION NAME.
//     SensorReset
//
// FUNCTIONAL DESCRIPTION.
//     Reset the image sensor using GPIO.
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//

void SensorReset(void)
{
    CyU3PReturnStatus_t apiRetStatus;

    //
    //  Drive the GPIO low to reset the sensor.
    //
    apiRetStatus = CyU3PGpioSetValue(SENSOR_RESET_GPIO, CyFalse);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint(4, "GPIO Set Value Error, Error Code = %d\n", apiRetStatus);
        return;
    }

    //
    // Wait for some time to allow proper reset.
    //
    CyU3PThreadSleep(10);

    //
    // Drive the GPIO high to bring the sensor out of reset.
    //
    apiRetStatus = CyU3PGpioSetValue(SENSOR_RESET_GPIO, CyTrue);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint(4, "GPIO Set Value Error, Error Code = %d\n", apiRetStatus);
        return;
    }

    //
    // Delay the allow the sensor to power up.
    //
    CyU3PThreadSleep(10);
    return;
}

//
//   Verify that the sensor can be accessed over the I2C bus from FX3/CX3
//
uint8_t SensorI2cBusTest(void)
{
    //
    // The sensor ID register can be read here to verify sensor connectivity.
    //
    uint8_t buf[2];


//    CyU3PDebugPrint (4, "\rSensorI2cBusTest ================ \r\n");


#if 0
    //
    // Reading sensor ID
    //
    if (SensorRead2B(SENSOR_ADDR_RD, 0x00, 0x00, buf) == CY_U3P_SUCCESS)
    {
        if ((buf[0] == 0x01) && (buf[1] == 0x02))
        {
            return CY_U3P_SUCCESS;
        }
    }
#endif

    //
    // Read chip version register
    //
    if (SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x00, 0x12, 0x13) == CY_U3P_SUCCESS)
    {
        CyU3PDebugPrint (4, "\rSensorI2cBusTest: SensorRead2B %x %x \r\n",buf[0],buf[1]);
        if (SensorRead2B(SENSOR_ADDR_RD, 0x30, 0x00, buf) == CY_U3P_SUCCESS)
        {
    	    CyU3PDebugPrint (4, "\rrSensorI2cBusTest: SensorRead2B %x %x \r\n",buf[0],buf[1]);
    	    return CY_U3P_SUCCESS;
    	}
    	else
    	{
    	    CyU3PDebugPrint (4, "\rSensorI2cBusTest: SensorRead2B fail \r\n");
    	}
        return CY_U3P_SUCCESS;
    }
    else
    {
        CyU3PDebugPrint (4, "\rSensorI2cBusTest: SensorWrite2B fail \r\n");
    }

//    if (SensorRead2B(SENSOR_ADDR_RD, 0x30, 0x00, buf) == CY_U3P_SUCCESS)
//    {
//        CyU3PDebugPrint (4, "\rrSensorI2cBusTest: SensorRead2B %x %x \r\n",buf[0],buf[1]);
//        return CY_U3P_SUCCESS;
//    }
//    else
//    {
//        CyU3PDebugPrint (4, "\rSensorI2cBusTest: SensorRead2B fail \r\n");
//    }

    return 1;
}


void PLL_SETUP(void)
{

	// Assuming Input Clock of 24MHz.  Output Clock will be 98MHz
	// Op_pix_clk_div and op_sys_clk_div are ignored in parallel mode



}

//
// 1280x720
//
/*
void MIPI_SETUP(void0)
{
    uint8_t    buf[2];

    //
	// Disable Streaming
    //

    SensorRead2B(SENSOR_ADDR_RD, 0x30, 0x1A, buf);
    buf[0] = buf[0] & DISABLE_BYTE_BIT_2_MASK;
    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x1A, buf[1], buf[0]);

    //
    // MIPI output
    //

    SensorWrite2B(SENSOR_ADDR_WR, 0x31, 0xAE, 0x02, 0x04);

    //
    // array readout setting
    //

    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x04, 0, 6);                   // X_ADDR_START

    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x08, 0x09, 0x05);             // X_ADDR_END

    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x02, 0, 120);                 // Y_ADDR_START

    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x08, 0x05, 0x87);             // Y_ADDR_END

    //
    // Sub-Sampling
    //

    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0xA2, 0, 0x01);                // X_ODD_INCREMENT
    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0xA6, 0, 0x01);                // Y_ODD_INCREMENT

    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0xA6, 0, 0x01);                // Y_ODD_INCREMENT

    SensorRead2B(SENSOR_ADDR_RD, 0x30, 0x40, buf);
    buf[1] = buf[1] | 0x10;
    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0xA6, buf[1], buf[0]);         // Row Bin

    SensorRead2B(SENSOR_ADDR_RD, 0x30, 0x40, buf);
    buf[1] = buf[1] | 0x20;
    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x40, buf[1], buf[0]);         // Column Bin

    SensorRead2B(SENSOR_ADDR_RD, 0x30, 0x40, buf);
    buf[1] = buf[1] | 0x02;
    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x40, buf[1], buf[0]);         // Column SF Bin

    SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xD4, 0x8F, 0x6C);             // GrGb improved setting for non-binning

    SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xD6, 0x66, 0xCC);             // GrGb improved setting for non-binning

    //
    // Frame-Timing
    //

    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x0C, 0x41, 0x16);             // Line Length PCK

    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x0A, 0x03, 0x28);             // Frame Length Lines

    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x14, 0x00, 0x00);             // Fine Integration Time

    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x12, 0x03, 0x27);             // Coarse Integration Time

    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x42, 0x02, 0xB0);             // Extra_Delay

	return;

}
*/

void CAMERA_Debug(void)
{

    uint8_t buf[2];
    int regdata16;
    int i;

    uint16_t tempdata;
    uint16_t tempaddr;


	tempaddr = 0x3000;
    for(i=0;i<256;i++)
    {
    	sensor_read(tempaddr, &tempdata);
    	CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);
    	tempaddr = tempaddr + 2;
    }

	tempaddr = 0x3ED0;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

	tempaddr = 0x3ED2;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

	tempaddr = 0x3ED4;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

	tempaddr = 0x3ED6;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

	tempaddr = 0x3ED8;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

	tempaddr = 0x3EDA;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

	tempaddr = 0x3EDC;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

	tempaddr = 0x3EDE;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

	tempaddr = 0x3EE0;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

	tempaddr = 0x3EE6;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

	tempaddr = 0x3EE8;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);


	tempaddr = 0x3EEA;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

	tempaddr = 0x3F06;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

	tempaddr = 0x305E;
	sensor_read(tempaddr, &tempdata);
	CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);


}

//
// FUNCTION NAME.
//      CAMERA_Base_Config
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
void CAMERA_Base_Config(void)
{
    uint8_t buf[2];
    int regdata16;

    uint16_t tempdata;
    uint16_t tempaddr;


    //
    // Reset Sensor
    //
    tempaddr = 0x301A;
    tempdata = 0x0059;
    sensor_write(tempaddr, tempdata);

    CyU3PThreadSleep(100);

    //
    // 2. read sensor ID and check protocol
    //

    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x00, 0x00, 0x00);
    CyU3PDebugPrint (4, "\rCamera_API : CAMERA_Base_Config \r\n");

    tempaddr = 0x3000;
    sensor_read(tempaddr, &tempdata);
    CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

    // check OTPM Version
    tempaddr = 0x300E;
    sensor_read(tempaddr, &tempdata);
    CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

    tempaddr = 0x30F0;
    sensor_read(tempaddr, &tempdata);
    CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

    tempaddr = 0x3072;
    sensor_read(tempaddr, &tempdata);
    CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);



    // Setup PLL/Timing/Address Settings
//    sensor_write_array(AR0330_SetOtherRegs, 40);
    sensor_write_array(AR0330_SetOtherRegs, 48);

    // setup OTPMv5 register
    sensor_write_array(AR0330_OTPMVsettingsV5, 4);

    // PLL setting for 1 Lane
 //   sensor_write_array(AR0330_setPLLRges1Lane, 4);


    // PLL setting for 2 Lane
//    sensor_write_array(AR0330_setPLLRges2Lane, 4);

    // using testing pattern
 //   sensor_write_array(test_pattern_setting, 1);


//     sensor_write_array(sensor_default_regs_OTPM_V5, 34);

//    tempaddr = 0x301A;
//    sensor_read(tempaddr, &tempdata);
//    CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

//    tempaddr = 0x3048;
//    sensor_read(tempaddr, &tempdata);
//    CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

#if 0

    //
    // 2. read sensor ID and check protocol
    //
    CyU3PDebugPrint (4, "\rCamera_API : Reset Sensor \r\n");
    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x1A, 0x00, 0x01);     // Reset Sensor

    CyU3PThreadSleep(200);

    //
    // 3. configure the output interface
    //
    CyU3PDebugPrint (4, "\rCamera_API : Reset Sensor \r\n");
    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x1A, 0x02, 0x04);


    //
    // 4. Disable Streaming
    //
    SensorRead2B(SENSOR_ADDR_RD, 0x30, 0x1A, buf);
    buf[0] = buf[0] | 0x04;
    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x1A, buf[1], buf[0]);

    CyU3PThreadSleep(200);

    //
    // 5. Drive Pins
    //
    SensorRead2B(SENSOR_ADDR_RD, 0x30, 0x1A, buf);
    buf[0] = buf[0] | 0x40;
    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x1A, buf[1], buf[0]);

    //
    // 6. Parallel Enable
    //
    SensorRead2B(SENSOR_ADDR_RD, 0x30, 0x1A, buf);
    buf[0] = buf[0] | 0x80;
    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x1A, buf[1], buf[0]);

    //
    // 6. SMIA Serializer Disable
    //
    SensorRead2B(SENSOR_ADDR_RD, 0x30, 0x1A, buf);
    buf[1] = buf[1] | 0x10;
    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x1A, buf[1], buf[0]);

    //
    // 7. Disable Embedded Data
    //
    SensorRead2B(SENSOR_ADDR_RD, 0x30, 0x64, buf);
    buf[1] = buf[1] | 0x01;
    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x64, buf[1], buf[0]);

    //
    // Load default registers
    //

    // Set test_pattern GreenB to "1" to indicate that the default register macro has been run
    // All mode macros will test for "1" in 0x3078.
    // Default initialization sequence will be run if 0x3078 does equals "0".

    SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x78, 0x00, 0x01);  //Marker to say that 'Defaults' have been run

    //Check OTPM Version.  If R0x3072 is "8" or greater, then do not write default register changes.
    //Only OTPM V5 part do not need write default register changes.

    SensorRead2B(SENSOR_ADDR_RD, 0x30, 0x64, buf);
    regdata16 = (buf[1] << 8) + buf[0];
    CyU3PDebugPrint (4, "\rCamera_API : OTPM version 0x%x \r\n", regdata16);

    if(regdata16 < 0x08)
    {
        //
    	// Default Register Changes - OTPM V2
    	//
    	CyU3PDebugPrint (4, "\rCamera_API : Start default register changes - OTPM V2 \r\n");
        SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0xBA, 0x00, 0x2C);

        SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0xFE, 0x00, 0x80);

        SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0xE0, 0x00, 0x03);     // changed form 0x0000 to 0x00003 to match ODS spec test conditions and correct fused defect clusters

        SensorRead2B(SENSOR_ADDR_RD, 0x3E, 0xCE, buf);
        SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0xE0, buf[1], 0xFF);   // RESERVED - Do not write to MASK 0xFF00

        SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xD0, 0xE4, 0xF6);

        SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xD2, 0x01, 0x46);     //0x0146 Back to defualt due to cisco concern about unsat pixels

        SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xD4, 0x8F, 0x6C);

        SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xD6, 0x66, 0xCC);     // 0x6666

        SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xD8, 0x8C, 0x42);     // 0x8642

        SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xDA, 0x88, 0x9B);

        SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xDC, 0x88, 0x63);

        SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xDE, 0xAA, 0x04);

        SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xE0, 0x15, 0xF0);

        SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xE6, 0x00, 0x8C);

        SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xE8, 0x20, 0x24);

        SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xEA, 0xFF, 0x1F);

        SensorWrite2B(SENSOR_ADDR_WR, 0x3F, 0x06, 0x04, 0x6A);

        //
        //  Below updates will fix eclipse issue.
        //  For Again of 1x, 2x, 4x, and 8x, it needs Dgain of 1.04x (Dgain register REG0x305E set to 0x0085),
        //  1.06x (0x0088), 1.14x (0x0092) and 1.25x (0x00A0) respectively. This INI only provide maximum Dgain.
        //
        SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xDA, 0x88, 0xBC);

        SensorWrite2B(SENSOR_ADDR_WR, 0x3E, 0xDC, 0xAA, 0x63);

        SensorWrite2B(SENSOR_ADDR_WR, 0x30, 0x5E, 0x00, 0xA0);
    }
#endif

    return;
}



//
// FUNCTION NAME.
//      CAMERA_VGA_config
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
void CAMERA_VGA_config(void)
{
    CyU3PDebugPrint (4, "\rCAMERA_VGA_config ============= \r\n");




    return;
}


//
// FUNCTION NAME.
//      CAMERA_VGA_HS_config
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
void CAMERA_VGA_HS_config(void)
{
    uint16_t tempdata;
    uint16_t tempaddr;

//    tempaddr = 0x3070;
//    tempdata = 0x0003;
//    sensor_write(tempaddr, tempdata);

    CyU3PDebugPrint (4, "\rCAMERA_VGA_HS_config ============= \r\n");
//    sensor_write_array(sensor_vga_regs, 16);


    tempaddr = 0x301A;
//    tempdata = 0x105C;
    tempdata = 0x005C;
//    tempdata = 0x001C;
//    tempaddr = 0x301C;
//    tempdata = 0x0001;

    sensor_write(tempaddr, tempdata);


//    tempaddr = 0x3048;
//    sensor_read(tempaddr, &tempdata);
//    CyU3PDebugPrint (4, "\rCAMERA_Base_Config : read address = 0x%x data = 0x%x \r\n",tempaddr,tempdata);

    CAMERA_Debug();

    return;
}


//
// FUNCTION NAME.
//      CAMERA_720P_config
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
void CAMERA_720P_config(void)
{
    CyU3PDebugPrint (4, "\rCAMERA_720P_config ============= \r\n");
    sensor_write_array(sensor_720p_regs, 18);


    return;
}

//
// FUNCTION NAME.
//      CAMERA_1080P_config
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
void CAMERA_1080P_config(void)
{
    CyU3PDebugPrint (4, "\rCAMERA_1080P_config ============= \r\n");

    sensor_write_array(sensor_1080p_regs, 16);

    return;
}

//
// FUNCTION NAME.
//      CAMERA_5MP_config
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
void CAMERA_5MP_config(void)
{
    CyU3PDebugPrint (4, "\rCAMERA_5MP_config ============= \r\n");

    return;
}

//
// FUNCTION NAME.
//      CAMERA_Power_Down
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//

void CAMERA_Power_Down(void)
{
    CyU3PDebugPrint (4, "\rCAMERA_Power_Down ================ \r\n");


    return;
}


//
// FUNCTION NAME.
//      CAMERA_Power_Up
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
void CAMERA_Power_Up(void)
{
    CyU3PDebugPrint (4, "\rCAMERA_Power_Up ============= \r\n");





    return;
}

//
// FUNCTION NAME.
//      CAMERA_Auto_Focus_Config
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
void CAMERA_Auto_Focus_Config(void)
{

    CyU3PDebugPrint (4, "\rCAMERA_Auto_Focus_Config ================ \r\n");



    return;
}


//
// FUNCTION NAME.
//      CAMERA_SetBrightness
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
void    CAMERA_SetBrightness(int8_t Brightness)
{
    CyU3PDebugPrint (4, "\rCAMERA_SetBrightness ================ \r\n");



    return;
}

//
// FUNCTION NAME.
//      CAMERA_GetBrightness
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
int8_t  CAMERA_GetBrightness(uint8_t option)
{
    CyU3PDebugPrint (4, "\rCAMERA_GetBrightness ================ \r\n");

    return 0;
}

//
// FUNCTION NAME.
//      CAMERA_SetContrast
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
void    CAMERA_SetContrast(int8_t Contrast)
{
    CyU3PDebugPrint (4, "\rCAMERA_SetContrast ================ \r\n");


    return;
}



// FUNCTION NAME.
//      CAMERA_GetContrast
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
int8_t  CAMERA_GetContrast(int8_t option)
{
	CyBool_t clr_error_count;
	CyU3PMipicsiErrorCounts_t error_count;

    CyU3PDebugPrint (4, "\rCAMERA_GetContrast ================ \r\n");


    CyU3PMipicsiGetErrors(clr_error_count, &error_count);
    CyU3PDebugPrint (4, "\rError count %d \r\n", clr_error_count);

    CyU3PDebugPrint (4, "\rfrmErrCnt =  %d \r\n", error_count.frmErrCnt);
    CyU3PDebugPrint (4, "\rcrcErrCnt =  %d \r\n", error_count.crcErrCnt);
    CyU3PDebugPrint (4, "\rmdlErrCnt =  %d \r\n", error_count.mdlErrCnt);
    CyU3PDebugPrint (4, "\rctlErrCnt =  %d \r\n", error_count.ctlErrCnt);

    CyU3PDebugPrint (4, "\reidErrCnt =  %d \r\n", error_count.eidErrCnt);
    CyU3PDebugPrint (4, "\rrecrErrCnt =  %d \r\n", error_count.recrErrCnt);
    CyU3PDebugPrint (4, "\runrcErrCnt =  %d \r\n", error_count.unrcErrCnt);
    CyU3PDebugPrint (4, "\rrecSyncErrCnt =  %d \r\n", error_count.recSyncErrCnt);
    CyU3PDebugPrint (4, "\runrSyncErrCnt =  %d \r\n", error_count.unrSyncErrCnt);


    return 0;
}

//
// FUNCTION NAME.
//      CAMERA_SetHue
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
void    CAMERA_SetHue(int32_t Hue)
{
    CyU3PDebugPrint (4, "\rCAMERA_SetHue ================ \r\n");


    return;
}

//
// FUNCTION NAME.
//      CAMERA_GetHue
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
int32_t CAMERA_GetHue(uint8_t option)
{
    CyU3PDebugPrint (4, "\rCAMERA_SetHue ================ \r\n");


    return 0;
}


//
// FUNCTION NAME.
//      CAMERA_SetSaturation
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
void    CAMERA_SetSaturation(uint32_t Saturation)
{
    CyU3PDebugPrint (4, "\rCAMERA_SetSaturation ================ \r\n");


    return;
}

//
// FUNCTION NAME.
//      CAMERA_GetSaturation
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
int8_t  CAMERA_GetSaturation(uint8_t option)
{
    CyU3PDebugPrint (4, "\rCAMERA_GetSaturation ================ \r\n");


    return 0;
}


//
// FUNCTION NAME.
//      CAMERA_SetSharpness
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
void    CAMERA_SetSharpness(uint8_t Sharp)
{
    CyU3PDebugPrint (4, "\rCAMERA_SetSharpness ================ \r\n");


    return;

}

//
// FUNCTION NAME.
//      CAMERA_GetSharpness
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
int8_t  CAMERA_GetSharpness(uint8_t option)
{
    CyU3PDebugPrint (4, "\rCAMERA_GetSharpness ================ \r\n");



    return 0;
}

//
// FUNCTION NAME.
//      CAMERA_SetWhiteBalance
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//

void    CAMERA_SetWhiteBalance(uint8_t WhiteBalance)
{
    CyU3PDebugPrint (4, "\rCAMERA_SetWhiteBalance ================ \r\n");

    return;
}


//
// FUNCTION NAME.
//      CAMERA_GetWhiteBalance
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//

uint8_t CAMERA_GetWhiteBalance(uint8_t option)
{
    CyU3PDebugPrint (4, "\rCAMERA_GetWhiteBalance ================ \r\n");



    return 0;
}

void    CAMERA_SetAutoWhiteBalance(uint8_t AutoWhiteBalance)
{
    CyU3PDebugPrint (4, "\rCAMERA_SetAutoWhiteBalance ================ \r\n");


	return;
}

//
// FUNCTION NAME.
//      CAMERA_GetAutoWhiteBalance
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
uint8_t CAMERA_GetAutoWhiteBalance(uint8_t option)
{
    CyU3PDebugPrint (4, "\rCAMERA_GetAutoWhiteBalance ================ \r\n");


    return 0;
}


void    CAMERA_SetExposure(int32_t Exposure)
{
    CyU3PDebugPrint (4, "\rCAMERA_SetExposure ================ \r\n");

	return;
}

//
// FUNCTION NAME.
//      CAMERA_GetExposure
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
int32_t CAMERA_GetExposure(uint8_t option)
{


	CyBool_t clr_error_count;
	CyU3PMipicsiErrorCounts_t error_count;

    CyU3PDebugPrint (4, "\rCAMERA_GetExposure ================ \r\n");

    CyU3PMipicsiGetErrors(clr_error_count, &error_count);
    CyU3PDebugPrint (4, "\rError count %d \r\n", clr_error_count);

    CyU3PDebugPrint (4, "\rfrmErrCnt =  %d \r\n", error_count.frmErrCnt);
    CyU3PDebugPrint (4, "\rcrcErrCnt =  %d \r\n", error_count.crcErrCnt);
    CyU3PDebugPrint (4, "\rmdlErrCnt =  %d \r\n", error_count.mdlErrCnt);
    CyU3PDebugPrint (4, "\rctlErrCnt =  %d \r\n", error_count.ctlErrCnt);

    CyU3PDebugPrint (4, "\reidErrCnt =  %d \r\n", error_count.eidErrCnt);
    CyU3PDebugPrint (4, "\rrecrErrCnt =  %d \r\n", error_count.recrErrCnt);
    CyU3PDebugPrint (4, "\runrcErrCnt =  %d \r\n", error_count.unrcErrCnt);
    CyU3PDebugPrint (4, "\rrecSyncErrCnt =  %d \r\n", error_count.recSyncErrCnt);
    CyU3PDebugPrint (4, "\runrSyncErrCnt =  %d \r\n", error_count.unrSyncErrCnt);

    return 0;
}


//
// FUNCTION NAME.
//      CAMERA_SetAutofocus
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
void    CAMERA_SetAutofocus(uint8_t Is_Enable)
{
    CyU3PDebugPrint (4, "\rCAMERA_SetAutofocus ================ \r\n");


    return;
}


//
// FUNCTION NAME.
//      CAMERA_GetAutofocus
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//
uint8_t CAMERA_GetAutofocus(uint8_t option)
{
    CyU3PDebugPrint (4, "\rCAMERA_GetAutofocus ================ \r\n");

    return 0;
}

//
// FUNCTION NAME.
//      CAMERA_SetManualfocus
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//

void    CAMERA_SetManualfocus(uint16_t manualfocus)
{
    CyU3PDebugPrint (4, "\rCAMERA_SetManualfocus ================ \r\n");

    return;
}


//
// FUNCTION NAME.
//      CAMERA_GetManualfocus
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//

uint16_t CAMERA_GetManualfocus(uint8_t option)
{
    CyU3PDebugPrint (4, "\rCAMERA_GetManualfocus ================ \r\n");

    return 0;
}


//
// FUNCTION NAME.
//      CAMERA_SetAutoExposure
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//

void    CAMERA_SetAutoExposure(uint8_t AutoExp)
{
    CyU3PDebugPrint (4, "\rCAMERA_SetAutoExposure ================ \r\n");

	return;
}

//
// FUNCTION NAME.
//      CAMERA_GetAutoExposure
//
// FUNCTIONAL DESCRIPTION.
//
//
// ENTRY PARAMETERS.
//      void
//
// EXIT PARAMETERS.
//      Function Return
//

uint8_t CAMERA_GetAutoExposure(uint8_t option)
{
    CyU3PDebugPrint (4, "\rCAMERA_GetAutoExposure ================ \r\n");



	return 0;
}







