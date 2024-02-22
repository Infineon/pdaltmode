/***************************************************************************//**
* \file cy_pdaltmode_vdm_task.h
* \version 1.0
*
* \brief
* This header file defines the data structures and function prototypes associated
* with the VDM Task Manager
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_PDALTMODE_VDM_TASK_H_
#define _CY_PDALTMODE_VDM_TASK_H_

/*******************************************************************************
 * Header file addition
 ******************************************************************************/

#include <stdint.h>

#include "cy_pdaltmode_defines.h"

/**
* \addtogroup group_pdaltmode_vdm_alt_mode_manager
* \{
*
********************************************************************************
*
* \defgroup group_pdaltmode_vdm_alt_mode_manager_macros Macros
* \defgroup group_pdaltmode_vdm_alt_mode_manager_enums Enumerated Types
* \defgroup group_pdaltmode_vdm_alt_mode_manager_functions Functions
*/

/*******************************************************************************
 * MACRO definition
 ******************************************************************************/

/**
* \addtogroup group_pdaltmode_vdm_alt_mode_manager_macros
* \{
*/
 
/** Start index of received VDM packet in case of response to DISC SVID command */
#define CY_PDALTMODE_VDMTASK_PD_SVID_ID_HDR_VDO_START_IDX        (4u)

/** Index of AMA VDO in DISCOVER_ID response from port partner (after skipping the VDM header) */
#define CY_PDALTMODE_VDMTASK_PD_DISC_ID_AMA_VDO_IDX              (3u)

/** Maximum number of cable SVIDs VDM task manager can hold in the memory */
#define CY_PDALTMODE_VDMTASK_MAX_CABLE_SVID_SUPP                 (4u)

/** Apple SVID defined by Apple specification */
#define CY_PDALTMODE_VDMTASK_APPLE_SVID                          (0x05ACu)

/** Maximum number of DISCOVER_SVID attempts that will be performed */
#define CY_PDALTMODE_VDMTASK_MAX_DISC_SVID_COUNT                 (10u)

/** Maximum number of Data Reset command retries */
#define CY_PDALTMODE_VDMTASK_DATA_RST_RETRY_NUMB                 (5u)

/** Bit shift to match host_supp configuration parameter with USB4 EUDO */
#define CY_PDALTMODE_VDMTASK_USB4_EUDO_HOST_PARAM_SHIFT          (13u)

/** Maximum number of EMCA SOFT_RESET attempts to be made */
#define CY_PDALTMODE_VDMTASK_MAX_EMCA_DP_RESET_COUNT             (3u)

/** \} group_pdaltmode_vdm_alt_mode_manager_macros */

/**************************************************************************************************
 ************************************ Function definitions ****************************************
 *************************************************************************************************/

