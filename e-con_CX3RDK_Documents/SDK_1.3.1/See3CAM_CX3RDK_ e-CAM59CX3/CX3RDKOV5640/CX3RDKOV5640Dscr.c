/*
 ## e-con Systems USB UVC Stack – See3CAMCX3RDK Platform

 ## source file : CX3RDKOV5640Dscr.c
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

/* This file contains the USB enumeration descriptors for the UVC (in memory) application example.
 * The descriptor arrays must be 32 byte aligned if the D-cache is turned on. If the linker
 * used is not capable of supporting the aligned feature for this, then dynamically allocated
 * buffer must be used, and the descriptor must be loaded into it. */

#include "CX3RDKOV5640.h"
#include "cyu3mipicsi.h"

/* Standard Device Descriptor for USB 3 */
const uint8_t esUVCUSB30DeviceDscr[] =
{
    0x12,                               /* Descriptor size */
    CY_U3P_USB_DEVICE_DESCR,            /* Device descriptor type */
    0x00, 0x03,                         /* USB 3.0 */
    0xEF,                               /* Device class */
    0x02,                               /* Device Sub-class */
    0x01,                               /* Device protocol */
    0x09,                               /* Maxpacket size for EP0 : 2^9 */
    0x60, 0x25,                         /* Vendor ID */
    0x51, 0xD0,                         /* Product ID */
    0x00, 0x00,                         /* Device release number */
    0x01,                               /* Manufacture string index */
    0x02,                               /* Product string index */
    0x00,                               /* Serial number string index */
    0x01                                /* Number of configurations */
};

/* Standard Device Descriptor for USB 2 */
const uint8_t esUVCUSB20DeviceDscr[] =
{
    0x12,                               /* Descriptor size */
    CY_U3P_USB_DEVICE_DESCR,            /* Device descriptor type */
    0x10, 0x02,                         /* USB 2.1 */
    0xEF,                               /* Device class */
    0x02,                               /* Device sub-class */
    0x01,                               /* Device protocol */
    0x40,                               /* Maxpacket size for EP0 : 64 bytes */
    0x60, 0x25,                         /* Vendor ID */
    0x51, 0xD0,                         /* Product ID */
    0x00, 0x00,                         /* Device release number */
    0x01,                               /* Manufacture string index */
    0x02,                               /* Product string index */
    0x00,                               /* Serial number string index */
    0x01                                /* Number of configurations */
};

/* Binary Device Object Store (BOS) Descriptor */
const uint8_t esUVCUSBBOSDscr[] =
{
    0x05,                               /* Descriptor size */
    CY_U3P_BOS_DESCR,                   /* Device descriptor type */
    0x16, 0x00,                         /* Length of this descriptor and all sub descriptors */
    0x02,                               /* Number of device capability descriptors */
    /* USB 2.0 Extension */
    0x07,                               /* Descriptor size */
    CY_U3P_DEVICE_CAPB_DESCR,           /* Device capability type descriptor */
    CY_U3P_USB2_EXTN_CAPB_TYPE,         /* USB 2.1 extension capability type */
    0x02, 0x00, 0x00, 0x00,             /* Supported device level features - LPM support */

    /* SuperSpeed Device Capability */
    0x0A,                               /* Descriptor size */
    CY_U3P_DEVICE_CAPB_DESCR,           /* Device capability type descriptor */
    CY_U3P_SS_USB_CAPB_TYPE,            /* SuperSpeed device capability type */
    0x00,                               /* Supported device level features  */
    0x0E, 0x00,                         /* Speeds supported by the device : SS, HS and FS */
    0x03,                               /* Functionality support */
    0x00,                               /* U1 device exit latency */
    0x00, 0x00                          /* U2 device exit latency */
};

/* Standard Device Qualifier Descriptor */
const uint8_t esUVCUSBDeviceQualDscr[] =
{
    0x0A,                               /* descriptor size */
    CY_U3P_USB_DEVQUAL_DESCR,           /* Device qualifier descriptor type */
    0x00, 0x02,                         /* USB 2.0 */
    0xEF,                               /* Device class */
    0x02,                               /* Device sub-class */
    0x01,                               /* Device protocol */
    0x40,                               /* Maxpacket size for EP0 : 64 bytes */
    0x01,                               /* Number of configurations */
    0x00                                /* Reserved */
};

