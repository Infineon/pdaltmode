/***************************************************************************//**
* \file cy_pdaltmode_defines.h
* \version 1.0
*
* \brief
* This header file provides common data structures of the Alternate mode feature.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_PDALTMODE_DEFINES_H_
#define _CY_PDALTMODE_DEFINES_H_

#include "cy_usbpd_defines.h"
#include "cy_usbpd_config_table.h"
#include "cy_pdstack_common.h"
#include "cy_pdaltmode_rom.h"
#include "cy_scb_i2c.h"
#if CY_HPI_ENABLED
#include "cy_hpi_defines.h"
#endif /* CY_HPI_ENABLED */

#ifndef STORE_DETAILS_OF_HOST
/* Data Exchange between ports enabled */
#define STORE_DETAILS_OF_HOST                       (0u)
#endif /* STORE_DETAILS_OF_HOST */

#ifndef RIDGE_SLAVE_ENABLE
/* Enable Ridge Slave Interface*/
#define RIDGE_SLAVE_ENABLE                          (0u)
#endif /* RIDGE_SLAVE_ENABLE */


/**
********************************************************************************
* \mainpage PDAltMode Middleware Library Documentation
*
* The PDAltMode middleware implements state machines defined in:
* * **Universal Serial Bus Power Delivery Specification Rev 3.1 Ver 1.8**
* * **Universal Serial Bus Type-C Cable and Connector Specification Ver 2.2**
* * **VESA DisplayPort Alt Mode on USB Type-C Standard. Ver 1.3/1.4**
*
* The PDAltMode middleware operates on top of PdStack Middleware and USBPD driver 
* included in the MTB PDL CAT2 (mtb-pdl-cat2) Peripheral Driver Library. The middleware 
* provides a set of Alt Mode APIs through which the application can initialize, 
* monitor and configure the following PD Alt Modes:
* * Display Port.
* * TBT.
* * vPro.
* * USB4.
*
* The PDAltMode Middleware is released as a combination of source files and a 
* pre-compiled library (pdaltmode_dock_tbt). The pre-compiled library implements support 
* for TBT and vPro alternate modes. All of the remaining state machines are implemented 
* in the form of source files.
*
* <b>Features:</b>
* 1. Support PD Alternate Modes Discovery, entry and simultaneously handling up to 4
* alternate modes.
* 2. Support Intel Ridge Slave Interface.
* 3. Support the following AltModes (in DFP and UFP roles) by default:
*    * DP
*    * TBT
*    * vPro
*    * USB4
* 4. Support mechanisms to add user custom alternate mode.
********************************************************************************
* \section section_pdaltmode_quick_start Quick Start Guide
********************************************************************************
*
* PDAltMode Middleware can be used in various development environments such as
* ModusToolbox(TM), MBED, and so on. Refer to the \ref section_pdaltmode_toolchain
* section.
*
* The following steps describe the simplest way of enabling the PDAltMode
* Middleware in the application.
*
* 1. Open/Create an application where PdAltMode functionality is needed.
*
* 2. Add the PDAltMode Middleware to your project. This quick start guide
* assumes that the environment is configured to use the MTB CAT2 Peripheral
* Driver Library(PDL) for development and the PDL is included in the project.
* If you are using the ModusToolbox(TM) development environment, select the
* application in the Project Explorer window and select the PDAltMode Middleware
* in the Library Manager.
*
* 3. Include cy_pdaltmode_defines.h, cy_pdaltmode_vdm_task.h, cy_pdaltmode_mngr.h and
* cy_pdaltmode_hw.h header files to get access to common Alternate Modes functionality as
* Discovery process and Alternate Modes handling.
*    \snippet pdaltmode/main.c snippet_configuration_include_common
*
* 4. Include desired Alternate Mode header/source files to get access to selected
* Alternate Mode. For example, for DisplayPort Alternate Mode, include next header file:
*    \snippet pdaltmode/main.c snippet_configuration_include_dp
*
* 5. Define the following data structures, pointers (or) configuration required by the PDAltMode Middleware:
*    \snippet pdaltmode/main.c snippet_configuration_common
*
* 6. Enable next macros:
*    * **DFP_ALT_MODE_SUPP UFP_ALT_MODE_SUPP** to enable DFP/UFP Alternate Modes functionality
*    * **DP_DFP_SUPP DP_UFP_SUPP** to enable DisplayPort DFP/UFP Alternate Modes functionality
*    * **CY_PD_USB4_SUPPORT_ENABLE** to enable USB4 functionality
*    * **STORE_DETAILS_OF_HOST** to enable Dock exchange capabilities functionality
*
* 7. Configure supported Alternate Modes using next sections in 
* <a href=" https://www.infineon.com/cms/en/design-support/tools/configuration/usb-ez-pd-configuration-utility/">
*      <b>EZ-PD Configuration Utility:</b>
*   </a>
*    * **Base Alternate Modes** - Configures supported Alternate Modes SVIDs, data role supported 
*        for selected SVID, and modes entry priority.
*    * **DP Mode Parameters** - DisplayPort related configurations.
*
* 8. Register desired Alternate Mode callback function (for ex. Display port):
*    \snippet pdaltmode/main.c snippet_register_dp
*
* The PDAltMode library uses these set of configuration for running internal
* state machine that implement different PD Alternate Modes.
*
* 9. Initialize the PDAltMode Middleware once at the start.
*    \snippet pdaltmode/main.c snippet_register_am_layer
*
* 10. Start the VDM and Alt Mode Managers operations and start state machines
*    that control external mux.
*    \snippet pdaltmode/main.c snippet_am_task_enable
*
* 11. Invoke Cy_PdAltMode_Manager_Task and Cy_PdAltMode_VdmTask_Manager functions from the main 
* processing loop of the application to handle the Vdm and Alt Mode Managers tasks for each PD port.
*    \snippet pdaltmode/main.c snippet_am_task
*
********************************************************************************
* \section section_pdaltmode_toolchain Supported Software and Tools
********************************************************************************
*
* This version of the PDAltMode Middleware was validated for the compatibility
* with the following software and tools:
*
* <table class="doxtable">
*   <tr>
*     <th>Software and Tools</th>
*     <th>Version</th>
*   </tr>
*   <tr>
*     <td>ModusToolbox(TM) software environment</td>
*     <td>3.1</td>
*   </tr>
*   <tr>
*     <td>mtb-pdl-cat2</td>
*     <td>2.4.0</td>
*   </tr>
*   <tr>
*     <td>pdstack</td>
*     <td>3.1.0</td>
*   </tr>
*   <tr>
*     <td>GCC Compiler</td>
*     <td>10.3.1</td>
*   </tr>
*   <tr>
*     <td>IAR Compiler</td>
*     <td>8.42.2</td>
*   </tr>
*   <tr>
*     <td>Arm(R) Compiler 6</td>
*     <td>6.13</td>
*   </tr>
* </table>
*
********************************************************************************
* \section section_pdaltmode_changelog Changelog
********************************************************************************
*
* <table class="doxtable">
*   <tr><th>Version</th><th>Changes</th><th>Reason for change</th></tr>
*   <tr>
*     <td>1.0</td>
*     <td>Initial version</td>
*     <td></td>
*   </tr>
* </table>
*
********************************************************************************
* \section section_pdaltmode_more_information More Information
********************************************************************************
*
* For more information, see the following resources:
*
* * <a href=" https://www.infineon.com/modustoolbox">
*      <b>ModusToolbox(TM) software, quick start guide, documentation,
*         and videos</b>
*   </a>
*
* * <a href="https://infineon.github.io/mtb-pdl-cat2/pdl_api_reference_manual/html/index.html">
*   <b>PDL API Reference Manual</b></a>
*   </a>
*
* * <a href="https://infineon.github.io/pdstack/pdstack_api_reference_manual/html/index.html">
*   <b>PDStack API Reference Manual</b></a>
*
* \note
* The links to the other software component's documentation (middleware and PDL)
* point to GitHub for the latest available version of the software.
* To get documentation of the specified version, download from GitHub, and unzip
* the component archive. The documentation is available in
* the <i>docs</i> folder.
**/

/* The list below defines the order of the module groups in the API Reference
*  Manual. */
/**
*
* \{
* \defgroup group_pdaltmode_common                Common
* \defgroup group_pdaltmode_intel                 Intel
* \defgroup group_pdaltmode_billboard             Billboard
* \defgroup group_pdaltmode_display_port          Display Port
* \defgroup group_pdaltmode_alt_mode_hw           Alternate Mode Hardware
* \defgroup group_pdaltmode_vdm_alt_mode_manager  Alternate Modes Manager
* \defgroup group_pdaltmode_usb4                  USB4
* \}
*
*/

/** @cond DOXYGEN_HIDE */
/**
*
* \{
* \defgroup group_pdaltmode_custom_hpi_am         HPI Alternate mode
* \defgroup group_pdaltmode_amd                   AMD
* \defgroup group_pdaltmode_retimer_wrapper       Retimer Wrapper
* \}
*
*/
/** @endcond */

/*******************************************************************************
*                              Type definitions
*******************************************************************************/


/* Provides default values for feature selection macros where not already defined. */

#ifndef CCG_PD_REV3_ENABLE
#define CCG_PD_REV3_ENABLE                                (0u)
#endif /* CCG_PD_REV3_ENABLE */

#ifndef DEBUG_ACCESSORY_SNK_ENABLE
#define DEBUG_ACCESSORY_SNK_ENABLE                        (0u)
#endif /* DEBUG_ACCESSORY_SNK_ENABLE */

#ifndef TITAN_RIDGE_ENABLE
#define TITAN_RIDGE_ENABLE                               (0u)
#endif /* TITAN_RIDGE_ENABLE */

#ifndef ICL_OCP_ENABLE
#define ICL_OCP_ENABLE                                   (0u)
#endif /* ICL_OCP_ENABLE */

#ifndef CCG_SYNC_ENABLE
#define CCG_SYNC_ENABLE                                  (0u)
#endif /* CCG_SYNC_ENABLE */

#ifndef BB_RETIMER_HPI_CTRL_ENABLE
#define BB_RETIMER_HPI_CTRL_ENABLE                       (0u)
#endif /* BB_RETIMER_HPI_CTRL_ENABLE */

#ifndef ICL_FORCE_TBT_MODE_SUPP
#define ICL_FORCE_TBT_MODE_SUPP                          (0u)
#endif /* ICL_FORCE_TBT_MODE_SUPP */


