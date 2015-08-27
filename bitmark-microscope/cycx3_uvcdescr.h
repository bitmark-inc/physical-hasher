// Cypress CX3 Firmware Example Header (cycx3_uvcdescr.h)
// ===========================
//
//  Copyright Cypress Semiconductor Corporation, 2013-2014,
//  All Rights Reserved
//  UNPUBLISHED, LICENSED SOFTWARE.
//
//  CONFIDENTIAL AND PROPRIETARY INFORMATION
//  WHICH IS THE PROPERTY OF CYPRESS.
//
//  Use of this file is governed
//  by the license agreement included in the file
//
//     <install>/license/license.txt
//
//  where <install> is the Cypress software
//  installation root directory path.
//
// ===========================

#if !defined(_CYCX3_UVCDESCR_H_)
#define _CYCX3_UVCDESCR_H_ 1

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3os.h"
#include "cyu3utils.h"
#include "cyu3mipicsi.h"

#include "cyu3externcstart.h"

// Extern definitions of the USB Enumeration constant arrays used for the Application
extern const uint8_t esUVCUSB20DeviceDscr[];
extern const uint8_t esUVCUSB30DeviceDscr[];
extern const uint8_t esUVCUSBDeviceQualDscr[];
extern const uint8_t esUVCUSBFSConfigDscr[];
extern const uint8_t esUVCUSBHSConfigDscr[];
extern const uint8_t esUVCUSBBOSDscr[];
extern const uint8_t esUVCUSBSSConfigDscr[];
extern const uint8_t esUVCUSBStringLangIDDscr[];
extern const uint8_t esUVCUSBManufactureDscr[];
extern const uint8_t esUVCUSBProductDscr[];
extern const uint8_t esUVCUSBConfigSSDscr[];
extern const uint8_t esUVCUSBConfigHSDscr[];
extern const uint8_t esUVCUSBConfigFSDscr[];

// UVC Probe Control Setting
extern uint8_t glProbeCtrl[ES_UVC_MAX_PROBE_SETTING];
extern const uint8_t gl720pProbeCtrl[ES_UVC_MAX_PROBE_SETTING];
extern const uint8_t gl1080pProbeCtrl[ES_UVC_MAX_PROBE_SETTING];
extern const uint8_t glVga60ProbeCtrl[ES_UVC_MAX_PROBE_SETTING];
extern const uint8_t glVga30ProbeCtrl[ES_UVC_MAX_PROBE_SETTING];
extern const uint8_t gl5MpProbeCtrl[ES_UVC_MAX_PROBE_SETTING];

extern uint8_t glStillProbeCtrl[ES_UVC_MAX_STILL_PROBE_SETTING];

// MIPI Configuration parameters
extern CyU3PMipicsiCfg_t cfgUvc1080p30NoMclk, cfgUvc720p60NoMclk,  cfgUvcVgaNoMclk, cfgUvc5Mp15NoMclk ,cfgUvcVga30NoMclk;


#include "cyu3externcend.h"


#endif
