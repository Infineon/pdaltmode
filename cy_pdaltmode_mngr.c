/***************************************************************************//**
* \file cy_pdaltmode_mngr.c
* \version 1.0
*
* \brief
* Alternate Mode Source implementation.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "cy_pdaltmode_defines.h"

#include "cy_pdutils.h"
#include "cy_pdstack_dpm.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_timer_id.h"
#include "cy_pdaltmode_dp_sid.h"

#include "cy_pdaltmode_mngr.h"
#include "cy_pdaltmode_hw.h"
#include "cy_pdaltmode_vdm_task.h"
#include "cy_pdaltmode_timer_id.h"
#if RIDGE_SLAVE_ENABLE
#include "cy_pdaltmode_ridge_slave.h"
#endif /* RIDGE_SLAVE_ENABLE */

#if GATKEX_CREEK
#include "cy_pdaltmode_intel_ridge_common.h"
#endif

#if STORE_DETAILS_OF_HOST
#include "cy_pdaltmode_host_details.h"
#endif

#if (CCG_BB_ENABLE != 0)
#include "cy_pdaltmode_billboard.h"
#endif /* (CCG_BB_ENABLE != 0) */

#if ((CY_PDALTMODE_MAX_SUPP_ALT_MODES < MAX_SVID_SUPP) && ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)))  
#error "The number of user alternate modes exceed maximum supported alt modes. \
        Please increase CY_PDALTMODE_MAX_SUPP_ALT_MODES"
#endif

/* Alternate Modes status structure */
static cy_stc_pdaltmode_status_t altModeAppStatus[NO_OF_TYPEC_PORTS];

cy_stc_pdaltmode_status_t* Cy_PdAltMode_Mngr_GetAmStatus(uint8_t port)
{
    return &altModeAppStatus[port];
}

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))

#if UFP_ALT_MODE_SUPP
/* This function verifies possibility of entering of corresponding UFP Alt Mode */
static bool Cy_PdAltMode_Mngr_IsModeActivated(cy_stc_pdaltmode_context_t *ptrAltModeContext, const cy_stc_pdstack_pd_packet_t *vdm);
/* UFP function for Alt Modes processing */
static bool Cy_PdAltMode_Mngr_UfpRegAltMode(cy_stc_pdaltmode_context_t *ptrAltModeContext, const cy_stc_pdstack_pd_packet_t *vdm);
/* Handles UFP enter mode processing */
static bool Cy_PdAltMode_Mngr_UfpEnterAltMode(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_mngr_info_t *amInfoP, const cy_stc_pdstack_pd_packet_t *vdm, uint8_t amIdx);
#endif /* UFP_ALT_MODE_SUPP */

/* Handles received VDM for specific Alt Mode */
static void Cy_PdAltMode_Mngr_VdmHandle(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_mngr_info_t *am_info, const cy_stc_pdstack_pd_packet_t *vdm);

/* Alt modes manager AMS prototypes */
static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_RunDiscMode(cy_stc_pdaltmode_context_t *ptrAltModeContext);
static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_EvalDiscMode(cy_stc_pdaltmode_context_t *ptrAltModeContext);
static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_DiscModeFail(cy_stc_pdaltmode_context_t *ptrAltModeContext);
static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_MonitorAltModes(cy_stc_pdaltmode_context_t *ptrAltModeContext);
static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_EvalAltModes(cy_stc_pdaltmode_context_t *ptrAltModeContext);
static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_FailAltModes(cy_stc_pdaltmode_context_t *ptrAltModeContext);
static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_AltModeMngrDeinit(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*State Table*/
static cy_en_pdaltmode_vdm_task_t (*const altModeAmsTable [CY_PDALTMODE_MNGR_STATE_PROCESS + 1u]
        [CY_PDALTMODE_VDM_EVT_EXIT + 1u]) (cy_stc_pdaltmode_context_t *ptrAltModeContext) = {
    {
        /* Send next discovery SVID */
        Cy_PdAltMode_Mngr_RunDiscMode,
        /* Evaluate disc SVID response */
        Cy_PdAltMode_Mngr_EvalDiscMode,
        /* Process failed disc SVID response */
        Cy_PdAltMode_Mngr_DiscModeFail,
        /* Exit from Alt Mode manager */
        Cy_PdAltMode_Mngr_AltModeMngrDeinit
    },
    {
        /* Monitor if any changes appears in modes  */
        Cy_PdAltMode_Mngr_MonitorAltModes,
        /* Evaluate Alt Mode response */
        Cy_PdAltMode_Mngr_EvalAltModes,
        /* Process failed Alt Modes response */
        Cy_PdAltMode_Mngr_FailAltModes,
        /* Exit from Alt Mode manager */
        Cy_PdAltMode_Mngr_AltModeMngrDeinit
    }
};

/* Structure which holds Alt Modes registration info */
static cy_stc_pdaltmode_reg_am_t regAltModeInfo[CY_PDALTMODE_MAX_SUPP_ALT_MODES];
/* Alternate Modes manager structure */
static cy_stc_pdaltmode_alt_mode_mngr_status_t altModeMngrStatus[NO_OF_TYPEC_PORTS];


cy_stc_pdaltmode_alt_mode_mngr_status_t* Cy_PdAltMode_Mngr_GetMngrStatus(uint8_t port)
{
    return &altModeMngrStatus[port];
}

cy_stc_pdaltmode_reg_am_t* Cy_PdAltMode_Mngr_GetRegAmInfo(uint8_t idx)
{
    return &regAltModeInfo[idx];
}

ATTRIBUTES_ALT_MODE bool Cy_PdAltMode_Mngr_RegSvidHdlr(uint16_t svid, cy_pdaltmode_reg_alt_modes_cbk_t regSvidFn)
{
    uint8_t idx;
    bool ret = false;

    for (idx = 0; idx < CY_PDALTMODE_MAX_SUPP_ALT_MODES; idx++)
    {
        if ((ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetRegAmInfo)(idx)->svid == CY_PDALTMODE_MNGR_NO_DATA) && (regSvidFn != NULL))
        {
            ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetRegAmInfo)(idx) ->svid = svid;
            ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetRegAmInfo)(idx)->regAmPtr = regSvidFn;
            ret = true;
            break;
        }
    }

    return ret;
}

void Cy_PdAltMode_Mngr_Task(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
#if STORE_DETAILS_OF_HOST
    Cy_PdAltMode_HostDetails_Task(ptrAltModeContext);
#else
    (void) ptrAltModeContext;
#endif /* STORE_DETAILS_OF_HOST */
}

/************************* DFP related function definitions *******************/

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_RegAltModeMngr(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_atch_tgt_info_t* atchTgtInfo, cy_stc_pdaltmode_vdm_msg_info_t* vdmMsgInfo)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    bool pd3Live = ((ptrPdStackContext->dpmConfig.specRevSopLive >= CY_PD_REV3) &&
            (ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP));

    ptrAltModeContext->altModeMngrStatus->altModesNumb = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModeNumb)(ptrAltModeContext);
    ptrAltModeContext->altModeMngrStatus->vdmInfo       = vdmMsgInfo;

    /* Check device role to start with. */
    if(ptrAltModeContext->pdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP)
    {
        if (atchTgtInfo->tgtSvid[0] == CY_PDALTMODE_MNGR_NO_DATA)
        {
            if(ptrAltModeContext->altModeAppStatus->usb4Active != false)
            {
                return CY_PDALTMODE_VDM_TASK_ALT_MODE;
            }
            /* UFP does not support any of the SVIDs of interest, hence exit VDM manager. */
            return CY_PDALTMODE_VDM_TASK_EXIT;
        }
        else
        {
            if (ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModeNumb)(ptrAltModeContext) != 0U)
            {
                /* Register pointers to VDM manager info */
                ptrAltModeContext->altModeMngrStatus->regInfo.atchTgtInfo  = atchTgtInfo;
                /* Set Alt Modes manager state to Discovery Mode process */
                ptrAltModeContext->altModeMngrStatus->regInfo.dataRole = (uint8_t)CY_PD_PRT_TYPE_DFP;
                ptrAltModeContext->altModeMngrStatus->state                   = CY_PDALTMODE_MNGR_STATE_DISC_MODE;

                /* Set Alt Mode trigger based on config */
                ptrAltModeContext->altModeAppStatus->altModeTrigMask = 0;
                return CY_PDALTMODE_VDM_TASK_ALT_MODE;
            }
        }
    }
    else
    {
        ptrAltModeContext->altModeMngrStatus->pd3Ufp = pd3Live;

        if (atchTgtInfo->tgtSvid[0] == CY_PDALTMODE_MNGR_NO_DATA)
        {
            /* If no there are no SVIDs to evaluate by UFP, then go to regular monitoring */
            ptrAltModeContext->altModeMngrStatus->state    = CY_PDALTMODE_MNGR_STATE_PROCESS;
            return CY_PDALTMODE_VDM_TASK_ALT_MODE;
        }
        else if (pd3Live)
        {
            /* If PD spec revision is 3.0, can start with discover mode process */
            return CY_PDALTMODE_VDM_TASK_ALT_MODE;
        }
        else
        {
        ; /* No action required - ; is optional */
        }
    }

    return CY_PDALTMODE_VDM_TASK_EXIT;
}

ATTRIBUTES_ALT_MODE bool Cy_PdAltMode_Mngr_IsAltModeMngrIdle(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    bool    isIdle = true;
    cy_stc_pdaltmode_mngr_info_t*  amInfoP = NULL;
    uint8_t           altModeIdx;

    if ((ptrAltModeContext->altModeMngrStatus->state == CY_PDALTMODE_MNGR_STATE_DISC_MODE) ||
            (!ALT_MODE_CALL_MAP(Cy_PdAltMode_HW_IsIdle)(ptrAltModeContext)))
    {
        return false;
    }

    for (altModeIdx = 0; altModeIdx < ptrAltModeContext->altModeMngrStatus->altModesNumb; altModeIdx++)
    {
        amInfoP = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetModeInfo)(ptrAltModeContext, altModeIdx);
        /* If mode is active */
        if ((CY_PDALTMODE_MNGR_IS_FLAG_CHECKED(ptrAltModeContext->altModeMngrStatus->amActiveModes, altModeIdx) != 0u))
        {
            /* If Alt Mode is not idle, then return current Alt Mode state */
            if ((amInfoP->modeState != CY_PDALTMODE_STATE_IDLE) ||
                    (amInfoP->appEvtNeeded != false))
            {
                isIdle = false;
            }
        }
    }

    return (isIdle);
}
#if SYS_DEEPSLEEP_ENABLE
void Cy_PdAltMode_Mngr_Sleep(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    Cy_PdAltMode_HW_Sleep (ptrAltModeContext);
}

void Cy_PdAltMode_Mngr_Wakeup(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    Cy_PdAltMode_HW_Wakeup (ptrAltModeContext);
}
#endif /* SYS_DEEPSLEEP_ENABLE */

cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_AltModeProcess(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_vdm_evt_t vdmEvt)
{
    /* Run Alt Modes manager ams table */
    return altModeAmsTable[ptrAltModeContext->altModeMngrStatus->state][vdmEvt](ptrAltModeContext);
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_Mngr_SetDiscModeParams(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pd_sop_t sop)
{
    cy_stc_pdaltmode_vdm_msg_info_t *vdmP = ptrAltModeContext->altModeMngrStatus->vdmInfo;

    vdmP->vdmHeader.val   = CY_PDALTMODE_MNGR_NO_DATA;

    /* Trigger TBT cable Disc mode if requested */
    if (ptrAltModeContext->altModeAppStatus->trigTbtVdoDisc)
    {
        vdmP->vdmHeader.std_vdm_hdr.svid = CY_PDALTMODE_TBT_SVID;        
    }
    else
    {
        vdmP->vdmHeader.std_vdm_hdr.svid     = ptrAltModeContext->altModeMngrStatus->regInfo.atchTgtInfo->tgtSvid[ptrAltModeContext->altModeMngrStatus->svidIdx];
    }
    vdmP->vdmHeader.std_vdm_hdr.cmd      = (uint32_t)CY_PDSTACK_VDM_CMD_DSC_MODES;
    vdmP->vdmHeader.std_vdm_hdr.objPos   = CY_PDALTMODE_MNGR_NO_DATA;
    vdmP->sopType         = (uint8_t)sop;
    vdmP->vdoNumb         = CY_PDALTMODE_MNGR_NO_DATA;
    vdmP->vdmHeader.std_vdm_hdr.vdmType = (uint32_t)CY_PDSTACK_VDM_TYPE_STRUCTURED;
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_Mngr_SendSlnEventNoData(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t svid, uint8_t am_id, cy_en_pdaltmode_app_evt_t evtype)
{
    ptrAltModeContext->pdStackContext->ptrAppCbk->app_event_handler (ptrAltModeContext->pdStackContext, APP_EVT_ALT_MODE,
           ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_FormAltModeEvent)(ptrAltModeContext, svid, am_id, evtype, CY_PDALTMODE_MNGR_NO_DATA));
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_Mngr_SendSlnAppEvt(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint32_t data)
{
    ptrAltModeContext->pdStackContext->ptrAppCbk->app_event_handler (ptrAltModeContext->pdStackContext, APP_EVT_ALT_MODE,
            ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_FormAltModeEvent)(ptrAltModeContext, (uint16_t)(ptrAltModeContext->altModeMngrStatus->vdmInfo->vdmHeader.std_vdm_hdr.svid),
                ptrAltModeContext->altModeMngrStatus->regInfo.altModeId,
                (data == CY_PDALTMODE_MNGR_NO_DATA) ? ptrAltModeContext->altModeMngrStatus->regInfo.appEvt : CY_PDALTMODE_EVT_DATA_EVT, data));
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_Mngr_SendAppEvtWrapper(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_alt_mode_reg_info_t *reg)
{
    if (reg->appEvt != CY_PDALTMODE_NO_EVT)
    {
        /* Send notifications to the solution */
        ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_SendSlnAppEvt)(ptrAltModeContext, CY_PDALTMODE_MNGR_NO_DATA);
        reg->appEvt = CY_PDALTMODE_NO_EVT;
    }
}

static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_RunDiscMode(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    return ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_RunDiscModeWrapper)(ptrAltModeContext);
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_RunDiscModeWrapper(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
#if DFP_ALT_MODE_SUPP
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    uint16_t curSvid;

    /* Set cable sop flag not needed at start*/
    ptrAltModeContext->altModeMngrStatus->regInfo.cblSopFlag = CY_PD_SOP_INVALID;

    /* Search for next SVID until SVID array is not empty */
    while ((curSvid = ptrAltModeContext->altModeMngrStatus->regInfo.atchTgtInfo->tgtSvid[ptrAltModeContext->altModeMngrStatus->svidIdx]) != 0U)
    {
        /* Check is current port date role UFP */
        if (ptrAltModeContext->altModeMngrStatus->pd3Ufp)
        {
            /* Send Disc Mode cmd */
            ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_SetDiscModeParams)(ptrAltModeContext, CY_PD_SOP);
            return CY_PDALTMODE_VDM_TASK_SEND_MSG;
        }

        if (ptrPdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP)
        {
            if (ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_IsSvidSupported)(ptrAltModeContext, curSvid) != CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED)
            {
                /* Send notifications to the solution */
                ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_SendSlnEventNoData)(ptrAltModeContext, curSvid, 0, CY_PDALTMODE_EVT_SVID_SUPP);

                /* If SVID is supported, send Disc Mode cmd */
                ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_SetDiscModeParams)(ptrAltModeContext, CY_PD_SOP);
                return CY_PDALTMODE_VDM_TASK_SEND_MSG;
            }

            /* Send notifications to the solution */
            ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_SendSlnEventNoData)(ptrAltModeContext, curSvid, 0, CY_PDALTMODE_EVT_SVID_NOT_SUPP);

            /* If SVID is not supported */
            ptrAltModeContext->altModeMngrStatus->svidIdx++;
        }
    }

    if ( 
            (ptrAltModeContext->altModeMngrStatus->amSupportedModes == CY_PDALTMODE_MNGR_NONE_MODE_MASK)
            && (ptrAltModeContext->altModeAppStatus->usb4Active == false)
       )
    {
        /* No supp modes */
        return CY_PDALTMODE_VDM_TASK_EXIT;
    }

    /* Send SVID discovery finished notification to the solution. Enabled only if HPI commands are enabled. */
    ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_SendSlnEventNoData)(ptrAltModeContext, 0, 0, CY_PDALTMODE_EVT_DISC_FINISHED);

    /* Goto Alt Mode process */
    if (ptrPdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP)
    {
        (void)ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetNextAltMode)(ptrAltModeContext);
    }

    ptrAltModeContext->altModeMngrStatus->state = CY_PDALTMODE_MNGR_STATE_PROCESS;

    return CY_PDALTMODE_VDM_TASK_ALT_MODE;
