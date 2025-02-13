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

/*** Includes ***/
#include "general.h"

/*** Globals ***/
UINT64 flag1;

/* Define state machine states */
typedef enum {
    STATE_INIT,
    STATE_CONNECT_MODBUS,
    STATE_READ_MODBUS,
    STATE_INSERT_DB,
    STATE_PUBLISH_MQTT,
    STATE_ERROR
} STATE_TYPE;

/* Define structure to hold program arguments */
typedef struct {
    CHAR *sensorIP[3];
    UINT16 sensorPort[3];
    UINT16 readInterval[3];
    CHAR *mqttIP;
    UINT16 mqttPort;
    CHAR *mqttUsername;
    CHAR *mqttPassword;
    UINT16 publishInterval;
} PROGRAM_ARGS;

/* Function prototypes */
static ERROR_CODE readConfig(const CHAR *filename, PROGRAM_ARGS *args);
static ERROR_CODE connectModbus(modbus_t **ctx, const CHAR *ip, UINT16 port);
static ERROR_CODE readModbus(modbus_t *ctx, UINT16 *power);
static ERROR_CODE insertDB(sqlite3 *db, UINT16 sensorID, UINT16 power);
static ERROR_CODE publishMQTT(struct mosquitto *mosq, sqlite3 *db, UINT16 publishInterval);

/****************************************************************
* Main
****************************************************************/
INT32 main(INT32 argc, CHAR **argv, CHAR **envp)
{
    PROGRAM_ARGS args;
    STATE_TYPE state = STATE_INIT;
    modbus_t *ctx[3] = {NULL, NULL, NULL};
    sqlite3 *db = NULL;
    struct mosquitto *mosq = NULL;
    UINT16 power[3] = {0, 0, 0};
    INT32 rc = 0;

    if (readConfig("config.ini", &args) != RET_OK)
    {
        return RET_FAILURE;
    }

    /* Initialize SQLite database */
    rc = sqlite3_open("sensor_data.db", &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return RET_FAILURE;
    }

    /* Create table if not exists */
    const CHAR *sql = "CREATE TABLE IF NOT EXISTS SensorData ("
                      "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "Device_ID INTEGER, "
                      "Timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
                      "Power_Consumption INTEGER);";
    rc = sqlite3_exec(db, sql, 0, 0, 0);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return RET_FAILURE;
    }

    /* Initialize MQTT */
    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq)
    {
        fprintf(stderr, "Failed to create mosquitto instance\n");
        sqlite3_close(db);
        return RET_FAILURE;
    }

    if (args.mqttUsername && args.mqttPassword)
    {
        mosquitto_username_pw_set(mosq, args.mqttUsername, args.mqttPassword);
    }

    while (state != STATE_ERROR)
    {
        switch (state)
        {
            case STATE_INIT:
                state = STATE_CONNECT_MODBUS;
                break;

            case STATE_CONNECT_MODBUS:
                for (int i = 0; i < 3; i++)
                {
                    if (connectModbus(&ctx[i], args.sensorIP[i], args.sensorPort[i]) != RET_OK)
                    {
                        state = STATE_ERROR;
                        break;
                    }
                }
                if (state != STATE_ERROR)
                {
                    state = STATE_READ_MODBUS;
                }
                break;

            case STATE_READ_MODBUS:
                for (int i = 0; i < 3; i++)
                {
                    if (readModbus(ctx[i], &power[i]) != RET_OK)
                    {
                        state = STATE_ERROR;
                        break;
                    }
                }
                if (state != STATE_ERROR)
                {
                    state = STATE_INSERT_DB;
                }
                break;

            case STATE_INSERT_DB:
                for (int i = 0; i < 3; i++)
                {
                    if (insertDB(db, i + 1, power[i]) != RET_OK)
                    {
                        state = STATE_ERROR;
                        break;
                    }
                }
                if (state != STATE_ERROR)
                {
                    state = STATE_PUBLISH_MQTT;
                }
                break;

            case STATE_PUBLISH_MQTT:
                if (publishMQTT(mosq, db, args.publishInterval) != RET_OK)
                {
                    state = STATE_ERROR;
                }
                else
                {
                    for (int i = 0; i < 3; i++)
                    {
                        sleep(args.readInterval[i]);
                    }
                    state = STATE_READ_MODBUS;
                }
                break;

            default:
                state = STATE_ERROR;
                break;
        }
    }

    /* Cleanup */
    for (int i = 0; i < 3; i++)
    {
        if (ctx[i])
        {
            modbus_close(ctx[i]);
            modbus_free(ctx[i]);
        }
    }

    if (db)
    {
        sqlite3_close(db);
    }

    if (mosq)
    {
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
    }

    return RET_OK;
}