/* Standard Super Speed Configuration Descriptor */
const uint8_t esUVCUSBSSConfigDscr[] =
{
    /* Configuration Descriptor*/
    0x09,                               /* Descriptor Size */
    CY_U3P_USB_CONFIG_DESCR,            /* Configuration Descriptor Type */
    0x3E, 0x01,                         /* Length of this descriptor and all sub descriptors */
    0x02,                               /* Number of interfaces */
    0x01,                               /* Configuration number */
    0x03,                               /* Configuration string index */
    0xC0,                               /* Config characteristics - Self Powered */
    0x0C,                               /* Max power consumption of device (in 8mA unit) : 96mA */

    /* Interface Association Descriptor */
    0x08,                               /* Descriptor Size */
    ES_UVC_INTRFC_ASSN_DESCR,              /* Interface Association Descriptor Type */
    0x00,                               /* Interface number of the VideoControl interface
                                           that is associated with this function*/
    0x02,                               /* Number of contiguous Video interfaces that are
                                           associated with this function */
    0x0E,                               /* Video Interface Class Code: CC_VIDEO */
    0x03,                               /* Subclass code: SC_VIDEO_INTERFACE_COLLECTION*/
    0x00,                               /* Protocol: PC_PROTOCOL_UNDEFINED*/
    0x00,                               /* String Descriptor index for interface */

    /* Standard Video Control Interface Descriptor (Interface 0, Alternate Setting 0)*/
    0x09,                               /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,            /* Interface Descriptor type */
    0x00,                               /* Index of this Interface */
    0x00,                               /* Alternate setting number */
    0x01,                               /* Number of end points - 1 Interrupt Endpoint*/
    0x0E,                               /* Video Interface Class Code: CC_VIDEO  */
    0x01,                               /* Interface sub class: SC_VIDEOCONTROL */
    0x00,                               /* Interface protocol code: PC_PROTOCOL_UNDEFINED.*/
    0x00,                               /* Interface descriptor string index */

    /* Class specific VC Interface Header Descriptor */
    0x0D,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,                /* Class Specific Interface Descriptor type: CS_INTERFACE */
    0x01,                               /* Descriptor Sub type: VC_HEADER */
    0x00, 0x01,                         /* Revision of UVC class spec: 1.0 - Legacy version */
    0x51, 0x00,                         /* Total Size of class specific descriptors (till Output terminal) */
    0x00, 0x6C, 0xDC, 0x02,             /* Clock frequency : 48MHz(Deprecated) */
    0x01,                               /* Number of streaming interfaces */
    0x01,                               /*VideoStreaming interface 1 belongs to this VideoControl interface*/

    /* Input (Camera) Terminal Descriptor */
    0x12,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,                /* CS_INTERFACE */
    0x02,                               /* VC_INPUT_TERMINAL subtype */
    0x01,                               /* ID of this input terminal */
    0x01, 0x02,                         /* ITT_CAMERA type. This terminal is a camera
                                           terminal representing the CCD sensor*/
    0x00,                               /* No association terminal */
    0x00,                               /* Unused */
    0x00, 0x00,                         /* No optical zoom supported */
    0x00, 0x00,                         /* No optical zoom supported */
    0x00, 0x00,                         /* No optical zoom supported */
    0x03,                               /* Size of controls field for this terminal : 3 bytes */
	0x2A,0x00,0x02,                 	/* No controls supported */

    /* Processing Unit Descriptor */
    0x0D,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,                /* Class specific interface desc type */
    0x05,                               /* Processing Unit Descriptor type: VC_PROCESSING_UNIT*/
    0x02,                               /* ID of this unit */
    0x01,                               /* Source ID: 1: Conencted to input terminal */
    0x00, 0x40,                         /* Digital multiplier */
    0x03,                               /* Size of controls field for this terminal: 3 bytes */
	0x5F,0x10,0x00,                 	/* No controls supported */
    0x00,                               /* String desc index: Not used */
    0x00,                               /* Analog Video Standards Supported: None */

    /* Extension Unit Descriptor */
    0x1C,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,                /* Class specific interface desc type */
    0x06,                               /* Extension Unit Descriptor type */
    0x03,                               /* ID of this terminal */
    0xFF, 0xFF, 0xFF, 0xFF,             /* 16 byte GUID */
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0x00,                               /* Number of controls in this terminal */
    0x01,                               /* Number of input pins in this terminal */
    0x02,                               /* Source ID : 2 : Connected to Proc Unit */
    0x03,                               /* Size of controls field for this terminal : 3 bytes */
    0x00, 0x00, 0x00,                   /* No controls supported */
    0x00,                               /* String descriptor index : Not used */

    /* Output Terminal Descriptor */
    0x09,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,                /* Class specific interface desc type */
    0x03,                               /* Output Terminal Descriptor type */
    0x04,                               /* ID of this terminal */
    0x01, 0x01,                         /* USB Streaming terminal type */
    0x00,                               /* No association terminal */
    0x03,                               /* Source ID : 3 : Connected to Extn Unit */
    0x00,                               /* String desc index : Not used */

    /* Video Control Status Interrupt Endpoint Descriptor */
    0x07,                               /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,            /* Endpoint Descriptor Type */
    ES_UVC_EP_CONTROL_STATUS,              /* Endpoint address and description: EP-2 IN */
    CY_U3P_USB_EP_INTR,                 /* Interrupt End point Type */
    0x40, 0x00,                         /* Max packet size: 64 bytes */
    0x01,                               /* Servicing interval */

    /* Super Speed Endpoint Companion Descriptor */
    0x06,                               /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,           /* SS Endpoint Companion Descriptor Type */
    0x00,                               /* Max no. of packets in a Burst: 1 */
    0x00,                               /* Attribute: N.A. */
    0x40,                               /* Bytes per interval: 1024 */
    0x00,

    /* Class Specific Interrupt Endpoint Descriptor */
    0x05,                               /* Descriptor size */
    0x25,                               /* Class Specific Endpoint Descriptor Type */
    CY_U3P_USB_EP_INTR,                 /* End point Sub Type */
    0x40, 0x00,                         /* Max packet size = 64 bytes */

    /* Standard Video Streaming Interface Descriptor (Interface 1, Alternate Setting 0) */
    0x09,                               /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,            /* Interface Descriptor type */
    0x01,                               /* Interface number: 1 */
    0x00,                               /* Alternate setting number: 0 */
    0x01,                               /* Number of end points: 1 Bulk Endpoint */
    0x0E,                               /* Interface class : CC_VIDEO */
    0x02,                               /* Interface sub class : SC_VIDEOSTREAMING */
    0x00,                               /* Interface protocol code : PC_PROTOCOL_UNDEFINED */
    0x00,                               /* Interface descriptor string index */

    /* Class-specific Video Streaming Input Header Descriptor */
    0x0E,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,                /* Class-specific VS I/f Type */
    0x01,                               /* Descriptor Subtype: Input Header */
    0x01,                               /* 1 format desciptor follows */
    0xAB,0x00,                          /* Total size of Class specific VS descr */
    ES_UVC_EP_BULK_VIDEO,                  /* EP address for BULK video data: EP 3 IN */
    0x00,                               /* No dynamic format change supported */
    0x04,                               /* Output terminal ID : 4 */
    0x02,                               /* No Still image capture method supported */
    0x01,                               /* Hardware trigger NOT supported */
    0x01,                               /* Hardware to initiate still image capture not supported */
    0x01,                               /* Size of controls field : 1 byte */
    0x00,                               /* D2 : Compression quality supported - No compression*/

    /* Class specific Uncompressed VS format descriptor */
    0x1B,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,                /* Class-specific VS interface Type */
    0x04,                               /* Subtype : VS_FORMAT_UNCOMPRESSED */
    0x01,                               /* Format desciptor index */
    0x04,                               /* Number of Frame Descriptors that follow this descriptor: 3 */

    /* GUID, globally unique identifier used to identify streaming-encoding format: YUY2  */
    0x59, 0x55, 0x59, 0x32,             /*MEDIASUBTYPE_YUY2 GUID: 32595559-0000-0010-8000-00AA00389B71 */
    0x00, 0x00, 0x10, 0x00,
    0x80, 0x00, 0x00, 0xAA,
    0x00, 0x38, 0x9B, 0x71,
    0x10,                               /* Number of bits per pixel: 16*/
    0x01,                               /* Optimum Frame Index for this stream: 2 (720p) */
    0x00,                               /* X dimension of the picture aspect ratio; Non-interlaced */
    0x00,                               /* Y dimension of the pictuer aspect ratio: Non-interlaced */
    0x00,                               /* Interlace Flags: Progressive scanning, no interlace */
    0x00,                               /* duplication of the video stream restriction: 0 - no restriction */

    /* Class specific Uncompressed VS Frame Descriptor 1 - 1080p@30fps */
    0x1E,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,                /* Descriptor type*/
    0x05,                               /* Subtype: Uncompressed frame interface*/
    0x01,                               /* Frame Descriptor Index: 1 */
    0x00,                               /* No Still image capture method supported */
    0x80, 0x07,                         /* Width in pixel:  1080 */
    0x38, 0x04,                         /* Height in pixel: 1920 */
    0x00, 0x80, 0x53, 0x3B,             /* Min bit rate (bits/s): 1080 x 1920 x 2 x 30 x 8 = 995328000 */
    0x00, 0x80, 0x53, 0x3B,             /* Max bit rate (bits/s): Fixed rate so same as Min */
    0x00, 0x48, 0x3F, 0x00,             /* Maximum video or still frame size in bytes(Deprecated): 1920 x 1080 x 2 */
    0x15, 0x16, 0x05, 0x00,             /* Default frame interval (in 100ns units): (1/30)x10^7 */
    0x01,                               /* Frame interval type : No of discrete intervals */
    0x15, 0x16, 0x05, 0x00,             /* Frame interval 3: Same as Default frame interval */


    /* Class specific Uncompressed VS frame descriptor 1 - 640x480@60fps*/
    0x1E,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,                /* Descriptor type*/
    0x05,                               /* Subtype: Uncompressed frame interface*/
    0x02,                               /* Frame Descriptor Index: 1 */
    0x00,                               /* No Still image capture method supported */
    0x80, 0x02,                         /* Width in pixel:  640 */
    0xE0, 0x01,                         /* Height in pixel: 480 */
    0x00, 0x00, 0x94, 0x11,             /* Min bit rate (bits/s): 640 x 480 x 2 x 60 x 8 = 294912000 */
    0x00, 0x00, 0x94, 0x11,             /* Max bit rate (bits/s): Fixed rate so same as Min  */
    0x00, 0x60, 0x09, 0x00,             /* Maximum video or still frame size in bytes(Deprecated): 640 x 480 x 2*/
    0x0A, 0x8B, 0x02, 0x00,             /* Default frame interval (in 100ns units): (1/60)x10^7 */
    0x01,                               /* Frame interval type : No of discrete intervals */
    0x0A, 0x8B, 0x02, 0x00,             /* Frame interval 3: Same as Default frame interval */

    /* Class specific Uncompressed VS frame descriptor 2 - 720p@ 60fps*/
    0x1E,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,                /* Descriptor type*/
    0x05,                               /* Subtype: Uncompressed frame interface*/
    0x03,                               /* Frame Descriptor Index: 2 */
    0x00,                               /* No Still image capture method supported */
    0x00, 0x05,                         /* Width in pixel: 720 */
    0xD0, 0x02,                         /* Height in pixel: 1280 */
    0x00, 0x00, 0x57, 0x30,             /* Min bit rate @55fps (bits/s): 720 x 1280 x 2 x 55 x 8 = 811008000 */
    0x00, 0x00, 0xBC, 0x34,             /* Max bit rate @60fps (bits/s). 720 x 1280 x 2 x 60 x 8 884736000 */
    0x00, 0x20, 0x1C, 0x00,             /* Maximum video frame size in bytes(Deprecated): 1280 x 720 x 2   */
    0x0A, 0x8B, 0x02, 0x00,             /* Default frame interval (in 100ns units): (1/60)x10^7 */
    0x01,                               /* Frame interval type : No of discrete intervals */
    0x0A, 0x8B, 0x02, 0x00,             /* Frame interval 3: Same as Default frame interval */


    /* Class specific Uncompressed VS frame descriptor 3 - 5MP@15fps*/
    0x1E,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,                /* Descriptor type*/
    0x05,                               /* Subtype: Uncompressed frame interface*/
    0x04,                               /* Frame Descriptor Index: 3 */
    0x00,                               /* No Still image capture method supported */
    0x20, 0x0A,                         /* Width in pixel: 2592 */
    0x98, 0x07,                         /* Height in pixel: 1944 */
    0x00, 0xD0, 0x14, 0x48,             /* Min bit rate @15fps (bits/s): 2592 x 1944 x 2 x 15 x 8 = 1209323520 */
    0x00, 0xD0, 0x14, 0x48,             /* Max bit rate @15fps (bits/s): 2592 x 1944 x 2 x 15 x 8 = 1209323520 */
    0x00, 0xC6, 0x99, 0x00,             /* Maximum video frame size in bytes(Deprecated): 2592 x 1944 x 2 = 10077696  */
    0x2A, 0x2C, 0x0A, 0x00,             /* Default frame interval (in 100ns units): (1/15)x10^7 */
    0x01,                               /* Frame interval type : No of discrete intervals */
    0x2A, 0x2C, 0x0A, 0x00,             /* Frame interval 3: Same as Default frame interval */

    /* Still image descriptor -YUV with QVGA resolution */
	0x0A,
	0x24,
	0x03,
	0x00,
	0x01,					//No of frame Resolutions Follows
	0x20,0x0A,				//2592x1944
	0x98,0x07,
	0x00,

    /* Endpoint Descriptor for BULK Streaming Video Data */
    0x07,                               /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,            /* Endpoint Descriptor Type */
    ES_UVC_EP_BULK_VIDEO,               /* Endpoint address and description: EP 3 IN */
    CY_U3P_USB_EP_BULK,                 /* BULK End point */
    ES_UVC_EP_BULK_VIDEO_PKT_SIZE_L,    /* ES_UVC_EP_BULK_VIDEO_PKT_SIZE_L */
    ES_UVC_EP_BULK_VIDEO_PKT_SIZE_H,    /* ES_UVC_EP_BULK_VIDEO_PKT_SIZE_H */
    0x00,                               /* Servicing interval for data transfers */

    /* Super Speed Endpoint Companion Descriptor */
    0x06,                               /* Descriptor size */
    CY_U3P_SS_EP_COMPN_DESCR,           /* SS Endpoint Companion Descriptor Type */
    0x0F,                               /* Max number of packets per burst: 12 */
    0x00,                               /* Attribute: Streams not defined */
    0x00,                               /* No meaning for bulk */
    0x00
};