#ifndef DFP_ALT_MODE_SUPP
/* Enable Alternate Mode support when CCG is DFP */
#define DFP_ALT_MODE_SUPP                           (1u)
#endif /* DFP_ALT_MODE_SUPP */

#ifndef UFP_ALT_MODE_SUPP
/* Enable Alt mode as UFP */
#define UFP_ALT_MODE_SUPP                           (1u)
#endif /* UFP_ALT_MODE_SUPP */

#ifndef TBT_DFP_SUPP
/* Enable DisplayPort Source support as DFP */
#define TBT_DFP_SUPP                                (1u)
#endif /* TBT_DFP_SUPP */

#ifndef TBT_UFP_SUPP
/* Enable DisplayPort Source support as DFP */
#define TBT_UFP_SUPP                                (1u)
#endif /* TBT_UFP_SUPP */

#ifndef DP_DFP_SUPP
/* Enable DisplayPort Source support as DFP */
#define DP_DFP_SUPP                                 (1u)
#endif /* DP_DFP_SUPP */

#ifndef DP_UFP_SUPP
/* Enable DisplayPort Source support as DFP */
#define DP_UFP_SUPP                                 (1u)
#endif /* DP_UFP_SUPP */

#ifndef VIRTUAL_HPD_ENABLE
/* Virtual HPD usage enable */
#define VIRTUAL_HPD_ENABLE                          (0u)
#endif /* VIRTUAL_HPD_ENABLE */

#ifndef VPRO_WITH_USB4_MODE
/* Enable vPro for USB4 */
#define VPRO_WITH_USB4_MODE                         (0u)
#endif /* VPRO_WITH_USB4_MODE */

#ifndef VIRTUAL_HPD_DOCK
#define VIRTUAL_HPD_DOCK                            (0u)
#endif /* VIRTUAL_HPD_DOCK */

#ifndef GATKEX_CREEK
#define GATKEX_CREEK                                (0u)
#endif /* GATKEX_CREEK */

#ifndef MUX_DELAY_EN
#define MUX_DELAY_EN                                (0u)
#endif /* MUX_DELAY_EN */

#ifndef CCG_BB_ENABLE
#define CCG_BB_ENABLE                               (0u)
#endif /* CCG_BB_ENABLE */

#ifndef AM_PD3_FLOW_CTRL_EN
#define AM_PD3_FLOW_CTRL_EN                         (1u)
#endif /* AM_PD3_FLOW_CTRL_EN */

#ifndef HPI_AM_SUPP
#define HPI_AM_SUPP                                 (0u)
#endif /* HPI_AM_SUPP */

#ifndef ICL_SLAVE_ENABLE
#define ICL_SLAVE_ENABLE                            (0u)
#endif /* ICL_SLAVE_ENABLE */

#ifndef CY_PD_REV3_ENABLE
#define CY_PD_REV3_ENABLE                          (0u)
#endif /* CY_PD_REV3_ENABLE */

#ifndef AMD_RETIMER_ENABLE
#define AMD_RETIMER_ENABLE                          (0u)
#endif /* AMD_RETIMER_ENABLE */

#ifndef ICL_DP_DISABLE_APP_CHANGES
#define ICL_DP_DISABLE_APP_CHANGES                  (0u)
#endif /* ICL_DP_DISABLE_APP_CHANGES */

#ifndef CCG_BACKUP_FIRMWARE
#define CCG_BACKUP_FIRMWARE                         (0u)
#endif /* CCG_BACKUP_FIRMWARE */

#ifndef DELAYED_TBT_LSXX_CONNECT
#define DELAYED_TBT_LSXX_CONNECT                    (0u)
#endif /* DELAYED_TBT_LSXX_CONNECT */

#ifndef ICL_TBT_DISABLE_APP_CHANGES
#define ICL_TBT_DISABLE_APP_CHANGES                 (0u)
#endif /* ICL_TBT_DISABLE_APP_CHANGES */

#ifndef MAX_SVID_SUPP
#define MAX_SVID_SUPP                               (0u)
#endif /* MAX_SVID_SUPP */

#ifndef LEGACY_CFG_TABLE_SUPPORT
#define LEGACY_CFG_TABLE_SUPPORT                    (0u)
#endif /* LEGACY_CFG_TABLE_SUPPORT */

#ifndef UCSI_ALT_MODE_ENABLED
#define UCSI_ALT_MODE_ENABLED                       (0u)
#endif /* UCSI_ALT_MODE_ENABLED */

#ifndef UVDM_SUPP
#define UVDM_SUPP                                   (0u)
#endif /* UVDM_SUPP */

#ifndef APP_ALTMODE_CMD_ENABLE
#define APP_ALTMODE_CMD_ENABLE                      (0u)
#endif /* APP_ALTMODE_CMD_ENABLE */

#ifndef CCG_CBL_DISC_DISABLE
#define CCG_CBL_DISC_DISABLE                        (0u)
#endif /* CCG_CBL_DISC_DISABLE */

#ifndef UFP_MODE_DISC_ENABLE
#define UFP_MODE_DISC_ENABLE                        (0u)
#endif /* UFP_MODE_DISC_ENABLE */

#ifndef TBT_HOST_APP
#define TBT_HOST_APP                                (0u)
#endif /* TBT_HOST_APP */

#ifndef MUX_UPDATE_PAUSE_FSM
#define MUX_UPDATE_PAUSE_FSM                        (0u)
#endif /* MUX_UPDATE_PAUSE_FSM */

#ifndef AMD_SUPP_ENABLE
#define AMD_SUPP_ENABLE                             (0u)
#endif /* AMD_SUPP_ENABLE */

#ifndef ICL_ENABLE
#define ICL_ENABLE                                  (0u)
#endif /* ICL_ENABLE */

#ifndef ICL_ALT_MODE_HPI_DISABLED
#define ICL_ALT_MODE_HPI_DISABLED                   (0u)
#endif /* ICL_ALT_MODE_HPI_DISABLED */

#ifndef DP_2_1_ENABLE
#define DP_2_1_ENABLE                               (0u)
#endif /* DP_2_1_ENABLE */

#ifndef CCG_UCSI_ENABLE
#define CCG_UCSI_ENABLE                             (0u)
#endif /* CCG_UCSI_ENABLE */

#ifndef ICL_ALT_MODE_EVTS_DISABLED
#define ICL_ALT_MODE_EVTS_DISABLED                  (0u)
#endif /* ICL_ALT_MODE_EVTS_DISABLED */

#ifndef DP_GPIO_CONFIG_SELECT
#define DP_GPIO_CONFIG_SELECT                       (0u)
#endif /* DP_GPIO_CONFIG_SELECT */

#ifndef DP_UFP_DONGLE
#define DP_UFP_DONGLE                               (0u)
#endif /* DP_UFP_DONGLE */

#ifndef DEBUG_LOG
#define DEBUG_LOG                                   (0u)
#endif /* DEBUG_LOG */

#ifndef BB_RETIMER_ENABLE
#define BB_RETIMER_ENABLE                           (0u)
#endif /* BB_RETIMER_ENABLE */

#ifndef HPD_GPIO_SUPP_EN
#define HPD_GPIO_SUPP_EN                            (0u)
#endif /* HPD_GPIO_SUPP_EN */

#ifndef CY_HPI_ENABLED
#define CY_HPI_ENABLED                              (0u)
#endif /* CY_HPI_ENABLED */

#ifndef ICL_ENABLE_WAIT_FOR_SOC
#define ICL_ENABLE_WAIT_FOR_SOC                     (0u)
#endif /* ICL_ENABLE_WAIT_FOR_SOC */

#ifndef OBJ_POS_FIX
#define OBJ_POS_FIX                                 (0u)
#endif /* OBJ_POS_FIX */

#ifndef CUSTOM_OBJ_POSITION
#define CUSTOM_OBJ_POSITION                         (0u)
#endif /* CUSTOM_OBJ_POSITION */

#ifndef DEFER_VCONN_SRC_ROLE_SWAP
#define DEFER_VCONN_SRC_ROLE_SWAP                   (0u)
#endif /* DEFER_VCONN_SRC_ROLE_SWAP */

#ifndef MINOR_SVDM_VER_SUPPORT
#define MINOR_SVDM_VER_SUPPORT                      (0u)
#endif /* MINOR_SVDM_VER_SUPPORT */

#ifndef BILLBOARD_1_2_2_SUPPORT
#define BILLBOARD_1_2_2_SUPPORT                     (0u)
#endif /* BILLBOARD_1_2_2_SUPPORT */

#if (defined (CY_DEVICE_CCG5) || defined(CY_DEVICE_CCG6) || defined(CY_DEVICE_CCG5C) || \
        defined(CY_DEVICE_CCG6DF) || defined(CY_DEVICE_CCG6SF) || defined(CY_DEVICE_PMG1S3))
/* Enabled features common for CCG5, CCG5C, CCG6, CCG6SF, CCG6DF, and PMG1S3 */
#define CY_PD_DP_VCONN_SWAP_FEATURE                (1u)
#endif

#ifndef CY_PD_DP_VCONN_SWAP_FEATURE
#define CY_PD_DP_VCONN_SWAP_FEATURE                (1u)
#endif /* CY_PD_DP_VCONN_SWAP_FEATURE */

#ifndef CY_APP_USBC_DOCK_EXCHANGE_CAP_DP_USB3
#define CY_APP_USBC_DOCK_EXCHANGE_CAP_DP_USB3      (0u)
#endif /* CY_APP_USBC_DOCK_EXCHANGE_CAP_DP_USB3 */

#ifndef DP_21_FORCE_EN
#define DP_21_FORCE_EN                              (0u)
#endif /* DP_21_FORCE_EN */


/**
* \addtogroup group_pdaltmode_common 
* \{
* Refer to the \ref section_pdaltmode_quick_start section for compatibility.
*
********************************************************************************
* \defgroup group_pdaltmode_common_macros Macros
* \defgroup group_pdaltmode_common_data_structures Data structures
*/

/*******************************************************************************
 * MACRO definition
 ******************************************************************************/

/**
* \addtogroup group_pdaltmode_common_macros
* \{
*/

/** Total number of Type C ports supported */
#define CY_PDALTMODE_TOTAL_NO_OF_PORTS_SUP          (2u)

