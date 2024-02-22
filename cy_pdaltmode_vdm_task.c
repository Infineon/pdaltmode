/***************************************************************************//**
* \file cy_pdaltmode_vdm_task.c
* \version 1.0
*
* \brief
* VDM Task Manager implementation.
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
#include "cy_pdutils.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_timer_id.h"
#include "cy_pdaltmode_timer_id.h"

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
#include "cy_pdaltmode_hw.h"
#include "cy_pdaltmode_vdm_task.h"
#include "cy_pdaltmode_mngr.h"
#include "cy_pdaltmode_usb4.h"

/* Callback function for sent VDM */
static void Cy_PdAltMode_VdmTask_RecCbk(cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_resp_status_t status, const cy_stc_pdstack_pd_packet_t* recVdm);

/* Function to send VDM */
static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_SendVdm(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/* VDM manager status structure */
static cy_stc_pdaltmode_vdm_task_status_t vdmMngrStatus[NO_OF_TYPEC_PORTS];

cy_stc_pdaltmode_vdm_task_status_t* Cy_PdAltMode_VdmTask_GetMngrStatus(uint8_t port)
{
    return &vdmMngrStatus[port];
}

void Cy_PdAltMode_VdmTask_Enable(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    /* If the VDM task is already running, do nothing. */
    if (ptrAltModeContext->altModeAppStatus->vdmTaskEn == (uint8_t)false)
    {
        ptrAltModeContext->vdmStat->vdmTask = CY_PDALTMODE_VDM_TASK_INIT;
        ptrAltModeContext->altModeAppStatus->vdmTaskEn = (uint8_t)true;
        ptrAltModeContext->altModeAppStatus->vdmPrcsFailed = false;
        ptrAltModeContext->vdmStat->appCblStartDelay = CY_PDALTMODE_APP_CABLE_VDM_START_DELAY;
        ptrAltModeContext->vdmStat->vdmBusyDelay = CY_PDALTMODE_APP_VDM_BUSY_TIMER_PERIOD;
        ptrAltModeContext->vdmStat->enterUsb4WaitDelay = CY_PDALTMODE_APP_ENTER_USB4_WAIT_PERIOD;
        ptrAltModeContext->vdmStat->vdmFailRetryDelay = CY_PDALTMODE_APP_VDM_FAIL_RETRY_PERIOD;
        ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_ResetDiscInfo)(ptrAltModeContext);
        /* Make sure all Alternate Mode manager state has been cleared at start. */
        ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_ResetInfo)(ptrAltModeContext);

#if HPI_AM_SUPP
        /* Check if there is any custom Alt Mode available and save it. */
        ptrAltModeContext->altModeAppStatus->customHpiSvid = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetCustomSvid)(ptrAltModeContext);
#endif /* HPI_AM_SUPP */
#if DFP_ALT_MODE_SUPP
#if (DEFER_VCONN_SRC_ROLE_SWAP)
        if (ptrPdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP)
        {
            /* Currently in DFP, do not relinquish VConn Source role until Alt Mode processing is complete. */
            ptrAltModeContext->appStatusContext->keep_vconn_src = true;
        }
        else
        {
            /* If in UFP, can allow the port partner to become VConn source. */
            ptrAltModeContext->appStatusContext->keep_vconn_src = false;
        }
#endif /* (DEFER_VCONN_SRC_ROLE_SWAP) */
#endif /* DFP_ALT_MODE_SUPP */
    }
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_Init(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    cy_en_pdaltmode_vdm_task_t ret = CY_PDALTMODE_VDM_TASK_EXIT;

    /* Reset VDM manager info */
    ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_ResetMngr)(ptrAltModeContext);
    ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_RUN;

    /* Set Alt Mode trigger based on config */
    ptrAltModeContext->altModeAppStatus->altModeTrigMask = ptrAltModeContext->dpCfg->dp_mode_trigger;
    ptrAltModeContext->dfpAltModeMask = ptrAltModeContext->altModeCfg->dfp_alt_mode_mask;
    ptrAltModeContext->ufpAltModeMask = ptrAltModeContext->altModeCfg->ufp_alt_mode_mask;

    ptrAltModeContext->vdmStat->vdmEmcaRstState = CY_PDALTMODE_CABLE_DP_RESET_IDLE;
    ptrAltModeContext->vdmStat->vdmVcsRqtState = CY_PDALTMODE_VCONN_RQT_INACTIVE;
    ptrAltModeContext->hwDetails->muxCurState = (uint8_t)CY_PDALTMODE_MUX_CONFIG_ISOLATE;

    if (ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModeNumb)(ptrAltModeContext) != (uint8_t)false)
    {
        /* Check if current data role is DFP */
        if(ptrPdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP)
        {
            ret = CY_PDALTMODE_VDM_TASK_DISC_ID;
        }
        else
        {
            ret = CY_PDALTMODE_VDM_TASK_REG_ATCH_TGT_INFO;
        }
    }

    return ret;

}

ATTRIBUTES_ALT_MODE bool Cy_PdAltMode_VdmTask_IsIdle(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    /*
       If VDM manager is enabled, check:
       1. Whether BUSY timer is running
       2. Whether the Alt Mode tasks are idle
     */
    return !((bool)ptrAltModeContext->altModeAppStatus->vdmTaskEn &&
            ((TIMER_CALL_MAP(Cy_PdUtils_SwTimer_IsRunning) (ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_VDM_BUSY_TIMER))) ||
             (ptrAltModeContext->vdmStat->vdmTask != CY_PDALTMODE_VDM_TASK_ALT_MODE) ||
             (ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_IsAltModeMngrIdle)(ptrAltModeContext) == false))
            );
}

void Cy_PdAltMode_VdmTask_Manager(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    /* VDM task enabled check is performed by the caller */
    if (
            (TIMER_CALL_MAP(Cy_PdUtils_SwTimer_IsRunning)(ptrPdStackContext->ptrTimerContext,
                    CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_VDM_BUSY_TIMER)) == true)
#if MUX_DELAY_EN
              || (ptrAltModeContext->altModeAppStatus->isMuxBusy != false)
#endif /* MUX_DELAY_EN */
       )
    {
        return;
    }

#if MUX_UPDATE_PAUSE_FSM
    /* Check if MUX is not busy */
    if (
          (ptrAltModeContext->altModeAppStatus->muxStat == CY_PDALTMODE_APP_MUX_STATE_BUSY)
#if ((!RIDGE_I2C_HPD_ENABLE) && (DP_DFP_SUPP))
       || (ptrAltModeContext->hwDetails->appMuxUpdateReq != false)
#endif /* (!RIDGE_I2C_HPD_ENABLE) && (DP_DFP_SUPP) */
       )
    {
        return;
    }
#endif /* MUX_UPDATE_PAUSE_FSM */

    /* Check if cable reset is required */
    if (ptrAltModeContext->altModeAppStatus->trigCblRst != false)
    {
        if(PDSTACK_CALL_MAP(Cy_PdStack_Dpm_SendPdCommand)(ptrPdStackContext, CY_PDSTACK_DPM_CMD_SEND_CABLE_RESET, NULL, false, NULL) == CY_PDSTACK_STAT_SUCCESS)
        {
            (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_StartWocb)(ptrPdStackContext->ptrTimerContext,
                    CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_VDM_BUSY_TIMER), ptrAltModeContext->vdmStat->appCblStartDelay);
            ptrAltModeContext->altModeAppStatus->trigCblRst = false;
        }
        return;
    }

#if CY_PD_USB4_SUPPORT_ENABLE
    if (ptrAltModeContext->vdmStat->usb4UpdatePending != 0x00U)
    {
        /* Set USB4 related MUX */
        (void)Cy_PdAltMode_Usb4_UpdateDataStatus (ptrAltModeContext, ptrAltModeContext->vdmStat->eudoBuf.cmdDo[0], CY_PDALTMODE_MNGR_NO_DATA);
        ptrAltModeContext->vdmStat->usb4UpdatePending = 0x00U;
    }
#endif /* CY_PD_USB4_SUPPORT_ENABLE */

    /* Get current VDM task */
    switch (ptrAltModeContext->vdmStat->vdmTask)
    {
        case CY_PDALTMODE_VDM_TASK_WAIT:
            break;

        case CY_PDALTMODE_VDM_TASK_INIT:
            ptrAltModeContext->vdmStat->vdmTask = ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_Init)(ptrAltModeContext);
            break;

#if ((DFP_ALT_MODE_SUPP) || (UFP_MODE_DISC_ENABLE))
        case CY_PDALTMODE_VDM_TASK_DISC_ID:
            ptrAltModeContext->vdmStat->vdmTask =  ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_MngDiscId)(ptrAltModeContext, ptrAltModeContext->vdmStat->vdmEvt);
            break;

        case CY_PDALTMODE_VDM_TASK_DISC_SVID:
            ptrAltModeContext->vdmStat->vdmTask =  ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_MngDiscSvid)(ptrAltModeContext, ptrAltModeContext->vdmStat->vdmEvt);
            break;
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_MODE_DISC_ENABLE)) */

        case CY_PDALTMODE_VDM_TASK_SEND_MSG:
            (void)ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_ComposeVdm)(ptrAltModeContext);
            ptrAltModeContext->vdmStat->vdmTask = Cy_PdAltMode_VdmTask_SendVdm(ptrAltModeContext);
            break;

#if CY_PD_USB4_SUPPORT_ENABLE
        case CY_PDALTMODE_VDM_TASK_USB4_TBT:
            ptrAltModeContext->vdmStat->vdmTask = Cy_PdAltMode_Usb4_TbtHandler(ptrAltModeContext, ptrAltModeContext->vdmStat->vdmEvt);
            break;
#endif /* CY_PD_USB4_SUPPORT_ENABLE */

        case CY_PDALTMODE_VDM_TASK_REG_ATCH_TGT_INFO:
            ptrAltModeContext->vdmStat->vdmTask = ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_RegAltModeMngr)(ptrAltModeContext,
                                  &ptrAltModeContext->vdmStat->atchTgt, &ptrAltModeContext->vdmStat->vdmMsg);
            ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_RUN;
            break;

        case CY_PDALTMODE_VDM_TASK_ALT_MODE:
            ptrAltModeContext->vdmStat->vdmTask =  Cy_PdAltMode_Mngr_AltModeProcess(ptrAltModeContext, ptrAltModeContext->vdmStat->vdmEvt);
            if ((ptrAltModeContext->vdmStat->vdmTask == CY_PDALTMODE_VDM_TASK_SEND_MSG) || (ptrAltModeContext->vdmStat->vdmTask == CY_PDALTMODE_VDM_TASK_ALT_MODE))
            {
                ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_RUN;
            }
            break;

        case CY_PDALTMODE_VDM_TASK_EXIT:
            Cy_PdAltMode_VdmTask_MngrDeInit(ptrAltModeContext);
            ptrAltModeContext->altModeAppStatus->vdmPrcsFailed = true;
            ptrAltModeContext->vdmStat->usb4UpdatePending = 0x00U;
            break;

        default:
            /* No statement */
            break;
    }
}

