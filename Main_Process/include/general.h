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
#include "common.h"
#include "ini.h"

/*
*Macros
*/
#define APP_VERSION				"MP 1.0.1 16022025"
#define MODBUS_DEBUG			FALSE
#define DEBUG_LOG				TRUE
#define MODBUS_DEBUG_LOG		FALSE
#define MAX_SENS_SIMULATOR		3
#define CUR_SENS_SIMULATOR		1
#define MIN_MQTT_PUB_INTERVAL	1
#define MAX_MQTT_PUB_INTERVAL	59

#define CONFIG_FILE				"source/config.ini"
#define MQTT_TOPIC				"sensor/data"
#define DB_NAME					"sensor_data.db"

//for Flags use only
extern UINT64 flag1;

#define	POWER_ON				0

#define SET_FLAG(n)				((flag1) |= (UINT64)(1ULL << (n)))
#define CLR_FLAG(n)				((flag1) &= (UINT64)~((1ULL) << (n)))
#define CHECK_FLAG(n)			((flag1) & (UINT64)(1ULL<<(n)))

/*
*Enum
*/
/* Define state machine states */
typedef enum {
    STATE_INIT,
    STATE_CONNECT_MODBUS,
    STATE_READ_MODBUS,
    STATE_INSERT_DB,
    STATE_PUBLISH_MQTT,
    STATE_ERROR
} STATE_TYPE;

/*
*Structure
*/
#pragma pack(push,1)
/* Define structure to hold program arguments */
typedef struct
{
    CHAR		*sensorIP[CUR_SENS_SIMULATOR];
    UINT16		sensorPort[CUR_SENS_SIMULATOR];
    UINT16		readInterval[CUR_SENS_SIMULATOR];
    CHAR		*mqttIP;
    UINT16		mqttPort;
    CHAR		*mqttUsername;
    CHAR		*mqttPassword;
    UINT16		publishInterval;
}PROGRAM_ARGS;

typedef struct
{
    PROGRAM_ARGS		args;
    STATE_TYPE			state;
    modbus_t			*ctx[CUR_SENS_SIMULATOR];
    sqlite3				*db;
    struct mosquitto	*mosq;
    UINT16				power[CUR_SENS_SIMULATOR];
    CHAR				payload[SIZE_2048];
}MP_INST;
#pragma pack(pop)

/*
*Function declarations
*/

#endif

/* EOF */
