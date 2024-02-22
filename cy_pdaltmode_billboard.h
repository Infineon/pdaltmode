/***************************************************************************//**
* \file cy_pdaltmode_billboard.h
* \version 1.0
*
* This header file defines the data structures and function prototypes associated
* with the USB Billboard.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_PDALTMODE_BILLBOARD_H_
#define _CY_PDALTMODE_BILLBOARD_H_

/*******************************************************************************
 * Header file addition
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "cy_pdaltmode_defines.h"
#include "cy_pdaltmode_mngr.h"

/**
* \addtogroup group_pdaltmode_billboard
* \{
* Billboard provides APIs and data structures to support Billboard functionality.
*
********************************************************************************
*
* \defgroup group_pdaltmode_billboard_macros Macros
* \defgroup group_pdaltmode_billboard_enums Enumerated Types
* \defgroup group_pdaltmode_billboard_data_structures Data Structures
* \defgroup group_pdaltmode_billboard_functions Functions
*/

/*******************************************************************************
 * Macro definition
 ******************************************************************************/

/**
* \addtogroup group_pdaltmode_billboard_macros
* \{
*/

/**
 * @brief Maximum number of alternate modes supported by the Billboard module
 */
#define CY_PDALTMODE_BILLBOARD_MAX_ALT_MODES                (8u)

/**
 * @brief Initialization value for mode status register
*/
#define CY_PDALTMODE_BILLBOARD_ALT_MODE_STATUS_INIT_VAL     (0x5555u)

/**
 * @brief Alternate Mode status mask for each mode
 */
#define CY_PDALTMODE_BILLBOARD_ALT_MODE_STATUS_MASK         (0x03u)

/**
 * @brief Billboard OFF timer's maximum interval. One second is used to ensure that,
 * timer module goes through only one interrupt per request.
 */
#define CY_PDALTMODE_BILLBOARD_OFF_TIMER_MAX_INTERVAL       (1000u)

/**
 * @brief Timeout special value for not disabling the Billboard interface
 */
#define CY_PDALTMODE_BILLBOARD_OFF_TIMER_NO_DISABLE         (0xFFFFu)

/**
 * @brief Minimum timeout value in allowed in seconds = 60 s
 */
#define CY_PDALTMODE_BILLBOARD_OFF_TIMER_MIN_VALUE          (60u)

/**
 * @brief Maximum buffering for EP0 transactions inside the Billboard module

 * This definition is valid only for internal Billboard implementation
 */
#define CY_PDALTMODE_BILLBOARD_MAX_EP0_XFER_SIZE            (256u)

/**
 * @brief Additional failure information due to lack of power as per the
 * Billboard capability descriptor.
 */
#define CY_PDALTMODE_BILLBOARD_CAP_ADD_FAILURE_INFO_PWR     (0x01u)

/**
 * @brief Additional failure information due to USBPD communication failure
 * as per billboard capability descriptor.
 */
#define CY_PDALTMODE_BILLBOARD_CAP_ADD_FAILURE_INFO_PD      (0x02u)

/**
 * @brief Disconnect BB Status for BB connect event
 */
#define CY_PDALTMODE_BILLBOARD_DISCONNECT_STAT              (0xFFu)

/**
 * @brief Connect BB Status for BB connect event
 */
#define CY_PDALTMODE_BILLBOARD_CONNECT_STAT                 (0x01u)

/**
 * @brief Maximum number of Alternate Modes supported
 */
#define CY_PDALTMODE_BILLBOARD_MAX_SVID                     (2u)

/** Bit field in bb_option from configuration table which indicates
 * that the device needs to enable Vendor interface along with Billboard.
 */
#define CY_PDALTMODE_BILLBOARD_OPTION_VENDOR_ENABLE          (1 << 0)

/** Bit field in bb_option from configuration table which indicates
 * that the device needs to enable Vendor interface along with Billboard.
 */
