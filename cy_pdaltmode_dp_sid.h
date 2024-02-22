/***************************************************************************//**
* \file cy_pdaltmode_dp_sid.h
* \version 1.0
*
* \brief
* This header file defines the data structures and function prototypes associated
* with the Display Port Alternate Mode.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_PDALTMODE_DP_SID_H_
#define _CY_PDALTMODE_DP_SID_H_

/*******************************************************************************
 * Header file addition
 ******************************************************************************/

#include "cy_pdaltmode_defines.h"
#include "cy_pdaltmode_mngr.h"

/**
* \addtogroup group_pdaltmode_display_port
* \{
* Provides APIs and data structures to support DisplayPort (Source and/or Sink) Alternate Mode.
*
********************************************************************************
*
* \defgroup group_pdaltmode_display_port_macros Macros
* \defgroup group_pdaltmode_display_port_enums Enumerated Types
* \defgroup group_pdaltmode_display_port_data_structures Data Structures
* \defgroup group_pdaltmode_display_port_functions Functions
*/

/*******************************************************************************
 * MACRO definition
 ******************************************************************************/

/**
* \addtogroup group_pdaltmode_display_port_macros
* \{
*/

/** DisplayPort SVID */
#define CY_PDALTMODE_DP_SVID                         (0xFF01u)

/** Unique ID assigned to DP Alternate Mode in the CCGx SDK */
#define CY_PDALTMODE_DP_ALT_MODE_ID                  (0u)

/** Index of VDO buffer element which is used to handle DP VDO */
#define CY_PDALTMODE_DP_VDO_IDX                      (0u)

#define CY_PDALTMODE_DP_STATUS_UPDATE_VDO               (0x01u)
/**< DP Status Update command VDO used by DFP_U */

#define CY_PDALTMODE_DP_DFP_D_CONN                      (0x01u)
/**< DP Status Update command VDO when DP is DFP (DFP_D DP data role) */

#define CY_PDALTMODE_DP_UFP_D_CONN                      (0x02u)
/**< UFP_D DP data role configuration */

/** Standard DP signaling type value in the signaling field for Config VDO */
#define CY_PDALTMODE_DP_1_3_SIGNALING                (0x0001u)

/** Value when DFP set configuration for UFP_U as UFP_D in the configuration
     select field for Config VDO. */
#define CY_PDALTMODE_DP_CONFIG_SELECT                (2u)

/** Value when DFP set configuration for UFP_U as DFP_D in the configuration
     select field of Config VDO. */
#define CY_PDALTMODE_DP_CONFIG_SELECT_DPSRC          (1u)

/** Value when DFP set configuration as USB in the configuration select field for Config VDO */
#define CY_PDALTMODE_DP_USB_CONFIG_SELECT               (0u)

/** Pin assignment mask for USB pin assignment in the Configure Pin Assignment
     field for Config VDO. */
#define CY_PDALTMODE_DP_USB_SS_CONFIG                (0u)

/** Pin assignment mask for USB pin assignment in the Configure Pin Assignment
     field for Config VDO for DP2.1 spec revision. */
#define CY_PDALTMODE_DP_USB_SS_CONFIG_MASK           (0x3FFFFFFFu)

/** Pin assignment mask for C pin assignment in the Configure Pin Assignment field for Config VDO */
#define CY_PDALTMODE_DP_DFP_D_CONFIG_C               (4u)

/** Pin assignment mask for D pin assignment in the Configure Pin Assignment field for Config VDO */
#define CY_PDALTMODE_DP_DFP_D_CONFIG_D               (8u)

/** Pin assignment mask for E pin assignment in the Configure Pin Assignment field for Config VDO */
#define CY_PDALTMODE_DP_DFP_D_CONFIG_E               (0x10u)

/** Pin assignment mask for F pin assignment in the Configure Pin Assignment field for Config VDO */
#define CY_PDALTMODE_DP_DFP_D_CONFIG_F               (0x20u)

/** Internal DP denotation for invalid pin assignment */
#define CY_PDALTMODE_DP_INVALID_CFG                  (0xFFu)

#define CY_PDALTMODE_DP_HPD_LOW_IRQ_LOW                 (0x0u)
/**< HPD low and IRQ low value obtained from UFP Attention/Status Update VDO */