#else
    return CY_PDALTMODE_VDM_TASK_EXIT;
#endif /* DFP_ALT_MODE_SUPP */
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_Mngr_HandleCblDiscMode(cy_stc_pdaltmode_context_t *ptrAltModeContext, bool failed)
{
    uint8_t tblSvidIdx, vdoIdx, cfgSvidIdx, trigTbtVdoDiscFlg = 0x00U;
    cy_stc_pdaltmode_mngr_info_t *amInfoP;
    cy_stc_pdaltmode_vdm_msg_info_t    *vdmP = ptrAltModeContext->altModeMngrStatus->vdmInfo;
    cy_stc_pdaltmode_alt_mode_reg_info_t *reg   = &(ptrAltModeContext->altModeMngrStatus->regInfo);

    /*
       Get index of function in alt_mode_config related to receive SVID.
       This is expected to be valid as Discover MODE for the UFP 
       has already been gone through.
       */
    if (ptrAltModeContext->altModeAppStatus->trigTbtVdoDisc)
    {
        tblSvidIdx        = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetBaseAltModeSvidIdx)(ptrAltModeContext, CY_PDALTMODE_DP_SVID);
        trigTbtVdoDiscFlg = 0x01;
    }
    else
    {
        tblSvidIdx = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetBaseAltModeSvidIdx)(ptrAltModeContext, (uint16_t)vdmP->vdmHeader.std_vdm_hdr.svid);
    }

    reg->atchType = CY_PDALTMODE_MNGR_CABLE;

    /* Analyze all received VDOs */
    for (vdoIdx = 0; ((failed) || (vdoIdx < vdmP->vdoNumb)); vdoIdx++)
    {
        if (failed)
        {
            reg->cblSopFlag = CY_PD_SOP_INVALID;
        }
        else
        {
            /* Save current VDO and its position in SVID structure */
            reg->svidEmcaVdo = vdmP->vdo[vdoIdx];
            /* Save TBT cable VDO */
            if (vdmP->vdmHeader.std_vdm_hdr.svid == CY_PDALTMODE_TBT_SVID)
            {
                ptrAltModeContext->altModeAppStatus->tbtCblVdo = reg->svidEmcaVdo;
            }
        }

        /* Check if DFP support attached target Alt Mode */
        amInfoP = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetRegAmInfo)(tblSvidIdx)->regAmPtr(ptrAltModeContext, reg);
        if (amInfoP == NULL)
        {
            if (trigTbtVdoDiscFlg != 0x00U)
            {
                cfgSvidIdx = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_IsSvidSupported)(ptrAltModeContext, CY_PDALTMODE_DP_SVID);
                trigTbtVdoDiscFlg = 0x00U;
            }
            else
            {
                /* Get index of SVID related configuration from config.c */
                cfgSvidIdx = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_IsSvidSupported)(ptrAltModeContext, (uint16_t)vdmP->vdmHeader.std_vdm_hdr.svid);
            }

            if (cfgSvidIdx != CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED)
            {
                /* Remove pointer to Alt Mode info struct */
                ptrAltModeContext->altModeMngrStatus->altModeInfo[cfgSvidIdx] = NULL;

                /* Remove flag that Alt Mode could be run */
                CY_PDALTMODE_MNGR_REMOVE_FLAG(ptrAltModeContext->altModeMngrStatus->amSupportedModes, cfgSvidIdx);
            }
            reg->cblSopFlag = CY_PD_SOP_INVALID;
        }
        else
        {
            if (!failed){
                amInfoP->cblObjPos = (vdoIdx + CY_PDALTMODE_MNGR_VDO_START_IDX);
            }
        }

        ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_SendAppEvtWrapper)(ptrAltModeContext, reg);

        if (failed)
        {
            break;
        }
    }
}

static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_EvalDiscMode(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    return ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_EvalDiscModeWrapper)(ptrAltModeContext);
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_EvalDiscModeWrapper(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    volatile uint8_t tblSvidIdx, cfgSvidIdx;
    uint8_t  vdoIdx;
    cy_stc_pdaltmode_mngr_info_t      *amInfoP;
    cy_stc_pdaltmode_vdm_msg_info_t       *vdmP = ptrAltModeContext->altModeMngrStatus->vdmInfo;
    cy_stc_pdaltmode_alt_mode_reg_info_t  *reg   = &(ptrAltModeContext->altModeMngrStatus->regInfo);

    /* Check is current port date role UFP */
    if (ptrAltModeContext->altModeMngrStatus->pd3Ufp)
    {
        /* Goto next SVID */
        ptrAltModeContext->altModeMngrStatus->svidIdx++;
        return CY_PDALTMODE_VDM_TASK_ALT_MODE;
    }

    /* Evaluate SOP response */
    if (vdmP->sopType == (uint8_t)CY_PD_SOP)
    {
        /* Get index of function in alt_mode_config related to receive SVID */
        tblSvidIdx = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetBaseAltModeSvidIdx)(ptrAltModeContext, (uint16_t)vdmP->vdmHeader.std_vdm_hdr.svid);

        /* Get index of SVID related configuration from config.c */
        cfgSvidIdx = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_IsSvidSupported)(ptrAltModeContext, (uint16_t)vdmP->vdmHeader.std_vdm_hdr.svid);

        /* Additional check if there is support for current SVID operations */
        if (cfgSvidIdx != CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED)
        {
            reg->atchType = CY_PDALTMODE_MNGR_ATCH_TGT;

            /* Analyze all rec VDOs */
            for (vdoIdx = 0; vdoIdx < vdmP->vdoNumb; vdoIdx++)
            {
                /* Save current VDO and its position in SVID structure */
                reg->svidVdo = vdmP->vdo[vdoIdx];

                /* Check if DFP support attached target Alt Mode */
                amInfoP =  ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetRegAmInfo)(tblSvidIdx)->regAmPtr(ptrAltModeContext, reg);

                /* If VDO relates to any of supported Alt Modes */
                if (reg->altModeId != CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED)
                {
                    ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_SendAppEvtWrapper)(ptrAltModeContext, reg);

                    /* If Alternate Modes discovered and could be run */
                    if (amInfoP != NULL)
                    {
                        /* Save Alt Mode ID and object position */
                        amInfoP->altModeId = reg->altModeId;
                        amInfoP->objPos     = (vdoIdx + CY_PDALTMODE_MNGR_VDO_START_IDX);

                        if (amInfoP->appEvtNeeded != false)
                        {
                            /* Send notifications to the solution */
                            ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_SendSlnAppEvt)(ptrAltModeContext, amInfoP->appEvtData.val);
                            amInfoP->appEvtNeeded = false;
                        }

                        /* Save pointer to Alt Mode info struct */
                        ptrAltModeContext->altModeMngrStatus->altModeInfo[cfgSvidIdx] = amInfoP;

                        /* Set flag that Alt Mode could be run */
                        CY_PDALTMODE_MNGR_SET_FLAG(ptrAltModeContext->altModeMngrStatus->amSupportedModes, cfgSvidIdx);
                    }
                }

                if (vdmP->vdmHeader.std_vdm_hdr.svid == CY_PDALTMODE_DP_SVID)
                {
                    break;                    
                }
            }

            /* If cable DISC Mode is needed - send VDM */
            if (reg->cblSopFlag != CY_PD_SOP_INVALID)
            {
                ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_SetDiscModeParams)(ptrAltModeContext, CY_PD_SOP_PRIME);
                return CY_PDALTMODE_VDM_TASK_SEND_MSG;
            }
        }
    }
    /* Evaluate cable response: Packet type will be SOP' or SOP'' here */
    else
    {
        ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_HandleCblDiscMode)(ptrAltModeContext, false);

        /* If cable DISC Mode is needed, send VDM */
        if (reg->cblSopFlag != CY_PD_SOP_INVALID)
        {
            ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_SetDiscModeParams)(ptrAltModeContext, CY_PD_SOP_DPRIME);
            return CY_PDALTMODE_VDM_TASK_SEND_MSG;
        }
    }

    /* If there is no result, goto next SVID */
    ptrAltModeContext->altModeMngrStatus->svidIdx++;

    return CY_PDALTMODE_VDM_TASK_ALT_MODE;
}

static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_DiscModeFail(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    return ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_DiscModeFailWrapper)(ptrAltModeContext);
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_DiscModeFailWrapper(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{

    /* Check is current port date role UFP */
    if (ptrAltModeContext->altModeMngrStatus->pd3Ufp)
    {
        /* Goto next SVID */
        ptrAltModeContext->altModeMngrStatus->svidIdx++;
        return CY_PDALTMODE_VDM_TASK_ALT_MODE;
    }

    if (ptrAltModeContext->altModeMngrStatus->vdmInfo->sopType == (uint8_t)CY_PD_SOP_PRIME)
    {
        ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_HandleCblDiscMode)(ptrAltModeContext, true);
    }
    /* If Disc Mode cmd fails, goto next SVID */
    ptrAltModeContext->altModeMngrStatus->svidIdx++;

    return CY_PDALTMODE_VDM_TASK_ALT_MODE;
}

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
ATTRIBUTES_ALT_MODE cy_en_pd_sop_t Cy_PdAltMode_Mngr_MonitorGetSopState(cy_stc_pdaltmode_mngr_info_t  *amInfoP, uint8_t faultStatus)
{
    cy_en_pd_sop_t sopState;

    /* Do not send cable messages if VConn fault is present */
   if((faultStatus & (CY_PDALTMODE_FAULT_APP_PORT_VCONN_FAULT_ACTIVE | CY_PDALTMODE_FAULT_APP_PORT_V5V_SUPPLY_LOST)) != 0U)
   {
       amInfoP->sopState[CY_PD_SOP_PRIME]  = CY_PDALTMODE_STATE_IDLE;
       amInfoP->sopState[CY_PD_SOP_DPRIME] = CY_PDALTMODE_STATE_IDLE;
   }

   /* Set Exit mode sequence SOP -> SOP''-> SOP' */
   if (
           (amInfoP->vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_EXIT_MODE) ||
          ((amInfoP->vdmHeader.std_vdm_hdr.svid == CY_PDALTMODE_DP_SVID) &&
          ((amInfoP->vdo[CY_PD_SOP]->val & CY_PDALTMODE_DP_USB_SS_CONFIG_MASK) == 0x00UL) &&
           (amInfoP->vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDALTMODE_DP_STATE_CONFIG))
      )
   {
       if (amInfoP->sopState[CY_PD_SOP] == CY_PDALTMODE_STATE_SEND)
       {
           sopState = CY_PD_SOP;
       }
       else if (amInfoP->sopState[CY_PD_SOP_DPRIME] == CY_PDALTMODE_STATE_SEND)
       {
           sopState = CY_PD_SOP_DPRIME;
       }
       else
       {
           sopState = CY_PD_SOP_PRIME;
       }
   }
   else
   {
       if (amInfoP->sopState[CY_PD_SOP_PRIME] == CY_PDALTMODE_STATE_SEND)
       {
           sopState = CY_PD_SOP_PRIME;
       }
       else if (amInfoP->sopState[CY_PD_SOP_DPRIME] == CY_PDALTMODE_STATE_SEND)
       {
           sopState = CY_PD_SOP_DPRIME;
       }
       else
       {
           sopState = CY_PD_SOP;
       }
   }

   return sopState;
}
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */

static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_MonitorAltModes(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_en_pdaltmode_vdm_task_t stat = CY_PDALTMODE_VDM_TASK_ALT_MODE;
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
    uint8_t altModeIdx;
    cy_en_pdaltmode_state_t altModeState;
    cy_en_pd_sop_t sopState;
    cy_stc_pdaltmode_mngr_info_t  *amInfoP  = NULL;

    uint8_t faultStatus = ptrAltModeContext->appStatusContext->faultStatus;

    if (ptrAltModeContext->altModeMngrStatus->runAttFlag != 0x00U)
    {
        cy_stc_pdstack_pd_packet_t vdm;
        cy_stc_pdaltmode_alt_mode_mngr_status_t *am = ptrAltModeContext->altModeMngrStatus;
        am->runAttFlag = 0x00U;
        if ((am->attAltMode != NULL) && (am->attAltMode->isActive))
        {

            /* Form VDM packet from the saved Attention and try to re-call attention handler in Alt Mode. */
            vdm.dat[CY_PD_VDM_HEADER_IDX] = am->attHeader;
            vdm.dat[CY_PDALTMODE_MNGR_VDO_START_IDX]  = am->attVdo;
            vdm.len                 = CY_PDALTMODE_MNGR_VDO_START_IDX + 0x01U;
            /* Run Alt Mode VDM handler */
            Cy_PdAltMode_Mngr_VdmHandle(ptrAltModeContext, am->attAltMode, &vdm);
            return stat;
        }
    }

    /* Look through all Alt Modes  */
    for (altModeIdx = 0; altModeIdx < ptrAltModeContext->altModeMngrStatus->altModesNumb; altModeIdx++)
    {
        /* If mode is active */
        if (CY_PDALTMODE_MNGR_IS_FLAG_CHECKED(ptrAltModeContext->altModeMngrStatus->amActiveModes, altModeIdx) != 0u)
        {
            amInfoP = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetModeInfo)(ptrAltModeContext, altModeIdx);
            /* Get Alt Mode state */
            altModeState = amInfoP->modeState;
            switch (altModeState)
            {
                case CY_PDALTMODE_STATE_SEND:
                    /* This case activates when VDM sequence for given Alt Mode */
                    /* was interrupted by Alt Mode with higher priority. */
                case CY_PDALTMODE_STATE_WAIT_FOR_RESP:

                    /* Check if SOP' or SOP'' messages are required */
                    sopState = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_MonitorGetSopState)(amInfoP, faultStatus);

                    /* Check if MUX can and needs to be set*/
                    if (
                            ((amInfoP->vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_ENTER_MODE) ||
                             (amInfoP->vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_EXIT_MODE)) &&
                            (amInfoP->setMuxIsolate != false)          &&
#if HPI_AM_SUPP
                            ((ptrAltModeContext->altModeMngrStatus->amActiveModes & (~(1 << altModeIdx)) &ptrAltModeContext->altModeMngrStatus->amDatapathChangeModes) == false)
#else
                            ((ptrAltModeContext->altModeMngrStatus->amActiveModes & (~(0x01UL << altModeIdx))) == 0x0UL)
#endif /* HPI_AM_SUPP */
                       )
                    {
#if GATKEX_CREEK
                        /* To set the Ridge to disconnect between USB and TBT states */
                        Cy_PdAltMode_Ridge_SetDisconnect(ptrAltModeContext);
#else
                        (void)Cy_PdAltMode_HW_SetMux(ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SAFE, CY_PDALTMODE_MNGR_NO_DATA, CY_PDALTMODE_MNGR_NO_DATA);
#endif
                    }

#if AMD_SUPP_ENABLE
                    /* Check if MUX should be set to SafeState while entering Alt Mode*/
                    if (
                           (amInfoP->vdmHeader.std_vdm_hdr.cmd == CY_PDSTACK_VDM_CMD_ENTER_MODE) &&
                           (amInfoP->amSafeReq == true)
                        )
                    {
                        amInfoP->amSafeReq = false;
                        Cy_PdAltMode_HW_SetMux(ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SAFE, CY_PDALTMODE_MNGR_NO_DATA, CY_PDALTMODE_MNGR_NO_DATA);
                        return stat;
                    }
#endif /* AMD_SUPP_ENABLE */

                    if (
                            (amInfoP->isActive) ||
                            (amInfoP->vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_ENTER_MODE) || 
                            (amInfoP->vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_EXIT_MODE)
                       )
                    {
                        /* Copy to vdm info and send vdm */
                        (void)ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_MoveToVdmInfo)(ptrAltModeContext, amInfoP, sopState);
                        amInfoP->modeState = CY_PDALTMODE_STATE_WAIT_FOR_RESP;
                    }

                    stat = CY_PDALTMODE_VDM_TASK_SEND_MSG;
                    break;

                case CY_PDALTMODE_STATE_EXIT:
#if HPI_AM_SUPP
#if DYNAMIC_CUSTOM_SVID_CHANGE_ENABLE
                    if (ptrAltModeContext->altModeMngrStatus->resetCustomMode != false)
                    {
                        /* Send enter mode with new custom SVID Alt Mode with new SVID */
                        if (amInfoP->cbk != NULL)
                        {
                            cy_stc_pdaltmode_alt_mode_evt_t tmp_data;
                            /* Send command to change custom SVID */
                            amInfoP->evalAppCmd(port, tmp_data);
                            amInfoP->modeState = CY_PDALTMODE_STATE_INIT;
                            amInfoP->cbk(ptrAltModeContext);
                            /* Set flag that reset is required */
                            ptrAltModeContext->altModeMngrStatus->resetCustomMode = false;
                            break;
                        }                   
                    }
#endif /* DYNAMIC_CUSTOM_SVID_CHANGE_ENABLE */
#endif /* HPI_AM_SUPP */

#if DFP_ALT_MODE_SUPP
                    if(ptrAltModeContext->pdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP)
                    {
                        /* Remove flag from active mode */
                        CY_PDALTMODE_MNGR_REMOVE_FLAG(ptrAltModeContext->altModeMngrStatus->amActiveModes, altModeIdx);

                        /* Set Alt Mode as disabled */
                        amInfoP->modeState = CY_PDALTMODE_STATE_DISABLE;

                        /* Set flag as exited mode */
                        CY_PDALTMODE_MNGR_SET_FLAG(ptrAltModeContext->altModeMngrStatus->amExitedModes, altModeIdx);

                        /* If any active modes are present */
                        if (ptrAltModeContext->altModeMngrStatus->amActiveModes == CY_PDALTMODE_MNGR_NONE_MODE_MASK)
                        {
                            /* Notify APP layer that Alt Mode has been exited */
                            ptrAltModeContext->altModeAppStatus->altModeEntered = false;

                            /* Set MUX to SS config */
                            (void)Cy_PdAltMode_HW_SetMux(ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SS_ONLY, CY_PDALTMODE_MNGR_NO_DATA, CY_PDALTMODE_MNGR_NO_DATA);
                            if (ptrAltModeContext->altModeMngrStatus->exitAllFlag == false)
                            {
                                /* Get next Alt Mode if available */
                                (void)ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetNextAltMode)(ptrAltModeContext);
                            }
                            else
                            {
                                /* Run callback command after all Alt Modes were exited */
                                ptrAltModeContext->altModeMngrStatus->exitAllCbk(ptrAltModeContext->pdStackContext, true);
                                ptrAltModeContext->altModeMngrStatus->exitAllFlag = false;
                                stat = CY_PDALTMODE_VDM_TASK_WAIT;
                            }

                        }
                    }
#endif /* DFP_ALT_MODE_SUPP */

#if UFP_ALT_MODE_SUPP
                    if(ptrAltModeContext->pdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP)
                    {
                        if (ptrAltModeContext->altModeMngrStatus->amActiveModes != CY_PDALTMODE_MNGR_NONE_MODE_MASK)
                        {
                            /* Notify APP layer that Alt mode has been exited */
                            ptrAltModeContext->altModeAppStatus->altModeEntered = false;
                        }

#if (!ICL_ALT_MODE_HPI_DISABLED)
                        /* Send notifications to the solution if Alt Mode was entered */
                        ptrAltModeContext->pdStackContext->ptrAppCbk->app_event_handler (ptrAltModeContext->pdStackContext, APP_EVT_ALT_MODE,
                                ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_FormAltModeEvent)(ptrAltModeContext,(uint16_t)amInfoP->vdmHeader.std_vdm_hdr.svid,
                                    amInfoP->altModeId, CY_PDALTMODE_EVT_ALT_MODE_EXITED, CY_PDALTMODE_MNGR_NO_DATA));
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */

                        /* Set Alt Mode not active */
                        amInfoP->isActive = false;

                        /* Remove flag that Alt Mode could be processed */
                        CY_PDALTMODE_MNGR_REMOVE_FLAG(ptrAltModeContext->altModeMngrStatus->amActiveModes, altModeIdx);
                    }
#endif /* UFP_ALT_MODE_SUPP   */
                    break;

                case CY_PDALTMODE_STATE_IDLE:
#if (!ICL_ALT_MODE_HPI_DISABLED)
                    /* If Alt Modes need to send app event data */
                    if (amInfoP->appEvtNeeded != false)
                    {
                        /* Send notifications to the solution. */
                        ptrAltModeContext->pdStackContext->ptrAppCbk->app_event_handler (ptrAltModeContext->pdStackContext, APP_EVT_ALT_MODE,
                                ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_FormAltModeEvent)(ptrAltModeContext,(uint16_t)amInfoP->vdmHeader.std_vdm_hdr.svid,
                                    amInfoP->altModeId, CY_PDALTMODE_EVT_DATA_EVT, amInfoP->appEvtData.val));

                        amInfoP->appEvtNeeded = false;
                    }
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */                
                    break;

                case CY_PDALTMODE_STATE_RUN:
                    /* Run UFP evaluation function */
                    if (amInfoP->cbk != NULL)
                    {
                        amInfoP->cbk(ptrAltModeContext);
                    }
                    break;

                default:
                    /* Intentionally left empty */
                    break;

            }

            if(stat == CY_PDALTMODE_VDM_TASK_SEND_MSG)
            {
                break;
            }
        }
    }
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */
    return stat;
}

