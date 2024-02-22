/***************************************************************************//**
* \file cy_pdaltmode_dp_sid.c
* \version 1.0
*
* \brief
* DisplayPort Alternate Mode Source implementation.
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

#include "cy_pdstack_dpm.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_timer_id.h"
#include "cy_pdaltmode_timer_id.h"
#include "cy_usbpd_config_table.h"

#include "cy_usbpd_hpd.h"

#include "cy_pdaltmode_hw.h"
#include "cy_pdaltmode_mngr.h"
#include "cy_pdaltmode_dp_sid.h"

#if RIDGE_SLAVE_ENABLE
#include "cy_pdaltmode_intel_ridge_common.h"
#endif /* RIDGE_SLAVE_ENABLE */

#if ICL_ENABLE
#include "cy_pdaltmode_icl.h"
#endif /* ICL_ENABLE */

#if DP_DFP_SUPP
/* Analyses DP DISC MODE info for DFP */
static cy_stc_pdaltmode_mngr_info_t* Cy_PdAltMode_DP_RegDfp(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_alt_mode_reg_info_t *reg);

/* Analyse received DP sink DISC MODE VDO */
static cy_en_pdaltmode_dp_state_t Cy_PdAltMode_DP_AnalyseSinkVdo(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_pd_pd_do_t svidVdo);