/* Standard High Speed Configuration Descriptor */
const uint8_t esUVCUSBHSConfigDscr[] =
{
    /* Configuration descriptor */
    0x09,                               /* Descriptor size */
    CY_U3P_USB_CONFIG_DESCR,            /* Configuration descriptor type */
    0xCD,0x00,                          /* Length of this descriptor and all sub descriptors */
    0x02,                               /* Number of interfaces */
    0x01,                               /* Configuration number */
    0x04,                               /* Configuration string index */
    0xC0,                               /* Config characteristics - self powered */
    0x32,                               /* Max power consumption of device (in 2mA unit) : 100mA */

    /* Interface Association Descriptor */
    0x08,                               /* Descriptor Size */
    ES_UVC_INTRFC_ASSN_DESCR,              /* Interface Association Descriptor Type */
    0x00,                               /* Interface number of the VideoControl interface
                                           that is associated with this function*/
    0x02,                               /* Number of contiguous Video interfaces that are
                                           associated with this function */
    0x0E,                               /* Video Interface Class Code: CC_VIDEO */
    0x03,                               /* Subclass code: SC_VIDEO_INTERFACE_COLLECTION*/
    0x00,                               /* Protocol: PC_PROTOCOL_UNDEFINED*/
    0x00,                               /* String Descriptor index for interface */

    /* Standard Video Control Interface Descriptor (Interface 0, Alternate Setting 0)*/
    0x09,                               /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,            /* Interface Descriptor type */
    0x00,                               /* Index of this Interface */
    0x00,                               /* Alternate setting number */
    0x01,                               /* Number of end points - 1 Interrupt Endpoint*/
    0x0E,                               /* Video Interface Class Code: CC_VIDEO  */
    0x01,                               /* Interface sub class: SC_VIDEOCONTROL */
    0x00,                               /* Interface protocol code: PC_PROTOCOL_UNDEFINED.*/
    0x00,                               /* Interface descriptor string index */

    /* Class specific VC Interface Header Descriptor */
    0x0D,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,             /* Class Specific Interface Descriptor type: CS_INTERFACE */
    0x01,                               /* Descriptor Sub type: VC_HEADER */
    0x00, 0x01,                         /* Revision of UVC class spec: 1.0 - Legacy version */
    0x50, 0x00,                         /* Total Size of class specific descriptors
                                           (till Output terminal) */
    0x00, 0x6C, 0xDC, 0x02,             /* Clock frequency : 48MHz(Deprecated) */
    0x01,                               /* Number of streaming interfaces */
    0x01,                               /*VideoStreaming interface 1 belongs to this
                                          VideoControl interface*/

    /* Input (Camera) Terminal Descriptor */
    0x12,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,             /* CS_INTERFACE */
    0x02,                               /* VC_INPUT_TERMINAL subtype */
    0x01,                               /* ID of this input terminal */
    0x01, 0x02,                         /* ITT_CAMERA type. This terminal is a camera
                                           terminal representing the CCD sensor*/
    0x00,                               /* No association terminal */
    0x00,                               /* Unused */
    0x00, 0x00,                         /* No optical zoom supported */
    0x00, 0x00,                         /* No optical zoom supported */
    0x00, 0x00,                         /* No optical zoom supported */
    0x03,                               /* Size of controls field for this terminal : 3 bytes */
    0x2A,0x00,0x02,                 	/* No controls supported */

    /* Processing Unit Descriptor */
    0x0C,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,             /* Class specific interface desc type */
    0x05,                               /* Processing Unit Descriptor type: VC_PROCESSING_UNIT*/
    0x02,                               /* ID of this unit */
    0x01,                               /* Source ID: 1: Conencted to input terminal */
    0x00, 0x40,                         /* Digital multiplier */
    0x03,                               /* Size of controls field for this terminal: 3 bytes */
    0x5F,0x10,0x00,                 	/* No controls supported */
    0x00,                               /* String desc index: Not used */

    /* Extension Unit Descriptor */
    0x1C,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,             /* Class specific interface desc type */
    0x06,                               /* Extension Unit Descriptor type */
    0x03,                               /* ID of this terminal */
    0xFF, 0xFF, 0xFF, 0xFF,             /* 16 byte GUID */
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,
    0x00,                               /* Number of controls in this terminal */
    0x01,                               /* Number of input pins in this terminal */
    0x02,                               /* Source ID : 2 : Connected to Proc Unit */
    0x03,                               /* Size of controls field for this terminal : 3 bytes */
    0x00, 0x00, 0x00,                   /* No controls supported */
    0x00,                               /* String descriptor index : Not used */

    /* Output Terminal Descriptor */
    0x09,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,             /* Class specific interface desc type */
    0x03,                               /* Output Terminal Descriptor type */
    0x04,                               /* ID of this terminal */
    0x01, 0x01,                         /* USB Streaming terminal type */
    0x00,                               /* No association terminal */
    0x03,                               /* Source ID : 3 : Connected to Extn Unit */
    0x00,                               /* String desc index : Not used */

    /* Video Control Status Interrupt Endpoint Descriptor */
    0x07,                               /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,            /* Endpoint Descriptor Type */
    ES_UVC_EP_CONTROL_STATUS,           /* Endpoint address and description: EP-2 IN */
    CY_U3P_USB_EP_INTR,                 /* Interrupt End point Type */
    0x40, 0x00,                         /* Max packet size: 64 bytes */
    0x01,                               /* Servicing interval */

    /* Class Specific Interrupt Endpoint Descriptor */
    0x05,                               /* Descriptor size */
    0x25,                               /* Class specific endpoint descriptor type */
    CY_U3P_USB_EP_INTR,                 /* End point sub type */
    0x40,0x00,                          /* Max packet size = 64 bytes */

    /* Standard Video Streaming Interface Descriptor (Interface 1, Alternate Setting 0) */
    0x09,                               /* Descriptor size */
    CY_U3P_USB_INTRFC_DESCR,            /* Interface descriptor type */
    0x01,                               /* Interface number: 1 */
    0x00,                               /* Alternate setting number: 0 */
    0x01,                               /* Number of end points: 1 Bulk endopoint*/
    0x0E,                               /* Interface class : CC_VIDEO */
    0x02,                               /* Interface sub class : SC_VIDEOSTREAMING */
    0x00,                               /* Interface protocol code : PC_PROTOCOL_UNDEFINED */
    0x00,                               /* Interface descriptor string index */

    /* Class-specific Video Streaming Input Header Descriptor */
    0x0E,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,             /* Class-specific VS interface type */
    0x01,                               /* Descriptotor subtype : Input Header */
    0x01,                               /* 1 format desciptor follows */
    0x47, 0x00,                         /* Total size of class specific VS descr */
    ES_UVC_EP_BULK_VIDEO,               /* EP address for BULK video data: EP 3 IN  */
    0x00,                               /* No dynamic format change supported */
    0x04,                               /* Output terminal ID : 4 */
    0x00,                               /* No Still image capture method supported */
    0x00,                               /* Hardware trigger not supported */
    0x00,                               /* Hardware to initiate still image capture not supported*/
    0x01,                               /* Size of controls field : 1 byte */
    0x00,                               /* D2 : Compression quality supported - No compression */

    /* Class specific VS format descriptor */
    0x1B,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,             /* Class-specific VS interface Type */
    0x04,                               /* Subtype : VS_FORMAT_UNCOMPRESSED  */
    0x01,                               /* Format desciptor index */
    0x01,                               /* Number of Frame Descriptors that follow this descriptor: 2 */
    /* GUID, globally unique identifier used to identify streaming-encoding format: YUY2  */
    0x59, 0x55, 0x59, 0x32,             /*MEDIASUBTYPE_YUY2 GUID: 32595559-0000-0010-8000-00AA00389B71 */
    0x00, 0x00, 0x10, 0x00,
    0x80, 0x00, 0x00, 0xAA,
    0x00, 0x38, 0x9B, 0x71,
    0x10,                               /* Number of bits per pixel: 16*/
    0x01,                               /* Optimum Frame Index for this stream: 1 (640x480) */
    0x00,                               /* X dimension of the picture aspect ratio; Non-interlaced */
    0x00,                               /* Y dimension of the pictuer aspect ratio: Non-interlaced */
    0x00,                               /* Interlace Flags: Progressive scanning, no interlace */
    0x00,                               /* duplication of the video stream restriction: 0 - no restriction */

    /* Class specific Uncompressed VS frame descriptor 1 - 640x480@60fps*/
    0x1E,                               /* Descriptor size */
    ES_UVC_CS_INTRFC_DESCR,             /* Descriptor type*/
    0x05,                               /* Subtype: Uncompressed frame interface*/
    0x01,                               /* Frame Descriptor Index: 1 */
    0x00,                               /* No Still image capture method supported */
    0x80, 0x02,                         /* Width in pixel:  640 */
    0xE0, 0x01,                         /* Height in pixel: 480 */
    0x00, 0x00, 0x94, 0x11,             /* Min bit rate (bits/s): 640 x 480 x 2 x 60 x 8 = 294912000 */
    0x00, 0x00, 0x94, 0x11,             /* Max bit rate (bits/s): Fixed rate so same as Min  */
    0x00, 0x60, 0x09, 0x00,             /* Maximum video or still frame size in bytes(Deprecated): 640 x 480 x 2*/
    0x15, 0x16, 0x05, 0x00,             /* Default frame interval (in 100ns units): (1/60)x10^7 */
    0x01,                               /* Frame interval type : No of discrete intervals */
    0x15, 0x16, 0x05, 0x00,             /* Frame interval 3: Same as Default frame interval */

    /* Endpoint descriptor for Bulk streaming video data */
    0x07,                               /* Descriptor size */
    CY_U3P_USB_ENDPNT_DESCR,            /* Endpoint Descriptor Type */
    ES_UVC_EP_BULK_VIDEO,               /* Endpoint address and description: EP 3 IN */
    CY_U3P_USB_EP_BULK,                 /* BULK End point */
    0x00,                               /* Packet Size: 512 bytes */
    0x02,
    0x00                                /* Servicing interval for data transfers */
};

