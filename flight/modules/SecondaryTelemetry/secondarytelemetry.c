/**
 ******************************************************************************
 * @addtogroup OpenPilotModules OpenPilot Modules
 * @{
 * @addtogroup SecondaryTelemetryModule SecondaryTelemetry Module
 * @brief Secondary telemetry module
 * Exports basic telemetry data using a selectable serial protocol
 * to external devices such as on screen displays.
 * @{
 *
 * @file       secondarytelemetry.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2015.
 * @brief      Secondary telemetry module, exports basic telemetry.
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <openpilot.h>
#include <pios_com.h>

#include "secondarytelemetrysettings.h"
#include "hwsettings.h"

#include "secondarytelemetry.h"

extern protocolHandler_t uavtalkProtocolHandler;

// Private constants
#define STACK_SIZE_BYTES PIOS_TELEM_STACK_SIZE

#define TASK_PRIORITY    (tskIDLE_PRIORITY + 2)

// Private types

// Private variables
static uint32_t comPort;
static xTaskHandle taskHandle;
static bool modEnabled;
static protocolHandler_t *activeProtocolHandler = NULL;
static uint8_t updatePeriod;
static uint8_t intervalCounts[SECONDARYTELEMETRYSETTINGS_UPDATEINTERVALS_NUMELEM];
static uint8_t updateIntervals[SECONDARYTELEMETRYSETTINGS_UPDATEINTERVALS_NUMELEM];

// Private functions
static void telemetryTask(void *parameters);
static void updateSettings(UAVObjEvent *ev);

/**
 * Initialise the telemetry module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t SecondaryTelemetryStart(void)
{
    // Start telemetry tasks
    if (modEnabled && comPort && activeProtocolHandler) {
        xTaskCreate(telemetryTask, "SecondTel", STACK_SIZE_BYTES / 4, NULL, TASK_PRIORITY, &taskHandle);
    }
    return 0;
}

/**
 * Initialise the telemetry module
 * \return -1 if initialisation failed
 * \return 0 on success
 */
int32_t SecondaryTelemetryInitialize(void)
{
    comPort = PIOS_COM_SECOND_TELEM;

    HwSettingsInitialize();
    HwSettingsOptionalModulesData optionalModules;

    HwSettingsOptionalModulesGet(&optionalModules);

    if (optionalModules.SecondaryTelemetry == HWSETTINGS_OPTIONALMODULES_ENABLED) {
        modEnabled = true;
    } else {
        modEnabled = false;
    }

    if (modEnabled && comPort) {
        SecondaryTelemetrySettingsInitialize();
        SecondaryTelemetrySettingsConnectCallback(updateSettings);
        updateSettings(0);
        if (activeProtocolHandler) {
            activeProtocolHandler->initialize(comPort);
        }
    }

    return 0;
}

MODULE_INITCALL(SecondaryTelemetryInitialize, SecondaryTelemetryStart);

/**
 */
static void telemetryTask(__attribute__((unused)) void *parameters)
{
    uint8_t i;
    portTickType lastSysTime = xTaskGetTickCount();

    // Loop forever
    while (1) {
        vTaskDelayUntil(&lastSysTime, updatePeriod / portTICK_RATE_MS);

        /* Update interval counters */
        for (i = 0; i < SECONDARYTELEMETRYSETTINGS_UPDATEINTERVALS_NUMELEM; i++) {
            if (updateIntervals[i]) {
                intervalCounts[i]++;
                if (intervalCounts[i] >= updateIntervals[i]) {
                    intervalCounts[i] = 0;
                    activeProtocolHandler->updateData(i);
                }
            }
        }
    }
}

/**
 * Update the telemetry settings, called on startup.
 */
static void updateSettings(__attribute__((unused)) UAVObjEvent *ev)
{
    if (modEnabled && comPort) {
        uint8_t speed;
        // Retrieve settings
        SecondaryTelemetrySettingsOutputSpeedGet(&speed);
        SecondaryTelemetrySettingsUpdatePeriodGet(&updatePeriod);
        SecondaryTelemetrySettingsUpdateIntervalsArrayGet(updateIntervals);

        // Set port speed
        switch (speed) {
        case SECONDARYTELEMETRYSETTINGS_OUTPUTSPEED_2400:
            PIOS_COM_ChangeBaud(comPort, 2400);
            break;
        case SECONDARYTELEMETRYSETTINGS_OUTPUTSPEED_4800:
            PIOS_COM_ChangeBaud(comPort, 4800);
            break;
        case SECONDARYTELEMETRYSETTINGS_OUTPUTSPEED_9600:
            PIOS_COM_ChangeBaud(comPort, 9600);
            break;
        case SECONDARYTELEMETRYSETTINGS_OUTPUTSPEED_19200:
            PIOS_COM_ChangeBaud(comPort, 19200);
            break;
        case SECONDARYTELEMETRYSETTINGS_OUTPUTSPEED_38400:
            PIOS_COM_ChangeBaud(comPort, 38400);
            break;
        case SECONDARYTELEMETRYSETTINGS_OUTPUTSPEED_57600:
            PIOS_COM_ChangeBaud(comPort, 57600);
            break;
        case SECONDARYTELEMETRYSETTINGS_OUTPUTSPEED_115200:
            PIOS_COM_ChangeBaud(comPort, 115200);
            break;
        }

        // TODO: Select between protocols
        activeProtocolHandler = &uavtalkProtocolHandler;
    }
}

/**
 * @}
 * @}
 */
