// Cypress CX3 Firmware Example Source (cycx3_uvc.c)
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

#include "cyu3system.h"
#include "cyu3os.h"
#include "cyu3dma.h"
#include "cyu3error.h"
#include "cyu3usb.h"
#include "cyu3i2c.h"
#include "cyu3uart.h"
#include "cyu3gpio.h"
#include "cyu3utils.h"
#include "cyu3pib.h"
#include "cyu3socket.h"
#include "sock_regs.h"
#include "cyu3mipicsi.h"

#include "cycx3_uvc.h"
#include "cycx3_uvcdescr.h"
#include "ar0330.h"
#include "focus.h"

// Event generated on Timer overflow
#define ES_TIMER_RESET_EVENT		(1<<4)

// Event generated on a USB Suspend Request
#define ES_USB_SUSP_EVENT_FLAG		(1<<5)

// Firmware version
#define MajorVersion 				1
#define MinorVersion 				3
#define SubVersion			      131
#define SubVersion1			      308

#define RESET_TIMER_ENABLE 1

#ifdef RESET_TIMER_ENABLE
#define TIMER_PERIOD	(500)

static CyU3PTimer UvcTimer;

static void UvcAppProgressTimer(uint32_t arg) {
	// This frame has taken too long to complete.
	// Abort it, so that the next frame can be started.
	CyU3PEventSet(&glTimerEvent, ES_TIMER_RESET_EVENT, CYU3P_EVENT_OR);
}
#endif

volatile int32_t glDMATxCount = 0;	   // Counter used to count the Dma Transfers
volatile int32_t glDmaDone = 0;
volatile uint8_t glActiveSocket = 0;
volatile CyBool_t doLpmDisable = CyTrue;   // Flag used to Enable/Disable low USB 3.0 LPM
CyBool_t glHitFV = CyFalse;		   // Flag used for state of FV signal.
CyBool_t glMipiActive = CyFalse;	   // Flag set to true when Mipi interface is active. Used for Suspend/Resume.
CyBool_t glIsClearFeature = CyFalse;	   // Flag to signal when AppStop is called from the ClearFeature request. Need to Reset Toggle
CyBool_t glPreviewStarted = CyFalse;	   // Flag to support Mac os

// UVC Header
uint8_t glUVCHeader[ES_UVC_HEADER_LENGTH] = {
	0x0C,				   // Header Length
	0x8C,				   // Bit field header field
	0x00, 0x00, 0x00, 0x00,		   // Presentation time stamp field
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // Source clock reference field
};

// Video Probe Commit Control
uint8_t glCommitCtrl[ES_UVC_MAX_PROBE_SETTING_ALIGNED];
uint8_t glCurrentFrameIndex = 1;
uint8_t glStillCommitCtrl[ES_UVC_MAX_STILL_PROBE_SETTING_ALIGNED];
uint8_t glCurrentStillFrameIndex = 1;
uint8_t glStillTriggerCtrl = 0;
uint8_t glFrameIndexToSet = 0;
CyBool_t glStillCaptureStart = CyFalse;
CyBool_t glStillCaptured = CyFalse;
uint8_t glStillSkip = 0;

CyBool_t glIsApplnActive = CyFalse;	   // Whether the Mipi->USB application is active or not.
CyBool_t glIsConfigured = CyFalse;	   // Whether Application is in configured state or not
CyBool_t glIsStreamingStarted = CyFalse;   // Whether streaming has started - Used for MAC OS support

// DMA Channel
CyU3PDmaMultiChannel glChHandleUVCStream;  // DMA Channel Handle for UVC Stream
uint16_t ES_UVC_STREAM_BUF_SIZE = 0;
uint16_t ES_UVC_DATA_BUF_SIZE = 0;
uint8_t ES_UVC_STREAM_BUF_COUNT = 0;

uint8_t g_IsAutoFocus = 1;		   // Check the AutoFocus is Enabled or not

// USB control request processing variables
uint8_t glGet_Info = 0;
//int16_t gl8GetControl = 0;
//int16_t gl8SetControl = 0;
//int16_t gl16GetControl = 0;
//int16_t gl16SetControl = 0;
int32_t gl32GetControl = 0;
int32_t gl32SetControl = 0;

uint8_t glcommitcount = 0, glcheckframe = 1;

// Application critical error handler
void esUVCAppErrorHandler(CyU3PReturnStatus_t status) {
	// Application failed with the error code status

	// Add custom debug or recovery actions here

	// Loop indefinitely
	for (;;) {
		// Thread sleep : 100 ms
		CyU3PThreadSleep(100);
	}
}

// UVC header addition function
static void esUVCUvcAddHeader(uint8_t *buffer_p,    // Buffer pointer
			      uint8_t frameInd) {   // EOF or normal frame indication
	// Copy header to buffer
	CyU3PMemCopy(buffer_p, (uint8_t *)glUVCHeader, ES_UVC_HEADER_LENGTH);

	// Check if last packet of the frame.
	if (frameInd == ES_UVC_HEADER_EOF) {
		// Modify UVC header to toggle Frame ID
		glUVCHeader[1] ^= ES_UVC_HEADER_FRAME_ID;

		// Indicate End of Frame in the buffer
		buffer_p[1] |= ES_UVC_HEADER_EOF;
	}
}

// This function starts the video streaming application. It is called
// when there is a SET_INTERFACE event for alternate interface 1
// (in case of UVC over Isochronous Endpoint usage) or when a
// COMMIT_CONTROL(SET_CUR) request is received (when using BULK only UVC).
CyU3PReturnStatus_t esUVCUvcApplnStart(void) {
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

	glIsApplnActive = CyTrue;
	glDmaDone = 0;
	glDMATxCount = 0;
	glHitFV = CyFalse;
	doLpmDisable = CyTrue;

#ifdef RESET_TIMER_ENABLE
	CyU3PTimerStop(&UvcTimer);
#endif

	CyU3PDebugPrint(4, "esUVCUvcApplnStart:\r\n");

	// Place the EP in NAK mode before cleaning up the pipe.
	CyU3PUsbSetEpNak(ES_UVC_EP_BULK_VIDEO, CyTrue);
	CyU3PBusyWait(100);

	// Reset USB EP and DMA
	CyU3PUsbFlushEp(ES_UVC_EP_BULK_VIDEO);
	status = CyU3PDmaMultiChannelReset(&glChHandleUVCStream);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AplnStrt:ChannelReset Err = 0x%x\r\n", status);
		return status;
	}

	status = CyU3PDmaMultiChannelSetXfer(&glChHandleUVCStream, 0, 0);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AplnStrt:SetXfer Err = 0x%x\r\n", status);
		return status;
	}

	CyU3PUsbSetEpNak(ES_UVC_EP_BULK_VIDEO, CyFalse);
	CyU3PBusyWait(200);
//
//    // Place the EP in NAK mode before cleaning up the pipe.
//    CyU3PUsbSetEpNak (ES_UVC_EP_BULK_VIDEO, CyTrue);
//    CyU3PBusyWait (100);
//
//    // Reset USB EP and DMA
//    CyU3PUsbFlushEp(ES_UVC_EP_BULK_VIDEO);
//    status = CyU3PDmaMultiChannelReset (&glChHandleUVCStream);
//    if (CY_U3P_SUCCESS != status)
//    {
//        CyU3PDebugPrint (4,"AplnStrt:ChannelReset Err = 0x%x\r\n", status);
//        return status;
//    }
//    status = CyU3PDmaMultiChannelSetXfer (&glChHandleUVCStream, 0, 0);
//    if (CY_U3P_SUCCESS != status)
//    {
//        CyU3PDebugPrint (4, "AplnStrt:SetXfer Err = 0x%x\r\n", status);
//        return status;
//    }
//    CyU3PUsbSetEpNak (ES_UVC_EP_BULK_VIDEO, CyFalse);
//    CyU3PBusyWait (200);

	// Night Mode function
	//  --------------------
	//  esOV5640_Nightmode API is used to enable the Nightmode
	//  of OV5640 sensor.
	//  Set Enable -- Cytrue to enable Nightmode
	//                                CyFalse to Disable Nightmode
	//
	//  Set NightMode_option -- 1 to 6 to set different night modes
	//
	// To test different night modes, uncomment the below statement and build the firmware

	//esOV5640_Nightmode(CyTrue,3);

	// Resume the Fixed Function GPIF State machine
	CyU3PGpifSMControl(CyFalse);

	glActiveSocket = 0;
	CyU3PGpifSMSwitch(ES_UVC_INVALID_GPIF_STATE, CX3_START_SCK0,
			  ES_UVC_INVALID_GPIF_STATE, ALPHA_CX3_START_SCK0, ES_UVC_GPIF_SWITCH_TIMEOUT);

	CyU3PThreadSleep(10);

	// Wake Mipi interface and Image Sensor
	CyU3PMipicsiWakeup();

	AR0330_Power_Up();

	glMipiActive = CyTrue;

	if (glStillCaptureStart != CyTrue) {
		if (g_IsAutoFocus) {
			AR0330_SetAutofocus(g_IsAutoFocus);
		}
	}
	return CY_U3P_SUCCESS;
}

