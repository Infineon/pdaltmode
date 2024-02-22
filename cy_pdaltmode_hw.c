/***************************************************************************//**
* \file cy_pdaltmode_hw.c
* \version 1.0
*
* \brief
* Hardware control for Alternate Mode implementation.
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
#include "cy_pdaltmode_hw.h"
#include "cy_pdaltmode_mngr.h"

#include "cy_pdstack_dpm.h"
#include "cy_pdutils.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_timer_id.h"
#include "cy_pdaltmode_timer_id.h"

#if DP_UFP_SUPP
#include "cy_usbpd_hpd.h"
#endif

#if RIDGE_SLAVE_ENABLE
#include "cy_pdaltmode_ridge_slave.h"
#include "cy_pdaltmode_intel_ridge_common.h"
#endif /* (RIDGE_SLAVE_ENABLE) */

#if BB_RETIMER_ENABLE
#include "cy_pdaltmode_bb_retimer.h"
#endif /* BB_RETIMER_ENABLE */

#if AMD_SUPP_ENABLE
#include "cy_pdaltmode_amd.h"
#endif /* AMD_SUPP_ENABLE */

#if AMD_RETIMER_ENABLE
#include "cy_pdaltmode_amd_retimer.h"
#endif /* AMD_RETIMER_ENABLE */

#if DEBUG_LOG
#include "debug.h"
#endif

#if CY_HPI_ENABLED
#include "cy_hpi.h"
#endif /* CY_HPI_ENABLED */

#if HPD_GPIO_SUPP_EN
#include "gpio_hpd.h"
#endif /* HPD_GPIO_SUPP_EN */

#define CY_PDALTMODE_HW_MAKE_MUX_STATE(polarity,cfg)    ((uint8_t)(polarity << 7U) | (cfg & 0x7FU))

#if DP_DFP_SUPP
/* DFP HPD callback */
static void Cy_PdAltMode_HW_DpSrcHpdCbk(void * context, cy_en_usbpd_hpd_events_t event);
#endif /* DP_DFP_SUPP */

#if DP_UFP_SUPP
/* UFP HPD callback */
static void Cy_PdAltMode_HW_DpSnkHpdCbk(void * context, cy_en_usbpd_hpd_events_t event);
#endif /* DP_UFP_SUPP */

/* Alt Mode HW info status structure */
static cy_stc_pdaltmode_hw_details_t hwStatus[NO_OF_TYPEC_PORTS];

cy_stc_pdaltmode_hw_details_t* Cy_PdAltMode_HW_GetStatus(uint8_t port)
{
    return &hwStatus[port];
}

/********************** Function definitions **********************/
bool Cy_PdAltMode_HW_EvalAppAltHwCmd(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t *cmdParam)
{
    bool stat = false;
#if (!ICL_ALT_MODE_HPI_DISABLED)
#if (!CCG_BACKUP_FIRMWARE)
    uint8_t             hwType, dataRole;
    cy_stc_pdaltmode_hw_evt_t     cmdInfo;
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    /* Convert received cmd bytes as info and data */
    cmdInfo.val  = CY_PDUTILS_MAKE_DWORD(cmdParam[3], cmdParam[2], cmdParam[1], cmdParam[0]);
    hwType   = (uint8_t)cmdInfo.hw_evt.hwType;
    dataRole = (uint8_t)cmdInfo.hw_evt.dataRole;

    if(dataRole == (uint8_t)ptrPdStackContext->dpmConfig.curPortType)
    {
        switch (hwType)
        {
            case (uint8_t)CY_PDALTMODE_MUX:
                stat = Cy_PdAltMode_HW_EvalMuxCmd(ptrAltModeContext, cmdInfo.hw_evt.evtData);
                break;

#if ((DP_DFP_SUPP) || (DP_UFP_SUPP))
            case (uint8_t)CY_PDALTMODE_HPD:
                stat = Cy_PdAltMode_HW_EvalHpdCmd(ptrAltModeContext, cmdInfo.hw_evt.evtData);
                break;
#endif /* (DP_DFP_SUPP) || (DP_UFP_SUPP) */

            default:
                /* Intentionally left empty */
                break;
        }
    }

#endif /* (!CCG_BACKUP_FIRMWARE) */
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */

    return stat;
}

#if (!CCG_BACKUP_FIRMWARE)
bool Cy_PdAltMode_HW_EvalMuxCmd(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint32_t cmd)
{
    if (cmd < (uint32_t)CY_PDALTMODE_MUX_CONFIG_RIDGE_CUSTOM)
    {
        return Cy_PdAltMode_HW_SetMux(ptrAltModeContext, (cy_en_pdaltmode_mux_select_t)cmd, CY_PDALTMODE_MNGR_NO_DATA, CY_PDALTMODE_MNGR_NO_DATA);
    }

    return false;
}
#endif /* (!CCG_BACKUP_FIRMWARE) */

#if ((DP_DFP_SUPP) || (DP_UFP_SUPP))
void Cy_PdAltMode_HW_SetCbk(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_pdaltmode_hw_cmd_cbk_t cbk)
{
    /* Register the callback if the port is valid */
    if (ptrAltModeContext->pdStackContext->port < ptrAltModeContext->noOfTypeCPorts)
    {
        ptrAltModeContext->hwDetails->hwCmdCbk = cbk;
    }
}