/* Evaluates DP sink Status Update/Attention VDM */
static cy_en_pdaltmode_dp_state_t Cy_PdAltMode_DP_EvalUfpStatus(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/* Initialize DP DFP Alt Mode */
static void Cy_PdAltMode_DP_Init(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/* Checks if cable supports DP handling */
static bool Cy_PdAltMode_DP_IsCableCapable(cy_stc_pdaltmode_context_t *ptrAltModeContext, const cy_stc_pdaltmode_atch_tgt_info_t *atchTgtInfo);

/* HPD callback function */
static void Cy_PdAltMode_DP_SrcHpdRespCbk(void *context, uint32_t event);

/* Add received DP source HPD status to queue */
static void Cy_PdAltMode_DP_DfpEnqueueHpd(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint32_t status);

/* Clean HPD queue */
static void Cy_PdAltMode_DP_CleanHpdQueue(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/* Main DP Source Alt Mode function */
static void Cy_PdAltMode_DP_DfpRun(void *Context);

/* Analyses received status update VDO */
static void Cy_PdAltMode_DP_AnalyseStatusUpdateVdo(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/* Analyse if DFP and UFP DP modes consistent */
static bool Cy_PdAltMode_DP_IsPrtnrCcgConsistent(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t config);

/* Analyses DP DISC MODE info for the cable */
static cy_stc_pdaltmode_mngr_info_t* Cy_PdAltMode_DP_RegCbl(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_alt_mode_reg_info_t *reg);

/* Check if cable contains DP SVID */
static bool Cy_PdAltMode_DP_IsCableSvid(const cy_stc_pdaltmode_atch_tgt_info_t *atchTgtInfo, uint16_t svid);
#endif /* DP_DFP_SUPP */

#if DP_UFP_SUPP
/* HPD callback function(Snk) */
static void Cy_PdAltMode_DP_SnkHpdRespCbk(void *context, uint32_t cmd);

/* Main DP Sink Alt Mode function */
static void Cy_PdAltMode_DP_UfpRun(void *Context);

/* Updates DP Sink Status */
static bool Cy_PdAltMode_DP_UfpUpdateStatusField (cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_dp_stat_bm_t bitPos, bool status);
static void Cy_PdAltMode_DP_UfpSetStatusField (cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_dp_stat_bm_t bitPos);
static void Cy_PdAltMode_DP_UfpClearStatusField (cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_dp_stat_bm_t bitPos);

/* Add received DP sink HPD status to queue */
static void Cy_PdAltMode_DP_UfpEnqueueHpd(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_usbpd_hpd_events_t status);

/* Dequeue DP Sink HPD status */
static void Cy_PdAltMode_DP_UfpDequeueHpd(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/* Verifies is input configuration valid */
static bool Cy_PdAltMode_DP_IsConfigCorrect(cy_stc_pdaltmode_context_t *ptrAltModeContext);

#if CY_PD_DP_VCONN_SWAP_FEATURE
/* Vconn Swap initiate callback function */
static void Cy_PdAltMode_DP_VconnSwapInitCbk (cy_timer_id_t id, void *ptrContext);

/* Vconn Swap callback function */
static void Cy_PdAltMode_DP_VconnSwapCbk(cy_stc_pdstack_context_t * ptrPdStackContext, cy_en_pdstack_resp_status_t resp, const cy_stc_pdstack_pd_packet_t* pktPtr);
#endif /* CY_PD_DP_VCONN_SWAP_FEATURE */
#endif /* DP_UFP_SUPP */

#if ((DP_DFP_SUPP) || (DP_UFP_SUPP))
static void Cy_PdAltMode_DP_AddHpdEvt(cy_stc_pdaltmode_dp_status_t *dpStatus, cy_en_usbpd_hpd_events_t evt);

/* Composes VDM for sending by Alt Mode manager */
static void Cy_PdAltMode_DP_SendCmd(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/* Returns pointer to DP Alt Mode cmd info structure */
static cy_stc_pdaltmode_mngr_info_t* Cy_PdAltMode_DP_Info(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/* Evaluates DP sink Configuration command response */
static cy_en_pdaltmode_mux_select_t Cy_PdAltMode_DP_EvalConfig(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/* Exits DP Alt Mode */
static void Cy_PdAltMode_DP_Exit(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/* Initialize HPD functionality */
static void Cy_PdAltMode_DP_InitHpd(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t portRole);

/* Returns pointer to DP VDO buffer */
static cy_pd_pd_do_t* Cy_PdAltMode_DP_GetVdo(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/* Resets DP variables when exits DP Alt Mode */
static void Cy_PdAltMode_DP_ResetVar(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/* Stores given VDO in DP VDO buffer for sending */
static void Cy_PdAltMode_DP_AssignVdo(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint32_t vdo);

#if (!ICL_ALT_MODE_HPI_DISABLED)
/* Evaluates received command from application */
static bool Cy_PdAltMode_DP_EvalAppCmd(void *context, cy_stc_pdaltmode_alt_mode_evt_t cmdData);
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */
#endif /* DP_DFP_SUPP || DP_UFP_SUPP */

#if DP_DFP_SUPP
#if (!ICL_ALT_MODE_HPI_DISABLED)    
static void Cy_PdAltMode_DP_SetAppEvt(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint32_t evtype, uint32_t evdata)
{
    ptrAltModeContext->dpStatus->info.appEvtNeeded = true;
    ptrAltModeContext->dpStatus->info.appEvtData.alt_mode_event_data.evtType = evtype;
    ptrAltModeContext->dpStatus->info.appEvtData.alt_mode_event_data.evtData = evdata;
}
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */
#endif

#if ((DP_DFP_SUPP) || (DP_UFP_SUPP))
/* DP info status structure */
static cy_stc_pdaltmode_dp_status_t gl_dpStatus[NO_OF_TYPEC_PORTS];

cy_stc_pdaltmode_dp_status_t* Cy_PdAltMode_DP_GetStatus(uint8_t port)
{
    return &gl_dpStatus[port];
}
#endif /* DP_DFP_SUPP || DP_UFP_SUPP */
/********************* Alt Modes manager register function ********************/

cy_stc_pdaltmode_mngr_info_t* Cy_PdAltMode_DP_RegModes(void *context, cy_stc_pdaltmode_alt_mode_reg_info_t* regInfo)
{
#if (DP_DFP_SUPP || DP_UFP_SUPP)
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)context;
#endif /* (DP_DFP_SUPP ||  DP_UFP_SUPP) */
    cy_stc_pdaltmode_mngr_info_t* retInfo = NULL;
    regInfo->altModeId = CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED;

#if (DP_DFP_SUPP || DP_UFP_SUPP)
    if(ptrAltModeContext->altModeAppStatus->usb4Active)
    {
        return retInfo;
    }
#endif /* (DP_DFP_SUPP ||  DP_UFP_SUPP) */

    {
#if DP_UFP_SUPP
        /* If DP sink */
        if (regInfo->dataRole == (uint8_t)CY_PD_PRT_TYPE_UFP)
        {
            /* Reset DP info struct */
            ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_ResetAltModeInfo)(&(ptrAltModeContext->dpStatus->info));
            Cy_PdAltMode_DP_ResetVar(ptrAltModeContext);
            regInfo->altModeId = CY_PDALTMODE_DP_ALT_MODE_ID;
            Cy_PdAltMode_DP_Info(ptrAltModeContext)->cbk = (cy_pdaltmode_alt_mode_cbk_t) Cy_PdAltMode_DP_UfpRun;
            Cy_PdAltMode_DP_Info(ptrAltModeContext)->vdmHeader.std_vdm_hdr.svid = CY_PDALTMODE_DP_SVID;
            Cy_PdAltMode_DP_Info(ptrAltModeContext)->vdo[CY_PD_SOP] = ptrAltModeContext->dpStatus->vdo;
            Cy_PdAltMode_DP_Info(ptrAltModeContext)->vdoMaxNumb = CY_PDALTMODE_MAX_DP_VDO_NUMB;
            Cy_PdAltMode_DP_Info(ptrAltModeContext)->evalAppCmd = (cy_pdaltmode_app_cbk_t) Cy_PdAltMode_DP_EvalAppCmd;

            /* Set application event */
            regInfo->appEvt = CY_PDALTMODE_EVT_ALT_MODE_SUPP;

            /* Get supported DP sink pin assignment */
            if ((regInfo->svidVdo.std_dp_vdo.recep) != 0u)
            {
                ptrAltModeContext->dpStatus->ccgDpPinsSupp = (uint8_t)regInfo->svidVdo.std_dp_vdo.ufpDPin;
            }
            else
            {
                ptrAltModeContext->dpStatus->ccgDpPinsSupp = (uint8_t)regInfo->svidVdo.std_dp_vdo.dfpDPin;
            }
           return &(ptrAltModeContext->dpStatus->info);
        }
#endif /* DP_UFP_SUPP */

#if DP_DFP_SUPP
        /* If DP source */
        if (regInfo->atchType == CY_PDALTMODE_MNGR_ATCH_TGT)
        {
#if STORE_DETAILS_OF_HOST
            cy_stc_pdaltmode_context_t *dsContext =
                    (cy_stc_pdaltmode_context_t *)ptrAltModeContext->hostDetails->dsAltModeContext;
            if((dsContext->hostDetails->dsModeMask  & (uint8_t)CY_PDALTMODE_RIDGE_DP_MODE_DFP) == 0u)
            {
                return NULL;
            }
#endif /* STORE_DETAILS_OF_HOST */

            /* Analyze DFP DP Disc mode response */
            retInfo = Cy_PdAltMode_DP_RegDfp(ptrAltModeContext, regInfo);
        }
        else if (regInfo->atchType == CY_PDALTMODE_MNGR_CABLE)
        {
            /* Analyze DP cable Disc mode response */
           retInfo = Cy_PdAltMode_DP_RegCbl(ptrAltModeContext, regInfo);
        }
        else
        {
            /* Do nothing */
        }
#endif /* DP_DFP_SUPP */
    }

    return retInfo;
}

#if DP_DFP_SUPP
static cy_stc_pdaltmode_mngr_info_t* Cy_PdAltMode_DP_RegDfp(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_alt_mode_reg_info_t *reg)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    /* Reset DP info struct */
    ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_ResetAltModeInfo)(&ptrAltModeContext->dpStatus->info);
    reg->altModeId = CY_PDALTMODE_DP_ALT_MODE_ID;
    Cy_PdAltMode_DP_ResetVar(ptrAltModeContext);
#if (!ICL_ALT_MODE_HPI_DISABLED)  
    /* Set config ctrl as false at the start */
    ptrAltModeContext->dpStatus->dpCfgCtrl = 0u;
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */

    /* Check if cable discovered and if active cable has no fixed SS lines */
    if (!Cy_PdAltMode_DP_IsCableCapable(ptrAltModeContext, reg->atchTgtInfo))
    {
        /* Set application event */
        reg->appEvt = CY_PDALTMODE_EVT_CBL_NOT_SUPP_ALT_MODE;
        return NULL;
    }
    /* Analyze sink VDO */
    ptrAltModeContext->dpStatus->state = Cy_PdAltMode_DP_AnalyseSinkVdo(ptrAltModeContext, reg->svidVdo);
    if (ptrAltModeContext->dpStatus->state == CY_PDALTMODE_DP_STATE_EXIT)
    {
        /* Set application event */
        reg->appEvt = CY_PDALTMODE_EVT_NOT_SUPP_PARTNER_CAP;
        return NULL;
    }

    /* Check if cable is active and supports PD 3.0 */
    if (
            (ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_ACT_CBL)
#if DP_2_1_ENABLE
            || ((ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_PAS_CBL) && ptrAltModeContext->dpStatus->dp21Supp)
#endif /* DP_2_1_ENABLE */
        )
    {
        /* Check if cable has DP SVID */
        if (Cy_PdAltMode_DP_IsCableSvid(reg->atchTgtInfo, CY_PDALTMODE_DP_SVID)== false)
        {
            /* SOP' disc svid is not needed */
            reg->cblSopFlag = CY_PD_SOP_INVALID;

            /* Set application event */
            reg->appEvt = CY_PDALTMODE_EVT_ALT_MODE_SUPP;
        }
        else
        {
            /* SOP' disc svid  needed */
            reg->cblSopFlag = CY_PD_SOP_PRIME;
        }
    }

    if (
            (reg->cblSopFlag != CY_PD_SOP_PRIME) &&
            Cy_PdAltMode_DP_IsCableSvid(reg->atchTgtInfo, CY_PDALTMODE_TBT_SVID) &&
            (ptrAltModeContext->altModeAppStatus->tbtCblVdo.val == CY_PDALTMODE_MNGR_NO_DATA)
        )
    {
        reg->cblSopFlag = CY_PD_SOP_PRIME;
        ptrAltModeContext->altModeAppStatus->trigTbtVdoDisc = true;
    }

    /* Copy cable, device/AMA info pointer */
    ptrAltModeContext->dpStatus->tgtInfoPtr       = reg->atchTgtInfo;
    ptrAltModeContext->dpStatus->maxSopSupp       = (uint8_t)CY_PD_SOP;

#if CCG_UCSI_ENABLE
#if (!ICL_ALT_MODE_HPI_DISABLED)    
    /* New event to notify the UCSI module */
    reg->appEvt = CY_PDALTMODE_EVT_SUPP_ALT_MODE_CHNG;
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */
#endif /*CCG_UCSI_ENABLE*/

#if (!ICL_ALT_MODE_HPI_DISABLED)
    /* Notify application which DP MUX config is supported by both DFP and UFP */
    Cy_PdAltMode_DP_SetAppEvt(ptrAltModeContext, CY_PDALTMODE_DP_ALLOWED_MUX_CONFIG_EVT,
            ((uint32_t)ptrAltModeContext->dpStatus->partnerDpPinsSupp & (uint32_t)ptrAltModeContext->dpStatus->ccgDpPinsSupp));
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */

    Cy_PdAltMode_DP_Info(ptrAltModeContext)->vdoMaxNumb = CY_PDALTMODE_MAX_DP_VDO_NUMB;

    /* Prepare DP Enter command */
    Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState   = CY_PDALTMODE_STATE_INIT;
    Cy_PdAltMode_DP_DfpRun(ptrAltModeContext);

    return &ptrAltModeContext->dpStatus->info;
}

static cy_stc_pdaltmode_mngr_info_t* Cy_PdAltMode_DP_RegCbl(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_stc_pdaltmode_alt_mode_reg_info_t *reg)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    reg->altModeId          = CY_PDALTMODE_DP_ALT_MODE_ID;
    uint8_t cblSuppPinAsgn = CY_PDALTMODE_MNGR_NO_DATA;

    if (!ptrAltModeContext->altModeAppStatus->trigTbtVdoDisc)
    {
        cblSuppPinAsgn = (uint8_t)(reg->svidEmcaVdo.std_dp_vdo.dfpDPin | reg->svidEmcaVdo.std_dp_vdo.ufpDPin);
    }

    if (reg->cblSopFlag != CY_PD_SOP_INVALID)
    {
        /* Analyze TBT cable response */
        if (ptrAltModeContext->altModeAppStatus->trigTbtVdoDisc)
        {
            ptrAltModeContext->altModeAppStatus->trigTbtVdoDisc = false;
            /* DP mode not supported for TBT Retimer cable */
            if((reg->svidEmcaVdo.tbt_vdo.b22RetimerCbl != 0x0U) || (reg->svidEmcaVdo.tbt_vdo.cableActive != 0x0U))
            {
#if (!ICL_ALT_MODE_EVTS_DISABLED)
                /* Set application event */
                reg->appEvt = CY_PDALTMODE_EVT_CBL_NOT_SUPP_ALT_MODE;
#endif /* (!ICL_ALT_MODE_EVTS_DISABLED) */
                /* Disable TBT functionality */
                Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_DISABLE;
                return NULL;
            }
        }
        else if (
                (Cy_PdAltMode_DP_IsPrtnrCcgConsistent(ptrAltModeContext, cblSuppPinAsgn)  &&
                ((cblSuppPinAsgn & (CY_PDALTMODE_DP_DFP_D_CONFIG_C | CY_PDALTMODE_DP_DFP_D_CONFIG_D | CY_PDALTMODE_DP_DFP_D_CONFIG_E)) != 0x00U))
            )
        {
#if DP_2_1_ENABLE
            /* Check if DP cable meets DP2.1 conditions */
            if (
                    (reg->svidEmcaVdo.dp_cbl_vdo.dpAmVersion >= CY_PDALTMODE_DP_2_1_VERSION)  &&
                    (ptrPdStackContext->cblVdmMinVersion >= CY_PDSTACK_STD_VDM_MINOR_VER1)  &&
                    (ptrPdStackContext->dpmStat.cblVdmVersion == CY_PDSTACK_STD_VDM_VER2)
                )
            {
                ptrAltModeContext->altModeAppStatus->dpCblVdo = reg->svidEmcaVdo;
                if (
                        ptrAltModeContext->dpStatus->dp21Supp &&
                        (reg->svidEmcaVdo.dp_cbl_vdo.actComp == CY_PDSTACK_CBL_TYPE_PASSIVE)
                   )
                {
#if (!ICL_ALT_MODE_EVTS_DISABLED)
                    /* Set application event */
                    reg->appEvt = CY_PDALTMODE_EVT_ALT_MODE_SUPP;
#endif /* (!ICL_ALT_MODE_EVTS_DISABLED) */
                    reg->cblSopFlag = CY_PD_SOP_INVALID;
                    return &(ptrAltModeContext->dpStatus->info);
                }
            }
#endif /* DP_2_1_ENABLE */

            /* SOP' VDM needed */
            ptrAltModeContext->dpStatus->maxSopSupp = (uint8_t)CY_PD_SOP_PRIME;
            /* Allow send SOP' packets */
            Cy_PdAltMode_DP_Info(ptrAltModeContext)->sopState[CY_PD_SOP_PRIME] = CY_PDALTMODE_STATE_SEND;
            /* Allow SOP'/SOP" DP handling */
            ptrAltModeContext->dpStatus->dpActCblSupp = true;
            /* Save supported cable pin assignment */
            ptrAltModeContext->dpStatus->cableConfigSupp = cblSuppPinAsgn;

            /* If SOP'' controller present (active cables only) allow SOP'' VDM */
            if ((ptrAltModeContext->dpStatus->tgtInfoPtr->cblVdo->val != CY_PDALTMODE_MNGR_NO_DATA) &&
                    (ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_ACT_CBL) &&
                    (ptrAltModeContext->dpStatus->tgtInfoPtr->cblVdo->std_cbl_vdo.sopDp == 0x01U))
            {
                Cy_PdAltMode_DP_Info(ptrAltModeContext)->sopState[CY_PD_SOP_DPRIME] = CY_PDALTMODE_STATE_SEND;
                ptrAltModeContext->dpStatus->maxSopSupp = (uint8_t)CY_PD_SOP_DPRIME;
            }

#if (!ICL_ALT_MODE_EVTS_DISABLED)
            /* Set application event */
            reg->appEvt = CY_PDALTMODE_EVT_ALT_MODE_SUPP;
#endif /* (!ICL_ALT_MODE_EVTS_DISABLED) */
        }
#if DP_2_1_ENABLE
        else if (reg->svidEmcaVdo.dp_cbl_vdo.dpAmVersion >= CY_PDALTMODE_DP_2_1_VERSION)
        {
#if (!ICL_ALT_MODE_EVTS_DISABLED)
            /* Set application event */
            reg->appEvt = CY_PDALTMODE_EVT_CBL_NOT_SUPP_ALT_MODE;
#endif /* (!ICL_ALT_MODE_EVTS_DISABLED) */
            /* Disable TBT functionality */
            Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_DISABLE;
            return NULL;
        }
#endif /* DP_2_1_ENABLE */
        else
        {
            /* Do nothing */
        }

        reg->cblSopFlag = CY_PD_SOP_INVALID;
    }

    return &(ptrAltModeContext->dpStatus->info);
}
#endif /* DP_DFP_SUPP */

/************************* DP Source functions definitions ********************/

#if DP_DFP_SUPP
#if DP_2_1_ENABLE
static void Cy_PdAltMode_DP_HpdWaitTmrCbk (cy_timer_id_t id, void * ptrContext)
{
    (void)id;
    cy_stc_pdstack_context_t * ptrPdStackContext = (cy_stc_pdstack_context_t *) ptrContext;

    Cy_PdAltMode_DP_DfpDequeueHpd((cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext);
}
#endif /* DP_2_1_ENABLE */

static void Cy_PdAltMode_DP_SendUsbConfig(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdaltmode_dp_status_t *dpStatus = ptrAltModeContext->dpStatus;
    (void)Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SAFE, 0, 0);
    /* Clear HPD queue */
    Cy_PdAltMode_DP_CleanHpdQueue(ptrAltModeContext);
    dpStatus->state = CY_PDALTMODE_DP_STATE_CONFIG;
    dpStatus->configVdo.val = CY_PDALTMODE_DP_USB_SS_CONFIG;
#if DP_2_1_ENABLE
    /* Set VDO version */
    dpStatus->configVdo.dp_cfg_vdo.dpAmVersion = dpStatus->dp21Supp;
#endif /* DP_2_1_ENABLE */
    Cy_PdAltMode_DP_AssignVdo(ptrAltModeContext, dpStatus->configVdo.val);
    Cy_PdAltMode_DP_SendCmd(ptrAltModeContext);
}
#if MUX_UPDATE_PAUSE_FSM
static void Cy_PdAltMode_DP_DfpRunCbk(cy_timer_id_t id, void * ptrContext)
{
    (void)id;

    cy_stc_pdstack_context_t * ptrPdStackContext = (cy_stc_pdstack_context_t *) ptrContext;
    cy_stc_pdaltmode_context_t * ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    if (
            (ptrAltModeContext->altModeAppStatus->muxStat == CY_PDALTMODE_APP_MUX_STATE_BUSY) ||
           ((Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState != CY_PDALTMODE_STATE_IDLE) ||
            (Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState != CY_PDALTMODE_STATE_PAUSE))
       )
    {
        /* Run timer if MUX is busy */
        TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_ALT_MODE_CBK_TIMER), CY_PDALTMODE_APP_POLL_PERIOD, Cy_PdAltMode_DP_DfpRunCbk);
    }
    else
    {
        Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_IDLE;

        /* If Status Update or Config command is saved */
        if (ptrAltModeContext->dpStatus->prevState != CY_PDALTMODE_DP_STATE_IDLE)
        {
            /* Process saved DP state */
            ptrAltModeContext->dpStatus->state            = ptrAltModeContext->dpStatus->prevState;
            Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_WAIT_FOR_RESP;
            ptrAltModeContext->dpStatus->prevState       = CY_PDALTMODE_DP_STATE_IDLE;

            /* Run DP DFP handler */
            Cy_PdAltMode_DP_DfpRun(ptrAltModeContext);
        }
    }
}
#endif /* MUX_UPDATE_PAUSE_FSM */
    
static void Cy_PdAltMode_DP_DfpRun(void *Context)
{
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)Context;
#if MUX_UPDATE_PAUSE_FSM || DP_2_1_ENABLE
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
#endif /* MUX_UPDATE_PAUSE_FSM */

    cy_en_pdaltmode_state_t dpModeState = Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState;
    cy_stc_pdaltmode_dp_status_t *dpStatus = ptrAltModeContext->dpStatus;

    switch (dpModeState)
    {
        case CY_PDALTMODE_STATE_INIT:
            /* Enable DP functionality */
            Cy_PdAltMode_DP_Init(ptrAltModeContext);

            /* Send Enter Mode command */
            Cy_PdAltMode_DP_SendCmd(ptrAltModeContext);
            break;

        case CY_PDALTMODE_STATE_WAIT_FOR_RESP:
            dpStatus->state = (cy_en_pdaltmode_dp_state_t) dpStatus->info.vdmHeader.std_vdm_hdr.cmd;
            Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_IDLE;
            CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 16.1', 1, 'Intentional Return to avoid send_cmd.');
            CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 16.3', 1, 'Intentional Return to avoid send_cmd.');
            switch (dpStatus->state)
            {
                case CY_PDALTMODE_DP_STATE_ENTER:
                    /* Init HPD */
                    Cy_PdAltMode_DP_InitHpd(ptrAltModeContext, (uint8_t)CY_PD_PRT_TYPE_DFP);

                    /* Go to Status update */
                    dpStatus->state = CY_PDALTMODE_DP_STATE_STATUS_UPDATE;
                    Cy_PdAltMode_DP_AssignVdo(ptrAltModeContext, CY_PDALTMODE_DP_STATUS_UPDATE_VDO);
                    break;

                case CY_PDALTMODE_DP_STATE_STATUS_UPDATE:
#if MUX_UPDATE_PAUSE_FSM
                    if (ptrAltModeContext->altModeAppStatus->muxStat == CY_PDALTMODE_APP_MUX_STATE_BUSY)
                    {
                        /* Run timer if MUX is busy */
                        TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                                CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_ALT_MODE_CBK_TIMER),
                                    CY_PDALTMODE_APP_POLL_PERIOD, Cy_PdAltMode_DP_DfpRunCbk);
                        dpStatus->prevState = CY_PDALTMODE_DP_STATE_STATUS_UPDATE;
                        Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_PAUSE;
                        return;
                    }
#endif /* MUX_UPDATE_PAUSE_FSM */
                    /* Analyse received VDO */
                    Cy_PdAltMode_DP_AnalyseStatusUpdateVdo(ptrAltModeContext);
                    Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_IDLE;
#if (!ICL_ALT_MODE_HPI_DISABLED)
                    if (dpStatus->dpCfgCtrl == CY_PDALTMODE_DP_MUX_CTRL_CMD)
                    {
                        return;
                    }
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */
                    break;

                case CY_PDALTMODE_DP_STATE_CONFIG:
#if MUX_UPDATE_PAUSE_FSM
                    if (ptrAltModeContext->altModeAppStatus->muxStat == CY_PDALTMODE_APP_MUX_STATE_BUSY)
                    {
                        /* Run timer if MUX is busy */
                        TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                                CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_ALT_MODE_CBK_TIMER) ,
                                        CY_PDALTMODE_APP_POLL_PERIOD, Cy_PdAltMode_DP_DfpRunCbk);
                        dpStatus->prevState = CY_PDALTMODE_DP_STATE_CONFIG;
                        Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_PAUSE;
                        return;
                    }
#endif /* MUX_UPDATE_PAUSE_FSM */

#if (!ICL_ALT_MODE_HPI_DISABLED)                    
                    /* If App controls DP MUX Config - send ACK to App */
                    if (dpStatus->dpCfgCtrl == CY_PDALTMODE_DP_MUX_CTRL_CMD)
                    {
                        Cy_PdAltMode_DP_SetAppEvt(ptrAltModeContext, CY_PDALTMODE_DP_STATUS_UPDATE_EVT, CY_PDALTMODE_DP_CFG_CMD_ACK_MASK);
                    }
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */

                    Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_IDLE;
#if AM_PD3_FLOW_CTRL_EN
                    /* Make sure Rp is set back to SinkTxOK here */
                    (void)Cy_PdStack_Dpm_Pd3SrcRpFlowControl (ptrAltModeContext->pdStackContext, false);
#endif /* AM_PD3_FLOW_CTRL_EN */

                    /* Set MUX */
                    dpStatus->dpMuxCfg = Cy_PdAltMode_DP_EvalConfig(ptrAltModeContext);

                    if (!dpStatus->dpExit)
                    {
                        /* Set MUX with DP config data */
                        (void)Cy_PdAltMode_HW_SetMux (ptrAltModeContext, dpStatus->dpMuxCfg, dpStatus->configVdo.val,
                                    (((dpStatus->dp21Supp != 0x00U) || (dpStatus->dpMuxCfg == CY_PDALTMODE_MUX_CONFIG_SS_ONLY))
                                            ? CY_PDALTMODE_MNGR_NO_DATA : dpStatus->statusVdo.val));

                        /* Clear HPD queue if USB config is set */
                        if (dpStatus->dpMuxCfg == CY_PDALTMODE_MUX_CONFIG_SS_ONLY)
                        {
#if DP_2_1_ENABLE
                            if (dpStatus->dp21Supp)
                            {
                                /* Stop HPD WAIT timer */
                                TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrPdStackContext->ptrTimerContext,
                                        CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_HPD_WAIT_TIMER));
                            }
#endif /* DP_2_1_ENABLE */
                            Cy_PdAltMode_DP_CleanHpdQueue(ptrAltModeContext);
                        }

                        {
                            /* Handle HPD queue */
                            if ((dpStatus->configVdo.val & CY_PDALTMODE_DP_USB_SS_CONFIG_MASK) == 0x00UL)
                            {
                                Cy_PdAltMode_DP_DfpEnqueueHpd(ptrAltModeContext, CY_PDALTMODE_MNGR_EMPTY_VDO);
                            }
                            else
                            {
#if DP_2_1_ENABLE
                                /* Run HPD WAIT timer */
                                if (dpStatus->configVdo.dp_cfg_vdo.dpAmVersion)
                                {
                                    TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                                            CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_HPD_WAIT_TIMER),
                                                CY_PDALTMODE_APP_HPD_WAIT_TIMER_PERIOD, Cy_PdAltMode_DP_HpdWaitTmrCbk);
                                }
#endif /* DP_2_1_ENABLE */
                                /* Add HPD to queue*/
                                Cy_PdAltMode_DP_DfpEnqueueHpd(ptrAltModeContext, dpStatus->statusVdo.val);
                            }
                        }
                    }
                    else
                    {
                        /* Exit DP required */
                        dpStatus->state = CY_PDALTMODE_DP_STATE_EXIT;
                        dpStatus->dpExit = false;
                        break;
                    }
                    /* Return to avoid send_cmd here */
                    return;

               case CY_PDALTMODE_DP_STATE_EXIT:
                    Cy_PdAltMode_DP_Exit(ptrAltModeContext);
                    /* Return to avoid send_cmd here */
                    return;

                default:
                    /* Return to avoid send_cmd here */
                    Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_IDLE;
                    return;
            } 
            CY_MISRA_BLOCK_END('MISRA C-2012 Rule 16.3');
            CY_MISRA_BLOCK_END('MISRA C-2012 Rule 16.1');
            Cy_PdAltMode_DP_SendCmd(ptrAltModeContext);
            break;

        case CY_PDALTMODE_STATE_IDLE:
            /* Verify if input message is Attention */
            if ((cy_en_pdaltmode_dp_state_t)Cy_PdAltMode_DP_Info(ptrAltModeContext)->vdmHeader.std_vdm_hdr.cmd == CY_PDALTMODE_DP_STATE_ATT)
            {   
                /* Analyse received VDO */
                Cy_PdAltMode_DP_AnalyseStatusUpdateVdo(ptrAltModeContext);

                /* Handle HPD status */
                if (
                       (dpStatus->dpActiveFlag)            &&
                       (Cy_PdAltMode_DP_GetVdo(ptrAltModeContext)->dp_stat_vdo.exit == 0u)    &&
                       (Cy_PdAltMode_DP_GetVdo(ptrAltModeContext)->dp_stat_vdo.usbCfg == 0u) &&
                       (dpStatus->state != CY_PDALTMODE_DP_STATE_CONFIG)
                    )
                {
                    /* Add HPD to queue*/
                    Cy_PdAltMode_DP_DfpEnqueueHpd(ptrAltModeContext, (*Cy_PdAltMode_DP_GetVdo(ptrAltModeContext)).val);
                }
#if (!ICL_ALT_MODE_HPI_DISABLED)
                if (dpStatus->dpCfgCtrl == CY_PDALTMODE_DP_MUX_CTRL_CMD)
                {
                   return;
                }
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */
                Cy_PdAltMode_DP_SendCmd(ptrAltModeContext);
            }
            break;

        case CY_PDALTMODE_STATE_FAIL:
            if ((dpStatus->state == CY_PDALTMODE_DP_STATE_ENTER) || (dpStatus->state == CY_PDALTMODE_DP_STATE_EXIT))
            {
                Cy_PdAltMode_DP_Exit(ptrAltModeContext);
            }
            else if (dpStatus->state == CY_PDALTMODE_DP_STATE_CONFIG)
            {
                /* Set MUX to USB config in case of failed config command */
                if (dpStatus->configVdo.dp_cfg_vdo.selConf != CY_PDALTMODE_DP_USB_SS_CONFIG)
                {
                    Cy_PdAltMode_DP_SendUsbConfig (ptrAltModeContext);
                }
                else
                {
                    (void)Cy_PdAltMode_HW_SetMux(ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SS_ONLY, CY_PDALTMODE_MNGR_NO_DATA, CY_PDALTMODE_MNGR_NO_DATA);
                }
            }
            else
            {
                /* DP remains in current configuration */
                dpStatus->state = CY_PDALTMODE_DP_STATE_IDLE;
                Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_IDLE;
            }