ATTRIBUTES_ALT_MODE bool Cy_PdAltMode_VdmTask_RetryCheck(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_fail_status_t failStat)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    (ptrAltModeContext->vdmStat->recRetryCnt)++;

    /* If we don't receive response more than recRetryCnt notify it is fail */
    if (ptrAltModeContext->vdmStat->recRetryCnt > CY_PDALTMODE_MNGR_MAX_RETRY_CNT)
    {
        ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SetVdmFailed)(ptrAltModeContext, failStat);
        return false;
    }

    /* Try to send message again */
    ptrAltModeContext->vdmStat->vdmTask = CY_PDALTMODE_VDM_TASK_SEND_MSG;

    if (failStat == CY_PDALTMODE_VDM_BUSY)
    {
        /* Start a BUSY timer to delay the retry attempt if port partner is busy. */
        (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_StartWocb) (ptrPdStackContext->ptrTimerContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_VDM_BUSY_TIMER), ptrAltModeContext->vdmStat->vdmFailRetryDelay);
    }
    else
    {
        /* Fix for Ellisys VDMD tests: delay the retry by 10 ms. */
        (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_StartWocb) (ptrPdStackContext->ptrTimerContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_VDM_BUSY_TIMER), 10u);
    }

    return true;
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_VdmTask_SetVdmFailed(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_fail_status_t failStat)
{
    /* Reset retry counters */
    ptrAltModeContext->vdmStat->recRetryCnt = 0;
    ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_FAIL;

    /* Set failed and save failed command */
    ptrAltModeContext->vdmStat->vdmMsg.vdo->std_vdm_hdr.cmd = ptrAltModeContext->altModeMngrStatus->vdmBuf.cmdDo[0].std_vdm_hdr.cmd;

    /* Save failure code in the object position field */
    ptrAltModeContext->vdmStat->vdmMsg.vdmHeader.std_vdm_hdr.objPos = (uint32_t)failStat;
}

/* Received VDM callback */
static void Cy_PdAltMode_VdmTask_RecCbk(cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_resp_status_t status, const cy_stc_pdstack_pd_packet_t* recVdm)
{
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    uint32_t response;
    bool     run_task_flag = false;

    /* If response ACK */
    if (status == CY_PDSTACK_RES_RCVD)
    {
        /* Check whether received message is a VDM and treat as NAK otherwise. */
        if ((recVdm->msg != (uint8_t)CY_PDSTACK_DATA_MSG_VDM) || (recVdm->len == 0U))
        {
            /* Notify manager with failed event */
            ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SetVdmFailed)(ptrAltModeContext, CY_PDALTMODE_VDM_NACK);
            run_task_flag = true;
        }
        else
        {
            /* Handle standard VDM */
            if (recVdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.vdmType == (uint32_t)CY_PDSTACK_VDM_TYPE_STRUCTURED)
            {
                response = recVdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType;

#if ((CY_PD_REV3_ENABLE))
                /* Update VDM version based on the value received from UFP */
                if (recVdm->sop == CY_PD_SOP)
                {
                    ptrAltModeContext->appStatusContext->vdmVersion =
                        (uint8_t)CY_PDUTILS_GET_MIN ((ptrAltModeContext->appStatusContext->vdmVersion), (recVdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stVer));
#if MINOR_SVDM_VER_SUPPORT
                    ptrAltModeContext->appStatusContext->vdmMinorVersion =
                            (uint8_t)CY_PDUTILS_GET_MIN (ptrAltModeContext->appStatusContext->vdmMinorVersion, recVdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stMinVer);
#endif /* MINOR_SVDM_VER_SUPPORT */
                }
                else if (recVdm->sop == CY_PD_SOP_PRIME)
                {
                    /* Update VDM version based on the value received from the cable */
                    ptrPdStackContext->dpmStat.cblVdmVersion =
                        (cy_en_pdstack_std_vdm_ver_t)CY_PDUTILS_GET_MIN (((uint32_t)ptrPdStackContext->dpmStat.cblVdmVersion), (recVdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stVer));
#if MINOR_SVDM_VER_SUPPORT
                    ptrPdStackContext->cblVdmMinVersion =
                        (cy_en_pdstack_std_minor_vdm_ver_t)CY_PDUTILS_GET_MIN ((uint32_t)ptrPdStackContext->cblVdmMinVersion, recVdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stMinVer);
#endif /* MINOR_SVDM_VER_SUPPORT */
                }
                else
                {/* No statement */}
#endif /* ((CY_PD_REV3_ENABLE)) */

                /* Actual response received */
                switch (response)
                {
                    case (uint32_t)CY_PDSTACK_CMD_TYPE_RESP_ACK:
                        {
#if CY_PD_USB4_SUPPORT_ENABLE
                            if (
                                   (ptrAltModeContext->altModeAppStatus->usb4Active == false)   &&
                                   (ptrAltModeContext->vdmStat->usb4Flag > CY_PDALTMODE_USB4_TBT_CBL_FIND) &&
                                   ((recVdm->sop == CY_PD_SOP_PRIME) || (recVdm->sop == CY_PD_SOP_DPRIME))
                                )
                            {
                                (void)ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_ParseVdm)(ptrAltModeContext, recVdm);
                                ptrAltModeContext->vdmStat->vdmTask = CY_PDALTMODE_VDM_TASK_USB4_TBT;
                                ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_EVAL;
                                return;
                            }
#endif /* CY_PD_USB4_SUPPORT_ENABLE */
                            /* Check if received response is expected */
                            if ((recVdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd) ==
                                     ptrAltModeContext->altModeMngrStatus->vdmBuf.cmdDo[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd)
                            {
                                (void)ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_ParseVdm)(ptrAltModeContext, recVdm);
                                ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_EVAL;

                                /* Reset timer counter when ACK */
                                ptrAltModeContext->vdmStat->recRetryCnt = 0;

                                /* Continue the state machine operation */
                                run_task_flag = true;
                            }
                            else if (ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_RetryCheck)(ptrAltModeContext, CY_PDALTMODE_VDM_BUSY) == false)
                                /* Check for retries. If failure persists after all retries, go to exit. */
                            {
                                run_task_flag = true;
                            }
                            else
                            {
                                /* No statement */
                            }
                        }
                        break;

                    case (uint32_t)CY_PDSTACK_CMD_TYPE_INITIATOR:
                    case (uint32_t)CY_PDSTACK_CMD_TYPE_RESP_BUSY:
                        /* Target is BUSY. */
                        {
                            /* Check for retries. If failure persists after all retries, go to exit. */
                            if (ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_RetryCheck)(ptrAltModeContext, CY_PDALTMODE_VDM_BUSY) == false)
                            {
                                run_task_flag = true;
                            }
                        }
                        break;

                    default:
                        /* Target NAK-ed the message */
                        {
                            /* Notify manager with failed event */
                            ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SetVdmFailed)(ptrAltModeContext, CY_PDALTMODE_VDM_NACK);
                            run_task_flag = true;
                        }
                        break;
                }
            }
            /* Handle UVDM */
            else
            {
                (void)ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_ParseVdm)(ptrAltModeContext, recVdm);
                ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_EVAL;
                /* Reset timer counter when ACK */
                ptrAltModeContext->vdmStat->recRetryCnt = 0;
                run_task_flag = true;
            }
        }
    }
    /* Attention related handler */
    else if (ptrAltModeContext->vdmStat->vdmMsg.vdmHeader.std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_ATTENTION)
    {
        /* This statement needs to notify Alt Mode that Attention VDM was successfully sent. */
        if ((status == CY_PDSTACK_CMD_SENT) && (ptrAltModeContext->vdmStat->vdmTask == CY_PDALTMODE_VDM_TASK_WAIT))
        {
            /* Start a timer; Command will be retried when timer expires. */
            TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_VDM_BUSY_TIMER));
            ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_EVAL;

            /* Reset retry counter */
            ptrAltModeContext->vdmStat->recRetryCnt = 0;
            /* Continue the state machine operation */
            run_task_flag = true;
        }
        else if (status == CY_PDSTACK_SEQ_ABORTED)
        {
            /* Try to send message again */
            run_task_flag = true;
        }
        else
        {
            /* No statement */
        }
    }
    else
    {
        /* Good CRC not received or no response (maybe corrupted packet). */
        if ((status == CY_PDSTACK_CMD_FAILED) || (status == CY_PDSTACK_RES_TIMEOUT))
        {
            /* Check for retries. If failure persists after all retries, go to exit. */
            if (ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_RetryCheck)(ptrAltModeContext, (cy_en_pdaltmode_fail_status_t)status) == false)
            {
                run_task_flag = true;
            }
        }
        else
        {
            if (status == CY_PDSTACK_SEQ_ABORTED)
            {
                /* Try to send message again */
                ptrAltModeContext->vdmStat->vdmTask = CY_PDALTMODE_VDM_TASK_SEND_MSG;
            }
        }
    }

    /* Check if we need run any task */
    if (run_task_flag == true)
    {
#if ((DFP_ALT_MODE_SUPP) || (UFP_MODE_DISC_ENABLE))
#if CY_PD_USB4_SUPPORT_ENABLE
        if (
                (ptrAltModeContext->altModeAppStatus->usb4Active == false) &&
                (ptrAltModeContext->vdmStat->usb4Flag > CY_PDALTMODE_USB4_TBT_CBL_FIND) &&
                ((recVdm->sop == CY_PD_SOP_PRIME) || (recVdm->sop == CY_PD_SOP_DPRIME))
           )
        {
            /* Set USB4 Task */
            ptrAltModeContext->vdmStat->vdmTask = CY_PDALTMODE_VDM_TASK_USB4_TBT;
            ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_FAIL;
            return;
        }
#endif /* CY_PD_USB4_SUPPORT_ENABLE */

        switch (ptrAltModeContext->vdmStat->vdmMsg.vdmHeader.std_vdm_hdr.cmd)
        {
            case (uint32_t)CY_PDSTACK_VDM_CMD_DSC_IDENTITY:
                ptrAltModeContext->vdmStat->vdmTask = CY_PDALTMODE_VDM_TASK_DISC_ID;
                break;
            case (uint32_t)CY_PDSTACK_VDM_CMD_DSC_SVIDS:
                ptrAltModeContext->vdmStat->vdmTask = CY_PDALTMODE_VDM_TASK_DISC_SVID;
                break;
            default:
                ptrAltModeContext->vdmStat->vdmTask = CY_PDALTMODE_VDM_TASK_ALT_MODE;
                break;
        }
#else /* ((DFP_ALT_MODE_SUPP) || (UFP_MODE_DISC_ENABLE)) */
        ptrAltModeContext->vdmStat->vdmTask = CY_PDALTMODE_VDM_TASK_ALT_MODE;
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_MODE_DISC_ENABLE)) */
    }
}

