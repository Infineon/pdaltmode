/***************************************************************************//**
* \file cy_pdaltmode_usb4.c
* \version 1.0
*
* \brief
* USB4 mode handler implementation.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
#if CY_PD_USB4_SUPPORT_ENABLE

#include "cy_pdaltmode_defines.h"

#include "cy_pdstack_dpm.h"
#include "cy_pdutils.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_timer_id.h"
#include "cy_pdaltmode_timer_id.h"

#include "cy_pdaltmode_hw.h"
#include "cy_pdaltmode_vdm_task.h"
#include "cy_pdaltmode_mngr.h"
#include "cy_pdaltmode_usb4.h"

#if STORE_DETAILS_OF_HOST
#include "cy_pdaltmode_host_details.h"
#endif /* STORE_DETAILS_OF_HOST */

#if AMD_RETIMER_ENABLE
#include "cy_pdaltmode_retimer_wrapper.h"
#endif /* AMD_RETIMER_ENABLE */

static cy_en_pdstack_status_t Cy_PdAltMode_Usb4_SendEnter(cy_stc_pdaltmode_context_t *ptrAltModeContext);

ATTRIBUTES_ALT_MODE cy_pd_pd_do_t* Cy_PdAltMode_Usb4_GetEudo(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    return &ptrAltModeContext->vdmStat->eudoBuf.cmdDo[0];
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Usb4_EvalCblResp(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdaltmode_vdm_msg_info_t *msgP       = &ptrAltModeContext->vdmStat->vdmMsg;
    cy_en_pdaltmode_vdm_task_t ret              = CY_PDALTMODE_VDM_TASK_REG_ATCH_TGT_INFO;
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    /* Save TBT cable response */
    ptrAltModeContext->altModeAppStatus->tbtCblVdo = msgP->vdo[0];
    /* Set common active/passive cable parameters */
    ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_GetEudo)(ptrAltModeContext)->enterusb_vdo.cableSpeed = msgP->vdo[0].tbt_vdo.cblSpeed;

    /* Handle active/passive cable */
    if  (ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_PAS_CBL)
    {
        /*
         * Special handling for LRD cable - bit#24 in TBT disc mode will be set to 1 */
        if(msgP->vdo[0].tbt_vdo.cableActive != 0x0U)
        {
            ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_GetEudo)(ptrAltModeContext)->enterusb_vdo.cableType = (uint32_t)CY_PDSTACK_CBL_TYPE_ACTIVE_REDRIVER;
            ptrAltModeContext->vdmStat->usb4Flag = CY_PDALTMODE_USB4_TBT_CBL_ENTER_SOP_P;
            ret = CY_PDALTMODE_VDM_TASK_USB4_TBT;
        }
        else
        {
            /* Enter USB4 mode */
            ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_Enter)(ptrAltModeContext, CY_PD_SOP, false);
        }
    }
    else if(ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_ACT_CBL)
    {
        Cy_PdAltMode_Usb4_GetEudo(ptrAltModeContext)->enterusb_vdo.cableType  = (uint32_t)CY_PDSTACK_CBL_TYPE_ACTIVE_REDRIVER;

        /* Set active cable bit */
        if(msgP->vdo[0].tbt_vdo.b22RetimerCbl != 0x0U)
        {
            ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_GetEudo)(ptrAltModeContext)->enterusb_vdo.cableType = (uint32_t)CY_PDSTACK_CBL_TYPE_ACTIVE_RETIMER;
        }

        if (msgP->vdo[0].tbt_vdo.cblGen != 0x0U)
        {
            ptrAltModeContext->vdmStat->usb4Flag = CY_PDALTMODE_USB4_TBT_CBL_ENTER_SOP_P;
            ret = CY_PDALTMODE_VDM_TASK_USB4_TBT;
        }
        else
        {
            /* Resume VDM handling */
            ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_ResumeHandler)(ptrAltModeContext);
        }
    }
    else
    {
        /* Do nothing */
    }

    return ret;
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Usb4_FailCblResp(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    cy_en_pdaltmode_vdm_task_t ret = CY_PDALTMODE_VDM_TASK_REG_ATCH_TGT_INFO;

    if (
            (ptrAltModeContext->vdmStat->usb4Flag >= CY_PDALTMODE_USB4_TBT_CBL_DISC_RUN)    &&
            (ptrAltModeContext->vdmStat->usb4Flag <= CY_PDALTMODE_USB4_TBT_CBL_ENTER_SOP_DP)
        )
    {
        if  (ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_PAS_CBL)
        {
            /* Enter USB4 mode with passive cable Disc ID response parameters */
            ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_Enter)(ptrAltModeContext, CY_PD_SOP, false);
        }
         else if (ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_ACT_CBL)
        {
            /* Resume VDM handling */
            ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_ResumeHandler)(ptrAltModeContext);
        }
        else
        {
            /* Do nothing */
        }
    }

    return ret;
}

cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Usb4_TbtHandler(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_vdm_evt_t vdmEvt)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    cy_en_pdaltmode_vdm_task_t ret               = CY_PDALTMODE_VDM_TASK_REG_ATCH_TGT_INFO;
    cy_stc_pdaltmode_vdm_msg_info_t *msgP        = &ptrAltModeContext->vdmStat->vdmMsg;

    ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_RUN;

    if (vdmEvt == CY_PDALTMODE_VDM_EVT_RUN)
    {
        ret = CY_PDALTMODE_VDM_TASK_SEND_MSG;
        switch (ptrAltModeContext->vdmStat->usb4Flag)
        {
            case CY_PDALTMODE_USB4_TBT_CBL_DISC_RUN:
                /* Send Disc mode to TBT cable */
                ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SetDiscParam)(ptrAltModeContext, (uint8_t)CY_PD_SOP_PRIME, CY_PDSTACK_VDM_CMD_DSC_MODES);
                break;
            case CY_PDALTMODE_USB4_TBT_CBL_ENTER_SOP_P:
                /* Set MUX to safe state as CCG enters to TBT Alt Mode */
                (void)Cy_PdAltMode_HW_SetMux(ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SAFE, CY_PDALTMODE_MNGR_NO_DATA, CY_PDALTMODE_MNGR_NO_DATA);

                /* Send Disc mode to TBT cable */
                ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SetDiscParam)(ptrAltModeContext, (uint8_t)CY_PD_SOP_PRIME, CY_PDSTACK_VDM_CMD_ENTER_MODE);
                msgP->vdmHeader.std_vdm_hdr.objPos = 1u;
                break;
            case CY_PDALTMODE_USB4_TBT_CBL_ENTER_SOP_DP:
                /* Send Disc mode to TBT cable */
                ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SetDiscParam)(ptrAltModeContext, (uint8_t)CY_PD_SOP_DPRIME, CY_PDSTACK_VDM_CMD_ENTER_MODE);
                msgP->vdmHeader.std_vdm_hdr.objPos = 1u;
                break;
            case CY_PDALTMODE_USB4_SEND_USB4:
                if (Cy_PdAltMode_Usb4_SendEnter(ptrAltModeContext) != CY_PDSTACK_STAT_SUCCESS)
                {
                    /* In case of failure, start timer to retry the request. */
                        (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start) (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                                CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext,  CY_PDALTMODE_VDM_BUSY_TIMER),
                                     CY_PDALTMODE_APP_USB4_ENTRY_RETRY_PERIOD, ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_EnterUsb4RetryCbk));
                }
                ret = CY_PDALTMODE_VDM_TASK_WAIT;
                break;
            default:
                /* Do nothing */
                break;
        }
        msgP->vdmHeader.std_vdm_hdr.svid = CY_PDALTMODE_TBT_SVID;
    }
    else if (vdmEvt == CY_PDALTMODE_VDM_EVT_EVAL)
    {
        msgP->vdmHeader.std_vdm_hdr.svid = CY_PDALTMODE_TBT_SVID;
        switch (ptrAltModeContext->vdmStat->usb4Flag)
        {
            case CY_PDALTMODE_USB4_TBT_CBL_DISC_RUN:
                /* Evaluate TBT cable discovery mode response */
                ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_EvalCblResp)(ptrAltModeContext);
                break;
            case CY_PDALTMODE_USB4_TBT_CBL_ENTER_SOP_P:
                /* Check if SOP" is present */
                if (ptrPdStackContext->dpmStat.cblVdo.std_cbl_vdo.sopDp != 0x0U)
                {
                    /* Send Enter TBT cable SOP" command */
                    ptrAltModeContext->vdmStat->usb4Flag = CY_PDALTMODE_USB4_TBT_CBL_ENTER_SOP_DP;
                    ret = CY_PDALTMODE_VDM_TASK_USB4_TBT;
                }
                else
                {
                    /* Set TBT mode cable enter flag */
                    ptrAltModeContext->altModeAppStatus->tbtCblEntered = true;
                    /* Enter USB4 mode */
                    ret = Cy_PdAltMode_Usb4_Enter(ptrAltModeContext, CY_PD_SOP, false);
                }
                break;
            case CY_PDALTMODE_USB4_TBT_CBL_ENTER_SOP_DP:
                /* Set TBT mode cable enter flag */
                ptrAltModeContext->altModeAppStatus->tbtCblEntered = true;
                /* Enter USB4 mode */
                ret = Cy_PdAltMode_Usb4_Enter(ptrAltModeContext, CY_PD_SOP, false);
                break;
            case CY_PDALTMODE_USB4_SEND_USB4:
                /* Evaluate USB4 Accept response */
                ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_EvalAcceptResp)(ptrAltModeContext);
                break;
            default:
                /* Do nothing */
                break;
        }
    }
    else if (vdmEvt == CY_PDALTMODE_VDM_EVT_FAIL)
    {
        if (ptrAltModeContext->vdmStat->usb4Flag == CY_PDALTMODE_USB4_SEND_USB4)
        {
            /* Handle EnterUSB failed response */
            ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_EvalFailResp)(ptrAltModeContext);
        }
        else
        {
            /* Handle failed TBT cable response */
            ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_FailCblResp)(ptrAltModeContext);
        }
    }
    else
    {
        /* Do nothing */
    }

    return ret;
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_Usb4_DataRstRecCbk(cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_resp_status_t status, const cy_stc_pdstack_pd_packet_t* recVdm)
{
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    /* Stack already does error recovery if the Data Reset response from port partner is not valid. */
    if (status <= CY_PDSTACK_CMD_FAILED)
    {
        /* Send Data Reset message */
        if (PDSTACK_CALL_MAP(Cy_PdStack_Dpm_SendPdCommand)(ptrPdStackContext, CY_PDSTACK_DPM_CMD_SEND_DATA_RESET, NULL, false, ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_DataRstRecCbk)) != CY_PDSTACK_STAT_SUCCESS)
        {
            (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start) (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                    CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_VDM_BUSY_TIMER),
                        ptrAltModeContext->vdmStat->vdmBusyDelay, ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_DataRstRetryCbk));
        }
    }

    (void) recVdm;
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_Usb4_DataRstRetryCbk (cy_timer_id_t id, void *context)
{
    (void)id;

    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    /* Increment Data Reset counter */
    ptrAltModeContext->altModeAppStatus->usb4DataRstCnt++;

    /* Send Data Reset command */
    if (PDSTACK_CALL_MAP(Cy_PdStack_Dpm_SendPdCommand)(ptrPdStackContext, CY_PDSTACK_DPM_CMD_SEND_DATA_RESET, NULL, false, ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_DataRstRecCbk)) != CY_PDSTACK_STAT_SUCCESS)
    {
        (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start) (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_VDM_BUSY_TIMER),
                    ptrAltModeContext->vdmStat->vdmBusyDelay, ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_DataRstRetryCbk));
    }
}