/* Standard Full Speed Configuration Descriptor*/
const uint8_t esUVCUSBFSConfigDscr[] =
{
		/* Configuration descriptor */
	    0x09,                           /* Descriptor size */
	    CY_U3P_USB_CONFIG_DESCR,        /* Configuration descriptor type */
	    0x09,0x00,                      /* Length of this descriptor and all sub descriptors */
	    0x00,                           /* Number of interfaces */
	    0x01,                           /* Configuration number */
	    0x00,                           /* COnfiguration string index */
	    0x80,                           /* Config characteristics - bus powered */
	    0x32,                           /* Max power consumption of device (in 2mA unit) : 100mA */
};

/* Standard language ID string descriptor */
const uint8_t esUVCUSBStringLangIDDscr[] =
{
    0x04,                               /* Descriptor size */
    CY_U3P_USB_STRING_DESCR,            /* Device descriptor type */
    0x09,0x04                           /* Language ID supported */
};

/* Standard manufacturer string descriptor */
const uint8_t esUVCUSBManufactureDscr[] =
{
	0x1A,                           /* Descriptor size */
	CY_U3P_USB_STRING_DESCR,        /* Device descriptor type */
	'e',0x00,
	'-',0x00,
	'c',0x00,
	'o',0x00,
	'n',0x00,
	's',0x00,
	'y',0x00,
	's',0x00,
	't',0x00,
	'e',0x00,
	'm',0x00,
	's',0x00
};