// This function stops the video streaming. It is called from the USB event
// handler, when there is a reset / disconnect or SET_INTERFACE for alternate
// interface 0 in case of ischronous implementation or when a Clear Feature (Halt)
// request is received (in case of bulk only implementation).
void esUVCUvcApplnStop(void) {
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

	// Update the flag so that the application thread is notified of this.
	glIsApplnActive = CyFalse;

	// Stop the image sensor and CX3 mipi interface
	status = CyU3PMipicsiSleep();

	AR0330_Power_Down();

	glMipiActive = CyFalse;

#ifdef RESET_TIMER_ENABLE
	CyU3PTimerStop(&UvcTimer);
#endif

	// Pause the GPIF interface
	CyU3PGpifSMControl(CyTrue);

	CyU3PUsbSetEpNak(ES_UVC_EP_BULK_VIDEO, CyTrue);
	CyU3PBusyWait(100);

	// Abort and destroy the video streaming channel
	// Reset the channel: Set to DSCR chain starting point in PORD/CONS SCKT; set DSCR_SIZE field in DSCR memory
	status = CyU3PDmaMultiChannelReset(&glChHandleUVCStream);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AplnStop:ChannelReset Err = 0x%x\r\n", status);
	}
	CyU3PThreadSleep(25);

	// Flush the endpoint memory
	CyU3PUsbFlushEp(ES_UVC_EP_BULK_VIDEO);
	// Clear the stall condition and sequence numbers if ClearFeature.
	if (glIsClearFeature) {
		CyU3PUsbStall(ES_UVC_EP_BULK_VIDEO, CyFalse, CyTrue);
		glIsClearFeature = CyFalse;
	}
	CyU3PUsbSetEpNak(ES_UVC_EP_BULK_VIDEO, CyFalse);
	CyU3PBusyWait(200);

	glDMATxCount = 0;
	glDmaDone = 0;

	// Enable USB 3.0 LPM
	CyU3PUsbLPMEnable();
}

// GpifCB callback function is invoked when FV triggers GPIF interrupt
void esUVCGpifCB(CyU3PGpifEventType event, uint8_t currentState) {
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
	// Handle interrupt from the State Machine

	//CyU3PDebugPrint(4, "GpifCB callback function\r\n");

	if (event == CYU3P_GPIF_EVT_SM_INTERRUPT) {
		//CyU3PDebugPrint(4, "CYU3P_GPIF_EVT_SM_INTERRUPT\r\n");

		if (currentState == CX3_PARTIAL_BUFFER_IN_SCK0) {
			// Wrapup Socket 0
			//CyU3PDebugPrint(4, "CX3_PARTIAL_BUFFER_IN_SCK0\r\n");

			status = CyU3PDmaMultiChannelSetWrapUp(&glChHandleUVCStream, 0);
			if (CY_U3P_SUCCESS != status) {
				CyU3PDebugPrint(4, "GpifCB:WrapUp SCK0 Err = 0x%x\r\n", status);
			}
		} else if (currentState == CX3_PARTIAL_BUFFER_IN_SCK1) {
			// Wrapup Socket 1
			//CyU3PDebugPrint(4, "CX3_PARTIAL_BUFFER_IN_SCK1\r\n");

			status = CyU3PDmaMultiChannelSetWrapUp(&glChHandleUVCStream, 1);
			if (CY_U3P_SUCCESS != status) {
				CyU3PDebugPrint(4, "GpifCB:WrapUp SCK1 Err = 0x%x\r\n", status);
			}
		}
	}
}

// DMA callback function to handle the produce and consume events.
void esUVCUvcAppDmaCallback(CyU3PDmaMultiChannel *chHandle, CyU3PDmaCbType_t type, CyU3PDmaCBInput_t *input) {
	CyU3PDmaBuffer_t DmaBuffer;
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

	if (type == CY_U3P_DMA_CB_PROD_EVENT) {
		// This is a produce event notification to the CPU. This notification is
		// received upon reception of every buffer. The buffer will not be sent
		// out unless it is explicitly committed. The call shall fail if there
		// is a bus reset / usb disconnect or if there is any application error.

		// Disable USB 3.0 LPM while Buffer is being transmitted out
		if ((CyU3PUsbGetSpeed() == CY_U3P_SUPER_SPEED) && (doLpmDisable)) {
			CyU3PUsbLPMDisable();
			CyU3PUsbSetLinkPowerState(CyU3PUsbLPM_U0);
			CyU3PBusyWait(200);

			doLpmDisable = CyFalse;
		}
#ifdef RESET_TIMER_ENABLE
		CyU3PTimerStart(&UvcTimer);
#endif

		status = CyU3PDmaMultiChannelGetBuffer(chHandle, &DmaBuffer, CYU3P_NO_WAIT);
		while (status == CY_U3P_SUCCESS) {
			// Add Headers
			if (DmaBuffer.count < ES_UVC_DATA_BUF_SIZE) {
				esUVCUvcAddHeader((DmaBuffer.buffer - ES_UVC_PROD_HEADER), ES_UVC_HEADER_EOF);
				glHitFV = CyTrue;
			} else {
				esUVCUvcAddHeader((DmaBuffer.buffer - ES_UVC_PROD_HEADER), ES_UVC_HEADER_FRAME);
			}

			// Commit Buffer to USB
			//status = CyU3PDmaMultiChannelCommitBuffer(chHandle, (DmaBuffer.count + 12), 0);
			status = CyU3PDmaMultiChannelCommitBuffer(chHandle, (DmaBuffer.count + ES_UVC_PROD_HEADER), 0);
			if (CY_U3P_SUCCESS != status) {
				CyU3PEventSet(&glTimerEvent, ES_TIMER_RESET_EVENT, CYU3P_EVENT_OR);
				break;
			} else {
				Focus_SetLine(glDMATxCount, DmaBuffer.buffer, DmaBuffer.count); // maybe capture some pixels
				glDMATxCount++;
				glDmaDone++;
			}

			glActiveSocket ^= 1;	// Toggle the Active Socket
			status = CyU3PDmaMultiChannelGetBuffer(chHandle, &DmaBuffer, CYU3P_NO_WAIT);
		}
	} else if (type == CY_U3P_DMA_CB_CONS_EVENT) {
		glDmaDone--;

		// Check if Frame is completely transferred
		glIsStreamingStarted = CyTrue;

		if ((glHitFV == CyTrue) && (glDmaDone == 0)) {
			glHitFV = CyFalse;
			Focus_EndFrame(glDMATxCount);  // mark end of frame
			glDMATxCount = 0;
#ifdef RESET_TIMER_ENABLE
			CyU3PTimerStop(&UvcTimer);
#endif

			if (glActiveSocket) {
				CyU3PGpifSMSwitch(ES_UVC_INVALID_GPIF_STATE, CX3_START_SCK1,
						  ES_UVC_INVALID_GPIF_STATE, ALPHA_CX3_START_SCK1,
						  ES_UVC_GPIF_SWITCH_TIMEOUT);
			} else {
				CyU3PGpifSMSwitch(ES_UVC_INVALID_GPIF_STATE, CX3_START_SCK0,
						  ES_UVC_INVALID_GPIF_STATE, ALPHA_CX3_START_SCK0,
						  ES_UVC_GPIF_SWITCH_TIMEOUT);
			}

			CyU3PUsbLPMEnable();
			doLpmDisable = CyTrue;
#ifdef RESET_TIMER_ENABLE
			CyU3PTimerModify(&UvcTimer, TIMER_PERIOD, 0);
#endif

			if (glStillCaptured == CyTrue) {
				glStillCaptured = CyFalse;
				glUVCHeader[1] ^= ES_UVC_HEADER_STILL_IMAGE;
				glFrameIndexToSet = glCurrentFrameIndex;
				CyU3PEventSet(&glTimerEvent, ES_TIMER_RESET_EVENT, CYU3P_EVENT_OR);
			}
			if (glStillCaptureStart == CyTrue) {
				if (glStillSkip == 3) {
					--glStillSkip;
					glFrameIndexToSet = 4;
					CyU3PEventSet(&glTimerEvent, ES_TIMER_RESET_EVENT, CYU3P_EVENT_OR);
				} else if (glStillSkip == 0) {
					glStillCaptureStart = CyFalse;
					glStillCaptured = CyTrue;
					glUVCHeader[1] ^= ES_UVC_HEADER_STILL_IMAGE;
				} else {
					--glStillSkip;
				}
			}
		}
	}
}