uint32_t Cy_PdAltMode_Usb4_UpdateDataStatus(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_pd_pd_do_t eudo, uint32_t val)
{
    (void) val;
    uint32_t ret = 0x00U;

#if AMD_RETIMER_ENABLE
    if (
            (ptrAltModeContext->pdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP) &&
            (eudo.enterusb_vdo.usbMode == CY_PD_USB_MODE_USB4)
        )
    {
        /* Enable Retimer in case of UFP data role */
        Cy_PdAltMode_RetimerWrapper_Enable(ptrAltModeContext, false, true);
    }
#endif

    /* Save EUDO */
    ptrAltModeContext->altModeAppStatus->eudo = eudo;

    if (eudo.enterusb_vdo.usbMode == CY_PD_USB_MODE_USB4)
    {
        ret = (uint32_t)Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_USB4_CUSTOM, eudo.val, CY_PDALTMODE_MNGR_NO_DATA);        
    }
    else if (eudo.enterusb_vdo.usbMode == CY_PD_USB_MODE_USB3)
    {
        ret = (uint32_t)Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SS_ONLY, CY_PDALTMODE_MNGR_NO_DATA, CY_PDALTMODE_MNGR_NO_DATA);        
    }
    else if (eudo.enterusb_vdo.usbMode == CY_PD_USB_MODE_USB2)
    {
        ret = (uint32_t)Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SAFE, CY_PDALTMODE_MNGR_NO_DATA, CY_PDALTMODE_MNGR_NO_DATA);        
    }
    else
    {
        /* Do nothing */
    }

    return ret;
}