/** Periodicity of checking if Alt Mode which is on pause could be run */
#define CY_PDALTMODE_APP_POLL_PERIOD                                (5u)

/** App MUX VDM delay timer period in ms */
#define CY_PDALTMODE_APP_MUX_VDM_DELAY_TIMER_PERIOD                  (10u)

/** Periodicity of checking if paused HPD Dequeuing could be run */
#define CY_PDALTMODE_APP_HPD_DEQUE_POLL_PERIOD                       (3u)

/** Mask to identify is vPro mode supported in HPI host capabilities register */
#define CY_PDALTMODE_APP_HPI_VPRO_SUPP_MASK                          (0x10u)

/** Mask to identify active Alternate Modes */
#define CY_PDALTMODE_APP_ALT_MODE_STAT_MASK                          (0x3Fu)

/** Mask to identify that Discovery process completed */
#define CY_PDALTMODE_APP_DISC_COMPLETE_MASK                          (0x80u)

/** Mask to identify is USB4 mode entered*/
#define CY_PDALTMODE_APP_USB4_ACTIVE_MASK                            (0x40u)

/** Mask to identify that USB4 functionality is enabled */
#define CY_PDALTMODE_USB4_EN_HOST_PARAM_MASK                         (0x20u)

/** Mask to identify that TBT functionality is enabled */
#define CY_PDALTMODE_TBT_EN_HOST_PARAM_MASK                          (0x40u)

/** Mask to identify that DP 2.1 functionality is enabled */
#define CY_PDALTMODE_DP21_EN_HOST_PARAM_MASK                         (0x80u)

/** VDM busy timer period (in ms) */
#define CY_PDALTMODE_APP_VDM_BUSY_TIMER_PERIOD                       (50u)

/** Enter USB4 retry (on failure) timer period in ms */
#define CY_PDALTMODE_APP_USB4_ENTRY_RETRY_PERIOD                     (5u)

/** Enter USB4 wait timer period in ms */
#define CY_PDALTMODE_APP_ENTER_USB4_WAIT_PERIOD                      (150u)

/** Maximum number of Enter USB command retries in case of WAIT response */
#define CY_PDALTMODE_ENTER_USB_RETRY_MAX_LIMIT                       (1u)

/** VDM retry (on failure) timer period in ms */
#define CY_PDALTMODE_APP_VDM_FAIL_RETRY_PERIOD                       (100u)

/** Time allowed for cable power up to be complete */
#define CY_PDALTMODE_APP_CABLE_POWER_UP_DELAY                        (55u)

/** Cable query delay period in ms */
#define CY_PDALTMODE_APP_CABLE_VDM_START_DELAY                       (5u)

/** tAME timer period (in ms) */
#define CY_PDALTMODE_APP_AME_TIMEOUT_TIMER_PERIOD                    (1000u)

/** USB4 Timeout timer period (in ms) */
#define CY_PDALTMODE_APP_USB4_TIMEOUT_TIMER_PERIOD                   (1000u)

/** Timer period in milliseconds used to run Vconn swap after V5V was lost and recovered while UFP. */
#define CY_PDALTMODE_APP_UFP_RECOV_VCONN_SWAP_TIMER_PERIOD           (1000u)

/** DP2.1 HPD Wait timer period */
#define CY_PDALTMODE_APP_HPD_WAIT_TIMER_PERIOD                       (100u)

/** Maximum number of Alternate Modes which Alt Modes manager could operate in the same time.
     Setting this to larger values will increase RAM requirement for the projects. */
#define CY_PDALTMODE_MAX_SUPP_ALT_MODES                              (4u)

/** Maximum number of attached target SVIDs VDM task manager can hold in the memory. */
#define CY_PDALTMODE_MAX_SVID_VDO_SUPP                               (8u)

/** Size of Alt Mode APP event data in 4 byte words */
#define CY_PDALTMODE_EVT_SIZE                               (2u)

/** Maximum number of VDOs DP uses for the Alt Mode flow */
#define CY_PDALTMODE_MAX_DP_VDO_NUMB                                 (1u)

/** Maximum number of VDOs Thunderbolt-3 uses for the Alt Mode flow */
#define CY_PDALTMODE_MAX_TBT_VDO_NUMB                                (1u)

#define CY_PDALTMODE_MNGR_NO_DATA                            (0u)
/**< Zero data */

/** Enable HPD application command value */
#define CY_PDALTMODE_HPD_ENABLE_CMD                     (0u)

/** Disable HPD application command value */
#define CY_PDALTMODE_HPD_DISABLE_CMD                    (5u)

/** Standard SVID */
#define CY_PDATMODE_STD_SVID                            (0xFF00u)

/** Display port SVID defined by VESA specification */
#define CY_PDALTMODE_DP_SVID                            (0xFF01u)

/** Thunderbolt SVID defined by Intel specification */
#define CY_PDALTMODE_TBT_SVID                           (0x8087u)

/** USB4 Mode */
#define CY_PDALTMODE_USB_MODE_USB4                      (2u)

#define CY_PDALTMODE_FAULT_APP_PORT_VCONN_FAULT_ACTIVE                     (0x01U)
/**< Status bit that indicates VConn fault is active */

#define CY_PDALTMODE_FAULT_APP_PORT_V5V_SUPPLY_LOST                        (0x10U)
/**< Status bit that indicates that V5V supply (for VConn) has been lost */

/** Alternate Modes Support UFP VDO1 mask */
#define CY_PDALTMODE_UFP_ALT_MODE_SUPP_MASK              (0x38u)

/** TBT Alternate Modes Support UFP VDO1 mask */
#define CY_PDALTMODE_UFP_TBT_ALT_MODE_SUPP_MASK          (0x8u)

/** Mask to indicate that TBT data path should be disabled */
#define CY_PDALTMODE_TBT_DATA_EXIT_MASK                  (0xCu)

/** Maximum length of Discover ID response */
#define CY_PDALTMODE_MAX_DISC_ID_RESP_LEN                 (CY_PD_MAX_NO_OF_DO)

/** Maximum length of Discover SVID response */
#define CY_PDALTMODE_MAX_DISC_SVID_RESP_LEN               (18u)

/** SVID value assigned to HPI based Alternate Mode. 0 indicates that actual SVID value is taken from
    the configuration table or HPI register. */
#define CY_PDALTMODE_HPI_AM_SVID                        (0x0000u)

/** Number of Data Objects used in VDMs associated with HPI based Alternate Mode implementation */
#define CY_PDALTMODE_MAX_HPI_AM_VDO_NUMB                (1u)

/** Discover ID response minimum size */
#define CY_PDALTMODE_VDM_DID_MIN_SIZE                   (20)

/** Discover ID response ID header VDO offset */
#define CY_PDALTMODE_VDM_RESP_ID_HEADER_OFFSET          (8)

/** \} group_pdaltmode_common_macros */

/** \cond */
#ifndef ALT_MODE_CALL_MAP
#define ALT_MODE_CALL_MAP
#endif /* ALT_MODE_CALL_MAP */

#ifndef ATTRIBUTES_ALT_MODE
#define ATTRIBUTES_ALT_MODE
#endif /* ATTRIBUTES_HPISS_HPI */

#ifndef SROM_CODE_ALT_MODE
#define SROM_CODE_ALT_MODE                        (0U)
#endif /* SROM_CODE_ALT_MODE */
/** \endcond */

/** \} group_pdaltmode_common */

/**
* \addtogroup group_pdaltmode_vdm_alt_mode_manager 
* \{
*/

/*******************************************************************************
 * Enumerated data definition
 ******************************************************************************/

/**
* \addtogroup group_pdaltmode_vdm_alt_mode_manager_enums
* \{
*/

/**
  @typedef cy_en_pdaltmode_vdm_vconn_rqt_state_t
  @brief Enumeration lists the VDM state with respect to VConn Swap and cable discovery
 */
typedef enum
{
    CY_PDALTMODE_VCONN_RQT_INACTIVE = 0,             /**< No VConn request active. */
    CY_PDALTMODE_VCONN_RQT_PENDING,                  /**< VConn swap and cable discovery request is pending. */
    CY_PDALTMODE_VCONN_RQT_ONGOING,                  /**< VDM state machine waiting for VConn Swap and cable discovery completion. */
    CY_PDALTMODE_VCONN_RQT_FAILED                    /**< VConn Swap request rejected by port partner. */
} cy_en_pdaltmode_vdm_vconn_rqt_state_t;

/**
 * @typedef cy_en_pdaltmode_mngr_state_t
 * @brief Enumeration holds all possible Alt Modes manager states when DFP
 */
typedef enum
{
    CY_PDALTMODE_MNGR_STATE_DISC_MODE = 0,  /**< Alt Modes manager discovery mode state. */
    CY_PDALTMODE_MNGR_STATE_PROCESS         /**< Alt Modes manager Alternate Modes processing state. */
}cy_en_pdaltmode_mngr_state_t;

/**
 * @typedef cy_en_pdaltmode_state_t
 * @brief Enumeration holds all possible states of each Alt Mode which is handled by Alt Modes manager
 */
typedef enum
{
    CY_PDALTMODE_STATE_DISABLE = 0,         /**< State when Alternate Mode functionality is disabled */
    CY_PDALTMODE_STATE_IDLE,                /**< State when Alternate Mode is idle */
    CY_PDALTMODE_STATE_INIT,                /**< State when Alternate Mode initiate its functionality */
    CY_PDALTMODE_STATE_SEND,                /**< State when Alternate Mode needs to send VDM message */
    CY_PDALTMODE_STATE_WAIT_FOR_RESP,       /**< State while Alternate Mode wait for VDM response */
    CY_PDALTMODE_STATE_PAUSE,               /**< State while Alternate Mode is on pause and waiting for some trigger */
    CY_PDALTMODE_STATE_FAIL,                /**< State when Alternate Mode VDM response fails */
    CY_PDALTMODE_STATE_RUN,                 /**< State when Alternate Mode need to be running */
    CY_PDALTMODE_STATE_EXIT                 /**< State when Alternate Mode exits and resets all related variables */
}cy_en_pdaltmode_state_t;

/**
 * @typedef cy_en_pdaltmode_app_evt_t
 * @brief Enumeration holds all possible application events related to Alt Modes handling
 */
