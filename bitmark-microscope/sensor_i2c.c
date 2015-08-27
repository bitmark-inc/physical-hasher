// Cypress CX3 Firmware Example Source (sensor_i2c.c)
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

#include "sensor_i2c.h"


// I2C Slave address for the image sensor
// Slave address used to read and write sensor registers.
#define SENSOR_WRITE_ADDRESS      0x20
#define SENSOR_READ_ADDRESS      0x21

// Mask to get actual I2C slave address value without direction bit.
#define I2C_SLAVEADDR_MASK  0xFE

// standard Linux definition - Invalid argument
#define EINVAL              22

// This function inserts a delay between successful I2C transfers to
// prevent false errors due to the slave being busy.
static void SensorI2CAccessDelay(CyU3PReturnStatus_t status) {
	if (status == CY_U3P_SUCCESS) {
		CyU3PBusyWait(10);
	}
}

// Write to an I2C slave with two bytes of data.
static CyU3PReturnStatus_t SensorWrite2B(uint8_t slaveAddress,
					 uint8_t highAddress,
					 uint8_t lowAddress,
					 uint8_t highData,
					 uint8_t lowData) {
	CyU3PReturnStatus_t api_status = CY_U3P_SUCCESS;
	CyU3PI2cPreamble_t preamble;
	uint8_t buf[2];


	// Validate the I2C slave address.
	if (slaveAddress != SENSOR_WRITE_ADDRESS)
	{
		CyU3PDebugPrint(4, "I2C Slave address is not valid!\r\n");
		return 1;
	}

	// Set the parameters for the I2C API access and then call the write API.
	preamble.buffer[0] = slaveAddress;
	preamble.buffer[1] = highAddress;
	preamble.buffer[2] = lowAddress;
	preamble.length = 3;            //  Three byte preamble.
	preamble.ctrlMask = 0x0000;     //  No additional start and stop bits.

	buf[0] = highData;
	buf[1] = lowData;

	api_status = CyU3PI2cTransmitBytes(&preamble, buf, 2, 0);
	SensorI2CAccessDelay(api_status);

	if (api_status == CY_U3P_SUCCESS) {
		CyU3PDebugPrint (4, "Write Camera REG address = 0x%x%x%x%x data = 0x%x%x%x%x\r\n",
				 highAddress >> 4,
				 highAddress & 0x0f,
				 lowAddress >> 4,
				 lowAddress & 0x0f,
				 highData >> 4,
				 highData & 0x0f,
				 lowData >> 4,
				 lowData & 0x0f);

	} else {
		CyU3PDebugPrint (4, "Failed to write . Error Code = 0x%x\r\n", api_status);
	}

	return api_status;
}

#if 0
static CyU3PReturnStatus_t SensorWrite(uint8_t slaveAddress,
				       uint8_t highAddress,
				       uint8_t lowAddress,
				       uint8_t count,
				       uint8_t *buffer) {
	CyU3PReturnStatus_t api_status = CY_U3P_SUCCESS;
	CyU3PI2cPreamble_t preamble;

	// Validate the I2C slave address.
	if (slaveAddress != SENSOR_WRITE_ADDRESS) {
		CyU3PDebugPrint(4, "I2C Slave address is not valid!\r\n");
		return 1;
	}

	if (count > 64) {
		CyU3PDebugPrint(4, "ERROR: SensorWrite count > 64\r\n");
		return 1;
	}

	// Set up the I2C control parameters and invoke the write API.
	preamble.buffer[0] = slaveAddress;
	preamble.buffer[1] = highAddress;
	preamble.buffer[2] = lowAddress;
	preamble.length = 3;
	preamble.ctrlMask = 0x0000;

	api_status = CyU3PI2cTransmitBytes(&preamble, buffer, count, 0);
	SensorI2CAccessDelay(api_status);

	return api_status;
}
#endif