ATTRIBUTES_ALT_MODE static void Cy_PdAltMode_Usb4_CblRstRecCbk(cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_resp_status_t status, const cy_stc_pdstack_pd_packet_t* recVdm)
{
    (void)recVdm;

    if (status == CY_PDSTACK_CMD_SENT)
    {
        (void)ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_ResumeHandler)(ptrPdStackContext->ptrAltModeContext);
    }
}

ATTRIBUTES_ALT_MODE static void Cy_PdAltMode_Usb4_CblRstRetryCb (cy_timer_id_t id, void *context)
{
    (void)id;
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *) context;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    if (PDSTACK_CALL_MAP(Cy_PdStack_Dpm_SendPdCommand)(ptrPdStackContext, CY_PDSTACK_DPM_CMD_SEND_CABLE_RESET, NULL, false,
            Cy_PdAltMode_Usb4_CblRstRecCbk) != CY_PDSTACK_STAT_SUCCESS)
    {
        (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start) (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext,  CY_PDALTMODE_VDM_BUSY_TIMER),
                    ptrAltModeContext->vdmStat->appCblStartDelay, Cy_PdAltMode_Usb4_CblRstRetryCb);
    }
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Usb4_EvalAcceptResp(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    cy_en_pd_sop_t sopType = CY_PD_SOP;

    /* Remove USB4 pending flag to allow VDM manager handling */
    ptrAltModeContext->vdmStat->usb4Flag = CY_PDALTMODE_USB4_NONE;

    /* USB4 entered */
    if (ptrAltModeContext->vdmStat->eudoBuf.cmdSop == CY_PD_SOP)
    {
        /* Set USB4 related MUX */
        ptrAltModeContext->vdmStat->usb4UpdatePending = 0x01U;

        /* Check if there is need to continue with Discovery */
        if (
               (ptrAltModeContext->vdmStat->altModesNotSupp != 0x0U)
            /* If TBT mode only (not vPro) supported, then do not resume VDM Discovery. */
            || ((ptrAltModeContext->vdmDiscIdResp[CY_PD_PRODUCT_TYPE_VDO_1_IDX] & CY_PDALTMODE_UFP_ALT_MODE_SUPP_MASK) == CY_PDALTMODE_UFP_TBT_ALT_MODE_SUPP_MASK)
           )
        {
            return CY_PDALTMODE_VDM_TASK_WAIT;
        }
        else
        {
            /* Set SVID idx to zero before start Disc SVID */
            ptrAltModeContext->vdmStat->svidIdx  = CY_PDALTMODE_MNGR_NO_DATA;
            ptrAltModeContext->vdmStat->dSvidCnt = CY_PDALTMODE_MNGR_NO_DATA;
            /* Restart Discovery flow to discover Alternate Modes */
            ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_RUN;
            return CY_PDALTMODE_VDM_TASK_DISC_SVID;
        }
    }
    /* USB4 SOP or SOP" should be sent to finish USB4 entry */
    else if (ptrAltModeContext->vdmStat->eudoBuf.cmdSop == CY_PD_SOP_PRIME)
    {
        sopType = (ptrPdStackContext->dpmStat.cblVdo.act_cbl_vdo.sopDp != 0x0UL) ? CY_PD_SOP_DPRIME : CY_PD_SOP;
    }
    else
    {
        /* Do nothing */
    }

    return ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_Enter)(ptrAltModeContext, sopType, false);
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Usb4_EvalFailResp(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    cy_en_pdaltmode_vdm_task_t ret = CY_PDALTMODE_VDM_TASK_WAIT;

    if (ptrAltModeContext->vdmStat->eudoBuf.cmdSop == CY_PD_SOP)
    {
        ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_Enter)(ptrAltModeContext, ptrAltModeContext->vdmStat->eudoBuf.cmdSop, true);
    }
    /* Send cable reset command */
    else if (PDSTACK_CALL_MAP(Cy_PdStack_Dpm_SendPdCommand)(ptrPdStackContext, CY_PDSTACK_DPM_CMD_SEND_CABLE_RESET, NULL, false, Cy_PdAltMode_Usb4_CblRstRecCbk) != CY_PDSTACK_STAT_SUCCESS)
    {
        (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start) (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext,  CY_PDALTMODE_VDM_BUSY_TIMER),
                    ptrAltModeContext->vdmStat->appCblStartDelay, Cy_PdAltMode_Usb4_CblRstRetryCb);
    }
    else
    {
        /* Do nothing */
    }

    return ret;
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_Usb4_EnterUsb4RetryCbk (cy_timer_id_t id, void *context)
{
    (void)id;

    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *) context;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;
    /* Try to send Enter USB4 command again. */
    ptrAltModeContext->vdmStat->vdmTask = ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_Enter)(ptrAltModeContext, ptrAltModeContext->vdmStat->eudoBuf.cmdSop, true);
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_Usb4_EnterRecCbk(cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_resp_status_t status, const cy_stc_pdstack_pd_packet_t* recVdm)
{
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    if (status == CY_PDSTACK_RES_RCVD)
    {
        if (recVdm->hdr.hdr.msgType == (uint32_t)CY_PDSTACK_REQ_ACCEPT)
        {
            /* Evaluate EnterUSB Accept response */
            ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_EVAL;
            ptrAltModeContext->vdmStat->vdmTask = CY_PDALTMODE_VDM_TASK_USB4_TBT;
        }
        else if (recVdm->hdr.hdr.msgType == (uint32_t)CY_PDSTACK_REQ_WAIT)
        {
            /* Increment the retry count */
            ptrAltModeContext->vdmStat->usb4EnterRetryCount++;
            
            /* If Wait response is received, retry Enter USB command until the maximum retry limit. */
            if(ptrAltModeContext->vdmStat->usb4EnterRetryCount <= CY_PDALTMODE_ENTER_USB_RETRY_MAX_LIMIT)
            {
                /* Retry Enter USB command after tEnterUSBWait = 150 ms */
                (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start) (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                         CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext,  CY_PDALTMODE_VDM_BUSY_TIMER),
                             ptrAltModeContext->vdmStat->enterUsb4WaitDelay, ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_EnterUsb4RetryCbk));
            }
            else
            {
                /* Resume VDM handling */
                (void)ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_ResumeHandler)(ptrAltModeContext);
            }
        }
        else if ((recVdm->hdr.hdr.msgType == (uint32_t)CY_PDSTACK_REQ_REJECT) || (recVdm->hdr.hdr.msgType == (uint32_t)CY_PDSTACK_REQ_NOT_SUPPORTED))
        {
            /* Resume VDM handling */
            (void)ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_ResumeHandler)(ptrAltModeContext);
        }
        else
        {
            /* Do nothing */
        }
    }
    else if (status <= CY_PDSTACK_CMD_FAILED)
    {
        /* Evaluate failed response */
        ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_FAIL;
        ptrAltModeContext->vdmStat->vdmTask = CY_PDALTMODE_VDM_TASK_USB4_TBT;
    }
    else
    {
        /* Do nothing */
    }
}