#if CY_PD_USB4_SUPPORT_ENABLE
ATTRIBUTES_ALT_MODE bool Cy_PdAltMode_VdmTask_IsUSB4Supp(cy_pd_pd_do_t* vdo_do_p, uint8_t len)
{
    bool    ufp_usb4_supp = false;
    bool    dfp_usb4_supp = false;
    uint8_t dfp_vdo_idx   = CY_PD_PRODUCT_TYPE_VDO_3_IDX;
    uint8_t vdo_shift     = CY_PD_ID_HEADER_IDX;

    /* Check if UFP supports USB4 */
    if (
            (len >= CY_PD_PRODUCT_TYPE_VDO_1_IDX)                                                                  &&
           ((vdo_do_p[CY_PD_ID_HEADER_IDX - vdo_shift].std_id_hdr.prodType == (uint32_t)CY_PDSTACK_PROD_TYPE_PERI) ||
            (vdo_do_p[CY_PD_ID_HEADER_IDX - vdo_shift].std_id_hdr.prodType == (uint32_t)CY_PDSTACK_PROD_TYPE_HUB)) &&
            (vdo_do_p[CY_PD_PRODUCT_TYPE_VDO_1_IDX - vdo_shift].val != CY_PDALTMODE_MNGR_NO_DATA)                       &&
           ((vdo_do_p[CY_PD_PRODUCT_TYPE_VDO_1_IDX - vdo_shift].ufp_vdo_1.devCap & (uint32_t)CY_PDSTACK_DEV_CAP_USB_4_0) != 0x0UL)
       )
    {
        ufp_usb4_supp = true;
    }

    /* Get DFP VDO index based on product type */
    if(
            (vdo_do_p[CY_PD_ID_HEADER_IDX - vdo_shift].std_id_hdr.prodType == (uint32_t)CY_PDSTACK_PROD_TYPE_PSD) ||
            (vdo_do_p[CY_PD_ID_HEADER_IDX - vdo_shift].std_id_hdr.prodType == (uint32_t)CY_PDSTACK_PROD_TYPE_UNDEF)
       )
    {
        dfp_vdo_idx = CY_PD_PRODUCT_TYPE_VDO_1_IDX;
    }

    /* Check if DFP supports USB4 */
    if (
           (len >= dfp_vdo_idx)                                                                                       &&
           (vdo_do_p[CY_PD_ID_HEADER_IDX - vdo_shift].std_id_hdr.prodTypeDfp != (uint32_t)CY_PDSTACK_PROD_TYPE_UNDEF) &&
           (vdo_do_p[dfp_vdo_idx - vdo_shift].val != CY_PDALTMODE_MNGR_NO_DATA)                                            &&
          ((vdo_do_p[dfp_vdo_idx - vdo_shift].dfp_vdo.hostCap & (uint32_t)CY_PDSTACK_HOST_CAP_USB_4_0) != 0x0UL)
       )
    {
        dfp_usb4_supp = true;
    }

    return (ufp_usb4_supp || dfp_usb4_supp);
}

ATTRIBUTES_ALT_MODE cy_en_pdstack_usb_sig_supp_t Cy_PdAltMode_VdmTask_GetEudoCblSpeed(cy_stc_pdstack_context_t *ptrPdStackContext)
{
    cy_en_pdstack_usb_sig_supp_t cblSpeed = CY_PDSTACK_USB_2_0;

    if (ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_ACT_CBL)
    {
        cblSpeed = (cy_en_pdstack_usb_sig_supp_t)((ptrPdStackContext->dpmStat.cblVdo.pas_cbl_vdo.usbSsSup >= (uint32_t)CY_PDSTACK_USB_GEN_4) ?
            (uint32_t)CY_PDSTACK_USB_GEN_4 : ptrPdStackContext->dpmStat.cblVdo.std_cbl_vdo.usbSsSup);
    }
    else
    {
        /* Upgrade passive cable speed according to Type-C specification Gen1->Gen2 and Gen3->Gen4 */
        if (ptrPdStackContext->dpmStat.cblVdo.std_cbl_vdo.usbSsSup == (uint32_t)CY_PDSTACK_USB_GEN_1)
        {
            cblSpeed = (cy_en_pdstack_usb_sig_supp_t)CY_PDSTACK_USB_GEN_2;
        }
        else
        {
            cblSpeed = (cy_en_pdstack_usb_sig_supp_t)((ptrPdStackContext->dpmStat.cblVdo.std_cbl_vdo.usbSsSup >= (uint32_t)CY_PDSTACK_USB_GEN_3) ?
            (uint32_t)CY_PDSTACK_USB_GEN_4 : ptrPdStackContext->dpmStat.cblVdo.std_cbl_vdo.usbSsSup);
        }
    }

    return cblSpeed;
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_SetEudoParams(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdaltmode_vdm_msg_info_t *msgP  = &ptrAltModeContext->vdmStat->vdmMsg;
    cy_pd_pd_do_t* eudo_p = &ptrAltModeContext->vdmStat->eudoBuf.cmdDo[0];
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    cy_en_pdaltmode_vdm_task_t ret = CY_PDALTMODE_VDM_TASK_EXIT;

    /* Check if cable is USB 4.0 capable */
    if (
            (ptrAltModeContext->vdmStat->usb4Flag != CY_PDALTMODE_USB4_FAILED)  &&
            (ptrPdStackContext->dpmConfig.emcaPresent != false)
       )
    {
        if (msgP->vdo[CY_PD_PRODUCT_TYPE_VDO_1_IDX - CY_PDALTMODE_MNGR_VDO_START_IDX].ufp_vdo_1.altModes == 0x0U)
        {
            /*
             * Discovery process could not be run if Alternate Modes' field in
             * UFP VDO 1 response is set to zero.
             */
            ptrAltModeContext->vdmStat->altModesNotSupp = 0x01;
        }

        /* Fill EUDO */
        eudo_p->enterusb_vdo.usbMode    = CY_PD_USB_MODE_USB4;
        eudo_p->enterusb_vdo.cableType  = 0x0UL;
        eudo_p->enterusb_vdo.cableSpeed = ptrPdStackContext->dpmStat.cblVdo.std_cbl_vdo.usbSsSup;

        /* Set active cable type */
        if(ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_ACT_CBL)
        {
            eudo_p->enterusb_vdo.cableType = (uint32_t)CY_PDSTACK_CBL_TYPE_ACTIVE_REDRIVER;

            /* If VDO version is 1.3 and higher */
            if ((ptrPdStackContext->dpmStat.cblVdo2.val != CY_PDALTMODE_MNGR_NO_DATA) &&
                    (ptrPdStackContext->dpmStat.cblVdo.act_cbl_vdo1.vdoVersion >= CY_PD_CBL_VDO_VERS_1_3))
            {
                if(ptrPdStackContext->dpmStat.cblVdo2.act_cbl_vdo2.optIsolated != 0x0UL)
                {
                    eudo_p->enterusb_vdo.cableType = (uint32_t)CY_PDSTACK_CBL_TYPE_OPTICAL;
                }
                else if ((ptrPdStackContext->dpmStat.cblVdo2.act_cbl_vdo2.phyConn != 0x0UL)  ||
                            (ptrPdStackContext->dpmStat.cblVdo2.act_cbl_vdo2.activeEl == 0x0UL ))
                {
                    eudo_p->enterusb_vdo.cableType = (uint32_t)CY_PDSTACK_CBL_TYPE_ACTIVE_REDRIVER;
                }
                else
                {
                    eudo_p->enterusb_vdo.cableType = (uint32_t)CY_PDSTACK_CBL_TYPE_ACTIVE_RETIMER;
                }
            }
        }

        /* Set EUDO cable current: Only honor 3A and 5A capable cables. */
        eudo_p->enterusb_vdo.cableCurrent = 0;
        if (
                (
                    (ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_PAS_CBL) ||
                    (ptrPdStackContext->dpmStat.cblVdo.std_cbl_vdo.vbusThruCbl != 0x0UL)
                ) &&
                (
                 (ptrPdStackContext->dpmStat.cblVdo.std_cbl_vdo.vbusCur == (uint32_t)CY_PDSTACK_CBL_VBUS_CUR_3A) ||
                  (ptrPdStackContext->dpmStat.cblVdo.std_cbl_vdo.vbusCur == (uint32_t)CY_PDSTACK_CBL_VBUS_CUR_5A)
                )
           )
        {
            eudo_p->enterusb_vdo.cableCurrent = ptrPdStackContext->dpmStat.cblVdo.std_cbl_vdo.vbusCur + 0x01UL;
        }

        /* Set USB4 not supported as default */
        ptrAltModeContext->vdmStat->usb4Flag = CY_PDALTMODE_USB4_FAILED;

        /* Active cable handler */
        if (ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_ACT_CBL)
        {
            if (
                    (ptrPdStackContext->dpmStat.cblVdmVersion == CY_PDSTACK_STD_VDM_VER2) &&
                    (ptrPdStackContext->dpmStat.cblVdo.act_cbl_vdo1.vdoVersion >= CY_PD_CBL_VDO_VERS_1_3)
                )
            {
                /* Active cable supports USB4 */
                if (
                        (ptrPdStackContext->dpmStat.cblVdo2.act_cbl_vdo2.usb4Supp == 0x0UL)   &&
                        (ptrPdStackContext->dpmStat.cblVdo.act_cbl_vdo.usbSsSup >= (uint32_t)CY_PDSTACK_USB_GEN_1)
                    )
                {
                    /* Set EUDO cable speed */
                    eudo_p->enterusb_vdo.cableSpeed = (uint32_t)ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_GetEudoCblSpeed)(ptrPdStackContext);

                    /* Enter USB4 mode */
                    ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_Enter)(ptrAltModeContext, CY_PD_SOP_PRIME, false);

                }
            }
            else if (ptrPdStackContext->dpmStat.cblModeEn != false)
            {
                /* Go through TBT discovery */
                ptrAltModeContext->vdmStat->usb4Flag = CY_PDALTMODE_USB4_TBT_CBL_FIND;
            }
            else
            {
                /* Do nothing */
            }
        }
        else
        {
            /* Set cable speed in EUDO. Since it is known that cable marker is present, a non-active cable has to be passive. */
            eudo_p->enterusb_vdo.cableSpeed = (uint32_t)ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_GetEudoCblSpeed)(ptrPdStackContext);

            if ((ptrPdStackContext->dpmStat.cblVdo.std_cbl_vdo.usbSsSup == (uint32_t)CY_PDSTACK_USB_GEN_2) &&
                     (ptrPdStackContext->dpmStat.cblModeEn != false))
            {
                /* Run TBT cable discovery */
                ptrAltModeContext->vdmStat->usb4Flag = CY_PDALTMODE_USB4_TBT_CBL_FIND;
            }
            else if (ptrPdStackContext->dpmStat.cblVdo.std_cbl_vdo.usbSsSup != (uint32_t)CY_PDSTACK_USB_2_0)
            {
                /* Enter USB4 mode */
                ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_Enter)(ptrAltModeContext, CY_PD_SOP, false);
            }
            else
            {
                /* Do nothing */
            }
        }
    }

    return ret;
}

