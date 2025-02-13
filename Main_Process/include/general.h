/**************************************************************************************
*
*	BITS Pilani - Copyright (c) 2025
*	All rights reserved.
*
*	Project 		: Assignment - Energy Monitoring System - Semester 1 - SES
*	Author			: Ganesh
*
*	Revision History
***************************************************************************************
*	Date			Version		Name		Description
***************************************************************************************
*	11/02/2025		1.0			Ganesh		Initial Development
*
**************************************************************************************/

#ifndef _GENERAL_H_
#define _GENERAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <modbus/modbus.h>
#include <sqlite3.h>
#include <mosquitto.h>
#include <ini.h>
#include "common.h"

/*
*Macros
*/
#define APP_VERSION				    "MP 1.0.0 13012018"
#define DEBUG_LOG				    1

//for Flags use only
extern UINT64 flag1;

#define	POWER_ON				0

#define SET_FLAG(n)				((flag1) |= (UINT64)(1ULL << (n)))
#define CLR_FLAG(n)				((flag1) &= (UINT64)~((1ULL) << (n)))
#define CHECK_FLAG(n)			((flag1) & (UINT64)(1ULL<<(n)))

/*
*Structure
*/
#pragma pack(push,1)

/*************************************************************************
* @brief        Structure to hold the simulation instance data.
*
* @details      This structure contains all the necessary data for a simulation instance,
*               including the state of the state machine, Modbus TCP port, sensor ID,
*               power consumption range, power value, server socket, and Modbus context
*               and mapping.
*************************************************************************/
typedef struct
{
    STATE_TYPE          state;          /**< The current state of the state machine */
    UINT16              modbusPort;     /**< The Modbus TCP port */
    UINT16              sensorID;       /**< The ID of the sensor */
    UINT16              minPower;       /**< The minimum power consumption value */
    UINT16              maxPower;       /**< The maximum power consumption value */
    UINT16              power;          /**< The current power consumption value */
    INT32               serverSocket;   /**< The server socket for Modbus TCP */
    modbus_t            *ctx;           /**< The Modbus context */
    modbus_mapping_t    *mbMapping;     /**< The Modbus mapping */
} SIM_INSTANCE;

#pragma pack(pop)

/*
*Function declarations
*/

#endif

/* EOF */
