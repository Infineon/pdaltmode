/***************************************************************************//**
* \file cy_pdaltmode_usb4.h
* \version 1.0
*
* \brief
* This header file defines the data structures and function prototypes associated
* with the USB4 mode handler.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_PDALTMODE_USB4_H_
#define _CY_PDALTMODE_USB4_H_

/*****************************************************************************
 * Header file addition
 *****************************************************************************/

#include <stdint.h>

#include "cy_pdaltmode_defines.h"

/**
* \addtogroup group_pdaltmode_usb4
* \{
* Provides APIs and data structures to support USB4 functionality. 
*
********************************************************************************
*
* \defgroup group_pdaltmode_usb4_enums Enumerated Types
* \defgroup group_pdaltmode_usb4_functions Functions
*/

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/

/**
* \addtogroup group_pdaltmode_usb4_functions
* \{
*/

/*******************************************************************************
* Function name: Cy_PdAltMode_Usb4_Enter
****************************************************************************//**
*
* Initiate USB4 enter mode command to port partner or cable.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param sopType
* SOP packet type
*
* \param retry
* Indicates that the request is being retried
*
* \return
* New VDM manager task
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Usb4_Enter(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pd_sop_t sopType, bool retry);


/*******************************************************************************
* Function name: Cy_PdAltMode_Usb4_UpdateDataStatus
****************************************************************************//**
*
* Get USB4 enter mode command parameters.
*
* \param ptrAltModeContext
* PdAltMode Library Context pointer
*
* \param eudo
* Enter USB Data Object used to enter USB4
*
* \param val
* Original value of the MUX data status register
*
* \return
* Updated value of the MUX data status register
*
*******************************************************************************/
uint32_t Cy_PdAltMode_Usb4_UpdateDataStatus (cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_pd_pd_do_t eudo, uint32_t val);

/*******************************************************************************
* Function name: Cy_PdAltMode_Usb4_DataRstRetryCbk
****************************************************************************//**
*
* Data Reset Retry callback
*
* \param id
* Timer index
*
* \param context
* Callback Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_Usb4_DataRstRetryCbk (cy_timer_id_t id, void *context);

/*******************************************************************************
* Function name: Cy_PdAltMode_Usb4_TbtHandler
****************************************************************************//**
*
* USB4 TBT handler
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param vdmEvt
* VDM Task Manager event
*
* \return
* Return VDM Task
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Usb4_TbtHandler(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_vdm_evt_t vdmEvt);

/*******************************************************************************
* Function name: Cy_PdAltMode_Usb4_GetEudo
****************************************************************************//**
*
* Returns EUDO buffer pointer
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* EUDO buffer pointer
*
*******************************************************************************/
cy_pd_pd_do_t* Cy_PdAltMode_Usb4_GetEudo(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Usb4_EvalCblResp
****************************************************************************//**
*
* Evaluates accepted cable Enter USB command response.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* Next VDM manager task to perform.
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Usb4_EvalCblResp(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Usb4_FailCblResp
****************************************************************************//**
*
* Evaluates failed cable Enter USB command response.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* Next VDM manager task to perform
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Usb4_FailCblResp(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Usb4_DataRstRecCbk
****************************************************************************//**
*
* Callback to handle Data Reset command response.
*
* \param ptrPdStackContext
* PD Stack Library Context pointer
*
* \param status
* Data Reset response status
*
* \param recVdm
* Data Reset response pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_Usb4_DataRstRecCbk(cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_resp_status_t status, const cy_stc_pdstack_pd_packet_t* recVdm);

/*******************************************************************************
* Function name: Cy_PdAltMode_Usb4_EvalAcceptResp
****************************************************************************//**
*
* Evaluates accepted UFP Enter USB command response.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* Next VDM manager task to perform
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Usb4_EvalAcceptResp(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Usb4_EvalFailResp
****************************************************************************//**
*
* Evaluates failed UFP Enter USB command response
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* Next VDM manager task to perform
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Usb4_EvalFailResp(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Usb4_EnterUsb4RetryCbk
****************************************************************************//**
*
* Callback to process Enter USB command retry attempt.
*
* \param id Timer ID used to postpone received Attention VDM
* \param context Timer callback context
*
* \return None
*
*******************************************************************************/
void Cy_PdAltMode_Usb4_EnterUsb4RetryCbk (cy_timer_id_t id, void *context);

/*******************************************************************************
* Function name: Cy_PdAltMode_Usb4_EnterRecCbk
****************************************************************************//**
*
* Callback to evaluate Enter_USB command response.
*
* \param ptrPdStackContext
* PD Stack Library Context pointer
*
* \param status
* Data Reset response status
*
* \param recVdm
* Data Reset response pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_Usb4_EnterRecCbk(cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_resp_status_t status, const cy_stc_pdstack_pd_packet_t* recVdm);

/** \} group_pdaltmode_usb4_functions */
/** \} group_pdaltmode_usb4 */

#endif /* _CY_PDALTMODE_USB4_H_ */
