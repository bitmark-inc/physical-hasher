// Cypress CX3 Firmware Example Header (sensor_i2c.h)
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

#if !defined(_SENSOR_I2C_H_)
#define _SENSOR_I2C_H_ 1

#include <cyu3types.h>
#include "macros.h"


// dummy register address for delay during array operation
#define SENSOR_DELAY 0xffff

typedef struct sensor_data_struct {
  unsigned short address;
  unsigned short data;
} sensor_data_t;


CyU3PReturnStatus_t sensor_write(uint16_t address, uint16_t data);
CyU3PReturnStatus_t sensor_write_array(const sensor_data_t *data_list, size_t array_size);
#define SENSOR_WRITE_ARRAY(a) sensor_write_array((a), SIZE_OF_ARRAY(a));

CyU3PReturnStatus_t sensor_read(uint16_t address, uint16_t *data);
// CyU3PReturnStatus_t sensor_read_array(const address_data_t *data_list, size_t array_size);

//void sensor_reset(void);

#endif