// This is the Callback function to handle the USB Events
static void esUVCUvcApplnUSBEventCB(CyU3PUsbEventType_t evtype,	uint16_t evdata) {
	uint8_t interface = 0, altSetting = 0;

	switch (evtype) {
	case CY_U3P_USB_EVENT_SUSPEND:
		// Suspend the device with Wake On Bus Activity set
		glIsStreamingStarted = CyFalse;
		CyU3PEventSet(&glTimerEvent, ES_USB_SUSP_EVENT_FLAG, CYU3P_EVENT_OR);
		break;
	case CY_U3P_USB_EVENT_SETINTF:
		// Start the video streamer application if the
		// interface requested was 1. If not, stop the
		// streamer.
		interface = CY_U3P_GET_MSB(evdata);
		altSetting = CY_U3P_GET_LSB(evdata);

		glIsStreamingStarted = CyFalse;

		if ((altSetting == ES_UVC_STREAM_INTERFACE) && (interface == 1)) {
			// Stop the application before re-starting.
			if (glIsApplnActive) {
				glIsClearFeature = CyTrue;
				esUVCUvcApplnStop();
			}
			esUVCUvcApplnStart();

		} else if ((altSetting == 0x00) && (interface == 1)) {
			glPreviewStarted = CyFalse;
			// Stop the application before re-starting.
			glIsClearFeature = CyTrue;
			esUVCUvcApplnStop();
			glcommitcount = 0;
		}
		break;

	case CY_U3P_USB_EVENT_SETCONF:
	case CY_U3P_USB_EVENT_RESET:
	case CY_U3P_USB_EVENT_DISCONNECT:
	case CY_U3P_USB_EVENT_CONNECT:
		glIsStreamingStarted = CyFalse;
		if (evtype == CY_U3P_USB_EVENT_SETCONF) {
			glIsConfigured = CyTrue;
		} else {
			glIsConfigured = CyFalse;
		}

		// Stop the video streamer application and enable LPM.
		CyU3PUsbLPMEnable();
		if (glIsApplnActive) {
			glIsClearFeature = CyTrue;
			esUVCUvcApplnStop();
		}
		break;

	default:
		break;
	}
}

// Callback for LPM requests. Always return true to allow host to transition device
// into required LPM state U1/U2/U3. When data transmission is active LPM management
// is explicitly disabled to prevent data transmission errors.
static CyBool_t esUVCApplnLPMRqtCB(CyU3PUsbLinkPowerMode link_mode) {	//USB 3.0 linkmode requested by Host
	return CyTrue;
}

void esSetCameraResolution(uint8_t FrameIndex) {
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
	CyU3PDebugPrint(4, "esSetCameraResolution == 1 ==\r\n");

	if (CyU3PUsbGetSpeed() == CY_U3P_SUPER_SPEED) {
		if (FrameIndex == 0x01) {
			// Write 1080pSettings
			CyU3PDebugPrint(4, "Write 1080p Settings -1-\r\n");
			status = CyU3PMipicsiSetIntfParams(&cfgUvc1080p30NoMclk, CyFalse);
			if (CY_U3P_SUCCESS != status) {
				CyU3PDebugPrint(4, "USBStpCB:SetIntfParams SS1 Err = 0x%x\r\n", status);
			}
			AR0330_1080P_config();
		} else if (FrameIndex == 0x02) {
			// Write VGA Settings
			CyU3PDebugPrint(4, "Write VGA Settings -2-\r\n");
			status = CyU3PMipicsiSetIntfParams(&cfgUvcVga30NoMclk, CyFalse);
			if (CY_U3P_SUCCESS != status) {
				CyU3PDebugPrint(4, "USBStpCB:SetIntfParams FS Err = 0x%x\r\n", status);
			}
			AR0330_VGA_config();
		} else if (FrameIndex == 0x03) {
			// Write 720pSettings
			CyU3PDebugPrint(4, "Write 720p Settings -3-\r\n");
			status = CyU3PMipicsiSetIntfParams(&cfgUvc720p60NoMclk, CyFalse);
			if (CY_U3P_SUCCESS != status) {
				CyU3PDebugPrint(4, "USBStpCB:SetIntfParams SS2 Err = 0x%x\r\n", status);
			}
			AR0330_720P_config();
		} else if (FrameIndex == 0x04) {
			CyU3PDebugPrint(4, "Write 5M Settings -4-\r\n");
			status = CyU3PMipicsiSetIntfParams(&cfgUvc5Mp15NoMclk, CyFalse);
			if (CY_U3P_SUCCESS != status) {
				CyU3PDebugPrint(4, "USBStpCB:SetIntfParams SS2 Err = 0x%x\r\n", status);
			}
			AR0330_5MP_config();
		}
	} else if (CyU3PUsbGetSpeed() == CY_U3P_HIGH_SPEED) {
		// Write VGA Settings
		CyU3PDebugPrint(4, "Write VGA Setting -5-\r\n");
		status = CyU3PMipicsiSetIntfParams(&cfgUvcVga30NoMclk, CyFalse);
		if (CY_U3P_SUCCESS != status) {
			CyU3PDebugPrint(4, "USBStpCB:SetIntfParams HS Err = 0x%x\r\n", status);
		}
		AR0330_VGA_config();
		AR0330_VGA_HS_config();
	} else { // Full Speed USB Streams
		// Write VGA Settings
		CyU3PDebugPrint(4, "Write VGA Setting -6-\r\n");
		AR0330_VGA_config();
		status = CyU3PMipicsiSetIntfParams(&cfgUvcVga30NoMclk, CyFalse);
		if (CY_U3P_SUCCESS != status) {
			CyU3PDebugPrint(4, "USBStpCB:SetIntfParams FS Err = 0x%x\r\n", status);
		}
	}
}

