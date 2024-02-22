/***************************************************************************//**
* \file cy_pdaltmode_billboard.c
* \version 1.0
*
* Billboard interface implementation.
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
#include "cy_pdaltmode_billboard.h"

#include "cy_usbpd_hpd.h"
#include "cy_pdaltmode_mngr.h"

#if CCG_BB_ENABLE

cy_en_pdaltmode_billboard_status_t Cy_PdAltMode_Billboard_Enable(cy_stc_pdaltmode_context_t *ptrAltModeContext, cy_en_pdaltmode_billboard_cause_t cause)
{
    /* Queue an enable only if the block is initialized and is not in flashing mode */
    if ((ptrAltModeContext->billboard.state == CY_PDALTMODE_BB_STATE_DEINITED) ||
            (ptrAltModeContext->billboard.state == CY_PDALTMODE_BB_STATE_LOCKED))
    {
        return CY_PDALTMODE_BILLBOARD_STAT_NOT_READY;
    }

    /* Update additional failure information data */
    switch (cause)
    {
        case CY_PDALTMODE_BILLBOARD_CAUSE_AME_TIMEOUT:
        case CY_PDALTMODE_BILLBOARD_CAUSE_AME_FAILURE:
            ptrAltModeContext->billboard.bbAddInfo = CY_PDALTMODE_BILLBOARD_CAP_ADD_FAILURE_INFO_PD;
            break;

        case CY_PDALTMODE_BILLBOARD_CAUSE_PWR_FAILURE:
            ptrAltModeContext->billboard.bbAddInfo = CY_PDALTMODE_BILLBOARD_CAP_ADD_FAILURE_INFO_PWR;
            break;

        default:
            ptrAltModeContext->billboard.bbAddInfo = 0;
            break;
    }

    /* Update BOS descriptor with Alternate Mode status */
    ptrAltModeContext->billboard.updateBosDescrCbk(ptrAltModeContext);

    /* Load the information */
    ptrAltModeContext->billboard.queueEnable = true;

    return CY_PDALTMODE_BILLBOARD_STAT_SUCCESS;
}


cy_en_pdaltmode_billboard_status_t Cy_PdAltMode_Billboard_AltStatusUpdate(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t modeIndex,
        cy_en_pdaltmode_billboard_alt_mode_status_t altStatus)
{
    uint32_t status;

    /* Allow update of Alternate Mode only if initialized */
    if (ptrAltModeContext->billboard.state == CY_PDALTMODE_BB_STATE_DEINITED)
    {
        return CY_PDALTMODE_BILLBOARD_STAT_NOT_READY;
    }

    if (modeIndex >= CY_PDALTMODE_BILLBOARD_MAX_ALT_MODES)
    {
        return CY_PDALTMODE_BILLBOARD_STAT_INVALID_ARGUMENT;
    }

    status = (ptrAltModeContext->billboard.altStatus & ~(CY_PDALTMODE_BILLBOARD_ALT_MODE_STATUS_MASK << (modeIndex << 1)));
    status |= (altStatus << (modeIndex << 1));
    ptrAltModeContext->billboard.altStatus = status;

    return CY_PDALTMODE_BILLBOARD_STAT_SUCCESS;
}

bool Cy_PdAltMode_Billboard_IsPresent(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    if(ptrAltModeContext->billboard.state == CY_PDALTMODE_BB_STATE_DEINITED)
    {
        return false;
    }

    return true;
}


cy_en_pdaltmode_billboard_status_t Cy_PdAltMode_Billboard_Disable(cy_stc_pdaltmode_context_t *ptrAltModeContext, bool force)
{
    cy_en_pdaltmode_billboard_status_t status = CY_PDALTMODE_BILLBOARD_STAT_SUCCESS;

    /* Allow disable during flashing only if force parameter is true */
    if (((force != false) && (ptrAltModeContext->billboard.state <= CY_PDALTMODE_BB_STATE_DISABLED)) ||
            ((force == false) && (ptrAltModeContext->billboard.state != CY_PDALTMODE_BB_STATE_BILLBOARD))
       )
    {
        return CY_PDALTMODE_BILLBOARD_STAT_NOT_READY;
    }

    /* Clear any pending enable request and then queue a disable request */
    ptrAltModeContext->billboard.queueEnable = false;
    ptrAltModeContext->billboard.queueDisable = true;
#if (APP_I2CM_BRIDGE_ENABLE)
    ptrAltModeContext->billboard.queueI2cmEnable = false;
#endif /* (APP_I2CM_BRIDGE_ENABLE) */

    return status;
}

cy_en_pdaltmode_billboard_status_t Cy_PdAltMode_Billboard_UpdateAllStatus(cy_stc_pdaltmode_context_t *ptrAltModeContext, uint32_t status)
{
    uint32_t state;

    /* Allow update of Alternate Mode only if initialized */
    if (ptrAltModeContext->billboard.state == CY_PDALTMODE_BB_STATE_DEINITED)
    {
        return CY_PDALTMODE_BILLBOARD_STAT_NOT_READY;
    }

    state = Cy_SysLib_EnterCriticalSection();
    /*
     * Mask out the unused Alternate Modes. The expression does the following:
     * 2 ^ (2 * number of Alternate Modes) - 1. This is the mask for
     * value status information (two bits per Alternate Mode).
     */
    ptrAltModeContext->billboard.altStatus = ((1 << (ptrAltModeContext->billboard.numAltModes << 1)) - 1) & status;

    Cy_SysLib_ExitCriticalSection(state);

    return CY_PDALTMODE_BILLBOARD_STAT_SUCCESS;
}
#endif /* CCG_BB_ENABLE */

/* [] END OF FILE */
