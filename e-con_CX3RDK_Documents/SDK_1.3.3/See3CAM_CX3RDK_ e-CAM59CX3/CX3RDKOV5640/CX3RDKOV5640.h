/*
 ## e-con Systems USB UVC Stack – See3CAMCX3RDK Platform

 ## source file : CX3RDKOV5640.h
 ## ===========================
 ##
 ##  Copyright E-Con Systems, 2013-2014,
 ##  All Rights Reserved
 ##  UNPUBLISHED, LICENSED SOFTWARE.
 ##
 ##  CONFIDENTIAL AND PROPRIETARY INFORMATION
 ##  PROPERTY OF ECON SYSTEMS

 ## www.e-consystems.com
 ##
 ##
 ## ===========================
*/

#ifndef _INCLUDED_CX3RDKOV5640_H_
#define _INCLUDED_CX3RDKOV5640_H_

#include "cyu3types.h"
#include "cyu3usbconst.h"
#include "cyu3os.h"
#include "cyu3utils.h"
#include "cyu3mipicsi.h"

#include "cyu3externcstart.h"

/* This header file comprises of the UVC application constants and
 * the video frame configurations */

#define UVC_APP_THREAD_STACK                    (0x1000)        /* Thread stack size */
#define UVC_APP_THREAD_PRIORITY                 (8)             /* Thread priority */

/* Endpoint definition for UVC application */
#define ES_UVC_EP_BULK_VIDEO                   	(0x83)          /* EP 1 IN */
#define ES_UVC_EP_CONTROL_STATUS               	(0x82)          /* EP 2 IN */

#define ES_UVC_EP_VIDEO_CONS_SOCKET            	(CY_U3P_UIB_SOCKET_CONS_3)      /* Consumer socket 1 */
#define ES_UVC_PRODUCER_PPORT_SOCKET_0         	(CY_U3P_PIB_SOCKET_0)           /* P-port Socket 0 is producer */
#define ES_UVC_PRODUCER_PPORT_SOCKET_1			(CY_U3P_PIB_SOCKET_1)           /* P-port Socket 0 is producer */

/* UVC descriptor types */
#define ES_UVC_INTRFC_ASSN_DESCR                (11)            /* Interface association descriptor type. */
#define ES_UVC_CS_INTRFC_DESCR					(0x24)          /* Class Specific Interface Descriptor type: CS_INTERFACE */

/* UVC video streaming endpoint packet Size */
#define ES_UVC_EP_BULK_VIDEO_PKT_SIZE			(0x400)

/* UVC Buffer Parameters*/
#define ES_UVC_SS_DATA_BUF_SIZE					(0x5FF0)        /* DMA Buffer Data Size Used: 12272 Bytes*/
#define ES_UVC_HS_DATA_BUF_SIZE					(3056)        	/* DMA Buffer Data Size Used: 3056 Bytes*/

#define ES_UVC_PROD_HEADER                     	(12)            /* UVC DMA Buffer Header Size */
#define ES_UVC_PROD_FOOTER						(4)             /* UVC DMA Buffer Footer Size */

/* UVC Buffer size - Will map to bulk Transaction size SuperSpeed*/
#define ES_UVC_SS_STREAM_BUF_SIZE				(ES_UVC_SS_DATA_BUF_SIZE + ES_UVC_PROD_HEADER + ES_UVC_PROD_FOOTER)

/* UVC Buffer size - Will map to bulk Transaction size HighSpeed*/
#define ES_UVC_HS_STREAM_BUF_SIZE				(ES_UVC_HS_DATA_BUF_SIZE + ES_UVC_PROD_HEADER + ES_UVC_PROD_FOOTER)

/* UVC Buffer count */
#define ES_UVC_SS_STREAM_BUF_COUNT              (4)
#define ES_UVC_HS_STREAM_BUF_COUNT				(8)

/* Low byte - UVC video streaming endpoint packet size */
#define ES_UVC_EP_BULK_VIDEO_PKT_SIZE_L        	CY_U3P_GET_LSB(ES_UVC_EP_BULK_VIDEO_PKT_SIZE)

/* High byte - UVC video streaming endpoint packet size */
#define ES_UVC_EP_BULK_VIDEO_PKT_SIZE_H        	CY_U3P_GET_MSB(ES_UVC_EP_BULK_VIDEO_PKT_SIZE)

#define ES_UVC_HEADER_LENGTH                   	(ES_UVC_PROD_HEADER)           /* Maximum number of header bytes in UVC */
#define ES_UVC_HEADER_DEFAULT_BFH              	(0x8C)                          /* Default BFH(Bit Field Header) for the UVC Header */

#define ES_UVC_MAX_PROBE_SETTING               	(26)            /* UVC 1.0 Maximum number of bytes in Probe Control */
#define ES_UVC_MAX_PROBE_SETTING_ALIGNED       	(32)            /* Maximum number of bytes in Probe Control aligned to 32 byte */