/* Standard product string descriptor */
const uint8_t esUVCUSBProductDscr[] =
{
	0x38,                           /* Descriptor Size */
	CY_U3P_USB_STRING_DESCR,        /* Device descriptor type */
	'e',0x00,
	'-',0x00,
	'c',0x00,
	'o',0x00,
	'n',0x00,
	'\'',0x00,
	's',0x00,
	' ',0x00,
	'C',0x00,
	'X',0x00,
	'3',0x00,
	' ',0x00,
	'R',0x00,
	'D',0x00,
	'K',0x00,
	' ',0x00,
	'w',0x00,
	'i',0x00,
	't',0x00,
	'h',0x00,
	' ',0x00,
	'O',0x00,
	'V',0x00,
	'5',0x00,
	'6',0x00,
	'4',0x00,
	'0',0x00,
};

/* Standard product string descriptor */
const uint8_t esUVCUSBConfigSSDscr[] =
{
    0x10,                               /* Descriptor Size */
    CY_U3P_USB_STRING_DESCR,            /* Device descriptor type */
    'U', 0x00,                          /* Super-Speed Configuration Descriptor */
    'S', 0x00,
    'B', 0x00,
    '-', 0x00,
    '3', 0x00,
    '.', 0x00,
    '0', 0x00
};
/* Standard product string descriptor */
const uint8_t esUVCUSBConfigHSDscr[] =
{
    0x10,                               /* Descriptor Size */
    CY_U3P_USB_STRING_DESCR,            /* Device descriptor type */
    'U', 0x00,                          /* High-Speed Configuration Descriptor */
    'S', 0x00,
    'B', 0x00,
    '-', 0x00,
    '2', 0x00,
    '.', 0x00,
    '1', 0x00
};
/* Standard product string descriptor */
const uint8_t esUVCUSBConfigFSDscr[] =
{
    0x10,                               /* Descriptor Size */
    CY_U3P_USB_STRING_DESCR,            /* Device descriptor type */
    'U', 0x00,                          /* Full-Speed Configuration Descriptor */
    'S', 0x00,
    'B', 0x00,
    '-', 0x00,
    '1', 0x00,
    '.', 0x00,
    '1', 0x00
};