static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_EvalAltModes(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    uint8_t         altModeIdx;
    bool            evalFlag   = false;
    bool            skipHandle = false;
    cy_en_pdaltmode_state_t   altModeState;
    cy_en_pdaltmode_vdm_task_t         stat = CY_PDALTMODE_VDM_TASK_ALT_MODE;
    cy_stc_pdaltmode_mngr_info_t    *amInfoP;
    cy_stc_pdaltmode_vdm_msg_info_t     *vdmP = ptrAltModeContext->altModeMngrStatus->vdmInfo;

#if (!ICL_ALT_MODE_EVTS_DISABLED)    
    cy_en_pdaltmode_app_evt_t appEvtType = CY_PDALTMODE_NO_EVT;
#if (!ICL_ALT_MODE_HPI_DISABLED)
    uint32_t           appevt_data = CY_PDALTMODE_MNGR_NO_DATA;
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */
#endif /*  !ICL_ALT_MODE_EVTS_DISABLED */   

    /* Look through all Alt Modes  */
    for (altModeIdx = 0; altModeIdx < ptrAltModeContext->altModeMngrStatus->altModesNumb; altModeIdx++)
    {
        /* If mode is active */
        if (CY_PDALTMODE_MNGR_IS_FLAG_CHECKED(ptrAltModeContext->altModeMngrStatus->amActiveModes, altModeIdx) != 0u)
        {
            amInfoP = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetModeInfo)(ptrAltModeContext, altModeIdx);

            /* If pointer to cbk is NULL then return */
            if (amInfoP->cbk == NULL)
            {
                break;
            }

            /* Get Alt Mode state */
            altModeState = amInfoP->modeState;
            cy_en_pd_sop_t next_sop = CY_PD_SOP_INVALID;
            switch (altModeState)
            {
                case CY_PDALTMODE_STATE_WAIT_FOR_RESP:
                    /* Set flag that send transaction passed successful */
                    amInfoP->sopState[vdmP->sopType] = CY_PDALTMODE_STATE_IDLE;

                    /* Copy received resp to Alt Mode info struct */
                    ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetVdmInfoVdo)(ptrAltModeContext, amInfoP, (cy_en_pd_sop_t)vdmP->sopType);

                    /* Set Exit mode sequence SOP -> SOP''-> SOP' */
                    if (amInfoP->vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_EXIT_MODE)
                    {
                        if (amInfoP->sopState[CY_PD_SOP_DPRIME] == CY_PDALTMODE_STATE_SEND)
                        {
                            /* If needed send SOP'' VDM */
                            next_sop = CY_PD_SOP_DPRIME;
                        }
                        else if (amInfoP->sopState[CY_PD_SOP_PRIME] == CY_PDALTMODE_STATE_SEND)
                        {
                            /* If SOP'' not needed - send SOP VDM */
                            next_sop = CY_PD_SOP_PRIME;
                        }
                        else
                        {
                            evalFlag = true;
                        }
                    }
#if DP_DFP_SUPP
                    /* Set DP USB SS command sequence SOP -> SOP'-> SOP'' */
                    if ((amInfoP->vdmHeader.std_vdm_hdr.svid == CY_PDALTMODE_DP_SVID) &&
                            ((amInfoP->vdo[CY_PD_SOP]->val & CY_PDALTMODE_DP_USB_SS_CONFIG_MASK) == 0x00U) &&
                            (amInfoP->vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDALTMODE_DP_STATE_CONFIG))
                    {
                        if (amInfoP->sopState[CY_PD_SOP_PRIME] == CY_PDALTMODE_STATE_SEND)
                        {
                            /* If needed send SOP'' VDM */
                            next_sop = CY_PD_SOP_PRIME;
                        }
                        else if (amInfoP->sopState[CY_PD_SOP_DPRIME] == CY_PDALTMODE_STATE_SEND)
                        {
                            /* If needed, send SOP'' VDM */
                            next_sop = CY_PD_SOP_DPRIME;
                        }
                        else
                        {
                            evalFlag = true;
                        }
                    }
#endif /* DP_DFP_SUPP */
                    if (next_sop != CY_PD_SOP_INVALID)
                    {
                        skipHandle = true;
                    }

                    if (skipHandle == false)
                    {
                        /* If received VDM is SOP */
                        if ((vdmP->sopType == (uint8_t)CY_PD_SOP) || (evalFlag != false))
                        {
                            /* Run alt mode analysis function */
                            amInfoP->cbk(ptrAltModeContext);
                            /* If UVDM command then break */
                            if (vdmP->vdmHeader.std_vdm_hdr.vdmType != (uint32_t)CY_PDSTACK_VDM_TYPE_STRUCTURED)
                            {
                                break;
                            }
                            /* If Alt Mode entered */
                            if (vdmP->vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_ENTER_MODE)
                            {
                                /* Notify APP layer that Alt Mode has been entered */
                                ptrAltModeContext->altModeAppStatus->altModeEntered = true;

                                /* Set mode as active */
                                amInfoP->isActive = true;

#if (!ICL_ALT_MODE_EVTS_DISABLED)
                                /* Queue notifications to the solution */
                                appEvtType = CY_PDALTMODE_EVT_ALT_MODE_ENTERED;
#endif /* (!ICL_ALT_MODE_EVTS_DISABLED) */
                            }

                            if (vdmP->vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_EXIT_MODE)
                            {
                                /* Set mode as not active */
                                amInfoP->isActive = false;
#if (!ICL_ALT_MODE_EVTS_DISABLED)
                                /* Queue notifications to the solution */
                                appEvtType = CY_PDALTMODE_EVT_ALT_MODE_EXITED;
#endif /* (!ICL_ALT_MODE_EVTS_DISABLED) */
                            }

#if (!ICL_ALT_MODE_EVTS_DISABLED)
#if (!ICL_ALT_MODE_HPI_DISABLED)
                            /* If Alt Modes need to send app event data */
                            if (amInfoP->appEvtNeeded != false)
                            {
                                /* Queue notifications to the solution */
                                appEvtType = CY_PDALTMODE_EVT_DATA_EVT;
                                appevt_data = amInfoP->appEvtData.val;
                                amInfoP->appEvtNeeded = false;
                            }
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */

                            /* Send event to solution space if required */
                            if (appEvtType != CY_PDALTMODE_NO_EVT)
                            {
#if (!ICL_ALT_MODE_HPI_DISABLED)
                                ptrAltModeContext->pdStackContext->ptrAppCbk->app_event_handler (ptrAltModeContext->pdStackContext, APP_EVT_ALT_MODE,
                                        ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_FormAltModeEvent)(ptrAltModeContext, (uint16_t)amInfoP->vdmHeader.std_vdm_hdr.svid,
                                            amInfoP->altModeId, appEvtType, appevt_data));
#else
                                        ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_SendSlnEventNoData)(port,
                                        amInfoP->VDM_HDR.svid,
                                        amInfoP->altModeId,
                                        appEvtType);
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */
                            }
#endif /* (!ICL_ALT_MODE_EVTS_DISABLED) */
                        }
                        else
                        {
                            /* If received VDM is SOP' type, check if SOP'' is needed.
                               If received VDM is SOP'', the check on SOP_DPRIME will fail trivially. */
                            if (amInfoP->sopState[CY_PD_SOP_DPRIME] == CY_PDALTMODE_STATE_SEND)
                            {
                                /* If needed send SOP'' VDM */
                                next_sop = CY_PD_SOP_DPRIME;
                            }
                            else if (amInfoP->sopState[CY_PD_SOP] == CY_PDALTMODE_STATE_SEND)
                            {
                                /* If SOP'' not needed - send SOP VDM */
                                next_sop = CY_PD_SOP;
                            }
                            else
                            {
                                amInfoP->cbk(ptrAltModeContext);
                            }
                        }
                    }

                    if (next_sop != CY_PD_SOP_INVALID)
                    {
                        (void)ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_MoveToVdmInfo)(ptrAltModeContext, amInfoP, next_sop);
                        stat = CY_PDALTMODE_VDM_TASK_SEND_MSG;
                    }
                    break;

                default:
                    /* Intentionally left empty */
                    break;
            }
        }
    }

    return stat;
}

static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_FailAltModes(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
   return ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_FailAltModesWrapper)(ptrAltModeContext);
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_FailAltModesWrapper(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    uint8_t             altModeIdx;
    cy_en_pdaltmode_state_t    altModeState;
    cy_en_pdaltmode_app_evt_t  appEvtType = CY_PDALTMODE_EVT_CBL_RESP_FAILED;
    cy_stc_pdaltmode_mngr_info_t     *amInfoP = NULL;
    cy_stc_pdaltmode_vdm_msg_info_t      *vdmP = ptrAltModeContext->altModeMngrStatus->vdmInfo;

    /* Look through all Alt Modes */
    for (altModeIdx = 0; altModeIdx < ptrAltModeContext->altModeMngrStatus->altModesNumb; altModeIdx++)
    {
        /* If mode is active */
        if (CY_PDALTMODE_MNGR_IS_FLAG_CHECKED(ptrAltModeContext->altModeMngrStatus->amActiveModes, altModeIdx) != 0u)
        {
            amInfoP = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetModeInfo)(ptrAltModeContext, altModeIdx);

            /* Get Alt Mode state */
            altModeState = amInfoP->modeState;

            /* If mode waits for response */
            if (altModeState == CY_PDALTMODE_STATE_WAIT_FOR_RESP)
            {
                /* Change status to fail */
                amInfoP->sopState[vdmP->sopType] = CY_PDALTMODE_STATE_FAIL;

                /* Set Failure code at the object pos field */
                amInfoP->vdmHeader.std_vdm_hdr.objPos = vdmP->vdmHeader.std_vdm_hdr.objPos;

                if (vdmP->sopType == (uint8_t)CY_PD_SOP)
                {
                    appEvtType = CY_PDALTMODE_EVT_SOP_RESP_FAILED;
                }

                /* Send notifications to the solution */
                ptrAltModeContext->pdStackContext->ptrAppCbk->app_event_handler (ptrAltModeContext->pdStackContext, APP_EVT_ALT_MODE,
                        ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_FormAltModeEvent)(ptrAltModeContext, (uint16_t)amInfoP->vdmHeader.std_vdm_hdr.svid,
                            amInfoP->altModeId, appEvtType, CY_PDALTMODE_MNGR_NO_DATA));

                /* Run Alt Mode analysis function */
                amInfoP->modeState = CY_PDALTMODE_STATE_FAIL;
                if (amInfoP->cbk != NULL)
                {
                    amInfoP->cbk(ptrAltModeContext);
                }
            }
        }
    }

    return CY_PDALTMODE_VDM_TASK_ALT_MODE;
}

#if DFP_ALT_MODE_SUPP
ATTRIBUTES_ALT_MODE uint8_t Cy_PdAltMode_Mngr_GetNextAltMode(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    uint16_t* amVdoPtr = NULL;
    cy_stc_pdaltmode_mngr_info_t  *amInfoP  = NULL;
    ptrAltModeContext->altModeMngrStatus->amDatapathChangeModes = CY_PDALTMODE_MNGR_NO_DATA;
    uint8_t amIdx = CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED;
    uint32_t idx1, idx2, amConfigSvid, amNumb = ptrAltModeContext->altModeMngrStatus->altModesNumb;
    uint32_t avalModes = (
            (ptrAltModeContext->altModeMngrStatus->amSupportedModes) &
            (~ptrAltModeContext->altModeMngrStatus->amExitedModes) &
            (~ptrAltModeContext->altModeAppStatus->altModeTrigMask)
            );

    /* Start looking for next supported Alt Modes */
    for (idx1 = 0; idx1 < amNumb; idx1++)
    {
        /* If mode supported by CCG and not processed yet */
        if (CY_PDALTMODE_MNGR_IS_FLAG_CHECKED((avalModes), idx1) != 0x0UL)
        {
            /* Look for the Alt Modes which could be run */
            amVdoPtr = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModesVdoInfo)(ptrAltModeContext, CY_PD_PRT_TYPE_DFP, (uint8_t)idx1);
            amConfigSvid = CY_PDALTMODE_MNGR_NO_DATA;

            for (idx2 = 0; (uint16_t)idx2 < (amVdoPtr[CY_PDALTMODE_MNGR_AM_SVID_CONFIG_SIZE_IDX] >> 1); idx2++)
            {
                /* Check which Alternate Modes could be run simultaneously */
                amIdx = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModesConfigSvidIdx)(ptrAltModeContext, CY_PD_PRT_TYPE_DFP,
                        amVdoPtr[idx2 + CY_PDALTMODE_MNGR_AM_SVID_CONFIG_OFFSET_IDX]);
                if (amIdx != CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED)
                {
                    CY_PDALTMODE_MNGR_SET_FLAG(amConfigSvid, amIdx);
                    /* Set datapath mask */
                    amInfoP = ptrAltModeContext->altModeMngrStatus->altModeInfo[amIdx];
                    if ((amInfoP != NULL) && (amInfoP->setMuxIsolate != false))
                    {
                        CY_PDALTMODE_MNGR_SET_FLAG(ptrAltModeContext->altModeMngrStatus->amDatapathChangeModes, amIdx);
                    }
                }
            }

            /* Check if HPI SVID is supported */
            if (ptrAltModeContext->altModeAppStatus->customHpiSvid != CY_PDALTMODE_MNGR_NO_DATA)
            {
                uint8_t hpiSvidIdx = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_IsSvidSupported)(ptrAltModeContext, ptrAltModeContext->altModeAppStatus->customHpiSvid);

                /* Set bit as active if it's HPI Alt Mode */
                if (hpiSvidIdx != CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED)
                {
                    CY_PDALTMODE_MNGR_SET_FLAG(amConfigSvid, hpiSvidIdx);
                }
            }

            /* Set Alternate Mode Enter mask */
            ptrAltModeContext->altModeMngrStatus->amActiveModes = amConfigSvid & avalModes;

            return 0x01U;
        }
    }

    return 0x00U;
}

#endif /* DFP_ALT_MODE_SUPP   */

ATTRIBUTES_ALT_MODE uint8_t Cy_PdAltMode_Mngr_MoveToVdmInfo(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_mngr_info_t *info, cy_en_pd_sop_t sopType)
{
    cy_stc_pdaltmode_vdm_msg_info_t *vdmP = ptrAltModeContext->altModeMngrStatus->vdmInfo;

    vdmP->sopType   = (uint8_t)sopType;
    vdmP->vdoNumb   = CY_PDALTMODE_MNGR_NO_DATA;
    vdmP->vdmHeader = info->vdmHeader;    
    if ((info->vdo[sopType] != NULL) && (info->vdoNumb[sopType] != CY_PDALTMODE_MNGR_NO_DATA))
    {
        vdmP->vdoNumb = info->vdoNumb[sopType];
        /* Save received VDO */
        TIMER_CALL_MAP(Cy_PdUtils_MemCopy)((uint8_t *)vdmP->vdo, (const uint8_t *)info->vdo[sopType],((uint32_t)(info->vdoNumb[sopType]) << 2u));
    }
    if (info->uvdmSupp == false || ((vdmP->vdmHeader.std_vdm_hdr.vdmType == ((uint32_t)CY_PDSTACK_VDM_TYPE_STRUCTURED)) && (info->uvdmSupp)))
    {
        vdmP->vdmHeader.std_vdm_hdr.vdmType = (uint32_t)CY_PDSTACK_VDM_TYPE_STRUCTURED;

        if (sopType == CY_PD_SOP)
        {
            vdmP->vdmHeader.std_vdm_hdr.objPos = info->objPos;            
        }
        else
        {
            vdmP->vdmHeader.std_vdm_hdr.objPos = info->cblObjPos;
        }

        /* Set object position if custom Attention should be sent */
        if ((info->vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_ATTENTION) && (info->customAttObjPos))
        {
            vdmP->vdmHeader.std_vdm_hdr.objPos = info->vdmHeader.std_vdm_hdr.objPos;
        }
    }

    return 0x01U;
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_Mngr_GetVdmInfoVdo(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_mngr_info_t* info, cy_en_pd_sop_t sopType)
{
    cy_stc_pdaltmode_vdm_msg_info_t *vdmP = ptrAltModeContext->altModeMngrStatus->vdmInfo;

    if (info->vdmHeader.std_vdm_hdr.vdmType == (uint32_t)CY_PDSTACK_VDM_TYPE_STRUCTURED)
    {
        /* Copy object position to the VDM header */
        info->vdmHeader.std_vdm_hdr.objPos = vdmP->vdmHeader.std_vdm_hdr.objPos;
    }

    info->vdoNumb[sopType] = vdmP->vdoNumb;

    /* Copy received VDO to Alt Mode info */
    if ((vdmP->vdoNumb != 0u) && (info->vdo[sopType] != NULL))
    {
        /* Save Rec VDO */
        if (vdmP->vdoNumb <= info->vdoMaxNumb)
        {
            /* Save received VDO */
            TIMER_CALL_MAP(Cy_PdUtils_MemCopy)((uint8_t *)info->vdo[sopType], (const uint8_t *)vdmP->vdo,((uint32_t)(vdmP->vdoNumb) << 2u));
        }
    }
}

static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Mngr_AltModeMngrDeinit(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    uint8_t             altModeIdx;
    cy_stc_pdaltmode_mngr_info_t     *amInfoP = NULL;
    bool                waitForExit = false;

    /* Find and reset Alt Modes info structures */
    for (altModeIdx = 0; altModeIdx < ptrAltModeContext->altModeMngrStatus->altModesNumb; altModeIdx++)
    {
        amInfoP = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetModeInfo)(ptrAltModeContext, altModeIdx);
        if (amInfoP != NULL)
        {
            /* If current data role is UFP - set mode to idle */
            if (ptrAltModeContext->pdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP)
            {
                amInfoP->modeState = CY_PDALTMODE_STATE_IDLE;
                amInfoP->vdmHeader.std_vdm_hdr.cmd = (uint32_t)CY_PDSTACK_VDM_CMD_EXIT_MODE;
            }
            else
            {
                amInfoP->modeState = CY_PDALTMODE_STATE_EXIT;
                if (
                        (ptrAltModeContext->pdStackContext->dpmConfig.contractExist != false) &&
                        (CY_PDALTMODE_MNGR_IS_FLAG_CHECKED(ptrAltModeContext->altModeMngrStatus->amActiveModes, altModeIdx) != 0u)
                   )
                {
                    /* DFP: If PD contract is still present, make sure that EXIT_MODE request
                     * is sent to the UFP.
                     */
                    waitForExit = true;
                }
            }

            /* Exit from Alt Mode */
            if (amInfoP->cbk != NULL)
            {
                amInfoP->cbk(ptrAltModeContext);
            }

            if (!waitForExit)
            {
                /* Reset Alt Mode info */
                ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_ResetAltModeInfo)(amInfoP);
            }
        }
    }

    if (waitForExit)
    {
        return CY_PDALTMODE_VDM_TASK_ALT_MODE;
    }

    /* Reset Alt Mode manager info */
    ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_ResetInfo)(ptrAltModeContext);

