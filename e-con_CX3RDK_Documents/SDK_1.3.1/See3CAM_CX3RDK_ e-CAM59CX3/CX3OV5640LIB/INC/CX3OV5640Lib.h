/*
 * CX3OV5640Lib.h
 *
 *  Created on: 18-Sep-2013
 *      Author: Econsystems
 */

#ifndef CX3OV5640LIB_H_
#define CX3OV5640LIB_H_


#include "cyu3types.h"
#include "cyu3externcstart.h"

int32_t gl32SetControl;
int16_t gl16SetControl;

#define GET_DEF 0x07
#define GET_MIN	0x02
#define GET_MAX 0x03
#define GET_CUR 0x01
#define GET_RES 0x04


//Camera Configuration functions
void esOV5640_Base_Config(void);
void esOV5640_VGA_config(void);
void esOV5640_VGA_HS_config(void);
void esOV5640_720P_config(void);
void esOV5640_1080P_config(void);
void esOV5640_5MP_config(void);
void esCamera_Power_Down(void);
void esCamera_Power_Up(void);

//Brighntess
void esOV5640_SetBrightness(int8_t Brightness);
int8_t esOV5640_GetBrightness(uint8_t option);
//Contrast
void esOV5640_SetContrast(int8_t Contrast);
int8_t esOV5640_GetContrast(uint8_t option);
//Hue
void esOV5640_SetHue(int32_t Hue);
int32_t esOV5640_GetHue(uint8_t option);
//Saturation
void esOV5640_SetSaturation(uint32_t Saturation);
int8_t esOV5640_GetSaturation(uint8_t option);
//Sharpness
void esOV5640_SetSharpness(uint8_t Sharp);
int8_t esOV5640_GetSharpness(uint8_t option);
//White Balance Manual
void esOV5640_SetWhiteBalance(uint8_t WhiteBalance);
uint8_t esOV5640_GetWhiteBalance(uint8_t option);
//White Balance Auto
void esOV5640_SetAutoWhiteBalance(uint8_t AutoWhiteBalance);
uint8_t esOV5640_GetAutoWhiteBalance(uint8_t option);
//Manual Exposure
void esOV5640_SetExposure(int32_t Exposure);
int32_t esOV5640_GetExposure(uint8_t option);
//Auto Exposure
uint8_t esOV5640_GetAutoExposure(uint8_t option);
void esOV5640_SetAutoExposure(uint8_t AutoExp);
//AutoFocus
void esOV5640_SetAutofocus(uint8_t Is_Enable);
uint8_t esOV5640_GetAutofocus(uint8_t option);
void esOV5640_Auto_Focus_Config(void);
//Manual Focus
void esOV5640_SetManualfocus(uint16_t manualfocus);
uint16_t esOV5640_GetManualfocus(uint8_t option);

//NightMode
void esOV5640_Nightmode(CyBool_t Enable,uint8_t NightMode_option);
#endif /* CX3OV5640LIB_H_ */