/* UVC Probe Control Settings */
uint8_t glProbeCtrl[ES_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                         /* bmHint : No fixed parameters */
    0x01,                               /* Use 1st Video format index */
    0x01,                               /* Use 1st Video frame index */
    0x0A, 0x8B, 0x02, 0x00,             /* Desired frame interval in 100ns */
    0x00, 0x00,                         /* Key frame rate in key frame/video frame units */
    0x00, 0x00,                         /* PFrame rate in PFrame / key frame units */
    0x00, 0x00,                         /* Compression quality control */
    0x00, 0x00,                         /* Window size for average bit rate */
    0x00, 0x00,                         /* Internal video streaming i/f latency in ms */
    0x00, 0x48, 0x3F, 0x00,             /* Max video frame size in bytes */
    0x00, 0x90, 0x00, 0x00              /* No. of bytes device can rx in single payload: 32KB */
};

/* UVC Probe Control Setting - 1080p@30FPS(Super Speed) */
uint8_t const gl1080pProbeCtrl[ES_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                         /* bmHint : No fixed parameters */
    0x01,                               /* Use 1st Video format index */
    0x01,                               /* Use 1st Video frame index */
    0x15, 0x16, 0x05, 0x00,             /* Desired frame interval in 100ns = (1/30)x10^7 */
    0x00, 0x00,                         /* Key frame rate in key frame/video frame units */
    0x00, 0x00,                         /* PFrame rate in PFrame / key frame units */
    0x00, 0x00,                         /* Compression quality control */
    0x00, 0x00,                         /* Window size for average bit rate */
    0x00, 0x00,                         /* Internal video streaming i/f latency in ms */
    0x00, 0x48, 0x3F, 0x00,             /* Max video frame size in bytes = 1920 x 1080 x 2 */
    0x00, 0x90, 0x00, 0x00              /* No. of bytes device can rx in single payload: 36KB */
};