typedef enum
{
    CY_PDALTMODE_NO_EVT = 0,                      /**< Empty event */
    CY_PDALTMODE_EVT_SVID_NOT_FOUND,              /**< Sends to EC if UFP does not support any SVID */
    CY_PDALTMODE_EVT_ALT_MODE_ENTERED,            /**< Alternate Mode entered */
    CY_PDALTMODE_EVT_ALT_MODE_EXITED,             /**< Alternate Mode exited */
    CY_PDALTMODE_EVT_DISC_FINISHED,               /**< Discovery process was finished */
    CY_PDALTMODE_EVT_SVID_NOT_SUPP,               /**< CCGx does not support received SVID */
    CY_PDALTMODE_EVT_SVID_SUPP,                   /**< CCGx supports received SVID */
    CY_PDALTMODE_EVT_ALT_MODE_SUPP,               /**< CCGx supports Alternate Mode */
    CY_PDALTMODE_EVT_SOP_RESP_FAILED,             /**< UFP VDM response failed */
    CY_PDALTMODE_EVT_CBL_RESP_FAILED,             /**< Cable response failed */
    CY_PDALTMODE_EVT_CBL_NOT_SUPP_ALT_MODE,       /**< Cable capabilities could not provide Alternate Mode handling */
    CY_PDALTMODE_EVT_NOT_SUPP_PARTNER_CAP,        /**< CCGx and UFP capabilities not consistent */
    CY_PDALTMODE_EVT_DATA_EVT,                    /**< Specific Alternate Mode event with data */
    CY_PDALTMODE_EVT_SUPP_ALT_MODE_CHNG,          /**< Same functionality as ALT_MODE_SUPP; new one for UCSI */
}cy_en_pdaltmode_app_evt_t;

/**
  @typedef cy_en_pdaltmode_vdm_task_t
  @brief Enumeration lists the various VDM manager tasks to handle VDMs
 */
typedef enum
{
    CY_PDALTMODE_VDM_TASK_WAIT = 0,           /**< DFP manager wait task while waiting for VDM response */
    CY_PDALTMODE_VDM_TASK_INIT,               /**< Task responsible for initializing of VDM manager */
    CY_PDALTMODE_VDM_TASK_DISC_ID,            /**< Task responsible for VDM Discovery ID flow */
    CY_PDALTMODE_VDM_TASK_DISC_SVID,          /**< Task responsible for VDM Discovery SVID flow */
    CY_PDALTMODE_VDM_TASK_REG_ATCH_TGT_INFO,  /**< Task responsible for registering of Discovery result
                                                   information in Alt Mode manager */
    CY_PDALTMODE_VDM_TASK_USB4_TBT,           /**< Task handles the USB4 data discovery and entry */
    CY_PDALTMODE_VDM_TASK_EXIT,               /**< Task deinits VDM task manager */
    CY_PDALTMODE_VDM_TASK_SEND_MSG,           /**< Task responsible for forming and sending VDM message */
    CY_PDALTMODE_VDM_TASK_ALT_MODE            /**< Task responsible for running of Alt Mode manager */
}cy_en_pdaltmode_vdm_task_t; 

/**
  @typedef cy_en_pdaltmode_vdm_evt_t
  @brief Enumeration lists the various VDM manager events to handle VDMs
 */
typedef enum
{
    CY_PDALTMODE_VDM_EVT_RUN = 0,             /**< Event responsible for running any of DFP VDM manager task */
    CY_PDALTMODE_VDM_EVT_EVAL,                /**< Event responsible for evaluating VDM response */
    CY_PDALTMODE_VDM_EVT_FAIL,                /**< Event notifies task manager task if VDM response fails */
    CY_PDALTMODE_VDM_EVT_EXIT                 /**< Event runs exiting from VDM task manager task */
}cy_en_pdaltmode_vdm_evt_t;

/**
   @typedef cy_en_pdaltmode_cable_dp_reset_state_t
   @brief Enumeration for tracking current cable (SOP'') soft reset state.
 */
typedef enum {
    CY_PDALTMODE_CABLE_DP_RESET_IDLE = 0,    /**< No Cable (SOP'') reset state */
    CY_PDALTMODE_CABLE_DP_RESET_WAIT,        /**< Waiting for response from cable (SOP'') soft reset */
    CY_PDALTMODE_CABLE_DP_RESET_RETRY,       /**< Cable soft reset (SOP'') to be attempted */
    CY_PDALTMODE_CABLE_DP_RESET_DONE,        /**< Cable soft reset (SOP'') has been completed */
    CY_PDALTMODE_CABLE_DP_RESET_SEND_SPRIME  /**< Cable soft reset (SOP'') has been completed and send SOP Prime msg again */
} cy_en_pdaltmode_cable_dp_reset_state_t;

/**
  @enum cy_en_pdaltmode_app_cmd_t
  @brief Enumeration holds all possible APP command related to Alt modes handling
 */
typedef enum
{
    CY_PDALTMODE_NO_CMD = 0,                      /**< Empty command */
    CY_PDALTMODE_SET_TRIGGER_MASK,                /**< Sets trigger mask to prevent auto-entering of the selected
                                                        Alternate Modes */
    CY_PDALTMODE_CMD_ENTER = 3,                   /**< Enter to selected Alternate Mode */
    CY_PDALTMODE_CMD_EXIT,                        /**< Exit from selected Alternate Mode */
    CY_PDALTMODE_CMD_SPEC,                        /**< Specific alternate EC mode command with data */
#if CY_PD_REV3_ENABLE
    CY_PDALTMODE_CMD_RUN_UFP_DISC                 /**< Runs Discover command when CCG is UFP due to PD 3.0 spec */
#endif /* CY_PD_REV3_ENABLE */
}cy_en_pdaltmode_app_cmd_t;

/**
 * @enum cy_en_pdaltmode_fail_status_t
 * @brief Enum of the VDM failure response status
 */
typedef enum
{
    CY_PDALTMODE_VDM_BUSY = 0,                           /**<  Target is BUSY  */
    CY_PDALTMODE_VDM_GOOD_CRC_NOT_RSVD,                  /**<  Good CRC wasn't received  */
    CY_PDALTMODE_VDM_TIMEOUT,                            /**<  No response or corrupted packet  */
    CY_PDALTMODE_VDM_NACK                                /**<  Sent VDM NACKed  */
} cy_en_pdaltmode_fail_status_t;

/** \} group_pdaltmode_vdm_alt_mode_manager_enums */

/** \} group_pdaltmode_vdm_alt_mode_manager */

/**
* \addtogroup group_pdaltmode_display_port 
* \{
*/

/**
* \addtogroup group_pdaltmode_display_port_enums
* \{
*/

/**
 * @typedef cy_en_pdaltmode_dp_state_t
 * @brief Enumeration holds all possible DP states
 */
typedef enum
{
    CY_PDALTMODE_DP_STATE_IDLE = 0,                  /**< Idle state */
    CY_PDALTMODE_DP_STATE_ENTER = 4,                 /**< Enter mode state */
    CY_PDALTMODE_DP_STATE_EXIT = 5,                  /**< Exit mode state */
    CY_PDALTMODE_DP_STATE_ATT = 6,                   /**< DP Attention state */
    CY_PDALTMODE_DP_STATE_STATUS_UPDATE = 16,        /**< DP Status Update state */
    CY_PDALTMODE_DP_STATE_CONFIG = 17                /**< DP Configure state */
}cy_en_pdaltmode_dp_state_t;

/**
  @typedef cy_en_pdaltmode_dp_stat_bm_t
  @brief Enumeration holds corresponding bit positions of Status Update VDM fields
 */
typedef enum
{
    CY_PDALTMODE_DP_STAT_DFP_CONN = 0,               /**< DFP_D is connected field bit position */
    CY_PDALTMODE_DP_STAT_UFP_CONN,                   /**< UFP_D is connected field bit position */
    CY_PDALTMODE_DP_STAT_PWR_LOW,                    /**< Power Low field bit position */
    CY_PDALTMODE_DP_STAT_EN,                         /**< Enabled field bit position */
    CY_PDALTMODE_DP_STAT_MF,                         /**< Multi-function Preferred field bit position */
    CY_PDALTMODE_DP_STAT_USB_CNFG,                   /**< USB Configuration Request field bit position */
    CY_PDALTMODE_DP_STAT_EXIT,                       /**< Exit DP Request field bit position */
    CY_PDALTMODE_DP_STAT_HPD,                        /**< HPD state field bit position */
    CY_PDALTMODE_DP_STAT_IRQ                         /**< HPD IRQ field bit position */
}cy_en_pdaltmode_dp_stat_bm_t;
 
 /** \} group_pdaltmode_display_port_enums */

 /** \} group_pdaltmode_display_port */

 /**
* \addtogroup group_pdaltmode_usb4 
* \{
*/

/**
* \addtogroup group_pdaltmode_usb4_enums
* \{
*/

/**
  @typedef cy_en_pdaltmode_usb4_flag_t
  @brief Enumeration lists the various vdm manager flags due to USB4 related handling.
 */
typedef enum
{
    CY_PDALTMODE_USB4_NONE = 0,                  /**< Empty flag */
    CY_PDALTMODE_USB4_FAILED,                    /**< Flag indicates of USB4 discovery procedure failure */
    CY_PDALTMODE_USB4_PENDING,                   /**< Flag indicates that USB4 entry handling should be processed */
    CY_PDALTMODE_USB4_TBT_CBL_FIND,              /**< Flag is responsible for initiating of finding TBT VID in cable Disc SVID response */
    CY_PDALTMODE_USB4_TBT_CBL_DISC_RUN,          /**< Flag is responsible for initiating of TBT cable Disc mode */
    CY_PDALTMODE_USB4_TBT_CBL_ENTER_SOP_P,       /**< Flag is responsible for initiating of TBT cable SOP' Enter mode */
    CY_PDALTMODE_USB4_TBT_CBL_ENTER_SOP_DP,      /**< Flag is responsible for initiating of TBT cable SOP" Disc mode */
    CY_PDALTMODE_USB4_SEND_USB4                  /**< Flag is responsible for sending of EnterUSB command */

}cy_en_pdaltmode_usb4_flag_t;

 /** \} group_pdaltmode_usb4_enums */

 /** \} group_pdaltmode_usb4 */

 /**
* \addtogroup group_pdaltmode_alt_mode_hw 
* \{
*/

/**
* \addtogroup group_pdaltmode_alt_mode_hw_enums
* \{
*/