CyU3PReturnStatus_t SensorRead2B(uint8_t slaveAddress,
				 uint8_t highAddress,
				 uint8_t lowAddress,
				 uint8_t *buffer) {
	CyU3PReturnStatus_t api_status = CY_U3P_SUCCESS;
	CyU3PI2cPreamble_t preamble;

	preamble.buffer[0] = slaveAddress & I2C_SLAVEADDR_MASK;    //  Mask out the transfer type bit.
	preamble.buffer[1] = (uint8_t) highAddress;
	preamble.buffer[2] = (uint8_t) lowAddress;
	preamble.buffer[3] =  (slaveAddress | 0x01);
	preamble.length = 4;
	preamble.ctrlMask = 0x0004;     //  Send start bit after third byte of preamble.

	api_status = CyU3PI2cReceiveBytes(&preamble, buffer, 2, 0);
	SensorI2CAccessDelay(api_status);

	return api_status;
}

#if 0
static CyU3PReturnStatus_t SensorRead(uint8_t slaveAddress,
				      uint8_t highAddress,
				      uint8_t lowAddress,
				      uint8_t count,
				      uint8_t *buffer) {
	CyU3PReturnStatus_t api_status = CY_U3P_SUCCESS;
	CyU3PI2cPreamble_t preamble;

	// Validate the parameters.
	if (slaveAddress != SENSOR_READ_ADDRESS) {
		CyU3PDebugPrint(4, "I2C Slave address is not valid!\r\n");
		return 1;
	}

	if (count > 64) {
		CyU3PDebugPrint(4, "ERROR: SensorWrite count > 64\r\n");
		return 1;
	}

	preamble.buffer[0] = slaveAddress & I2C_SLAVEADDR_MASK;    //  Mask out the transfer type bit.
	preamble.buffer[1] = highAddress;
	preamble.buffer[2] = lowAddress;
	preamble.buffer[3] = slaveAddress;
	preamble.length = 4;
	preamble.ctrlMask = 0x0004; // Send start bit after third byte of preamble.

	api_status = CyU3PI2cReceiveBytes(&preamble, buffer, count, 0);
	SensorI2CAccessDelay(api_status);

	return api_status;
}
#endif


CyU3PReturnStatus_t sensor_write(uint16_t address, uint16_t data) {
	int count = 0;

	uint8_t highAddress;
	uint8_t lowAddress;
	uint8_t highData;
	uint8_t lowData;

	CyU3PReturnStatus_t api_status;

	highAddress  = (uint8_t) ((address >> 8) & 0x00ff);
	lowAddress   = (uint8_t) ((address >> 0) & 0x00ff);

	highData  = (uint8_t) ((data >> 8) & 0x00ff);
	lowData   = (uint8_t) ((data >> 0) & 0x00ff);

	api_status = SensorWrite2B(SENSOR_WRITE_ADDRESS,highAddress,lowAddress,highData,lowData);

	while ((api_status != CY_U3P_SUCCESS) && (count < 2)) {
		api_status = SensorWrite2B(SENSOR_WRITE_ADDRESS,highAddress,lowAddress,highData,lowData);
		count++;
	}
	if (count > 0) {
		CyU3PDebugPrint(4, "Sensor read retry = %d\r\n",count);
	}
	return api_status;
}


CyU3PReturnStatus_t sensor_write_array(const sensor_data_t *data_list, size_t array_size) {
	CyU3PReturnStatus_t api_status;

	if(!data_list) {
		return -EINVAL;
	}

	int i = 0;
	for (i = 0; i < array_size; ++i, ++data_list) {
		if (data_list->address == SENSOR_DELAY) {
			CyU3PThreadSleep(data_list->data);
		} else {
			api_status = sensor_write(data_list->address, data_list->data);
		}
	}
	return api_status;
}


CyU3PReturnStatus_t sensor_read(uint16_t address, uint16_t *value) {
	int count = 0;
	uint8_t highAddress;
	uint8_t lowAddress;
	uint8_t buffer[2];
	CyU3PReturnStatus_t api_status;

	highAddress  = (uint8_t) ((address >> 8) & 0x00ff);
	lowAddress   = (uint8_t) ((address >> 0) & 0x00ff);

	api_status = SensorRead2B(SENSOR_READ_ADDRESS, highAddress, lowAddress, buffer);
	while ((api_status != CY_U3P_SUCCESS) && (count<3)) {
		api_status = SensorRead2B(SENSOR_READ_ADDRESS, highAddress, lowAddress, buffer);
		count++;
	}

	*value = (unsigned short) ((buffer[0] << 8) + buffer[1]);

	if (count > 0) {
		CyU3PDebugPrint(4, "Sensor read retry = %d\r\n",count);
	}
	return api_status;
}