#if (CCG_BB_ENABLE != 0)
    Cy_PdAltMode_Billboard_Disable(ptrAltModeContext, true);
    Cy_PdAltMode_Billboard_UpdateAllStatus(ptrAltModeContext, CY_PDALTMODE_BILLBOARD_ALT_MODE_STATUS_INIT_VAL);
#endif /* (CCG_BB_ENABLE != 0) */

    return CY_PDALTMODE_VDM_TASK_EXIT;
}

/******************** Common Alt Mode DFP and UFP functions *******************/

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))

ATTRIBUTES_ALT_MODE uint8_t Cy_PdAltMode_Mngr_IsSvidSupported(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t svid)
{
    uint8_t retIdx = CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED;

    if (
            (svid != CY_PDALTMODE_MNGR_NO_DATA)          &&
            (ptrAltModeContext->pdStackContext->port < ptrAltModeContext->noOfTypeCPorts) &&
            (ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetBaseAltModeSvidIdx)(ptrAltModeContext, svid) != CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED)
       )
    {
        retIdx = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModesConfigSvidIdx)(ptrAltModeContext, ptrAltModeContext->pdStackContext->dpmConfig.curPortType, svid);
    }

    return retIdx;    
}    

bool Cy_PdAltMode_Mngr_SetCustomSvid(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t svid)
{
    bool                 ret = false;
#if HPI_AM_SUPP    
#if DYNAMIC_CUSTOM_SVID_CHANGE_ENABLE
    uint8_t              cfgIdx;
    cy_stc_pdaltmode_mngr_info_t     *amInfoP;
    cy_stc_pdaltmode_alt_mode_reg_info_t *reg = &ptrAltModeContext->altModeMngrStatus->regInfo;
#endif /* DYNAMIC_CUSTOM_SVID_CHANGE_ENABLE */
    volatile uint16_t prevSvid = ptrAltModeContext->altModeAppStatus->customHpiSvid;

    /* Proceed only if custom SVID has been changed */
    if (svid != prevSvid)
    {
#if DYNAMIC_CUSTOM_SVID_CHANGE_ENABLE
        /* Check custom alt mode state */
        if (ptrAltModeContext->altModeMngrStatus->state == CY_PDALTMODE_MNGR_STATE_PROCESS)
        {
            cfgIdx   = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModeNumb)(port) - 1;
            amInfoP = ptrAltModeContext->altModeMngrStatus->altModeInfo[cfgIdx];
            /* Save new custom SVID */
            ptrAltModeContext->altModeAppStatus->customHpiSvid = svid;

            /* Check if Alt Mode is active */
            if (CY_PDALTMODE_MNGR_IS_FLAG_CHECKED(ptrAltModeContext->altModeMngrStatus->amActiveModes, cfgIdx) != false)
            {
                if (amInfoP->cbk != NULL)
                {
                    amInfoP->modeState = CY_PDALTMODE_STATE_EXIT;
                    amInfoP->cbk(ptrAltModeContext);
                    if (svid != CY_PDALTMODE_MNGR_NO_DATA)
                    {
                        /* Set flag that reset is required */
                        ptrAltModeContext->altModeMngrStatus->resetCustomMode = true;
                    }
                }
            }
            else if (svid == CY_PDALTMODE_MNGR_NO_DATA)
            {
                /* Empty handler to ignore empty SVID */
            }
            /* Check if mode is supported, then enter it. */
            else if (CY_PDALTMODE_MNGR_IS_FLAG_CHECKED(ptrAltModeContext->altModeMngrStatus->amSupportedModes, cfgIdx) != false)
            {
                if (amInfoP->cbk != NULL)
                {
                    cy_stc_pdaltmode_alt_mode_evt_t tmp_data;

                    /* Send command to change custom SVID */
                    amInfoP->evalAppCmd(port, tmp_data);
                    amInfoP->modeState = CY_PDALTMODE_STATE_INIT;
                    amInfoP->cbk(ptrAltModeContext);
                    CY_PDALTMODE_MNGR_SET_FLAG(ptrAltModeContext->altModeMngrStatus->amActiveModes, cfgIdx);
                }                
            }
            /* If no custom SVID was registered, then register it. */
            else 
            {
                reg->atchType = CY_PDALTMODE_MNGR_ATCH_TGT;
                amInfoP = gl_reg_alt_mode[ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetBaseAltModeSvidIdx)(port, CY_PDALTMODE_HPI_AM_SVID)].regAmPtr(ptrAltModeContext, reg);
                if ((reg->altModeId != CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED) && (amInfoP != NULL))
                {
                    amInfoP->objPos = 1;

                    /* Save pointer to Alt Mode info struct */
                    ptrAltModeContext->altModeMngrStatus->altModeInfo[cfgIdx] = amInfoP;

                    /* Set flag that Alt Mode could be run */
                    CY_PDALTMODE_MNGR_SET_FLAG(ptrAltModeContext->altModeMngrStatus->amSupportedModes, cfgIdx);
                    CY_PDALTMODE_MNGR_SET_FLAG(ptrAltModeContext->altModeMngrStatus->amActiveModes, cfgIdx);
                }
            }
        }
#endif /* DYNAMIC_CUSTOM_SVID_CHANGE_ENABLE */

        /* Save new custom SVID */
        ptrAltModeContext->altModeAppStatus->customHpiSvid = svid;
        ret = true;
    }
#endif /* HPI_AM_SUPP */    

    (void) ptrAltModeContext;
    (void) svid;

    return ret;
}

ATTRIBUTES_ALT_MODE uint16_t Cy_PdAltMode_Mngr_GetCustomSvid(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{    
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    uint16_t ret = CY_PDALTMODE_MNGR_NO_DATA;

    const cy_stc_pdaltmode_custom_alt_cfg_settings_t *ptr = ptrAltModeContext->custAltModeCfg;

    /* Check custom Alt Mode reg at first */
    if (ptrAltModeContext->altModeAppStatus->customHpiSvid != CY_PDALTMODE_MNGR_NO_DATA)
    {
        ret = ptrAltModeContext->altModeAppStatus->customHpiSvid;
    }  
    else if ((ptrPdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP) && (ptr->dfp_alt_mode_mask != CY_PDALTMODE_MNGR_NO_DATA))
    {
        /* Check config table custom Alt Mode*/
        ret = ptr->custom_alt_mode;
    }
    else
    {
    ; /* No action required - ; is optional */
    }

    return ret;
}

void Cy_PdAltMode_Mngr_SetAltModeMask(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t mask)
{
#if HPI_AM_SUPP    
    ptrAltModeContext->dfpAltModeMask = (uint8_t)(mask >> CY_PDALTMODE_MNGR_DFP_ALT_MODE_HPI_OFFSET);
    ptrAltModeContext->ufpAltModeMask = (uint8_t)(mask & CY_PDALTMODE_MNGR_UFP_ALT_MODE_HPI_MASK);

    /* Check if there is need to disable some Alt Modes and restart Alt Mode layer */
    if (ptrAltModeContext->altModeMngrStatus->amActiveModes != (ptrAltModeContext->altModeMngrStatus->amActiveModes & ptrAltModeContext->dfpAltModeMask))
    {
        Cy_PdAltMode_Mngr_LayerReset(ptrAltModeContext);
    }
#else
    (void) ptrAltModeContext;
    (void) mask;
#endif /* HPI_AM_SUPP */    
}

ATTRIBUTES_ALT_MODE uint16_t Cy_PdAltMode_Mngr_GetSvidFromIdx(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t idx)
{
    uint16_t* amVdoPtr;
    uint16_t  svid       = CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED;

    /* Look for the SVID with index position */
    amVdoPtr = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModesVdoInfo)(ptrAltModeContext, ptrAltModeContext->pdStackContext->dpmConfig.curPortType, idx);
    if (amVdoPtr != NULL)
    {
        svid = amVdoPtr[CY_PDALTMODE_MNGR_AM_SVID_CONFIG_OFFSET_IDX];
    }
    /* Return custom Alt Mode idx */
    else if ( 
            (ptrAltModeContext->altModeAppStatus->customHpiSvid != CY_PDALTMODE_MNGR_NO_DATA) &&
            (idx == (ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModeNumb)(ptrAltModeContext) - 1u))
            )
    {
        svid = ptrAltModeContext->altModeAppStatus->customHpiSvid;
    }
    else
    {
    ; /* No action required - ; is optional */
    }

    return svid;
}

ATTRIBUTES_ALT_MODE uint16_t* Cy_PdAltMode_Mngr_GetAltModesVdoInfo(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pd_port_type_t type, uint8_t idx)
{
    uint8_t mask  = (type != CY_PD_PRT_TYPE_UFP) ? ptrAltModeContext->dfpAltModeMask : ptrAltModeContext->ufpAltModeMask;

    uint16_t *ptr = ptrAltModeContext->baseAmInfo;

    uint8_t len   = (uint8_t)(ptrAltModeContext->altModeCfgLen >> 1u) - CY_PDALTMODE_MNGR_AM_SVID_CONFIG_OFFSET_START_IDX;
    uint8_t locIdx = 0;

    for (uint16_t i = 0; i < len; i += ((ptr[i] >> 1u) + 1u))
    {
        if ((locIdx == idx) && ((mask & (1u << idx)) != 0u))
        {
            return (ptr + i);
        }

        locIdx++;
    }

    return NULL;
}

ATTRIBUTES_ALT_MODE uint8_t Cy_PdAltMode_Mngr_GetAltModesConfigSvidIdx(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pd_port_type_t type, uint16_t svid)
{
    if (ptrAltModeContext->altModeAppStatus->customHpiSvid == svid)
    {
        /* Return last possible index */
        return (ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModeNumb)(ptrAltModeContext) - 1u);
    }

    uint8_t mask  = (type != CY_PD_PRT_TYPE_UFP) ? ptrAltModeContext->dfpAltModeMask : ptrAltModeContext->ufpAltModeMask;

    uint16_t locLen = 0;
    uint8_t  idx = 0, i = 0;

    uint16_t *ptr = ptrAltModeContext->baseAmInfo;
    uint8_t len   = (uint8_t)(ptrAltModeContext->altModeCfgLen >> 1u) - CY_PDALTMODE_MNGR_AM_SVID_CONFIG_OFFSET_START_IDX;
    
    while(i < len)
    {
        if ((svid == ptr[i + 1u]) && ((mask & (1u << idx)) != 0u))
        {
            return idx;
        }

        locLen = ptr[i] >> 1u;
        idx++;
        i += (uint8_t)locLen + 1u;
    }

    return CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED;
}

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.2', 1, 'Intentional usage of recursive function.');
ATTRIBUTES_ALT_MODE uint8_t Cy_PdAltMode_Mngr_GetBaseAltModeSvidIdx(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t svid)
{
    uint8_t idx;

    /* Look through all Alt Modes */
    for (idx = 0; idx < CY_PDALTMODE_MAX_SUPP_ALT_MODES; idx++)
    {
        /* If Alt Mode with given index is supported by CCG */
        if (ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetRegAmInfo)(idx)->svid == svid)
        {
            return idx;
        }
    }   

    /* Check if custom HPI Alt Mode is available */
    if (
            (ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetBaseAltModeSvidIdx)(ptrAltModeContext, CY_PDALTMODE_HPI_AM_SVID) != CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED) &&
            (ptrAltModeContext->altModeAppStatus->customHpiSvid == svid)
       )
    {
        return ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetBaseAltModeSvidIdx)(ptrAltModeContext, CY_PDALTMODE_HPI_AM_SVID);
    }

    return CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED;
}
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.2');

#else
void Cy_PdAltMode_Mngr_SetAltModeMask(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t mask)
{
    (void)ptrAltModeContext;
    (void)mask;
}

bool Cy_PdAltMode_Mngr_SetCustomSvid(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t svid)
{
    (void)ptrAltModeContext;
    (void)svid;

    return false;
}
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */

ATTRIBUTES_ALT_MODE uint8_t Cy_PdAltMode_Mngr_GetAltModeNumb(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    uint8_t count   = 0;

    if(ptrAltModeContext->altModeCfgLen != 0x0000U)
    {
        uint16_t *ptr = ptrAltModeContext->baseAmInfo;
        uint8_t len   = (uint8_t)(ptrAltModeContext->altModeCfgLen >> 1u) - CY_PDALTMODE_MNGR_AM_SVID_CONFIG_OFFSET_START_IDX;
        uint16_t locLen;
        uint8_t i = 0;
        
        while (i < len)
        {
            locLen = ptr[i] >> 1;
            count++;
            i += (uint8_t)locLen + 1u;
        }
    }

    if (ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetCustomSvid)(ptrAltModeContext) != CY_PDALTMODE_MNGR_NO_DATA)
    {
        count++;
    }

    return count;
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_Mngr_ResetAltModeInfo(cy_stc_pdaltmode_mngr_info_t *info)
{
    UTILS_CALL_MAP(Cy_PdUtils_MemSet)((uint8_t *)info, 0u, (uint32_t)sizeof(*info));
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_Mngr_ResetInfo(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    UTILS_CALL_MAP(Cy_PdUtils_MemSet)((uint8_t *)ptrAltModeContext->altModeMngrStatus, 0u, (uint32_t)sizeof(*ptrAltModeContext->altModeMngrStatus));
}

/******************* Alt Mode solution related functions ****************************/
/* Function to exit all active alternate modes. */

void Cy_PdAltMode_Mngr_ExitAll(cy_stc_pdaltmode_context_t *ptrAltModeContext, bool sendVdmExit, cy_pdstack_pd_cbk_t exitAllCbk)
{
    uint8_t altModeIdx;
    cy_stc_pdaltmode_mngr_info_t *amInfoP = NULL;
    cy_stc_pdaltmode_alt_mode_mngr_status_t *mngr_ptr  = ptrAltModeContext->altModeMngrStatus;
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    if (ptrPdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_DFP)
    {
        /* Configure the MUX to get out of Alt Modes*/
        if(ptrAltModeContext->altModeAppStatus->altModeEntered)
        {
            (void)Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_ISOLATE, 0, 0);
        }
    }

    if ((sendVdmExit != false) && (ptrAltModeContext->altModeAppStatus->altModeEntered))
    {
        mngr_ptr->exitAllCbk  = exitAllCbk;
        mngr_ptr->exitAllFlag = true;
    }

    /* For each Alternate Mode, initiate mode exit */
    for (altModeIdx = 0; altModeIdx < mngr_ptr->altModesNumb; altModeIdx++)
    {
        if (CY_PDALTMODE_MNGR_IS_FLAG_CHECKED(mngr_ptr->amActiveModes, altModeIdx) != 0x00U)
        {
            amInfoP = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetModeInfo)(ptrAltModeContext, altModeIdx);
            if (amInfoP != NULL)
            {

                /* Logically exit all Alt Modes */
                amInfoP->modeState = CY_PDALTMODE_STATE_EXIT;
                if (amInfoP->cbk != NULL)
                {
                    amInfoP->cbk (ptrAltModeContext);
                }
            }
        }
    }
}

/* Function to reset Alt Mode layer */
void Cy_PdAltMode_Mngr_LayerReset(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
    ptrAltModeContext->altModeAppStatus->skipMuxConfig = true;
    Cy_PdAltMode_VdmTask_MngrDeInit (ptrAltModeContext);
    Cy_PdAltMode_VdmTask_Enable (ptrAltModeContext);
    ptrAltModeContext->altModeAppStatus->skipMuxConfig = false;
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */
}

#if ((CY_HPI_ENABLED) || (APP_ALTMODE_CMD_ENABLE))

#if DFP_ALT_MODE_SUPP
static bool Cy_PdAltmode_Mngr_IsEnterAllowed(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t svid)
{
    bool    amAllowed         = false;
    uint8_t amNumb            = ptrAltModeContext->altModeMngrStatus->altModesNumb;
    uint8_t idx, idx2;
    uint16_t* amVdoPtr       = NULL;

    /* Find out if any Alt Mode is already entered */
    if (ptrAltModeContext->altModeMngrStatus->amActiveModes != CY_PDALTMODE_MNGR_NO_DATA)
    {
        for (idx = 0; idx < amNumb; idx++)
        {
            /* Find active Alt Modes */
            if ((CY_PDALTMODE_MNGR_IS_FLAG_CHECKED(ptrAltModeContext->altModeMngrStatus->amActiveModes, idx)) != 0u)
            {
                amAllowed = false;
                /* Get pointer to SVID configuration */
                amVdoPtr = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModesVdoInfo)(ptrAltModeContext, CY_PD_PRT_TYPE_DFP, idx);
                if (amVdoPtr != NULL)
                {
                    /* Find out if selected Alt Mode could be processed simultaneously with active Alt Modes */
                    for (idx2 = 0; idx2 < (amVdoPtr[CY_PDALTMODE_MNGR_AM_SVID_CONFIG_SIZE_IDX] >> 1); idx2++)
                    {
                        /* Check which alternate modes could be run simultaneously */
                        if (amVdoPtr[idx2 + CY_PDALTMODE_MNGR_AM_SVID_CONFIG_OFFSET_IDX] == svid)
                        {
                            amAllowed = true;
                            break;
                        }
                    }
                    /* If selected Alt Mode could not run simultaneously with active Alt Modes, return false */
                    if (amAllowed == false)
                    {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

#endif /* DFP_ALT_MODE_SUPP */

bool Cy_PdAltMode_Mngr_EvalAppAltModeCmd(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t *cmd, uint8_t *data)
{
#if (!ICL_ALT_MODE_HPI_DISABLED)
#if (!CCG_BACKUP_FIRMWARE)
    cy_stc_pdaltmode_alt_mode_evt_t  cmdInfo, cmdData;
    cy_stc_pdaltmode_mngr_info_t *amInfoP = NULL;
    uint8_t         altModeIdx;
    bool            found = false;

    /* Convert received cmd bytes as info and data */
    cmdInfo.val = CY_PDUTILS_MAKE_DWORD(cmd[3], cmd[2], cmd[1], cmd[0]);
    cmdData.val = CY_PDUTILS_MAKE_DWORD(data[3], data[2], data[1], data[0]);

#if ((CCG_PD_REV3_ENABLE) && (UFP_ALT_MODE_SUPP) && (UFP_MODE_DISC_ENABLE))
    /* Check if received app command is start discover process for UFP when PD 3.0 is supported */
    if (
            (ptrAltModeContext->altModeMngrStatus->pd3Ufp) &&
            (cmdInfo.alt_mode_event.dataRole == CY_PD_PRT_TYPE_UFP) &&
            (cmdInfo.alt_mode_event.altModeEvt == CY_PDALTMODE_CMD_RUN_UFP_DISC)
       )
    {
        /* Try to start Discovery process if VDM manager is not busy  */
        return ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_IsUfpDiscStarted)(ptrAltModeContext);
    }
#endif /* (CCG_PD_REV3_ENABLE) && (UFP_ALT_MODE_SUPP) && (UFP_MODE_DISC_ENABLE)) */

    /* Look for the Alternate Mode entry which matches the SVID and Alt Mode id. */
    for (altModeIdx = 0; altModeIdx < ptrAltModeContext->altModeMngrStatus->altModesNumb; altModeIdx++)
    {
        /* If mode is supported */
        if (CY_PDALTMODE_MNGR_IS_FLAG_CHECKED(ptrAltModeContext->altModeMngrStatus->amSupportedModes, altModeIdx) != 0u)
        {
            amInfoP = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetModeInfo)(ptrAltModeContext, altModeIdx);
            if ((amInfoP->vdmHeader.std_vdm_hdr.svid == cmdInfo.alt_mode_event.svid) &&
                    (amInfoP->altModeId  == cmdInfo.alt_mode_event.altMode))
            {
                found = true;
                break;
            }
        }
    }

#if DFP_ALT_MODE_SUPP
    if ((ptrAltModeContext->pdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP) && (cmdInfo.alt_mode_event.dataRole != (uint32_t)CY_PD_PRT_TYPE_UFP))
    {
        if (cmdInfo.alt_mode_event.altModeEvt == (uint32_t)CY_PDALTMODE_SET_TRIGGER_MASK)
        {
            ptrAltModeContext->altModeAppStatus->altModeTrigMask = (uint8_t)cmdData.alt_mode_event_data.evtData;
            return true;
        }
        /* Check if Enter command and trigger is set */
        if (cmdInfo.alt_mode_event.altModeEvt == (uint32_t)CY_PDALTMODE_CMD_ENTER)
        {
            /* If found an Alternate Mode entry for this {SVID, ID} pair */
            if (found)
            {
                if (
                        (amInfoP->cbk != NULL) && 
                        (amInfoP->isActive == false) &&
                        (Cy_PdAltmode_Mngr_IsEnterAllowed(ptrAltModeContext, (uint16_t)amInfoP->vdmHeader.std_vdm_hdr.svid) != false)
                   )
                {
                    /* Set flag as active mode */
                    CY_PDALTMODE_MNGR_SET_FLAG(ptrAltModeContext->altModeMngrStatus->amActiveModes, altModeIdx);
                    CY_PDALTMODE_MNGR_REMOVE_FLAG(ptrAltModeContext->altModeMngrStatus->amExitedModes, altModeIdx);

                    /* Initializes Alt Mode */
                    amInfoP->modeState = CY_PDALTMODE_STATE_INIT;
                    amInfoP->cbk(ptrAltModeContext);

                    /* Goto Alt Mode processing */
                    ptrAltModeContext->altModeMngrStatus->state = CY_PDALTMODE_MNGR_STATE_PROCESS;
                    return true;
                }
            }
            return false;
        }
    }
#endif /* DFP_ALT_MODE_SUPP */

    if ((found) && (amInfoP->isActive == true) && (amInfoP->modeState == CY_PDALTMODE_STATE_IDLE) &&
            (cmdInfo.alt_mode_event.dataRole == (uint32_t)(ptrAltModeContext->pdStackContext->dpmConfig.curPortType)))
    {
        /* If received cmd is specific Alt Mode command */
        if (cmdInfo.alt_mode_event.altModeEvt == (uint32_t)CY_PDALTMODE_CMD_SPEC)
        {
            return (amInfoP->evalAppCmd(ptrAltModeContext, cmdData));
        }
#if DFP_ALT_MODE_SUPP
        else if (cmdInfo.alt_mode_event.altModeEvt == (uint32_t)CY_PDALTMODE_CMD_EXIT)
        {
            if (ptrAltModeContext->pdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP)
            {
                amInfoP->modeState = CY_PDALTMODE_STATE_EXIT;
                if (amInfoP->cbk != NULL)
                {
                    amInfoP->cbk(ptrAltModeContext);
                    return true;
                }
            }
        }
        else
        {
           /* No action required - ; is optional */
        }
#endif /* DFP_ALT_MODE_SUPP */
    }
#endif /* (!CCG_BACKUP_FIRMWARE) */
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */
    return false;
}

#endif /* ((CY_HPI_ENABLED) || (APP_ALTMODE_CMD_ENABLE)) */



ATTRIBUTES_ALT_MODE const uint32_t* Cy_PdAltMode_Mngr_FormAltModeEvent(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t svid, uint8_t amIdx, cy_en_pdaltmode_app_evt_t evt, uint32_t data)
{
    cy_stc_pdaltmode_alt_mode_evt_t temp;

    temp.alt_mode_event.svid                           = (uint32_t)svid;
    temp.alt_mode_event.altMode                       = (uint32_t)amIdx;
    temp.alt_mode_event.altModeEvt                   = (uint32_t)evt;
    temp.alt_mode_event.dataRole = (uint32_t)ptrAltModeContext->pdStackContext->dpmConfig.curPortType;
    ptrAltModeContext->altModeMngrStatus->appEvtData[CY_PDALTMODE_MNGR_ALT_MODE_EVT_IDX]      = temp.val;
    ptrAltModeContext->altModeMngrStatus->appEvtData[CY_PDALTMODE_MNGR_ALT_MODE_EVT_DATA_IDX] = CY_PDALTMODE_MNGR_NO_DATA;

    if (data != CY_PDALTMODE_MNGR_NO_DATA)
    {
        ptrAltModeContext->altModeMngrStatus->appEvtData[CY_PDALTMODE_MNGR_ALT_MODE_EVT_DATA_IDX] = data;
    }

    return ptrAltModeContext->altModeMngrStatus->appEvtData;
}

ATTRIBUTES_ALT_MODE cy_stc_pdaltmode_mngr_info_t* Cy_PdAltMode_Mngr_GetModeInfo(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t altModeIdx)
{
    return ptrAltModeContext->altModeMngrStatus->altModeInfo[altModeIdx];
}

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
static void Cy_PdAltMode_Mngr_VdmHandle(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_mngr_info_t *am_info, const cy_stc_pdstack_pd_packet_t *vdm)
{
    /* Check if it is attention message which comes while Alt Mode state machine is busy */
    if (
            (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_ATTENTION) &&
            ( 
             (am_info->modeState != CY_PDALTMODE_STATE_IDLE)
#if MUX_UPDATE_PAUSE_FSM
             || (ptrAltModeContext->altModeAppStatus->muxStat == CY_PDALTMODE_APP_MUX_STATE_BUSY)
#endif /* MUX_UPDATE_PAUSE_FSM */
            )
       )
    {
        /* Save attention for the further processing */
        ptrAltModeContext->altModeMngrStatus->attHeader = vdm->dat[CY_PD_VDM_HEADER_IDX];
        ptrAltModeContext->altModeMngrStatus->attVdo = vdm->dat[CY_PDALTMODE_MNGR_VDO_START_IDX];
        ptrAltModeContext->altModeMngrStatus->attAltMode = am_info;
        (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrAltModeContext->pdStackContext->ptrTimerContext, ptrAltModeContext->pdStackContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrAltModeContext->pdStackContext, (uint16_t)CY_PDALTMODE_ALT_MODE_ATT_CBK_TIMER),
                    CY_PDALTMODE_APP_POLL_PERIOD, ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_AttentionCbk));
        return;
    }

    /* Save Header */
    am_info->vdmHeader.val = vdm->dat[CY_PD_VDM_HEADER_IDX].val;
    am_info->vdoNumb[CY_PD_SOP]  = vdm->len - CY_PDALTMODE_MNGR_VDO_START_IDX;

    if ((vdm->len > CY_PDALTMODE_MNGR_VDO_START_IDX) && (vdm->len <= (am_info->vdoMaxNumb + CY_PDALTMODE_MNGR_VDO_START_IDX)))
    {
        /* Save received VDO */
        TIMER_CALL_MAP(Cy_PdUtils_MemCopy)((uint8_t*)am_info->vdo[CY_PD_SOP], (uint8_t*)&(vdm->dat[CY_PDALTMODE_MNGR_VDO_START_IDX]),
                ((uint32_t)(am_info->vdoNumb[CY_PD_SOP]) * (uint32_t)CY_PD_WORD_SIZE));
    }

    /* Run ufp alt mode cbk */
    am_info->modeState = CY_PDALTMODE_STATE_IDLE;
    if(NULL != am_info->cbk)
    {
        am_info->cbk(ptrAltModeContext);
    }
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_Mngr_AttentionCbk(cy_timer_id_t id, void* context)
{
    (void)id;
    cy_stc_pdstack_context_t * ptrPdStackContext = (cy_stc_pdstack_context_t *) context;
    cy_stc_pdaltmode_context_t * ptrAltModeContext = (cy_stc_pdaltmode_context_t *) ptrPdStackContext->ptrAltModeContext;

    cy_stc_pdaltmode_alt_mode_mngr_status_t *am = ptrAltModeContext->altModeMngrStatus;

    if ((am->attAltMode != NULL) && (am->attAltMode->isActive))
    {
        am->runAttFlag = 0x01U;
    }    
}
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */
#if ( CCG_UCSI_ENABLE && UCSI_ALT_MODE_ENABLED )
        
uint32_t Cy_PdAltMode_Mngr_GetActiveAltModeMask(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    return ptrAltModeContext->altModeMngrStatus->amActiveModes;
}

uint32_t Cy_PdAltMode_Mngr_GetSuppAltModes(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    return ptrAltModeContext->altModeMngrStatus->amSupportedModes;
}

void Cy_PdAltMode_Mngr_SetActiveAltModeState(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t altModeIdx)
{
    /* Set flag as active mode */
    CY_PDALTMODE_MNGR_SET_FLAG(ptrAltModeContext->altModeMngrStatus->amActiveModes, altModeIdx);
    CY_PDALTMODE_MNGR_REMOVE_FLAG(ptrAltModeContext->altModeMngrStatus->amExitedModes, altModeIdx);
}
#endif /*CCG_UCSI_ENABLE && UCSI_ALT_MODE_ENABLED*/


#if UFP_ALT_MODE_SUPP
#if (CCG_BB_ENABLE != 0)    
void Cy_PdAltMode_Mngr_BillboardUpdate(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t amIdx, bool bbStat)
{
    if (Cy_PdAltMode_Billboard_IsPresent(ptrAltModeContext) != false)
    {
        /* Disable AME timeout timer */
        TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrAltModeContext->pdStackContext->ptrTimerContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrAltModeContext->pdStackContext, CY_PDALTMODE_AME_TIMEOUT_TIMER));
        if (bbStat == false)
        {
            /* Update BB Alt Mode status register as Error */
            Cy_PdAltMode_Billboard_AltStatusUpdate(ptrAltModeContext, amIdx, CY_PDALTMODE_BILLBOARD_ALT_MODE_STAT_UNSUCCESSFUL);

            /* Enable BB controller */
            Cy_PdAltMode_Billboard_Enable(ptrAltModeContext, CY_PDALTMODE_BILLBOARD_CAUSE_AME_FAILURE);
        }
        else
        {
            /* Update BB Alt Mode status register as successful config */
            Cy_PdAltMode_Billboard_AltStatusUpdate(ptrAltModeContext, amIdx, CY_PDALTMODE_BILLBOARD_ALT_MODE_STAT_SUCCESSFUL);

            /* Enable BB controller */
            Cy_PdAltMode_Billboard_Enable(ptrAltModeContext, CY_PDALTMODE_BILLBOARD_CAUSE_AME_SUCCESS);
        }
    }
}
#endif /* (CCG_BB_ENABLE != 0) */
 