#if AM_PD3_FLOW_CTRL_EN
            /* Make sure Rp is set back to SinkTxOK here */
            (void)Cy_PdStack_Dpm_Pd3SrcRpFlowControl (ptrAltModeContext->pdStackContext, false);
#endif /* AM_PD3_FLOW_CTRL_EN */
            break;

        case CY_PDALTMODE_STATE_EXIT:
            if (ptrAltModeContext->pdStackContext->dpmConfig.attach)
            {
                Cy_PdAltMode_DP_SendUsbConfig (ptrAltModeContext);
                dpStatus->dpExit = true;
            }
            else
            {
                /* Force DP mode exit if port is unattached */
                Cy_PdAltMode_DP_Exit(ptrAltModeContext);
            }
            break;

        default:
            /* Do nothing */
            break;
    }
}

static void Cy_PdAltMode_DP_AnalyseStatusUpdateVdo(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdaltmode_dp_status_t *dpStatus = ptrAltModeContext->dpStatus;
#if (!ICL_ALT_MODE_HPI_DISABLED)
    /* If App layer controls DP MUX config */
    if (dpStatus->dpCfgCtrl == CY_PDALTMODE_DP_MUX_CTRL_CMD)
    {
        Cy_PdAltMode_DP_SetAppEvt(ptrAltModeContext, CY_PDALTMODE_DP_STATUS_UPDATE_EVT, (*Cy_PdAltMode_DP_GetVdo(ptrAltModeContext)).val);

        /* Set DP to idle and wait for config cmd from App */
        Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_IDLE;
        return;
    }
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */
    /* CCG handles Status Update in auto mode */
    dpStatus->state = Cy_PdAltMode_DP_EvalUfpStatus(ptrAltModeContext);

    if (dpStatus->state == CY_PDALTMODE_DP_STATE_CONFIG)
    {
#if AM_PD3_FLOW_CTRL_EN
        /* If we are a PD 3.0 source, change Rp to SinkTxNG to prevent sink from initiating any AMS */
        (void)Cy_PdStack_Dpm_Pd3SrcRpFlowControl (ptrAltModeContext->pdStackContext, true);
#endif /* AM_PD3_FLOW_CTRL_EN */
        /* Clear HPD queue in case of USB SS configuration */
        Cy_PdAltMode_DP_CleanHpdQueue(ptrAltModeContext);
#if (AMD_RETIMER_ENABLE || !VIRTUAL_HPD_ENABLE)
        (void)Cy_PdAltMode_HW_EvalHpdCmd (ptrAltModeContext, (uint32_t)CY_USBPD_HPD_EVENT_UNPLUG);
#endif /* AMD_RETIMER_ENABLE || !VIRTUAL_HPD_ENABLE */
        /* Move MUX into SAFE state while going through DP configuration */
        (void)Cy_PdAltMode_HW_SetMux(ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SAFE, CY_PDALTMODE_MNGR_NO_DATA, CY_PDALTMODE_MNGR_NO_DATA);

        if (dpStatus->configVdo.val != CY_PDALTMODE_DP_USB_SS_CONFIG)
        {
            dpStatus->configVdo.dp_cfg_vdo.selConf = CY_PDALTMODE_DP_CONFIG_SELECT;
            dpStatus->configVdo.dp_cfg_vdo.sign    = CY_PDALTMODE_DP_1_3_SIGNALING;
#if DP_2_1_ENABLE
            /* DP2.1 processing */
            if (dpStatus->dp21Supp)
            {
                /* Set cable speed and type */
                if (ptrAltModeContext->altModeAppStatus->dpCblVdo.dp_cbl_vdo.dpAmVersion)
                {
                    dpStatus->configVdo.dp_cfg_vdo.sign = CY_PDALTMODE_MNGR_NO_DATA;
                    dpStatus->configVdo.val |= (ptrAltModeContext->altModeAppStatus->dpCblVdo.val & CY_PDALTMODE_DP_ACT_CBL_PARAM_MASK);
                }
                else if (
                            (ptrAltModeContext->altModeAppStatus->tbtCblVdo.val != CY_PDALTMODE_MNGR_NO_DATA) &&
                            (ptrAltModeContext->altModeAppStatus->tbtCblVdo.tbt_vdo.b22RetimerCbl == false)
                        )
                {
                    if (ptrAltModeContext->altModeAppStatus->tbtCblVdo.tbt_vdo.cblSpeed >= CY_PDSTACK_USB_GEN_1)
                    {
                        dpStatus->configVdo.dp_cfg_vdo.sign |= CY_PDALTMODE_DP_HBR10_SUPP_MASK;
                    }
                    if (ptrAltModeContext->altModeAppStatus->tbtCblVdo.tbt_vdo.cblSpeed >= CY_PDSTACK_USB_GEN_3)
                    {
                        dpStatus->configVdo.dp_cfg_vdo.sign |= CY_PDALTMODE_DP_HBR20_SUPP_MASK;
                    }
                }
                else if (ptrAltModeContext->pdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_PAS_CBL)
                {
                    if (ptrAltModeContext->pdStackContext->dpmStat.cblVdo.std_cbl_vdo.usbSsSup >= CY_PDSTACK_USB_GEN_1)
                        dpStatus->configVdo.dp_cfg_vdo.sign |= CY_PDALTMODE_DP_HBR10_SUPP_MASK;
                    if (ptrAltModeContext->pdStackContext->dpmStat.cblVdo.std_cbl_vdo.usbSsSup >= CY_PDSTACK_USB_GEN_3)
                        dpStatus->configVdo.dp_cfg_vdo.sign |= CY_PDALTMODE_DP_HBR20_SUPP_MASK;
                }
            }
#endif /* DP_2_1_ENABLE */

            if (dpStatus->dpActCblSupp)
            {
                /* Check if DP active cable could provide config pin assignment */
                if ((dpStatus->cableConfigSupp & dpStatus->configVdo.dp_cfg_vdo.dfpAsgmnt) == CY_PDALTMODE_MNGR_NO_DATA)
                {
#if AM_PD3_FLOW_CTRL_EN
                    /* Let Rp change to SinkTxOK. */
                    (void)Cy_PdStack_Dpm_Pd3SrcRpFlowControl (ptrAltModeContext->pdStackContext, false);
#endif /* AM_PD3_FLOW_CTRL_EN */
                    dpStatus->state =  CY_PDALTMODE_DP_STATE_EXIT;
                    return;
                }
            }
        }
#if DP_2_1_ENABLE
        /* Set VDO version */
        dpStatus->configVdo.dp_cfg_vdo.dpAmVersion = dpStatus->dp21Supp;
#endif /* DP_2_1_ENABLE */

        Cy_PdAltMode_DP_AssignVdo(ptrAltModeContext, dpStatus->configVdo.val);
    }
}

static cy_en_pdaltmode_dp_state_t Cy_PdAltMode_DP_AnalyseSinkVdo(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_pd_pd_do_t svidVdo)
{
    uint8_t    commonConf = 0;
    cy_en_pdaltmode_dp_state_t ret = CY_PDALTMODE_DP_STATE_EXIT;

    uint8_t dp4Lane = CY_PDALTMODE_MNGR_NO_DATA;
    uint8_t dp2Lane = CY_PDALTMODE_MNGR_NO_DATA;

#if STORE_DETAILS_OF_HOST
    cy_stc_pdaltmode_context_t *deviceContext =
                                        (cy_stc_pdaltmode_context_t *)ptrAltModeContext->hostDetails->dsAltModeContext;
    uint8_t ds_det_dp_mode = deviceContext->hostDetails->dsDp2LaneModeCtrl;
    cy_stc_pdaltmode_context_t *hostContext =
                                        (cy_stc_pdaltmode_context_t *)ptrAltModeContext->hostDetails->usAltModeContext;
#endif

    /* Check the UFP_D-Capable bit */
    if ((svidVdo.std_dp_vdo.portCap & 0x01U) != 0U)
    {
        /* Read DP_MODES_SUPPORTED in DP structure */
        ptrAltModeContext->dpStatus->ccgDpPinsSupp = ptrAltModeContext->dpCfg->dp_config_supported;

        /* If DP Interface is presented on a USB TYPE C receptacle,
         * compare with UFP_D Pin Assignments advertised by UFP. */
        if (svidVdo.std_dp_vdo.recep!=0U)
        {
            ptrAltModeContext->dpStatus->partnerDpPinsSupp = (uint8_t)svidVdo.std_dp_vdo.ufpDPin;
        }
        else
        {
            ptrAltModeContext->dpStatus->partnerDpPinsSupp = (uint8_t)svidVdo.std_dp_vdo.dfpDPin;
        }

        commonConf = ptrAltModeContext->dpStatus->partnerDpPinsSupp & ptrAltModeContext->dpStatus->ccgDpPinsSupp;

        /* There is at least one shared pin configuration between CCG and Port-partner */
        if ((commonConf & (CY_PDALTMODE_DP_DFP_D_CONFIG_E | CY_PDALTMODE_DP_DFP_D_CONFIG_C | CY_PDALTMODE_DP_DFP_D_CONFIG_D)) != 0U)
        {
            /* If one of pin configurations C, E or D is supported by both DFP & UFP, can proceed. */
            ret = CY_PDALTMODE_DP_STATE_ENTER;

            /* Assume that only 4-lane configurations are supported */
            dp2Lane = CY_PDALTMODE_DP_INVALID_CFG;

            if ((commonConf & CY_PDALTMODE_DP_DFP_D_CONFIG_E) != 0U)
            {
                dp4Lane = CY_PDALTMODE_DP_DFP_D_CONFIG_E;
            }

            /* Configuration C has higher priority than E */
            if ((commonConf & CY_PDALTMODE_DP_DFP_D_CONFIG_C) != 0U)
            {
                dp4Lane = CY_PDALTMODE_DP_DFP_D_CONFIG_C;
            }

            if (
                    ((commonConf & CY_PDALTMODE_DP_DFP_D_CONFIG_D) != 0U)
#if STORE_DETAILS_OF_HOST
                    && (ds_det_dp_mode == 1u)
#endif
                )
            {
                dp2Lane = CY_PDALTMODE_DP_DFP_D_CONFIG_D;
            }
            else
            {
                if ((commonConf & CY_PDALTMODE_DP_DFP_D_CONFIG_F) != 0U)
                {
                    dp2Lane = CY_PDALTMODE_DP_DFP_D_CONFIG_F;
                }
            }
        }
    }
    
#if STORE_DETAILS_OF_HOST
    /* Handle exchange capabilities for DFP only if the host is connected */
    if((hostContext->pdStackContext->dpmConfig.attach) && (ptrAltModeContext->pdStackContext->port == TYPEC_PORT_1_IDX))
    {
        /* 2 lane or 4 lane DP mode is allowed */
        if(ds_det_dp_mode == 1u)
        {
            if (Cy_PdAltMode_DP_IsPrtnrCcgConsistent(ptrAltModeContext, CY_PDALTMODE_DP_DFP_D_CONFIG_D))
            {
                dp2Lane = CY_PDALTMODE_DP_DFP_D_CONFIG_D;  
            }
            else if (Cy_PdAltMode_DP_IsPrtnrCcgConsistent(ptrAltModeContext, CY_PDALTMODE_DP_DFP_D_CONFIG_F))
            {
                dp2Lane = CY_PDALTMODE_DP_DFP_D_CONFIG_F; 
            }
            else
            {
                /* Disable 2 lane mode if US is in DP mode */
                if(
                        (gl_dpStatus[TYPEC_PORT_0_IDX].dpMuxCfg == CY_PDALTMODE_MUX_CONFIG_DP_2_LANE) ||
                        (gl_dpStatus[TYPEC_PORT_0_IDX].dpMuxCfg == CY_PDALTMODE_MUX_CONFIG_DP_4_LANE)
                  )
                {
                    dp2Lane = CY_PDALTMODE_MNGR_NO_DATA;
                }
            }
        }
        /* Only 4 lane allowed */
        else
        {
            if (
                (CY_PDALTMODE_DP_DFP_D_CONFIG_C != dp4Lane) &&
                (CY_PDALTMODE_DP_DFP_D_CONFIG_E != dp4Lane)
               )
            {
                ret = CY_PDALTMODE_DP_STATE_EXIT;
            }
            else
            {
                /* Disable 2 lane configuration only if US is in DP mode */
                if (gl_dpStatus[TYPEC_PORT_0_IDX].dpMuxCfg == CY_PDALTMODE_MUX_CONFIG_DP_4_LANE)
                {
                    /* Proceed with 4-lane configuration and disable 2-lane options */
                    dp2Lane = CY_PDALTMODE_MNGR_NO_DATA;
                }
            }
        }
    }
#endif /* STORE_DETAILS_OF_HOST */

#if CY_APP_USBC_DOCK_EXCHANGE_CAP_DP_USB3
    if(ptrAltModeContext->ptrAltPortAltmodeCtx->pdStackContext->dpmConfig.attach)
    {
        /* Process DFP only in case if DP UFP is active */
        if (ptrAltModeContext->pdStackContext->port == TYPEC_PORT_1_IDX)
        {
            /* If US port is in 4-lane connection DS can be in 4-lane connection only */
            if  (gl_dpStatus[TYPEC_PORT_0_IDX].dpMuxCfg == CY_PDALTMODE_MUX_CONFIG_DP_4_LANE)
            {
                if (
                        (CY_PDALTMODE_DP_DFP_D_CONFIG_C != dp4Lane) &&
                        (CY_PDALTMODE_DP_DFP_D_CONFIG_E != dp4Lane)
                    )
                {
                    ret = CY_PDALTMODE_DP_STATE_EXIT;
                }
                else
                {
                    /* Proceed with 4-lane configuration and disable 2-lane options */
                    dp2Lane = CY_PDALTMODE_MNGR_NO_DATA;
                }
            }
            /* If US port is in 2-lane connection, DS can be in 2-lane or 4-lane connection. */
            else if  (gl_dpStatus[TYPEC_PORT_0_IDX].dpMuxCfg == CY_PDALTMODE_MUX_CONFIG_DP_2_LANE)
            {
                /* No action needed. The DS port can enter 2L or 4L mode based on capabilities reported by the port partner. */
            }
            else
            {
                ret = CY_PDALTMODE_DP_STATE_EXIT;
            }
        }
    }
#endif /* CY_APP_USBC_DOCK_EXCHANGE_CAP_DP_USB3 */

    /* Set DP2.1 support flag */
    if (
           (svidVdo.std_dp_vdo.dpAmVersion >= CY_PDALTMODE_DP_2_1_VERSION)                   &&
           (ptrAltModeContext->appStatusContext->vdmVersion >= (uint8_t)CY_PDSTACK_STD_VDM_VER2)              &&
           (ptrAltModeContext->appStatusContext->vdmMinorVersion >= (uint8_t)CY_PDSTACK_STD_VDM_MINOR_VER1)  &&
           (ptrAltModeContext->pdStackContext->dpmStat.specRevPeer > CY_PD_REV2)
       )
    {
        ptrAltModeContext->dpStatus->dp21Supp = (uint8_t)((svidVdo.std_dp_vdo.dpAmVersion != 0x00UL) &&
                ((ptrAltModeContext->altModeAppStatus->customHpiHostCapControl & CY_PDALTMODE_DP21_EN_HOST_PARAM_MASK) != CY_PDALTMODE_MNGR_NO_DATA) &&
                /* Force DP 2.1 Enable */
                ((ptrAltModeContext->dpCfg->dp_operation & CY_PDALTMODE_DP2_1_CFG_MASK) != 0x00U));
    }

    ptrAltModeContext->dpStatus->dp4Lane = dp4Lane;
    ptrAltModeContext->dpStatus->dp2Lane = dp2Lane;

    return ret;
}