ATTRIBUTES_ALT_MODE bool Cy_PdAltMode_VdmTask_IsUsb4Cap(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdaltmode_vdm_msg_info_t *msgP  = &ptrAltModeContext->vdmStat->vdmMsg;
    cy_pd_pd_do_t* vdo_do_p = ptrAltModeContext->appStatusContext->vdmIdVdoResp;
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    /* Check if USB4 mode is already entered */
    if (ptrAltModeContext->altModeAppStatus->usb4Active != false)
    {
        ptrAltModeContext->vdmStat->usb4Flag = CY_PDALTMODE_USB4_FAILED;
        if ((msgP->vdo[CY_PD_PRODUCT_TYPE_VDO_1_IDX - CY_PD_ID_HEADER_IDX].ufp_vdo_1.altModes & CY_PD_UFP_NON_PH_ALT_MODE_SUPP_MASK) == 0x0U)
        {
            return false;
        }
    }
    /* Check if CCG and port partner supports USB 4.0 */
    else if (
               ((ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_IsUSB4Supp)(msgP->vdo, msgP->vdoNumb) == false)                                                             ||
                (ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_IsUSB4Supp)(&vdo_do_p[CY_PD_ID_HEADER_IDX], (ptrAltModeContext->appStatusContext->vdmIdVdoCnt - CY_PD_ID_HEADER_IDX)) == false)) ||
                (ptrPdStackContext->dpmConfig.specRevSopLive < CY_PD_REV3)                                                                                             ||
                (ptrAltModeContext->altModeAppStatus->usb4DataRstCnt > CY_PDALTMODE_VDMTASK_DATA_RST_RETRY_NUMB)                                                                                 ||
               ((ptrAltModeContext->altModeAppStatus->customHpiHostCapControl & CY_PDALTMODE_USB4_EN_HOST_PARAM_MASK) == 0x0U)
            )
    {
        /*
         * Cannot proceed with USB4 if there is no USB4 host support, UFP does not have USB4
         * device support, in PD 2.0 contract, or if there has been too many data resets.
         */
        ptrAltModeContext->vdmStat->usb4Flag = CY_PDALTMODE_USB4_FAILED;
    }
    else
    {
        /* Do nothing */
    }

    return true;
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_VdmTask_HostCapCheck(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    cy_stc_pdaltmode_context_t *deviceContext =
                        (cy_stc_pdaltmode_context_t *)ptrAltModeContext->hostDetails->dsAltModeContext;

    /* If USB4 mode is NOT enabled - then do not issue USB4 mode enter command. */
    if(
           ((deviceContext->hostDetails->dsModeMask & (1u<<4u)) != (1u<<4u)) &&
           (ptrPdStackContext->port == TYPEC_PORT_1_IDX)
      )
    {
        ptrAltModeContext->vdmStat->usb4Flag = CY_PDALTMODE_USB4_FAILED;
    }
}
#endif /* CY_PD_USB4_SUPPORT_ENABLE */

ATTRIBUTES_ALT_MODE bool Cy_PdAltMode_VdmTask_GotoDiscSvid(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdaltmode_vdm_msg_info_t *msgP  = &ptrAltModeContext->vdmStat->vdmMsg;
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    /* Set SVID idx to zero before start Disc SVID */
    ptrAltModeContext->vdmStat->svidIdx  = CY_PDALTMODE_MNGR_NO_DATA;
    ptrAltModeContext->vdmStat->dSvidCnt = 0;

    /* Copy ID header to info struct */
    ptrAltModeContext->vdmStat->atchTgt.amaVdo.val = msgP->vdo[CY_PDALTMODE_VDMTASK_PD_DISC_ID_AMA_VDO_IDX].val;

    /* Copy AMA VDO to info struct */
    ptrAltModeContext->vdmStat->atchTgt.tgtIdHeader.val = msgP->vdo[CY_PDALTMODE_MNGR_VDO_START_IDX - 1U].val;

    /* If AMA and cable (if present) do not need Vconn, disable VConn */
    if (
            (ptrPdStackContext->dpmStat.vconnRetain == 0x0U) &&
            ((msgP->vdo[CY_PDALTMODE_MNGR_VDO_START_IDX - 1U].std_id_hdr.prodType != (uint32_t)CY_PDSTACK_PROD_TYPE_AMA) ||
             (msgP->vdo[CY_PDALTMODE_VDMTASK_PD_DISC_ID_AMA_VDO_IDX].std_ama_vdo.vconReq == 0x0UL)) &&
            ((ptrPdStackContext->dpmConfig.emcaPresent == false) ||
            (ptrPdStackContext->dpmStat.cblVdo.std_cbl_vdo.cblTerm == (uint32_t)CY_PDSTACK_CBL_TERM_BOTH_PAS_VCONN_NOT_REQ))
       )
    {
        ptrPdStackContext->ptrAppCbk->vconn_disable(ptrPdStackContext, ptrPdStackContext->dpmConfig.revPol);
    }

    /* If Alt Modes are not supported */
    if (msgP->vdo[CY_PDALTMODE_MNGR_VDO_START_IDX - 1U].std_id_hdr.modSupport == 0x0UL)
    {
        ptrAltModeContext->vdmStat->altModesNotSupp = 0x01U;
        /* Check if USB4 is supported */
        if ((ptrAltModeContext->altModeAppStatus->customHpiHostCapControl & CY_PDALTMODE_USB4_EN_HOST_PARAM_MASK) != 0u)
        {
           if (
                     (ptrAltModeContext->vdmStat->usb4Flag == CY_PDALTMODE_USB4_FAILED) ||
                     ((ptrPdStackContext->dpmConfig.emcaPresent == false)   &&
                      (ptrAltModeContext->vdmStat->usb4Flag != CY_PDALTMODE_USB4_FAILED))
               )
           {
               return false;
           }
        }
        else
        {
            return false;
        }
    }

    return true;
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_VdmTask_SetDiscParam(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t sop, cy_en_pdstack_std_vdm_cmd_t cmd)
{
    cy_stc_pdaltmode_vdm_msg_info_t *msgP = &ptrAltModeContext->vdmStat->vdmMsg;

    /* Form Discover ID VDM packet */
    msgP->vdmHeader.val                       = CY_PDALTMODE_MNGR_NO_DATA;
    msgP->vdmHeader.std_vdm_hdr.svid          = CY_PDATMODE_STD_SVID;
    msgP->vdmHeader.std_vdm_hdr.cmd           = (uint32_t)cmd;
    msgP->vdoNumb                             = CY_PDALTMODE_MNGR_NO_DATA;
    msgP->sopType                             = sop;
    msgP->vdmHeader.std_vdm_hdr.vdmType       = (uint32_t)CY_PDSTACK_VDM_TYPE_STRUCTURED;
}

ATTRIBUTES_ALT_MODE static void Cy_PdAltMode_VdmTask_UpdateUsbSupportVars(cy_stc_pdaltmode_context_t *ptrAltModeContext, bool ufp_ack)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    const cy_stc_pdstack_dpm_status_t   *dpmStat = &ptrPdStackContext->dpmStat;
    cy_stc_pdaltmode_status_t           *app      = ptrAltModeContext->altModeAppStatus;
    cy_stc_pdaltmode_vdm_msg_info_t     *msgP    = &ptrAltModeContext->vdmStat->vdmMsg;

    /* First set USB support flags based on cable marker properties. */
    if (dpmStat->cblType == CY_PDSTACK_PROD_TYPE_ACT_CBL)
    {
        if (dpmStat->cblVdo2.act_cbl_vdo2.usb2Supp != 0u)
        {
            app->usb2Supp = true;
        }
        if (dpmStat->cblVdo2.act_cbl_vdo2.ssSupp != 0u)
        {
            app->usb3Supp = true;
        }
    }
    else if (dpmStat->cblType == CY_PDSTACK_PROD_TYPE_PAS_CBL)
    {
        app->usb2Supp = true;

        if (dpmStat->cblVdo.pas_cbl_vdo.usbSsSup > ((uint32_t)CY_PDSTACK_USB_GEN_2))
        {
            app->usb3Supp = true;
        }
    }
    else
    {
     /* No action required - ; is optional */
    }

    /* If UFP response is available, confirm USB support in its D_ID response. */
    if (ufp_ack)
    {
        uint32_t dev_cap = msgP->vdo[CY_PD_PRODUCT_TYPE_VDO_1_IDX - CY_PDALTMODE_MNGR_VDO_START_IDX].ufp_vdo_1.devCap;

        if ((dev_cap & ((uint32_t)CY_PDSTACK_DEV_CAP_USB_2_0)) != 0u)
        {
            app->usb2Supp = true;
        }

        if ((dev_cap & ((uint32_t)CY_PDSTACK_DEV_CAP_USB_3_2)) != 0u)
        {
            app->usb3Supp = true;
        }
    }
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_MngDiscId(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_vdm_evt_t vdmEvt)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    cy_stc_pdaltmode_vdm_msg_info_t *msgP  = &ptrAltModeContext->vdmStat->vdmMsg;
    cy_en_pdaltmode_vdm_task_t ret         = CY_PDALTMODE_VDM_TASK_EXIT;

    bool pd3Ufp = ((ptrPdStackContext->dpmConfig.specRevSopLive >= CY_PD_REV3) &&
            (ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP));

    switch (vdmEvt)
    {
        case CY_PDALTMODE_VDM_EVT_RUN:
            /* Form Discover ID VDM packet */
            ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SetDiscParam)(ptrAltModeContext, (uint8_t)CY_PD_SOP, CY_PDSTACK_VDM_CMD_DSC_IDENTITY);
            ret  = CY_PDALTMODE_VDM_TASK_SEND_MSG;
            break;

        /* Evaluate received response */
        case CY_PDALTMODE_VDM_EVT_EVAL:
            /* Check is current port date role DFP */
            if(ptrPdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP)
            {
                if (ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_IsUsb4Cap)(ptrAltModeContext) == false)
                {
                    ret = CY_PDALTMODE_VDM_TASK_WAIT;
                    break;
                }

                /* Check if USB4 is allowed for Host Exchange capabilities */
                if (ptrAltModeContext->hostDetails != NULL)
                {
                    ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_HostCapCheck)(ptrAltModeContext);
                }

                /* Store the D_ID response received */
                ptrAltModeContext->vdmDiscIdResp[0] = msgP->vdmHeader.val;
                TIMER_CALL_MAP(Cy_PdUtils_MemCopy)((uint8_t *)&ptrAltModeContext->vdmDiscIdResp[1], (uint8_t *)msgP->vdo, (uint32_t)msgP->vdoNumb * sizeof(uint32_t));
                ptrAltModeContext->vdmDiscIdRespLen = msgP->vdoNumb + 1u;

                /* Analyze Disc ID SOP & SOP' responses for USB2/USB3 compatibility */
                Cy_PdAltMode_VdmTask_UpdateUsbSupportVars (ptrAltModeContext, true);

                /* Set EUDO parameters*/
                if (ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SetEudoParams)(ptrAltModeContext) != CY_PDALTMODE_VDM_TASK_EXIT)
                {
                    ret = CY_PDALTMODE_VDM_TASK_USB4_TBT;
                    break;
                }

                /* Check if Set Discovery SVID process can be provided */
                if (ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_GotoDiscSvid)(ptrAltModeContext) == false)
                {
                    break;
                }
                /* Send Disc SVID cmd */
                ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_RUN;
                ret = CY_PDALTMODE_VDM_TASK_DISC_SVID;
            }

            /* Check is current port data role UFP */
            if (pd3Ufp)
            {
                /* Send Disc SVID cmd */
                ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_RUN;
                ret = CY_PDALTMODE_VDM_TASK_DISC_SVID;
            }
            break;

        case CY_PDALTMODE_VDM_EVT_FAIL:
            /* Check is current port data role UFP */
            if (pd3Ufp)
            {
                /* Send Disc SVID cmd */
                ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_RUN;
                ret = CY_PDALTMODE_VDM_TASK_DISC_SVID;
            }
            /* Update USB support variables based on cable capabilities alone. */
            Cy_PdAltMode_VdmTask_UpdateUsbSupportVars (ptrAltModeContext, false);
            break;
        default:
            /* Do nothing */
            break;
    }

    return ret;
}

