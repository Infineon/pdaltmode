/***************************************************************************//**
* \file cy_pdaltmode_hw.h
* \version 1.0
*
* \brief
* This header file defines the data structures and function prototypes associated
* with the Alternate Mode Hardware Control implementation.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_PDALTMODE_HW_H_
#define _CY_PDALTMODE_HW_H_

/*****************************************************************************
 * Header file addition
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>

#include "cy_pdaltmode_defines.h"

/**
* \addtogroup group_pdaltmode_alt_mode_hw
* \{
* Alternate Mode Hardware driver provides APIs and data structures to control/configure Alternate Modes related
* HW like external MUXes (for datapath update), HPD (Hot Plug Detect for DisplayPort), internal MUXes, and so on.
*
********************************************************************************
*
* \defgroup group_pdaltmode_alt_mode_hw_enums Enumerated types
* \defgroup group_pdaltmode_alt_mode_hw_data_structures Data structures
* \defgroup group_pdaltmode_alt_mode_hw_functions Functions
*/

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/

/**
* \addtogroup group_pdaltmode_alt_mode_hw_functions
* \{
*/
/*******************************************************************************
* Function name: Cy_PdAltMode_HW_SetCbk
****************************************************************************//**
*
* Registers an ALT. MODE hardware callback function.
*
* \param ptrAltModeContext
* Pointer to the Alt Mode context
*
* \param cbk
* Callback function for ALT. MODE hardware events
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_HW_SetCbk(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_pdaltmode_hw_cmd_cbk_t cbk);


/*******************************************************************************
* Function name: Cy_PdAltMode_HW_DeInit
****************************************************************************//**
*
* De-init all HW related to Alt Modes.
*
* \param ptrAltModeContext
* Pointer to the Alt Mode context
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_HW_DeInit(cy_stc_pdaltmode_context_t *ptrAltModeContext);


/*******************************************************************************
* Function name: Cy_PdAltMode_HW_EvalAppAltHwCmd
****************************************************************************//**
*
* Evaluates received HPD application command.
*
* \param ptrAltModeContext
* Pointer to Alt Mode context
*
* \param cmdParam
* Pointer to received application HW command data
*
* \return
* True if APP command passed is successful, false if APP command is invalid
* or contains unacceptable fields.
*
*******************************************************************************/
bool Cy_PdAltMode_HW_EvalAppAltHwCmd(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t *cmdParam);


/*******************************************************************************
* Function name: Cy_PdAltMode_HW_EvalHpdCmd
****************************************************************************//**
*
* Evaluates received HPD application command.
*
* \param ptrAltModeContext
* Pointer to Alt Mode context
*
* \param cmd
* Received HPD application command data
*
* \return
* True if HPD APP command passed is successful, false if HPD APP command
* is invalid or contains unacceptable fields.
*
*******************************************************************************/
bool Cy_PdAltMode_HW_EvalHpdCmd(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint32_t cmd);


/*******************************************************************************
* Function name: Cy_PdAltMode_HW_EvalMuxCmd
****************************************************************************//**
*
* Evaluates received MUX application command.
*
* \param ptrAltModeContext
* Pointer to Alt Mode context
*
* \param cmd
* Received MUX application command data
*
* \return
* True if MUX APP command passed is successful, false if MUX APP command
* is invalid or contains unacceptable fields.
*
*******************************************************************************/
bool Cy_PdAltMode_HW_EvalMuxCmd(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint32_t cmd);


/*******************************************************************************
* Function name: Cy_PdAltMode_HW_SetMux
****************************************************************************//**
*
* Sets appropriate MUX configuration based on the function parameters.
*
* \param ptrAltModeContext
* Pointer to the Alt Mode context
*
* \param muxCfg
* Required MUX configuration
*
* \param modeVdo
* Additional VDO data related to DP/TBT/USB4 configuration
*
* \param customData
* Additional data in case of custom MUX configuration
*
* \return
* True if MUX set passed is successful, false if MUX setting is invalid or
* contains unacceptable fields.
*
*******************************************************************************/
bool Cy_PdAltMode_HW_SetMux(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_mux_select_t muxCfg, uint32_t modeVdo, uint32_t customData);


#if ICL_ENABLE
/*******************************************************************************
* Function name: Cy_PdAltMode_HW_IgnoreMuxChanges
****************************************************************************//**
*
* Ignores changes to MUX settings.
*
* \param ptrAltModeContext
* PDStack Library Context pointer
*
* \param ignore
* Ignore changes to any MUX state on this
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_HW_IgnoreMuxChanges(cy_stc_pdaltmode_context_t *ptrAltModeContext, bool ignore);
#endif /* ICL_ENABLE */