/* UVC Probe Control Setting - VGA @60fps (Super Speed)*/
uint8_t const glVga60ProbeCtrl[ES_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                         /* bmHint : No fixed parameters */
    0x01,                               /* Use 1st Video format index */
    0x02,                               /* Use 2nd Video frame index */
    0x0A, 0x8B, 0x02, 0x00,             /* Desired frame interval in 100ns = (1/60)x10^7 */
    0x00, 0x00,                         /* Key frame rate in key frame/video frame units */
    0x00, 0x00,                         /* PFrame rate in PFrame / key frame units */
    0x00, 0x00,                         /* Compression quality control */
    0x00, 0x00,                         /* Window size for average bit rate */
    0x00, 0x00,                         /* Internal video streaming i/f latency in ms */
    0x00, 0x60, 0x09, 0x00,             /* Max video frame size in bytes: 640 x 480 x 2*/
    0x00, 0x90, 0x00, 0x00              /* No. of bytes device can rx in single payload: 36KB */
};

/* UVC Probe Control Setting - 720p@60fps(Super Speed)*/
uint8_t const gl720pProbeCtrl[ES_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                         /* bmHint : No fixed parameters */
    0x01,                               /* Use 1st Video format index */
    0x03,                               /* Use 2nd Video frame index */
    0x0A, 0x8B, 0x02, 0x00,             /* Desired frame interval in 100ns = (1/60)x10^7*/
    0x00, 0x00,                         /* Key frame rate in key frame/video frame units */
    0x00, 0x00,                         /* PFrame rate in PFrame / key frame units */
    0x00, 0x00,                         /* Compression quality control */
    0x00, 0x00,                         /* Window size for average bit rate */
    0x00, 0x00,                         /* Internal video streaming i/f latency in ms */
    0x00, 0x20, 0x1C, 0x00,             /* Max video frame size in bytes: 720 x 1280 x 2*/
    0x00, 0x90, 0x00, 0x00              /* No. of bytes device can rx in single payload: 36KB */
};

/* UVC Probe Control Setting - 5Mp@15FPS(Super Speed) */
uint8_t const gl5MpProbeCtrl[ES_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                         /* bmHint : No fixed parameters */
    0x01,                               /* Use 1st Video format index */
    0x04,                               /* Use 1st Video frame index */
    0x2A, 0x2C, 0x0A, 0x00,             /* Desired frame interval in 100ns = (1/15)x10^7 */
    0x00, 0x00,                         /* Key frame rate in key frame/video frame units */
    0x00, 0x00,                         /* PFrame rate in PFrame / key frame units */
    0x00, 0x00,                         /* Compression quality control */
    0x00, 0x00,                         /* Window size for average bit rate */
    0x00, 0x00,                         /* Internal video streaming i/f latency in ms */
    0x00, 0xC6, 0x99, 0x00,             /* Max video frame size in bytes = 2592 x 1944 x 2 */
    0x00, 0x90, 0x00, 0x00              /* No. of bytes device can rx in single payload: 36KB */
};
/* UVC Probe Control Setting - VGA @30fps (High Speed)*/
uint8_t const glVga30ProbeCtrl[ES_UVC_MAX_PROBE_SETTING] = {
    0x00, 0x00,                         /* bmHint : No fixed parameters */
    0x01,                               /* Use 1st Video format index */
    0x01,                               /* Use 2nd Video frame index */
    0x0A, 0x8B, 0x02, 0x00,             /* Desired frame interval in 100ns = (1/60)x10^7 */
    0x00, 0x00,                         /* Key frame rate in key frame/video frame units */
    0x00, 0x00,                         /* PFrame rate in PFrame / key frame units */
    0x00, 0x00,                         /* Compression quality control */
    0x00, 0x00,                         /* Window size for average bit rate */
    0x00, 0x00,                         /* Internal video streaming i/f latency in ms */
    0x00, 0x60, 0x09, 0x00,             /* Max video frame size in bytes: 640 x 480 x 2*/
    0x00, 0x30, 0x00, 0x00              /* No. of bytes device can rx in single payload: 36KB */
};