/* Checks if input SVID already saved */
ATTRIBUTES_ALT_MODE bool Cy_PdAltMode_VdmTask_IsSvidStored(uint16_t *svid_arr, uint16_t svid)
{
    uint8_t  idx;

    /* Go through all SVIDs and compare with input SVID */
    for (idx = 0; idx < CY_PDALTMODE_MAX_SVID_VDO_SUPP; idx++)
    {
        /* If input SVID is already saved */
        if (svid_arr[idx] == svid)
        {
            return true;
        }
    }

    return false;
}

/*
   This function saves received Discover SVID response,
   returns true if a NULL SVID was received.
 */
ATTRIBUTES_ALT_MODE bool Cy_PdAltMode_VdmTask_SaveSvids(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint16_t *svid_arr, uint8_t max_svid)
{
    uint8_t         idx, vdo_count;
    uint16_t        svid;
    uint8_t         svidIdx = ptrAltModeContext->vdmStat->svidIdx;
    bool            retval   = false;
    cy_stc_pdaltmode_vdm_msg_info_t  *msgP   = &ptrAltModeContext->vdmStat->vdmMsg;

    /* Compare received SVIDs with supported SVIDs */
    vdo_count = msgP->vdoNumb;

    /* Stop further discovery if this response does not have the maximum number of DOs. */
    if (vdo_count < (CY_PD_MAX_NO_OF_DO - 1U))
    {
        retval = true;
    }

    for (idx = 0U; idx < (vdo_count * 2U); idx++)
    {
        if ((idx & 1U) == 0U)
        {
            /* Upper half of the DO */
            svid = (uint16_t)(msgP->vdo[idx >> 1].val >> 16u);
        }
        else
        {
            /* Lower half of the DO */
            svid = (uint16_t)(msgP->vdo[idx >> 1].val & 0xFFFFu);
        }

        if (svidIdx < (max_svid - 1U))
        {
            /* Stop on NULL SVID */
            if (svid == CY_PDALTMODE_MNGR_NO_DATA)
            {
                retval = true;
            }
            else
            {
                /* If SVID is not saved previously then save */
                if (ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_IsSvidStored)(svid_arr, svid) == false)
                {
                    if (
                            (ALT_MODE_CALL_MAP(Cy_PdAltMode_Mngr_GetAltModesConfigSvidIdx)(ptrAltModeContext, ptrAltModeContext->pdStackContext->dpmConfig.curPortType, svid) != CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED)
                         || ((svid == CY_PDALTMODE_TBT_SVID) && (ptrAltModeContext->vdmStat->vdmMsg.sopType == ((uint32_t)CY_PD_SOP_PRIME)))
                        )
                    {
                        svid_arr[svidIdx] = svid;
                        svidIdx++;
                    }
                }

                /* Check if TBT SVID is supported by cable, if USB4 related
                 * TBT cable Disc Mode procedure should be provided.
                 */
                if (
                       (ptrAltModeContext->vdmStat->vdmMsg.sopType == (uint8_t)CY_PD_SOP_PRIME) &&
                       (ptrAltModeContext->vdmStat->usb4Flag == CY_PDALTMODE_USB4_TBT_CBL_FIND) &&
                       (svid == CY_PDALTMODE_TBT_SVID)
                    )
                {
                    /* Start TBT cable Discover mode process */
                    ptrAltModeContext->vdmStat->usb4Flag = CY_PDALTMODE_USB4_TBT_CBL_DISC_RUN;
                }
            }
        }
        else
        {
            /* Cannot continue as there is no more space. */
            retval = true;
            break;
        }
    }

    /* Set zero after last SVID in info */
    svid_arr[svidIdx] = CY_PDALTMODE_MNGR_NO_DATA;
    ptrAltModeContext->vdmStat->svidIdx = svidIdx;

    /* Terminate discovery after CY_PDALTMODE_VDMTASK_MAX_DISC_SVID_COUNT attempts. */
    ptrAltModeContext->vdmStat->dSvidCnt++;
    if (ptrAltModeContext->vdmStat->dSvidCnt >= CY_PDALTMODE_VDMTASK_MAX_DISC_SVID_COUNT){
        retval = true;
    }

    return retval;
}

bool Cy_PdAltMode_VdmTask_IsVconnPresent(cy_stc_pdstack_context_t *ptrPdStackContext, uint8_t channel)
{
    return Cy_USBPD_Vconn_IsPresent(ptrPdStackContext->ptrUsbPdContext, channel);
}