/*************************************************************************
* @brief        Reads configuration from a file.
*
* @details      This function reads and parses the configuration file provided
*               to the main process. It extracts the IP address of the sensor simulator,
*               Modbus TCP port, periodic interval to read the data, MQTT broker IP or URL,
*               MQTT port, MQTT username, MQTT password, and MQTT publish periodic interval.
*
* @param[in]    filename    The name of the configuration file.
* @param[out]   args        Pointer to the structure where the arguments will be stored.
*
* @return       ERROR_CODE  Returns RET_OK if the configuration is successfully read and valid,
*                           otherwise returns RET_FAILURE.
*************************************************************************/
static ERROR_CODE readConfig(const CHAR *filename, PROGRAM_ARGS *args)
{
    ini_t *config = ini_load(filename);
    if (config == NULL)
    {
        fprintf(stderr, "Failed to load config file: %s\n", filename);
        return RET_FAILURE;
    }

    args->sensorIP[0] = strdup(ini_get(config, "sensor1", "sensorIP"));
    args->sensorPort[0] = (UINT16)atoi(ini_get(config, "sensor1", "sensorPort"));
    args->readInterval[0] = (UINT16)atoi(ini_get(config, "sensor1", "readInterval"));

    args->sensorIP[1] = strdup(ini_get(config, "sensor2", "sensorIP"));
    args->sensorPort[1] = (UINT16)atoi(ini_get(config, "sensor2", "sensorPort"));
    args->readInterval[1] = (UINT16)atoi(ini_get(config, "sensor2", "readInterval"));

    args->sensorIP[2] = strdup(ini_get(config, "sensor3", "sensorIP"));
    args->sensorPort[2] = (UINT16)atoi(ini_get(config, "sensor3", "sensorPort"));
    args->readInterval[2] = (UINT16)atoi(ini_get(config, "sensor3", "readInterval"));

    args->mqttIP = strdup(ini_get(config, "mqtt", "mqttIP"));
    args->mqttPort = (UINT16)atoi(ini_get(config, "mqtt", "mqttPort"));
    args->mqttUsername = strdup(ini_get(config, "mqtt", "mqttUsername"));
    args->mqttPassword = strdup(ini_get(config, "mqtt", "mqttPassword"));
    args->publishInterval = (UINT16)atoi(ini_get(config, "mqtt", "publishInterval"));

    ini_free(config);

    if (!args->sensorIP[0] || !args->sensorPort[0] || !args->readInterval[0] ||
        !args->sensorIP[1] || !args->sensorPort[1] || !args->readInterval[1] ||
        !args->sensorIP[2] || !args->sensorPort[2] || !args->readInterval[2] ||
        !args->mqttIP || !args->publishInterval)
    {
        fprintf(stderr, "Invalid configuration values\n");
        return RET_FAILURE;
    }

    if (args->publishInterval < 1 || args->publishInterval > 59)
    {
        fprintf(stderr, "Error: MQTT publish interval must be between 1 and 59 seconds.\n");
        return RET_FAILURE;
    }

    return RET_OK;
}

/*************************************************************************
* @brief        Connects to the Modbus TCP server.
*
* @details      This function connects to the Modbus TCP server using the provided
*               IP address and port.
*
* @param[out]   ctx         Pointer to the Modbus context.
* @param[in]    ip          The IP address of the Modbus TCP server.
* @param[in]    port        The port of the Modbus TCP server.
*
* @return       ERROR_CODE  Returns RET_OK if the connection is successful,
*                           otherwise returns RET_FAILURE.
*************************************************************************/
static ERROR_CODE connectModbus(modbus_t **ctx, const CHAR *ip, UINT16 port)
{
    *ctx = modbus_new_tcp(ip, port);
    if (*ctx == NULL)
    {
        fprintf(stderr, "Unable to allocate libmodbus context\n");
        return RET_FAILURE;
    }

    if (modbus_connect(*ctx) == -1)
    {
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        modbus_free(*ctx);
        return RET_FAILURE;
    }

    return RET_OK;
}

