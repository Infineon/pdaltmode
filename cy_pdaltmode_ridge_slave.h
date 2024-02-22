/***************************************************************************//**
* \file cy_pdaltmode_ridge_slave.h
* \version 1.0
*
* \brief
* This header file defines the data structures and function prototypes associated
* with the Intel Alpine/Titan Ridge control interface.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_PDALTMODE_RIDGE_SLAVE_H_
#define _CY_PDALTMODE_RIDGE_SLAVE_H_

/*****************************************************************************
 * Header file addition
 *****************************************************************************/

#include "cy_pdaltmode_defines.h"
#include <stdbool.h>
#include <stdint.h>

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

#define CY_PDALTMODE_RIDGESLAVE_SCB_INDEX                      (0x01u)
/**< Default SCB index used for the Ridge/SoC Slave interface */

#define CY_PDALTMODE_RIDGESLAVE_SCB_CLOCK_FREQ                 (I2C_SCB_CLOCK_FREQ_1_MHZ)
/**< Maximum I2C clock frequency used on the Ridge/SoC slave interface */

#define CY_PDALTMODE_RIDGESLAVE_MIN_WRITE_SIZE                 (2u)
/**< Minimum slave write size: Corresponds to slave address + register address */

#define CY_PDALTMODE_RIDGESLAVE_MAX_WRITE_SIZE                 (16u)
/**< Maximum slave write size: We should never get writes longer than 16 bytes */

#define CY_PDALTMODE_RIDGESLAVE_MAX_READ_SIZE                  (20u)
/**< Maximum slave read size */
    
#define CY_PDALTMODE_RIDGESLAVE_READ_BUFFER_SIZE               (4u)
/**< Ridge slave buffer read size */

#if ICL_SLAVE_ENABLE

#define CY_PDALTMODE_RIDGESLAVE_PMC_SLAVE_ADDR_PORT0                            (0x50u)
/**< Port 0 slave default slave address */

#define CY_PDALTMODE_RIDGESLAVE_PMC_SLAVE_ADDR_PORT1                            (0x51u)
/**< Port 1 slave default slave address */
    
#define CY_PDALTMODE_RIDGESLAVE_ICL_SOC_ACK_TIMEOUT_PERIOD                      (500u)
/**< t_SOCAckTimeout period defined in PD controller specs from Intel */

#define CY_PDALTMODE_RIDGESLAVE_ADDR_MASK_GENERAL                   (0x80u)
/**< @brief I2C slave address mask to be applied on the incoming slave address */

#else    

#define CY_PDALTMODE_RIDGESLAVE_ADDR_P0                        (0x38u)
/**< Slave address associated with USB PD port number 0. This is defined by Intel */

#define CY_PDALTMODE_RIDGESLAVE_ADDR_P1                        (0x3Fu)
/**< Slave address associated with USB PD port number 1. This is defined by Intel */

#define CY_PDALTMODE_RIDGESLAVE_ADDR_MASK                      (0xF0u)
/**< @brief I2C slave address mask to be applied on the incoming slave address */
    
#endif /* ICL_SLAVE_ENABLE */

#define CY_PDALTMODE_RIDGESLAVE_RIDGE_CMD_CCG_RESET                        (0x02u)
/**< Alpine/Titan Ridge command value that requests a CCG device reset */

#define CY_PDALTMODE_RIDGESLAVE_RIDGE_CMD_INT_CLEAR                        (0x04u)
/**< Alpine/Titan Ridge command value to clear the interrupt from CCG */

#define CY_PDALTMODE_RIDGESLAVE_RIDGE_IRQ_ACK                              (0x2000u)
/**< Alpine/Titan Ridge command IRQ ACK PD Controller to Titan Ridge */

#if BB_RETIMER_ENABLE
    
#define CY_PDALTMODE_RIDGESLAVE_RIDGE_CMD_WRITE_TO_RETIMER                 (0x10u)
/**< Ice Lake command value to request to write to the Retimer */    
    
#define CY_PDALTMODE_RIDGESLAVE_RIDGE_STS_RETIMER_DATA_VALID               (0x400000u)
/**< Ice Lake status bit to indicate that Retimer data is valid */
    
#endif /* BB_RETIMER_ENABLE */    

/** \} group_pdaltmode_intel_macros */

/**
* \addtogroup group_pdaltmode_intel_enums
* \{
*/


/** \} group_pdaltmode_intel_enums */

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/

/**
* \addtogroup group_pdaltmode_intel_functions
* \{
*/