#define CY_PDALTMODE_BILLBOARD_OPTION_BILLBOARD_ENABLE       (1 << 1)

/** Bit field in bb_option from configuration table which indicates
 * that the device needs to enable HID interface along with Billboard.
 */
#define CY_PDALTMODE_BILLBOARD_OPTION_HID_ENABLE            (0x01)

/** Number of Alternate Mode field offset in the BOS descriptor */
#define CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_NUM_ALT_MODE_OFFSET (36)

/** Offset for Alternate Mode status field offset in the BOS descriptor */
#define CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_ALT_STATUS_OFFSET  (40)

/** Offset for Alternate Mode bAdditionalFailureInfo field in the BOS descriptor */
#define CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_ADD_INFO_OFFSET    (74)

/** Offset for Alternate Mode 0 information offset in the BOS descriptor */
#define CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_MODE0_INFO_OFFSET  (76)

/** Size of each Alternate Mode information in bytes */
#define CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_MODE_INFO_SIZE     (4)

/** Offset of dwAlternateModeVdo in AUM Capability descriptor */
#define CY_PDALTMODE_BILLBOARD_USB_AUM_CAP_DSCR_ALT_MODE_VDO_OFFSET   (4)

/** \} group_pdaltmode_billboard_macros */


/*******************************************************************************
 * Enumerated data definition
 ******************************************************************************/

/**
* \addtogroup group_pdaltmode_billboard_enums
* \{
*/
/**
 * @brief USB Billboard status enumeration
 *
 * Enumeration for the Billboard status codes
 */
typedef enum
{
    CY_PDALTMODE_BILLBOARD_STAT_SUCCESS = 0,       /**< Success status */
    CY_PDALTMODE_BILLBOARD_STAT_FAILURE,           /**< Failure status */
    CY_PDALTMODE_BILLBOARD_STAT_BUSY,              /**< Status code indicating Billboard is busy */
    CY_PDALTMODE_BILLBOARD_STAT_NOT_READY,         /**< Billboard not ready */
    CY_PDALTMODE_BILLBOARD_STAT_INVALID_ARGUMENT,  /**< Invalid parameter */
    CY_PDALTMODE_BILLBOARD_STAT_NOT_SUPPORTED,     /**< Operation not supported */
} cy_en_pdaltmode_billboard_status_t;


/**
 * @brief Cause for USB Billboard enumeration
 *
 * Enumeration lists all the supported causes for Billboard enumeration
 */
typedef enum
{
    CY_PDALTMODE_BILLBOARD_CAUSE_AME_TIMEOUT,       /**< AME timeout event */
    CY_PDALTMODE_BILLBOARD_CAUSE_AME_SUCCESS,       /**< Successful alternate mode entry */
    CY_PDALTMODE_BILLBOARD_CAUSE_AME_FAILURE,       /**< Failed alternate mode entry */
    CY_PDALTMODE_BILLBOARD_CAUSE_PWR_FAILURE        /**< Failed to get sufficient power */
} cy_en_pdaltmode_billboard_cause_t;

/**
 * @brief USB Billboard Alternate Mode status
 *
 * Enumeration lists the different status values as defined by
 * the billboard class specification.
 */
typedef enum
{
    CY_PDALTMODE_BILLBOARD_ALT_MODE_STAT_ERROR,             /**< Undefined error or Alternate Mode does not exist */
    CY_PDALTMODE_BILLBOARD_ALT_MODE_STAT_NOT_ATTEMPTED,     /**< Alternate Mode entry not attempted */
    CY_PDALTMODE_BILLBOARD_ALT_MODE_STAT_UNSUCCESSFUL,      /**< Alternate Mode entry attempted but failed */
    CY_PDALTMODE_BILLBOARD_ALT_MODE_STAT_SUCCESSFUL         /**< Alternate Mode entry succeeded */
} cy_en_pdaltmode_billboard_alt_mode_status_t;