// Callback to handle the USB Setup Requests and UVC Class events
static CyBool_t esUVCUvcApplnUSBSetupCB(uint32_t setupdat0, uint32_t setupdat1) {
	uint8_t bRequest, bType, bRType, bTarget;
	uint16_t wValue, wIndex, wLength;
	uint16_t readCount = 0;
	uint8_t ep0Buf[2];
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;
	uint8_t temp = 0;
	CyBool_t isHandled = CyFalse;
	uint8_t RequestOption = 0;

	// Decode the fields from the setup request.
	bRType = (setupdat0 & CY_U3P_USB_REQUEST_TYPE_MASK);
	bType = (bRType & CY_U3P_USB_TYPE_MASK);
	bTarget = (bRType & CY_U3P_USB_TARGET_MASK);
	bRequest = ((setupdat0 & CY_U3P_USB_REQUEST_MASK) >> CY_U3P_USB_REQUEST_POS);
	wValue = ((setupdat0 & CY_U3P_USB_VALUE_MASK) >> CY_U3P_USB_VALUE_POS);
	wIndex = ((setupdat1 & CY_U3P_USB_INDEX_MASK) >> CY_U3P_USB_INDEX_POS);
	wLength = ((setupdat1 & CY_U3P_USB_LENGTH_MASK) >> CY_U3P_USB_LENGTH_POS);

#if 1
	CyU3PDebugPrint(4, "bRType = 0x%x, bRequest = 0x%x, wValue = 0x%x, wIndex = 0x%x, wLength= 0x%x\r\n", bRType,
			bRequest, wValue, wIndex, wLength);
#endif

	// ClearFeature(Endpoint_Halt) received on the Streaming Endpoint. Stop Streaming
	if ((bTarget == CY_U3P_USB_TARGET_ENDPT) && (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)
	    && (wIndex == ES_UVC_EP_BULK_VIDEO) && (wValue == CY_U3P_USBX_FS_EP_HALT)) {
		if ((glIsApplnActive) && (glIsStreamingStarted)) {
			glPreviewStarted = CyFalse;
			glIsClearFeature = CyTrue;
			esUVCUvcApplnStop();
			glcommitcount = 0;
		}
		return CyFalse;
	}

	if (bRType == CY_U3P_USB_GS_DEVICE) {
		// Make sure that we bring the link back to U0, so that the ERDY can be sent.
		if (CyU3PUsbGetSpeed() == CY_U3P_SUPER_SPEED) {
			CyU3PUsbSetLinkPowerState(CyU3PUsbLPM_U0);
		}
	}

	if ((bTarget == CY_U3P_USB_TARGET_INTF) && ((bRequest == CY_U3P_USB_SC_SET_FEATURE)
						    || (bRequest == CY_U3P_USB_SC_CLEAR_FEATURE)) && (wValue == 0)) {
		if (glIsConfigured) {
			CyU3PUsbAckSetup();
		} else {
			CyU3PUsbStall(0, CyTrue, CyFalse);
		}
		return CyTrue;
	}

	if ((bRequest == CY_U3P_USB_SC_GET_STATUS) && (bTarget == CY_U3P_USB_TARGET_INTF)) {
		// We support only interface 0.
		if (wIndex == 0) {
			ep0Buf[0] = 0;
			ep0Buf[1] = 0;
			CyU3PUsbSendEP0Data(0x02, ep0Buf);
		} else {
			CyU3PUsbStall(0, CyTrue, CyFalse);
			CyU3PDebugPrint(4, "***** NOT Interface 0 stalling: if = 0x%x\r\n", wIndex);
		}
		CyU3PDebugPrint(4, "...done\r\n");
		return CyTrue;
	}

	// Check for UVC Class Requests
	if (bType == CY_U3P_USB_CLASS_RQT) {

		// UVC Class Requests
		// Requests to the Video Streaming Interface (IF 1)
		if ((wIndex & 0x00FF) == ES_UVC_STREAM_INTERFACE) {
			// GET_CUR Request Handling Probe/Commit Controls
			if ((bRequest == ES_UVC_USB_GET_CUR_REQ) || (bRequest == ES_UVC_USB_GET_MIN_REQ)
			    || (bRequest == ES_UVC_USB_GET_MAX_REQ) || (bRequest == ES_UVC_USB_GET_DEF_REQ)) {
				isHandled = CyTrue;
				if ((wValue == ES_UVC_VS_PROBE_CONTROL) || (wValue == ES_UVC_VS_COMMIT_CONTROL)) {
					// Host requests for probe data of 34 bytes (UVC 1.1) or 26 Bytes (UVC1.0). Send it over EP0.
					if (CyU3PUsbGetSpeed() == CY_U3P_SUPER_SPEED) {
						CyU3PDebugPrint(4, "setting frame index = %d\r\n", glCurrentFrameIndex);
						if (glCurrentFrameIndex == 4) {
							CyU3PMemCopy(glProbeCtrl, (uint8_t *) gl5MpProbeCtrl,
								     ES_UVC_MAX_PROBE_SETTING);
						}
						// Probe Control for 1280x720 stream
						else if (glCurrentFrameIndex == 3) {
							CyU3PMemCopy(glProbeCtrl, (uint8_t *) gl720pProbeCtrl,
								     ES_UVC_MAX_PROBE_SETTING);
						}
						// Probe Control for 640x480 stream
						else if (glCurrentFrameIndex == 2) {
							CyU3PMemCopy(glProbeCtrl, (uint8_t *) glVga60ProbeCtrl,
								     ES_UVC_MAX_PROBE_SETTING);
						}
						// Probe Control for 1920x1080 stream
						else if (glCurrentFrameIndex == 1) {
							CyU3PMemCopy(glProbeCtrl, (uint8_t *) gl1080pProbeCtrl,
								     ES_UVC_MAX_PROBE_SETTING);
						}

					} else if (CyU3PUsbGetSpeed() == CY_U3P_HIGH_SPEED) {
						// Probe Control for 640x480 stream
						CyU3PMemCopy(glProbeCtrl, (uint8_t *) glVga30ProbeCtrl,
							     ES_UVC_MAX_PROBE_SETTING);
					} else {	// FULL-Speed

						CyU3PDebugPrint(4, "Full Speed Not Supported!\r\n");
					}

					status = CyU3PUsbSendEP0Data(ES_UVC_MAX_PROBE_SETTING, glProbeCtrl);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:GET_CUR:SendEP0Data Err = 0x%x\r\n",
								status);
					}
				} else if ((wValue == ES_UVC_STILL_PROBE_CONTROL)
					   || (wValue == ES_UVC_STILL_COMMIT_CONTROL)) {
					if (CyU3PUsbGetSpeed() == CY_U3P_SUPER_SPEED) {
						status =
						    CyU3PUsbSendEP0Data(ES_UVC_MAX_STILL_PROBE_SETTING,
									glStillProbeCtrl);
						if (CY_U3P_SUCCESS != status) {
							CyU3PDebugPrint(4,
									"USBStpCB:GET_CUR:SendEP0Data Err = 0x%x\r\n",
									status);
						}
					}
				}
			}
			// SET_CUR request handling Probe/Commit controls
			else if (bRequest == ES_UVC_USB_SET_CUR_REQ) {
				isHandled = CyTrue;
				if ((wValue == ES_UVC_VS_PROBE_CONTROL) || (wValue == ES_UVC_VS_COMMIT_CONTROL)) {
					// Get the UVC probe/commit control data from EP0
					status = CyU3PUsbGetEP0Data(ES_UVC_MAX_PROBE_SETTING_ALIGNED,
								    glCommitCtrl, &readCount);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:SET_CUR:GetEP0Data Err = 0x%x.\r\n",
								status);
					}
					// Check the read count. Expecting a count of CX3_UVC_MAX_PROBE_SETTING bytes.
					if (readCount > (uint16_t) ES_UVC_MAX_PROBE_SETTING) {
						CyU3PDebugPrint(4, "USBStpCB:Invalid SET_CUR Rqt Len.\r\n");
					} else {
						// Set Probe Control
						if (wValue == ES_UVC_VS_PROBE_CONTROL) {
							glCurrentFrameIndex = glCommitCtrl[3];
						}
						// Set Commit Control and Start Streaming
						else if (wValue == ES_UVC_VS_COMMIT_CONTROL) {

							if ((glcommitcount == 0) || (glcheckframe != glCommitCtrl[3])) {
								glcommitcount++;
								glcheckframe = glCommitCtrl[3];
								glCurrentFrameIndex = glCommitCtrl[3];
								glFrameIndexToSet = glCurrentFrameIndex;
								glPreviewStarted = CyTrue;

								esSetCameraResolution(glCurrentFrameIndex);
								//esSetCameraResolution(glCommitCtrl[3]);

								if (glIsApplnActive) {
									if (glcommitcount) {
										glIsClearFeature = CyFalse;
									} else {
										glIsClearFeature = CyTrue;
									}
									esUVCUvcApplnStop();
								}
								esUVCUvcApplnStart();
							}
						}
					}
				} else if ((wValue == ES_UVC_STILL_PROBE_CONTROL)
					   || (wValue == ES_UVC_STILL_COMMIT_CONTROL)) {
					// Get the UVC STILL probe/commit control data from EP0
					status =
					    CyU3PUsbGetEP0Data(ES_UVC_MAX_STILL_PROBE_SETTING_ALIGNED,
							       glStillCommitCtrl, &readCount);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:SET_CUR:GetEP0Data Err = 0x%x.\r\n",
								status);
					}
					// Check the read count. Expecting a count of CX3_UVC_MAX_PROBE_SETTING bytes.
					if (readCount > (uint16_t) ES_UVC_MAX_STILL_PROBE_SETTING) {
						CyU3PDebugPrint(4, "USBStpCB:Invalid SET_CUR Rqt Len.\r\n");
					} else {
						// Set Probe Control
						if (wValue == ES_UVC_STILL_PROBE_CONTROL) {
							glCurrentStillFrameIndex = glStillCommitCtrl[1];
						}
						// Set Commit Control and Start Streaming
						else if (wValue == ES_UVC_STILL_COMMIT_CONTROL) {
							glCurrentStillFrameIndex = glStillCommitCtrl[1];
						}
					}

				} else if (wValue == ES_UVC_STILL_TRIGGER) {
					status =
					    CyU3PUsbGetEP0Data(ES_UVC_STILL_TRIGGER_ALIGNED, &glStillTriggerCtrl,
							       &readCount);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:SET_CUR:GetEP0Data Err = 0x%x.\r\n",
								status);
					}
					// Check the read count. Expecting a count of CX3_UVC_MAX_PROBE_SETTING bytes.
					if (readCount > (uint16_t) ES_UVC_STILL_TRIGGER_COUNT) {
						CyU3PDebugPrint(4, "USBStpCB:Invalid SET_CUR Rqt Len.\r\n");
					} else {
						if (glStillTriggerCtrl == 0x01) {
							glStillSkip = 3;
							glStillCaptureStart = CyTrue;
						}
					}
				}
			} else {
				// Mark with error.
				status = CY_U3P_ERROR_FAILURE;
				CyU3PDebugPrint(4, "***** FAILED *****\r\n");
			}
		} else if ((wIndex & 0x00FF) == ES_UVC_CONTROL_INTERFACE) {	// Video Control Interface
			isHandled = CyTrue;

			if ((wIndex == 0x200) && (wValue == 0x200)) {	// Brightness
				switch (bRequest) {
				case ES_UVC_USB_GET_INFO_REQ:
					glGet_Info = 0x03;
					status = CyU3PUsbSendEP0Data(1, &glGet_Info);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_GET_MIN_REQ:
				case ES_UVC_USB_GET_MAX_REQ:
				case ES_UVC_USB_GET_RES_REQ:
				case ES_UVC_USB_GET_CUR_REQ:
				case ES_UVC_USB_GET_DEF_REQ:
					RequestOption = (bRequest & 0x0F);
					gl32GetControl = AR0330_GetBrightness(RequestOption);
					status = CyU3PUsbSendEP0Data(4, (uint8_t *)&gl32GetControl);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_SET_CUR_REQ:
					status = CyU3PUsbGetEP0Data(32, (uint8_t *)&gl32SetControl, &readCount);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					AR0330_SetBrightness(gl32SetControl);
					break;
				}
			} else if ((wIndex == 0x100) && (wValue == 0x200)) {	// Auto Exposure
				switch (bRequest) {
				case ES_UVC_USB_GET_INFO_REQ:
					glGet_Info = 0x03;
					status = CyU3PUsbSendEP0Data(1, &glGet_Info);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_GET_MIN_REQ:
				case ES_UVC_USB_GET_MAX_REQ:
				case ES_UVC_USB_GET_RES_REQ:
				case ES_UVC_USB_GET_CUR_REQ:
				case ES_UVC_USB_GET_DEF_REQ:
					RequestOption = (bRequest & 0x0F);
					gl32GetControl = AR0330_GetAutoExposure(RequestOption);
					status = CyU3PUsbSendEP0Data(4, (uint8_t *)&gl32GetControl);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_SET_CUR_REQ:
					status = CyU3PUsbGetEP0Data(32, (uint8_t *)&gl32SetControl, &readCount);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					AR0330_SetAutoExposure(gl32SetControl);
					break;
				}
			} else if ((wIndex == 0x200) && (wValue == 0x300)) {	// Contrast
				switch (bRequest) {
				case ES_UVC_USB_GET_INFO_REQ:
					glGet_Info = 0x03;
					status = CyU3PUsbSendEP0Data(1, &glGet_Info);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_GET_MIN_REQ:
				case ES_UVC_USB_GET_MAX_REQ:
				case ES_UVC_USB_GET_RES_REQ:
				case ES_UVC_USB_GET_CUR_REQ:
				case ES_UVC_USB_GET_DEF_REQ:
					RequestOption = (bRequest & 0x0F);
					gl32GetControl = AR0330_GetContrast(RequestOption);
					status = CyU3PUsbSendEP0Data(4, (uint8_t *)&gl32GetControl);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_SET_CUR_REQ:
					status = CyU3PUsbGetEP0Data(32, (uint8_t *)&gl32SetControl, &readCount);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					AR0330_SetContrast(gl32SetControl);
					break;
				}
			} else if ((wIndex == 0x100) && (wValue == 0x400)) {	// Manual Exposure
				//CyU3PDebugPrint (4, "Manual Exposure\r\n");
				switch (bRequest) {
				case ES_UVC_USB_GET_INFO_REQ:
					glGet_Info = 0x03;
					status = CyU3PUsbSendEP0Data(1, &glGet_Info);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_GET_MIN_REQ:
				case ES_UVC_USB_GET_MAX_REQ:
				case ES_UVC_USB_GET_RES_REQ:
				case ES_UVC_USB_GET_CUR_REQ:
				case ES_UVC_USB_GET_DEF_REQ:
					RequestOption = (bRequest & 0x0F);
					gl32GetControl = AR0330_GetExposure(RequestOption);
					status = CyU3PUsbSendEP0Data(4, (uint8_t *)&gl32GetControl);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_SET_CUR_REQ:
					status = CyU3PUsbGetEP0Data(32, (uint8_t *)&gl32SetControl, &readCount);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					AR0330_SetExposure(gl32SetControl);
					break;
				}
			} else if ((wIndex == 0x200) && (wValue == 0x600)) {	// Hue
				switch (bRequest) {
				case ES_UVC_USB_GET_INFO_REQ:
					glGet_Info = 0x03;
					status = CyU3PUsbSendEP0Data(1, &glGet_Info);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_GET_MIN_REQ:
				case ES_UVC_USB_GET_MAX_REQ:
				case ES_UVC_USB_GET_RES_REQ:
				case ES_UVC_USB_GET_CUR_REQ:
				case ES_UVC_USB_GET_DEF_REQ:
					RequestOption = (bRequest & 0x0F);
					gl32GetControl = (int32_t) AR0330_GetHue(RequestOption);
					status = CyU3PUsbSendEP0Data(4, (uint8_t *)&gl32GetControl);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_SET_CUR_REQ:
					status = CyU3PUsbGetEP0Data(32, (uint8_t *)&gl32SetControl, &readCount);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					AR0330_SetHue(gl32SetControl);
					break;
				}
			} else if ((wIndex == 0x100) && (wValue == 0x600)) {	// Manual Focus
				switch (bRequest) {
				case ES_UVC_USB_GET_INFO_REQ:
					glGet_Info = 0x03;
					status = CyU3PUsbSendEP0Data(1, &glGet_Info);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_GET_MIN_REQ:
				case ES_UVC_USB_GET_MAX_REQ:
				case ES_UVC_USB_GET_RES_REQ:
				case ES_UVC_USB_GET_CUR_REQ:
				case ES_UVC_USB_GET_DEF_REQ:
					RequestOption = (bRequest & 0x0F);
					gl32GetControl = AR0330_GetManualfocus(RequestOption);
					status = CyU3PUsbSendEP0Data(4, (uint8_t *)&gl32GetControl);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_SET_CUR_REQ:
					status = CyU3PUsbGetEP0Data(32, (uint8_t *)&gl32SetControl, &readCount);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					AR0330_SetManualfocus(gl32SetControl);
					g_IsAutoFocus = 0;
					break;
				}
			} else if ((wIndex == 0x200) && (wValue == 0x700)) {	// Saturation
				switch (bRequest) {
				case ES_UVC_USB_GET_INFO_REQ:
					glGet_Info = 0x03;
					status = CyU3PUsbSendEP0Data(1, &glGet_Info);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_GET_MIN_REQ:
				case ES_UVC_USB_GET_MAX_REQ:
				case ES_UVC_USB_GET_RES_REQ:
				case ES_UVC_USB_GET_CUR_REQ:
				case ES_UVC_USB_GET_DEF_REQ:
					RequestOption = (bRequest & 0x0F);
					gl32GetControl = AR0330_GetSaturation(RequestOption);
					status = CyU3PUsbSendEP0Data(4, (uint8_t *)&gl32GetControl);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_SET_CUR_REQ:
					status = CyU3PUsbGetEP0Data(32, (uint8_t *)&gl32SetControl, &readCount);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					AR0330_SetSaturation(gl32SetControl);
					break;
				}
			} else if ((wIndex == 0x200) && (wValue == 0x800)) {	// Sharpness
				switch (bRequest) {
				case ES_UVC_USB_GET_INFO_REQ:
					glGet_Info = 0x03;
					status = CyU3PUsbSendEP0Data(1, &glGet_Info);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_GET_MIN_REQ:
				case ES_UVC_USB_GET_MAX_REQ:
				case ES_UVC_USB_GET_RES_REQ:
				case ES_UVC_USB_GET_CUR_REQ:
				case ES_UVC_USB_GET_DEF_REQ:
					RequestOption = (bRequest & 0x0F);
					gl32GetControl = AR0330_GetSharpness(RequestOption);
					status = CyU3PUsbSendEP0Data(4, (uint8_t *)&gl32GetControl);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_SET_CUR_REQ:
					status = CyU3PUsbGetEP0Data(32, (uint8_t *)&gl32SetControl, &readCount);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					AR0330_SetSharpness(gl32SetControl);
					break;
				}
			} else if ((wIndex == 0x100) && (wValue == 0x800)) {	// Auto Focus
				switch (bRequest) {
				case ES_UVC_USB_GET_INFO_REQ:
					glGet_Info = 0x03;
					status = CyU3PUsbSendEP0Data(1, &glGet_Info);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_GET_MIN_REQ:
				case ES_UVC_USB_GET_MAX_REQ:
				case ES_UVC_USB_GET_RES_REQ:
				case ES_UVC_USB_GET_CUR_REQ:
				case ES_UVC_USB_GET_DEF_REQ:
					RequestOption = (bRequest & 0x0F);
					gl32GetControl = AR0330_GetAutofocus(RequestOption);
					status = CyU3PUsbSendEP0Data(4, (uint8_t *)&gl32GetControl);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_SET_CUR_REQ:
					status = CyU3PUsbGetEP0Data(32, (uint8_t *)&gl32SetControl, &readCount);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					AR0330_SetAutofocus(gl32SetControl);
					g_IsAutoFocus = 1;
					break;
				}
			} else if ((wIndex == 0x200) && (wValue == 0xA00)) {	// White Balance manual
				switch (bRequest) {
				case ES_UVC_USB_GET_INFO_REQ:
					glGet_Info = 0x03;
					status = CyU3PUsbSendEP0Data(1, &glGet_Info);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_GET_MIN_REQ:
				case ES_UVC_USB_GET_MAX_REQ:
				case ES_UVC_USB_GET_RES_REQ:
				case ES_UVC_USB_GET_CUR_REQ:
				case ES_UVC_USB_GET_DEF_REQ:
					RequestOption = (bRequest & 0x0F);
					gl32GetControl = AR0330_GetWhiteBalance(RequestOption);
					status = CyU3PUsbSendEP0Data(4, (uint8_t *)&gl32GetControl);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_SET_CUR_REQ:
					status = CyU3PUsbGetEP0Data(32, (uint8_t *)&gl32SetControl, &readCount);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					AR0330_SetWhiteBalance(gl32SetControl);
					break;
				}
			} else if ((wIndex == 0x200) && (wValue == 0xB00)) {	// White Balance Auto
				switch (bRequest) {
				case ES_UVC_USB_GET_INFO_REQ:
					glGet_Info = 0x03;
					status = CyU3PUsbSendEP0Data(1, &glGet_Info);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_GET_MIN_REQ:
				case ES_UVC_USB_GET_MAX_REQ:
				case ES_UVC_USB_GET_RES_REQ:
				case ES_UVC_USB_GET_CUR_REQ:
				case ES_UVC_USB_GET_DEF_REQ:
					RequestOption = (bRequest & 0x0F);
					gl32GetControl = AR0330_GetAutoWhiteBalance(RequestOption);
					status = CyU3PUsbSendEP0Data(4, (uint8_t *)&gl32GetControl);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					break;
				case ES_UVC_USB_SET_CUR_REQ:
					status = CyU3PUsbGetEP0Data(32, (uint8_t *)&gl32SetControl, &readCount);
					if (CY_U3P_SUCCESS != status) {
						CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
					}
					AR0330_SetAutoWhiteBalance(gl32SetControl);
					break;
				}
			} else if ((wValue == ES_UVC_VC_REQUEST_ERROR_CODE_CONTROL) && (wIndex == 0x00)) {
				temp = ES_UVC_ERROR_INVALID_CONTROL;
				status = CyU3PUsbSendEP0Data(0x01, &temp);
				if (CY_U3P_SUCCESS != status) {
					CyU3PDebugPrint(4, "USBStpCB:VCI SendEP0Data = %d\r\n", status);
				}
			} else {
				CyU3PUsbStall(0, CyTrue, CyTrue);
				CyU3PDebugPrint(4, "***** Stall HERE *****\r\n");
			}
		}
	}

	return isHandled;
}