static bool Cy_PdAltMode_DP_CheckDp2LaneSupport(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_pd_pd_do_t status)
{
    /*
     * Return true if UFP is requesting multi-function mode and we have a valid 2-lane configuration.
     * Please note that pin configuration F is not valid for a UFP-D.
     */
    return (
            (status.dp_stat_vdo.multFun != 0x0U) &&
            (ptrAltModeContext->dpStatus->dp2Lane != CY_PDALTMODE_DP_INVALID_CFG) &&
            ((status.dp_stat_vdo.dfpUfpConn != (uint32_t)CY_PDALTMODE_DP_CONN_UFP_D) || (ptrAltModeContext->dpStatus->dp2Lane != CY_PDALTMODE_DP_DFP_D_CONFIG_F))
           );
}

static cy_en_pdaltmode_dp_state_t Cy_PdAltMode_DP_EvalUfpStatus(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_en_pdaltmode_dp_state_t ret = CY_PDALTMODE_DP_STATE_CONFIG;
    bool       dpMode = false;
    uint32_t   dpAsgn = CY_PDALTMODE_DP_INVALID_CFG;

    cy_pd_pd_do_t status = *Cy_PdAltMode_DP_GetVdo(ptrAltModeContext);
    cy_stc_pdaltmode_dp_status_t *dpStatus = ptrAltModeContext->dpStatus;

    dpStatus->statusVdo = status;
    dpStatus->configVdo.val = 0;

    /* If UFP sends ATTENTION with no Status byte we just remain in the same state */
    if (Cy_PdAltMode_DP_Info(ptrAltModeContext)->vdoNumb[CY_PD_SOP] == CY_PDALTMODE_MNGR_NO_DATA){
        return CY_PDALTMODE_DP_STATE_IDLE;
    }
    /* Check whether the "USB Configuration Request" or "Exit DisplayPort Mode Request" bits are set */
    if ((status.val & 0x00000060UL) != 0UL)
    {
        /* Ignore re-configuration to USB if MUX is already in USB configuration */
        if ((status.dp_stat_vdo.usbCfg != 0U) && (dpStatus->dpMuxCfg == CY_PDALTMODE_MUX_CONFIG_SS_ONLY)){
             return CY_PDALTMODE_DP_STATE_IDLE;
        }
        dpStatus->configVdo.val = CY_PDALTMODE_DP_USB_SS_CONFIG;

        /* If Exit request bit is set then send Exit cmd */
        if (status.dp_stat_vdo.exit != 0U)
        {
            dpStatus->dpExit = true;
        }
    }
    else if (!dpStatus->dpActiveFlag)
    {
        /* Previously in USB configuration. Check if we can switch to DP. */
        if (status.dp_stat_vdo.dfpUfpConn > (uint32_t)CY_PDALTMODE_DP_CONN_DFP_D)
        {
            /* Check if Multi Function is requested and CCG can support 2 lane */
            if (
                    (Cy_PdAltMode_DP_CheckDp2LaneSupport (ptrAltModeContext, status)) ||
                    (dpStatus->dp4Lane == CY_PDALTMODE_MNGR_NO_DATA)
               )
            {
                dpAsgn = dpStatus->dp2Lane;
            }
            else
            {
                dpAsgn = dpStatus->dp4Lane;
            }

            dpMode = true;
        }
        else
        {
            ret = CY_PDALTMODE_DP_STATE_IDLE;
        }
    }
    else
    {
        /* Previously in DP Configuration */
        /* Check if UFP_D is no longer connected */
        if (status.dp_stat_vdo.dfpUfpConn < (uint32_t)CY_PDALTMODE_DP_CONN_UFP_D)
        {
            /* Move to USB Config. */
            dpStatus->configVdo.val = CY_PDALTMODE_DP_USB_SS_CONFIG;
        }
        /* Check if DP sink is requesting for 2 lane, and the current
         * config is not 2 DP lane. */
        else if (
                    (Cy_PdAltMode_DP_CheckDp2LaneSupport (ptrAltModeContext, status)) &&
                    (!dpStatus->dp2LaneActive)
#if (STORE_DETAILS_OF_HOST || CY_APP_USBC_DOCK_EXCHANGE_CAP_DP_USB3)
                    /* Proceed for 2 lane only if UFP supports 2 lane or if UFP is not connected */
                    && (((gl_dpStatus[TYPEC_PORT_0_IDX].dpMuxCfg == CY_PDALTMODE_MUX_CONFIG_DP_2_LANE) &&
                            (ptrAltModeContext->ptrAltPortAltmodeCtx->pdStackContext->dpmConfig.attach))    ||
                            (ptrAltModeContext->ptrAltPortAltmodeCtx->pdStackContext->dpmConfig.attach == false))
#endif /* (STORE_DETAILS_OF_HOST || CY_APP_USBC_DOCK_EXCHANGE_CAP_DP_USB3) */
                )
        {
            /* Move to DP 2 lane */
            dpMode = true;
            dpAsgn = dpStatus->dp2Lane;
        }
        /* Check if DP Sink is requesting for NON-MULTI Function Mode
         * and current mode is MULTI FUNCTION. In this case switch to
         * 4 lane DP. */
        else if (
                (dpStatus->dp2LaneActive && (status.dp_stat_vdo.multFun == 0x0U))
#if (STORE_DETAILS_OF_HOST || CY_APP_USBC_DOCK_EXCHANGE_CAP_DP_USB3)
                    /* Proceed for 4 lane only if UFP supports 4 lane or if UFP is not connected */
                    && (((gl_dpStatus[TYPEC_PORT_0_IDX].dpMuxCfg == CY_PDALTMODE_MUX_CONFIG_DP_4_LANE) &&
                        (ptrAltModeContext->ptrAltPortAltmodeCtx->pdStackContext->dpmConfig.attach))    ||
                        (ptrAltModeContext->ptrAltPortAltmodeCtx->pdStackContext->dpmConfig.attach == false))
#endif /* (STORE_DETAILS_OF_HOST || CY_APP_USBC_DOCK_EXCHANGE_CAP_DP_USB3) */
                )
        {
            /* Move to DP 4 lane */
            dpMode = true;
            dpAsgn = dpStatus->dp4Lane;
        }
        else
        {
            ret = CY_PDALTMODE_DP_STATE_IDLE;
        }
    }

    if (dpMode)
    {
        dpStatus->configVdo.dp_cfg_vdo.dfpAsgmnt = dpAsgn;
    }

    return ret;
}

static bool Cy_PdAltMode_DP_IsCableCapable(cy_stc_pdaltmode_context_t *ptrAltModeContext, const cy_stc_pdaltmode_atch_tgt_info_t* atchTgtInfo)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    if ((atchTgtInfo->cblVdo != NULL) && (atchTgtInfo->cblVdo->val != CY_PDALTMODE_MNGR_NO_DATA))
    {
        /*
         * If the cable supports DP mode explicitly, can go ahead with it.
         * Otherwise, the cable has to support USB 3.1 Gen1 or higher data rates.
         */
        if (
               ((ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetCableUsbCap)(ptrPdStackContext) == CY_PDSTACK_USB_2_0_SUPP)        ||
                (ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_ACT_CBL)) &&
                (!Cy_PdAltMode_DP_IsCableSvid(atchTgtInfo, CY_PDALTMODE_DP_SVID))
           )
        {
            return false;
        }
    }

    /* Allow DP mode to go through if no cable marker is found */
    return true;
}

static bool Cy_PdAltMode_DP_IsCableSvid(const cy_stc_pdaltmode_atch_tgt_info_t* atchTgtInfo, uint16_t svid)
{
    uint8_t idx = 0;

    while (atchTgtInfo->cblSvid[idx] != 0x0U)
    {
        /* If Cable Intel SVID is found */
        if (atchTgtInfo->cblSvid[idx] == svid)
        {
            return true;
        }
        idx++;
    }

    return false;
}

static void Cy_PdAltMode_DP_Init(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdaltmode_mngr_info_t *info = Cy_PdAltMode_DP_Info(ptrAltModeContext);
    
#if (!ICL_ALT_MODE_HPI_DISABLED)
    info->evalAppCmd = Cy_PdAltMode_DP_EvalAppCmd;
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */
    info->vdoMaxNumb    = CY_PDALTMODE_MAX_DP_VDO_NUMB;
    info->vdo[CY_PD_SOP]        = ptrAltModeContext->dpStatus->vdo;
    info->vdo[CY_PD_SOP_PRIME]  = ptrAltModeContext->dpStatus->cableVdo;
    info->vdo[CY_PD_SOP_DPRIME] = ptrAltModeContext->dpStatus->cableVdo;

    /* No need to put MUX into SAFE state at mode entry as this is managed in CONFIG state */
    info->setMuxIsolate = false;
    info->cbk = (cy_pdaltmode_alt_mode_cbk_t)Cy_PdAltMode_DP_DfpRun;

    /* Goto enter state */
    ptrAltModeContext->dpStatus->state = CY_PDALTMODE_DP_STATE_ENTER;
    ptrAltModeContext->dpStatus->dpMuxCfg = CY_PDALTMODE_MUX_CONFIG_SS_ONLY;
}

#if MUX_DELAY_EN && ICL_ENABLE
static void Cy_PdAltMode_DP_HpdDelayCbk(cy_timer_id_t id, void* context)
{
    (void)id;
    cy_stc_pdstack_context_t * ptrPdStackContext = (cy_stc_pdstack_context_t *) context;
    cy_stc_pdaltmode_context_t * ptrAltModeContext = (cy_stc_pdaltmode_context_t *) ptrPdStackContext->ptrAltModeContext;
    
    /* Run dequeue just if DP configuration is active */
    if (ptrAltModeContext->dpStatus->dpActiveFlag)
    {
        if(ptrAltModeContext->altModeAppStatus->isMuxBusy)
        {
            TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                    CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_HPD_DELAY_TIMER),
                        ptrAltModeContext->iclCfg->soc_mux_config_delay, (void *) Cy_PdAltMode_DP_HpdDelayCbk);
        }
        else
        {
            Cy_PdAltMode_Icl_SetEvt(ptrPdStackContext->port, CY_PDALTMODE_ICL_EVT_DEQUEUE_HPD);
        }
    }
}
#else
#if !VIRTUAL_HPD_ENABLE
static void Cy_PdAltMode_DP_HpdDelayCbk(cy_timer_id_t id, void *context)
{
    (void)id;

    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *) context;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    /* Run dequeue just if DP configuration is active */
    if (ptrAltModeContext->dpStatus->dpActiveFlag && (Cy_USBPD_Hpdt_IsCommandActive(ptrPdStackContext->ptrUsbPdContext) == true))
    {
        Cy_PdAltMode_DP_DfpDequeueHpd(ptrAltModeContext);
    }
    else if (
                (ptrAltModeContext->altModeAppStatus->muxStat == CY_PDALTMODE_APP_MUX_STATE_BUSY) ||
                (ptrAltModeContext->dpStatus->dpMuxCfg != CY_PDALTMODE_MUX_CONFIG_SS_ONLY)
            )
    {
        /* Postpone HPD update if datapath update is pending or MUX configured to USB SS */
        (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrAltModeContext->pdStackContext->ptrTimerContext, ptrAltModeContext->pdStackContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrAltModeContext->pdStackContext, CY_PDALTMODE_HPD_DELAY_TIMER),
                    CY_PDALTMODE_APP_HPD_DEQUE_POLL_PERIOD, Cy_PdAltMode_DP_HpdDelayCbk);
    }
}
#endif /* VIRTUAL_HPD_ENABLE */
#endif /* MUX_DELAY_EN && ICL_ENABLE */

static void Cy_PdAltMode_DP_CleanHpdQueue(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    ptrAltModeContext->dpStatus->hpdState = CY_PDALTMODE_MNGR_NO_DATA;
    ptrAltModeContext->dpStatus->queueReadIndex = CY_PDALTMODE_MNGR_NO_DATA;
    ptrAltModeContext->dpStatus->curHpdState = CY_PDALTMODE_MNGR_NO_DATA;
    TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_HPD_DELAY_TIMER));
#if DP_2_1_ENABLE
    /* Stop HPD WAIT timer */
    TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_HPD_WAIT_TIMER));
#endif /* DP_2_1_ENABLE */
}

