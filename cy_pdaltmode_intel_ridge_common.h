/***************************************************************************//**
* \file cy_pdaltmode_intel_ridge_common.h
* \version 1.0
*
* \brief
* This header file defines the data structures and function prototypes associated
* with the Intel Alpine/Titan Ridge control interface.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_PDALTMODE_INTEL_RIDGE_COMMON_H_
#define _CY_PDALTMODE_INTEL_RIDGE_COMMON_H_

/*****************************************************************************
 * Header file addition
 *****************************************************************************/
/**
* \addtogroup group_pdaltmode_intel
* \{
* Intel provides APIs and data structures to support Intel and Intel-related retimers 
* functionality for Host and Dock projects.
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
/** Over-Current status bit in the status register. */
#define CY_PDALTMODE_RIDGE_STATUS_OCP_MASK                    (0x08u)

/** Goshen Ridge MUX Delay Timer Period */
#define CY_PDALTMODE_RIDGE_GR_MUX_VDM_DELAY_TIMER_PERIOD      (10u)

/** \} group_pdaltmode_intel_macros */

/*******************************************************************************
 * Enumerated data definition
 ******************************************************************************/
/**
* \addtogroup group_pdaltmode_intel_enums
* \{
*/

/**
 * @typedef cy_en_pdaltmode_ridge_control_bits
 * @brief Holds all possible Alt Modes
 */
typedef enum
{
    CY_PDALTMODE_RIDGE_NONE_MODE                   = (0u),          /**< None Mode */
    CY_PDALTMODE_RIDGE_TBT_MODE_DFP                = (1u<<0u),      /**< TBT DFP  */
    CY_PDALTMODE_RIDGE_TBT_MODE_UFP                = (1u<<1u),      /**< TBT UFP */
    CY_PDALTMODE_RIDGE_DP_MODE_DFP                 = (1u<<2u),      /**< DP DFP */
    CY_PDALTMODE_RIDGE_DP_MODE_UFP                 = (1u<<3u),      /**< DP UFP */
    CY_PDALTMODE_RIDGE_USB4_MODE_DFP               = (1u<<4u),      /**< USB4 DFP */
    CY_PDALTMODE_RIDGE_USB4_MODE_UFP               = (1u<<5u)       /**< USB4 UFP */
}cy_en_pdaltmode_ridge_control_bits;

/** \} group_pdaltmode_intel_enums */

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/

/**
* \addtogroup group_pdaltmode_intel_functions
* \{
*/

/*******************************************************************************
* Function name: Cy_PdAltMode_Ridge_SetDisconnect
****************************************************************************//**
*
* Set disconnect
*
* \param ptrAltModeContext
* PdAltMode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_Ridge_SetDisconnect(cy_stc_pdaltmode_context_t *ptrAltModeContext);

#if BB_RETIMER_ENABLE
/*******************************************************************************
* Function name: Cy_PdAltMode_Ridge_SlaveSetDebug
****************************************************************************//**
*
* Update Debug mode field in Status register of Ridge slave
*
* \param port
* Port number
*
* \param muxCfg
* Debug mode
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_Ridge_SlaveSetDebug(uint8_t port, uint32_t debug);

#endif /* BB_RETIMER_ENABLE */

/*******************************************************************************
* Function name: Cy_PdAltMode_Ridge_SetMux
****************************************************************************//**
*
* Set AR/TR registers in accordance to input parameters
*
* \param ptrAltModeContext
* PdAltMode Library Context pointer
*
* \param muxCfg
* MUX configuration
*
* \param modeVdo
* MUX configuration related DP/TBT/USB4 VDO
*
* \param customData
* Additional custom data related to MUX configuration
*
* \return
* True if AR/TR was set successful, otherwise false
*
*******************************************************************************/
bool Cy_PdAltMode_Ridge_SetMux(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_mux_select_t muxCfg, uint32_t modeVdo, uint32_t customData);

/*******************************************************************************
* Function name: Cy_PdAltMode_Ridge_Task
****************************************************************************//**
*
* Handles pending Ridge tasks
*
* \param ptrAltModeContext
* PdAltMode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_Ridge_Task(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Ridge_HpdInit
****************************************************************************//**
*
* Enables Titan Ridge the HPD functionality for the specified PD port
*
* \param ptrAltModeContext
* PdAltMode Library Context pointer
*
* \param cbk
* Callback to be used for command completion event
*
* \return
* Returns CY_PDSTACK_STAT_SUCCESS in case of success, error code otherwise
*
*******************************************************************************/
cy_en_pdstack_status_t Cy_PdAltMode_Ridge_HpdInit(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_cb_usbpd_hpd_events_t cbk);

/*******************************************************************************
* Function name: Cy_PdAltMode_Ridge_HpdDeInit
****************************************************************************//**
*
* Disables Titan Ridge the HPD functionality for the specified PD port
*
* \param ptrAltModeContext
* PdAltMode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_Ridge_HpdDeInit(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Ridge_HpdSendEvt
****************************************************************************//**
*
* Send the desired HPD event out through the Titan Ridge HPD GPIO. Only
* the HPD_EVENT_UNPLUG, HPD_EVENT_UNPLUG and HPD_EVENT_IRQ events
* should be requested.
*
* \param ptrAltModeContext
* PdAltMode Library Context pointer
*
* \param evtype
* Type of HPD event to be sent
*
* \return
* Returns CY_PDSTACK_STAT_SUCCESS in case of success, error code otherwise
*
*******************************************************************************/
cy_en_pdstack_status_t Cy_PdAltMode_Ridge_HpdSendEvt(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_usbpd_hpd_events_t evtype);

/*******************************************************************************
* Function name: Cy_PdAltMode_Ridge_GetHpdState
****************************************************************************//**
*
* Returns TR HPD state
*
* \param ptrAltModeContext
* PdAltMode Library Context pointer
*
* \return
* TR HPD state
*
*******************************************************************************/
bool Cy_PdAltMode_Ridge_GetHpdState(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/** \} group_pdaltmode_intel_functions */

/** \} group_pdaltmode_intel */

#endif /* _CY_PDALTMODE_INTEL_RIDGE_COMMON_H_ */

/* [] END OF FILE */