// This function initializes the USB Module, creates event group,
// sets the enumeration descriptors, configures the Endpoints and
// configures the DMA module for the UVC Application
void esUVCUvcApplnInit(void) {
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

	CyU3PDebugPrint(4, "\r\n\n\n\n\n\n\n\n\n\n\n");
	CyU3PDebugPrint(4, "esUVCUvcApplnInit\r\n");

	// Initialize the I2C interface for Mipi Block Usage and Camera.
	status = CyU3PMipicsiInitializeI2c(CY_U3P_MIPICSI_I2C_400KHZ);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:I2CInit Err = 0x%x.\r\n", status);
		esUVCAppErrorHandler(status);
	}

	// Initialize GPIO module.
	status = CyU3PMipicsiInitializeGPIO();
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:GPIOInit Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	// Initialize the PIB block
	status = CyU3PMipicsiInitializePIB();
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:PIBInit Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	// Start the USB functionality
	status = CyU3PUsbStart();
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:UsbStart Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}
	// The fast enumeration is the easiest way to setup a USB connection,
	// where all enumeration phase is handled by the library. Only the
	// class / vendor requests need to be handled by the application.
	CyU3PUsbRegisterSetupCallback(esUVCUvcApplnUSBSetupCB, CyTrue);

	// Setup the callback to handle the USB events
	CyU3PUsbRegisterEventCallback(esUVCUvcApplnUSBEventCB);

	// Register a callback to handle LPM requests from the USB 3.0 host.
	CyU3PUsbRegisterLPMRequestCallback(esUVCApplnLPMRqtCB);

	// Set the USB Enumeration descriptors

	// Super speed device descriptor.
	status = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_DEVICE_DESCR, 0, (uint8_t *) esUVCUSB30DeviceDscr);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:Set_SS_Device_Dscr Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	// High speed device descriptor.
	status = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_DEVICE_DESCR, 0, (uint8_t *) esUVCUSB20DeviceDscr);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:Set_HS_Device_Dscr Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	// BOS descriptor
	status = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_BOS_DESCR, 0, (uint8_t *) esUVCUSBBOSDscr);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:Set_BOS_Dscr Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	// Device qualifier descriptor
	status = CyU3PUsbSetDesc(CY_U3P_USB_SET_DEVQUAL_DESCR, 0, (uint8_t *) esUVCUSBDeviceQualDscr);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:Set_DEVQUAL_Dscr Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	// Super speed configuration descriptor
	status = CyU3PUsbSetDesc(CY_U3P_USB_SET_SS_CONFIG_DESCR, 0, (uint8_t *) esUVCUSBSSConfigDscr);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:Set_SS_CFG_Dscr Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	// High speed configuration descriptor
	status = CyU3PUsbSetDesc(CY_U3P_USB_SET_HS_CONFIG_DESCR, 0, (uint8_t *) esUVCUSBHSConfigDscr);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:Set_HS_CFG_Dscr Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	// Full speed configuration descriptor
	status = CyU3PUsbSetDesc(CY_U3P_USB_SET_FS_CONFIG_DESCR, 0, (uint8_t *) esUVCUSBFSConfigDscr);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:Set_FS_CFG_Dscr Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	// String descriptor 0
	status = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 0, (uint8_t *) esUVCUSBStringLangIDDscr);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:Set_STRNG_Dscr0 Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	// String descriptor 1
	status = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 1, (uint8_t *) esUVCUSBManufactureDscr);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:Set_STRNG_Dscr1 Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	// String descriptor 2
	status = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 2, (uint8_t *) esUVCUSBProductDscr);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:Set_STRNG_Dscr2 Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}
	// String descriptor 3
	status = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 3, (uint8_t *) esUVCUSBConfigSSDscr);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:Set_STRNG_Dscr3 Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	// String descriptor 4
	status = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 4, (uint8_t *) esUVCUSBConfigHSDscr);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:Set_STRNG_Dscr4 Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}
	// String descriptor 2
	status = CyU3PUsbSetDesc(CY_U3P_USB_SET_STRING_DESCR, 5, (uint8_t *) esUVCUSBConfigFSDscr);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:Set_STRNG_Dscr5 Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	CyU3PUsbVBattEnable(CyTrue);
	CyU3PUsbControlVBusDetect(CyFalse, CyTrue);