/*************************************************************************
* @brief        Reads data from the Modbus TCP server.
*
* @details      This function reads the power consumption data from the Modbus TCP server.
*
* @param[in]    ctx         The Modbus context.
* @param[out]   power       Pointer to the variable where the power consumption data will be stored.
*
* @return       ERROR_CODE  Returns RET_OK if the data is successfully read,
*                           otherwise returns RET_FAILURE.
*************************************************************************/
static ERROR_CODE readModbus(modbus_t *ctx, UINT16 *power)
{
    UINT16 tab_reg[1];

    if (modbus_read_registers(ctx, 0, 1, tab_reg) == -1)
    {
        fprintf(stderr, "Failed to read registers: %s\n", modbus_strerror(errno));
        return RET_FAILURE;
    }

    *power = tab_reg[0];
    return RET_OK;
}

/*************************************************************************
* @brief        Inserts data into the SQLite database.
*
* @details      This function inserts the power consumption data into the SQLite database.
*
* @param[in]    db          The SQLite database connection.
* @param[in]    sensorID    The ID of the sensor.
* @param[in]    power       The power consumption data.
*
* @return       ERROR_CODE  Returns RET_OK if the data is successfully inserted,
*                           otherwise returns RET_FAILURE.
*************************************************************************/
static ERROR_CODE insertDB(sqlite3 *db, UINT16 sensorID, UINT16 power)
{
    CHAR sql[256];
    snprintf(sql, sizeof(sql), "INSERT INTO SensorData (Device_ID, Power_Consumption) VALUES (%d, %d);", sensorID, power);

    if (sqlite3_exec(db, sql, 0, 0, 0) != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        return RET_FAILURE;
    }

    /* Delete old data beyond 24 hours */
    snprintf(sql, sizeof(sql), "DELETE FROM SensorData WHERE Timestamp < datetime('now', '-1 day');");
    if (sqlite3_exec(db, sql, 0, 0, 0) != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        return RET_FAILURE;
    }

    return RET_OK;
}

/*************************************************************************
* @brief        Publishes data to the MQTT broker.
*
* @details      This function publishes the power consumption data to the MQTT broker
*               in JSON format.
*
* @param[in]    mosq        The Mosquitto instance.
* @param[in]    db          The SQLite database connection.
* @param[in]    publishInterval The interval in seconds to publish the data.
*
* @return       ERROR_CODE  Returns RET_OK if the data is successfully published,
*                           otherwise returns RET_FAILURE.
*************************************************************************/
static ERROR_CODE publishMQTT(struct mosquitto *mosq, sqlite3 *db, UINT16 publishInterval)
{
    CHAR sql[256];
    sqlite3_stmt *stmt;
    CHAR payload[2048];
    CHAR temp[256];
    INT32 rc;

    snprintf(sql, sizeof(sql), "SELECT Device_ID, Power_Consumption, Timestamp FROM SensorData WHERE Timestamp >= datetime('now', '-%d seconds');", publishInterval);

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return RET_FAILURE;
    }

    strcpy(payload, "[");
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        snprintf(temp, sizeof(temp), "{\"sensorID\": %d, \"power\": %d, \"Timestamp\": \"%s\"},",
                 sqlite3_column_int(stmt, 0),
                 sqlite3_column_int(stmt, 1),
                 sqlite3_column_text(stmt, 2));
        strcat(payload, temp);
    }
    sqlite3_finalize(stmt);

    /* Remove the trailing comma and close the JSON array */
    if (payload[strlen(payload) - 1] == ',')
    {
        payload[strlen(payload) - 1] = '\0';
    }
    strcat(payload, "]");

    if (mosquitto_publish(mosq, NULL, "sensor/data", strlen(payload), payload, 0, false) != MOSQ_ERR_SUCCESS)
    {
        fprintf(stderr, "Failed to publish message\n");
        return RET_FAILURE;
    }

    return RET_OK;
}

/* EOF */