static void Cy_PdAltMode_DP_DfpEnqueueHpd(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint32_t status)
{
#if (MUX_DELAY_EN && ICL_ENABLE) || (!VIRTUAL_HPD_ENABLE)
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
#endif /* MUX_DELAY_EN && ICL_ENABLE  !VIRTUAL_HPD_ENABLE*/

    uint8_t  prevQueueIdx  = ptrAltModeContext->dpStatus->queueReadIndex;
    uint16_t *hpdState      = &ptrAltModeContext->dpStatus->hpdState;
    uint8_t  *hpdIdx        = &ptrAltModeContext->dpStatus->queueReadIndex;

    /* Check whether IRQ queue is full */
    if (ptrAltModeContext->dpStatus->queueReadIndex < CY_PDALTMODE_DP_QUEUE_FULL_INDEX)
    {
        switch(CY_PDALTMODE_DP_GET_HPD_IRQ_STAT(status))
        {
            case CY_PDALTMODE_DP_HPD_LOW_IRQ_LOW:
                /* Empty queue */
                *hpdIdx = CY_PDALTMODE_DP_QUEUE_STATE_SIZE;
                *hpdState = (uint16_t)CY_USBPD_HPD_EVENT_UNPLUG;
                prevQueueIdx = CY_PDALTMODE_DP_QUEUE_EMPTY_INDEX;
                break;

            case CY_PDALTMODE_DP_HPD_HIGH_IRQ_LOW:
                /* Add to queue High HPD state */
                Cy_PdAltMode_DP_AddHpdEvt(ptrAltModeContext->dpStatus, CY_USBPD_HPD_EVENT_PLUG);
                break;
            case CY_PDALTMODE_DP_HPD_LOW_IRQ_HIGH:
            default:   //Actually CY_PDALTMODE_DP_HPD_HIGH_IRQ_HIGH
                /*
                 * Add to queue HPD HIGH if previous HPD level was LOW or it's
                 * a first HPD transaction. In other cases add only HPD IRQ
                 * event to queue.
                 */
                if (ptrAltModeContext->dpStatus->curHpdState < (uint8_t)CY_USBPD_HPD_EVENT_PLUG)
                {
                    Cy_PdAltMode_DP_AddHpdEvt(ptrAltModeContext->dpStatus, CY_USBPD_HPD_EVENT_PLUG);
                }
                Cy_PdAltMode_DP_AddHpdEvt(ptrAltModeContext->dpStatus, CY_USBPD_HPD_EVENT_IRQ);
                break;
        }
        /* Allow dequeue only if no HPD Dequeuing in the process */
        if (prevQueueIdx == CY_PDALTMODE_DP_QUEUE_EMPTY_INDEX)
        {
#if MUX_DELAY_EN && ICL_ENABLE
            /* Prevent quick back-to-back HPD changes */
            if (ptrAltModeContext->altModeAppStatus->isMuxBusy)
            {
                TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                        CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_HPD_DELAY_TIMER),
                            CY_PDALTMODE_APP_MUX_VDM_DELAY_TIMER_PERIOD, Cy_PdAltMode_DP_HpdDelayCbk);
            }
            else
#endif /* MUX_DELAY_EN && ICL_ENABLE */
#if !VIRTUAL_HPD_ENABLE
            if (Cy_USBPD_Hpdt_IsCommandActive(ptrPdStackContext->ptrUsbPdContext) == true)
#endif
            {
                Cy_PdAltMode_DP_DfpDequeueHpd(ptrAltModeContext);
            }
        }
    }
}

void Cy_PdAltMode_DP_DfpDequeueHpd(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
#if !VIRTUAL_HPD_ENABLE || DP_2_1_ENABLE
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
#endif /* !VIRTUAL_HPD_ENABLE */
    uint32_t hpdEvt = (uint32_t)CY_USBPD_HPD_EVENT_NONE;
    uint16_t hpdState       = ptrAltModeContext->dpStatus->hpdState;
    uint8_t  hpdIdx         = ptrAltModeContext->dpStatus->queueReadIndex;

#if DP_2_1_ENABLE
    if (TIMER_CALL_MAP(Cy_PdUtils_SwTimer_IsRunning) (ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_HPD_WAIT_TIMER)))
        return;
#endif /* DP_2_1_ENABLE */

#if AMD_RETIMER_ENABLE
    if (ptrAltModeContext->altModeAppStatus->muxStat == CY_PDALTMODE_APP_MUX_STATE_BUSY)
    {
        TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_HPD_DELAY_TIMER),
                    CY_PDALTMODE_APP_HPD_DEQUE_POLL_PERIOD, Cy_PdAltMode_DP_HpdDelayCbk);
        return;
    }
#endif /* AMD_RETIMER_ENABLE */
    
#if !VIRTUAL_HPD_ENABLE
    /* If current MUX state is not DP - HPD processing is not allowed */
    if (!ptrAltModeContext->dpStatus->dpActiveFlag)
    {
        (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_HPD_DELAY_TIMER),
                    CY_PDALTMODE_APP_HPD_DEQUE_POLL_PERIOD, Cy_PdAltMode_DP_HpdDelayCbk);
    }
#endif /* !VIRTUAL_HPD_ENABLE */
    
    if (hpdIdx != CY_PDALTMODE_DP_QUEUE_EMPTY_INDEX)
    {
        hpdEvt = (uint32_t)((uint32_t)CY_PDALTMODE_DP_HPD_STATE_MASK & ((uint32_t)hpdState >> (hpdIdx - (uint32_t)CY_PDALTMODE_DP_QUEUE_STATE_SIZE)));
        (void)Cy_PdAltMode_HW_EvalHpdCmd (ptrAltModeContext, hpdEvt);
        /* Save current HPD state */
        ptrAltModeContext->dpStatus->curHpdState = (uint8_t)hpdEvt;
    }
}

static void Cy_PdAltMode_DP_SrcHpdRespCbk(void *context, uint32_t event)
{
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)context;
#if DP_2_1_ENABLE
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    if (TIMER_CALL_MAP(Cy_PdUtils_SwTimer_IsRunning) (ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_HPD_WAIT_TIMER)))
        return;
#endif /* DP_2_1_ENABLE */
    if (((event & 0xFFFFUL) == (uint32_t)CY_USBPD_HPD_COMMAND_DONE) && (Cy_PdAltMode_DP_Info(ptrAltModeContext)->isActive == true))
    {
        if (ptrAltModeContext->dpStatus->queueReadIndex != CY_PDALTMODE_DP_QUEUE_EMPTY_INDEX)
        {
            ptrAltModeContext->dpStatus->queueReadIndex -= CY_PDALTMODE_DP_QUEUE_STATE_SIZE;

            if (
                   (ptrAltModeContext->dpStatus->queueReadIndex == CY_PDALTMODE_DP_QUEUE_EMPTY_INDEX) &&
                   (ptrAltModeContext->dpStatus->hpdState == (uint16_t)CY_USBPD_HPD_EVENT_UNPLUG)                &&
                   (ptrAltModeContext->dpStatus->curHpdState != (uint8_t)CY_USBPD_HPD_EVENT_UNPLUG)
               )
            {
                ptrAltModeContext->dpStatus->queueReadIndex = CY_PDALTMODE_DP_QUEUE_STATE_SIZE;
            }

#if MUX_DELAY_EN && ICL_ENABLE
            /* Prevent quick back-to-back HPD changes */
            if (ptrAltModeContext->altModeAppStatus->isMuxBusy)
            {
                TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrAltModeContext->pdStackContext->ptrTimerContext, ptrAltModeContext->pdStackContext,
                        CY_PDALTMODE_GET_TIMER_ID(ptrAltModeContext->pdStackContext, CY_PDALTMODE_HPD_DELAY_TIMER),
                            ptrAltModeContext->iclCfg->soc_mux_config_delay, Cy_PdAltMode_DP_HpdDelayCbk);
            }
            else
#endif /* MUX_DELAY_EN && ICL_ENABLE */
            {
                Cy_PdAltMode_DP_DfpDequeueHpd(ptrAltModeContext);
            }
        }
    }
}
#endif /* DP_DFP_SUPP */

/************************* DP Sink functions definitions **********************/

#if DP_UFP_SUPP

#if DP_GPIO_CONFIG_SELECT

/* Global to hold DP Pin configuration when GPIO based selection is enabled */
uint8_t glDpSinkConfig = 0;

void Cy_PdAltMode_DP_SinkSetPinConfig(uint8_t dpConfig)
{
    /* Store DP Configuration supported as DP Sink */
    glDpSinkConfig = dpConfig;
}

uint8_t Cy_PdAltMode_DP_SinkGetPinConfig(void)
{
    return glDpSinkConfig;
}
#endif /* DP_GPIO_CONFIG_SELECT */

#if GATKEX_CREEK
/*Added a timer callback to add a delay between ridge disconnect and DP Alt Mode entry*/
static void Cy_PdAltMode_DP_RidgeMuxDelayCbk (cy_timer_id_t id, void* ptrContext)
{
    (void)id;
    cy_stc_pdstack_context_t * ptrPdStackContext = (cy_stc_pdstack_context_t *) ptrContext;
    cy_stc_pdaltmode_context_t * ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    /* Stop Delay timer */
    TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, (uint16_t)CY_PDALTMODE_GR_MUX_DELAY_TIMER));

    /* Set MUX */
    (void)Cy_PdAltMode_HW_SetMux(ptrAltModeContext, ptrAltModeContext->dpStatus->dpMuxCfg,
                ptrAltModeContext->dpStatus->configVdo.val, ptrAltModeContext->dpStatus->statusVdo.val);

    Cy_PdAltMode_DP_AssignVdo(ptrAltModeContext, CY_PDALTMODE_MNGR_NONE_VDO);
    Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_IDLE;

    /* If UFP should respond to VDM then send VDM response */
    if ((ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP) && ptrAltModeContext->appStatusContext->isVdmPending)
    {
        ptrAltModeContext->appStatusContext->vdmResp.doCount = 1u;

        ptrAltModeContext->appStatusContext->vdmRespCbk(ptrPdStackContext, &ptrAltModeContext->appStatusContext->vdmResp);
        ptrAltModeContext->appStatusContext->isVdmPending = false;
    }
    ptrAltModeContext->altModeAppStatus->isMuxBusy = false;
}
#endif /* GATKEX_CREEK */

static void Cy_PdAltMode_DP_UfpRun(void *Context)
{
    uint8_t dpConfig;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)Context;

    /* Get Alt Modes state and VDM command */
    cy_en_pdaltmode_state_t dpModeState = Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState;
    ptrAltModeContext->dpStatus->state = (cy_en_pdaltmode_dp_state_t)Cy_PdAltMode_DP_Info(ptrAltModeContext)->vdmHeader.std_vdm_hdr.cmd;

    if (
            (Cy_PdAltMode_DP_Info(ptrAltModeContext)->vdmHeader.std_vdm_hdr.objPos != Cy_PdAltMode_DP_Info(ptrAltModeContext)->objPos) &&
            (dpModeState == CY_PDALTMODE_STATE_IDLE)
        )
    {
        /* Deny command processing if object position does not match */
        Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_FAIL;
        return;
    }

    /* Set idle as default */
    Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_IDLE;

    /* If exit all modes cmd received */
    if (Cy_PdAltMode_DP_Info(ptrAltModeContext)->vdmHeader.std_vdm_hdr.cmd == CY_PDALTMODE_MNGR_EXIT_ALL_MODES)
    {
        ptrAltModeContext->dpStatus->state = CY_PDALTMODE_DP_STATE_EXIT;
    }
    CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 16.1', 1, 'Intentional return instead of breaking the switch clause'); 
    CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 16.3', 1, 'Intentional return instead of breaking the switch clause');     
    switch (dpModeState)
    {
        case CY_PDALTMODE_STATE_IDLE:

            switch(ptrAltModeContext->dpStatus->state)
            {
                case CY_PDALTMODE_DP_STATE_ENTER:
                    /* Initialize HPD if we are not using HPD via I2C */
#if !VIRTUAL_HPD_ENABLE
                    /* Enable HPD receiver */
                    Cy_PdAltMode_DP_InitHpd (ptrAltModeContext, (uint8_t)CY_PD_PRT_TYPE_UFP);
#endif /* !VIRTUAL_HPD_ENABLE */

                    /* Fill Status Update with appropriate bits */
                    ptrAltModeContext->dpStatus->statusVdo.val = CY_PDALTMODE_MNGR_EMPTY_VDO;

                    /* Update UFP Enabled status field */
                    Cy_PdAltMode_DP_UfpSetStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_EN);
#if !DP_UFP_DONGLE
                    /* Update UFP connection status */
                    Cy_PdAltMode_DP_UfpSetStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_UFP_CONN);
#endif /* DP_UFP_MONITOR */

                    /* Check if Multi-function allowed */
                    if ((ptrAltModeContext->dpStatus->ccgDpPinsSupp & (CY_PDALTMODE_DP_DFP_D_CONFIG_D | CY_PDALTMODE_DP_DFP_D_CONFIG_F)) != CY_PDALTMODE_MNGR_NO_DATA)
                    {
                        Cy_PdAltMode_DP_UfpSetStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_MF);
                    }
                    
                    /* No VDO is needed in response */
                    Cy_PdAltMode_DP_AssignVdo(ptrAltModeContext, CY_PDALTMODE_MNGR_NONE_VDO);
                    return;

                    /* QAC suppression 2023: Intentional return from function
                     * instead of breaking the switch case clause. */
                case CY_PDALTMODE_DP_STATE_STATUS_UPDATE: /* PRQA S 2023 */
#if VIRTUAL_HPD_ENABLE
                    /* 
                     * Report HPD low in case of DP not enabled. This is because, HPD block itself may not
                     * be enabled at this stage.
                     */
                    if (!ptrAltModeContext->dpStatus->dpActiveFlag)
                    {
                        /* Not update hpd state as hpd not started yet */
                        Cy_PdAltMode_DP_UfpClearStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_HPD);
                    }
#else /* !VIRTUAL_HPD_ENABLE */
                    /* Update current HPD status */
                    (void)Cy_PdAltMode_DP_UfpUpdateStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_HPD,
                            Cy_PdAltMode_HW_DpSnkGetHpdState(ptrAltModeContext));
#endif /* VIRTUAL_HPD_ENABLE */
#if DP_UFP_DONGLE
#if DP_GPIO_CONFIG_SELECT
                    /*
                     * Update UFP Connected status based on DP Pin configuration. For NON-DP
                     * configuration, always report connected.
                     */
                    if (Cy_PdAltMode_DP_SinkGetPinConfig () == CY_PDALTMODE_DP_DFP_D_CONFIG_C)
                    {
                        Cy_PdAltMode_DP_UfpSetStatusField(port, CY_PDALTMODE_DP_STAT_UFP_CONN);
                    }
                    else
                    {
                        Cy_PdAltMode_DP_UfpUpdateStatusField(port, CY_PDALTMODE_DP_STAT_UFP_CONN, Cy_PdAltMode_HW_DpSnkGetHpdState(port));
                    }
#else
                    /*
                     * Update UFP Connected status based on DP Pin configuration. For NON-DP
                     * configuration, always report connected.
                     */
                    if (((ptrAltModeContext->dpStatus->ccgDpPinsSupp & CY_PDALTMODE_DP_DFP_D_CONFIG_C) != CY_PDALTMODE_MNGR_NO_DATA) ||
                            ((ptrAltModeContext->dpStatus->ccgDpPinsSupp & CY_PDALTMODE_DP_DFP_D_CONFIG_D) != CY_PDALTMODE_MNGR_NO_DATA))
                    {
                        /* Always report UFP connected */
                        Cy_PdAltMode_DP_UfpSetStatusField (port, CY_PDALTMODE_DP_STAT_UFP_CONN);
                    }
                    else
                    {
                        Cy_PdAltMode_DP_UfpUpdateStatusField (port, CY_PDALTMODE_DP_STAT_UFP_CONN,
                                Cy_PdAltMode_HW_DpSnkGetHpdState(port));
                    }