/* Still Probe Control Setting */
uint8_t glStillProbeCtrl[ES_UVC_MAX_STILL_PROBE_SETTING] =
{
    0x01,                            	/* Use 1st Video format index */
    0x01,                            	/* Use 1st Video frame index */
    0x00,							 	/* Compression quality */
    0x00,0xC6,0x99,0x00,             	/* Max video frame size in bytes (100KB) */
    0x00,0x30,0x00,0x00              	/* No. of bytes device can rx in single payload */
};

/* Configuration parameters for 5Mp @15FPS for the OV5640 sensor */
CyU3PMipicsiCfg_t cfgUvc5Mp15NoMclk =  {
		CY_U3P_CSI_DF_YUV422_8_2,     	/* dataFormat   */
	    2,                          	/* numDataLanes */
	    1,                        		/* pllPrd       */
	    64,                         	/* pllFbd       */
	    CY_U3P_CSI_PLL_FRS_500_1000M, 	/* pllFrs      */
	    CY_U3P_CSI_PLL_CLK_DIV_8,   	/* csiRxClkDiv  */
	    CY_U3P_CSI_PLL_CLK_DIV_8,   	/* parClkDiv    */
	    0x00,                       	/* mclkCtl      */
	    CY_U3P_CSI_PLL_CLK_DIV_2,   	/* mClkRefDiv   */
	    2592,                       	/* hResolution  */
	    0x01                       		/* fifoDelay    */
};

/* Configuration parameters for 1080p @30FPS for the OV5640 sensor */
CyU3PMipicsiCfg_t cfgUvc1080p30NoMclk =  {
		CY_U3P_CSI_DF_YUV422_8_2,     	/* dataFormat   */
		2,                          	/* numDataLanes */
		1,                        		/* pllPrd       */
		62,                         	/* pllFbd       */
		CY_U3P_CSI_PLL_FRS_500_1000M, 	/* pllFrs      */
		CY_U3P_CSI_PLL_CLK_DIV_8,   	/* csiRxClkDiv  */
		CY_U3P_CSI_PLL_CLK_DIV_8,   	/* parClkDiv    */
		0x00,                       	/* mclkCtl      */
		CY_U3P_CSI_PLL_CLK_DIV_8,   	/* mClkRefDiv   */
		1920,                       	/* hResolution  */
		0x01                       		/* fifoDelay    */
};

/* Configuration parameters for 720p @60FPS for the OV5640 sensor */
CyU3PMipicsiCfg_t cfgUvc720p60NoMclk =  {
		CY_U3P_CSI_DF_YUV422_8_2,     	/* dataFormat   */
		2,                          	/* numDataLanes */
		1,                        		/* pllPrd       */
		62,                         	/* pllFbd       */
		CY_U3P_CSI_PLL_FRS_250_500M, 	/* pllFrs      */
		CY_U3P_CSI_PLL_CLK_DIV_4,   	/* csiRxClkDiv  */
		CY_U3P_CSI_PLL_CLK_DIV_4,   	/* parClkDiv    */
		0x00,                       	/* mclkCtl      */
		CY_U3P_CSI_PLL_CLK_DIV_8,   	/* mClkRefDiv   */
		1280,                       	/* hResolution  */
		0x01                       		/* fifoDelay    */
};

/* Configuration parameters for VGA @60FPS for the OV5640 sensor */
CyU3PMipicsiCfg_t cfgUvcVgaNoMclk =  {
		CY_U3P_CSI_DF_YUV422_8_2,       /* dataFormat   */
		1,                            	/* numDataLanes */
		0x1,                          	/* pllPrd       */
		90,                           	/* pllFbd       */
		CY_U3P_CSI_PLL_FRS_125_250M,  	/* pllFrs      */
		CY_U3P_CSI_PLL_CLK_DIV_2,     	/* csiRxClkDiv  */
		CY_U3P_CSI_PLL_CLK_DIV_8,     	/* parClkDiv    */
		0x00,                         	/* mclkCtl      */
		CY_U3P_CSI_PLL_CLK_DIV_8,     	/* mClkRefDiv   */
		640,                          	/* hResolution  */
		0x01                          	/* fifoDelay    */
};

/* Configuration parameters for VGA @30FPS for the OV5640 sensor */
CyU3PMipicsiCfg_t cfgUvcVga30NoMclk =  {
		CY_U3P_CSI_DF_YUV422_8_2,       /* dataFormat   */
		1,                            	/* numDataLanes */
		0x4,                          	/* pllPrd       */
		0x6F,                           /* pllFbd       */
		CY_U3P_CSI_PLL_FRS_250_500M,   	/* pllFrs      */
		CY_U3P_CSI_PLL_CLK_DIV_2,     	/* csiRxClkDiv  */
		CY_U3P_CSI_PLL_CLK_DIV_8,     	/* parClkDiv    */
		0x00,                         	/* mclkCtl      */
		CY_U3P_CSI_PLL_CLK_DIV_8,     	/* mClkRefDiv   */
		640,                          	/* hResolution  */
		0x01                          	/* fifoDelay    */
};
/* [ ] */