/**
  @typedef cy_en_pdaltmode_mux_select_t
  @brief Possible settings for the Type-C Data MUX.
  @note This type should be extended to cover all possible modes for the MUX.
 */
typedef enum
{
    CY_PDALTMODE_MUX_CONFIG_ISOLATE,                  /**< Isolate configuration */
    CY_PDALTMODE_MUX_CONFIG_SAFE,                     /**< USB Safe State (USB 2.0 lines remain active) */
    CY_PDALTMODE_MUX_CONFIG_SS_ONLY,                  /**< USB SS configuration */
    CY_PDALTMODE_MUX_CONFIG_DP_2_LANE,                /**< Two lane DP configuration */
    CY_PDALTMODE_MUX_CONFIG_DP_4_LANE,                /**< Four lane DP configuration */
    CY_PDALTMODE_MUX_CONFIG_TBT_CUSTOM,               /**< TBT custom configuration */
    CY_PDALTMODE_MUX_CONFIG_USB4_CUSTOM,              /**< USB4 custom configuration */
    CY_PDALTMODE_MUX_CONFIG_RIDGE_CUSTOM,             /**< Alpine/Titan Ridge custom configuration */
    CY_PDALTMODE_MUX_CONFIG_INIT,                     /**< Enables MUX functionality */
    CY_PDALTMODE_MUX_CONFIG_DEINIT,                   /**< Disables MUX functionality */
} cy_en_pdaltmode_mux_select_t;

/**
  @typedef cy_en_pdaltmode_app_mux_status_t
  @brief Possible states for the MUX handler
 */
typedef enum
{
    CY_PDALTMODE_APP_MUX_STATE_IDLE,                      /**< MUX idle state */
    CY_PDALTMODE_APP_MUX_STATE_FAIL,                      /**< MUX switch failed */
    CY_PDALTMODE_APP_MUX_STATE_BUSY,                      /**< MUX is busy */
    CY_PDALTMODE_APP_MUX_STATE_SUCCESS                    /**< MUX switched successfully */
} cy_en_pdaltmode_app_mux_status_t;

/**
  @typedef cy_en_pdaltmode_hw_type_t
  @brief Application HW type values which are used in alt_mode_hw_evt_t structure
 */
typedef enum
{
    CY_PDALTMODE_MUX = 1,                    /**< HW type - MUX */
    CY_PDALTMODE_HPD,                        /**< HW type - HPD transceiver/receiver */

}cy_en_pdaltmode_hw_type_t;

 /** \} group_pdaltmode_alt_mode_hw_enums */

 /** \} group_pdaltmode_alt_mode_hw */
 
  /**
* \addtogroup group_pdaltmode_intel 
* \{
*/

/**
* \addtogroup group_pdaltmode_intel_enums
* \{
*/

/**
  @typedef cy_en_pdaltmode_tbt_state_t
  @brief Enumeration holds all possible TBT states
 */
typedef enum
{
    CY_PDALTMODE_TBT_STATE_IDLE = 0,                 /**< Idle state */
    CY_PDALTMODE_TBT_STATE_ENTER = 4,                /**< Enter mode state */
    CY_PDALTMODE_TBT_STATE_EXIT = 5,                 /**< Exit mode state */
    CY_PDALTMODE_TBT_STATE_ATT = 6,                  /**< Attention state */
} cy_en_pdaltmode_tbt_state_t;

/**
 * @typedef cy_en_pdaltmode_intel_pf_type_t
 * @brief List of Intel TBT/USB platform types in which the EZ-PD(TM) CCG device is used
 */
typedef enum
{
    CY_PDALTMODE_PF_THUNDERBOLT = 0,                         /**< Thunderbolt platforms such as Alpine Ridge or Titan Ridge */
    CY_PDALTMODE_PF_ICE_LAKE,                                /**< Intel IceLake platform */
    CY_PDALTMODE_PF_TIGER_LAKE,                              /**< Intel TigerLake + AlderLake + RaptorLake + MeteorLake platform */
    CY_PDALTMODE_PF_MAPLE_RIDGE,                             /**< Intel RocketLake + Maple Ridge platform */
    CY_PDALTMODE_PF_BARLOW_RIDGE,                            /**< Intel Barlow Ridge platform */
    CY_PDALTMODE_PF_LUNAR_LAKE                               /**< Intel LunarLake platform */
} cy_en_pdaltmode_intel_pf_type_t;

 /** \} group_pdaltmode_intel_enums */

 /** \} group_pdaltmode_intel */

 /**
* \addtogroup group_pdaltmode_billboard 
* \{
*/

/**
* \addtogroup group_pdaltmode_billboard_enums
* \{
*/
/**
 * @enum cy_en_pdaltmode_bb_type_t
 * @brief Billboard implementation model
 */
typedef enum
{
    CY_PDALTMODE_BB_TYPE_NONE,               /**< No Billboard device */
    CY_PDALTMODE_BB_TYPE_EXTERNAL,           /**< External Billboard device */
    CY_PDALTMODE_BB_TYPE_INTERNAL,           /**< Device supports internal USB module to implement Billboard */
    CY_PDALTMODE_BB_TYPE_EXT_CONFIGURABLE,   /**< External Billboard device which supports configuration through PD controller */
    CY_PDALTMODE_BB_TYPE_EXT_CFG_HUB,        /**< External Billboard device behind USB hub */
    CY_PDALTMODE_BB_TYPE_EXT_HX3PD           /**< Billboard/DMC integrated in HX3PD hub */
} cy_en_pdaltmode_bb_type_t;

/**
 * @enum cy_en_pdaltmode_bb_state_t
 * @brief Billboard module states
 */
typedef enum
{
    CY_PDALTMODE_BB_STATE_DEINITED,          /**< Module is not initialized */
    CY_PDALTMODE_BB_STATE_DISABLED,          /**< Module is initialized but not enabled */
    CY_PDALTMODE_BB_STATE_BILLBOARD,         /**< Module is active with Billboard enumeration */
    CY_PDALTMODE_BB_STATE_LOCKED,            /**< Module is active with additional functionality */
    CY_PDALTMODE_BB_STATE_FLASHING,          /**< Module is active with flashing mode enumeration */

} cy_en_pdaltmode_bb_state_t;

 /** \} group_pdaltmode_billboard_enums */

 /** \} group_pdaltmode_billboard */

/** @cond DOXYGEN_HIDE */
 /**
* \addtogroup group_pdaltmode_amd 
* \{
*/

/**
* \addtogroup group_pdaltmode_amd_enums
* \{
*/
/**
 * @typedef cy_en_pdaltmode_amd_pf_type_t
 * @brief List of AMD platform types in which the EZ-PD(TM) CCG device is used
 */
typedef enum
{
    CY_PDALTMODE_AMD_PF_RENOIR = 0,                         /**< AMD Renoir-based platform */
    CY_PDALTMODE_AMD_PF_REMBRANDT_A0,                       /**< AMD Rembrandt A0-based platform */
    CY_PDALTMODE_AMD_PF_REMBRANDT_B0,                       /**< AMD Rembrandt B0-based platform */
    CY_PDALTMODE_AMD_PF_PHOENIX,                            /**< AMD Phoenix-based platform */
    CY_PDALTMODE_AMD_PF_STRIX                               /**< AMD Strix-based platform */
} cy_en_pdaltmode_amd_pf_type_t;

/**
 * @typedef cy_en_pdaltmode_amd_rtmr_type_t
 * @brief List of Retimer types which could be used via AMD projects
 */
typedef enum
{
    CY_PDALTMODE_AMD_RETIMER_NONE = 0,                      /**< No Retimer */
    CY_PDALTMODE_AMD_RETIMER_PI3DPX1205A,                   /**< Retimer PI3DPX1205A */
    CY_PDALTMODE_AMD_RETIMER_PS8828,                        /**< Retimer PS8828 */
    CY_PDALTMODE_AMD_RETIMER_PS8830,                        /**< Retimer PS8830 */
    CY_PDALTMODE_AMD_RETIMER_AUTO_PS8828A_OR_PS8830,        /**< Retimer type should be discovered */
    CY_PDALTMODE_AMD_RETIMER_KB800X_B0,                     /**< Retimer KB800X B0 */
    CY_PDALTMODE_AMD_RETIMER_KB800X_B1,                     /**< Retimer KB800X B1 */
    CY_PDALTMODE_AMD_RETIMER_PI2DPX1066,                    /**< Retimer PI2DPX1066 */
    CY_PDALTMODE_AMD_RETIMER_KB8010                         /**< Retimer KB8010 */
} cy_en_pdaltmode_amd_rtmr_type_t;

 /** \} group_pdaltmode_amd_enums */

 /** \} group_pdaltmode_amd */
/** @endcond */

/*****************************************************************************
 * Data structure definition
 ****************************************************************************/

 /**
* \addtogroup group_pdaltmode_intel 
* \{
*/

/**
* \addtogroup group_pdaltmode_intel_data_structures
* \{
*/

/**
 *  @struct cy_stc_pdaltmode_ridge_hw_config_t
 *  @brief This structure holds all HW related info due to Ridge/SoC interface such
 *  as I2C SCB block, interrupt GPIO, and interrupt configuration.
 */
#if TBT_HOST_APP
typedef struct
{
    CySCB_Type* ridgeScbBase;                       /**< Ridge SCB base address pointer */
    cy_stc_hpi_i2c_context_t* ridgeI2cContext;      /**< I2C Context pointer */
    const cy_stc_sysint_t* ridgeScbIrqConfig;       /**< SCB Irq configuration */
    GPIO_PRT_Type * intrAPortNum;                   /**< PortA interrupt port address */
    uint32_t intrAPinNum;                           /**< PortA interrupt pin */
    GPIO_PRT_Type * intrBPortNum;                   /**< PortB interrupt port address */
    uint32_t intrBPinNum;                           /**< PortB interrupt pin */
} cy_stc_pdaltmode_ridge_hw_config_t;
#else
typedef struct
{
    CySCB_Type* ridgeScbNum;                         /**< Ridge SCB number */
    const cy_stc_scb_i2c_config_t* ridgeI2cConfig;   /**< I2C Configuration pointer */
    const cy_stc_sysint_t* ridgeScbIrqConfig;        /**< SCB Irq configuration */
    GPIO_PRT_Type* intrPortA_port;                   /**< PortA interrupt port */
    uint32_t intrPortA_pin;                          /**< PortA interrupt pin */
    GPIO_PRT_Type* intrPortB_port;                   /**< PortB interrupt port */
    uint32_t intrPortB_pin;                          /**< PortB interrupt pin */
} cy_stc_pdaltmode_ridge_hw_config_t;
#endif /* TBT_HOST_APP */