#define CY_PDALTMODE_DP_HPD_HIGH_IRQ_LOW                (0x1u)
/**< HPD high and IRQ low value obtained from UFP Attention/Status Update VDO */

#define CY_PDALTMODE_DP_HPD_LOW_IRQ_HIGH                (0x2u)
/**< HPD low and IRQ high value obtained from UFP Attention/Status Update VDO */

#define CY_PDALTMODE_DP_HPD_HIGH_IRQ_HIGH               (0x3u)
/**< HPD high and IRQ high value obtained from UFP Attention/Status Update VDO. */
    
#define CY_PDALTMODE_DP_HPD_STATE_BIT_POS               (0x80u)
/**< HPD LVL bit position */

#define CY_PDALTMODE_DP_HPD_IRQ_BIT_POS                 (0x100u)
/**< HPD LVL bit position */

/** Size of one HPD queue element in bits */
#define CY_PDALTMODE_DP_QUEUE_STATE_SIZE             (2u)

/** Mask to extract the first element from the HPD queue */
#define CY_PDALTMODE_DP_HPD_STATE_MASK               (0x0003u)

/** Flag to indicate an empty HPD queue */
#define CY_PDALTMODE_DP_QUEUE_EMPTY_INDEX            (0x0u)

/** Flag to indicate a full HPD queue */
#define CY_PDALTMODE_DP_QUEUE_FULL_INDEX             (7u)

/** Maximum number of entries in the HPD queue */
#define CY_PDALTMODE_DP_UFP_MAX_QUEUE_SIZE           (4u)

/** Maximum number of IRQ entries in the HPD queue */        
#define CY_PDALTMODE_DP_MAX_IRQ_SIZE                (2u)

#define CY_PDALTMODE_DP_GET_HPD_IRQ_STAT(status)        (((status) >> 7UL)& 0x3UL )
/**< Macros to get HPD state from Attention/Status Update VDO */

/** DP Sink Control APP command value */
#define CY_PDALTMODE_DP_SINK_CTRL_CMD           (3u)

/** DP MUX Control APP command value */
#define CY_PDALTMODE_DP_MUX_CTRL_CMD            (1u)

/** DP MUX Configure APP command value */
#define CY_PDALTMODE_DP_APP_CFG_CMD             (2u)

/** DP UFP Vconn Swap command value */
#define CY_PDALTMODE_DP_APP_VCONN_SWAP_CFG_CMD  (4u)

/** Bit index of USB configuration in APP DP MUX Configure command data */
#define CY_PDALTMODE_DP_APP_CFG_USB_IDX            (6u)

/** Maximum value of USB configuration in APP DP MUX Configure command data */
#define CY_PDALTMODE_DP_APP_CFG_CMD_MAX_NUMB       (6u)

/** DP allowed MUX Configuration APP event value */
#define CY_PDALTMODE_DP_ALLOWED_MUX_CONFIG_EVT     (1u)

/** DP Status Update APP event value */
#define CY_PDALTMODE_DP_STATUS_UPDATE_EVT          (2u)

/** Acked field mask for EC DP MUX Configure command data */
#define CY_PDALTMODE_DP_CFG_CMD_ACK_MASK           (0x200)

/** Active cable parameter mask for the cable version is 1 and higher */
#define CY_PDALTMODE_DP_ACT_CBL_PARAM_MASK            (0xFC00003Cu)

/** HBR3 bit rate support mask */
#define CY_PDALTMODE_DP_HBR3_SUPP_MASK             (0x1u)

/** HBR10 bit rate support mask */
#define CY_PDALTMODE_DP_HBR10_SUPP_MASK            (0x2u)

/** HBR20 bit rate support mask */
#define CY_PDALTMODE_DP_HBR20_SUPP_MASK            (0x4u)

/** Active cable parameter mask for the cable version is 1 and higher */
#define CY_PDALTMODE_DP_ACT_CBL_PARAM_MASK        (0xFC00003Cu)

/** DP 2.1 Version index */
#define CY_PDALTMODE_DP_2_1_VERSION               (1u)

/** DP 2.1 Cable configuration mask */
#define CY_PDALTMODE_DP_2_1_CBL_CFG_MASK          (0xFF07u)