bool Cy_PdAltMode_HW_EvalHpdCmd(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint32_t cmd)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;
#if (VIRTUAL_HPD_ENABLE)
    bool virtualHpd = Cy_PdAltMode_HW_IsHostHpdVirtual(ptrAltModeContext);
#endif /* ((VIRTUAL_HPD_ENABLE) && (!ICL_ENABLE)) */

    if (cmd == CY_PDALTMODE_HPD_DISABLE_CMD)
    {
        ptrAltModeContext->hwDetails->altModeCmdPending = false;
        
#if AMD_RETIMER_ENABLE
        Cy_PdAltMode_AmdRetimer_SetHpd (ptrAltModeContext, CY_USBPD_HPD_EVENT_UNPLUG);
#endif /* AMD_RETIMER_ENABLE */

#if VIRTUAL_HPD_ENABLE
        if (virtualHpd)
        {
            Cy_PdAltMode_Ridge_HpdDeInit(ptrAltModeContext);
        }
#endif /* VIRTUAL_HPD_ENABLE */
        {
#if !HPD_GPIO_SUPP_EN
            (void)Cy_USBPD_Hpd_Deinit(ptrPdStackContext->ptrUsbPdContext);
#else
            Cy_USBPD_GPIO_Hpd_Deinit (ptrAltModeContext);
#endif /* !HPD_GPIO_SUPP_EN */
        }

        return true;
    }

#if DP_DFP_SUPP
    if(ptrPdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP)
    {
        if (cmd == CY_PDALTMODE_HPD_ENABLE_CMD)
        {
            ptrAltModeContext->hwDetails->altModeCmdPending = false;
            
#if AMD_RETIMER_ENABLE
            Cy_PdAltMode_AmdRetimer_SetHpd (ptrAltModeContext, CY_USBPD_HPD_EVENT_NONE);
#endif /* AMD_RETIMER_ENABLE */

#if VIRTUAL_HPD_ENABLE
            if (virtualHpd)
            {
                (void)Cy_PdAltMode_Ridge_HpdInit(ptrAltModeContext, Cy_PdAltMode_HW_DpSrcHpdCbk);
            }
            else
#endif /* VIRTUAL_HPD_ENABLE */
            {
#if !HPD_GPIO_SUPP_EN
                (void)Cy_USBPD_Hpd_TransmitInit(ptrPdStackContext->ptrUsbPdContext , Cy_PdAltMode_HW_DpSrcHpdCbk);
#else
                Cy_USBPD_GPIO_Hpd_TransmitInit(ptrPdStackContext->ptrUsbPdContext , Cy_PdAltMode_HW_DpSrcHpdCbk);
#endif /* !HPD_GPIO_SUPP_EN */
            }

            return true;
        }
        else
        {
            if ((cmd < CY_PDALTMODE_HPD_DISABLE_CMD) && (ptrAltModeContext->hwDetails->hwCmdCbk != NULL))
            {
                ptrAltModeContext->hwDetails->altModeCmdPending = true;
                ptrAltModeContext->hwDetails->altModeHpdState = (cy_en_usbpd_hpd_events_t)cmd;
#if VIRTUAL_HPD_ENABLE
                if (virtualHpd)
                {
                    (void)Cy_PdAltMode_Ridge_HpdSendEvt(ptrAltModeContext, (cy_en_usbpd_hpd_events_t)cmd);
                }
                else
#endif /* VIRTUAL_HPD_ENABLE */
                {
#if !HPD_GPIO_SUPP_EN
                    (void)Cy_USBPD_Hpd_TransmitSendevt(ptrPdStackContext->ptrUsbPdContext, (cy_en_usbpd_hpd_events_t) cmd);
#else
                    Cy_USBPD_GPIO_Hpd_TransmitSendevt(ptrPdStackContext, (cy_en_usbpd_hpd_events_t) cmd);
#endif /* !HPD_GPIO_SUPP_EN */
                }

                return true;
            }
        }
    }
#endif /* DP_DFP_SUPP */

#if DP_UFP_SUPP
    if(ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP)
    {
        if (cmd == CY_PDALTMODE_HPD_ENABLE_CMD)
        {
#if VIRTUAL_HPD_ENABLE
            if (virtualHpd)
            {
                (void)Cy_PdAltMode_Ridge_HpdInit(ptrAltModeContext, Cy_PdAltMode_HW_DpSnkHpdCbk);
            }
            else
#endif /* VIRTUAL_HPD_ENABLE */
            {
                ptrAltModeContext->hwDetails->altModeHpdState = CY_USBPD_HPD_EVENT_UNPLUG;
                (void)Cy_USBPD_Hpd_ReceiveInit(ptrPdStackContext->ptrUsbPdContext, Cy_PdAltMode_HW_DpSnkHpdCbk);
            }

            return true;
        }

#if VIRTUAL_HPD_ENABLE
        if ((virtualHpd) && (cmd == (uint32_t)CY_USBPD_HPD_COMMAND_DONE))
        {
            (void)Cy_PdAltMode_Ridge_HpdSendEvt(ptrAltModeContext, (cy_en_usbpd_hpd_events_t)cmd);
        }
#endif /* VIRTUAL_HPD_ENABLE */
    }