/** \} group_pdaltmode_intel_data_structures */

/** \} group_pdaltmode_intel */

 /**
* \addtogroup group_pdaltmode_alt_mode_hw 
* \{
*/

/**
* \addtogroup group_pdaltmode_alt_mode_hw_data_structures
* \{
*/

/**
  @union cy_stc_pdaltmode_hw_evt_t
  @brief Alt Mode HW application event/command structure
 */
typedef union
{

    uint32_t val;                       /**< Integer field used for direct manipulation of reason code */

    /** @brief Structure containing Alternate Modes HW event/command */
    struct ALT_MODE_HW_EVT
    {
        uint32_t evtData    : 16;      /**< Current event/command data */
        uint32_t hwType     : 8;       /**< HW type event/command related to */
        uint32_t dataRole   : 8;       /**< Current data role */
    }hw_evt;                           /**< Union containing the application HW event/command value */

}cy_stc_pdaltmode_hw_evt_t;

/**
 * @union cy_stc_pdaltmode_alt_mode_evt_t
 * @brief Alt Modes manager application event/command structure
 */
typedef union
{
    uint32_t val;                        /**< Integer field used for direct manipulation of reason code */

    /** @brief Struct containing Alternate Modes manager event/command */
    struct ALT_MODE_EVT
    {
        uint32_t dataRole       : 1;      /**< Current event sender data role. */
        uint32_t altModeEvt    : 7;       /**< Alt Mode event index from alt_mode_app_evt_t structure */
        uint32_t altMode        : 8;      /**< Alt Mode ID */
        uint32_t svid            : 16;    /**< Alt Mode related SVID */
    }alt_mode_event;                      /**< Union containing the Alt Mode event value */

    /** @brief Struct containing Alternate Modes manager event/command data. */
    struct ALT_MODE_EVT_DATA
    {
        uint32_t evtData        : 24;     /**< Alt Mode event's data */
        uint32_t evtType        : 8;      /**< Alt Mode event's type */
    }alt_mode_event_data;                 /**< Union containing the Alt Mode event's data value */

}cy_stc_pdaltmode_alt_mode_evt_t;

/** \} group_pdaltmode_alt_mode_hw_data_structures */

/** \} group_pdaltmode_alt_mode_hw */

 /**
* \addtogroup group_pdaltmode_intel 
* \{
*/

/**
* \addtogroup group_pdaltmode_intel_data_structures
* \{
*/

/**
 * @struct cy_stc_pdaltmode_host_details_t
 * @brief Host Details Status
 */
typedef struct
{
    /*
     *   Mode mask:
     *   Bit 0 - TBT DFP mode
     *   Bit 1 - TBT UFP mode
     *   Bit 2 - DP DFP mode
     *   Bit 3 - DP UFP mode
    */
    void *          dsAltModeContext;                   /**< PD Alt Mode context pointer for DS port*/
    void *          usAltModeContext;                   /**< PD Alt Mode context pointer for US port*/
    uint32_t        backUpRidgeStatus;                  /**< Back up Ridge status*/
    uint8_t         hostModeMask;                       /**< Host mode mask */
    uint8_t         dsModeMask;                         /**< DS mode mask  */
    uint8_t         hostDp2LaneModeCtrl;                /**< Host DP 2-lane mode control */
    uint8_t         dsDp2LaneModeCtrl;                  /**< DS DP 2-lane mode control */
    cy_pd_pd_do_t   hostEudo;                           /**< Host USB4 data object */
    uint32_t        hostDetails;                        /**< Host details */
    uint8_t         grRdyBit;                           /**< Ridge gr_rdy bit value */
    bool            isHostDetailsAvailable;             /**< Is Host details available */
    uint8_t         dsSendEuWithHostPresentSet;         /**< Send USB4 EUDO with host present bit set  */
}cy_stc_pdaltmode_host_details_t;

/** \} group_pdaltmode_intel_data_structures */

/** \} group_pdaltmode_intel */

 /**
* \addtogroup group_pdaltmode_alt_mode_hw 
* \{
*/

/**
* \addtogroup group_pdaltmode_alt_mode_hw_data_structures
* \{
*/

/**
  @union cy_stc_pdaltmode_mux_cfg_t
  @brief Holds AMD APU MUX related configurations
 */
typedef union
{
    uint8_t val;                        /**< Integer field used for direct manipulation of reason code */

    /** @brief Struct containing AMD APU related MUX configuration */
    struct MUX_CONFIG
    {
        uint8_t polarity        : 1;    /**< MUX polarity */
        uint8_t cfg             : 7;    /**< MUX configuration */
    }mux;                               /**< Union containing the MUX configuration to configure AMD APU */

}cy_stc_pdaltmode_mux_cfg_t;

/** \} group_pdaltmode_alt_mode_hw_data_structures */

/** \} group_pdaltmode_alt_mode_hw */

 /**
* \addtogroup group_pdaltmode_vdm_alt_mode_manager 
* \{
*/

/**
* \addtogroup group_pdaltmode_vdm_alt_mode_manager_data_structures
* \{
*/

/**
 * @struct cy_stc_pdaltmode_atch_tgt_info_t
 * @brief Struct holds Alternate Mode discovery information which is used by Alt Mode manager
 */
typedef struct
{
    cy_pd_pd_do_t tgtIdHeader;                             /**< Holds device/AMA discovery ID header */
    cy_pd_pd_do_t amaVdo;                                  /**< Holds AMA discovery ID response VDO */
    const cy_pd_pd_do_t* cblVdo;                           /**< Pointer to cable VDO */
    uint16_t tgtSvid[CY_PDALTMODE_MAX_SVID_VDO_SUPP];      /**< Holds received SVID for device/AMA */
    uint16_t cblSvid[CY_PDALTMODE_MAX_SVID_VDO_SUPP];      /**< Holds received SVID for cable */
}cy_stc_pdaltmode_atch_tgt_info_t;

/**
 * @struct cy_stc_pdaltmode_vdm_msg_info_t
 * @brief Struct holds received/sent VDM information which is used by VDM Alternate Mode managers
 */
typedef struct
{
    cy_pd_pd_do_t vdmHeader;                   /**< Holds VDM buffer. */
    uint8_t sopType;                           /**< VDM SOP type */
    uint8_t vdoNumb;                           /**< Number of received VDOs in VDM */
    cy_pd_pd_do_t vdo[7u];                     /**< VDO objects buffer */
}cy_stc_pdaltmode_vdm_msg_info_t;

/**
  @struct cy_stc_pdaltmode_alt_mode_reg_info_t
  @brief Structure holds all necessary information on Discovery Mode stage
  for supported Alternate Mode when Alt Modes manager registers new Alt Mode.
 */
typedef struct
{
    uint8_t atchType;                       /**< Target of disc SVID (cable or device/AMA) */
    uint8_t dataRole;                       /**< Current data role */
    uint8_t altModeId;                      /**< Alt Mode ID */
    cy_en_pd_sop_t cblSopFlag;              /**< Sop indication flag */
    cy_pd_pd_do_t svidEmcaVdo;              /**< SVID VDO from cable Discovery mode response */
    cy_pd_pd_do_t svidVdo;                  /**< SVID VDO from Discovery mode SOP response */
    const cy_stc_pdaltmode_atch_tgt_info_t* atchTgtInfo;   /**< Attached targets info (dev/ama/cbl) from Discovery ID response */
    cy_en_pdaltmode_app_evt_t appEvt;      /**< APP event */
} cy_stc_pdaltmode_alt_mode_reg_info_t;

/**
 * @struct cy_stc_pdaltmode_vdm_task_status_t
 * @brief Struct to hold VDM manager status
 */
typedef struct
{
    /** Current VDM manager state */
    cy_en_pdaltmode_vdm_task_t      vdmTask;
    /** Current VDM manager event */
    cy_en_pdaltmode_vdm_evt_t vdmEvt;
    /** Info about cable, device/AMA */
    cy_stc_pdaltmode_atch_tgt_info_t atchTgt;
    /** Struct with received/sent VDM info */
    cy_stc_pdaltmode_vdm_msg_info_t  vdmMsg;
    /** Sent/received VDM retry counters */
    uint8_t         recRetryCnt;
    /** Holds svid index if we have more than one Disc SVID response */
    uint8_t         svidIdx;
    /** Holds count of D_SVID requests sent */
    uint8_t         dSvidCnt;
    /** VDM EMCA Resets count */
    uint8_t         vdmEmcaRstCount;
    /** VDM EMCA Reset state */
    cy_en_pdaltmode_cable_dp_reset_state_t vdmEmcaRstState;
    /** VDM VCONN SWAP Request state */
    cy_en_pdaltmode_vdm_vconn_rqt_state_t vdmVcsRqtState;
    /** VDM VCONN SWAP Request count */
    uint8_t vdmVcsRqtCount;
    /** Enter USB data object */
    cy_stc_pdstack_dpm_pd_cmd_buf_t eudoBuf;
    /** Flag indicating USB4 status */
    cy_en_pdaltmode_usb4_flag_t      usb4Flag;
    /** Alt modes supported flag */
    uint8_t          altModesNotSupp;
    /** USB4 pending flag */
    uint8_t          usb4UpdatePending;
    /** USB4 Enter mode retry count for wait response */
    uint8_t          usb4EnterRetryCount;
    /** Cable start-up delay period */
    uint16_t          appCblStartDelay;
    /** VDM busy delay period */
    uint16_t          vdmBusyDelay;
    /** Enter USB4 wait period */
    uint16_t          enterUsb4WaitDelay;
    /** VDM fail retry period */
    uint16_t          vdmFailRetryDelay;
}cy_stc_pdaltmode_vdm_task_status_t;

/**
 * @brief Billboard USB descriptor update function callback
 * 
 * @param context Pointer to the Alt Mode context
 *
 */
typedef void (*cy_pdaltmode_bb_bos_decr_update_cbk_t)(void* context);

/**
 * @brief Function is used by Alt Modes manager to communicate with
 * any of supported Alt Modes.
 *
 * @param context Pointer to the Alt Mode context
 *
 * @return None
 */