ATTRIBUTES_ALT_MODE bool Cy_PdAltMode_VdmTask_IsCblSvidReq(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    uint8_t faultStatus = ptrAltModeContext->appStatusContext->faultStatus;

    bool ret = false;

    /* If cable supports Alternate Modes, send SOP' disc SVID */
    if(ptrPdStackContext->dpmStat.cblModeEn != false)
    {
        if ((ptrPdStackContext->dpmConfig.vconnLogical) && ((faultStatus & CY_PDALTMODE_FAULT_APP_PORT_VCONN_FAULT_ACTIVE) == 0u))
        {
            if(ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_IsVconnPresent)(ptrPdStackContext, ptrPdStackContext->dpmConfig.revPol) == false)
            {
                /*
                 * Currently you are the VConn source, and VConn is off. Enable and apply a delay to let
                 * the EMCA power up.
                 */
                if (ptrAltModeContext->pdStackContext->ptrAppCbk->vconn_enable (ptrPdStackContext, ptrPdStackContext->dpmConfig.revPol))
                {
                    /* Set a flag to indicate that there is need for cable SVID checks */
                    ret = true;
                    /* Start a timer to delay the retry attempt */
                    (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start) (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                            CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_VDM_BUSY_TIMER),
                                CY_PDALTMODE_APP_CABLE_POWER_UP_DELAY, NULL);
                }
            }
            else
            {
                /* Set a flag to indicate that there is need for cable SVID checks */
                ret = true;
            }
        }
    }    
    
    return ret;
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_HandleSopSvidResp(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_en_pdaltmode_vdm_task_t          ret        = CY_PDALTMODE_VDM_TASK_EXIT;
    cy_stc_pdaltmode_vdm_msg_info_t     *msgP      = &ptrAltModeContext->vdmStat->vdmMsg;
    uint8_t tmp;

    if (ptrAltModeContext->vdmStat->svidIdx == 0u)
    {
        /* Save the Disc SVID response */
        ptrAltModeContext->vdmDiscSvidResp[0] = (uint32_t)msgP->vdmHeader.val;
        TIMER_CALL_MAP(Cy_PdUtils_MemCopy) ((uint8_t *)&ptrAltModeContext->vdmDiscSvidResp[1], (uint8_t *)msgP->vdo, (uint32_t)msgP->vdoNumb * sizeof (uint32_t));
        ptrAltModeContext->vdmDiscSvidRespLen = msgP->vdoNumb + 1u;
    }
    else
    {
        /* Save the incremental Disc SVID response */
        tmp = (msgP->vdoNumb > ((uint8_t)CY_PDALTMODE_MAX_DISC_SVID_RESP_LEN - ptrAltModeContext->vdmDiscSvidRespLen)) ?
                    ((uint8_t)CY_PDALTMODE_MAX_DISC_SVID_RESP_LEN - ptrAltModeContext->vdmDiscSvidRespLen) : msgP->vdoNumb;
        TIMER_CALL_MAP(Cy_PdUtils_MemCopy) ((uint8_t *)&ptrAltModeContext->vdmDiscSvidResp[ptrAltModeContext->vdmDiscSvidRespLen], (uint8_t *)msgP->vdo,
                (uint32_t)tmp * sizeof(uint32_t));
        ptrAltModeContext->vdmDiscSvidRespLen += tmp;
    }

    if (
            (msgP->vdo[0].val == CY_PDALTMODE_MNGR_NO_DATA) &&
            (ptrAltModeContext->vdmStat->dSvidCnt == 0x00U)
        )
    {
        /* No SVID supported */
        return ret;
    }

    /* Save received SVIDs and check if a NULL SVID was received. */
    if (ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SaveSvids)(ptrAltModeContext, ptrAltModeContext->vdmStat->atchTgt.tgtSvid, CY_PDALTMODE_MAX_SVID_VDO_SUPP) != false)
    {
        /* Check if cable Disc SVID should be done */
        if (ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_IsCblSvidReq)(ptrAltModeContext) != false)
        {
            ptrAltModeContext->vdmStat->svidIdx  = 0;
            ptrAltModeContext->vdmStat->dSvidCnt = 0;
            ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SetDiscParam)(ptrAltModeContext, (uint8_t)CY_PD_SOP_PRIME, CY_PDSTACK_VDM_CMD_DSC_SVIDS);
            ret = CY_PDALTMODE_VDM_TASK_SEND_MSG;
        }
        else
        {
            /*
             * You are now either not the VConn source, or failed to turn the VConn ON.
             * Skip SOP' checks in the unlikely case where this happens.
             */
            ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_RUN;
            ret = CY_PDALTMODE_VDM_TASK_REG_ATCH_TGT_INFO;
        }
    }
    else
    {
        /* If not all SVID is received, ask for the next set of SVIDs. */
        ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SetDiscParam)(ptrAltModeContext, (uint8_t)CY_PD_SOP, CY_PDSTACK_VDM_CMD_DSC_SVIDS);
        ret = CY_PDALTMODE_VDM_TASK_SEND_MSG;
    }

    return ret;
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_HandleCblSvidResp(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_en_pdaltmode_vdm_task_t          ret     = CY_PDALTMODE_VDM_TASK_EXIT;
    cy_stc_pdaltmode_vdm_msg_info_t     *msgP   = &ptrAltModeContext->vdmStat->vdmMsg;
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    /* If the EMCA returned any DOs, save the content. */
    if ((msgP->vdo[CY_PDALTMODE_MNGR_VDO_START_IDX - 1U].val != CY_PDALTMODE_MNGR_NO_DATA) &&
            (ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SaveSvids)(ptrAltModeContext, ptrAltModeContext->vdmStat->atchTgt.cblSvid, CY_PDALTMODE_VDMTASK_MAX_CABLE_SVID_SUPP) == false))
    {
        /* If not all SVID is received, ask for the next set of SVIDs. */
        ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SetDiscParam)(ptrAltModeContext, (uint8_t)CY_PD_SOP_PRIME, CY_PDSTACK_VDM_CMD_DSC_SVIDS);
        ret = CY_PDALTMODE_VDM_TASK_SEND_MSG;
    }
    else
    {
        /*
           If EMCA did not support any SVIDs of interest and does not require VConn for operation, disable VConn.
         */
        if (ptrAltModeContext->vdmStat->atchTgt.cblSvid[0] == CY_PDALTMODE_MNGR_NO_DATA)
        {
            if (
                    (ptrPdStackContext->dpmStat.vconnRetain == 0x0U) &&
                    (ptrPdStackContext->dpmStat.cblVdo.std_cbl_vdo.cblTerm == (uint32_t)CY_PDSTACK_CBL_TERM_BOTH_PAS_VCONN_NOT_REQ)
                    )
            {
                ptrPdStackContext->ptrAppCbk->vconn_disable(ptrPdStackContext, ptrPdStackContext->dpmConfig.revPol);
            }
        }

        /* Move to the next step */
        ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_RUN;
        ret = CY_PDALTMODE_VDM_TASK_REG_ATCH_TGT_INFO;

        if (ptrAltModeContext->vdmStat->usb4Flag == CY_PDALTMODE_USB4_TBT_CBL_DISC_RUN)
        {
            /* Run Enter USB4 related Disc mode process */
            ret = CY_PDALTMODE_VDM_TASK_USB4_TBT;
        }
        else if (
                    (ptrAltModeContext->vdmStat->usb4Flag == CY_PDALTMODE_USB4_TBT_CBL_FIND) &&
                    (ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_PAS_CBL)
                )
        {
            /* Enter USB4 mode */
            ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_Enter)(ptrAltModeContext, CY_PD_SOP, false);
        }
        else
        {
            /* Do nothing */
        }
    }

    return ret;
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_HandleFailSvidResp(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_en_pdaltmode_vdm_task_t          ret        = CY_PDALTMODE_VDM_TASK_EXIT;
    cy_stc_pdaltmode_vdm_msg_info_t     *msgP      = &ptrAltModeContext->vdmStat->vdmMsg;
    cy_stc_pdstack_context_t *ptrPdStackContext    = ptrAltModeContext->pdStackContext;

    if (ptrAltModeContext->altModeAppStatus->usb4Active == false)
    {
        if (ptrAltModeContext->vdmStat->usb4Flag > CY_PDALTMODE_USB4_FAILED)
        {
            if (ptrPdStackContext->dpmStat.cblType == CY_PDSTACK_PROD_TYPE_PAS_CBL)
            {
                /* Enter USB4 mode */
                ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_Usb4_Enter)(ptrAltModeContext, CY_PD_SOP, false);
            }
            else
            {
                /* Resume VDM handling */
                ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_ResumeHandler)(ptrAltModeContext);
            }
            return ret;
        }
    }
    else
    {
        return CY_PDALTMODE_VDM_TASK_WAIT;
    }

    /* If cable SVID fails */
    if (msgP->sopType == (uint8_t)CY_PD_SOP_PRIME)
    {
        ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_RUN;
        ret = CY_PDALTMODE_VDM_TASK_REG_ATCH_TGT_INFO;
    }

    return ret;
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_MngDiscSvid(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_vdm_evt_t vdmEvt)
{
    cy_stc_pdstack_context_t *ptrPdStackContext    = ptrAltModeContext->pdStackContext;
    cy_en_pdaltmode_vdm_task_t          ret        = CY_PDALTMODE_VDM_TASK_EXIT;
    cy_stc_pdaltmode_vdm_msg_info_t     *msgP      = &ptrAltModeContext->vdmStat->vdmMsg;

    bool pd3Ufp = ((ptrPdStackContext->dpmConfig.specRevSopLive >= CY_PD_REV3) &&
            (ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP));

    switch (vdmEvt)
    {
        case CY_PDALTMODE_VDM_EVT_RUN:
            if (ptrAltModeContext->vdmStat->usb4Flag == CY_PDALTMODE_USB4_TBT_CBL_FIND)
            {
                /* Enable Vconn if required */
                (void)ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_IsCblSvidReq)(ptrAltModeContext);

                /* Form Discover SVID VDM packet to find out TBT cable */
                ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SetDiscParam)(ptrAltModeContext, (uint8_t)CY_PD_SOP_PRIME, CY_PDSTACK_VDM_CMD_DSC_SVIDS);
                ret  = CY_PDALTMODE_VDM_TASK_SEND_MSG;
            }
            else
            {
                /* Form Discover SVID VDM packet */
                ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SetDiscParam)(ptrAltModeContext, (uint8_t)CY_PD_SOP, CY_PDSTACK_VDM_CMD_DSC_SVIDS);
                ret  = CY_PDALTMODE_VDM_TASK_SEND_MSG;
            }
            break;

        case CY_PDALTMODE_VDM_EVT_EVAL:
            /* Check is current port data role DFP */
            if(ptrPdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP)
            {
                /* For attached target response */
                if (msgP->sopType == (uint8_t)CY_PD_SOP)
                {
                    ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_HandleSopSvidResp)(ptrAltModeContext);
                }
                /* For cable response */
                else
                {
                    ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_HandleCblSvidResp)(ptrAltModeContext);
                }
            }

            /* Check is current port data role UFP */
            if (pd3Ufp)
            {
                /* Send Disc SVID cmd */
                ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_RUN;
                ret = CY_PDALTMODE_VDM_TASK_REG_ATCH_TGT_INFO;
            }
            break;

        case CY_PDALTMODE_VDM_EVT_FAIL:
            ret = ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_HandleFailSvidResp)(ptrAltModeContext);
            break;

        default:
            /* Do nothing */
            break;
    }

    return ret;
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_VdmTask_ResetMngr(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    ptrAltModeContext->vdmStat->recRetryCnt       = CY_PDALTMODE_MNGR_NO_DATA;
    ptrAltModeContext->vdmStat->svidIdx           = CY_PDALTMODE_MNGR_NO_DATA;
    ptrAltModeContext->vdmStat->dSvidCnt          = CY_PDALTMODE_MNGR_NO_DATA;
    ptrAltModeContext->vdmStat->usb4UpdatePending = 0x00U;
    ptrAltModeContext->vdmStat->usb4EnterRetryCount = CY_PDALTMODE_MNGR_NO_DATA;
    ptrAltModeContext->vdmStat->altModesNotSupp   = CY_PDALTMODE_MNGR_NO_DATA;
    ptrAltModeContext->vdmStat->usb4Flag          = CY_PDALTMODE_USB4_NONE;
    ptrAltModeContext->vdmStat->vdmVcsRqtState    = CY_PDALTMODE_VCONN_RQT_INACTIVE;
    ptrAltModeContext->vdmStat->vdmVcsRqtCount    = 0;
    /* Clear the SOP'' reset state */
    ptrAltModeContext->vdmStat->vdmEmcaRstState   = CY_PDALTMODE_CABLE_DP_RESET_IDLE;
    ptrAltModeContext->vdmStat->vdmEmcaRstCount   = 0;

    /* Clear EUDO buffer */
    UTILS_CALL_MAP(Cy_PdUtils_MemSet)((uint8_t *)&ptrAltModeContext->vdmStat->eudoBuf, CY_PDALTMODE_MNGR_NO_DATA, sizeof(cy_stc_pdstack_dpm_pd_cmd_buf_t));
    /* Clear arrays which hold SVIDs */
    UTILS_CALL_MAP(Cy_PdUtils_MemSet)((uint8_t *)&ptrAltModeContext->vdmStat->atchTgt, CY_PDALTMODE_MNGR_NO_DATA, sizeof(cy_stc_pdaltmode_atch_tgt_info_t));
    /* Store the pointer to the cable VDO discovered by PD Stack */
    ptrAltModeContext->vdmStat->atchTgt.cblVdo = (cy_pd_pd_do_t*)&ptrPdStackContext->dpmStat.cblVdo;
    /* Stop the VDM task delay timer */
    TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_VDM_BUSY_TIMER));
}

ATTRIBUTES_ALT_MODE cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_ResumeHandler(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    /* Remove USB4 pending flag to allow VDM manager handling */
    ptrAltModeContext->vdmStat->usb4Flag = CY_PDALTMODE_USB4_FAILED;
    ptrAltModeContext->vdmStat->svidIdx  = CY_PDALTMODE_MNGR_NO_DATA;
    UTILS_CALL_MAP(Cy_PdUtils_MemSet)((uint8_t *)ptrAltModeContext->vdmStat->atchTgt.cblSvid, CY_PDALTMODE_MNGR_NO_DATA, sizeof(ptrAltModeContext->vdmStat->atchTgt.cblSvid));
    UTILS_CALL_MAP(Cy_PdUtils_MemSet)((uint8_t *)ptrAltModeContext->vdmStat->atchTgt.tgtSvid, CY_PDALTMODE_MNGR_NO_DATA, sizeof(ptrAltModeContext->vdmStat->atchTgt.tgtSvid));
    ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_RUN;

    /* Resume VDM flow or exit VDM handler based on Alt Mode support flag */
    ptrAltModeContext->vdmStat->vdmTask = (ptrAltModeContext->vdmStat->altModesNotSupp == 0x0U) ? CY_PDALTMODE_VDM_TASK_DISC_SVID : CY_PDALTMODE_VDM_TASK_EXIT;

    return ptrAltModeContext->vdmStat->vdmTask;
}

void Cy_PdAltMode_VdmTask_MngrDeInit(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    ptrAltModeContext->altModeAppStatus->vdmPrcsFailed = false;

    /* If VDM task is not active, no need to go through the rest of the steps. */
    if (ptrAltModeContext->altModeAppStatus->vdmTaskEn != 0x0U)
    {
        ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_ResetMngr)(ptrAltModeContext);
        /* Clear all application status flags. */
        ptrAltModeContext->altModeAppStatus->trigTbtVdoDisc     = false;
        ptrAltModeContext->vdmStat->usb4UpdatePending           = 0x00U;
        ptrAltModeContext->altModeAppStatus->vdmTaskEn          = 0x0U;
        ptrAltModeContext->altModeAppStatus->altModeEntered     = false;
        ptrAltModeContext->altModeAppStatus->cblRstDone         = false;
        ptrAltModeContext->altModeAppStatus->trigCblRst         = false;
        ptrAltModeContext->altModeAppStatus->tbtCblEntered      = false;

        /* Exit from Alt Mode manager */
        (void)Cy_PdAltMode_Mngr_AltModeProcess(ptrAltModeContext, CY_PDALTMODE_VDM_EVT_EXIT);

        /* Skip MUX update if port was de-attached */
        if (ptrPdStackContext->dpmConfig.attach == false)
        {
            ptrAltModeContext->altModeAppStatus->skipMuxConfig = true;
        }

        /* Deinit App HW */
        Cy_PdAltMode_HW_DeInit(ptrAltModeContext);
        
        ptrAltModeContext->altModeAppStatus->skipMuxConfig    = false;
    }