#endif /* DP_UFP_SUPP */

    return false;
}

#endif /* (DP_DFP_SUPP) || (DP_UFP_SUPP) */

#if DP_UFP_SUPP
static void Cy_PdAltMode_HW_DpSnkHpdCbk(void *context, cy_en_usbpd_hpd_events_t event)
{
    cy_stc_usbpd_context_t *ptrUsbpdContext = (cy_stc_usbpd_context_t *) context;
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrUsbpdContext->pdStackContext;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    cy_stc_pdaltmode_hw_evt_t altModeHwData;

    if (((uint8_t)event > CY_PDALTMODE_HPD_ENABLE_CMD) && ((uint8_t)event < CY_PDALTMODE_HPD_DISABLE_CMD))
    {
        altModeHwData.hw_evt.dataRole = (uint32_t)CY_PD_PRT_TYPE_UFP;
        altModeHwData.hw_evt.hwType   = (uint32_t)CY_PDALTMODE_HPD;

        /* Save first 4 bytes of event */
        altModeHwData.hw_evt.evtData = (uint32_t)event;
        ptrAltModeContext->hwDetails->hwSlnData = (uint32_t)altModeHwData.val;

        /* Store current HPD event */
        ptrAltModeContext->hwDetails->altModeHpdState = (cy_en_usbpd_hpd_events_t)event;

        /* Call the event callback, if it exists */
        if (ptrAltModeContext->hwDetails->hwCmdCbk != NULL)
        {
            ptrAltModeContext->hwDetails->hwCmdCbk(ptrAltModeContext, altModeHwData.val);
        }
    }
    if(event == CY_USBPD_HPD_INPUT_CHANGE)
    {
        /*
         * Start HPD ACTIVITY TIMER to prevent re-entry into deep sleep.
         * HPD RX hardware block can't detect HPD events if device enters
         * deepsleep while HPD state is changing (or if HPD is HIGH).
         * If HPD is not connected, timer period shall be 100 ms
         * (this is the default minimum HPD Connect debounce time).
         * Otherwise, timer period is 5 ms (this is large enough
         * to capture HPD LOW and IRQ events.
         */
        if (Cy_PdAltMode_HW_DpSnkGetHpdState(ptrAltModeContext) == false)
        {
            /* Start a timer of 100 ms to prevent deep sleep entry */
            (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_StartWocb)(ptrPdStackContext->ptrTimerContext,
                    CY_PDSTACK_GET_PD_TIMER_ID(ptrPdStackContext, (uint16_t)CY_PDSTACK_HPD_RX_ACTIVITY_TIMER_ID), CY_PD_HPD_RX_ACTIVITY_TIMER_PERIOD_MAX);
        }
        else
        {
            (void)TIMER_CALL_MAP(Cy_PdUtils_SwTimer_StartWocb)(ptrPdStackContext->ptrTimerContext,
                    CY_PDSTACK_GET_PD_TIMER_ID(ptrPdStackContext, (uint16_t)CY_PDSTACK_HPD_RX_ACTIVITY_TIMER_ID), CY_PD_HPD_RX_ACTIVITY_TIMER_PERIOD_MIN);
        }
    }
}

bool Cy_PdAltMode_HW_DpSnkGetHpdState(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    /*
     * Return HPD state based on last HPD event from HAL.
     * If last event was UNPLUG, HPD is not connected. If it was
     * PLUG or IRQ, HPD is connected.
     */
    if (
           (ptrAltModeContext->hwDetails->altModeHpdState == CY_USBPD_HPD_EVENT_UNPLUG) ||
           (ptrAltModeContext->hwDetails->altModeHpdState == CY_USBPD_HPD_EVENT_NONE)
       )
    {
        return false;
    }
    else
    {
        return true;
    }
}
#endif /* DP_UFP_SUPP */

