/***************************************************************************//**
* \file cy_pdaltmode_intel_vid.h
* \version 1.0
*
* \brief
* This header file defines the data structures and function prototypes associated
* with the Thunderbolt (Intel VID) alternate mode handler.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_PDALTMODE_INTEL_VID_H_
#define _CY_PDALTMODE_INTEL_VID_H_

/*******************************************************************************
 * Header file addition
 ******************************************************************************/

#include "cy_pdaltmode_defines.h"
#include "cy_pdaltmode_mngr.h"

/**
* \addtogroup group_pdaltmode_intel
* \{
*
********************************************************************************
*
* \defgroup group_pdaltmode_intel_macros Macros
* \defgroup group_pdaltmode_intel_enums Enumerated Types
* \defgroup group_pdaltmode_intel_data_structures Data Structures
* \defgroup group_pdaltmode_intel_functions Functions
*/

/*******************************************************************************
 * MACRO definition
 ******************************************************************************/

/**
* \addtogroup group_pdaltmode_intel_macros
* \{
*/

#define CY_PDALTMODE_TBT_INTEL_VID                       (0x8087u)
/**< Intel (Thunderbolt 3) SVID */

#define CY_PDALTMODE_TBT_ALT_MODE_ID                 (1u)
/**< Unique ID assigned to Thunderbolt mode in the CCGx SDK */

#define CY_PDALTMODE_TBT_VPRO_ALT_MODE_ID                (2u)
/**< Unique ID assigned to Vpro mode in the CCGx SDK */    

//#define CY_PDALTMODE_MAX_TBT_VDO_NUMB                (1u)
/**< Maximum number of VDOs Thunderbolt-3 uses for the alt mode flow */

#define CY_PDALTMODE_TBT_VDO_IDX                     (0u)
/**< Index of VDO used for Thunderbolt */

#define CY_PDALTMODE_TBT_MODE_VPRO_AVAIL             (0x04000000u)
/**< VPro available bit in TBT Mode VDO */

#define CY_PDALTMODE_TBT_MODE_VPRO_DOCK_HOST         (0x04000000u)
/**< Vpro_Dock_and_Host bit in TBT Enter Mode VDO */

#define CY_PDALTMODE_TBT_GET_VPRO_AVAILABLE_STAT(status) ((status & CY_PDALTMODE_TBT_MODE_VPRO_AVAIL) != 0U)
/**< Macro to get VPro Available bit from Discover Mode response VDO */

#define CY_PDALTMODE_TBT_GET_LEGACY_TBT_ADAPTER(status)  ((status >> 16U) & 0x1U)
/**< Macro to get legacy adapter status from Discovery Mode response  VDO */

#define CY_PDALTMODE_TBT_EXIT(status)                ((status >> 4U) & 0x1U)
/**< Macro to get Exit status from Attention VDO */

#define CY_PDALTMODE_TBT_BB_STATUS(status)               ((status >> 3U) & 0x1U)
/**< Macro to get BB status from Attention VDO */

#define CY_PDALTMODE_TBT_USB2_ENABLE(status)             ((status >> 2U) & 0x1U)
/**< Macro to get USB enable status from Attention VDO */

#if ICL_ENABLE    

#define CY_PDALTMODE_TBT_BB_STS_USB2_ENABLE(status)      ((status >> 2u) & 0x3u)
/**< Macros to get USB enable and BB status from Attention VDO */

#define CY_PDALTMODE_TBT_BB_PRESENT_EXIT_MODE            (0x03u)   
/**< Billboard is present and mode has to be exited */
#endif /* ICL_ENABLE */    

/** \} group_pdaltmode_intel_macros */

/*****************************************************************************
 * Data structure definition
 ****************************************************************************/

/**
* \addtogroup group_pdaltmode_intel_data_structures
* \{
*/

/**
 * @struct cy_stc_pdaltmode_tbt_status_t
 * @brief TBT Status
 */
typedef struct
{
    cy_stc_pdaltmode_mngr_info_t            info;                   /**< Alt Mode Manager info */
    cy_en_pdaltmode_tbt_state_t             state;                  /**< TBT state */
    cy_pd_pd_do_t                           vdo[CY_PDALTMODE_MAX_TBT_VDO_NUMB];  /**< VDO array */
    cy_pd_pd_do_t                           enterModeVdo;         /**< Enter mode VDO */
    uint8_t                                 maxSopSupp;           /**< Max SOP supported */
    bool                                    vproSupp;              /**< VPro supported */
#if TBT_DFP_SUPP
    cy_pd_pd_do_t                           devVdo;                /**< Device VDO */
    const cy_stc_pdaltmode_atch_tgt_info_t* tgtInfoPtr;           /**< Attached target info pointer */
#endif /* TBT_DFP_SUPP */
}cy_stc_pdaltmode_tbt_status_t;

/** \} group_pdaltmode_intel_structures */

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/

/**
* \addtogroup group_pdaltmode_intel_functions
* \{
*/

/*******************************************************************************
* Function Name: Cy_PdAltMode_TBT_RegIntelModes
****************************************************************************//**
*
* Analyzes Discovery information to find out
* if further TBT Alternate Mode processing is allowed.
*
* \param context
* Pointer to the Alt Mode context
*
* \param regInfo
* Pointer to structure which holds Alt Mode register info
*
* \return
* Pointer to TBT Alternate Mode command structure if analysis passed
* is successful. In case of failure, function returns NULL pointer.
*
*******************************************************************************/
cy_stc_pdaltmode_mngr_info_t* Cy_PdAltMode_TBT_RegIntelModes(void *context, cy_stc_pdaltmode_alt_mode_reg_info_t* regInfo);

/*******************************************************************************
* Function name: Cy_PdAltMode_TBT_SendExitModeCmd
****************************************************************************//**
*
* Initiate TBT mode exit command
*
* \param ptrAltModeContext
* AltMode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_TBT_SendExitModeCmd(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/** \} group_pdaltmode_intel_functions */

/** \} group_pdaltmode_intel */

#endif /* _CY_PDALTMODE_INTEL_VID_H_ */

/* [] END OF FILE */
