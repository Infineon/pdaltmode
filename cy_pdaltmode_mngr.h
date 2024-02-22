/***************************************************************************//**
* \file cy_pdaltmode_mngr.h
* \version 1.0
*
* \brief
* This header file defines the data structures and function prototypes associated
* with the Alternate Mode Source implementation.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_PDALTMODE_MNGR_H_
#define _CY_PDALTMODE_MNGR_H_

/*******************************************************************************
 * Header file addition
 ******************************************************************************/

#include "cy_pdaltmode_defines.h"

/**
* \addtogroup group_pdaltmode_vdm_alt_mode_manager
* \{
* Provides APIs and data structures to run/handle/evaluate alternate modes and 
* after configuration alternate mode manager automatically will handle Discovery/Entry/Configuration 
* of selected alternate modes (DP/TBT/USB4/Custom).
********************************************************************************
* \defgroup group_pdaltmode_vdm_alt_mode_manager_macros Macros
* \defgroup group_pdaltmode_vdm_alt_mode_manager_enums Enumerated Types
* \defgroup group_pdaltmode_vdm_alt_mode_manager_functions Functions
* \defgroup group_pdaltmode_vdm_alt_mode_manager_data_structures Data Structures
*/

/*******************************************************************************
 * MACRO definition
 ******************************************************************************/

/**
* \addtogroup group_pdaltmode_vdm_alt_mode_manager_macros
* \{
*/

#define CY_PDALTMODE_MNGR_ATCH_TGT                        (1u)
/**< Internal Alt Modes manager denotation of attached UFP target */

#define CY_PDALTMODE_MNGR_CABLE                           (2u)
/**< Internal Alt Modes manager denotation of the attached cable */

#define CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED              (0xFFu)
/**< Internal Alt Modes manager denotation of non supported Alternate Mode */

#define CY_PDALTMODE_MNGR_EMPTY_VDO                       (0x00000000u)
/**< Internal Alt Modes manager denotation in case if VDO sending not needed */

#define CY_PDALTMODE_MNGR_NONE_VDO                        (0xFFFFFFFFu)
/**< Internal Alt Modes manager denotation of zero VDO */

#define CY_PDALTMODE_MNGR_VDO_START_IDX                   (1u)
/**< Start index of VDO data in VDM message */

#define CY_PDALTMODE_MNGR_NONE_MODE_MASK                  (0x00000000u)
/**< Empty mask */

#define CY_PDALTMODE_MNGR_FULL_MASK                       (0xFFFFFFFFu)
/**< All bit set full mask */

#define CY_PDALTMODE_MNGR_EN_FLAG_MASK                    (0x00000001u)
/**< This mask is used by CY_PDALTMODE_MNGR_IS_FLAG_CHECKED macro to check if bit is set in some mask */

#define CY_PDALTMODE_MNGR_EXIT_ALL_MODES                  (0x7u)
/**< Object position corresponding to an Exit All Modes VDM command */

#define CY_PDALTMODE_MNGR_CBL_DIR_SUPP_MASK               (0x780u)
/**< Mask to find out cable directionality support for cable discovery ID response */

#define CY_PDALTMODE_MNGR_ALT_MODE_EVT_IDX                (0u)
/**< Index of Alt Mode APP event code */

#define CY_PDALTMODE_MNGR_ALT_MODE_EVT_DATA_IDX           (1u)
/**< Index of data for Alt Mode APP event */

#define CY_PDALTMODE_MNGR_NO_DATA            (0u)
/**< Internal Alt Modes manager denotation of object without useful data */

#define CY_PDALTMODE_MNGR_MAX_RETRY_CNT                   (9u)
/**< Maximum number of VDM send retries in case of no CRC response, timeout, or busy response */

#define CY_PDALTMODE_MNGR_AM_SVID_CONFIG_SIZE_IDX         (0u)
/**< Index of size of Alternate Mode configuration packet */    
    
#define CY_PDALTMODE_MNGR_AM_SVID_CONFIG_OFFSET_IDX       (1u)
/**< Offset of SVID in Alternate Mode configuration packet */

#define CY_PDALTMODE_MNGR_AM_SVID_CONFIG_OFFSET_START_IDX    (16u >> 1u)
/**< Offset of SVID start index in the config table */
    