#define ES_UVC_MAX_STILL_PROBE_SETTING			(11)			/* UVC 1.1 still probe control*/
#define ES_UVC_MAX_STILL_PROBE_SETTING_ALIGNED	(32)			/* UVC 1.1 still probe control*/
#define ES_UVC_STILL_TRIGGER_COUNT				(1)			/* UVC 1.1 still probe control*/
#define ES_UVC_STILL_TRIGGER_ALIGNED			(16)			/* UVC 1.1 still probe control*/

#define ES_UVC_HEADER_FRAME                    	(0)                             /* Normal frame indication */
#define ES_UVC_HEADER_EOF                      	(uint8_t)(1 << 1)               /* End of frame indication */
#define ES_UVC_HEADER_FRAME_ID                 	(uint8_t)(1 << 0)               /* Frame ID toggle bit */

#define ES_UVC_HEADER_STILL_IMAGE				(uint8_t)(1<<5)

#define ES_UVC_USB_GET_CUR_REQ              	(uint8_t)(0x81)                 /* UVC GET_CUR request */
#define ES_UVC_USB_SET_CUR_REQ              	(uint8_t)(0x01)                 /* UVC SET_CUR request */
#define ES_UVC_USB_GET_MIN_REQ              	(uint8_t)(0x82)                 /* UVC GET_MIN Request */
#define ES_UVC_USB_GET_MAX_REQ              	(uint8_t)(0x83)                 /* UVC GET_MAX Request */
#define ES_UVC_USB_GET_RES_REQ              	(uint8_t)(0x84)                 /* UVC GET_RES Request */
#define ES_UVC_USB_GET_INFO_REQ             	(uint8_t)(0x86)                 /* UVC GET_INFO Request */
#define ES_UVC_USB_GET_DEF_REQ					(uint8_t)(0x87)                 /* UVC GET_DEF Request */

#define ES_UVC_VS_PROBE_CONTROL                	(0x0100)                        /* Video Stream Probe Control Request */
#define ES_UVC_VS_COMMIT_CONTROL               	(0x0200)                        /* Video Stream Commit Control Request */
#define ES_UVC_STILL_PROBE_CONTROL              (0x0300)                        /* Video STILL Probe Control Request */
#define ES_UVC_STILL_COMMIT_CONTROL             (0x0400)                        /* Video STILL Commit Control Request */
#define ES_UVC_STILL_TRIGGER               		(0x0500)                        /* Video STILL trigger Request */

#define ES_UVC_VC_REQUEST_ERROR_CODE_CONTROL   (0x0200)                        /* Request Control Error Code*/
#define ES_UVC_ERROR_INVALID_CONTROL           (0x06)                          /* Error indicating invalid control requested*/
#define ES_UVC_STREAM_INTERFACE                (0x01)                          /* Streaming Interface : Alternate setting 1 */
#define ES_UVC_CONTROL_INTERFACE               (0x00)                          /* Control Interface: Alternate Setting 0 */

#define ES_UVC_GPIF_SWITCH_TIMEOUT             (2)             /* Timeout setting for the switch operation in GPIF clock cycles */
#define ES_UVC_INVALID_GPIF_STATE              (257)           /* Invalid state for use in CyU3PGpifSMSwitch calls */

/* Extern definitions of the USB Enumeration constant arrays used for the Application */
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

/* UVC Probe Control Setting */
extern uint8_t glProbeCtrl[ES_UVC_MAX_PROBE_SETTING];
extern const uint8_t gl720pProbeCtrl[ES_UVC_MAX_PROBE_SETTING];
extern const uint8_t gl1080pProbeCtrl[ES_UVC_MAX_PROBE_SETTING];
extern const uint8_t glVga60ProbeCtrl[ES_UVC_MAX_PROBE_SETTING];
extern const uint8_t glVga30ProbeCtrl[ES_UVC_MAX_PROBE_SETTING];
extern const uint8_t gl5MpProbeCtrl[ES_UVC_MAX_PROBE_SETTING];

extern uint8_t glStillProbeCtrl[ES_UVC_MAX_STILL_PROBE_SETTING];

/* MIPI Configuration parameters */
extern CyU3PMipicsiCfg_t cfgUvc1080p30NoMclk, cfgUvc720p60NoMclk,  cfgUvcVgaNoMclk, cfgUvc5Mp15NoMclk ,cfgUvcVga30NoMclk;


CyU3PThread uvcAppThread;               	/* Primary application thread used for data transfer from the Mipi interface to USB*/
CyU3PEvent glTimerEvent;                  	/* Application Event Group */


#include "cyu3externcend.h"


#endif /* _INCLUDED_CX3RDKOV5640_H_ */