//+
	// Send XRESET pulse to reset Image sensor
	status = CyU3PMipicsiInit();
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:MipicsiInit Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}
	CyU3PThreadSleep(100);

	// Send Xreset pulse to rease Image sensor
	status = CyU3PMipicsiSetSensorControl(CY_U3P_CSI_IO_XRES, CyTrue);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:MipicsiSetSensorControl Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}
	CyU3PThreadSleep(200);
	// Send Xreset pulse to rease Image sensor
	status = CyU3PMipicsiSetSensorControl(CY_U3P_CSI_IO_XRES, CyFalse);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:MipicsiSetSensorControl Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}
	CyU3PThreadSleep(200);
	// Send Xreset pulse to rease Image sensor
	status = CyU3PMipicsiSetSensorControl(CY_U3P_CSI_IO_XRES, CyTrue);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:MipicsiSetSensorControl Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}
	CyU3PThreadSleep(200);
//-

	// Setup Image Sensor
	AR0330_Base_Config();
	AR0330_Auto_Focus_Config();
	//[TBD] esCamera_Power_Down();

	// Connect the USB pins and enable super speed operation
	status = CyU3PConnectState(CyTrue, CyTrue);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:ConnectState Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	// Since the status interrupt endpoint is not used in this application,
	// just enable the EP in the beginning.
	// Control status interrupt endpoint configuration
	CyU3PEpConfig_t endPointConfig = {
		.enable   = 1,
		.epType   = CY_U3P_USB_EP_INTR,
		.pcktSize = 64,
		.isoPkts  = 1,
		.burstLen = 1
	};
	status = CyU3PSetEpConfig(ES_UVC_EP_CONTROL_STATUS, &endPointConfig);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:CyU3PSetEpConfig CtrlEp Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	CyU3PUsbFlushEp(ES_UVC_EP_CONTROL_STATUS);

	// Setup the Bulk endpoint used for Video Streaming
	endPointConfig.enable = CyTrue;
	endPointConfig.epType = CY_U3P_USB_EP_BULK;

	endPointConfig.isoPkts = 0;
	endPointConfig.streams = 0;

	CyU3PThreadSleep(1000);

	switch (CyU3PUsbGetSpeed()) {
	case CY_U3P_HIGH_SPEED:
		endPointConfig.pcktSize = 0x200;
		endPointConfig.burstLen = 1;
		ES_UVC_STREAM_BUF_SIZE = ES_UVC_HS_STREAM_BUF_SIZE;
		ES_UVC_DATA_BUF_SIZE = ES_UVC_HS_DATA_BUF_SIZE;
		ES_UVC_STREAM_BUF_COUNT = ES_UVC_HS_STREAM_BUF_COUNT;
		break;

	case CY_U3P_FULL_SPEED:
		endPointConfig.pcktSize = 0x40;
		endPointConfig.burstLen = 1;
		break;

	case CY_U3P_SUPER_SPEED:
	default:
		endPointConfig.pcktSize = ES_UVC_EP_BULK_VIDEO_PKT_SIZE;
		endPointConfig.burstLen = 16;
		ES_UVC_STREAM_BUF_SIZE = ES_UVC_SS_STREAM_BUF_SIZE;
		ES_UVC_DATA_BUF_SIZE = ES_UVC_SS_DATA_BUF_SIZE;
		ES_UVC_STREAM_BUF_COUNT = ES_UVC_SS_STREAM_BUF_COUNT;
		break;
	}

	status = CyU3PSetEpConfig(ES_UVC_EP_BULK_VIDEO, &endPointConfig);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:CyU3PSetEpConfig BulkEp Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	CyU3PUsbEPSetBurstMode(ES_UVC_EP_BULK_VIDEO, CyTrue);

	// Flush the endpoint memory
	CyU3PUsbFlushEp(ES_UVC_EP_BULK_VIDEO);

	// Create a DMA Manual OUT channel for streaming data
	// Video streaming Channel is not active till a stream request is received
	CyU3PDmaMultiChannelConfig_t dmaCfg = {
		.size           = ES_UVC_STREAM_BUF_SIZE,
		.count          = ES_UVC_STREAM_BUF_COUNT,
		.validSckCount  = 2,

		.prodSckId[0]   = ES_UVC_PRODUCER_PPORT_SOCKET_0,
		.prodSckId[1]   = ES_UVC_PRODUCER_PPORT_SOCKET_1,
		.consSckId[0]   = ES_UVC_EP_VIDEO_CONS_SOCKET,

		.dmaMode        = CY_U3P_DMA_MODE_BYTE,
		.notification   = CY_U3P_DMA_CB_PROD_EVENT | CY_U3P_DMA_CB_CONS_EVENT,
		.cb             = esUVCUvcAppDmaCallback,

		.prodHeader     = ES_UVC_PROD_HEADER,
		.prodFooter     = ES_UVC_PROD_FOOTER,
		.consHeader     = 0,
		.prodAvailCount = 0
	};
	status = CyU3PDmaMultiChannelCreate(&glChHandleUVCStream, CY_U3P_DMA_TYPE_MANUAL_MANY_TO_ONE, &dmaCfg);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:DmaMultiChannelCreate Err = 0x%x\r\n", status);
	}
	CyU3PThreadSleep(100);

	// Reset the channel: Set to DSCR chain starting point in PORD/CONS SCKT; set
	// DSCR_SIZE field in DSCR memory
	status = CyU3PDmaMultiChannelReset(&glChHandleUVCStream);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:MultiChannelReset Err = 0x%x\r\n", status);
	}

	// Configure the Fixed Function GPIF on the CX3 to use a 16 bit bus, and
	// a DMA Buffer of size CX3_UVC_DATA_BUF_SIZE
	status = CyU3PMipicsiGpifLoad(CY_U3P_MIPICSI_BUS_16, ES_UVC_DATA_BUF_SIZE);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:MipicsiGpifLoad Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}
	CyU3PThreadSleep(50);

	CyU3PGpifRegisterCallback(esUVCGpifCB);
	CyU3PThreadSleep(50);

	// Start the state machine.
	status = CyU3PGpifSMStart(CX3_START_SCK0, ALPHA_CX3_START_SCK0);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:GpifSMStart Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}
	CyU3PThreadSleep(50);

	// Pause the GPIF
	CyU3PGpifSMControl(CyTrue);
