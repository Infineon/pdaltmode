/***************************************************************************//**
* \file cy_pdaltmode_timer_id.h
* \version 1.0
*
* \brief
* Provides soft timer identifier definitions for PdAltmode.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_PDALTMODE_TIMER_ID_H_
#define _CY_PDALTMODE_TIMER_ID_H_

#include <stdint.h>
#include "cy_pdutils_sw_timer.h"
#include "cy_pdaltmode_defines.h"

#define CY_PDALTMODE_GET_TIMER_ID(context, id)                                 \
    (uint16_t)(((context)->port != 0U) ? ((((uint16_t)id) & 0x00FFU) + (uint16_t)CY_PDUTILS_TIMER_ALT_MODE_PORT1_START_ID) : (uint16_t)(id))


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
 * Enumerated data definition
 ******************************************************************************/

/**
* \addtogroup group_pdaltmode_vdm_alt_mode_manager_enums
* \{
*/
/**
 * @typedef cy_en_pdaltmode_timer_id_t
 * @brief List of soft timer IDs defined for PD Alt Mode
 */

typedef enum {

    /**< 0x500: Starts the index for USB PD Alt Mode timers */
    CY_PDALTMODE_PORT0_TIMERS_START_ID = CY_PDUTILS_TIMER_ALT_MODE_PORT0_START_ID,

    /** 0x501: Timer for initiating virtual HPD dequeue */
    CY_PDALTMODE_RIDGE_INIT_HPD_DEQUEUE_TIMER_ID,

    /** 0x502: Timer used to initiate Virtual HPD IRQ CLEAR ACK to the Thunderbolt controller */
    CY_PDALTMODE_INITIATE_SEND_IRQ_CLEAR_ACK,

    /** 0x503: Timer used to send Hard Reset upon USB3 Host connected */
    CY_PDALTMODE_SEND_HARD_RESET_UPON_USB3_HOST_CONNECTION_TIMER,

    /** 0x504: Timer used to restart DS port*/
    CY_PDALTMODE_RESTART_DS_PORT_USB3_HOST_CONNECTION_TIMER,

    /** 0x505: Timer used to delay US port SUB connection by AME timer  */
    CY_PDALTMODE_DELAY_US_PORT_USB_CONNECTION_BYAMETIMOUT_TIMER,

    /** 0x506: Timer used to update Ridge status after Hard Reset */
    CY_PDALTMODE_TIMER_UPDATE_RIDGE_STATUS_AFTER_HARD_RESET,

    /** 0x507: Goshen Ridge MUX delay timer */
    CY_PDALTMODE_GR_MUX_DELAY_TIMER,

    /** 0x508: Send Data Reset from Ridge controller timer */
    CY_PDALTMODE_SEND_DATA_RESET_FROM_RIDGE_TIMER,

    /** 0x509: Alt Mode callback timer */
    CY_PDALTMODE_ALT_MODE_CBK_TIMER,

    /** 0x50A: Alt Mode Attention callback timer */
    CY_PDALTMODE_ALT_MODE_ATT_CBK_TIMER,

    /** 0x50B: Timer to exit TBT Alt Mode after receiving exit Attention */
    CY_PDALTMODE_TBT_MODE_EXIT_CHECK_TIMER,

    /** 0x50C: SoC communication ACK timeout timer */
    CY_PDALTMODE_ICL_SOC_TIMEOUT_TIMER,

    /**< 0x50D: Timer used to delay retry of VDMs due to BUSY responses or errors */
    CY_PDALTMODE_VDM_BUSY_TIMER,

    /**< 0x50E: Timer used to implement AME timeout */
    CY_PDALTMODE_AME_TIMEOUT_TIMER,

    /**< 0x50F: Timer used to delay HPD events */
    CY_PDALTMODE_HPD_DELAY_TIMER,

    /**<0x510: Timer used to delay VDM response */
    CY_PDALTMODE_MUX_DELAY_TIMER,

    /**<0x511: Timer used for MUX status */
    CY_PDALTMODE_MUX_POLL_TIMER,

    /**<0x512: Timer to postpone Hard Reset in case MUX update is in progress*/
    CY_PDALTMODE_ICL_HARD_RST_TIMER,

    /**<0x513: Timer used for HPD DP2.1 handling */
    CY_PDALTMODE_HPD_WAIT_TIMER,

    /**<0x514: Timer used to implement USB4 timeout */
    CY_PDALTMODE_USB4_TIMEOUT_TIMER,

    /**<0x515: Timer is used for periodic read of AMD APU status */
    CY_PDALTMODE_AMD_BUSY_TIMER,

    /**<0x516: Timer is used for periodic read of AMD Retimer status */
    CY_PDALTMODE_AMD_RETIMER_BUSY_TIMER,

    /**<0x517: Timer is used to unblock Type-C state machine in case APU did not respond for some time */
    CY_PDALTMODE_AMD_INT_BUSY_TIMER,

    /**<0x518: Timer is used during AMD Retimer Hard Reset */
    CY_PDALTMODE_AMD_RETIMER_WAIT_TIMER,

    /**<0x519: Timer is used to retry reading the Debug register from the Retimer */
    CY_PDALTMODE_BB_DBR_DEBUG_POLL_TIMER,

    /**<0x51A: Timer is used to delay the Retimer to start accepting the state change requests */
    CY_PDALTMODE_BB_DBR_WAIT_TIMER,

    /**<0x51B: Timer is used to delay Retimer debug mode register write */
    CY_PDALTMODE_BB_DBR_DEBUG_WRITE_TIMER,

    /**<0x51C: Timer used for Retimer handling */
    CY_PDALTMODE_RETIMER_BUSY_TIMER,

    /**<0x51D: Timer is used during Retimer Hard reset */
    CY_PDALTMODE_RETIMER_WAIT_TIMER,

    /**<0x51E: Timer to let VSYS turn on change to stabilize */
    CY_PDALTMODE_ICL_VSYS_STABLE_TIMER,

    /**<0x51F: Timer is used to check ICL mux state changes */
    CY_PDALTMODE_ICL_MUX_STATE_CHANGE_TIMER,

    /**<0x520: Timer is used to check Retimer FW Exit state changes */
    CY_PDALTMODE_ICL_RFU_EXIT_TIMER,

    /**<0x521: Timer is used for comm delay in the RFU entry sequence */
    CY_PDALTMODE_ICL_ALT_MODE_EXIT_TIMER,

    /**<0x522: Timer is used debounce barrel state change */
    CY_PDALTMODE_ICL_ADP_DETECT_DEB_TIMER,

    /**<0x600: Starts the index for USB PD Alt Mode timers */
    CY_PDALTMODE_PORT1_TIMERS_START_ID = CY_PDUTILS_TIMER_ALT_MODE_PORT1_START_ID,

} cy_en_pdaltmode_timer_id_t;

/** \} group_pdaltmode_vdm_alt_mode_manager_enums */

/** \} group_pdaltmode_vdm_alt_mode_manager */

#endif /* _CY_PDALTMODE_TIMER_ID_H_ */

/* [] END OF FILE */
