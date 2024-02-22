/***************************************************************************//**
* \file cy_pdaltmode_host_details.h
* \version 1.0
*
* \brief
* This header file defines the data structures and function prototypes associated
* with the Host Details feature.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_PDALTMODE_HOST_DETAILS_H_
#define _CY_PDALTMODE_HOST_DETAILS_H_

/*****************************************************************************
 * Header file addition
 *****************************************************************************/

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

/*******************************************************************************
 * MACRO definition
 ******************************************************************************/
/**
* \addtogroup group_pdaltmode_intel_macros
* \{
*/
#if STORE_DETAILS_OF_HOST
/** Postponing Ridge update in case tAME timer is still running */
#define DO_NOT_UPDATE_US_IF_T_AME_TIMER_IS_RUNNING             (1u)
#endif /* STORE_DETAILS_OF_HOST */

/** \} group_pdaltmode_intel_macros */

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/

/**
* \addtogroup group_pdaltmode_intel_functions
* \{
*/

/*******************************************************************************
* Function name: Cy_PdAltMode_HostDetails_Task
****************************************************************************//**
*
* Host Details Task
*
* \param ptrAltModeContext
* Pointer to the Alt Mode context
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_HostDetails_Task(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_HostDetails_CheckIfRidgeNeedsToBeUpdated
****************************************************************************//**
*
* Check if Ridge needs to be updated.
*
* \param ptrAltModeContext
* PDAltMode Library Context pointer
*
* \param regConfig
* Ridge Register
*
* \return
* Flag that indicates if Ridge needs update.
*
*******************************************************************************/
bool Cy_PdAltMode_HostDetails_CheckIfRidgeNeedsToBeUpdated(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint32_t regConfig);


/*******************************************************************************
* Function name: Cy_PdAltMode_HostDetails_StatusUpdateAfterHostConnection
****************************************************************************//**
*
* Status update after host connection
*
* \param ptrAltModeContext
* PDAltMode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_HostDetails_StatusUpdateAfterHostConnection(cy_stc_pdaltmode_context_t *ptrAltModeContext);


/*******************************************************************************
* Function name: Cy_PdAltMode_HostDetails_ChangeDsPortBehaviorBasedOnHostCapability
****************************************************************************//**
*
* Change Ds port behavior based on host capability.
* 
* \param ptrAltModeContext
* PdAltMode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_HostDetails_ChangeDsPortBehaviorBasedOnHostCapability(cy_stc_pdaltmode_context_t *ptrAltModeContext);


/*******************************************************************************
* Function name: Cy_PdAltMode_HostDetails_ControlModeBasedOnHostType
****************************************************************************//**
*
* Control mode based on host type.
*
* \param ptrAltModeContext
* PdAltMode Library Context pointer
*
* \param modeMask
* Host Details Control Mode mask
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_HostDetails_ControlModeBasedOnHostType(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t modeMask);


/*******************************************************************************
* Function name: Cy_PdAltMode_HostDetails_Init
****************************************************************************//**
*
* Initiate Host Details structure.
*
* \param hostAltModeContext
* Host PdAltMode Library Context pointer
*
* \param deviceAltModeContext
* Device PdAltMode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_HostDetails_Init(cy_stc_pdaltmode_context_t *hostAltModeContext, cy_stc_pdaltmode_context_t *deviceAltModeContext );

/*******************************************************************************
* Function name: Cy_PdAltMode_HostDetails_SendHardResetCbk
****************************************************************************//**
*
* Timer Callback to send Hard Reset.
*
* \param id
* Timer index
*
* \param ptrContext
* Callback Context pointer
*
* \return
* None.
*
*******************************************************************************/
void Cy_PdAltMode_HostDetails_SendHardResetCbk(cy_timer_id_t id, void * ptrContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_HostDetails_RestartDpmState
****************************************************************************//**
*
* DPM callback to Reset DPM.
*
* \param id
* Timer index
*
* \param ptrContext
* Callback Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_HostDetails_RestartDpmState(cy_timer_id_t id, void * ptrContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_HostDetails_GetStatus
****************************************************************************//**
*
* Returns Host Details status pointer.
*
* \param port
* Selected port
*
* \return
* Returns pointer to host context status structure
*
*******************************************************************************/
cy_stc_pdaltmode_host_details_t* Cy_PdAltMode_HostDetails_GetStatus(uint8_t port);

/*******************************************************************************
* Function name: Cy_PdAltMode_HostDetails_ClearRidgeRdyBitOnDisconnect
****************************************************************************//**
*
* Clears RidgeRdy bit on disconnect.
*
* \param ptrAltModeContext
* Pointer to PD Alternate Mode context
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_HostDetails_ClearRidgeRdyBitOnDisconnect(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function Name: Cy_PdAltMode_HostDetails_IsUsb4Connected
****************************************************************************//**
*
* Returns USB4 connection status.
*
* \param ptrAltModeContext
* Pointer to PD Alternate Mode context
*
* \return
* true if USB4 is connected, false otherwise.
*
*******************************************************************************/
bool Cy_PdAltMode_HostDetails_IsUsb4Connected(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/** \} group_pdaltmode_intel_functions */

/** \} group_pdaltmode_intel */

#endif /* _CY_PDALTMODE_HOST_DETAILS_H_ */

/* [] END OF FILE */