#if 0
	// *** this code moved up to allow hard reset of image sensor using XRESET MIPI GPIO pin
	// Initialize the MIPI block
	status = CyU3PMipicsiInit();
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:MipicsiInit Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}

	status = CyU3PMipicsiSetIntfParams(&cfgUvcVgaNoMclk, CyFalse);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:MipicsiSetIntfParams Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}
#else
	status = CyU3PMipicsiSetIntfParams(&cfgUvc1080p30NoMclk, CyFalse);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "AppInit:MipicsiSetIntfParams Err = 0x%x\r\n", status);
		esUVCAppErrorHandler(status);
	}
#endif

#ifdef RESET_TIMER_ENABLE
	CyU3PTimerCreate(&UvcTimer, UvcAppProgressTimer, 0x00, TIMER_PERIOD, 0, CYU3P_NO_ACTIVATE);
#endif

	CyU3PDebugPrint(4, "Firmware Version: %d.%d.%d.%d\r\n", MajorVersion, MinorVersion, SubVersion, SubVersion1);
}

// This function initializes the debug module for the UVC application
void esUVCUvcApplnDebugInit(void) {
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

	// Initialize the UART for printing debug messages
	status = CyU3PUartInit();
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "esUVCUvcApplnDebugInit:CyU3PUartInit failed Error = 0x%x\r\n", status);
	}

	// Set UART Configuration
	CyU3PUartConfig_t uartConfig = {
		.baudRate = CY_U3P_UART_BAUDRATE_115200,
		.stopBit  = CY_U3P_UART_ONE_STOP_BIT,
		.parity   = CY_U3P_UART_NO_PARITY,
		.txEnable = CyTrue,
		.rxEnable = CyFalse,
		.flowCtrl = CyFalse,
		.isDma    = CyTrue
	};
	// Set the UART configuration
	status = CyU3PUartSetConfig(&uartConfig, NULL);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "esUVCUvcApplnDebugInit:CyU3PUartSetConfig failed Error = 0x%x\r\n", status);
	}

	// Set the UART transfer
	status = CyU3PUartTxSetBlockXfer(0xFFFFFFFF);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "esUVCUvcApplnDebugInit:CyU3PUartTxSetBlockXfer failed Error = 0x%x\r\n", status);
	}

	// Initialize the debug application
	status = CyU3PDebugInit(CY_U3P_LPP_SOCKET_UART_CONS, 8);
	if (CY_U3P_SUCCESS != status) {
		CyU3PDebugPrint(4, "esUVCUvcApplnDebugInit:CyU3PDebugInit failed Error = 0x%x\r\n", status);
	}

	CyU3PDebugPreamble(CyFalse);

}