/*******************************************************************************
* Function mame: Cy_PdAltMode_RidgeSlave_I2cCmdCbk
****************************************************************************//**
*
* I2C command callback function that implements the actual Alpine/Titan Ridge
* interface logic. This is called from SCB interrupt handler. Since the work
* to be done is limited, it is completely handled from the callback.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param count
* Count of bytes written to I2C buffer
*
* \param writeBuf
* Pointer to I2C Write buffer
*
* \param readBuf
* Pointer to I2C Read buffer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_RidgeSlave_I2cCmdCbk(cy_stc_pdaltmode_context_t* ptrAltModeContext, uint32_t count,
        uint8_t *writeBuf, uint8_t *readBuf);


/*******************************************************************************
* Function name: Cy_PdAltMode_RidgeSlave_StatusUpdate
****************************************************************************//**
*
* Update the AR/TR status register and send an event to the Alpine/Titan Ridge.
*
* This function is used by the application layer to update the content of the Alpine/Titan
* Ridge status register. If the content of the register is changing, CCG asserts
* the corresponding interrupt to notify Alpine/Titan Ridge about the status change.
*
* \param ptrAltModeContext
* PdAltMode Library Context pointer
*
* \param status
* Value to be written into the status register
*
* \param rewrite
* Flag to enable updating of the status register even if it remains the same
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_RidgeSlave_StatusUpdate(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint32_t status, bool rewrite);

/*******************************************************************************
* Function name: Cy_PdAltMode_RidgeSlave_DataReset
****************************************************************************//**
*
* Clean the AR/TR status register the Alpine/Titan Ridge
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_RidgeSlave_DataReset(cy_stc_pdaltmode_context_t* ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_RidgeSlave_Task
****************************************************************************//**
*
* Handler for pending Ridge Slave interface tasks
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \param writeBuf
* Pointer to the write buffer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_RidgeSlave_Task(cy_stc_pdaltmode_context_t *ptrAltModeContext,  uint8_t *writeBuf);

#if ICL_SLAVE_ENABLE
/*******************************************************************************
* Function name: Cy_PdAltMode_RidgeSlave_UpdateIsPending
****************************************************************************//**
*
* Checks if any PMC/Ridge update is pending
*
* \return
* Mask of updates pending
*
*******************************************************************************/
uint8_t Cy_PdAltMode_RidgeSlave_UpdateIsPending(void);

#endif /* ICL_SLAVE_ENABLE */

/*******************************************************************************
* Function name: Cy_PdAltMode_RidgeSlave_IsHostConnected
****************************************************************************//**
*
* Check whether a host is connected to the specified PD port
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* True if a host is connected, false otherwise.
*
*******************************************************************************/
bool Cy_PdAltMode_RidgeSlave_IsHostConnected(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_RidgeSlave_SetOcpStatus
****************************************************************************//**
*
* Update the data status register to indicate over current fault condition.
*
* \param ptrAltModeContext
* PD Alt Mode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_RidgeSlave_SetOcpStatus (cy_stc_pdaltmode_context_t *ptrAltModeContext);

#if ICL_SLAVE_ENABLE
/*******************************************************************************
* Function name: Cy_PdAltMode_RidgeSlave_SocTimeoutEventControl
****************************************************************************//**
*
* Enable or disable HPI notifications about SoC ACK timeout condition. This API is only valid
* on Ice Lake/Tiger Lake platforms and will be a NOP otherwise.
*
* \param disable
* Whether HPI notification on SoC ACK timeout should be disabled
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_RidgeSlave_SocTimeoutEventControl(bool disable);

#endif /* ICL_SLAVE_ENABLE */

/*******************************************************************************
* Function name: Cy_PdAltMode_RidgeSlave_ResetVirtualHpd
****************************************************************************//**
*
* Resets virtual HPD related variables
*
* \param ptrAltModeContext
* PdAltMode Library Context pointer
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_RidgeSlave_ResetVirtualHpd(cy_stc_pdaltmode_context_t *ptrAltModeContext);

#if BB_RETIMER_ENABLE
/*******************************************************************************
* Function name: Cy_PdAltMode_RidgeSlave_ClearWriteToRetimerBit
****************************************************************************//**
*
*  Clear the Write_To_Retimer bit in the ridge slave cmd register.
*
* \param ptrAltModeContext
*  PdAltMode Library Context pointer.
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_RidgeSlave_ClearWriteToRetimerBit(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/*******************************************************************************
* Function name: Cy_PdAltMode_RidgeSlave_DelayedSocAlert
****************************************************************************//**
*
* Enable/disable delayed SoC alter notification to the TBT controller.
*
* \param enable
* Whether notifications should be delayed
*
* \return
* None
*
*******************************************************************************/
void Cy_PdAltMode_RidgeSlave_DelayedSocAlert(bool enable);

#endif /* BB_RETIMER_ENABLE */

#if VPRO_WITH_USB4_MODE
void Cy_PdAltMode_RidgeSlave_VproStatusUpdateEnable(cy_stc_pdaltmode_context_t *ptrAltModeContext, bool vproValue);
#endif

/** \} group_pdaltmode_intel_functions */

/** \} group_pdaltmode_intel */

#endif /* _CY_PDALTMODE_RIDGE_SLAVE_H_ */

/* [] END OF FILE */