#if DP_DFP_SUPP
static void Cy_PdAltMode_HW_DpSrcHpdCbk(void *context, cy_en_usbpd_hpd_events_t event)
{
    cy_stc_usbpd_context_t *ptrUsbpdContext = (cy_stc_usbpd_context_t *) context;
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrUsbpdContext->pdStackContext;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    if ((cy_en_usbpd_hpd_events_t)event == CY_USBPD_HPD_COMMAND_DONE)
    {
#if (!ICL_ALT_MODE_HPI_DISABLED)            
        cy_stc_pdaltmode_hw_evt_t altModeHwData;
    
#if AMD_RETIMER_ENABLE
        /* Process HPD signal to Retimer */
        if (
               (ptrAltModeContext->hwDetails->altModeHpdState == CY_USBPD_HPD_EVENT_PLUG) ||
               (ptrAltModeContext->hwDetails->altModeHpdState == CY_USBPD_HPD_EVENT_UNPLUG)
           )
        {
            Cy_PdAltMode_AmdRetimer_SetHpd (ptrAltModeContext, ptrAltModeContext->hwDetails->altModeHpdState);
        }
#endif /* AMD_RETIMER_ENABLE */

        /* Alt Mode command completed */
        ptrAltModeContext->hwDetails->altModeCmdPending = false;

        /* Set data role and HW type */
        altModeHwData.hw_evt.dataRole = (uint32_t)CY_PD_PRT_TYPE_DFP;
        altModeHwData.hw_evt.hwType   = (uint32_t)CY_PDALTMODE_HPD;

        /* If HPD command done */
        altModeHwData.hw_evt.evtData = (uint32_t)event;
        ptrAltModeContext->hwDetails->hwSlnData = (uint32_t)altModeHwData.val;

        /* Call the event callback, if it exists */
        if (ptrAltModeContext->hwDetails->hwCmdCbk != NULL)
        {
            ptrAltModeContext->hwDetails->hwCmdCbk (ptrAltModeContext, altModeHwData.val);
        }

#else
        /* Alt Mode command completed */
        ptrAltModeContext->hwDetails->altModeCmdPending = false;
        /* Call the event callback, if it exists */
        if (ptrAltModeContext->hwDetails->hwCmdCbk != NULL)
        {
            ptrAltModeContext->hwDetails->hwCmdCbk (ptrAltModeContext, altModeHwData.val);
        }
#endif /* (!ICL_ALT_MODE_HPI_DISABLED) */
    }
}
#endif /* DP_DFP_SUPP */

void Cy_PdAltMode_HW_DeInit(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_en_pdaltmode_mux_select_t cfg = CY_PDALTMODE_MUX_CONFIG_SAFE;
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    /* If we still have a device connected, switch MUX to USB mode */
    if(ptrPdStackContext->dpmConfig.attach)
    {
        if(ptrPdStackContext->dpmStat.faultActive == false)
        {
            cfg = CY_PDALTMODE_MUX_CONFIG_SS_ONLY;
        }
        else
        {
#if ICL_ENABLE
            if (ptrAltModeContext->iclCfg->platform_selection != 0)
            {
                /* In fault condition, switch MUX to isolate state */
                cfg = CY_PDALTMODE_MUX_CONFIG_ISOLATE;
            }
#endif /* ICL_ENABLE */
        }
    }

    /* Enable the desired connection for the high speed data lines */
    (void)Cy_PdAltMode_HW_SetMux(ptrAltModeContext, cfg, CY_PDALTMODE_MNGR_NO_DATA, CY_PDALTMODE_MNGR_NO_DATA);
#if ((DP_DFP_SUPP) || (DP_UFP_SUPP))
    
#if VIRTUAL_HPD_ENABLE
    if (Cy_PdAltMode_HW_IsHostHpdVirtual (ptrAltModeContext))
    {
        Cy_PdAltMode_Ridge_HpdDeInit(ptrAltModeContext);
    }
    else
#endif /* VIRTUAL_HPD_ENABLE */
    {
#if !HPD_GPIO_SUPP_EN
        (void)Cy_USBPD_Hpd_Deinit(ptrPdStackContext->ptrUsbPdContext);
#else
        Cy_USBPD_GPIO_Hpd_Deinit (ptrAltModeContext);
#endif /* !HPD_GPIO_SUPP_EN */
    }

    /* Clear state variables */
    ptrAltModeContext->hwDetails->altModeCmdPending = false;
    ptrAltModeContext->hwDetails->altModeHpdState   = CY_USBPD_HPD_EVENT_NONE;
    ptrAltModeContext->hwDetails->hwCmdCbk           = NULL;
#endif /* (DP_DFP_SUPP) || (DP_UFP_SUPP) */
}

cy_en_pdaltmode_mux_select_t Cy_PdAltMode_HW_GetMuxState(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    if (ptrAltModeContext->pdStackContext->port < ptrAltModeContext->noOfTypeCPorts)
    {
        return (ptrAltModeContext->hwDetails->appMuxState);
    }
    return (CY_PDALTMODE_MUX_CONFIG_ISOLATE);
}

#if MUX_DELAY_EN && !GATKEX_CREEK
void Cy_PdAltMode_HW_MuxDelayCbk (cy_timer_id_t id, void * ptrContext)
{
    (void)id;
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *) ptrContext;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

#if MUX_POLL_EN
    /* Stop Delay timer: Possibility where this function is called manually */
    TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_MUX_DELAY_TIMER));
#endif /* MUX_POLL_EN */

    /* Clear the MUX busy flag */
    ptrAltModeContext->altModeAppStatus->isMuxBusy = false;

    /* If VDM response has been delayed, send it */
    if (ptrAltModeContext->appStatusContext->isVdmPending != false)
    {
        ptrAltModeContext->appStatusContext->vdmRespCbk(ptrPdStackContext, &ptrAltModeContext->appStatusContext->vdmResp);
        ptrAltModeContext->appStatusContext->isVdmPending = false;
    }
    ptrAltModeContext->altModeAppStatus->isMuxBusy = false;

#if MUX_POLL_EN
    TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_MUX_POLL_TIMER));