#define CY_PDALTMODE_MNGR_DFP_ALT_MODE_HPI_OFFSET         (8u)
/**< Offset to get DFP Alt Modes to be enabled */    

#define CY_PDALTMODE_MNGR_UFP_ALT_MODE_HPI_MASK           (0xFFu)
/**< Mask to get UFP Alt Modes to be enabled */  

#define CY_PDALTMODE_MNGR_SET_FLAG(status, bit_idx)       ((status) |= ((uint32_t)1u << ((uint32_t)(bit_idx))))
/**< Set appropriate bit_idx to 1 in the status variable */

#define CY_PDALTMODE_MNGR_REMOVE_FLAG(status, bit_idx)    ((status) &= (~((uint32_t)1u << ((uint32_t)(bit_idx)))))
/**< Set appropriate bit_idx to 0 in the status variable */

#define CY_PDALTMODE_MNGR_IS_FLAG_CHECKED(status, bit_idx)        (((status) >> (uint32_t)(bit_idx)) & CY_PDALTMODE_MNGR_EN_FLAG_MASK)
/**< Return true if bit_idx of the status variable is set to '1' */

/** \} group_pdaltmode_vdm_alt_mode_manager_macros */

/*******************************************************************************
 * Enumerated data definition
 ******************************************************************************/

/**
* \addtogroup group_pdaltmode_vdm_alt_mode_manager_enums
* \{
*/

/** \} group_pdaltmode_vdm_alt_mode_manager_enums */

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/

/**
* \addtogroup group_pdaltmode_vdm_alt_mode_manager_functions
* \{
*/

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_RegSvidHdlr
****************************************************************************//**
*
* Store SVID related registration function
*
* \param svid
* Stored SVID
* \param regSvidFn
* Function pointer which is used to register selected SVID
*
* \return
* True if function was stored in Alt Modes manager, false if there's no space for new SVID handlers.
*
*******************************************************************************/
bool Cy_PdAltMode_Mngr_RegSvidHdlr(uint16_t svid, cy_pdaltmode_reg_alt_modes_cbk_t regSvidFn);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_Task
****************************************************************************//**
*
* Handler for PD Alt Mode level asynchronous task
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_Mngr_Task(cy_stc_pdaltmode_context_t *ptrAltModeContext);


/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_RegAltModeMngr
****************************************************************************//**
*
* Register pointers to attached dev/ama/cable info and run
* Alt Modes manager if success.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param atchTgtInfo
* Pointer to struct which holds discovery info about attached targets
*
* \param vdmMsgInfo
* Pointer to struct which holds info of received/sent VDM
*
* \return
* Returns VDM manager task \n 
* CY_PDALTMODE_VDM_TASK_ALT_MODE if CCG supports any Alternate Mode. \n 
* If CCG does not support Alternate Modes, function returns CY_PDALTMODE_VDM_TASK_EXIT.
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_RegAltModeMngr(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_atch_tgt_info_t* atchTgtInfo, cy_stc_pdaltmode_vdm_msg_info_t* vdmMsgInfo);


/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_AltModeProcess
****************************************************************************//**
*
* Used by DFP VDM manager to run Alt Modes manager processing
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param vdmEvt
* Current DFP VDM manager event
*
* \return
* DFP VDM manager task based on Alt Modes manager processing results
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_AltModeProcess(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_vdm_evt_t vdmEvt);


/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_EvalRecVdm
****************************************************************************//**
*
* Run received VDM analysis if CCG is UFP
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param vdm
* Pointer to pd packet which contains received VDM
*
* \return
* True if received VDM could handle successfully and VDM response need
* to be sent with ACK. If received VDM should be NACKed, then returns false.
*
*******************************************************************************/
cy_en_pdstack_std_vdm_cmd_type_t Cy_PdAltMode_Mngr_EvalRecVdm(cy_stc_pdaltmode_context_t *ptrAltModeContext, const cy_stc_pdstack_pd_packet_t *vdm);