/** DP 2.1 enable configuration bit mask */
#define CY_PDALTMODE_DP2_1_CFG_MASK                (0x80u)


/** \} group_pdaltmode_display_port_macros */

/*******************************************************************************
 * Enumerated data definition
 ******************************************************************************/


/**
* \addtogroup group_pdaltmode_display_port_enums
* \{
*/

/**
 * @typedef cy_en_pdaltmode_dp_port_cap_t
 * @brief Holds possible DP capabilities
 */
typedef enum
{
    CY_PDALTMODE_DP_PORT_CAP_RSVD = 0,               /**< Reserved capability */
    CY_PDALTMODE_DP_PORT_CAP_UFP_D,                  /**< UFP is UFP_D-capable */
    CY_PDALTMODE_DP_PORT_CAP_DFP_D,                  /**< UFP is DFP_D-capable */
    CY_PDALTMODE_DP_PORT_CAP_BOTH                    /**< UFP is DFP_D and UFP-D capable */
}cy_en_pdaltmode_dp_port_cap_t;

/**
 * @typedef cy_en_pdaltmode_dp_conn_t
 * @brief Holds possible DFP_D/UFP_D Connected status (Status Update message)
 */
typedef enum
{
    CY_PDALTMODE_DP_CONN_NONE = 0,                   /**< Neither DFP_D nor UFP_D is connected */
    CY_PDALTMODE_DP_CONN_DFP_D,                      /**< DFP_D is connected */
    CY_PDALTMODE_DP_CONN_UFP_D,                      /**< UFP_D is connected */
    CY_PDALTMODE_DP_CONN_BOTH                        /**< Both DFP_D and UFP_D are connected */
}cy_en_pdaltmode_dp_conn_t;
/** \} group_pdaltmode_display_port_enums */

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/

/**
* \addtogroup group_pdaltmode_display_port_functions
* \{
*/

/*******************************************************************************
* Function Name: Cy_PdAltMode_DP_RegModes
****************************************************************************//**
*
* Analyze Discovery information to find out
* if further DP Alternate Mode processing is allowed.
*
* \param context
* Pointer to the Alt Mode context
*
* \param regInfo
* Pointer to structure which holds Alt Mode register info
*
* \return
* Pointer to DP Alternate Mode command structure if analysis passed is
* successful. In case of failure, function returns NULL pointer.
*
*******************************************************************************/
cy_stc_pdaltmode_mngr_info_t* Cy_PdAltMode_DP_RegModes(void *context, cy_stc_pdaltmode_alt_mode_reg_info_t* regInfo);

#if DP_GPIO_CONFIG_SELECT

/*******************************************************************************
* Function Name: Cy_PdAltMode_DP_SinkSetPinConfig
****************************************************************************//**
*
* Store the DP Pin configuration based on GPIO status.
*
* \param dpConfig
* DP Pin configuration
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_DP_SinkSetPinConfig(uint8_t dpConfig);


/*******************************************************************************
* Function Name: Cy_PdAltMode_DP_SinkGetPinConfig
****************************************************************************//**
*
* Return the DP Pin configuration based on GPIO status.
*
* \return
* DP Pin configuration
*
*******************************************************************************/
uint8_t Cy_PdAltMode_DP_SinkGetPinConfig(void);

#endif /* DP_GPIO_CONFIG_SELECT */

/*******************************************************************************
* Function Name: Cy_PdAltMode_DP_DfpDequeueHpd
****************************************************************************//**
*
* Dequeue next HPD status.
*
* \param ptrAltModeContext
* Pointer to the Alt Mode context
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_DP_DfpDequeueHpd(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function Name: Cy_PdAltMode_DP_GetStatus
****************************************************************************//**
*
* Return DP status pointer.
*
* \param port
* Selected port
*
* \return
* Returns pointer to DP status structure
*
*******************************************************************************/
cy_stc_pdaltmode_dp_status_t* Cy_PdAltMode_DP_GetStatus(uint8_t port);

/** \} group_pdaltmode_display_port_functions */

/** \} group_pdaltmode_display_port */

#endif /* _CY_PDALTMODE_DP_SID_H_ */

/* [] END OF FILE */