#endif /* DP_GPIO_CONFIG_SELECT */
#endif /* DP_UFP_DONGLE */

                    /* IRQ bit should be set to zero in status update response */
                    (void)Cy_PdAltMode_DP_UfpClearStatusField (ptrAltModeContext, CY_PDALTMODE_DP_STAT_IRQ);

                    /* Respond with current DP sink status */
                    Cy_PdAltMode_DP_AssignVdo(ptrAltModeContext, ptrAltModeContext->dpStatus->statusVdo.val);
                    return;

                    /* QAC suppression 2023: Intentional return from function
                     * instead of breaking the switch case clause. */
                case CY_PDALTMODE_DP_STATE_CONFIG: /* PRQA S 2023 */
                    /* Check if Config VDO is correct */
                    if (
                            /* QAC suppression 3415: These are read only checks and hence not side effects */
                            /* IF DP configuration */
                            ((Cy_PdAltMode_DP_GetVdo(ptrAltModeContext)->dp_cfg_vdo.selConf == CY_PDALTMODE_DP_CONFIG_SELECT) &&
                             (Cy_PdAltMode_DP_GetVdo(ptrAltModeContext)->dp_cfg_vdo.sign == CY_PDALTMODE_DP_1_3_SIGNALING)) || /* PRQA S 3415 */

                             /* If USB configuration requested */
                            (Cy_PdAltMode_DP_GetVdo(ptrAltModeContext)->dp_cfg_vdo.selConf == CY_PDALTMODE_DP_USB_CONFIG_SELECT) /* PRQA S 3415 */
                       )
                    {
                        /* Save port partner pin config */
                        ptrAltModeContext->dpStatus->partnerDpPinsSupp = (uint8_t)Cy_PdAltMode_DP_GetVdo(ptrAltModeContext)->dp_cfg_vdo.dfpAsgmnt;

#if DP_GPIO_CONFIG_SELECT
                        /* Pin configuration supported is based on GPIO status */
                        dpConfig = Cy_PdAltMode_DP_SinkGetPinConfig ();
#else
                        /* Get DP Pin configuration supported from config table */
                        dpConfig = ptrAltModeContext->dpCfg->dp_config_supported;
#endif /* DP_GPIO_CONFIG_SELECT */

                        /* Check if both UFP and DFP support selected pin configuration */
                        /* QAC suppression 3415: These are read only checks and hence not side effects */
                        if (
                                ((Cy_PdAltMode_DP_IsConfigCorrect(ptrAltModeContext)) &&
                                 ((dpConfig & ptrAltModeContext->dpStatus->partnerDpPinsSupp) != 0u)) ||
                                 (Cy_PdAltMode_DP_GetVdo(ptrAltModeContext)->dp_cfg_vdo.selConf == CY_PDALTMODE_DP_USB_CONFIG_SELECT) /* PRQA S 3415 */
                           )
                        {
                            /* Save config VDO */
                            ptrAltModeContext->dpStatus->configVdo = *(Cy_PdAltMode_DP_GetVdo(ptrAltModeContext));

                            /* Get DP MUX configuration */
                            ptrAltModeContext->dpStatus->dpMuxCfg = Cy_PdAltMode_DP_EvalConfig(ptrAltModeContext);

#if GATKEX_CREEK
                            /*
                             * To set the ridge to disconnect between USB and DP states when device is UFP.
                             * For DP Alt Mode, GR is updated in DP_CONFIG command instead of ENTER_MODE command.
                             * Before entering DP mode, GR disconnect is done and a 10 ms delay is added using a timer.
                             */
                            if(ptrAltModeContext->pdStackContext->port == TYPEC_PORT_0_IDX)
                            {
                                Cy_PdAltMode_Ridge_SetDisconnect(ptrAltModeContext);
                                ptrAltModeContext->altModeAppStatus->isMuxBusy = true;
                                (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrAltModeContext->pdStackContext->ptrTimerContext, ptrAltModeContext->pdStackContext,
                                        CY_PDALTMODE_GET_TIMER_ID(ptrAltModeContext->pdStackContext, (uint16_t)CY_PDALTMODE_GR_MUX_DELAY_TIMER),
                                        CY_PDALTMODE_RIDGE_GR_MUX_VDM_DELAY_TIMER_PERIOD, Cy_PdAltMode_DP_RidgeMuxDelayCbk);
                            }

                            if(!ptrAltModeContext->altModeAppStatus->isMuxBusy)
#endif /* GATKEX_CREEK */
                            {
                                /* Set DP MUX */
                                (void)Cy_PdAltMode_HW_SetMux(ptrAltModeContext, ptrAltModeContext->dpStatus->dpMuxCfg,
                                                                ptrAltModeContext->dpStatus->configVdo.val, ptrAltModeContext->dpStatus->statusVdo.val);
                                Cy_PdAltMode_DP_AssignVdo(ptrAltModeContext, CY_PDALTMODE_MNGR_NONE_VDO);
                                Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_IDLE;
                            }
#if CY_PD_DP_VCONN_SWAP_FEATURE
                            /* Stop timer to avoid hard reset */
                            TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrAltModeContext->pdStackContext->ptrTimerContext,
                                    CY_PDSTACK_GET_PD_TIMER_ID(ptrAltModeContext->pdStackContext, CY_PDSTACK_PD_VCONN_RECOVERY_TIMER));

                            /* Check if USB config received as part of Vconn Swap flow */
                            if (((Cy_PdAltMode_DP_GetVdo(ptrAltModeContext)->dp_cfg_vdo.selConf == CY_PDALTMODE_DP_USB_CONFIG_SELECT)) && ((ptrAltModeContext->dpStatus->vconnSwapReq != 0x0U)))
                            {
                                /* Start timer to run Vconn Swap */
                                (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start) (ptrAltModeContext->pdStackContext->ptrTimerContext, ptrAltModeContext->pdStackContext,
                                        CY_PDSTACK_GET_PD_TIMER_ID(ptrAltModeContext->pdStackContext, CY_PDSTACK_PD_VCONN_RECOVERY_TIMER),
                                            ptrAltModeContext->vdmStat->vdmBusyDelay, Cy_PdAltMode_DP_VconnSwapInitCbk);
                                return;
                            }
#endif /* CY_PD_DP_VCONN_SWAP_FEATURE */

#if VIRTUAL_HPD_ENABLE
                            /* Init HPD - Do deferred HPD initialization when using ridge MUX */
                            Cy_PdAltMode_DP_InitHpd(ptrAltModeContext, (uint8_t)CY_PD_PRT_TYPE_UFP);
#endif /* VIRTUAL_HPD_ENABLE */

                            /* If HPD is HIGH then send Attention */
                            if  (
#if (!ICL_ENABLE)
                                    (
#if VIRTUAL_HPD_ENABLE
                                     (Cy_PdAltMode_HW_IsHostHpdVirtual(ptrAltModeContext)) ||
#endif /* VIRTUAL_HPD_ENABLE */
                                     Cy_PdAltMode_HW_DpSnkGetHpdState(ptrAltModeContext)
                                    ) &&
#endif /* (!ICL_ENABLE) */
                                    (ptrAltModeContext->dpStatus->queueReadIndex == CY_PDALTMODE_MNGR_NO_DATA)
#if VIRTUAL_HPD_DOCK
                                    && Cy_PdAltMode_Ridge_GetHpdState(ptrAltModeContext)
#endif
                                )
                            {
                                /* If HPD already HIGH then add it to queue */
                                Cy_PdAltMode_DP_AddHpdEvt(ptrAltModeContext->dpStatus, CY_USBPD_HPD_EVENT_PLUG);
                                Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_RUN;
                            }
                            return;
                        }
                    }
                    break;

                case CY_PDALTMODE_DP_STATE_EXIT:
                    /* Deinit Alt Mode and reset gl_dp variables */
                    Cy_PdAltMode_DP_Exit(ptrAltModeContext);
                    Cy_PdAltMode_DP_AssignVdo(ptrAltModeContext, CY_PDALTMODE_MNGR_NONE_VDO);
                    return;

                    /* QAC suppression 2023: Intentional return from function
                     * instead of breaking the switch case clause. */
                default: /* PRQA S 2023 */
                    break;
            }
            break;

        case CY_PDALTMODE_STATE_WAIT_FOR_RESP:
            /* Analyse Attention callback */
            if (ptrAltModeContext->dpStatus->state == CY_PDALTMODE_DP_STATE_ATT)
            {
                /* Clear Request and IRQ status bits */
                Cy_PdAltMode_DP_UfpClearStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_USB_CNFG);
                Cy_PdAltMode_DP_UfpClearStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_EXIT);
                Cy_PdAltMode_DP_UfpClearStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_IRQ);

#if VIRTUAL_HPD_ENABLE
                if (
#if (!ICL_ENABLE)
                        (!Cy_PdAltMode_HW_IsHostHpdVirtual(ptrAltModeContext)) ||
#endif /* (!ICL_ENABLE) */
                        (Cy_PdAltMode_DP_GetVdo(ptrAltModeContext)->dp_stat_vdo.hpdIrq == 1u)
                   )
#endif /* VIRTUAL_HPD_ENABLE */
                {
                    (void)Cy_PdAltMode_HW_EvalHpdCmd (ptrAltModeContext, (uint32_t)CY_USBPD_HPD_COMMAND_DONE);
                }

                /* Check if any HPD event in queue */
                Cy_PdAltMode_DP_UfpDequeueHpd(ptrAltModeContext);
                return;
            }
            return;

            /* QAC suppression 2023: Intentional return from function
             * instead of breaking the switch case clause. */
        case CY_PDALTMODE_STATE_RUN: /* PRQA S 2023 */

            /* Send Attention if HPD is High while DP initialization */
            if (ptrAltModeContext->dpStatus->hpdState != CY_PDALTMODE_MNGR_NO_DATA)
            {
                Cy_PdAltMode_DP_UfpDequeueHpd(ptrAltModeContext);
            }
            return;

            /* QAC suppression 2023: Intentional return from function
             * instead of breaking the switch case clause. */
        default: /* PRQA S 2023 */
            break;
    }
    CY_MISRA_BLOCK_END('MISRA C-2012 Rule 16.3');
    CY_MISRA_BLOCK_END('MISRA C-2012 Rule 16.1');    

    /* Send NACK */
    Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_FAIL;
}

static bool Cy_PdAltMode_DP_IsConfigCorrect(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    bool retval;

    /* If input configuration is valid then return true */
    switch (ptrAltModeContext->dpStatus->partnerDpPinsSupp)
    {
        case CY_PDALTMODE_DP_DFP_D_CONFIG_C:
        case CY_PDALTMODE_DP_DFP_D_CONFIG_D:
        case CY_PDALTMODE_DP_DFP_D_CONFIG_E:
        case CY_PDALTMODE_DP_DFP_D_CONFIG_F:
            retval = true;
            break;
        default:
            retval = false;
            break;
    }

    return retval;
}

static inline void Cy_PdAltMode_DP_UfpSetStatusField(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_dp_stat_bm_t bitPos)
{
    CY_PDALTMODE_MNGR_SET_FLAG(ptrAltModeContext->dpStatus->statusVdo.val, (uint8_t)bitPos);
}

static inline void Cy_PdAltMode_DP_UfpClearStatusField(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_dp_stat_bm_t bitPos)
{
    CY_PDALTMODE_MNGR_REMOVE_FLAG(ptrAltModeContext->dpStatus->statusVdo.val, (uint8_t)bitPos);
}

static bool Cy_PdAltMode_DP_UfpUpdateStatusField(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_dp_stat_bm_t bitPos, bool status)
{
    uint32_t    prevStatus;

    /* Save current DP sink status */
    prevStatus = ptrAltModeContext->dpStatus->statusVdo.val;

    /* Update status field*/
    if (status)
    {
        CY_PDALTMODE_MNGR_SET_FLAG(ptrAltModeContext->dpStatus->statusVdo.val, (uint8_t)bitPos);
    }
    else
    {
        CY_PDALTMODE_MNGR_REMOVE_FLAG(ptrAltModeContext->dpStatus->statusVdo.val, (uint8_t)bitPos);
    }

    /* Check if status changed */
    return (prevStatus != ptrAltModeContext->dpStatus->statusVdo.val);
}

static void Cy_PdAltMode_DP_SnkHpdRespCbk(void *context, uint32_t cmd)
{
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)context;
    cy_en_usbpd_hpd_events_t event = (cy_en_usbpd_hpd_events_t)cmd;

    /* Handle hpd only when DP sink was entered */
    if (
            /* QAC suppression 3415: These are read only checks and hence not side effects */
            (event > CY_USBPD_HPD_EVENT_NONE)   &&
            (event < CY_USBPD_HPD_COMMAND_DONE) &&
            (Cy_PdAltMode_DP_Info(ptrAltModeContext)->isActive != false)
       )
    {
        /* If HPD received after Status update command was sent then add to HPD queue */
        Cy_PdAltMode_DP_UfpEnqueueHpd(ptrAltModeContext, event);
    }
}

