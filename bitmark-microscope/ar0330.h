// Copyright Bitmark Inc. 2015-2015

#ifndef _AR0330_H_
#define _AR0330_H_ 1

#include <cyu3types.h>

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
void AR0330_SetBrightness(int32_t Brightness);
int32_t AR0330_GetBrightness(int32_t option);

void AR0330_SetContrast(int32_t Contrast);
int32_t AR0330_GetContrast(int32_t option);

void AR0330_SetHue(int32_t Hue);
int32_t AR0330_GetHue(int32_t option);

void AR0330_SetSaturation(int32_t Saturation);
int32_t AR0330_GetSaturation(int32_t option);

void AR0330_SetSharpness(int32_t Sharp);
int32_t AR0330_GetSharpness(int32_t option);

void AR0330_SetWhiteBalance(int32_t WhiteBalance);
int32_t AR0330_GetWhiteBalance(int32_t option);

void AR0330_SetAutoWhiteBalance(int32_t AutoWhiteBalance);
int32_t AR0330_GetAutoWhiteBalance(int32_t option);

void AR0330_SetExposure(int32_t Exposure);
int32_t AR0330_GetExposure(int32_t option);

void AR0330_SetAutoExposure(int32_t AutoExp);
int32_t AR0330_GetAutoExposure(int32_t option);

void AR0330_SetAutofocus(int32_t Is_Enable);
int32_t AR0330_GetAutofocus(int32_t option);

void AR0330_SetManualfocus(int32_t manualfocus);
int32_t AR0330_GetManualfocus(int32_t option);


#endif