/**
 * @brief USB Billboard string descriptor indices
 *
 * Enumeration lists the different string indices used in the Billboard
 * implementation.
 */
typedef enum
{
    CY_PDALTMODE_BILLBOARD_LANG_ID_STRING_INDEX,            /**< Language ID index */
    CY_PDALTMODE_BILLBOARD_MFG_STRING_INDEX,                /**< Manufacturer string index */
    CY_PDALTMODE_BILLBOARD_PROD_STRING_INDEX,               /**< Product string index */
    CY_PDALTMODE_BILLBOARD_SERIAL_STRING_INDEX,             /**< Serial string index */
    CY_PDALTMODE_BILLBOARD_CONFIG_STRING_INDEX,             /**< Configuration string index */
    CY_PDALTMODE_BILLBOARD_BB_INF_STRING_INDEX,             /**< Billboard interface string index */
    CY_PDALTMODE_BILLBOARD_HID_INF_STRING_INDEX,            /**< HID interface string index */
    CY_PDALTMODE_BILLBOARD_URL_STRING_INDEX,                /**< Additional info URL string index */
    CY_PDALTMODE_BILLBOARD_ALT_MODE1_STRING_INDEX,          /**< Alternate Mode 1 string index */
    CY_PDALTMODE_BILLBOARD_ALT_MODE2_STRING_INDEX,          /**< Alternate Mode 2 string index */
    CY_PDALTMODE_BILLBOARD_ALT_MODE3_STRING_INDEX,          /**< Alternate Mode 3 string index */
    CY_PDALTMODE_BILLBOARD_ALT_MODE4_STRING_INDEX,          /**< Alternate Mode 4 string index */
    CY_PDALTMODE_BILLBOARD_ALT_MODE5_STRING_INDEX,          /**< Alternate Mode 5 string index */
    CY_PDALTMODE_BILLBOARD_ALT_MODE6_STRING_INDEX,          /**< Alternate Mode 6 string index */
    CY_PDALTMODE_BILLBOARD_ALT_MODE7_STRING_INDEX,          /**< Alternate Mode 7 string index */
    CY_PDALTMODE_BILLBOARD_ALT_MODE8_STRING_INDEX           /**< Alternate Mode 8 string index */
} cy_en_pdaltmode_billboard_usb_string_index_t;

/** \} group_pdaltmode_billboard_enums */

/*****************************************************************************
 * Data structure definition
 ****************************************************************************/

/**
* \addtogroup group_pdaltmode_billboard_data_structures
* \{
*/

/**
 * @brief Alternate Mode information to be used in BOS descriptor
 */
typedef struct
{
    uint16_t wSvid;                     /**< SVID for the Alternate Mode */
    uint8_t  bAltMode;                  /**< Index of the Alternate Mode */
    uint8_t  iAltModeString;            /**< String index for the Alternate Mode */
    uint32_t dwAltModeVDO;              /**< Contents of Mode VDO for the Alternate Mode */
} cy_stc_pdaltmode_billboard_am_info_t;

/**
 * @brief Complete BOS descriptor information to be used by Billboard Device
 */
typedef struct
{
    uint8_t enable;                     /**< Whether Billboard interface is to be enabled */
    uint8_t nSvid;                      /**< Number of SVIDs supported */
    uint8_t prefMode;                   /**< Preferred Alternate Mode index */
    uint8_t reserved;                   /**< Reserved field */

    cy_stc_pdaltmode_billboard_am_info_t modelist[CY_PDALTMODE_BILLBOARD_MAX_SVID]; /**< List of Alternate Mode information */
} cy_stc_pdaltmode_billboard_conn_info_t;

/** \} group_pdaltmode_billboard_data_structures */

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/

/**
* \addtogroup group_pdaltmode_billboard_functions
* \{
*/