typedef void (*cy_pdaltmode_alt_mode_cbk_t) (void *context);

/**
 * @brief Function is used by Alt Modes manager to run alternate
 * mode analysis of received APP command.
 *
 * @param context Pointer to the Alt Mode context
 * @param app_cmd Received APP command data
 *
 * @return true if APP command passed successful, false if APP command is invalid
 * or contains unacceptable fields.
 */
typedef bool (*cy_pdaltmode_app_cbk_t) (void *context, cy_stc_pdaltmode_alt_mode_evt_t  app_cmd);


/**
 * @brief Function is used by app layer to call MUX polling
 * function.
 *
 * @param context Pointer to the Alt Mode context
 *
 * @return MUX polling status
 */
typedef cy_en_pdaltmode_app_mux_status_t (*cy_pdaltmode_mux_poll_fnc_cbk_t)(void *context);

/**
  @typedef cy_pdaltmode_mtl_sbu_mux_ctrl_cbk_t
  @brief Callback used for MTL SBU MUX control based on the MUX configuration
 */
typedef bool (*cy_pdaltmode_mtl_sbu_mux_ctrl_cbk_t) (void *context,
                        cy_en_pdaltmode_mux_select_t cfg);

/** \} group_pdaltmode_vdm_alt_mode_manager_data_structures */

/** \} group_pdaltmode_vdm_alt_mode_manager */

 /**
* \addtogroup group_pdaltmode_common 
* \{
*/

/**
* \addtogroup group_pdaltmode_common_data_structures
* \{
*/

/**
  @struct cy_stc_pdaltmode_status_t
  @brief Structure holds all necessary information Alt Mode handling
 */
typedef struct
{
    uint8_t vdmTaskEn;                    /**< Flag to indicate if VDM task manager enabled */
    uint8_t discCblPending;               /**< Flag to indicate if cable discovery is pending */
    bool cblDiscIdFinished;               /**< Flag to indicate that cable disc id finished */
    uint8_t altModeTrigMask;              /**< Mask to indicate which Alt Mode should be enabled by EC */
    uint16_t customHpiSvid;               /**< Holds custom Alternate Mode SVID received from HPI */
    bool altModeEntered;                  /**< Flag to indicate if Alternate Modes currently entered */
    bool vdmPrcsFailed;                   /**< Flag to indicate if VDM process failed */
    bool vdmRetryPending;                 /**< Whether VDM retry on timeout is pending */
    uint8_t customHpiHostCapControl;      /**< Holds custom host capabilities register value received from HPI */
    bool cblRstDone;                      /**< Flag to indicate that cable reset was provided */
    bool trigCblRst;                      /**< Flag to trigger cable reset */
    bool isMuxBusy;                       /**< Flag to indicate that mux is switching */
    cy_pdaltmode_mux_poll_fnc_cbk_t muxPollCbk;        /**< Holds pointer to MUX polling function */
    bool usb4Supp;                        /**< Flag to indicate if USB4 is supported */
    bool usb4Active;                      /**< Indicates that USB4 mode was entered */
    uint8_t usb4DataRstCnt;               /**< Indicates number of Data Reset retries */
    bool usb2Supp;                        /**< USB2 supported flag for Ridge related applications */
    bool usb3Supp;                        /**< USB3 supported flag for Ridge related applications */
    bool skipMuxConfig;                   /**< Flag to indicate do not configure MUX */
    bool trigTbtVdoDisc;                  /**< Flag to indicate that TBT cable DISC MOde is in progress */
    bool tbtCblEntered;                   /**< Flag to indicate that TBT cable was entered TBT mode */
    cy_en_pdaltmode_app_mux_status_t muxStat;              /**< Indicates current MUX status */
    cy_pd_pd_do_t tbtCblVdo;              /**< Holds TBT cable VDO */
    cy_pd_pd_do_t dpCblVdo;               /**< Holds DP cable VDO */
    cy_pd_pd_do_t tbtModeVdo;             /**< Holds TBT mode VDO */
    uint8_t amdDataResetUsb4Mode;         /**< Holds current APU mux state during AMD DataReset flow */
    cy_pd_pd_do_t eudo;                   /**< Holds EUDO */
} cy_stc_pdaltmode_status_t;

/** \} group_pdaltmode_common_data_structures */

/** \} group_pdaltmode_common */

 /**
* \addtogroup group_pdaltmode_vdm_alt_mode_manager 
* \{
*/

/**
* \addtogroup group_pdaltmode_vdm_alt_mode_manager_data_structures
* \{
*/

/**
 * @struct cy_stc_pdaltmode_mngr_info_t
 * @brief Structure holds all necessary information for interaction between
 * Alt Modes manager and selected Alternate Mode.
 */
typedef struct
{
    cy_en_pdaltmode_state_t modeState;          /**< Alternate Mode state */
    cy_en_pdaltmode_state_t sopState[CY_PD_SOP_DPRIME + 1u]; /**< VDM state for SOP/SOP'/SOP" packets */
    uint8_t vdoMaxNumb;                         /**< Maximum number of VDO that Alt Mode can handle */
    uint8_t objPos;                             /**< Alternate Mode object position */
    uint8_t cblObjPos;                          /**< Cable object position */
    uint8_t altModeId;                          /**< Alternate mode ID */
    cy_pd_pd_do_t vdmHeader;                    /**< Buffer to hold VDM header */
    cy_pd_pd_do_t* vdo[CY_PD_SOP_DPRIME + 1u];  /**< Pointers array to Alt Mode VDO buffers */
    uint8_t vdoNumb[CY_PD_SOP_DPRIME + 1u];     /**< Current number of VDOs used for processing in VDO buffers */
    cy_pdaltmode_alt_mode_cbk_t cbk;            /**< Alternate Mode callback function */
    bool isActive;                              /**< Active mode flag */
    bool customAttObjPos;                       /**< Object position field in Att VDM used by Alt Mode as custom */
    bool uvdmSupp;                              /**< Flag to indicate if Alt Mode support unstructured VDMs */
    bool setMuxIsolate;                         /**< Flag to indicate if MUX should be set to safe state while
                                                     ENTER/EXIT Alt Mode is being processed */
    bool amSafeReq;                             /**< Flag to indicate that Safe state should be while entering AM */
    /* Application control information */
    bool appEvtNeeded;                            /**< APP event flag */
    cy_stc_pdaltmode_alt_mode_evt_t appEvtData;   /**< APP event data */
    cy_pdaltmode_app_cbk_t evalAppCmd;            /**< APP command cbk */
} cy_stc_pdaltmode_mngr_info_t;


/**
 * @typedef cy_pdaltmode_reg_alt_modes_cbk_t
 * @brief Alternate Mode handler type
 */
typedef cy_stc_pdaltmode_mngr_info_t*
(*cy_pdaltmode_reg_alt_modes_cbk_t)(
        void *context,
        cy_stc_pdaltmode_alt_mode_reg_info_t* regInfo);

/**
  @typedef cy_pdaltmode_hw_cmd_cbk_t
  @brief Callback type used for notifications about Alt Mode hardware Commands
 */
typedef void (*cy_pdaltmode_hw_cmd_cbk_t) (void *context, uint32_t command);

/**
 * @struct cy_stc_pdaltmode_reg_am_t
 * @brief Structure to hold the Alternate Modes SVID and handler
 */
typedef struct
{
    uint16_t svid;                                 /**< Alternate Mode SVID */
    cy_pdaltmode_reg_alt_modes_cbk_t regAmPtr;     /**< Alternate Mode SVID handler */
} cy_stc_pdaltmode_reg_am_t;

/**
 * @struct cy_stc_pdaltmode_alt_mode_mngr_status_t
 * @brief Struct to hold Alt Modes manager status
 */
typedef struct
{
    /** Holds info when register Alt Mode */
    cy_stc_pdaltmode_alt_mode_reg_info_t   regInfo;
    /** Supported Alternate Modes */
    uint32_t              amSupportedModes;
    /** Exited Alternate Modes */
    uint32_t              amExitedModes;
    /** Active Alternate Modes */
    uint32_t              amActiveModes;
    /** Pointers to each Alt Mode info structure */
    cy_stc_pdaltmode_mngr_info_t*      altModeInfo[CY_PDALTMODE_MAX_SUPP_ALT_MODES];
    /** Number of existed Alt Modes */
    uint8_t               altModesNumb;
    /** Buffer to hold VDM */
    cy_stc_pdstack_dpm_pd_cmd_buf_t      vdmBuf;
    /** Holds application event data */
    uint32_t              appEvtData[CY_PDALTMODE_EVT_SIZE];
    /** Current Alt Modes manager status */
    cy_en_pdaltmode_mngr_state_t state;
    /** Pointer to vdm_msg_info_t struct in vdm task manager */
    cy_stc_pdaltmode_vdm_msg_info_t       *vdmInfo;
    /** Hold current SVID index for Discovery mode command */
    uint8_t               svidIdx;
    /** Check whether the device is a PD 3.0 supporting UFP */
    bool                  pd3Ufp;
    /** Exit all Alt Modes procedure callback */
    cy_pdstack_pd_cbk_t   exitAllCbk;
    /** Flag to indicate that exit all Alt Modes is required */
    bool                  exitAllFlag;
    /** Modes which requires datapath update */
    uint32_t              amDatapathChangeModes;
    /** Flag to indicate is=f custom Alt Mode reset is required */
    bool                  resetCustomMode;
    /** Holds unprocessed attention VDM header */
    cy_pd_pd_do_t               attHeader;
    /** Holds unprocessed attention VDO */
    cy_pd_pd_do_t               attVdo;
    /** Holds pointer to Alt Mode which saved attention is related to */
    cy_stc_pdaltmode_mngr_info_t      *attAltMode;
    /** Flag which indicates that saved attention have to be run */
    uint8_t                       runAttFlag;
    
}cy_stc_pdaltmode_alt_mode_mngr_status_t;

/** \} group_pdaltmode_vdm_alt_mode_manager_data_structures */

/** \} group_pdaltmode_vdm_alt_mode_manager */

 /**
* \addtogroup group_pdaltmode_display_port 
* \{
*/

/**
* \addtogroup group_pdaltmode_display_port_data_structures
* \{
*/

/**
 * @struct cy_stc_pdaltmode_dp_status_t
 * @brief DP Status
 */