/*******************************************************************************
* Function name: Cy_PdAltMode_HW_GetMuxState
****************************************************************************//**
*
* Gets the current state of the MUX.
*
* \param ptrAltModeContext
* Pointer to the Alt Mode context
*
* \return
* Active MUX setting
*
*******************************************************************************/
cy_en_pdaltmode_mux_select_t Cy_PdAltMode_HW_GetMuxState(cy_stc_pdaltmode_context_t *ptrAltModeContext);


/*******************************************************************************
* Function name: Cy_PdAltMode_HW_IsIdle
****************************************************************************//**
*
* Checks whether the ALT. MODE hardware block is idle.
* This function is part of the deep sleep entry checks for the CCG device, and checks whether
* there are any pending ALT. MODE hardware commands that require the device to be active.
*
* \param ptrAltModeContext
* Pointer to the Alt Mode context
*
* \return
* True if the block is idle, false otherwise.
*
*******************************************************************************/
bool Cy_PdAltMode_HW_IsIdle(cy_stc_pdaltmode_context_t *ptrAltModeContext);


/*******************************************************************************
* Function Name: Cy_PdAltMode_HW_Sleep
****************************************************************************//**
*
* Prepare the Alt Mode hardware state for device deep sleep.
*
* \param ptrAltModeContext
* PdAltMode Library context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_HW_Sleep(cy_stc_pdaltmode_context_t *ptrAltModeContext);


/*******************************************************************************
* Function name: Cy_PdAltMode_HW_Wakeup
****************************************************************************//**
*
* Restores ALT. MODE hardware state after device resumes from deep sleep.
*
* \param ptrAltModeContext
* PDAltMode Library context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_HW_Wakeup(cy_stc_pdaltmode_context_t *ptrAltModeContext);


/*******************************************************************************
* Function name: Cy_PdAltMode_HW_DpSnkGetHpdState
****************************************************************************//**
*
* Returns HPD state based on HPD Queue events.
*
* \param ptrAltModeContext
* PDAltMode Library context pointer
*
* \return
* True if HPD is connected, false otherwise.
*
*******************************************************************************/
bool Cy_PdAltMode_HW_DpSnkGetHpdState(cy_stc_pdaltmode_context_t *ptrAltModeContext);


/*******************************************************************************
* Function name: Cy_PdAltMode_HW_MuxCtrlInit
****************************************************************************//**
*
* Initialize the Type-C Data MUX for a specific PD port.
*
* \param ptrAltModeContext
* Pointer to the Alt Mode context
*
* \return
* True if the MUX is initialized successfully, false otherwise.
*
*******************************************************************************/
bool Cy_PdAltMode_HW_MuxCtrlInit(cy_stc_pdaltmode_context_t *ptrAltModeContext);


/*******************************************************************************
* Function name: Cy_PdAltMode_HW_MuxCtrlSetCfg
****************************************************************************//**
*
* Sets the Type-C MUX to the desired configuration.
*
* \param ptrAltModeContext
* Pointer to the Alt Mode context
*
* \param cfg
* Desired MUX configuration
*
* \return
* Returns true if the operation is successful, false otherwise.
*
*******************************************************************************/
bool Cy_PdAltMode_HW_MuxCtrlSetCfg(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_mux_select_t cfg);


/*******************************************************************************
* Function name: Cy_PdAltMode_HW_IsHostHpdVirtual
****************************************************************************//**
*
* Checks whether the solution is configured to support virtual HPD.
*
* \param ptrAltModeContext
* PDAltMode Library context pointer
*
* \return
* Returns true if virtual HPD is enabled, false otherwise.
*
*******************************************************************************/
bool Cy_PdAltMode_HW_IsHostHpdVirtual(cy_stc_pdaltmode_context_t * ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_HW_CustomMuxSet
****************************************************************************//**
*
* Custom MUX setting function which can be used to call custom MUX setting from solution layer.
*
* \param ptrAltModeContext
* Pointer to the Alt Mode context
*
* \param muxCfg
* Required MUX configuration
*
* \param modeVdo
* Additional VDO data related to DP/TBT/USB4 configuration
*
* \param customData
* Additional data in case of custom MUX configuration
*
* \return
* True if MUX set passed is successful, false if MUX setting is invalid or
* contains unacceptable fields.
*
*******************************************************************************/
__attribute__ ((weak)) bool Cy_PdAltMode_HW_CustomMuxSet (cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_mux_select_t muxCfg, uint32_t modeVdo, uint32_t customData);

/*******************************************************************************
* Function name: Cy_PdAltMode_HW_GetStatus
****************************************************************************//**
*
* Returns Alt Mode hardware status pointer
*
* \param port
* Selected port
*
* \return
* Returns pointer to Alt Mode hardware status structure
*
*******************************************************************************/
cy_stc_pdaltmode_hw_details_t* Cy_PdAltMode_HW_GetStatus(uint8_t port);

/** \} group_pdaltmode_alt_mode_hw_functions */

/** \} group_pdaltmode_alt_mode_hw */

#endif /* _CY_PDALTMODE_HW_H_ */

/* [] END OF FILE */