#if (CY_HPI_ENABLED) && (CY_HPI_PD_ENABLE)
    /* Send MUX Error notification if MUX failed */
    if (
            (ptrAltModeContext->appStatusContext->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType == CY_PDSTACK_CMD_TYPE_RESP_NAK) ||
            (ptrAltModeContext->appStatusContext->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType == CY_PDSTACK_CMD_TYPE_RESP_BUSY)
       )
    {
        /* Notify the EC if there is a MUX access error */
        HPI_CALL_MAP(Cy_Hpi_SendHwErrorEvent)(ptrPdStackContext->ptrHpiContext, ptrPdStackContext->port, CY_HPI_SYS_HW_ERROR_MUX_ACCESS);
    }
#endif /* (CY_HPI_ENABLED) && (CY_HPI_PD_ENABLE) */
#endif /* MUX_POLL_EN */
}

#if MUX_POLL_EN
void Cy_PdAltMode_HW_MuxPollCbk (cy_timer_id_t id, void * ptrContext)
{
    (void)id;

    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *) ptrContext;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    mux_poll_status_t muxStat;

    if (ptrAltModeContext->altModeAppStatus->muxPollCbk != NULL)
    {
        /* Run and analyse MUX polling function */
        muxStat = ptrAltModeContext->altModeAppStatus->muxPollCbk(ptrAltModeContext);
        switch(muxStat)
        {
            case APP_MUX_STATE_FAIL:
                /* Save status and goto MUX handler */
                app_stat->appStatusContext->respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = CY_PDSTACK_CMD_TYPE_RESP_NAK;
                Cy_PdAltMode_HW_MuxDelayCbk(CY_PDALTMODE_MUX_DELAY_TIMER, ptrPdStackContext);
                break;
            case APP_MUX_STATE_SUCCESS:
                /* Save status and goto MUX handler */
                app_stat->appStatusContext->respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = CY_PDSTACK_CMD_TYPE_RESP_ACK;
                Cy_PdAltMode_HW_MuxDelayCbk(CY_PDALTMODE_MUX_DELAY_TIMER, ptrPdStackContext);
                break;
            default:
                /* Run polling again */
                app_stat->appStatusContext->respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = CY_PDSTACK_CMD_TYPE_RESP_BUSY;
                TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                        CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_MUX_POLL_TIMER),
                            APP_MUX_POLL_TIMER_PERIOD, Cy_PdAltMode_HW_MuxPollCbk);
        }
    }
}
#endif /* MUX_POLL_EN */
#endif /* MUX_DELAY_EN */

bool Cy_PdAltMode_HW_SetMux(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_mux_select_t muxCfg, uint32_t modeVdo, uint32_t customData)
{
    (void)customData;

    bool retval = true;

    if (ptrAltModeContext->altModeAppStatus->skipMuxConfig != false)
    {
        return retval;
    }

    if (muxCfg == CY_PDALTMODE_MUX_CONFIG_DEINIT)
    {
#if MUX_DELAY_EN
#if !GATKEX_CREEK
        /* Stop Delay timer */
        TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop)(ptrAltModeContext->pdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrAltModeContext->pdStackContext, CY_PDALTMODE_MUX_DELAY_TIMER));
#endif

        /* Clear the MUX busy flag */
        ptrAltModeContext->altModeAppStatus->isMuxBusy = false;
        ptrAltModeContext->hwDetails->appMuxUpdateReq = false;
#endif /* MUX_DELAY_EN */

        ptrAltModeContext->hwDetails->appMuxState = muxCfg;
        return retval;
    }
    
#if ((!VIRTUAL_HPD_ENABLE) && (DP_DFP_SUPP))
    if (Cy_PdAltMode_HW_IsHostHpdVirtual(ptrAltModeContext) == false)
    {
        if (
               (ptrAltModeContext->pdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP) &&
                (ptrAltModeContext->altModeAppStatus->altModeEntered != false) &&
               (ptrAltModeContext->hwDetails->altModeCmdPending == true)
            )
        {
            ptrAltModeContext->hwDetails->appMuxSavedCustomData = customData;
            ptrAltModeContext->hwDetails->appMuxSavedState       = muxCfg;
            ptrAltModeContext->hwDetails->appMuxSavedModeVdo    = modeVdo;
            ptrAltModeContext->hwDetails->appMuxUpdateReq        = true;
            return retval;
        }
        if (ptrAltModeContext->altModeAppStatus->isMuxBusy == false)
        {
            ptrAltModeContext->hwDetails->appMuxUpdateReq = false;
        }
    }
    
#endif /* (!VIRTUAL_HPD_ENABLE) && (DP_DFP_SUPP) */

#if MUX_DELAY_EN && !GATKEX_CREEK
    if (ptrAltModeContext->altModeAppStatus->isMuxBusy == false)
    {
        /* Run MUX delay timer */
        ptrAltModeContext->altModeAppStatus->isMuxBusy = true;
#if ICL_SLAVE_ENABLE
        TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrAltModeContext->pdStackContext->ptrTimerContext, ptrAltModeContext->pdStackContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrAltModeContext->pdStackContext, CY_PDALTMODE_MUX_DELAY_TIMER),
                ptrAltModeContext->iclCfg->soc_mux_config_delay, Cy_PdAltMode_HW_MuxDelayCbk);