// Entry function for the UVC application thread.
void esUVCUvcAppThread_Entry(uint32_t input) {
	uint16_t wakeReason;
	uint32_t eventFlag;
	CyU3PReturnStatus_t status;

	// Initialize the Debug Module
	esUVCUvcApplnDebugInit();

	// Initialize the UVC Application
	esUVCUvcApplnInit();

	for (;;) {
		CyU3PEventGet(&glTimerEvent, ES_USB_SUSP_EVENT_FLAG | ES_TIMER_RESET_EVENT, CYU3P_EVENT_OR_CLEAR,
			      &eventFlag, CYU3P_WAIT_FOREVER);

		// Handle TimerReset Event
		if (eventFlag & ES_TIMER_RESET_EVENT) {
			if (glIsApplnActive) {
				glIsClearFeature = CyFalse;
				esUVCUvcApplnStop();
			}
			if (glPreviewStarted == CyTrue) {
				esSetCameraResolution(glFrameIndexToSet);
				esUVCUvcApplnStart();
			}
#ifdef RESET_TIMER_ENABLE
			CyU3PTimerModify(&UvcTimer, TIMER_PERIOD, 0);
#endif
		}
		// Handle Suspend Event
		if (eventFlag & ES_USB_SUSP_EVENT_FLAG) {
			// Place CX3 in Low Power Suspend mode, with USB bus activity as the wakeup source.
			CyU3PMipicsiSleep();
			//[TBD] esCamera_Power_Down();

			status = CyU3PSysEnterSuspendMode(CY_U3P_SYS_USB_BUS_ACTVTY_WAKEUP_SRC, 0, &wakeReason);
			if (CY_U3P_SUCCESS != status) {
				CyU3PDebugPrint(4, "CyU3PSysEnterSuspendMode returnd status = 0x%x\r\n", status);
			}

			if (glMipiActive) {
				CyU3PMipicsiWakeup();
				//[TBD] esCamera_Power_Up();
			}
			continue;
		}
	}
}

// Application define function which creates the threads.
void CyFxApplicationDefine(void) {

	// Allocate the memory for the thread and create the thread
	void *stack = CyU3PMemAlloc(UVC_APP_THREAD_STACK);
	uint32_t rc = CyU3PThreadCreate(&uvcAppThread,           // UVC Thread structure
					"30:UVC_app_thread",     // Thread Id and name
					esUVCUvcAppThread_Entry, // UVC Application Thread Entry function
					0,                       // No input parameter to thread
					stack,                   // Pointer to the allocated thread stack
					UVC_APP_THREAD_STACK,    // UVC Application Thread stack size
					UVC_APP_THREAD_PRIORITY, // UVC Application Thread priority
					UVC_APP_THREAD_PRIORITY, // Pre-emption threshold
					CYU3P_NO_TIME_SLICE,     // No time slice for the application thread
					CYU3P_AUTO_START         // Start the Thread immediately
	    );

	// Check the return code
	if (CY_U3P_SUCCESS != rc) {
		// Thread Creation failed with the error code retThrdCreate

		// Add custom recovery or debug actions here

		// Application cannot continue
		// Loop indefinitely
		while (1) ;
	}

	// Create GPIO application event group
	rc = CyU3PEventCreate(&glTimerEvent);
	if (CY_U3P_SUCCESS != rc) {
		// Event group creation failed with the error code retThrdCreate

		// Add custom recovery or debug actions here

		// Application cannot continue
		// Loop indefinitely
		while (1) ;
	}
}


int main(void) {
	CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

	// Initialize the device
	status = CyU3PDeviceInit(NULL);
	if (CY_U3P_SUCCESS != status) {
		goto handle_fatal_error;
	}

	// Initialize the caches. Enable instruction cache and keep data cache disabled.
	// The data cache is useful only when there is a large amount of CPU based memory
	// accesses. When used in simple cases, it can decrease performance due to large
	// number of cache flushes and cleans and also it adds to the complexity of the
	// code.
	status = CyU3PDeviceCacheControl(CyTrue, CyFalse, CyFalse);
	if (CY_U3P_SUCCESS != status) {
		goto handle_fatal_error;
	}

	// Configure the IO matrix for the device.
	CyU3PIoMatrixConfig_t io_cfg = {
		.isDQ32Bit = CyFalse,

		.useUart = CyTrue,
		.useI2C  = CyTrue,
		.useI2S  = CyFalse,
		.useSpi  = CyTrue,
		.lppMode = CY_U3P_IO_MATRIX_LPP_DEFAULT,
		.s0Mode  = CY_U3P_SPORT_INACTIVE,
		.s1Mode  = CY_U3P_SPORT_INACTIVE,

		.gpioSimpleEn[0]  = 0,
		.gpioSimpleEn[1]  = 0,
		.gpioComplexEn[0] = 0,
		.gpioComplexEn[1] = 0
	};
	status = CyU3PDeviceConfigureIOMatrix(&io_cfg);
	if (CY_U3P_SUCCESS != status) {
		goto handle_fatal_error;
	}

	// This is a non returnable call for initializing the RTOS kernel
	CyU3PKernelEntry();

	// Dummy return to make the compiler happy
	return 0;

 handle_fatal_error:
	// Cannot recover from this error
	while (1) ;
}