static void Cy_PdAltMode_DP_UfpEnqueueHpd(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_usbpd_hpd_events_t status)
{
    uint16_t *hpdState       = &ptrAltModeContext->dpStatus->hpdState;
    uint8_t  *hpdIdx         = &ptrAltModeContext->dpStatus->queueReadIndex;

    /* Check if queue is not full */
    if ((*hpdIdx) <= (CY_PDALTMODE_DP_UFP_MAX_QUEUE_SIZE * CY_PDALTMODE_DP_QUEUE_STATE_SIZE))
    {
        CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 16.1', 1, 'Intentional fall through and return.'); 
        CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 16.3', 1, 'Intentional fall through and return.');     
        switch (status)
        {
            case CY_USBPD_HPD_EVENT_UNPLUG:
                /* Empty queue */
                *hpdIdx = CY_PDALTMODE_DP_QUEUE_STATE_SIZE;
                *hpdState = (uint16_t) status;
                break;
            case CY_USBPD_HPD_EVENT_PLUG:
            case CY_USBPD_HPD_EVENT_IRQ:
                /* Add to queue High HPD state or IRQ */
                Cy_PdAltMode_DP_AddHpdEvt(ptrAltModeContext->dpStatus, status);
                break;

            /* QAC suppression 2024: Intentional return from function 
             * instead of breaking the switch case clause. */
            default: /* PRQA S 2024 */
                /* Do nothing */
                return;
        }
        CY_MISRA_BLOCK_END('MISRA C-2012 Rule 16.3');
        CY_MISRA_BLOCK_END('MISRA C-2012 Rule 16.1');

        /* If any Attention already is in sending process then halt dequeue procedure */
        if (Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState == CY_PDALTMODE_STATE_IDLE)
        {
            /* Dequeue HPD events */
            Cy_PdAltMode_DP_UfpDequeueHpd(ptrAltModeContext);
        }
    }
}
    
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.2', 1, 'Intentional recursion with a maximum of 5 recursive calls');
static void Cy_PdAltMode_DP_UfpDequeueHpd(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_en_usbpd_hpd_events_t hpdEvt;
    uint16_t hpdEvtU16;
    bool    isAttNeeded    = false;
    uint16_t hpdState       = ptrAltModeContext->dpStatus->hpdState;
    uint8_t  hpdIdx         = ptrAltModeContext->dpStatus->queueReadIndex;

    if (hpdIdx != (uint8_t)CY_PDALTMODE_DP_QUEUE_EMPTY_INDEX)
    {
        /* Get queued HPD event */
        hpdEvtU16 = (CY_PDALTMODE_DP_HPD_STATE_MASK & (hpdState >>
                    (hpdIdx - CY_PDALTMODE_DP_QUEUE_STATE_SIZE)));
        hpdEvt = (cy_en_usbpd_hpd_events_t)hpdEvtU16;

        /* Decrease queue size */
        hpdIdx -= CY_PDALTMODE_DP_QUEUE_STATE_SIZE;

        /* Get queued HPD event */
        switch(hpdEvt)
        {
            case CY_USBPD_HPD_EVENT_UNPLUG:
                /* Update DP sink status */
                if (Cy_PdAltMode_DP_UfpUpdateStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_HPD, false))
                {
#if DP_UFP_DONGLE
#if DP_GPIO_CONFIG_SELECT
                    /* For DP Pin configuration, ensure that UFP Connected status bit is updated */
                    if (Cy_PdAltMode_DP_SinkGetPinConfig () == CY_PDALTMODE_DP_DFP_D_CONFIG_E)
                    {
                        Cy_PdAltMode_DP_UfpClearStatusField(port, CY_PDALTMODE_DP_STAT_UFP_CONN);
                    }
#else
                    if (((ptrAltModeContext->dpStatus->ccgDpPinsSupp & CY_PDALTMODE_DP_DFP_D_CONFIG_C) != CY_PDALTMODE_MNGR_NO_DATA) ||
                            ((ptrAltModeContext->dpStatus->ccgDpPinsSupp & CY_PDALTMODE_DP_DFP_D_CONFIG_D) != CY_PDALTMODE_MNGR_NO_DATA))
                    {
                        /* Always report UFP connected */
                        Cy_PdAltMode_DP_UfpSetStatusField (port, CY_PDALTMODE_DP_STAT_UFP_CONN);
                    }
                    else
                    {
                        /* Update UFP connection status */
                        Cy_PdAltMode_DP_UfpClearStatusField(port, CY_PDALTMODE_DP_STAT_UFP_CONN);
                    }
#endif /*  DP_GPIO_CONFIG_SELECT */
#endif /* DP_UFP_DONGLE */
                    /* Check flag to send Attention VDM */
                    isAttNeeded = true;
                }
                /* Zero IRQ status if Unattached */
                Cy_PdAltMode_DP_UfpClearStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_IRQ);
                break;
            case CY_USBPD_HPD_EVENT_PLUG:
                /* Update DP sink status */
                if (Cy_PdAltMode_DP_UfpUpdateStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_HPD, true))
                {
#if DP_UFP_DONGLE
#if DP_GPIO_CONFIG_SELECT
                    /* For DP Pin configuration, ensure that UFP Connected status bit is updated */
                    if (Cy_PdAltMode_DP_SinkGetPinConfig () == CY_PDALTMODE_DP_DFP_D_CONFIG_E)
                    {
                        Cy_PdAltMode_DP_UfpSetStatusField(port, CY_PDALTMODE_DP_STAT_UFP_CONN);
                    }
#else
                    /* Update UFP connection status */
                    Cy_PdAltMode_DP_UfpSetStatusField(port, CY_PDALTMODE_DP_STAT_UFP_CONN);
#endif /*  DP_GPIO_CONFIG_SELECT */
#endif /* DP_UFP_DONGLE */
                    /* Check flag to send Attention VDM */
                    isAttNeeded = true;
                }
                /* If next queued event is IRQ we can combine in one Attention */
                if ((((CY_PDALTMODE_DP_HPD_STATE_MASK & (hpdState >>
                                        (hpdIdx - CY_PDALTMODE_DP_QUEUE_STATE_SIZE)))) == (uint16_t)CY_USBPD_HPD_EVENT_IRQ) &&

                        /* QAC suppression 2995: This function is always called when the events are 
                         * present in queue and hence the hpdIdx is never expected to be DP_QUEUE_EMPTY_INDEX.
                         * Still this check is redundantly kept for protection as it is decrementing the hpdIdx
                         * to ensure that it is never negative. */
                        (hpdIdx != (uint8_t)CY_PDALTMODE_DP_QUEUE_EMPTY_INDEX)) /* PRQA S 2995 */
                {
                    /* Decrease queue size */
                    hpdIdx -= CY_PDALTMODE_DP_QUEUE_STATE_SIZE;

                    /* Update IRQ field in status */
                    Cy_PdAltMode_DP_UfpSetStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_IRQ);
                    isAttNeeded = true;
                }
                else
                {
                    /* Zero IRQ status */
                    Cy_PdAltMode_DP_UfpClearStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_IRQ);
                }
                break;
            case CY_USBPD_HPD_EVENT_IRQ:
                /* Update DP sink status */
                if (Cy_PdAltMode_DP_UfpUpdateStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_IRQ, true))
                {
                    /* Check flag to send Attention VDM */
                    isAttNeeded = true;
                }
                break;
            default:
                /* No statement */
                break;
        }
        ptrAltModeContext->dpStatus->hpdState = hpdState;
        ptrAltModeContext->dpStatus->queueReadIndex = hpdIdx;

        /* If status changed then send attention */
        if (isAttNeeded)
        {
            /* Copy Status VDO to VDO buffer send Attention VDM */
            ptrAltModeContext->dpStatus->state = CY_PDALTMODE_DP_STATE_ATT;
            Cy_PdAltMode_DP_AssignVdo(ptrAltModeContext, ptrAltModeContext->dpStatus->statusVdo.val);
            Cy_PdAltMode_DP_SendCmd(ptrAltModeContext);
            return;
        }
        /*
         * If Attention for current event not needed, but there are some events
         * left in queue then run dequeue.
         */
        else if (hpdIdx != (uint8_t)CY_PDALTMODE_DP_QUEUE_EMPTY_INDEX)
        {
            /* QAC suppression 3670: The queue is finite sized hence there can be maximum 5 
             * recursive calls, which will not result in a stack overflow. */
            Cy_PdAltMode_DP_UfpDequeueHpd(ptrAltModeContext);
        }
        else
        {
            /* No statement */
        }
    }
    Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_IDLE;
}
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.2');

#if CY_PD_DP_VCONN_SWAP_FEATURE
static void Cy_PdAltMode_DP_DpmCmdFailHdlr (cy_stc_pdstack_context_t * ptrPdStackContext, uint8_t *cntPtr)
{
    cy_stc_pdaltmode_context_t * ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    (*cntPtr)++;
    if ((*cntPtr) < CY_PDALTMODE_MNGR_MAX_RETRY_CNT)
    {
        (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start) (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_PDSTACK_GET_PD_TIMER_ID(ptrPdStackContext, CY_PDSTACK_PD_VCONN_RECOVERY_TIMER),
                    ptrAltModeContext->vdmStat->vdmBusyDelay, Cy_PdAltMode_DP_VconnSwapInitCbk);
    }
    else
    {
        (*cntPtr) = CY_PDALTMODE_MNGR_NO_DATA;
        /* Init Hard reset if Vconn Swap failed */
        (void)PDSTACK_CALL_MAP(Cy_PdStack_Dpm_SendPdCommand)(ptrPdStackContext, CY_PDSTACK_DPM_CMD_SEND_HARD_RESET, NULL, false, NULL);
    }
}

static void Cy_PdAltMode_DP_VconnSwapInitCbk (cy_timer_id_t id, void *ptrContext)
{
    (void)id;

    cy_stc_pdstack_context_t * ptrPdStackContext = (cy_stc_pdstack_context_t *) ptrContext;
    cy_stc_pdaltmode_context_t * ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    /* Send Vconn Swap command */
    if(PDSTACK_CALL_MAP(Cy_PdStack_Dpm_SendPdCommand)(ptrPdStackContext, CY_PDSTACK_DPM_CMD_SEND_VCONN_SWAP, NULL, true, Cy_PdAltMode_DP_VconnSwapCbk) != CY_PDSTACK_STAT_SUCCESS)
    {
        Cy_PdAltMode_DP_DpmCmdFailHdlr(ptrPdStackContext, &(ptrAltModeContext->dpStatus->vconnInitRetryCnt));
    }
}

static void Cy_PdAltMode_DP_ConfigAttFailedCbk(cy_timer_id_t id, void *ptrContext)
{
    (void)id;

    /* Init Hard reset if configure command not received */
    (void)PDSTACK_CALL_MAP(Cy_PdStack_Dpm_SendPdCommand)((cy_stc_pdstack_context_t *)ptrContext, CY_PDSTACK_DPM_CMD_SEND_HARD_RESET, NULL, false, NULL);
}

static void Cy_PdAltMode_DP_ConfigAttInitCbk (cy_timer_id_t id, void *ptrContext)
{
    (void)id;

    cy_stc_pdstack_context_t * ptrPdStackContext = (cy_stc_pdstack_context_t *) ptrContext;
    cy_stc_pdaltmode_context_t * ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    /* Send Attention VDM with DP config request */
    ptrAltModeContext->dpStatus->state = CY_PDALTMODE_DP_STATE_ATT;
    Cy_PdAltMode_DP_AssignVdo(ptrAltModeContext, ptrAltModeContext->dpStatus->statusVdo.val);
    Cy_PdAltMode_DP_SendCmd(ptrAltModeContext);

    /* Start timer to make sure that DFP sent configuration command */
    (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start) (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
            CY_PDSTACK_GET_PD_TIMER_ID(ptrPdStackContext, CY_PDSTACK_PD_VCONN_RECOVERY_TIMER),
                CY_PDALTMODE_APP_UFP_RECOV_VCONN_SWAP_TIMER_PERIOD, Cy_PdAltMode_DP_ConfigAttFailedCbk);
}

static void Cy_PdAltMode_DP_VconnSwapCbk(cy_stc_pdstack_context_t * ptrPdStackContext, cy_en_pdstack_resp_status_t resp, const cy_stc_pdstack_pd_packet_t* pktPtr)
{
    (void)pktPtr;

    cy_stc_pdaltmode_context_t * ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;
    cy_stc_pdaltmode_dp_status_t *dpStatus = ptrAltModeContext->dpStatus;

    /* Stop current Vconn timer */
    TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrPdStackContext->ptrTimerContext, CY_PDSTACK_GET_PD_TIMER_ID(ptrPdStackContext, CY_PDSTACK_PD_VCONN_RECOVERY_TIMER));

    dpStatus->vconnInitRetryCnt = CY_PDALTMODE_MNGR_NO_DATA;

    if (resp == CY_PDSTACK_RES_RCVD)
    {
        dpStatus->vconnCbkRetryCnt = CY_PDALTMODE_MNGR_NO_DATA;

        /* Start timer to run DP config Attention */
        (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start) (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_PDSTACK_GET_PD_TIMER_ID(ptrPdStackContext, CY_PDSTACK_PD_VCONN_RECOVERY_TIMER),
                    CY_PDALTMODE_APP_UFP_RECOV_VCONN_SWAP_TIMER_PERIOD, Cy_PdAltMode_DP_ConfigAttInitCbk);
        dpStatus->vconnSwapReq = 0x0U;
    }
    else if (resp < CY_PDSTACK_CMD_SENT)
    {
        Cy_PdAltMode_DP_DpmCmdFailHdlr(ptrPdStackContext, &(dpStatus->vconnCbkRetryCnt));
    }
    else
    {
        /* Do nothing */
    }
}

#endif /* CY_PD_DP_VCONN_SWAP_FEATURE */
#endif /* DP_UFP_SUPP */

/************************* Common DP functions definitions ********************/

#if ((DP_DFP_SUPP) || (DP_UFP_SUPP))
 
#if MUX_UPDATE_PAUSE_FSM
static void Cy_PdAltMode_DP_SendExitCmdCb(cy_timer_id_t id, void* context)
{
    (void)id;
    cy_stc_pdstack_context_t * ptrPdStackContext = (cy_stc_pdstack_context_t *) context;
    cy_stc_pdaltmode_context_t * ptrAltModeContext = (cy_stc_pdaltmode_context_t *) ptrPdStackContext->ptrAltModeContext;

    if (ptrAltModeContext->altModeAppStatus->muxStat == CY_PDALTMODE_APP_MUX_STATE_BUSY)
    {
        /* Run timer if MUX is busy */
        TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_ALT_MODE_CBK_TIMER),
                    CY_PDALTMODE_APP_POLL_PERIOD, Cy_PdAltMode_DP_SendExitCmdCb);
    }
    else
    {
        /* Set exit state and send Exit VDM */
        ptrAltModeContext->dpStatus->state = CY_PDALTMODE_DP_STATE_EXIT;
        Cy_PdAltMode_DP_SendCmd(ptrAltModeContext);
    }
}
#endif /* MUX_UPDATE_PAUSE_FSM */
    
static void Cy_PdAltMode_DP_SendCmd(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
#if DP_2_1_ENABLE || MUX_UPDATE_PAUSE_FSM
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
#endif /* DP_2_1_ENABLE || MUX_UPDATE_PAUSE_FSM */
    cy_stc_pdaltmode_mngr_info_t *info;
    uint8_t sopIdx;
    cy_en_pdaltmode_dp_state_t state = ptrAltModeContext->dpStatus->state;

    if (state != CY_PDALTMODE_DP_STATE_IDLE)
    {
        info = Cy_PdAltMode_DP_Info(ptrAltModeContext);
        
        /* QAC suppression 2982: val is a union object that holds the complete data of this union.
         * It is used here to clear the data of this union. Application uses other members of this 
         * union to specifically access the data required. */
        info->vdmHeader.val = CY_PDALTMODE_MNGR_NO_DATA; /* PRQA S 2982 */
        info->vdmHeader.std_vdm_hdr.cmd = (uint32_t)state;
        info->vdmHeader.std_vdm_hdr.svid = CY_PDALTMODE_DP_SVID;

        /* Set USB SS in case of Exit mode */
        if (state == CY_PDALTMODE_DP_STATE_EXIT)
        {
#if MUX_UPDATE_PAUSE_FSM
            if (ptrAltModeContext->altModeAppStatus->muxStat == CY_PDALTMODE_APP_MUX_STATE_BUSY)
            {
                /* Run timer if MUX is busy */
                TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                        CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_ALT_MODE_CBK_TIMER),
                            CY_PDALTMODE_APP_POLL_PERIOD, Cy_PdAltMode_DP_SendExitCmdCb);
            }
            else
#endif /* MUX_UPDATE_PAUSE_FSM */
            {
                (void)Cy_PdAltMode_HW_SetMux(ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SS_ONLY, CY_PDALTMODE_MNGR_NO_DATA, CY_PDALTMODE_MNGR_NO_DATA);
            }
        }
#if DP_DFP_SUPP
        for (sopIdx = (uint8_t)CY_PD_SOP; sopIdx <= ptrAltModeContext->dpStatus->maxSopSupp; sopIdx++)
#else
        sopIdx = CY_PD_SOP;
#endif /* DP_DFP_SUPP */
        {
            info->sopState[sopIdx] = CY_PDALTMODE_STATE_SEND;

            if ((state == CY_PDALTMODE_DP_STATE_ENTER) || (state == CY_PDALTMODE_DP_STATE_EXIT))
            {
                /* No parameters for the ENTER_MODE and EXIT_MODE VDMs */
                info->vdoNumb[sopIdx] = CY_PDALTMODE_MNGR_NO_DATA;
            }
            else
            {
                /* Status Update, Configure and Attention messages take one parameter Data Object */
                info->vdoNumb[sopIdx] = CY_PDALTMODE_MAX_DP_VDO_NUMB;
#if DP_DFP_SUPP
                if ((state == CY_PDALTMODE_DP_STATE_STATUS_UPDATE) && (sopIdx > (uint8_t)CY_PD_SOP))
                {
                    /* Status Update VDMs are only sent to the Port Partner */
                    info->sopState[sopIdx] = CY_PDALTMODE_STATE_IDLE;
                }
#endif /* DP_DFP_SUPP */
#if DP_2_1_ENABLE
                else if ((state == CY_PDALTMODE_DP_STATE_CONFIG) && (sopIdx > CY_PD_SOP))
                {
                    if (ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_PAS_CBL)
                    {
                        /* Config VDM can't be sent to Passive cable */
                        info->sopState[sopIdx] = CY_PDALTMODE_STATE_IDLE;
                    }
                    else
                    {
                        /* Clean unused configuration fields for cable */
                        info->vdo[sopIdx]->val &= CY_PDALTMODE_DP_2_1_CBL_CFG_MASK;
                        /* Set signaling bit for cable if it's not USB configuration */
                        if (info->vdo[sopIdx]->dp_cfg_vdo.selConf != CY_PDALTMODE_DP_USB_SS_CONFIG)
                        {
                            info->vdo[sopIdx]->dp_cfg_vdo.sign = CY_PDALTMODE_DP_1_3_SIGNALING;
                        }
                    }
                }
#endif /* DP_2_1_ENABLE */
            }
        }

        /* Start sending the VDMs out */
        info->modeState = CY_PDALTMODE_STATE_SEND;
    }
}

static inline cy_stc_pdaltmode_mngr_info_t* Cy_PdAltMode_DP_Info(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    return &ptrAltModeContext->dpStatus->info;
}

static void Cy_PdAltMode_DP_AddHpdEvt(cy_stc_pdaltmode_dp_status_t *dpStatus, cy_en_usbpd_hpd_events_t evt)
{
    dpStatus->hpdState <<= CY_PDALTMODE_DP_QUEUE_STATE_SIZE;
    dpStatus->hpdState |= (uint16_t) evt;
    dpStatus->queueReadIndex += CY_PDALTMODE_DP_QUEUE_STATE_SIZE;
}

#if DP_DFP_SUPP
static bool Cy_PdAltMode_DP_IsPrtnrCcgConsistent(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t config)
{
    return (
            /* DP MUX pin */
            ((ptrAltModeContext->dpStatus->ccgDpPinsSupp & ptrAltModeContext->dpStatus->partnerDpPinsSupp & config) != 0x0U) ||
            /* USB config */
            (config == CY_PDALTMODE_MNGR_NO_DATA)
       );
}
#endif /* DP_DFP_SUPP */

static cy_en_pdaltmode_mux_select_t Cy_PdAltMode_DP_EvalConfig(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_en_pdaltmode_mux_select_t ret = CY_PDALTMODE_MUX_CONFIG_SS_ONLY;
    cy_stc_pdaltmode_dp_status_t *dpStatus = ptrAltModeContext->dpStatus;
    
    switch (dpStatus->configVdo.dp_cfg_vdo.dfpAsgmnt)
    {
        case CY_PDALTMODE_DP_DFP_D_CONFIG_C:
        case CY_PDALTMODE_DP_DFP_D_CONFIG_E:
            dpStatus->dpActiveFlag = true;
            dpStatus->dp2LaneActive = false;
            ret = CY_PDALTMODE_MUX_CONFIG_DP_4_LANE;
            break;

        case CY_PDALTMODE_DP_DFP_D_CONFIG_D:
        case CY_PDALTMODE_DP_DFP_D_CONFIG_F:
            dpStatus->dpActiveFlag = true;
            dpStatus->dp2LaneActive = true;
            ret = CY_PDALTMODE_MUX_CONFIG_DP_2_LANE;
            break;

        case CY_PDALTMODE_DP_USB_SS_CONFIG:
            dpStatus->dpActiveFlag = false;
            dpStatus->dp2LaneActive = false;
            break;

        default:
            /* No statement */
            break;
    }

    return ret;
}

static void Cy_PdAltMode_DP_ResetVar(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    cy_stc_pdaltmode_dp_status_t *dpStatus = ptrAltModeContext->dpStatus;

    /* Zeros DP flags */
    dpStatus->dpActiveFlag = false;
    dpStatus->dp2LaneActive = false;
    dpStatus->configVdo.val = CY_PDALTMODE_MNGR_EMPTY_VDO;
    dpStatus->statusVdo.val = CY_PDALTMODE_MNGR_EMPTY_VDO;
    dpStatus->hpdState = 0u;
    dpStatus->queueReadIndex = 0u;
    dpStatus->curHpdState = 0u;
    dpStatus->state = CY_PDALTMODE_DP_STATE_IDLE;
    dpStatus->dp21Supp = 0u;
    dpStatus->dpMuxCfg = CY_PDALTMODE_MUX_CONFIG_ISOLATE;
    Cy_PdAltMode_DP_AssignVdo(ptrAltModeContext, CY_PDALTMODE_MNGR_NONE_VDO);
#if CY_PD_DP_VCONN_SWAP_FEATURE
    dpStatus->vconnSwapReq = 0x0U;
    dpStatus->vconnInitRetryCnt = CY_PDALTMODE_MNGR_NO_DATA;
    dpStatus->vconnCbkRetryCnt = CY_PDALTMODE_MNGR_NO_DATA;
#endif /* CY_PD_DP_VCONN_SWAP_FEATURE */
#if DP_DFP_SUPP
    dpStatus->dpExit = false;
    dpStatus->maxSopSupp = (uint8_t)CY_PD_SOP;
    dpStatus->tgtInfoPtr = NULL;
    dpStatus->dpActCblSupp = false;
#endif /* DP_DFP_SUPP */
#if MUX_UPDATE_PAUSE_FSM
    dpStatus->prevState    = CY_PDALTMODE_DP_STATE_IDLE;
    TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_ALT_MODE_CBK_TIMER));
#endif /* MUX_UPDATE_PAUSE_FSM */
    TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_HPD_DELAY_TIMER));
#if DP_2_1_ENABLE
    TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_HPD_WAIT_TIMER));
#endif /* DP_2_1_ENABLE */
}

static void Cy_PdAltMode_DP_AssignVdo(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint32_t vdo)
{
    cy_stc_pdaltmode_dp_status_t *dpStatus = ptrAltModeContext->dpStatus;
    /* No DP VDOs needed while send VDM */
    if (vdo == CY_PDALTMODE_MNGR_NONE_VDO)
    {
        dpStatus->info.vdoNumb[CY_PD_SOP] = 0;
#if DP_DFP_SUPP
        dpStatus->info.vdoNumb[CY_PD_SOP_PRIME]  = 0;
        dpStatus->info.vdoNumb[CY_PD_SOP_DPRIME] = 0;
#else
        Cy_PdAltMode_DP_Info(ptrAltModeContext)->vdoNumb[CY_PD_SOP] = 0;
#endif /* DP_DFP_SUPP */
    }
    else
    {
        /* Include given VDO as part of VDM */
        dpStatus->vdo[CY_PDALTMODE_DP_VDO_IDX].val = vdo;
        Cy_PdAltMode_DP_Info(ptrAltModeContext)->vdoNumb[CY_PD_SOP] = CY_PDALTMODE_MAX_DP_VDO_NUMB;

#if DP_DFP_SUPP
        if (dpStatus->dpActCblSupp)
        {
            dpStatus->cableVdo[CY_PDALTMODE_DP_VDO_IDX].val = vdo;
            Cy_PdAltMode_DP_Info(ptrAltModeContext)->vdoNumb[CY_PD_SOP_PRIME] = CY_PDALTMODE_MAX_DP_VDO_NUMB;
            if (dpStatus->maxSopSupp == (uint8_t)CY_PD_SOP_DPRIME)
            {
                Cy_PdAltMode_DP_Info(ptrAltModeContext)->vdoNumb[CY_PD_SOP_DPRIME] = CY_PDALTMODE_MAX_DP_VDO_NUMB;
            }
        }
#endif /* DP_DFP_SUPP */
    }
}

static cy_pd_pd_do_t* Cy_PdAltMode_DP_GetVdo(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    return &(ptrAltModeContext->dpStatus->vdo[CY_PDALTMODE_DP_VDO_IDX]);
}

static void Cy_PdAltMode_DP_Exit(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    Cy_PdAltMode_DP_ResetVar(ptrAltModeContext);
    (void)Cy_PdAltMode_HW_EvalHpdCmd(ptrAltModeContext, (uint32_t)CY_PDALTMODE_HPD_DISABLE_CMD);
    Cy_PdAltMode_HW_SetCbk(ptrAltModeContext, NULL);
    Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_EXIT;
}

static void Cy_PdAltMode_DP_InitHpd(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t portRole)
{
    (void)Cy_PdAltMode_HW_EvalHpdCmd (ptrAltModeContext, (uint32_t)CY_PDALTMODE_HPD_ENABLE_CMD);
    ptrAltModeContext->dpStatus->hpdState = 0;
    ptrAltModeContext->dpStatus->curHpdState = 0;
    ptrAltModeContext->dpStatus->queueReadIndex = 0;

#if DP_DFP_SUPP
    if (portRole != (uint8_t)CY_PD_PRT_TYPE_UFP)
    {
        /* Register a HPD callback and send the HPD_ENABLE command */
        Cy_PdAltMode_HW_SetCbk (ptrAltModeContext, Cy_PdAltMode_DP_SrcHpdRespCbk);
    }
#endif /* DP_DFP_SUPP */

#if DP_UFP_SUPP
    if (portRole == (uint8_t)CY_PD_PRT_TYPE_UFP)
    {
        /* Register a HPD callback and send the HPD_ENABLE command */
        Cy_PdAltMode_HW_SetCbk (ptrAltModeContext, Cy_PdAltMode_DP_SnkHpdRespCbk);
    }
#endif /* DP_UFP_SUPP */
}

#if (!ICL_ALT_MODE_HPI_DISABLED)
/****************************HPI command evaluation***************************/

static bool Cy_PdAltMode_DP_EvalAppCmd(void *context, cy_stc_pdaltmode_alt_mode_evt_t cmdData)
{
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)context;
    cy_stc_pdaltmode_dp_status_t *dpStatus = ptrAltModeContext->dpStatus;
#if !ICL_DP_DISABLE_APP_CHANGES
#if ((DP_DFP_SUPP != 0) || (DP_UFP_SUPP != 0))
    uint32_t data = cmdData.alt_mode_event_data.evtData;
#endif /* ((DP_DFP_SUPP != 0) || (DP_UFP_SUPP != 0)) */

#if DP_DFP_SUPP
    uint32_t config = CY_PDALTMODE_MNGR_NO_DATA;
#endif /* DP_DFP_SUPP */
#if DP_UFP_SUPP
    bool isAttNeeded = false;
#endif /* DP_UFP_SUPP */

#if DP_DFP_SUPP
    /* MUX configuration command */
    if (cmdData.alt_mode_event_data.evtType == CY_PDALTMODE_DP_MUX_CTRL_CMD)
    {
        dpStatus->dpCfgCtrl = (uint8_t)(data == (uint32_t)CY_PDALTMODE_DP_MUX_CTRL_CMD);
        return true;
    }
    /* DP config command received */
    else if ((cmdData.alt_mode_event_data.evtType == CY_PDALTMODE_DP_APP_CFG_CMD) &&
            (dpStatus->dpCfgCtrl != 0x0u))
    {
        if (data == CY_PDALTMODE_DP_APP_CFG_USB_IDX)
        {
            dpStatus->state = CY_PDALTMODE_DP_STATE_CONFIG;
#if DP_2_1_ENABLE
            /* Set VDO version */
            dpStatus->configVdo.dp_cfg_vdo.dpAmVersion = dpStatus->dp21Supp;
#endif /* DP_2_1_ENABLE */
            dpStatus->configVdo.val = CY_PDALTMODE_DP_USB_SS_CONFIG;
            Cy_PdAltMode_DP_AssignVdo(ptrAltModeContext, dpStatus->configVdo.val);
            Cy_PdAltMode_DP_SendCmd(ptrAltModeContext);
        }
        else if(data <= CY_PDALTMODE_DP_APP_CFG_CMD_MAX_NUMB)
        {
            /* Check if received DP config is possible */
            CY_PDALTMODE_MNGR_SET_FLAG(config, data);
            /* Check pin assignment compatibility */
            if ((Cy_PdAltMode_DP_IsPrtnrCcgConsistent(ptrAltModeContext, (uint8_t)config)) &&
                    (dpStatus->configVdo.dp_cfg_vdo.dfpAsgmnt != config))
            {
                dpStatus->state = CY_PDALTMODE_DP_STATE_CONFIG;
                /* Prepare Config cmd and send VDM */
                dpStatus->configVdo.val = 0;
                dpStatus->configVdo.dp_cfg_vdo.dfpAsgmnt = config;
                dpStatus->configVdo.dp_cfg_vdo.sign = CY_PDALTMODE_DP_1_3_SIGNALING;
                dpStatus->configVdo.dp_cfg_vdo.selConf = CY_PDALTMODE_DP_CONFIG_SELECT;
                Cy_PdAltMode_DP_AssignVdo(ptrAltModeContext, dpStatus->configVdo.val);
                Cy_PdAltMode_DP_SendCmd(ptrAltModeContext);
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        /* Do nothing */
    }
#endif /* DP_DFP_SUPP */

#if DP_UFP_SUPP
    /* MUX configuration command */
    if (cmdData.alt_mode_event_data.evtType == CY_PDALTMODE_DP_SINK_CTRL_CMD)
    {
        /* Check if received command is Exit/USB request */
        if ((data < (uint32_t)CY_PDALTMODE_DP_STAT_HPD) && (data > (uint32_t)CY_PDALTMODE_DP_STAT_MF))
        {
            /* Update DP Sink status */
            if (Cy_PdAltMode_DP_UfpUpdateStatusField(ptrAltModeContext, (cy_en_pdaltmode_dp_stat_bm_t)data, true))
            {
                /* Set send Attention flag */
                isAttNeeded = true;
            }
        }

        /* Check if received command is enabling of multi-function */
        if (data == (uint32_t)CY_PDALTMODE_DP_STAT_MF)
        {
            /* Check if MF is supported by CCG */
            if (
                    /* QAC suppression 3415: The second check is not required if the first check is incorrect
                     * and hence it is not a side effect. */
                    ((dpStatus->ccgDpPinsSupp & (CY_PDALTMODE_DP_DFP_D_CONFIG_D | CY_PDALTMODE_DP_DFP_D_CONFIG_F)) != 0u) &&

                    /* If status changed from previous */
                    Cy_PdAltMode_DP_UfpUpdateStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_MF, true) /* PRQA S 3415 */
               )
            {
                isAttNeeded = true;
            }
        }

        /* Check if received command is disabling of multi-function */
        if (
                /* QAC suppression 3415: The second check is not required if the first check is incorrect
                 * and hence it is not a side effect. */
                (data == ((uint32_t)CY_PDALTMODE_DP_STAT_MF - 1u)) &&
                Cy_PdAltMode_DP_UfpUpdateStatusField(ptrAltModeContext, CY_PDALTMODE_DP_STAT_MF, false) /* PRQA S 3415 */
           )
        {
            isAttNeeded = true;
        }

        /* If status changed then send attention */
        if (isAttNeeded)
        {
            /* Copy Status VDO to VDO buffer send Attention VDM */
            dpStatus->state = CY_PDALTMODE_DP_STATE_ATT;
            Cy_PdAltMode_DP_AssignVdo(ptrAltModeContext, dpStatus->statusVdo.val);
            Cy_PdAltMode_DP_SendCmd(ptrAltModeContext);
            return true;
        }
        else
        {
            Cy_PdAltMode_DP_Info(ptrAltModeContext)->modeState = CY_PDALTMODE_STATE_IDLE;
        }
    }
#if CY_PD_DP_VCONN_SWAP_FEATURE
    else if (cmdData.alt_mode_event_data.evtType == CY_PDALTMODE_DP_APP_VCONN_SWAP_CFG_CMD)
    {
        /* Init Vconn Swap request */
        dpStatus->vconnSwapReq = 0x01u;
        /* Send Attention VDM with USB request */
        dpStatus->state = CY_PDALTMODE_DP_STATE_ATT;
        Cy_PdAltMode_DP_AssignVdo(ptrAltModeContext, CY_PDALTMODE_DP_USB_SS_CONFIG);
        Cy_PdAltMode_DP_SendCmd(ptrAltModeContext);
        /* Start timer to make sure that DFP sent configuration command */
        (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start) (ptrAltModeContext->pdStackContext->ptrTimerContext, ptrAltModeContext->pdStackContext,
                CY_PDSTACK_GET_PD_TIMER_ID(ptrAltModeContext->pdStackContext, CY_PDSTACK_PD_VCONN_RECOVERY_TIMER),
                    CY_PDALTMODE_APP_UFP_RECOV_VCONN_SWAP_TIMER_PERIOD, Cy_PdAltMode_DP_ConfigAttFailedCbk);
        return true;
    }
    else
    {
        /* Do nothing */
    }
#endif /* CY_PD_DP_VCONN_SWAP_FEATURE */
#endif /* DP_UFP_SUPP */
#endif /* ICL_DP_DISABLE_APP_CHANGES */
    return false;
}
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */
#endif /* DP_DFP_SUPP || DP_UFP_SUPP */

/* [] END OF FILE */