/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_FormAltModeEvent
****************************************************************************//**
*
* Fill Alt Mode APP event fields with appropriate values
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param svid
* SVID of Alternate Mode which event refers to
*
* \param amIdx
* Index of Alternate Mode in compatibility table which event refers to
*
* \param evt
* Alternate Mode APP event
*
* \param data
* Alternate Mode APP event data
*
* \return
* Pointer to the event related data
*
*******************************************************************************/
const uint32_t* Cy_PdAltMode_Mngr_FormAltModeEvent(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t svid, uint8_t amIdx, cy_en_pdaltmode_app_evt_t evt, uint32_t data);

#if ((CY_HPI_ENABLED) || (APP_ALTMODE_CMD_ENABLE))
/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_EvalAppAltModeCmd
****************************************************************************//**
*
* Analyzes, parses, and runs Alternate Mode analysis function
* if received command is specific Alt Mode command.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param cmd
* Pointer to received Alt Mode APP command
*
* \param data
* Pointer to received Alt Mode APP command additional data
*
* \return
* True if APP command passed is successful; false if APP command is invalid,
* or contains unacceptable fields.
*
*******************************************************************************/
bool Cy_PdAltMode_Mngr_EvalAppAltModeCmd(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t *cmd, uint8_t *data);

#endif /* ((CY_HPI_ENABLED) || (APP_ALTMODE_CMD_ENABLE)) */


/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_IsAltModeMngrIdle
****************************************************************************//**
*
* Check whether the VDM manager for the selected port is idle
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* True if manager is busy, false if idle
*
*******************************************************************************/
bool Cy_PdAltMode_Mngr_IsAltModeMngrIdle(cy_stc_pdaltmode_context_t *ptrAltModeContext);

#if SYS_DEEPSLEEP_ENABLE
/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_Sleep
****************************************************************************//**
*
* Prepare the Alt Modes manager for device deep sleep
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/