#else
        TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrAltModeContext->pdStackContext->ptrTimerContext, ptrAltModeContext->pdStackContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrAltModeContext->pdStackContext, CY_PDALTMODE_MUX_DELAY_TIMER),
                CY_PDALTMODE_APP_MUX_VDM_DELAY_TIMER_PERIOD, Cy_PdAltMode_HW_MuxDelayCbk);
#endif /* ICL_SLAVE_ENABLE */
        ptrAltModeContext->hwDetails->appMuxUpdateReq = false;

#if MUX_POLL_EN
        /* Run MUX polling timer */
        TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start)(ptrAltModeContext->pdStackContext->ptrTimerContext, ptrAltModeContext->pdStackContext,
                CY_PDALTMODE_GET_TIMER_ID(ptrAltModeContext->pdStackContext, CY_PDALTMODE_MUX_POLL_TIMER),
                    APP_MUX_POLL_TIMER_PERIOD, Cy_PdAltMode_HW_MuxPollCbk);
#endif /* MUX_POLL_EN */
    }
    else
    {
        ptrAltModeContext->hwDetails->appMuxSavedCustomData = customData;
        ptrAltModeContext->hwDetails->appMuxSavedState       = muxCfg;
        ptrAltModeContext->hwDetails->appMuxSavedModeVdo    = modeVdo;
        ptrAltModeContext->hwDetails->appMuxUpdateReq        = true;
        return retval;
    }
#endif /* MUX_DELAY_EN */

    /* Store the current MUX configuration */
    ptrAltModeContext->hwDetails->appMuxState = muxCfg;

#if RIDGE_SLAVE_ENABLE
    /* In TBT use cases, this call is used to configure the SBU MUX.
     * This has to be configured before notifying Alpine/Titan Ridge. */
     if ((CY_PDALTMODE_MUX_CONFIG_RIDGE_CUSTOM == muxCfg) && (CY_PDALTMODE_MNGR_NO_DATA == customData))
     {
          retval = Cy_PdAltMode_HW_MuxCtrlSetCfg (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_ISOLATE);
     }
     else if (muxCfg <= CY_PDALTMODE_MUX_CONFIG_RIDGE_CUSTOM)
     {
           retval = Cy_PdAltMode_HW_MuxCtrlSetCfg (ptrAltModeContext, muxCfg);
     }
     else
     {
       /* No action required */
     }

    /* Update the Alpine/Titan Ridge status register. Do this even if the above call failed.
       This function is not expected to fail as it is an internal operation. */
     (void)Cy_PdAltMode_Ridge_SetMux(ptrAltModeContext, muxCfg, modeVdo, customData);

#elif AMD_SUPP_ENABLE
    /* In AMD use cases, this call is used to configure the SBU MUX.
     * This has to be configured before notifying the SoC. */
    if (muxCfg <= CY_PDALTMODE_MUX_CONFIG_RIDGE_CUSTOM)
    {
        retval = Cy_PdAltMode_HW_MuxCtrlSetCfg (ptrAltModeContext, muxCfg);
    }
    /* Update AMD SoC */
    Cy_PdAltMode_Amd_SetMux(ptrAltModeContext, muxCfg, modeVdo, customData);

#else
    if (muxCfg <= CY_PDALTMODE_MUX_CONFIG_RIDGE_CUSTOM)
    {
        retval = Cy_PdAltMode_HW_MuxCtrlSetCfg (ptrAltModeContext, muxCfg);
        /* Call custom MUX configuration function */
        retval = Cy_PdAltMode_HW_CustomMuxSet (ptrAltModeContext, muxCfg, modeVdo, customData);
#if (CY_HPI_ENABLED) && (CY_HPI_PD_ENABLE)
        if (!retval)
        {
            /* Notify the EC if there is a MUX access error. */
            HPI_CALL_MAP(Cy_Hpi_SendHwErrorEvent)(ptrAltModeContext->pdStackContext->ptrHpiContext,
                    ptrAltModeContext->pdStackContext->port, CY_HPI_SYS_HW_ERROR_MUX_ACCESS);
        }
#endif
    }
    else
    {
        retval = false;
    }
#endif /* RIDGE_SLAVE_ENABLE */

    return retval;
}

ATTRIBUTES_ALT_MODE bool Cy_PdAltMode_HW_IsIdle(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    return (!ptrAltModeContext->hwDetails->altModeCmdPending);
}