/*******************************************************************************
* Function name: Cy_PdAltMode_Billboard_Enable
****************************************************************************//**
*
* The function queues a Billboard enumeration or re-enumeration.
*
* The API queues the Billboard enumeration as per the configuration information
* provided. Enumeration details are retrieved from the configuration table.
*
* The function can be called multiple times applications to trigger a
* re-enumeration without explicit disable call.
*
* For internal implementation of Billboard, the USB module is controlled from
* the Cy_App_Usb_BbTask() and this function only queues the request. It should be noted
* that only one pending request is honored. If more than one request is
* queued, only the latest is handled. This request is failed if the flashing
* mode is in progress.
*
* \param ptrAltModeContext
* Pointer to the Alt Mode context
*
* \param cause
* Cause for Billboard enumeration
*
* \return
* Status of the call.
* \ref cy_en_pdaltmode_billboard_status_t
*
*******************************************************************************/
cy_en_pdaltmode_billboard_status_t Cy_PdAltMode_Billboard_Enable(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_billboard_cause_t cause);


/**
 * @brief Updates the Alternate Mode status
 *
 * The function updates the Alternate Mode status information for the specified
 * Alternate Mode index. The Alternate Mode index should match the DISCOVER_SVID
 * and DISCOVER_MODES response for the device.
 *
 * @param ptrAltModeContext
 * Pointer to the Alt Mode context
 *
 * @param modeIndex
 * Index of the mode as defined by the Alternate Mode manager
 *
 * @param altStatus
 * Current Alternate Mode status
 *
 * @return Status of the call
 */
cy_en_pdaltmode_billboard_status_t Cy_PdAltMode_Billboard_AltStatusUpdate(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t modeIndex,
        cy_en_pdaltmode_billboard_alt_mode_status_t altStatus);


/*******************************************************************************
* Function name: Cy_PdAltMode_Billboard_IsPresent
****************************************************************************//**
*        
* Checks whether a Billboard interface is present
*
* The function checks the configuration information and identifies if a Billboard
* device exists.
*
* \param ptrAltModeContext
* Pointer to the Alt Mode context
*
* \return
* Returns true if Billboard is present and false if absent
*
*******************************************************************************/
bool Cy_PdAltMode_Billboard_IsPresent(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/**
 * @brief Queues a Billboard interface disable
 *
 * The API disables the Billboard Device and disconnects the terminations.
 * For internal implementation of Billboard, the USB module is controlled
 * from the Cy_App_Usb_BbTask() and this function only queues the request. It should be
 * noted that only one pending request is honored. If more than one request is
 * queued, only the latest is handled. A disable call clears any pending enable.
 *
 * @param ptrAltModeContext
 * Pointer to the Alt Mode context
 *
 * @param force
 * Whether to force a disable; false = Interface not disabled when in flashing mode,
 * true = Interface disabled regardless of the operation mode.
 *
 * @return Status of the call
 */
cy_en_pdaltmode_billboard_status_t Cy_PdAltMode_Billboard_Disable(cy_stc_pdaltmode_context_t *ptrAltModeContext, bool force);

/**
 * @brief Updates the Alternate Mode status for all modes
 *
 * The current Billboard implementation supports a maximum of eight Alternate Modes,
 * and each mode as defined in the order of BOS descriptor has two bit
 * status. Bit 1:0 indicates status of Alt Mode 0 and Bit 3:2 indicates status of
 * Alt Mode 1, and so on. This function should be only used when the Billboard status
 * needs to be reinitialized to a specific value. In individual entry/exit cases,
 * Cy_PdAltMode_Billboard_AltStatusUpdate() should be used.
 *
 * @param ptrAltModeContext Pointer to the Alt Mode context
 *
 * @param status Status data for all Alternate Modes
 *
 * @return Status of the call
 */
cy_en_pdaltmode_billboard_status_t Cy_PdAltMode_Billboard_UpdateAllStatus(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint32_t status);

/** \} group_pdaltmode_billboard_functions */

/** \} group_pdaltmode_billboard */

#endif /* _CY_PDALTMODE_BILLBOARD_H_ */

/* [] END OF FILE */