/**
* \addtogroup group_pdaltmode_vdm_alt_mode_manager_functions
* \{
*/

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_Enable
****************************************************************************//**
*
* Enables VDM task manager functionality
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_VdmTask_Enable(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_Manager
****************************************************************************//**
*
* Main VDM task manager function
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_VdmTask_Manager(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_MngrDeInit
****************************************************************************//**
*
* De-init VDM task manager and resets all related variables
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_VdmTask_MngrDeInit(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_MngrExitModes
****************************************************************************//**
*
* Get the Alternate Mode state machine to exit any active modes. This is called in response to
* a VConn related fault condition.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_VdmTask_MngrExitModes(cy_stc_pdaltmode_context_t *ptrAltModeContext);


/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_IsIdle
****************************************************************************//**
*
* Check whether the VDM task for the port is idle
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* True is VDM Task Idle
*
*******************************************************************************/
bool Cy_PdAltMode_VdmTask_IsIdle(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_IsUfpDiscStarted
****************************************************************************//**
*
* Starts Discover process when CCG is UFP due to PD 3.0 spec.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* True if Discover process has started, false if VDM manager is busy.
*
*******************************************************************************/
bool Cy_PdAltMode_VdmTask_IsUfpDiscStarted(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_GetDiscIdResp
****************************************************************************//**
*
* Obtain the last DISC_ID response received by the CCG device from a port partner.
*
* \param ptrAltModeContext
* Pointer to the Alt Mode Context
*
* \param respLenP
* Length of response in PD data objects
*
* \return
* Pointer to the actual response data
*
*******************************************************************************/
uint8_t *Cy_PdAltMode_VdmTask_GetDiscIdResp(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t *respLenP);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_GetDiscSvidResp
****************************************************************************//**
*
* Obtain the last DISC_SVID response received by the CCG device from a port partner.
*
* \param ptrAltModeContext
* Pointer to the Alt Mode Context
*
* \param respLenP
* Length of response in PD data objects
*
* \return
* Pointer to the actual response data
*
*******************************************************************************/
uint8_t *Cy_PdAltMode_VdmTask_GetDiscSvidResp(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t *respLenP);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_SetDiscParam
****************************************************************************//**
*
* Prepare VDM Discovery message VDO
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param sop
* SOP/SOP_PRIME parameter to define UFP/Cable will receive Discovery VDM
*
* \param cmd
* Disc ID/ Disc SVID command
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_VdmTask_SetDiscParam(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t sop, cy_en_pdstack_std_vdm_cmd_t cmd);

#if CY_PD_USB4_SUPPORT_ENABLE
/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_ResumeHandler
****************************************************************************//**
*
* Restart VDM manager handling Discovery process starting from Disc SVID.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_ResumeHandler(cy_stc_pdaltmode_context_t *ptrAltModeContext);
#endif /* CY_PD_USB4_SUPPORT_ENABLE */
/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_InitiateVcsCblDiscovery
****************************************************************************//**
*
* Initiate VConn Swap and cable discovery before going ahead with the next SOP'/SOP'' message.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* True - VDM sequence needs to be delayed because there is need to do a VConn Swap.
*
*******************************************************************************/
bool Cy_PdAltMode_VdmTask_InitiateVcsCblDiscovery(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_UpdateVcsStatus
****************************************************************************//**
*
* Resume Alternate Mode discovery state machine after cable discovery tasks are complete.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_VdmTask_UpdateVcsStatus(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_ClearDiscResp
****************************************************************************//**
*
* Clear D_ID and D_SVID response buffer
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_VdmTask_ClearDiscResp(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_ResetDiscInfo
****************************************************************************//**
*
* Clear DP and TBT discovery info
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_VdmTask_ResetDiscInfo(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_GetMngrStatus
****************************************************************************//**
*
* Return VDM manager status pointer
*
* \param port
* Selected port
*
* \return
* Returns pointer to VDM manager status structure
*
*******************************************************************************/
cy_stc_pdaltmode_vdm_task_status_t* Cy_PdAltMode_VdmTask_GetMngrStatus(uint8_t port);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_IsVconnPresent
****************************************************************************//**
*
* Check status of VConn
*
* \param ptrPdStackContext
* Pointer to the PD context structure
*
* \param channel
* Channel index, where CC1 = 0, CC2 = 1
*
* \return bool
* Returns true if Vconn is turned on, false otherwise.
*
*******************************************************************************/
bool Cy_PdAltMode_VdmTask_IsVconnPresent(cy_stc_pdstack_context_t *ptrPdStackContext, uint8_t channel);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_Init
****************************************************************************//**
*
* Initialize VDM task manager handling
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* Next VDM manager task
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_Init(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_RetryCheck
****************************************************************************//**
*
* Check if failed VDM has to be re-sent
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param failStat
* Failure root cause status
*
* \return
* True if VDM re-send is needed, otherwise returns false.
*
*******************************************************************************/
bool Cy_PdAltMode_VdmTask_RetryCheck(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_fail_status_t failStat);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_SetVdmFailed
****************************************************************************//**
*
* Set received VDM response command as failed.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param failStat
* Failure root cause status
*
* \return
* True if VDM re-send is needed, otherwise returns false.
*
*******************************************************************************/
void Cy_PdAltMode_VdmTask_SetVdmFailed(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_fail_status_t failStat);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_IsUSB4Supp
****************************************************************************//**
*
* Check if USB4 functionality is supported in DiscID VDO response for 
* CCG and connected device.
*
* \param vdo_do_p
* DiscID VDO pointer
*
* \param len
* Number of VDOs in the DiscID response
*
* \return
* True if USB4 supported, otherwise returns false.
*
*******************************************************************************/
bool Cy_PdAltMode_VdmTask_IsUSB4Supp(cy_pd_pd_do_t* vdo_do_p, uint8_t len);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_GetEudoCblSpeed
****************************************************************************//**
*
* Return cable speed used for EUDO
*
* \param ptrPdStackContext
* PD Stack Library Context pointer
*
* \return
* Proper cable speed
*
*******************************************************************************/
cy_en_pdstack_usb_sig_supp_t Cy_PdAltMode_VdmTask_GetEudoCblSpeed(cy_stc_pdstack_context_t *ptrPdStackContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_SetEudoParams
****************************************************************************//**
*
* Fill EUDO buffer with corresponding parameters
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* Next VDM task
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_SetEudoParams(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_IsUsb4Cap
****************************************************************************//**
*
* Check if USB4 discovery process can be run
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* True if USB4 discovery process can be run, otherwise return false.
*
*******************************************************************************/
bool Cy_PdAltMode_VdmTask_IsUsb4Cap(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_HostCapCheck
****************************************************************************//**
*
* Check if Host supports USB4 functionality. Applicable for Dock projects.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_VdmTask_HostCapCheck(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_GotoDiscSvid
****************************************************************************//**
*
* Initiate Discovery SVID process
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* True if Discovery SVID process can be initiated, otherwise returns false.
*
*******************************************************************************/
bool Cy_PdAltMode_VdmTask_GotoDiscSvid(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_MngDiscId
****************************************************************************//**
*
* Discover ID command processing handler
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param vdmEvt
* VDM event related to the Discover ID command processing
*
* \return
* Next VDM manager task
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_MngDiscId(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_vdm_evt_t vdmEvt);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_IsSvidStored
****************************************************************************//**
*
* Check if selected SVID is already saved
*
* \param svid_arr
* Received SVID response VDO buffer pointer
*
* \param svid
* Selected SVID
*
* \return
* True if selected SVID already saved for the further processing, otherwise return false.
*
*******************************************************************************/
bool Cy_PdAltMode_VdmTask_IsSvidStored(uint16_t *svid_arr, uint16_t svid);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_SaveSvids
****************************************************************************//**
*
* Save supported SVIDs for further processing
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param svid_arr
* Received SVIDs buffer pointer
*
* \param max_svid
* Maximum number of SVIDs that can be supported
*
* \return
* False if all supported SVIDs can be saved, otherwise returns true.
*
*******************************************************************************/
bool Cy_PdAltMode_VdmTask_SaveSvids(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t *svid_arr, uint8_t max_svid);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_IsCblSvidReq
****************************************************************************//**
*
* Checks if cable Discovery SVID has to run
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* True if cable Discovery SVID has to run, otherwise returns false.
*
*******************************************************************************/
bool Cy_PdAltMode_VdmTask_IsCblSvidReq(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_HandleSopSvidResp
****************************************************************************//**
*
* Evaluates valid UFP Discovery SVID response.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* Next VDM manager task to perform
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_HandleSopSvidResp(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_HandleCblSvidResp
****************************************************************************//**
*
* Evaluates valid cable Discovery SVID response.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* Next VDM manager task to perform
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_HandleCblSvidResp(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_HandleFailSvidResp
****************************************************************************//**
*
* Evaluates failed Discovery SVID responses.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* Next VDM manager task to perform
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_HandleFailSvidResp(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*  */
/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_MngDiscSvid
****************************************************************************//**
*
* Discovery SVID command handler
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param vdmEvt
* VDM event related to the Discover SVID command processing
*
* \return
* Next VDM manager task to perform
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_MngDiscSvid(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_vdm_evt_t vdmEvt);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_ResetMngr
****************************************************************************//**
*
* Reset VDM manager flags and handlers
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_VdmTask_ResetMngr(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_SendSopDpSoftReset
****************************************************************************//**
*
* Sends cable SOP" Soft Reset
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_VdmTask_SendSopDpSoftReset(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_VconnSwapCb
****************************************************************************//**
*
* Callback to handle Vconn Swap response
*
* \param ptrPdStackContext
* PD Stack Library Context pointer
*
* \param resp
* Vconn Swap response status
*
* \param pktPtr
* Vconn Swap response pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_VdmTask_VconnSwapCb(cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_resp_status_t resp, const cy_stc_pdstack_pd_packet_t *pktPtr);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_ComposeVdm
****************************************************************************//**
*
* Fills VDM buffer with corresponding data before send the VDM.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* True if all VDM-related data to fill is valid
*
*******************************************************************************/
uint8_t Cy_PdAltMode_VdmTask_ComposeVdm(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_ParseVdm
****************************************************************************//**
*
* Saves necessary data from received VDM for further analysis.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param recVdm
* Received VDM buffer pointer.
* \return
* True if all VDM data to save is valid
*
*******************************************************************************/
uint8_t Cy_PdAltMode_VdmTask_ParseVdm(cy_stc_pdaltmode_context_t *ptrAltModeContext, const cy_stc_pdstack_pd_packet_t* recVdm);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_SopDpSoftResetCb
****************************************************************************//**
*
* Callback to handle cable SOP" Soft Reset response
*
* \param ptrPdStackContext
* PD Stack Library Context pointer
*
* \param resp
*  Soft Reset response status
*
* \param pktPtr
*  Soft Reset response pointer
*
* \return
* None.
*
*******************************************************************************/
void Cy_PdAltMode_VdmTask_SopDpSoftResetCb(cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_resp_status_t resp, const cy_stc_pdstack_pd_packet_t *pktPtr);

/*******************************************************************************
* Function name: Cy_PdAltMode_VdmTask_GetStoredDiscResp
****************************************************************************//**
*
* Returns Discovery ID/SVID configuration table response pointer.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param is_disc_id
* True for Discovery ID response, false for Discovery SVID response
*
* \param respLenP
* Discovery ID/SVID response length pointer
*
* \return
* Discovery ID/SVID configuration table response pointer, otherwise returns NULL.
*
*******************************************************************************/
uint8_t *Cy_PdAltMode_VdmTask_GetStoredDiscResp(cy_stc_pdaltmode_context_t *ptrAltModeContext, bool is_disc_id, uint8_t *respLenP);

/** \} group_pdaltmode_vdm_alt_mode_manager_functions */

/** \} group_pdaltmode_vdm_alt_mode_manager */

#endif /* _CY_PDALTMODE_VDM_TASK_H_ */

/* [] END OF FILE */