void Cy_PdAltMode_HW_Sleep(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
#if (!ICL_ENABLE)
#if VIRTUAL_HPD_ENABLE
    if (!Cy_PdAltMode_HW_IsHostHpdVirtual(ptrAltModeContext))
#endif /* VIRTUAL_HPD_ENABLE */
    {
#if (!((defined CCG5) || defined(CCG5C) || defined(CCG6) || defined(CCG6DF) || defined(CCG6SF)))
#if DP_DFP_SUPP
        uint8_t port = ptrAltModeContext->pdStackContext->port;
        /* Presence of a callback can be used as indication that the HPD block is active */
        if (ptrAltModeContext->hwDetails->hwCmdCbk != NULL)
        {
            /* Set the value of the HPD GPIO based on the last event */
            if (port == 0U)
            {
                Cy_GPIO_Write(Cy_GPIO_PortToAddr(HPD_P0_PORT_PIN >> 4), (HPD_P0_PORT_PIN & 0x0Fu),
                        (uint32_t)(ptrAltModeContext->hwDetails->altModeHpdState > CY_USBPD_HPD_EVENT_UNPLUG));
            }
            else
            {
                Cy_GPIO_Write(Cy_GPIO_PortToAddr(HPD_P1_PORT_PIN >> 4), (HPD_P1_PORT_PIN & 0x0Fu),
                        (uint32_t)(ptrAltModeContext->hwDetails->altModeHpdState > CY_USBPD_HPD_EVENT_UNPLUG));
            }
            /* Move the HPD pin from HPD IO mode to GPIO mode */
            Cy_USBPD_Hpd_SleepEntry (ptrAltModeContext->pdStackContext->ptrUsbPdContext);
        }
#endif /* DP_DFP_SUPP */
#endif /* (!((defined CCG5) || defined(CCG5C) || defined(CCG6) || defined(CCG6DF) || defined(CCG6SF))) */

#if (DP_UFP_SUPP) && (PMG1_HPD_RX_ENABLE)
        /* Prepare to enter deep sleep */
        Cy_USBPD_Hpd_RxSleepEntry (ptrAltModeContext->pdStackContext->ptrUsbPdContext,
                Cy_PdAltMode_HW_DpSnkGetHpdState(ptrAltModeContext));
#endif /* DP_UFP_SUPP && PMG1_HPD_RX_ENABLE */
    }
#else
    CY_UNUSED_PARAMETER(ptrAltModeContext);
#endif /* (!ICL_ENABLE) */
}

void Cy_PdAltMode_HW_Wakeup(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
#if (!ICL_ENABLE)
#if VIRTUAL_HPD_ENABLE
        if (!Cy_PdAltMode_HW_IsHostHpdVirtual(ptrAltModeContext))
#endif /* VIRTUAL_HPD_ENABLE */
        {
#if (!((defined CCG5) || defined(CCG5C) || defined(CCG6) || defined(CCG6DF) || defined(CCG6SF)))
#if DP_DFP_SUPP
        /* Presence of a callback can be used as indication that the HPD block is active */
        if (ptrAltModeContext->hwDetails->hwCmdCbk != NULL)
        {
            /* Move the HPD pin back to HPD IO mode */
            Cy_USBPD_Hpd_Wakeup (ptrAltModeContext->pdStackContext->ptrUsbPdContext,
                    (ptrAltModeContext->hwDetails->altModeHpdState > CY_USBPD_HPD_EVENT_UNPLUG));
        }
#endif /* DP_DFP_SUPP */
#endif /* (!((defined CCG5) || defined(CCG5C) || defined(CCG6) || defined(CCG6DF) || defined(CCG6SF))) */

#if (DP_UFP_SUPP) && (PMG1_HPD_RX_ENABLE)
        /* Wakeup and revert HPD RX configurations. */
        Cy_USBPD_Hpd_RxWakeup (ptrAltModeContext->pdStackContext->ptrUsbPdContext);
#endif /* DP_UFP_SUPP  && PMG1_HPD_RX_ENABLE */
    }
#else
    (void) ptrAltModeContext;
#endif /* (!ICL_ENABLE) */
}

bool Cy_PdAltMode_HW_MuxCtrlInit (cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    /* Leave the MUX in isolate mode at start */
    return (Cy_PdAltMode_HW_MuxCtrlSetCfg (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_ISOLATE));
}