#if CY_PD_USB4_SUPPORT_ENABLE
    /* Reset the Enter USB retry count */
    ptrAltModeContext->vdmStat->usb4EnterRetryCount = 0u;
#endif /* CY_PD_USB4_SUPPORT_ENABLE */
}

void Cy_PdAltMode_VdmTask_MngrExitModes(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    if (
            (ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_DFP) &&
            (ptrAltModeContext->altModeAppStatus->altModeEntered != false)
       )
    {
        /* Exit from Alt Mode manager */
        (void)Cy_PdAltMode_Mngr_AltModeProcess(ptrAltModeContext, CY_PDALTMODE_VDM_EVT_EXIT);
    }
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_VdmTask_SopDpSoftResetCb(cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_resp_status_t resp, const cy_stc_pdstack_pd_packet_t *pktPtr)
{
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    switch (resp)
    {
        case CY_PDSTACK_CMD_SENT:
            /* Do nothing */
            break;

        case CY_PDSTACK_RES_RCVD:
            /* Proceed with rest of Alternate mode state machine */
            ptrAltModeContext->vdmStat->vdmEmcaRstState = CY_PDALTMODE_CABLE_DP_RESET_DONE;
            break;

        default:
            /* Retry the cable SOFT_RESET */
            ptrAltModeContext->vdmStat->vdmEmcaRstCount++;
            if (ptrAltModeContext->vdmStat->vdmEmcaRstCount >= CY_PDALTMODE_VDMTASK_MAX_EMCA_DP_RESET_COUNT)
            {
                ptrAltModeContext->vdmStat->vdmEmcaRstState = CY_PDALTMODE_CABLE_DP_RESET_DONE;
            }
            else
            {
                ptrAltModeContext->vdmStat->vdmEmcaRstState = CY_PDALTMODE_CABLE_DP_RESET_RETRY;
            }
            break;
    }

    (void) pktPtr;
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_VdmTask_SendSopDpSoftReset(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    cy_stc_pdstack_dpm_pd_cmd_buf_t dpm_cmd_param;
    cy_en_pdstack_status_t     api_stat;

    dpm_cmd_param.cmdSop      = CY_PD_SOP_DPRIME;
    dpm_cmd_param.noOfCmdDo   = 0;
    dpm_cmd_param.datPtr      = NULL;
    dpm_cmd_param.timeout     = 15;

    api_stat = PDSTACK_CALL_MAP(Cy_PdStack_Dpm_SendPdCommand)(ptrPdStackContext, CY_PDSTACK_DPM_CMD_SEND_SOFT_RESET_EMCA, &dpm_cmd_param, true, Cy_PdAltMode_VdmTask_SopDpSoftResetCb);
    switch (api_stat)
    {
        case CY_PDSTACK_STAT_SUCCESS:
            /* Wait for the cable SOFT_RESET response */
            ptrAltModeContext->vdmStat->vdmEmcaRstState = CY_PDALTMODE_CABLE_DP_RESET_WAIT;
            break;

        case CY_PDSTACK_STAT_BUSY:
        case CY_PDSTACK_STAT_NOT_READY:
            /* Need to retry the command */
            ptrAltModeContext->vdmStat->vdmEmcaRstState = CY_PDALTMODE_CABLE_DP_RESET_RETRY;
            break;

        default:
            /* Connection state changed. Abort the cable SOFT_RESET process. */
            ptrAltModeContext->vdmStat->vdmEmcaRstState = CY_PDALTMODE_CABLE_DP_RESET_DONE;
            break;
    }
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_VdmTask_VconnSwapCb(cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_resp_status_t resp, const cy_stc_pdstack_pd_packet_t *pktPtr)
{
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    switch (resp)
    {
        case CY_PDSTACK_CMD_SENT:
            /* Do nothing */
            break;

        case CY_PDSTACK_RES_RCVD:
            /* Proceed with rest of Alternate Mode state machine */
            switch ((cy_en_pd_ctrl_msg_t)pktPtr->msg)
            {
                case CY_PD_CTRL_MSG_ACCEPT:
                    /*
                     * If EMCA has already been detected, apply a delay and proceed with next steps.
                     * Otherwise, keep waiting for cable discovery-related events.
                     */
                    if(ptrPdStackContext->dpmConfig.emcaPresent)
                    {
                        (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_StartWocb)(ptrPdStackContext->ptrTimerContext,
                                CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_VDM_BUSY_TIMER), ptrAltModeContext->vdmStat->appCblStartDelay);
                        ptrAltModeContext->vdmStat->vdmVcsRqtState = CY_PDALTMODE_VCONN_RQT_INACTIVE;
                    }
                    break;

                case CY_PD_CTRL_MSG_WAIT:
                    ptrAltModeContext->vdmStat->vdmVcsRqtCount++;
                    if (ptrAltModeContext->vdmStat->vdmVcsRqtCount >= 10u)
                    {
                        ptrAltModeContext->vdmStat->vdmVcsRqtCount = (uint8_t)CY_PDALTMODE_VCONN_RQT_FAILED;
                    }
                    else
                    {
                        /* Start a timer to repeat the VConn Swap request after a delay. */
                        (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_StartWocb)(ptrPdStackContext->ptrTimerContext,
                                CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_VDM_BUSY_TIMER), 10u);
                    }
                    break;

                default:
                    ptrAltModeContext->vdmStat->vdmVcsRqtState = CY_PDALTMODE_VCONN_RQT_FAILED;
                    break;
            }
            break;

        default:
            /* Keep retrying the VConn Swap request */
            ptrAltModeContext->vdmStat->vdmVcsRqtState = CY_PDALTMODE_VCONN_RQT_PENDING;
            break;
    }
}

/* Function to initiate VConn Swap and cable discovery before continuing with the next SOP'/SOP'' message. */
bool Cy_PdAltMode_VdmTask_InitiateVcsCblDiscovery(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    cy_stc_pdstack_dpm_pd_cmd_buf_t  dpm_cmd_param;
    cy_en_pdstack_status_t           api_stat;
    bool                stat = false;

    if(ptrPdStackContext->dpmConfig.vconnLogical == false)
    {
        /* Disable automatic VConn Swap from the Policy Engine at this stage. */
        (void)Cy_PdStack_Dpm_UpdateAutoVcsEnable(ptrPdStackContext, false);
        (void)Cy_PdStack_Dpm_UpdateVconnRetain(ptrPdStackContext, 1u);

        if (ptrAltModeContext->vdmStat->vdmVcsRqtState <= CY_PDALTMODE_VCONN_RQT_ONGOING)
        {
            /* VDM sequence needs to be delayed because VConn Swap needs to be done. */
            stat = true;

            if (ptrAltModeContext->vdmStat->vdmVcsRqtState != CY_PDALTMODE_VCONN_RQT_ONGOING)
            {
#if CY_PD_DP_VCONN_SWAP_FEATURE
                uint8_t faultStatus = ptrAltModeContext->appStatusContext->faultStatus;

                if (
                        (!Cy_USBPD_V5V_IsSupplyOn(ptrPdStackContext->ptrUsbPdContext)) ||
                        ((faultStatus & CY_PDALTMODE_FAULT_APP_PORT_VCONN_FAULT_ACTIVE) != 0U)
                   )
                {
                    /* Skip the VConn Swap sequence if V5V is not up or if there is VConn OCP fault status. */
                    ptrAltModeContext->vdmStat->vdmVcsRqtState = CY_PDALTMODE_VCONN_RQT_FAILED;
                    stat = false;
                }
                else
#endif /* CY_PD_DP_VCONN_SWAP_FEATURE */
                {
                    dpm_cmd_param.cmdSop      = CY_PD_SOP;
                    dpm_cmd_param.noOfCmdDo   = 0u;
                    dpm_cmd_param.datPtr      = NULL;
                    dpm_cmd_param.timeout     = ptrPdStackContext->senderRspTimeout;

                    api_stat = PDSTACK_CALL_MAP(Cy_PdStack_Dpm_SendPdCommand)(ptrPdStackContext, CY_PDSTACK_DPM_CMD_SEND_VCONN_SWAP, &dpm_cmd_param, false, ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_VconnSwapCb));
                    if (api_stat != CY_PDSTACK_STAT_SUCCESS)
                    {
                        ptrAltModeContext->vdmStat->vdmVcsRqtState = CY_PDALTMODE_VCONN_RQT_PENDING;
                    }
                    else
                    {
                        /* Clear the emca_present flag for the port so that the cable state machine will start
                         * from the beginning.
                         */
                        ptrPdStackContext->dpmConfig.emcaPresent = false;

                        ptrAltModeContext->vdmStat->vdmVcsRqtState = CY_PDALTMODE_VCONN_RQT_ONGOING;
                    }
                }
            }
        }
    }
    else
    {
        if (ptrAltModeContext->vdmStat->vdmVcsRqtState != CY_PDALTMODE_VCONN_RQT_ONGOING)
        {
            ptrAltModeContext->vdmStat->vdmVcsRqtState = CY_PDALTMODE_VCONN_RQT_INACTIVE;
            ptrAltModeContext->vdmStat->vdmVcsRqtCount = 0;

            if(!ptrPdStackContext->ptrAppCbk->vconn_is_present(ptrPdStackContext))
            {
                /* Make sure VConn is enabled and sufficient delay is provided for the cable to power up. */
                if (ptrPdStackContext->ptrAppCbk->vconn_enable (ptrPdStackContext, ptrPdStackContext->dpmConfig.revPol))
                {
                    (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_StartWocb)(ptrPdStackContext->ptrTimerContext,
                            CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_VDM_BUSY_TIMER), ptrAltModeContext->vdmStat->appCblStartDelay);
                        stat = true;
                }
            }
        }
        else
        {
            /* In case cable discovery is pending, wait for it. */
            stat = true;
        }
    }

    return stat;
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_VdmTask_UpdateVcsStatus(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    if (ptrAltModeContext->vdmStat->vdmVcsRqtState == CY_PDALTMODE_VCONN_RQT_ONGOING)
    {
        ptrAltModeContext->vdmStat->vdmVcsRqtState = CY_PDALTMODE_VCONN_RQT_INACTIVE;
    }
}