void Cy_PdAltMode_Mngr_Sleep(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_Wakeup
****************************************************************************//**
*
* Restore the Alt Modes manager state after waking from deep sleep
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_Mngr_Wakeup(cy_stc_pdaltmode_context_t *ptrAltModeContext);
#endif /* SYS_DEEPSLEEP_ENABLE */

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_ResetAltModeInfo
****************************************************************************//**
*
* Resets Alternate Mode command info structure
*
* \param info
* Pointer to Alternate Mode info structure
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_Mngr_ResetAltModeInfo(cy_stc_pdaltmode_mngr_info_t* info);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_IsSvidSupported
****************************************************************************//**
*
* Check for presence of alt modes for given svid in alt modes compatibility table.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param svid
* Received SVID
*
* \return
* Index of supported SVID by CCG
*
*******************************************************************************/
uint8_t Cy_PdAltMode_Mngr_IsSvidSupported(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t svid);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetStatus
****************************************************************************//**
*
* Retrieve the current Alternate Mode status for a PD port
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* Alternate Mode status
*
*******************************************************************************/
uint32_t Cy_PdAltMode_Mngr_GetStatus(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_LayerReset
****************************************************************************//**
*
* Reset Alt Modes layer
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_Mngr_LayerReset(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_ResetInfo
****************************************************************************//**
*
* Reset the Alternate Mode manager state
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_Mngr_ResetInfo(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_ExitAll
****************************************************************************//**
*
* Exit all active Alternate Modes
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param sendVdmExit
* Flag which indicates if sending Exit VDM is required during exit all modes procedure
*
* \param exitAllCbk
* Callback which will call after exit all Alt Modes procedure will be finished
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_Mngr_ExitAll(cy_stc_pdaltmode_context_t *ptrAltModeContext, bool sendVdmExit, cy_pdstack_pd_cbk_t exitAllCbk);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetAltModeNumb
****************************************************************************//**
*
* Get number of Alternate Modes for chosen port
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* Number of Alternate Modes for chosen port and current data role
*
*******************************************************************************/
uint8_t Cy_PdAltMode_Mngr_GetAltModeNumb(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetAltModesConfigSvidIdx
****************************************************************************//**
*
* Get index of selected SVID from config table
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param type
* Type of the port i.e., cy_en_pd_port_type_t
*
* \param svid
* SVID
*
* \return
* SVID index
*
*******************************************************************************/
uint8_t Cy_PdAltMode_Mngr_GetAltModesConfigSvidIdx(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pd_port_type_t type, uint16_t svid);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_SetAltModeMask
****************************************************************************//**
*
* Set mask for Alternate Modes which should be enabled
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param mask
* Mask
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_Mngr_SetAltModeMask(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t mask);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_SetCustomSvid
****************************************************************************//**
*
* Set Alternate Modes custom SVID
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param svid
* SVID
*
* \return
* True if the SVID is set, otherwise false
*
*******************************************************************************/
bool Cy_PdAltMode_Mngr_SetCustomSvid(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t svid);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetCustomSvid
****************************************************************************//**
*
* Returns alternate modes custom SVID
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* SVID value
*
*******************************************************************************/
uint16_t Cy_PdAltMode_Mngr_GetCustomSvid(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetSvidFromIdx
****************************************************************************//**
*
* Returns SVID which is at index position in configuration table if such SVID is available.
*
* \param ptrAltModeContext
* PdAltMode Library Context pointer
*
* \param idx
* Index of the SVID
*
* \return
* SVID value
*
*******************************************************************************/
uint16_t Cy_PdAltMode_Mngr_GetSvidFromIdx(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t idx);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetModeInfo
****************************************************************************//**
*
* Get Alt Mode info
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param altModeIdx
* Alt Mode index
*
* \return
* Pointer to the Alternate Mode info
*
*******************************************************************************/
cy_stc_pdaltmode_mngr_info_t* Cy_PdAltMode_Mngr_GetModeInfo(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t altModeIdx);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetCableUsbCap
****************************************************************************//**
*
* Function returns Cable USB capabilities
*
* \param ptrPdStackContext
* PD Stack Library Context pointer
*
* \return
* Return Cable USB capabilities
*
*******************************************************************************/
cy_en_pdstack_usb_data_sig_t Cy_PdAltMode_Mngr_GetCableUsbCap(cy_stc_pdstack_context_t *ptrPdStackContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetModesVdoInfo
****************************************************************************//**
*
* Get modes info for the SVID
*
* \param ptrAltModeContext
* Pointer to Alt Mode context
*
* \param svid
* SVID number
*
* \param tempP
* Pointer to VDO info
*
* \param no_of_vdo
* Pointer to number of VDOs
*
* \return
* Returns true if SIVD mode info is found
*
*******************************************************************************/
bool Cy_PdAltMode_Mngr_GetModesVdoInfo(cy_stc_pdaltmode_context_t * ptrAltModeContext, uint16_t svid, cy_pd_pd_do_t **tempP, uint8_t *no_of_vdo);

#if ( CCG_UCSI_ENABLE && UCSI_ALT_MODE_ENABLED )

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetActiveAltModeMask
****************************************************************************//**
*
* Get active Alt Modes
*
* \param ptrAltModeContext
* Pointer to Alt Mode context
*
* \return
* Return the active Alt Mode mask info
*
*******************************************************************************/
uint32_t Cy_PdAltMode_Mngr_GetActiveAltModeMask(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_SetActiveAltModeState
****************************************************************************//**
*
* Set the Alt Mode state for given port
*
* \param ptrAltModeContext
* Pointer to Alt Mode context
*
* \param altModeIdx
* Alt Mode index of Type-C port
*
*
* \return
* True if SVID is supported by CCG
*
*******************************************************************************/
void Cy_PdAltMode_Mngr_SetActiveAltModeState(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t altModeIdx);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetSuppAltModes
****************************************************************************//**
*
* Get a bitmap of supported Alt Modes, given the current connection.
*
* \param ptrAltModeContext
* Pointer to Alt Mode context
*
* \return
* Returns the supported Alternate Modes
*
*******************************************************************************/
uint32_t Cy_PdAltMode_Mngr_GetSuppAltModes(cy_stc_pdaltmode_context_t *ptrAltModeContext);

#endif /*CCG_UCSI_ENABLE && UCSI_ALT_MODE_ENABLED*/

#if UFP_ALT_MODE_SUPP
#if (CCG_BB_ENABLE != 0)
    
/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_BillboardUpdate
****************************************************************************//**
* Updates the Alternate Mode status for a mode
*
* The current Billboard implementation supports a maximum of eight alternate modes,
* and the each mode as defined in the order of BOS descriptor, has two bit
* status. Bit 1:0 indicates status of Alt Mode 0, Bit 3:2 indicates status of
* Alt Mode 1, and so on. This function should be only used when the Alt Mode status
* needs to be set.
*
* \param ptrAltModeContext
* Pointer to the Alt Mode context
*
* @param amIdx
* Index of the Alt Mode
*
* @param bbStat
* Status of the Alternate Mode
*
* \return
* None
*
*******************************************************************************/

void Cy_PdAltMode_Mngr_BillboardUpdate(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t amIdx, bool bbStat);
#endif /* (CCG_BB_ENABLE != 0) */
#endif /* UFP_ALT_MODE_SUPP */

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetMngrStatus
****************************************************************************//**
*
* Returns Alt Mode manager status pointer
*
* \param port
* Selected port
*
* \return
* Returns pointer to Alt Mode manager status structure
*
*******************************************************************************/
cy_stc_pdaltmode_alt_mode_mngr_status_t* Cy_PdAltMode_Mngr_GetMngrStatus(uint8_t port);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetAmStatus
****************************************************************************//**
*
* Returns Alt Modes layer status pointer
*
* \param port
* Selected port
*
* \return
* Returns pointer to Alt Modes status structure
*
*******************************************************************************/
cy_stc_pdaltmode_status_t* Cy_PdAltMode_Mngr_GetAmStatus(uint8_t port);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_SetDiscModeParams
****************************************************************************//**
*
* Set Discovery ID/SVID/Mode mode parameters
*
* \param ptrAltModeContext Pointer to Alt Mode context
* \param sop SOP/SOP'/SOP" packet type
*
* \return None
*
*******************************************************************************/
void Cy_PdAltMode_Mngr_SetDiscModeParams(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pd_sop_t sop);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_SendSlnEventNoData
****************************************************************************//**
*
* Sends solution event without additional data info
*
* \param ptrAltModeContext Pointer to Alt Mode context
* \param svid SVID for which event is related
* \param am_id Alternate Mode ID for which event is related
* \param evtype Application event type
*
* \return None
*
*******************************************************************************/
void Cy_PdAltMode_Mngr_SendSlnEventNoData(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t svid, uint8_t am_id, cy_en_pdaltmode_app_evt_t evtype);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_SendSlnAppEvt
****************************************************************************//**
*
* Sends solution event with additional data info
*
* \param ptrAltModeContext Pointer to Alt Mode context
* \param data Data related to the solution event
*
* \return None
*
*******************************************************************************/
void Cy_PdAltMode_Mngr_SendSlnAppEvt(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint32_t data);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_SendAppEvtWrapper
****************************************************************************//**
*
* Wrapper to send solution event
*
* \param ptrAltModeContext Pointer to Alt Mode context
* \param reg Alternate Mode register structure pointer
*
* \return None
*
*******************************************************************************/
void Cy_PdAltMode_Mngr_SendAppEvtWrapper(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_alt_mode_reg_info_t *reg);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_RunDiscModeWrapper
****************************************************************************//**
*
* Wrapper to prepare and send Discovery Mode command
*
* \param ptrAltModeContext Pointer to Alt Mode context
*
* \return Next VMD manager task
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_RunDiscModeWrapper(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_HandleCblDiscMode
****************************************************************************//**
*
* Handler for processing cable Discovery Mode response
*
* \param ptrAltModeContext Pointer to Alt Mode context
* \param failed Indicator if cable Discovery command failed or not
*
* \return None
*
*******************************************************************************/
void Cy_PdAltMode_Mngr_HandleCblDiscMode(cy_stc_pdaltmode_context_t *ptrAltModeContext, bool failed);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_EvalDiscModeWrapper
****************************************************************************//**
*
* Wrapper to evaluate Discovery Mode command
*
* \param ptrAltModeContext Pointer to Alt Mode context
*
* \return Next VMD manager task
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_EvalDiscModeWrapper(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_DiscModeFailWrapper
****************************************************************************//**
*
* Wrapper to evaluate failed Discovery Mode command
*
* \param ptrAltModeContext Pointer to Alt Mode context
*
* \return Next VMD manager task
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_DiscModeFailWrapper(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_MonitorGetSopState
****************************************************************************//**
*
* Returns selected Alternate Mode SOP/SOP'/SOP'' pending messages after receiving first response
*
* \param amInfoP Pointer to Alternate Mode info structure
* \param faultStatus Status of previous command which was sent by Alt Mode manager
*
* \return SOP type of next message to be sent
*
*******************************************************************************/
cy_en_pd_sop_t Cy_PdAltMode_Mngr_MonitorGetSopState(cy_stc_pdaltmode_mngr_info_t  *amInfoP, uint8_t faultStatus);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_FailAltModesWrapper
****************************************************************************//**
*
* Wrapper to evaluate failed Alternate Mode commands
*
* \param ptrAltModeContext Pointer to Alt Mode context
*
* \return Next VMD manager task
*
*******************************************************************************/
cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_FailAltModesWrapper(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetNextAltMode
****************************************************************************//**
*
* Finds next available Alt Mode for further processing
*
* \param ptrAltModeContext Pointer to Alt Mode context
*
* \return Index of next available Alternate Mode, otherwise returns 0xFF
*
*******************************************************************************/
uint8_t Cy_PdAltMode_Mngr_GetNextAltMode(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_MoveToVdmInfo
****************************************************************************//**
*
* Composes Alt Mode info to vdm_msg_info_t struct before sending
*
* \param ptrAltModeContext Pointer to Alt Mode context
* \param info Pointer to the selected Alternate Mode info structure
* \param sopType SOP/SOP'/SOP" message type to send
*
* \return Non-zero if function proceed is successful
*
*******************************************************************************/
uint8_t Cy_PdAltMode_Mngr_MoveToVdmInfo(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_mngr_info_t* info, cy_en_pd_sop_t sopType);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetVdmInfoVdo
****************************************************************************//**
*
* Parses received VDM info and moves it to Alt Mode info struct
*
* \param ptrAltModeContext Pointer to Alt Mode context
* \param info Pointer to the selected Alternate Mode info structure
* \param sopType SOP/SOP'/SOP" message type to send
*
* \return None
*
*******************************************************************************/
void Cy_PdAltMode_Mngr_GetVdmInfoVdo(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_mngr_info_t* info, cy_en_pd_sop_t sopType);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetVdmInfoVdo
****************************************************************************//**
*
* Returns Alternate Mode info from config table
*
* \param ptrAltModeContext Pointer to Alt Mode context
* \param type Port data role
* \param idx Selected Alternate Mode index
*
* \return Pointer to selected Alternate Mode configuration if present, otherwise NULL pointer is returned.
*
*******************************************************************************/
uint16_t* Cy_PdAltMode_Mngr_GetAltModesVdoInfo(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pd_port_type_t type, uint8_t idx);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetBaseAltModeSvidIdx
****************************************************************************//**
*
* Returns Alt Mode index for given SVID
*
* \param ptrAltModeContext Pointer to Alt Mode context
* \param svid SVID 2-bytes value that the selected Alt Mode is related to
*
* \return SVID related Alternate Mode index, otherwise returns 0xFF.

*******************************************************************************/
uint8_t Cy_PdAltMode_Mngr_GetBaseAltModeSvidIdx(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t svid);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_AttentionCbk
****************************************************************************//**
*
* Callback to process saved Attention VDM
*
* \param id Timer ID used to postpone received Attention VDM
* \param context Timer callback context
*
* \return None
*
*******************************************************************************/
void Cy_PdAltMode_Mngr_AttentionCbk(cy_timer_id_t id, void* context);

/*******************************************************************************
* Function name: Cy_PdAltMode_Mngr_GetRegAmInfo
****************************************************************************//**
*
* Returns pointer to the registry structure related to the selected Alt Mode
*
* \param idx Selected alternate mode index
*
* \return Pointer to the registry structure related to the selected Alt Mode,
* otherwise returns NULL pointer.
*
*******************************************************************************/
cy_stc_pdaltmode_reg_am_t* Cy_PdAltMode_Mngr_GetRegAmInfo(uint8_t idx);

/** \} group_pdaltmode_vdm_alt_mode_manager_functions */

/** \} group_pdaltmode_vdm_alt_mode_manager */

#endif /* _CY_PDALTMODE_MNGR_H_ */

/* [] END OF FILE */