/* Function used to configure the SBU MUX based on the active Alt Modes */
bool Cy_PdAltMode_HW_MuxCtrlSetCfg (cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_mux_select_t cfg)
{
    cy_en_usbpd_dpdm_mux_cfg_t dpdmConf;

    cy_en_usbpd_sbu_switch_state_t sbu1Conf = CY_USBPD_SBU_NOT_CONNECTED;
    cy_en_usbpd_sbu_switch_state_t sbu2Conf = CY_USBPD_SBU_NOT_CONNECTED;
    cy_stc_pdstack_context_t *ptrPdStackContext = ptrAltModeContext->pdStackContext;

    /* We need to configure the SBU and DP/DM MUX blocks */
    if ((ptrPdStackContext->port < ptrAltModeContext->noOfTypeCPorts) &&
            (CY_PDALTMODE_HW_MAKE_MUX_STATE((ptrPdStackContext->dpmConfig.polarity), ((uint8_t)cfg)) != ptrAltModeContext->hwDetails->muxCurState))
    {

        /* Enable USB 2.0 connections through the appropriate signals */
        if (ptrPdStackContext->dpmConfig.polarity != 0U)
        {
            dpdmConf = CY_USBPD_DPDM_MUX_CONN_USB_BOT;
        }
        else
        {
            dpdmConf = CY_USBPD_DPDM_MUX_CONN_USB_TOP;
        }

#if (GATKEX_CREEK != 1)
#if ((DP_DFP_SUPP) || (DP_UFP_SUPP) || (TBT_DFP_SUPP) || (TBT_UFP_SUPP))
        uint8_t appSbuConf =  ptrAltModeContext->tbtCfg->sbu_conf;
#endif /* ((DP_DFP_SUPP) || (DP_UFP_SUPP) || (TBT_DFP_SUPP) || (TBT_UFP_SUPP)) */

        switch(cfg)
        {
            case CY_PDALTMODE_MUX_CONFIG_SAFE:
            case CY_PDALTMODE_MUX_CONFIG_SS_ONLY:
                /* Ensure that DP/DM connection is still enabled */
                break;

#if ((DP_DFP_SUPP) || (DP_UFP_SUPP))
            case CY_PDALTMODE_MUX_CONFIG_DP_4_LANE:
            case CY_PDALTMODE_MUX_CONFIG_DP_2_LANE:
                if ((appSbuConf == CY_PD_HOST_SBU_CFG_FULL) && (ptrPdStackContext->dpmConfig.polarity != 0U))
                {
                    /* Flipped connection: SBU1 <-> AUXN, SBU2 <-> AUXP */
                    sbu1Conf = CY_USBPD_SBU_CONNECT_AUX2;
                    sbu2Conf = CY_USBPD_SBU_CONNECT_AUX1;
                }
                else
                {
                    /* Straight connection: SBU1 <-> AUXP, SBU2 <-> AUXN */
                    sbu1Conf = CY_USBPD_SBU_CONNECT_AUX1;
                    sbu2Conf = CY_USBPD_SBU_CONNECT_AUX2;
                }
                break;
#endif /* ((DP_DFP_SUPP) || (DP_UFP_SUPP)) */

#if ((TBT_DFP_SUPP) || (TBT_UFP_SUPP))
            case CY_PDALTMODE_MUX_CONFIG_RIDGE_CUSTOM:
            case CY_PDALTMODE_MUX_CONFIG_TBT_CUSTOM:
            case CY_PDALTMODE_MUX_CONFIG_USB4_CUSTOM:
#if DELAYED_TBT_LSXX_CONNECT
                sbu1Conf = SBU_NOT_CONNECTED;
                sbu2Conf = SBU_NOT_CONNECTED;
#else /* DELAYED_TBT_LSXX_CONNECT */
                if (appSbuConf == CY_PD_HOST_SBU_CFG_PASS_THROUGH)
                {
                    /* Always connect SBUx to AUX side */
                    sbu1Conf = CY_USBPD_SBU_CONNECT_AUX1;
                    sbu2Conf = CY_USBPD_SBU_CONNECT_AUX2;
                }
                else
                {
                    if ((appSbuConf == CY_PD_HOST_SBU_CFG_FULL) && (ptrPdStackContext->dpmConfig.polarity != 0U))
                    {
                        /* Flipped connection: SBU1 <-> LSRX, SBU2 <-> LSTX */
                        sbu1Conf = CY_USBPD_SBU_CONNECT_LSRX;
                        sbu2Conf = CY_USBPD_SBU_CONNECT_LSTX;
                    }
                    else
                    {
                        /* Straight connection: SBU1 <-> LSTX, SBU2 <-> LSRX */
                        sbu1Conf = CY_USBPD_SBU_CONNECT_LSTX;
                        sbu2Conf = CY_USBPD_SBU_CONNECT_LSRX;
                    }
                }
#endif /* DELAYED_TBT_LSXX_CONNECT */
                break;
#endif /* ((TBT_DFP_SUPP) || (TBT_UFP_SUPP)) */

            default:
                /* No USB 2.0 connections required */
                dpdmConf = CY_USBPD_DPDM_MUX_CONN_NONE;
                break;
        }

#else
        sbu1Conf = CY_USBPD_SBU_CONNECT_LSTX;
        sbu2Conf = CY_USBPD_SBU_CONNECT_LSRX;
#endif

        /* Configure SBU connections as required */
        (void)Cy_USBPD_Mux_SbuSwitchConfigure(ptrPdStackContext->ptrUsbPdContext, sbu1Conf, sbu2Conf);

        /* Configure D+/D- MUX as required */
        (void)Cy_USBPD_Mux_ConfigDpDm(ptrPdStackContext->ptrUsbPdContext, dpdmConf);
    }

    /* Store the current MUX configuration and polarity */
    ptrAltModeContext->hwDetails->muxCurState = CY_PDALTMODE_HW_MAKE_MUX_STATE ((ptrPdStackContext->dpmConfig.polarity), ((uint8_t)cfg));
    return true;
}

__attribute__ ((weak)) bool Cy_PdAltMode_HW_CustomMuxSet (cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_mux_select_t muxCfg, uint32_t modeVdo, uint32_t customData)
{
    (void) ptrAltModeContext;
    (void) muxCfg;
    (void) modeVdo;
    (void) customData;

    return true;
}

bool Cy_PdAltMode_HW_IsHostHpdVirtual(cy_stc_pdaltmode_context_t * ptrAltModeContext)
{
    bool ret = false;

#if VIRTUAL_HPD_ENABLE
    if(ptrAltModeContext->tbtCfg->hpd_handling != 0u)
    {
        ret = true;
    }
#else
    (void)ptrAltModeContext;
#endif /* VIRTUAL_HPD_ENABLE */

    return ret;
}

/* [] END OF FILE */
