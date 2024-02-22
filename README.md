# PDAltMode Middleware Library 1.0

### Overview

The PDAltMode Middleware implements state machines defined in:
1. Universal Serial Bus Power Delivery Specification Rev 3.1 Ver 1.8.
2. Universal Serial Bus Type-C Cable and Connector Specification Ver 2.2.
3. VESA DisplayPort Alt Mode on USB Type-C Standard. Ver 1.3/1.4.

The PDAltMode Middleware operates on top of PdStack Middleware and USBPD driver included in the MTB PDL CAT2(mtb-pdl-cat2) 
Peripheral Driver Library. The middleware provides a set of Alt Mode APIs through 
which the application can initialize, monitor and configure the following PD Alt Modes:
1. Display Port.
2. TBT.
3. vPro.
4. USB4.

The PDAltMode Middleware is released as a combination of source files and a pre-compiled library (pdaltmode_dock_tbt). 
The pre-compiled library implements support for TBT and vPro alternate modes. All of the remaining state machines are
implemented in the form of source files.

### Features
1) Support PD Alternate Modes Discovery, entry and simultaneously handling up to 4 alternate modes.
2) Supports the following PD AltModes (in DFP and UFP roles) by default:
    - DP
    - TBT
    - vPro
    - USB4
3) Supports Intel Ridge slave interface.
4) Support mechanisms to add user custom alternate mode.

## Quick Start

Refer to the [API Reference Guide Quick Start Guide](https://infineon.github.io/pdaltmode/html/index.html#section_pdaltmode_quick_start) section for step-by-step instructions on how to
enable the PdAltMode Middleware Library.

## Related resources

Resources    | Links
-------------|------------------------------------------------------------------
Libraries on GitHub | [mtb-pdl-cat2](https://github.com/Infineon/mtb-pdl-cat2) – Peripheral driver library (PDL) and docs
Middleware on GitHub | [pdstack](https://github.com/Infineon/pdstack) – PDStack middleware library and docs <br>[pdutils](https://github.com/Infineon/pdutils) – PDUtils middleware library and docs <br>[hpi](https://github.com/Infineon/hpi) – HPI middleware library and docs
Tools | [Eclipse IDE for ModusToolbox&trade; software](https://www.infineon.com/modustoolbox) <br> ModusToolbox&trade; software is a collection of easy-to-use software and tools enabling rapid development with Infineon MCUs, covering applications from embedded sense and control to wireless and cloud-connected systems using AIROC™ Wi-Fi & Bluetooth® combo devices.

### More information
The following resources contain more information:
* [PDAltMode middleware RELEASE.md](./RELEASE.md)
* [PDAltMode Middleware API Reference Guide](https://infineon.github.io/pdaltmode/html/index.html)
* [ModusToolbox Software Environment, Quick Start Guide, Documentation, and Videos](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software)
* [Infineon Technologies AG](https://www.infineon.com)
 
---
© 2024, Cypress Semiconductor Corporation (an Infineon company) or an affiliate of Cypress Semiconductor Corporation.