static cy_en_pdstack_status_t Cy_PdAltMode_Usb4_SendEnter(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    bool retVal = false;
    cy_en_pdstack_status_t stat = CY_PDSTACK_STAT_FAILURE;
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    if (ptrAltModeContext->vdmStat->eudoBuf.cmdSop == CY_PD_SOP_PRIME)
    {
        /* Enable Vconn for cable if required */
        retVal = Cy_PdAltMode_VdmTask_InitiateVcsCblDiscovery (ptrAltModeContext);
    }
    /* If sending message to EMCA, make sure you are the VConn Source and have the supply turned ON. */
    if ((ptrAltModeContext->vdmStat->eudoBuf.cmdSop == CY_PD_SOP) || (retVal == false))
    {
        stat = PDSTACK_CALL_MAP(Cy_PdStack_Dpm_SendPdCommand)(ptrPdStackContext, CY_PDSTACK_DPM_CMD_SEND_ENTER_USB,
                &ptrAltModeContext->vdmStat->eudoBuf, false, ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_EnterRecCbk));
    }

    return stat;
}

cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_Usb4_Enter(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pd_sop_t sopType, bool retry)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    cy_stc_pdstack_dpm_pd_cmd_buf_t  *cmd_buf = &ptrAltModeContext->vdmStat->eudoBuf;
    cy_pd_pd_do_t eudo = { .val = 0u, };

    /* Set EUDO USB role based on configuration table */
    if (ptrAltModeContext->tbtCfg->usb4_supp == (uint8_t)CY_PDSTACK_USB_ROLE_DRD)
    {
        eudo.enterusb_vdo.usb4Drd = 0x01UL;
    }
    if (ptrAltModeContext->tbtCfg->usb3_supp == (uint8_t)CY_PDSTACK_USB_ROLE_DRD)
    {
        eudo.enterusb_vdo.usb3Drd = 0x01UL;
    }

    /* Set USB Host supported parameters based on configuration table */
    eudo.val |= ((ptrAltModeContext->altModeAppStatus->customHpiHostCapControl & 0x0FUL) << CY_PDALTMODE_VDMTASK_USB4_EUDO_HOST_PARAM_SHIFT);

