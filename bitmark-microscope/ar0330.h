// Copyright Bitmark Inc. 2015-2015

#ifndef _AR0330_H_
#define _AR0330_H_ 1

#include "cyu3types.h"
#if 0
#include "cyu3externcstart.h"

// AR0330 register
#define AR0330_CHIP_VERSION_REG_ADDR         0x3000
#define AR0330_CHIP_VERSION_REG_ADDR_H       0x30
#define AR0330_CHIP_VERSION_REG_ADDR_L       0x00


/* #define SET_BIT_0        0 */
/* #define SET_BIT_1        1 */
/* #define SET_BIT_2        2 */
/* #define SET_BIT_3        3 */
/* #define SET_BIT_4        4 */
/* #define SET_BIT_5        5 */
/* #define SET_BIT_6        6 */
/* #define SET_BIT_7        7 */

/* #define SET_BYTE         8 */
/* #define SET_WORD         9 */
/* #define SET_DWORD        10 */


//
// I2C Slave address for the image sensor.   (TBD)
//
#define SENSOR_ADDR_WR               0x20         // Slave address used to write sensor registers.
#define SENSOR_ADDR_RD               0x21         // Slave address used to read from sensor registers.

#define I2C_SLAVEADDR_MASK           0xFE         // Mask to get actual I2C slave address value without direction bit.


#define DISABLE_BYTE_BIT_0_MASK      0xFE
#define DISABLE_BYTE_BIT_1_MASK      0xFD
#define DISABLE_BYTE_BIT_2_MASK      0xFB
#define DISABLE_BYTE_BIT_3_MASK      0xF7
#define DISABLE_BYTE_BIT_4_MASK      0xEF
#define DISABLE_BYTE_BIT_5_MASK      0xDF
#define DISABLE_BYTE_BIT_6_MASK      0xBF
#define DISABLE_BYTE_BIT_7_MASK      0x7F

#define ENABLE_BYTE_BIT_0            0x01
#define ENABLE_BYTE_BIT_1            0x02
#define ENABLE_BYTE_BIT_2            0x04
#define ENABLE_BYTE_BIT_3            0x08
#define ENABLE_BYTE_BIT_4            0x10
#define ENABLE_BYTE_BIT_5            0x20
#define ENABLE_BYTE_BIT_6            0x40
#define ENABLE_BYTE_BIT_7            0x80


// standard Linux definition
#define EINVAL          22      // Invalid argument
#endif

// Configuration functions
void AR0330_Base_Config(void);
void AR0330_VGA_config(void);
void AR0330_VGA_HS_config(void);
void AR0330_720P_config(void);
void AR0330_1080P_config(void);
void AR0330_5MP_config(void);

void AR0330_Power_Down(void);
void AR0330_Power_Up(void);

void AR0330_Auto_Focus_Config(void);


// parameters
void AR0330_SetBrightness(int8_t Brightness);
int8_t AR0330_GetBrightness(uint8_t option);

void AR0330_SetContrast(int8_t Contrast);
int8_t AR0330_GetContrast(int8_t option);

void AR0330_SetHue(int32_t Hue);
int32_t AR0330_GetHue(uint8_t option);

void AR0330_SetSaturation(uint32_t Saturation);
int8_t AR0330_GetSaturation(uint8_t option);

void AR0330_SetSharpness(uint8_t Sharp);
int8_t AR0330_GetSharpness(uint8_t option);

void AR0330_SetWhiteBalance(uint8_t WhiteBalance);
uint8_t AR0330_GetWhiteBalance(uint8_t option);

void AR0330_SetAutoWhiteBalance(uint8_t AutoWhiteBalance);
uint8_t AR0330_GetAutoWhiteBalance(uint8_t option);

void AR0330_SetExposure(int32_t Exposure);
int32_t AR0330_GetExposure(uint8_t option);

void AR0330_SetAutoExposure(uint8_t AutoExp);
uint8_t AR0330_GetAutoExposure(uint8_t option);

void AR0330_SetAutofocus(uint8_t Is_Enable);
uint8_t AR0330_GetAutofocus(uint8_t option);

void AR0330_SetManualfocus(uint16_t manualfocus);
uint16_t AR0330_GetManualfocus(uint8_t option);


#endif