#if 0
CyU3PReturnStatus_t sensor_read_array(const address_data_t *data_list, size_t array_size) {
	unsigned short temp = 0;
	int i=0;
	CyU3PReturnStatus_t api_status;

	for (i = 0; i < array_size; ++i, ++data_list) {
		if (data_list->address == DELAY) {
			CyU3PThreadSleep(data_list->data);
		}
		api_status = sensor_read(data_list->address, &temp);
		// CyU3PDebugPrint (4, "Read 0x%x=0x%2x\r\n", data_list->address, temp);
	}
	return api_status;
}
#endif


#if 0
// Reset the image sensor using MIPI XRESET
void sensor_reset(void) {
	CyU3PReturnStatus_t api_status;

	// initialise the MIPI  block

	api_status = CyU3PMipicsiInit();
	if (api_status != CY_U3P_SUCCESS) {
		CyU3PDebugPrint(4, "MIPI Init, Error Code = %d\r\n", api_status);
		//goto handle_fatal_error;
	}
	CyU3PThreadSleep(200);

	CyU3PDebugPrint(4, "XRST pulse: %d\r\n");

	// Send Xreset pulse to rease Image sensor
	api_status = CyU3PMipicsiSetSensorControl(CY_U3P_CSI_IO_XRES, CyTrue);
	if (api_status != CY_U3P_SUCCESS) {
		CyU3PDebugPrint(4, "XRES, Error Code = %d\r\n", api_status);
	}
	CyU3PThreadSleep(200);

	api_status = CyU3PMipicsiSetSensorControl(CY_U3P_CSI_IO_XRES, CyFalse);
	if (api_status != CY_U3P_SUCCESS) {
		CyU3PDebugPrint(4, "XRES, Error Code = %d\r\n", api_status);
	}
	CyU3PThreadSleep(200);

	api_status = CyU3PMipicsiSetSensorControl(CY_U3P_CSI_IO_XRES, CyTrue);
	if (api_status != CY_U3P_SUCCESS) {
		CyU3PDebugPrint(4, "XRES, Error Code = %d\r\n", api_status);
	}
	CyU3PThreadSleep(200);
}
#endif


#if 0
//   Verify that the sensor can be accessed over the I2C bus from FX3/CX3
static uint8_t SensorI2cBusTest(void){

	// The sensor ID register can be read here to verify sensor connectivity.
	uint8_t buffer[2];
#if 0
	// Reading sensor ID
	if (SensorRead2B(SENSOR_READ_ADDRESS, 0x00, 0x00, buffer) == CY_U3P_SUCCESS) {
		if ((buffer[0] == 0x01) && (buffer[1] == 0x02)) {
			return CY_U3P_SUCCESS;
		}
	}
#endif

	// Read chip version register
	if (SensorWrite2B(SENSOR_WRITE_ADDRESS, 0x30, 0x00, 0x12, 0x13) == CY_U3P_SUCCESS) {
		CyU3PDebugPrint (4, "SensorI2cBusTest: SensorRead2B %x %x\r\n", buffer[0], buffer[1]);
		if (SensorRead2B(SENSOR_READ_ADDRESS, 0x30, 0x00, buffer) == CY_U3P_SUCCESS) {
			CyU3PDebugPrint (4, "rSensorI2cBusTest: SensorRead2B %x %x\r\n", buffer[0], buffer[1]);
			return CY_U3P_SUCCESS;
		} else {
			CyU3PDebugPrint (4, "SensorI2cBusTest: SensorRead2B fail\r\n");
		}
		return CY_U3P_SUCCESS;
	} else {
		CyU3PDebugPrint (4, "SensorI2cBusTest: SensorWrite2B fail\r\n");
	}

	//    if (SensorRead2B(SENSOR_READ_ADDRESS, 0x30, 0x00, buffer) == CY_U3P_SUCCESS) {
	//        CyU3PDebugPrint (4, "rSensorI2cBusTest: SensorRead2B %x %x\r\n", buffer[0], buffer[1]);
	//        return CY_U3P_SUCCESS;
	//    } else {
	//        CyU3PDebugPrint (4, "SensorI2cBusTest: SensorRead2B fail\r\n");
	//    }

	return 1;
}
#endif