/* Fill DPM cmd buffer with proper VDM info */
ATTRIBUTES_ALT_MODE uint8_t Cy_PdAltMode_VdmTask_ComposeVdm(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    cy_stc_pd_dpm_config_t* ptrDpmConfig = &(ptrPdStackContext->dpmConfig);
    uint8_t           idx;
    cy_stc_pdstack_dpm_pd_cmd_buf_t  *vdmBuf = &ptrAltModeContext->altModeMngrStatus->vdmBuf;
    cy_stc_pdaltmode_vdm_msg_info_t    *msgP   = &ptrAltModeContext->vdmStat->vdmMsg;

    vdmBuf->cmdSop = (cy_en_pd_sop_t) msgP->sopType;
    vdmBuf->noOfCmdDo = msgP->vdoNumb + CY_PDALTMODE_MNGR_VDO_START_IDX;
    vdmBuf->cmdDo[CY_PD_VDM_HEADER_IDX] = msgP->vdmHeader;

    if (msgP->vdmHeader.std_vdm_hdr.vdmType == (uint32_t)CY_PDSTACK_VDM_TYPE_STRUCTURED)
    {
        msgP->vdmHeader.std_vdm_hdr.cmdType = (uint32_t)CY_PDSTACK_CMD_TYPE_INITIATOR;

        if (vdmBuf->cmdSop == CY_PD_SOP)
        {
            vdmBuf->cmdDo[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stVer = ptrAltModeContext->appStatusContext->vdmVersion;
            vdmBuf->cmdDo[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stMinVer = ptrAltModeContext->appStatusContext->vdmMinorVersion;
        }
        else
        {
            vdmBuf->cmdDo[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stVer = (uint32_t)ptrPdStackContext->dpmStat.cblVdmVersion;
            vdmBuf->cmdDo[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stMinVer = (uint32_t)ptrPdStackContext->cblVdmMinVersion;
        }

        /* Downgrade SOP/cable VMD version in case PD spec revision is 2.0 */
        if (
                (ptrDpmConfig->specRevSopLive == CY_PD_REV2) ||
                ((vdmBuf->cmdSop != CY_PD_SOP) && (ptrPdStackContext->dpmStat.specRevSopPrimeLive  == CY_PD_REV2))
            )
        {
            vdmBuf->cmdDo[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stVer = (uint32_t)CY_PDSTACK_STD_VDM_VER1;
            vdmBuf->cmdDo[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stMinVer = (uint32_t)CY_PDSTACK_STD_VDM_MINOR_VER0;
        }

        /* Set exceptions for Enter/Exit mode cmd period */
        switch (msgP->vdmHeader.std_vdm_hdr.cmd)
        {
            case (uint32_t)CY_PDSTACK_VDM_CMD_ENTER_MODE:
                vdmBuf->timeout = CY_PD_VDM_ENTER_MODE_RESPONSE_TIMER_PERIOD;
                break;
            case (uint32_t)CY_PDSTACK_VDM_CMD_EXIT_MODE:
                vdmBuf->timeout = CY_PD_VDM_EXIT_MODE_RESPONSE_TIMER_PERIOD;
                break;
            case (uint32_t)CY_PDSTACK_VDM_CMD_ATTENTION:
                /* No timeout required for attention messages */
                vdmBuf->timeout = CY_PDALTMODE_MNGR_NO_DATA;
                break;
            default:
                vdmBuf->timeout = CY_PD_VDM_RESPONSE_TIMER_PERIOD;
                break;
        }
    }
    /* Handle UVDM */
    else
    {
        vdmBuf->timeout = CY_PD_VDM_RESPONSE_TIMER_PERIOD;
    }

    /* Copy VDOs to send buffer */
    if (msgP->vdoNumb > CY_PDALTMODE_MNGR_NO_DATA)
    {
        for (idx = 0; idx < msgP->vdoNumb; idx++)
        {
            vdmBuf->cmdDo[idx + CY_PDALTMODE_MNGR_VDO_START_IDX].val = msgP->vdo[idx].val;
        }
    }

    return 0x01U;
}

/* Parse received VDM and save info  */
ATTRIBUTES_ALT_MODE uint8_t Cy_PdAltMode_VdmTask_ParseVdm(cy_stc_pdaltmode_context_t *ptrAltModeContext, const cy_stc_pdstack_pd_packet_t* recVdm)
{
    uint8_t vdoIdx;
    cy_stc_pdaltmode_vdm_msg_info_t *msgP  = &ptrAltModeContext->vdmStat->vdmMsg;

    msgP->vdmHeader = recVdm->dat[CY_PD_VDM_HEADER_IDX];
    msgP->vdoNumb = (recVdm->len - CY_PDALTMODE_MNGR_VDO_START_IDX);
    msgP->sopType = (uint8_t)recVdm->sop;

    /* If VDO is present in received VDM */
    if (recVdm->len > CY_PDALTMODE_MNGR_VDO_START_IDX)
    {
        for (vdoIdx = 0; vdoIdx < recVdm->len; vdoIdx++)
        {
            msgP->vdo[vdoIdx].val = recVdm->dat[vdoIdx + CY_PDALTMODE_MNGR_VDO_START_IDX].val;
        }
    }

    return 0x01U;
}

static cy_en_pdaltmode_vdm_task_t Cy_PdAltMode_VdmTask_SendVdm(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
    cy_en_pd_sop_t pkt_type = ptrAltModeContext->altModeMngrStatus->vdmBuf.cmdSop;

    if((ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_DFP) && (ptrPdStackContext->dpmConfig.attach != false))
    {
        if (pkt_type != CY_PD_SOP)
        {
            /* If VConn Swap and cable discovery are pending, wait for those steps. */
            if (Cy_PdAltMode_VdmTask_InitiateVcsCblDiscovery (ptrAltModeContext))
            {
                return CY_PDALTMODE_VDM_TASK_SEND_MSG;
            }
        }

        if (pkt_type == CY_PD_SOP_DPRIME)
        {
            CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 16.1', 1, 'Intentional return used instead of breaks in switch clause'); 
            CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 16.3', 1, 'Intentional return used instead of breaks in switch clause');
            CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 16.6', 1, 'Intentional return used instead of breaks in switch clause');            
            switch (ptrAltModeContext->vdmStat->vdmEmcaRstState)
            {
                /* If SOP'' SOFT RESET has not been done or a retry is pending, try to send it. */
                case CY_PDALTMODE_CABLE_DP_RESET_IDLE:
                case CY_PDALTMODE_CABLE_DP_RESET_RETRY:
                    ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_SendSopDpSoftReset)(ptrAltModeContext);
                    return CY_PDALTMODE_VDM_TASK_SEND_MSG;

                case CY_PDALTMODE_CABLE_DP_RESET_WAIT:
                    return CY_PDALTMODE_VDM_TASK_SEND_MSG;

                case CY_PDALTMODE_CABLE_DP_RESET_DONE:
                    if (ptrAltModeContext->altModeAppStatus->cblRstDone != false)
                    {
                        /* Send SOP Prime VDM again after cable reset */
                        ptrAltModeContext->vdmStat->vdmMsg.sopType = (uint8_t)CY_PD_SOP_PRIME;
                        ptrAltModeContext->vdmStat->vdmEmcaRstState = CY_PDALTMODE_CABLE_DP_RESET_SEND_SPRIME;
                        return CY_PDALTMODE_VDM_TASK_SEND_MSG;
                    }
                    else
                    {
                        break;
                    }
                default:
                    /* EMCA SOFT_RESET done; can proceed. */
                    break;
            }
            CY_MISRA_BLOCK_END('MISRA C-2012 Rule 16.6');
            CY_MISRA_BLOCK_END('MISRA C-2012 Rule 16.3');
            CY_MISRA_BLOCK_END('MISRA C-2012 Rule 16.1'); 
        }
    }

    ptrAltModeContext->altModeAppStatus->vdmRetryPending = (ptrAltModeContext->vdmStat->recRetryCnt != CY_PDALTMODE_MNGR_MAX_RETRY_CNT);

    if(PDSTACK_CALL_MAP(Cy_PdStack_Dpm_SendPdCommand)(ptrPdStackContext, CY_PDSTACK_DPM_CMD_SEND_VDM,
           &ptrAltModeContext->altModeMngrStatus->vdmBuf, false, Cy_PdAltMode_VdmTask_RecCbk) == CY_PDSTACK_STAT_SUCCESS)
    {
        return CY_PDALTMODE_VDM_TASK_WAIT;
    }

    /* If it fails - try to send VDM again */
    return CY_PDALTMODE_VDM_TASK_SEND_MSG;
}

ATTRIBUTES_ALT_MODE bool Cy_PdAltMode_VdmTask_IsUfpDiscStarted(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    if (ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_IsIdle)(ptrAltModeContext))
    {
        /* Set VDM task to send Disc ID VDM */
        ptrAltModeContext->vdmStat->vdmTask = CY_PDALTMODE_VDM_TASK_DISC_ID;
        ptrAltModeContext->vdmStat->vdmEvt = CY_PDALTMODE_VDM_EVT_RUN;
        return true;
    }

    return false;
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_VdmTask_ResetDiscInfo (cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    ptrAltModeContext->altModeAppStatus->tbtCblVdo.val  = CY_PDALTMODE_MNGR_NO_DATA;
    ptrAltModeContext->altModeAppStatus->tbtModeVdo.val = CY_PDALTMODE_MNGR_NO_DATA;
    ptrAltModeContext->altModeAppStatus->dpCblVdo.val   = CY_PDALTMODE_MNGR_NO_DATA;
}

ATTRIBUTES_ALT_MODE void Cy_PdAltMode_VdmTask_ClearDiscResp(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    ptrAltModeContext->vdmDiscIdRespLen = 0;
    UTILS_CALL_MAP(Cy_PdUtils_MemSet) ((uint8_t *)&ptrAltModeContext->vdmDiscIdResp, 0u, CY_PD_MAX_NO_OF_DO * sizeof (uint32_t));
    ptrAltModeContext->vdmDiscSvidRespLen = 0;
    UTILS_CALL_MAP(Cy_PdUtils_MemSet) ((uint8_t *)&ptrAltModeContext->vdmDiscSvidResp, 0u, CY_PDALTMODE_MAX_DISC_SVID_RESP_LEN * sizeof (uint32_t));
}

ATTRIBUTES_ALT_MODE uint8_t *Cy_PdAltMode_VdmTask_GetStoredDiscResp(cy_stc_pdaltmode_context_t *ptrAltModeContext, bool is_disc_id, uint8_t *respLenP)
{
    uint8_t *ptr = NULL;

    /* Check for bad pointer argument */
    if (respLenP == NULL)
    {
        return NULL;
    }

    /* Set response length to zero by default */
    *respLenP = 0;

    if (ptrAltModeContext->pdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_DFP)
    {
        if (is_disc_id)
        {
            *respLenP = ptrAltModeContext->vdmDiscIdRespLen;
            ptr         = (uint8_t *)ptrAltModeContext->vdmDiscIdResp;
        }
        else
        {
            *respLenP = ptrAltModeContext->vdmDiscSvidRespLen;
            ptr         = (uint8_t *)ptrAltModeContext->vdmDiscSvidResp;
        }
    }

    return (ptr);
}

ATTRIBUTES_ALT_MODE uint8_t *Cy_PdAltMode_VdmTask_GetDiscIdResp(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t *respLenP) /* PRQA S 3408 */
{
    return Cy_PdAltMode_VdmTask_GetStoredDiscResp(ptrAltModeContext, true, respLenP);
}

ATTRIBUTES_ALT_MODE uint8_t *Cy_PdAltMode_VdmTask_GetDiscSvidResp(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t *respLenP)
{
    return Cy_PdAltMode_VdmTask_GetStoredDiscResp(ptrAltModeContext, false, respLenP);
}

#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */
/* [] END OF FILE */