#if STORE_DETAILS_OF_HOST
    if(ptrPdStackContext->port == TYPEC_PORT_1_IDX)
    {
        cy_pd_pd_do_t temp_enter_usb_param;

        cy_stc_pdaltmode_context_t *hostContext =
                (cy_stc_pdaltmode_context_t *) ptrAltModeContext->hostDetails->usAltModeContext;

        temp_enter_usb_param.val = hostContext->hostDetails->hostEudo.val;

        if(Cy_PdAltMode_HostDetails_IsUsb4Connected(ptrAltModeContext) == true)
        {
            eudo.enterusb_vdo.hostDpSupp = temp_enter_usb_param.enterusb_vdo.hostDpSupp;
            eudo.enterusb_vdo.hostPcieSupp = temp_enter_usb_param.enterusb_vdo.hostPcieSupp;
            eudo.enterusb_vdo.hostTbtSupp = temp_enter_usb_param.enterusb_vdo.hostTbtSupp;
            eudo.enterusb_vdo.hostPresent = temp_enter_usb_param.enterusb_vdo.hostPresent;
            eudo.enterusb_vdo.usbMode = temp_enter_usb_param.enterusb_vdo.usbMode;
        }
        else
        {
            eudo.enterusb_vdo.hostDpSupp = 1;
            eudo.enterusb_vdo.hostPcieSupp = 1;
            eudo.enterusb_vdo.hostTbtSupp = 1;
            eudo.enterusb_vdo.hostPresent = 0;
            eudo.enterusb_vdo.usbMode = CY_PDALTMODE_USB_MODE_USB4;
        }

        /*eudo.enterusb_vdo.hostDpSupp = 1;
        eudo.enterusb_vdo.hostPcieSupp = 1;
        eudo.enterusb_vdo.hostTbtSupp = 1;
        eudo.enterusb_vdo.hostPresent = 1;*/
    }
#endif /* STORE_DETAILS_OF_HOST */

    ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_GetEudo)(ptrAltModeContext)->val |= eudo.val;

    /* Fill dpm buffer */
    cmd_buf->cmdSop      = sopType;
    cmd_buf->noOfCmdDo   = CY_PDALTMODE_MNGR_VDO_START_IDX;
    cmd_buf->timeout     = ptrPdStackContext->senderRspTimeout;

    if(!retry)
    {
        /* Set MUX to safe state */
        (void)Cy_PdAltMode_HW_SetMux(ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SAFE, CY_PDALTMODE_MNGR_NO_DATA, CY_PDALTMODE_MNGR_NO_DATA);
    }

    /* Set USB4 send flag state and set VDM manager event to run state */
    ptrAltModeContext->vdmStat->usb4Flag = CY_PDALTMODE_USB4_SEND_USB4;
    ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_RUN;

    return CY_PDALTMODE_VDM_TASK_USB4_TBT;
}
#endif /* CY_PD_USB4_SUPPORT_ENABLE */
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */
