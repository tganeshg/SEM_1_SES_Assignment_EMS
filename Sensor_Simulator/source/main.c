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

SIM_INSTANCE	simInst;

/****************************************************************
* Private Functions
****************************************************************/

/****************************************************************************************
* @brief        Prints the usage information for the sensor simulator program.
*
* @details      This function prints the usage information for the sensor simulator program,
*               including the available command line options and their descriptions.
*
* @param        None
*
* @return       None
****************************************************************************************/
static void printUsage(void)
{
    fprintf(stdout,"Usage: sensor_simulator [OPTIONS]\n");
    fprintf(stdout,"Options:\n");
    fprintf(stdout,"  -s <sensorID>    Sensor ID (1 for Fan, 2 for AC, 3 for Fridge)\n");
    fprintf(stdout,"  -m <minPower>    Minimum power consumption,Should be positive value\n");
    fprintf(stdout,"  -M <maxPower>    Maximum power consumption,Should be positive value\n");
    fprintf(stdout,"  -p <modbusPort>  Modbus TCP port\n");
    fprintf(stdout,"  -h, --help       Show this help message and exit\n");
}

/*************************************************************************
* @brief        Reads command line arguments for the sensor simulator program.
*
* @details      This function reads and parses the command line arguments provided
*               to the sensor simulator program. It extracts the sensor ID, minimum
*               power, maximum power, and Modbus TCP port from the arguments.
*
* @param[in]    argc        The number of command line arguments.
* @param[in]    argv        The array of command line arguments.
* @param[out]   sensorID    Pointer to the variable where the sensor ID will be stored.
* @param[out]   minPower    Pointer to the variable where the minimum power will be stored.
* @param[out]   maxPower    Pointer to the variable where the maximum power will be stored.
* @param[out]   modbusPort  Pointer to the variable where the Modbus TCP port will be stored.
*
* @return       ERROR_CODE  Returns RET_OK if the arguments are successfully read and valid,
*                           otherwise returns RET_FAILURE.
*************************************************************************/
static ERROR_CODE readArguments(INT32 argc, CHAR *argv[], UINT16 *sensorID, UINT16 *minPower, UINT16 *maxPower, UINT16 *modbusPort)
{
    INT32 opt=0;

	while ((opt = getopt(argc, argv, "s:m:M:p:h")) != RET_FAILURE)
    {
        switch (opt)
        {
            case 's':
                *sensorID = (UINT16)atoi(optarg);
            break;
            case 'm':
                *minPower = (UINT16)atoi(optarg);
            break;
            case 'M':
                *maxPower = (UINT16)atoi(optarg);
            break;
            case 'p':
                *modbusPort = (UINT16)atoi(optarg);
            break;
            case 'h':
                printUsage();
                exit(RET_OK);
			break;
            default:
				printUsage();
                return RET_FAILURE;
			break;
        }
    }

    if ((*sensorID == 0) || (*minPower > *maxPower) || (*modbusPort == 0))
	{
		fprintf(stderr, "Invalid inputs\n");
		printUsage();
        return RET_FAILURE;
	}

    return RET_OK;
}

/*************************************************************************
* @brief        Simulates power consumption within a specified range.
*
* @details      This function generates a random power consumption value within
*               the specified minimum and maximum power range.
*
* @param[in]    minPower    The minimum power consumption value.
* @param[in]    maxPower    The maximum power consumption value.
*
* @return       UINT16      Returns a random power consumption value within the specified range.
*************************************************************************/
static UINT16 simulatePowerConsumption(UINT16 minPower,UINT16 maxPower)
{
    srand((UINT32)time(NULL) ^ getpid());
    return (UINT16)((rand() % (maxPower - minPower + 1)) + minPower);
}

/*************************************************************************
* @brief        Outputs the power consumption for a given sensor.
*
* @details      This function prints the power consumption value for a specified sensor ID.
*
* @param[in]    sensorID    The ID of the sensor.
* @param[in]    power       The power consumption value to be printed.
*
* @return       None
*************************************************************************/
static void outputPowerConsumption(UINT16 sensorID, UINT16 power)
{
    fprintf(stdout,"Sensor ID: %d, Power Consumption: %d watts\n", sensorID, power);
}

/****************************************************************
* Main
****************************************************************/
/****************************************************************
* @brief        Main function for the sensor simulator program.
*
* @details      This function initializes the sensor simulator, reads command line arguments,
*               sets up the Modbus TCP server, and runs the state machine to simulate power
*               consumption and respond to Modbus queries.
*
* @param[in]    argc        The number of command line arguments.
* @param[in]    argv        The array of command line arguments.
* @param[in]    envp        The array of environment variables.
*
* @return       INT32       Returns RET_OK if the program runs successfully,
*                           otherwise returns RET_FAILURE.
****************************************************************/
INT32 main(INT32 argc, CHAR **argv, CHAR **envp)
{
    INT32 rc=0;
    if (readArguments(argc, argv, &simInst.sensorID, &simInst.minPower, &simInst.maxPower, &simInst.modbusPort) != RET_OK)
        simInst.state = STATE_ERROR;

    while (simInst.state != STATE_ERROR)
    {
        switch (simInst.state)
        {
            case STATE_INIT:
			{
				simInst.ctx = modbus_new_tcp("0.0.0.0", simInst.modbusPort);
				if (simInst.ctx == NULL)
				{
					fprintf(stderr, "Unable to allocate libmodbus context\n");
					return RET_FAILURE;
				}

				simInst.mbMapping = modbus_mapping_new(0, 0, MODBUS_REGISTER_COUNT, 0);
				if (simInst.mbMapping == NULL)
				{
					fprintf(stderr, "Failed to allocate the mapping: %s\n", modbus_strerror(errno));
					modbus_close(simInst.ctx);
					modbus_free(simInst.ctx);
					return RET_FAILURE;
				}

				simInst.serverSocket = modbus_tcp_listen(simInst.ctx, 1);
				if (simInst.serverSocket == RET_FAILURE)
				{
					fprintf(stderr, "Unable to listen TCP connection: %s\n", modbus_strerror(errno));
					modbus_mapping_free(simInst.mbMapping);
					modbus_close(simInst.ctx);
					modbus_free(simInst.ctx);
					return RET_FAILURE;
				}
                simInst.state = STATE_SIMULATE_ACCEPT;
			}
            break;
            case STATE_SIMULATE_ACCEPT:
			{
                #if DEBUG_LOG
                fprintf(stdout,"Waiting for server request from Main Process..\n");
                #endif
				modbus_tcp_accept(simInst.ctx, &simInst.serverSocket);
				// Enable debug mode for the Modbus context
                modbus_set_debug(simInst.ctx, TRUE);
                simInst.state = STATE_SIMULATE_POWER;
			}
			break;
            case STATE_SIMULATE_POWER:
			{
                simInst.power = simulatePowerConsumption(simInst.minPower, simInst.maxPower);
                simInst.state = STATE_OUTPUT_POWER;
			}
            break;
            case STATE_OUTPUT_POWER:
			{
				#if DEBUG_LOG
                outputPowerConsumption(simInst.sensorID, simInst.power);
				#endif
                simInst.state = STATE_RESPOND_MODBUS;
			}
            break;
			case STATE_RESPOND_MODBUS:
			{
				rc = modbus_receive(simInst.ctx, (UINT8 *)simInst.mbMapping->tab_registers);
				if (rc > 0)
				{
					simInst.mbMapping->tab_registers[MODBUS_REGISTER_ADDRESS] = (UINT16)simInst.power;
					modbus_reply(simInst.ctx, (UINT8 *)simInst.mbMapping->tab_registers, rc, simInst.mbMapping);
					simInst.state = STATE_SIMULATE_POWER;
				}
				else
				{
					if(errno == ECONNRESET)
					{
						fprintf(stderr, "Socket disconnected: %s\n", modbus_strerror(errno));
						simInst.state = STATE_SIMULATE_ACCEPT;
					}
				}
			}
			break;
            default:
				simInst.state = STATE_ERROR;
			break;
        }
    }

    modbus_mapping_free(simInst.mbMapping);
    modbus_close(simInst.ctx);
    modbus_free(simInst.ctx);

    return RET_OK;
}

/* EOF */
