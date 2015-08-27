//
// FILENAME.
//      Camera_API.h - Camera general API header
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


#ifndef _CAMERA_LIB_H_
#define _CAMERA_LIB_H_

#include "cyu3types.h"
#include "cyu3externcstart.h"

#define GET_DEF 0x01
#define GET_MIN	0x02
#define GET_MAX 0x03
#define GET_CUR 0x04
#define GET_RES 0x05


//
// AR0330 register
//
#define AR0330_CHIP_VERSION_REG_ADDR         0x3000
#define AR0330_CHIP_VERSION_REG_ADDR_H       0x30
#define AR0330_CHIP_VERSION_REG_ADDR_L       0x00


#define SET_BIT_0        0
#define SET_BIT_1        1
#define SET_BIT_2        2
#define SET_BIT_3        3
#define SET_BIT_4        4
#define SET_BIT_5        5
#define SET_BIT_6        6
#define SET_BIT_7        7

#define SET_BYTE         8
#define SET_WORD         9
#define SET_DWORD        10


//
// register structure
//
struct reg_list_a16_d16 {
  unsigned short addr;
  unsigned short data;
};


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


//
// standard Linux ddefnition
//

#define EINVAL          22      // Invalid argument

//
// GPIO 22 on FX3 is used to reset the Image sensor.  (TBD)
//

#define SENSOR_RESET_GPIO 37      // TBD




//
//Camera Configuration functions
//

void CAMERA_Base_Config(void);
void CAMERA_VGA_config(void);
void CAMERA_VGA_HS_config(void);
void CAMERA_720P_config(void);
void CAMERA_1080P_config(void);
void CAMERA_5MP_config(void);
void CAMERA_Power_Down(void);
void CAMERA_Power_Up(void);


void CAMERA_Auto_Focus_Config(void);



//
// Brighness
//
void    CAMERA_SetBrightness(int8_t Brightness);
int8_t  CAMERA_GetBrightness(uint8_t option);


//
// Contast
//
void    CAMERA_SetContrast(int8_t Contrast);
int8_t  CAMERA_GetContrast(int8_t option);

//
// Hue
//
void    CAMERA_SetHue(int32_t Hue);
int32_t CAMERA_GetHue(uint8_t option);

//
//Saturation
//
void    CAMERA_SetSaturation(uint32_t Saturation);
int8_t  CAMERA_GetSaturation(uint8_t option);

//
//Sharpness
//
void    CAMERA_SetSharpness(uint8_t Sharp);
int8_t  CAMERA_GetSharpness(uint8_t option);

//
//White Balance Manual
//
void    CAMERA_SetWhiteBalance(uint8_t WhiteBalance);
uint8_t CAMERA_GetWhiteBalance(uint8_t option);

//
//White Balance Auto
//
void    CAMERA_SetAutoWhiteBalance(uint8_t AutoWhiteBalance);
uint8_t CAMERA_GetAutoWhiteBalance(uint8_t option);

//
//Manual Exposure
//
void    CAMERA_SetExposure(int32_t Exposure);
int32_t CAMERA_GetExposure(uint8_t option);

//
// auto exposure
//
void    CAMERA_SetAutoExposure(uint8_t AutoExp);
uint8_t CAMERA_GetAutoExposure(uint8_t option);


//
//AutoFocus
//
void    CAMERA_SetAutofocus(uint8_t Is_Enable);
uint8_t CAMERA_GetAutofocus(uint8_t option);

//
//Manual Focus
//
void     CAMERA_SetManualfocus(uint16_t manualfocus);
uint16_t CAMERA_GetManualfocus(uint8_t option);


#endif // _CAMERA_LIB_H_