ATTRIBUTES_ALT_MODE bool Cy_PdAltMode_Mngr_GetModesVdoInfo(cy_stc_pdaltmode_context_t * ptrAltModeContext, uint16_t svid, cy_pd_pd_do_t **tempP, uint8_t *no_of_vdo)
{
    uint8_t dModeRespSizeTotal;
    uint8_t dModeRespSize;
    cy_pd_pd_do_t *header;
    uint8_t* respP = (uint8_t *)ptrAltModeContext->appStatusContext->vdmModeResp;

    /* Parse all responses based on SVID */
    dModeRespSizeTotal = ptrAltModeContext->appStatusContext->vdmModeDataLen;

    /* If size is less than or equal to 4, return NACK */
    if (dModeRespSizeTotal <= 0x04U)
    {
        return false;
    }
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 11.3', 1, 'Intentional type casting to reduce flash (code space) consumption');
    while (dModeRespSizeTotal != 0x00U)
    {
        /* Get the size of packet. */
        dModeRespSize = *respP;

        /* Read the SVID from header */
        header = (cy_pd_pd_do_t *)(respP + 4);
        if (header->std_vdm_hdr.svid == svid)
        {
            *no_of_vdo = ((dModeRespSize - 0x04U) >> 0x02U);
            *tempP = header;
            return true;
        }
        /* Move to next packet */
        respP += dModeRespSize;
        dModeRespSizeTotal -= dModeRespSize;
    }
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 11.3');  
    return false;
}

static bool Cy_PdAltMode_Mngr_UfpRegAltMode(cy_stc_pdaltmode_context_t *ptrAltModeContext, const cy_stc_pdstack_pd_packet_t *vdm)
{
    uint8_t              svidIdx, vdoNumb, altModeIdx = 0;
    cy_pd_pd_do_t             *dobj;
    cy_stc_pdaltmode_mngr_info_t     *amInfoP = NULL;
    cy_stc_pdaltmode_alt_mode_reg_info_t *reg       = &(ptrAltModeContext->altModeMngrStatus->regInfo);
    uint32_t             svid      = vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid;
    uint32_t             objPos   = vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.objPos;

    /* Check if any Alt Mode is supported by DFP */
    if (ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModeNumb)(ptrAltModeContext) != 0u)
    {
        /* Get index of related SVID register function */
        svidIdx = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetBaseAltModeSvidIdx)(ptrAltModeContext, (uint16_t)svid);

        /* If SVID processing supported by CCG */
        if (svidIdx != CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED)
        {
            reg->dataRole = (uint8_t)CY_PD_PRT_TYPE_UFP;

            /* Get SVID related VDOs and number of VDOs */
            if(ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetModesVdoInfo)(ptrAltModeContext, (uint16_t)svid, &dobj, &vdoNumb) == false)
            {
                return false;
            }
            /* Check if object position is not greater than VDO number */
            if (objPos < vdoNumb)
            {
                /* Save Disc mode VDO and object position */
                reg->svidVdo = dobj[objPos];

                /* Check if UFP support attached target Alt Modes */
                amInfoP = regAltModeInfo[svidIdx].regAmPtr(ptrAltModeContext, reg);

                /* If VDO relates to any of supported Alt Modes */
                if ((ptrAltModeContext->altModeMngrStatus->regInfo.altModeId != CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED) && (amInfoP != NULL))
                {
                    /* If Alternate Modes are discovered and could be run */
                    /* Get index of Alt Mode in the compatibility table */
                    altModeIdx = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModesConfigSvidIdx)(ptrAltModeContext, CY_PD_PRT_TYPE_UFP, (uint16_t)svid);
                    if (altModeIdx != CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED)
                    {
                        /* Save Alt Mode ID and obj position */
                        amInfoP->altModeId = reg->altModeId;
                        amInfoP->objPos = (uint8_t)objPos;

                        /* Save pointer to Alt Mode info struct */
                        ptrAltModeContext->altModeMngrStatus->altModeInfo[altModeIdx] = amInfoP;
                        /* Set flag that Alt Mode could be run */
                        CY_PDALTMODE_MNGR_SET_FLAG(ptrAltModeContext->altModeMngrStatus->amSupportedModes, altModeIdx);
                        return true;
                    }
                }
            }
#if (CCG_BB_ENABLE != 0)               
            else if (Cy_PdAltMode_Billboard_IsPresent(ptrAltModeContext) != false)
            {
                /* Go through all Alt Modes */
                for (altModeIdx = 0; altModeIdx < ptrAltModeContext->altModeMngrStatus->altModesNumb; altModeIdx++)
                {
                    amInfoP = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetModeInfo)(ptrAltModeContext, altModeIdx);
                    /* If Alt mode with corresponded SVID is already active then don't notify BB device */
                    if (
                            (amInfoP != NULL) && 
                            (amInfoP->vdmHeader.std_vdm_hdr.svid == vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid) &&
                            (amInfoP->isActive == true)
                       )
                    {
                        return false;
                    }                     
                    /*
                     * If Alt mode with corresponded SVID is not activated, and 
                     * Enter Mode command has object position which is not supported 
                     * by CCG, then enumerate BB.
                     */
#if BILLBOARD_1_2_2_SUPPORT
                    Cy_PdAltMode_Mngr_BillboardUpdate(ptrAltModeContext, (svidIdx + (uint8_t)ptrAltModeContext->altModeAppStatus->usb4Supp), false);
#else
                    Cy_PdAltMode_Mngr_BillboardUpdate(ptrAltModeContext, svidIdx, false);
#endif /* BILLBOARD_1_2_2_SUPPORT */
                }
            }
            else
            {
                /* No statement */
            }
#endif /* (CCG_BB_ENABLE != 0) */            
        }
    }

    return false;
}

static bool Cy_PdAltMode_Mngr_UfpEnterAltMode(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_mngr_info_t *amInfoP, const cy_stc_pdstack_pd_packet_t *vdm, uint8_t amIdx)
{
    /* Process VDM */
    Cy_PdAltMode_Mngr_VdmHandle(ptrAltModeContext, amInfoP, vdm);

    if (amInfoP->modeState != CY_PDALTMODE_STATE_FAIL)
    {
        /* Set mode as active */
        amInfoP->isActive = true;

        /* Notify APP layer that Alt Mode has been entered */
        ptrAltModeContext->altModeAppStatus->altModeEntered = true;

#if CY_PD_USB4_SUPPORT_ENABLE
        /* Disable USB4 timeout timer */
        TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrAltModeContext->pdStackContext->ptrTimerContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrAltModeContext->pdStackContext, CY_PDALTMODE_USB4_TIMEOUT_TIMER));
#endif /* CY_PD_USB4_SUPPORT_ENABLE */

        ptrAltModeContext->pdStackContext->ptrAppCbk->app_event_handler (ptrAltModeContext->pdStackContext, APP_EVT_ALT_MODE,
                ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_FormAltModeEvent)(ptrAltModeContext, (uint16_t)amInfoP->vdmHeader.std_vdm_hdr.svid,
                    amInfoP->altModeId, CY_PDALTMODE_EVT_ALT_MODE_ENTERED, CY_PDALTMODE_MNGR_NO_DATA));

#if (CCG_BB_ENABLE != 0)
        /* Update BB status success */
#if BILLBOARD_1_2_2_SUPPORT
        Cy_PdAltMode_Mngr_BillboardUpdate(ptrAltModeContext, (amIdx + (uint8_t)ptrAltModeContext->altModeAppStatus->usb4Supp), true);
#else
        Cy_PdAltMode_Mngr_BillboardUpdate(ptrAltModeContext, amIdx, true);
#endif /* BILLBOARD_1_2_2_SUPPORT */
#endif /* (CCG_BB_ENABLE != 0) */

        /* Set flag that Alt Mode could be processed */
        CY_PDALTMODE_MNGR_SET_FLAG(ptrAltModeContext->altModeMngrStatus->amActiveModes, amIdx);
        return true;
    }

#if (CCG_BB_ENABLE != 0)
    /* Update BB status not success */
#if BILLBOARD_1_2_2_SUPPORT
    Cy_PdAltMode_Mngr_BillboardUpdate(ptrAltModeContext, (amIdx + (uint8_t)ptrAltModeContext->altModeAppStatus->usb4Supp), false);
#else
    Cy_PdAltMode_Mngr_BillboardUpdate(ptrAltModeContext, amIdx, false);
#endif /* BILLBOARD_1_2_2_SUPPORT */
#endif /* (CCG_BB_ENABLE != 0) */
    return false;
}

static bool Cy_PdAltMode_Mngr_IsModeActivated(cy_stc_pdaltmode_context_t *ptrAltModeContext, const cy_stc_pdstack_pd_packet_t *vdm)
{
    uint8_t      idx;

    /* If any Alt Mode is already registered */
    if (ptrAltModeContext->altModeMngrStatus->amSupportedModes != CY_PDALTMODE_MNGR_NONE_MODE_MASK)
    {
        for (idx = 0; idx < ptrAltModeContext->altModeMngrStatus->altModesNumb; idx++)
        {
            /* Try to find Alt Mode among supported Alt Modes */
            if (
                    (ptrAltModeContext->altModeMngrStatus->altModeInfo[idx] != NULL) &&
                    (ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetModeInfo)(ptrAltModeContext, idx)->vdmHeader.std_vdm_hdr.svid ==
                     vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid) &&
                    (ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetModeInfo)(ptrAltModeContext, idx)->objPos ==
                     vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.objPos)
               )
            {
                /* Return true if ufp alt mode is already registered in Alt Mode manager */
                return true;
            }
        }
    }

    /* Register Alt Mode and check possibility of Alt Mode entering */
    return Cy_PdAltMode_Mngr_UfpRegAltMode(ptrAltModeContext, vdm);
}

#endif /* UFP_ALT_MODE_SUPP */

cy_en_pdstack_std_vdm_cmd_type_t Cy_PdAltMode_Mngr_EvalRecVdm(cy_stc_pdaltmode_context_t *ptrAltModeContext, const cy_stc_pdstack_pd_packet_t *vdm)
{
#if (!CCG_BACKUP_FIRMWARE)
    uint8_t         idx;
#if UFP_ALT_MODE_SUPP
    uint8_t         idx2;
    bool            enterFlag   = false;
    bool    amAllowed         = false;
#endif /* UFP_ALT_MODE_SUPP */

    cy_stc_pdaltmode_mngr_info_t *amInfoP   = NULL;
    cy_pd_pd_do_t   vdmHeader   = vdm->dat[CY_PD_VDM_HEADER_IDX];
    vdm_resp_t*     vdm_response = &(ptrAltModeContext->appStatusContext->vdmResp);

    if (vdmHeader.std_vdm_hdr.vdmType == (uint32_t)CY_PDSTACK_VDM_TYPE_STRUCTURED)
    {
        /* Discovery commands not processed by Alt Modes manager */
        if (vdmHeader.std_vdm_hdr.cmd < (uint32_t)CY_PDSTACK_VDM_CMD_ENTER_MODE)
        {
            return CY_PDSTACK_CMD_TYPE_RESP_NAK;
        }
        /* Save number of available Alt Modes */
        ptrAltModeContext->altModeMngrStatus->altModesNumb = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModeNumb)(ptrAltModeContext);

#if UFP_ALT_MODE_SUPP
        /* If Enter mode cmd */
        if (
                (vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_ENTER_MODE) &&
                (ptrAltModeContext->pdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP)
           )
        {
            if (Cy_PdAltMode_Mngr_IsModeActivated(ptrAltModeContext, vdm) == true)
            {

                enterFlag = true;
            }

            /* Send an ACK for ENTER_VPRO mode command if USB4 mode is active */
#if VPRO_WITH_USB4_MODE
            if(ptrAltModeContext->pdStackContext->port == TYPEC_PORT_0_IDX)
            {
                /**< Index of VDM header data object in a received message */
                if(
                        (vdmHeader.std_vdm_hdr.svid == CY_PDALTMODE_TBT_SVID) &&
                        (vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_ENTER_MODE) &&
                        (vdmHeader.std_vdm_hdr.objPos == 1u) &&
                        /* Enter mode command with Vpro mode */
                        (vdm->dat[CY_PD_ID_HEADER_IDX].tbt_vdo.vproDockHost == 1u) &&
                        (vdm->dat[CY_PD_ID_HEADER_IDX].tbt_vdo.intelMode == 2u)
                  )
                {
#if RIDGE_SLAVE_ENABLE
                    Cy_PdAltMode_RidgeSlave_VproStatusUpdateEnable(ptrAltModeContext, true);
#endif
                    return CY_PDSTACK_CMD_TYPE_RESP_ACK;
                }
            }
#endif /* #if VPRO_WITH_USB4_MODE */
        }
#endif /* UFP_ALT_MODE_SUPP */
    }
    /* Go through all Alt Modes */
    for (idx = 0; idx < ptrAltModeContext->altModeMngrStatus->altModesNumb; idx++)
    {
        amInfoP = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetModeInfo)(ptrAltModeContext, idx);

        /* Check if received command processing is allowed */
        if (
                (amInfoP != NULL) && (amInfoP->vdmHeader.std_vdm_hdr.svid == vdmHeader.std_vdm_hdr.svid) &&
                (
                   (((amInfoP->objPos == vdmHeader.std_vdm_hdr.objPos) && (amInfoP->vdmHeader.std_vdm_hdr.cmd <= (uint32_t)CY_PDSTACK_VDM_CMD_ATTENTION)) ||
                     (amInfoP->vdmHeader.std_vdm_hdr.cmd > (uint32_t)CY_PDSTACK_VDM_CMD_ATTENTION)) ||
#if CUSTOM_OBJ_POSITION
                 (amInfoP->customAttObjPos == true) || 
#endif /* CUSTOM_OBJ_POSITION */
                 ((amInfoP->uvdmSupp == true) && 
                  (vdmHeader.std_vdm_hdr.vdmType == (uint32_t)CY_PDSTACK_VDM_TYPE_UNSTRUCTURED))
                )
           )
        {
#if UFP_ALT_MODE_SUPP
            /* If Enter mode cmd */
            if (enterFlag == true)
            {
                /* If Alt Mode is already active, then ACK */
                if (amInfoP->isActive == true)
                {
                    return CY_PDSTACK_CMD_TYPE_RESP_ACK;
                }

                /* If all Alt Modes not active and is just entered  */
                if (ptrAltModeContext->altModeMngrStatus->amActiveModes == CY_PDALTMODE_MNGR_NONE_MODE_MASK)
                {
                    return ((Cy_PdAltMode_Mngr_UfpEnterAltMode(ptrAltModeContext, amInfoP, vdm, idx) == true) ? CY_PDSTACK_CMD_TYPE_RESP_ACK : CY_PDSTACK_CMD_TYPE_RESP_NAK);
                }
                else
                {
                    /* Check Alt Mode in ufp consistent table  */
                    for (idx2 = 0; idx2 < ptrAltModeContext->altModeMngrStatus->altModesNumb; idx2++)
                    {
                        /* Find table index of any active Alt Mode */
                        if ((CY_PDALTMODE_MNGR_IS_FLAG_CHECKED(ptrAltModeContext->altModeMngrStatus->amActiveModes, idx2) != 0u))
                        {
                            uint16_t* amVdoPtr = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModesVdoInfo)(ptrAltModeContext, CY_PD_PRT_TYPE_DFP, idx2);
                            if (amVdoPtr != NULL)
                            {
                                uint8_t idx3;

                                /* Find out if selected Alt Mode could be processed simultaneously with active Alt Modes */
                                for (idx3 = 0; idx3 < (amVdoPtr[CY_PDALTMODE_MNGR_AM_SVID_CONFIG_SIZE_IDX] >> 1); idx3++)
                                {
                                    /* Check which Alternate Modes could be run simultaneously */
                                    if (amVdoPtr[idx3 + CY_PDALTMODE_MNGR_AM_SVID_CONFIG_OFFSET_IDX] == vdmHeader.std_vdm_hdr.svid)
                                    {
                                        amAllowed = true;
                                        break;
                                    }
                                }
                                if(amAllowed == false)
                                {
                                    /* If selected Alt Mode could not run simultaneously with active Alt Modes, return false */
                                    return CY_PDSTACK_CMD_TYPE_RESP_NAK;
                                }
                            }
                        }             
                    }
                    /* Try to enter Alt Mode */
                    return ((Cy_PdAltMode_Mngr_UfpEnterAltMode(ptrAltModeContext, amInfoP, vdm, idx) == true) ? CY_PDSTACK_CMD_TYPE_RESP_ACK : CY_PDSTACK_CMD_TYPE_RESP_NAK);
                }
            }
            /* Any other received command */
            else
#endif /* UFP_ALT_MODE_SUPP */
            {
                /* Check if Alt Mode is active */
                if (amInfoP->isActive == false)
                {
                    return CY_PDSTACK_CMD_TYPE_RESP_NAK;
                }
                else if (
                            (amInfoP->modeState != CY_PDALTMODE_STATE_IDLE) &&
                             /*
                              * In case a VDM is received as UFP, honor the request if you are waiting for
                              * a response or attention message to be sent out. Do not do this in case you
                              * are DFP, as a DFP state machine is driven from the DUT.
                              */
                             (
#if DFP_ALT_MODE_SUPP
                              (ptrAltModeContext->pdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_DFP) ||
#endif /* DFP_ALT_MODE_SUPP */
                              (amInfoP->modeState != CY_PDALTMODE_STATE_WAIT_FOR_RESP)
                             )
                             && (vdmHeader.std_vdm_hdr.cmd != (uint32_t)CY_PDSTACK_VDM_CMD_ATTENTION)
                         )
                {
                    return CY_PDSTACK_CMD_TYPE_RESP_BUSY;
                }
                else
                {
                    /* Do nothing */
                }

#if CUSTOM_OBJ_POSITION
                /* Save custom attention object position if needed */
                if (
                        (vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_ATTENTION) &&
                        (amInfoP->customAttObjPos)    &&
                        (vdmHeader.std_vdm_hdr.vdmType == (uint32_t)CY_PDSTACK_VDM_TYPE_STRUCTURED)
                   )
                {
                    amInfoP->vdmHeader.std_vdm_hdr.objPos = vdmHeader.std_vdm_hdr.objPos;
                }
#endif /* CUSTOM_OBJ_POSITION */

                /* Process VDM */
                Cy_PdAltMode_Mngr_VdmHandle(ptrAltModeContext, amInfoP, vdm);

                /* If command processed is successful */
                if ((amInfoP->modeState != CY_PDALTMODE_STATE_FAIL) && (amInfoP->modeState != CY_PDALTMODE_STATE_PAUSE))
                {
                    /* Copy VDM header to respond buffer if Unstructured*/
                    if (amInfoP->vdmHeader.std_vdm_hdr.vdmType == (uint32_t)CY_PDSTACK_VDM_TYPE_UNSTRUCTURED)
                    {
                        vdm_response->respBuf[CY_PD_VDM_HEADER_IDX] = amInfoP->vdmHeader;
                    }
                    /* Set number of data objects */
                    vdm_response->doCount = amInfoP->vdoNumb[CY_PD_SOP] + CY_PDALTMODE_MNGR_VDO_START_IDX;
                    if (amInfoP->vdoNumb[CY_PD_SOP] != CY_PDALTMODE_MNGR_NO_DATA)
                    {
                        /* If VDO resp is needed */
                        TIMER_CALL_MAP(Cy_PdUtils_MemCopy)((uint8_t*) & (vdm_response->respBuf[CY_PDALTMODE_MNGR_VDO_START_IDX]),
                                (const uint8_t*) amInfoP->vdo[CY_PD_SOP],
                                ((uint32_t)(amInfoP->vdoNumb[CY_PD_SOP]) * CY_PD_WORD_SIZE));
                    }
                    return CY_PDSTACK_CMD_TYPE_RESP_ACK;
                }
                else if(amInfoP->modeState == CY_PDALTMODE_STATE_PAUSE)
                {
                    amInfoP->modeState = CY_PDALTMODE_STATE_IDLE;
                    return CY_PDSTACK_CMD_TYPE_RESP_BUSY;
                }
                else
                {
                    /* Set Alt Mode state as idle */
                    amInfoP->modeState = CY_PDALTMODE_STATE_IDLE;
                    return CY_PDSTACK_CMD_TYPE_RESP_NAK;
                }
            }
        }

#if UFP_ALT_MODE_SUPP
        /* If exit all modes */
        if (
                (vdmHeader.std_vdm_hdr.vdmType == (uint32_t)CY_PDSTACK_VDM_TYPE_STRUCTURED)   &&
                (vdmHeader.std_vdm_hdr.objPos == CY_PDALTMODE_MNGR_EXIT_ALL_MODES)                    &&
                (vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_EXIT_MODE)         &&
                (amInfoP != NULL) &&
                (amInfoP->vdmHeader.std_vdm_hdr.svid == vdmHeader.std_vdm_hdr.svid)   &&
                (amInfoP->isActive)
           )
        {
            /* Save cmd */
            amInfoP->vdmHeader.std_vdm_hdr.cmd = CY_PDALTMODE_MNGR_EXIT_ALL_MODES;
            if (amInfoP->cbk != NULL)
            {
                /* Run ufp alt mode cbk */
                amInfoP->cbk(ptrAltModeContext);
            }
            return CY_PDSTACK_CMD_TYPE_RESP_ACK;
        }
#endif /* UFP_ALT_MODE_SUPP */
    }
#endif /* (!CCG_BACKUP_FIRMWARE) */
    return CY_PDSTACK_CMD_TYPE_RESP_NAK;
}

uint32_t Cy_PdAltMode_Mngr_GetStatus(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    uint32_t ret = CY_PDALTMODE_MNGR_NO_DATA;
#if DFP_ALT_MODE_SUPP
    cy_stc_pdaltmode_mngr_info_t *amInfoP = NULL;
    uint8_t          altModeIdx;

    if (ptrAltModeContext->altModeMngrStatus->state == CY_PDALTMODE_MNGR_STATE_PROCESS)
    {
        /* Check each Alternate Mode */
        for (altModeIdx = 0; altModeIdx < ptrAltModeContext->altModeMngrStatus->altModesNumb; altModeIdx++)
        {
            if (CY_PDALTMODE_MNGR_IS_FLAG_CHECKED(ptrAltModeContext->altModeMngrStatus->amActiveModes, altModeIdx) != 0x0U)
            {
                amInfoP = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetModeInfo)(ptrAltModeContext, altModeIdx);
                if (amInfoP != NULL)
                {
                    if (amInfoP->isActive != false)
                    {
                        /* Set Alt Modes which were entered */
                        CY_PDALTMODE_MNGR_SET_FLAG(ret, altModeIdx);
                    }
                }
            }
        }
        /* Mode discovery complete */
        ret |= CY_PDALTMODE_APP_DISC_COMPLETE_MASK;
    }
    /* Check if USB4 mode is entered */
    if (ptrAltModeContext->altModeAppStatus->usb4Active != false)
    {
        ret |= CY_PDALTMODE_APP_USB4_ACTIVE_MASK;
    }
#endif /* DFP_ALT_MODE_SUPP */

    return ret;
}
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */

ATTRIBUTES_ALT_MODE cy_en_pdstack_usb_data_sig_t Cy_PdAltMode_Mngr_GetCableUsbCap(cy_stc_pdstack_context_t *ptrPdStackContext)
{
    cy_en_pdstack_usb_data_sig_t usbcap = CY_PDSTACK_USB_SIG_UNKNOWN;

    if(ptrPdStackContext->dpmConfig.emcaPresent)
    {
        if ((ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_ACT_CBL) && (ptrPdStackContext->dpmStat.cblVdo.act_cbl_vdo1.vdoVersion == CY_PD_CBL_VDO_VERS_1_2))
        {
            if (ptrPdStackContext->dpmStat.cblVdo2.act_cbl_vdo2.usb2Supp == (uint8_t)false)
            {
                usbcap = CY_PDSTACK_USB_2_0_SUPP;
            }
            if (ptrPdStackContext->dpmStat.cblVdo2.act_cbl_vdo2.ssSupp == (uint8_t)false)
            {
                usbcap = (ptrPdStackContext->dpmStat.cblVdo2.act_cbl_vdo2.usbGen == (uint8_t)false) ? CY_PDSTACK_USB_GEN_1_SUPP : CY_PDSTACK_USB_GEN_2_SUPP;
            }
        }
        else
        {
            /* By default, calculate USB signalling from passive/active Cable VDO (#1) */
            usbcap = (cy_en_pdstack_usb_data_sig_t)(ptrPdStackContext->dpmStat.cblVdo.std_cbl_vdo.usbSsSup);
        }
    }

    return (usbcap);
}

/* [] END OF FILE */
