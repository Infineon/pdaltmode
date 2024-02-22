/***************************************************************************//**
* \file cy_pdaltmode_soc_dock.h
* \version 1.0
*
* \brief
* This file defines SCB operations handlers for Intel SOC.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_PDALTMODE_SOC_DOCK_H_
#define _CY_PDALTMODE_SOC_DOCK_H_

#if (TBT_HOST_APP == 0)
#include "cy_pdaltmode_defines.h"

/**
* \addtogroup group_pdaltmode_intel
* \{
*
********************************************************************************
*
* \defgroup group_pdaltmode_intel_macros Macros
* \defgroup group_pdaltmode_intel_enums Enumerated Types
* \defgroup group_pdaltmode_intel_data_structures Data Structures
* \defgroup group_pdaltmode_intel_functions Functions
*/

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/

/**
* \addtogroup group_pdaltmode_intel_functions
* \{
*/

/*******************************************************************************
 * Function name: Cy_PdAltMode_SocDock_Init
 ****************************************************************************//**
 *
 * Initialize the Alpine/Titan Ridge slave interface module.
 *
 * This function initializes the Alpine/Titan Ridge slave interface module and configures it
 * to use the specified SCB block. The SCB will be configured as an I2C slave block,
 * and the interrupt output will also be initialized to a de-asserted state.
 *
 * Since only two registers are to be implemented, and the commands to be implemented are
 * simple, the complete module is implemented using the I2C command callbacks.
 *
 * \param ridgeHwConfig
 * Ridge HW Config pointer.
 *
 * \param ptrAltModeContext0
 * Pointer to PD Alt mode context for port 0
 *
 * \param ptrAltModeContext1
 * Pointer to PD Alt mode context for port 1
 *
 * \return
 * None
 *
 *******************************************************************************/
void Cy_PdAltMode_SocDock_Init(cy_stc_pdaltmode_ridge_hw_config_t* ridgeHwConfig, cy_stc_pdaltmode_context_t* ptrAltModeContext0, cy_stc_pdaltmode_context_t* ptrAltModeContext1);

/*******************************************************************************
 * Function name: Cy_PdAltMode_SocDock_DeInit
 ****************************************************************************//**
 *
 * De-initialize the Alpine/Titan Ridge slave interface module.
 * This function de-initializes the SCB block used to implement the I2C slave interface
 * between CCGx and Alpine/Titan Ridge.
 *
 * \param scbNum
 * SCB number
 *
 * \param ptrAltModeContext0
 * Pointer to PD Alt Mode context for port 0
 *
 * \param ptrAltModeContext1
 * Pointer to PD Alt Mode context for port 1
 *
 * \return
 * None
 *
 *******************************************************************************/
void Cy_PdAltMode_SocDock_DeInit(CySCB_Type* scbNum, cy_stc_pdaltmode_context_t* ptrAltModeContext0, cy_stc_pdaltmode_context_t* ptrAltModeContext1);

/*******************************************************************************
 * Function name: Cy_PdAltMode_SocDock_Task
 ****************************************************************************//**
 *
 * AltMode Soc state machine tasks handler
 *
 * \param ptrAltModeContext
 * Pointer to PD Alt Mode context
 *
 * \return
 * None
 *
 *******************************************************************************/
void Cy_PdAltMode_SocDock_Task(cy_stc_pdaltmode_context_t* ptrAltModeContext);

#if SYS_DEEPSLEEP_ENABLE
/*******************************************************************************
 * Function name: Cy_PdAltMode_SocDock_Sleep
 ****************************************************************************//**
 *
 * Check whether the AR/TR slave interface is idle, so that device can be placed into sleep.
 * This function should be called prior to placing the CCG device in deep sleep. Deep sleep
 * entry is only allowed if this function returns true.
 *
 * \param ptrAltModeContext
 * Pointer to PD Alt mode context.
 *
 * \return
 * True if the interface is idle, false otherwise.
 *
 *******************************************************************************/
bool Cy_PdAltMode_SocDock_Sleep(cy_stc_pdaltmode_context_t *ptrAltModeContext);

#endif /* SYS_DEEPSLEEP_ENABLE */

/*******************************************************************************
 * Function name: Cy_PdAltMode_SocDock_GetUUID
 ****************************************************************************//**
 *
 * Returns the pointer to the UUID buffer
 *
 * \param port
 * Selected port.
 *
 * \return
 *  Pointer to the UUID buffer
 *
 *******************************************************************************/
uint8_t * Cy_PdAltMode_SocDock_GetUUID(uint8_t port);

#endif /* (TBT_HOST_APP == 0) */

/** \} group_pdaltmode_intel_functions */

/** \} group_pdaltmode_intel */

#endif /* _CY_PDALTMODE_SOC_DOCK_H_ */

/* [] END OF FILE */