typedef struct
{
    cy_stc_pdaltmode_mngr_info_t info;                   /**< Alt Mode manager info  */
    cy_pd_pd_do_t vdo[CY_PDALTMODE_MAX_DP_VDO_NUMB];     /**< VDO */
    cy_en_pdaltmode_dp_state_t state;                    /**< DP state */
    cy_en_pdaltmode_mux_select_t dpMuxCfg;               /**< DP MUX configuration */
    cy_pd_pd_do_t configVdo;                             /**< Config VDO */
    bool dpActiveFlag;                                   /**< DP active flag */
    uint8_t ccgDpPinsSupp;                               /**< CCG's DP pin supported mask*/
    uint8_t partnerDpPinsSupp;                           /**< Port partner's DP pin supported mask*/
    bool dp2LaneActive;                                  /**< DP2 lane active flag */
    uint16_t hpdState;                                   /**< HPD State */
    uint8_t queueReadIndex;                              /**< HPD queue read index */
    uint8_t dp21Supp;                                    /**< DP2.1 support indication flag */
    uint8_t curHpdState;                                 /**< Current HPD state */
    cy_pd_pd_do_t statusVdo;                             /**< Status VDO */
    cy_pd_pd_do_t sinkVdo;                               /**< Disc Mode VDO */
#if CY_PD_DP_VCONN_SWAP_FEATURE
    uint8_t vconnSwapReq;                                /**< VCONN Swap request */
    uint8_t vconnInitRetryCnt;                           /**< VCONN init retry count*/
    uint8_t vconnCbkRetryCnt;                            /**< VCONN retry count callback */
#endif /* CY_PD_DP_VCONN_SWAP_FEATURE */
#if DP_DFP_SUPP
    bool dpExit;                                         /**< DP exit */
    uint8_t dp4Lane;                                     /**< DP4 lane */
    uint8_t dp2Lane;                                     /**< DP2 lane */
    uint8_t dpCfgCtrl;                                   /**< DP configuration control */
    uint8_t maxSopSupp;                                  /**< Max SOP supported */
    uint8_t cableConfigSupp;                             /**< Cable configuration support */
    const cy_stc_pdaltmode_atch_tgt_info_t* tgtInfoPtr;  /**< Attached target info pointer */
    bool dpActCblSupp;                                   /**< DP active cable support */
    cy_pd_pd_do_t cableVdo[CY_PDALTMODE_MAX_DP_VDO_NUMB];  /**< Cable VDO */
#if MUX_UPDATE_PAUSE_FSM
    cy_en_pdaltmode_dp_state_t prevState;                  /**< DP previous state */
#endif /* MUX_UPDATE_PAUSE_FSM */
#endif /* DP_DFP_SUPP */

}cy_stc_pdaltmode_dp_status_t;

/** \} group_pdaltmode_display_port_data_structures */

/** \} group_pdaltmode_display_port */

 /**
* \addtogroup group_pdaltmode_alt_mode_hw 
* \{
*/

/**
* \addtogroup group_pdaltmode_alt_mode_hw_data_structures
* \{
*/

/**
 * @struct cy_stc_pdaltmode_hw_details_t
 * @brief Alt Mode HW details
 */
typedef struct
{
    cy_pdaltmode_hw_cmd_cbk_t hwCmdCbk;                    /**< Holds command callback information */
    volatile cy_en_usbpd_hpd_events_t altModeHpdState;     /**< Holds current HPD pin status */
    cy_en_pdaltmode_mux_select_t appMuxState;              /**< App MUX state */
    uint32_t hwSlnData;                                    /**< Holds HW solution event/command data */
    uint32_t appMuxSavedCustomData;                        /**< Custom data parameter saved for future MUX update */
    uint32_t appMuxSavedModeVdo;                           /**< Alt Mode VDO saved for future MUX update */
    cy_en_pdaltmode_mux_select_t appMuxSavedState;         /**< MUX state saved for future MUX update */
    bool appMuxUpdateReq;                                  /**< Flag to indicate that MUX update is required */
    volatile uint8_t muxCurState;                          /**< Store the current MUX config */
    volatile bool altModeCmdPending;                       /**< Holds current HPD command status */
}cy_stc_pdaltmode_hw_details_t;

/** \} group_pdaltmode_alt_mode_hw_data_structures */

/** \} group_pdaltmode_alt_mode_hw */

 /**
* \addtogroup group_pdaltmode_billboard 
* \{
*/

/**
* \addtogroup group_pdaltmode_billboard_data_structures
* \{
*/

/**
 * @brief Internal Billboard module handle structure
 *
 * No explicit structure for the handle is expected to be created outside of
 * the Billboard module implementation.
 */
typedef struct cy_stc_pdaltmode_bb_handle
{
    /* Common Billboard fields */
    cy_en_pdaltmode_bb_type_t type;                     /**< Billboard implementation type */
    cy_en_pdaltmode_bb_state_t state;                   /**< Billboard current state */
    uint8_t numAltModes;              /**< Number of valid Alternate Modes */
    uint8_t bbAddInfo;                /**< AdditionalFailureInfo field in Billboard
                                             capability descriptor. */
    uint32_t altStatus;                /**< Current Alternate Mode status -
                                             The status can hold a maximum of 16
                                             Alternate Modes (2 bits each). */
    uint32_t timeout;                   /**< Pending timeout count in ms for Billboard
                                             interface disable. */

    /* Internal Billboard fields. */
    uint8_t *ep0Buffer;                /**< EP0 data buffer pointer in case of
                                             internal Billboard implementation. */
    bool usbConfigured;                /**< Internal flag indicating whether the
                                             configuration is enabled or not. This is valid
                                             only for internal Billboard implementation. */
    uint8_t usbPort;                   /**< USB port index used in case the device
                                             has multiple USB ports. */
    bool flashingMode;                 /**< USB flashing mode state. Valid only for
                                             internal Billboard implementation. */
    bool usbI2cmMode;                 /**< USB I2C Master bridge mode state. Valid only
                                             for internal Billboard implementation
                                             supporting I2C Master bridge interface. */
    bool queueEnable;                  /**< Billboard interface enable request
                                             queued. Valid only for internal
                                             Billboard implementation. */
    bool queueDisable;                 /**< Billboard interface disable request
                                             queued. Valid only for internal
                                             Billboard implementation. */
    bool queueI2cmEnable;             /**< USB-I2C Master mode enable request.
                                             Valid only for internal Billboard
                                             implementation. */

    cy_pdaltmode_bb_bos_decr_update_cbk_t updateBosDescrCbk;     /**< Callback for updating BOS descriptor */

} cy_stc_pdaltmode_bb_handle_t;

/** \} group_pdaltmode_billboard_data_structures */

/** \} group_pdaltmode_billboard */

 /**
* \addtogroup group_pdaltmode_common 
* \{
*/

/**
* \addtogroup group_pdaltmode_common_data_structures
* \{
*/

/**
 * @struct cy_stc_pdaltmode_context_t
 * @brief Structure to PDStack Alt Mode Middleware context information
 */
typedef struct cy_stc_pdaltmode_context
{
    cy_stc_pdstack_context_t *                      pdStackContext;                     /**< PD Stack Library context Pointer */
    cy_stc_pdaltmode_status_t*                      altModeAppStatus;                   /**< Alt mode Layer status  */
    cy_stc_pdstack_app_status_t*                    appStatusContext;                   /**< Application status  */
    cy_stc_pdaltmode_alt_mode_mngr_status_t *       altModeMngrStatus;                  /**< Alt Mode Manager Status */
    cy_stc_pdaltmode_vdm_task_status_t*             vdmStat;                            /**< VDM Task Manager Status */
    cy_stc_pdaltmode_hw_details_t*                  hwDetails;                          /**< Alt Mode HW details */
    cy_stc_pdaltmode_dp_status_t*                   dpStatus;                           /**< DP Status */
    cy_stc_pdaltmode_host_details_t*                hostDetails;                        /**< Host Details */
    cy_pdaltmode_mtl_sbu_mux_ctrl_cbk_t             mtlMuxCtrlCbk;                      /**< MTL SBU MUX control */
    volatile cy_stc_pdaltmode_bb_handle_t           billboard;                          /**< Billboard interface internal handle structure */
    uint16_t                                        altModeCfgLen;                      /**< Length of the Alt Mode Config table */
    uint8_t                                         dfpAltModeMask;                     /**< Mask to enable DFP Alternate Modes */
    uint8_t                                         ufpAltModeMask;                     /**< Mask to enable UFP Alternate Modes */
    uint16_t                                        baseAmInfo[CY_PD_MAX_NO_OF_DO * 2]; /**< Stores base AM configuration info */
    uint32_t                                        vdmDiscIdResp[CY_PDALTMODE_MAX_DISC_ID_RESP_LEN];
                                                                                        /**< Array to store the D_ID response received from the port partner */
    uint8_t                                         vdmDiscIdRespLen;                   /**< D_ID response length*/
    uint32_t                                        vdmDiscSvidResp[CY_PDALTMODE_MAX_DISC_SVID_RESP_LEN];
                                                                                        /**< Array to store the D_SVID response received from the port partner */
    uint8_t                                         vdmDiscSvidRespLen;                 /**< D_SVID response length*/
    const cy_stc_pdaltmode_cfg_settings_t *         altModeCfg;                         /**< Alt Mode config table */
    const cy_stc_pdaltmode_custom_alt_cfg_settings_t*   custAltModeCfg;                 /**< Custom Alt Mode config table */
    const tbthost_cfg_settings_t *                  tbtCfg;                             /**< TBT config table*/
    const cy_stc_pdaltmode_dp_cfg_settings_t *      dpCfg;                              /**< DP  config table */
    const intel_soc_cfg_settings_t *                iclCfg;                             /**< ICL config table */
    const amd_cfg_settings_t *                      amdCfg;                             /**< AMD config table */
    const cy_stc_pdaltmode_ridge_hw_config_t*       ridgeHwConfig;                      /**< Ridge communication configurations */
    struct cy_stc_pdaltmode_context*                ptrAltPortAltmodeCtx;               /**< Alternate Port PD Alternate Mode context */
    uint8_t                                         noOfTypeCPorts;                     /**< No of Type-C ports available in the controller */
} cy_stc_pdaltmode_context_t;

/** \} group_pdaltmode_common_data_structures */

/** \} group_pdaltmode_common */

#endif /* _CY_PDALTMODE_DEFINES_H_ */

/* [] END OF FILE */
