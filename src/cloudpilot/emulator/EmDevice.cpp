/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
        Copyright (c) 1999-2001 Palm, Inc. or its subsidiaries.
        All rights reserved.

        This file is part of the Palm OS Emulator.

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2 of the License, or
        (at your option) any later version.
\* ===================================================================== */

/*
        This file controls the creation of a new device in the Palm OS
        Emulator.  In order to add a new device to the set of supported
        devices:

        * Add a new kDeviceFoo value to DeviceType.

        * Add information about the new device to the kDeviceInfo array.

        * Define and add a sub-class of EmRegs to describe the hardware
          registers for the device (e.g., EmRegsEZPalmV).  This sub-class
          can be a typedef of another sub-class if the hardware is the
          same (e.g., EmRegsEZPalmVx).

        * Define and add any other EmRegs sub-classes as needed if the
          device has any special hardware needs (e.g., EmRegsUSBVisor).

        * Add a new "case" statement in CreateRegs that creates the
          appropriate EmRegs sub-classes.

        * If your needs are extensive and creating a sub-class of EmRegs
          isn't sufficient, then consider creating a new EmBankFoo and
          managing that in EmMemory.cpp along with all the other EmBankFoos.

        * Modify SupportsROM to make sure that your device appears on the
          device list when your ROM is selected, and try to make sure that
          it does not appear for other ROMs.  If ROMs for this device can
          be identified via companyID and halID fields in the card header,
          then you don't need to do anything here.
*/

#include "EmDevice.h"

#include <cstring>

#include "EmBankRegs.h"  // AddSubBank
#include "EmCPU68K.h"
#include "EmCommon.h"
#include "EmHandEraCFBus.h"
#include "EmHandEraSDBus.h"
#include "EmROMReader.h"  // EmROMReader
#include "EmRegs328PalmIII.h"
#include "EmRegs328PalmVII.h"
#include "EmRegs328Pilot.h"
#include "EmRegs330CPLD.h"
#include "EmRegs330CPLDMirror.h"
#include "EmRegsAcerDSPStub.h"
#include "EmRegsAcerUSBStub.h"
#include "EmRegsEZPalmIIIc.h"
#include "EmRegsEZPalmIIIe.h"
#include "EmRegsEZPalmIIIx.h"
#include "EmRegsEZPalmM100.h"
#include "EmRegsEZPalmV.h"
#include "EmRegsEZPalmVII.h"
#include "EmRegsEZPalmVIIx.h"
#include "EmRegsEZPalmVx.h"
#include "EmRegsEzPegS300.h"
#include "EmRegsEzPegS500C.h"
#include "EmRegsFMSound.h"
#include "EmRegsFrameBuffer.h"
#include "EmRegsLCDCtrlT2.h"
#include "EmRegsMB86189.h"
#include "EmRegsMediaQ11xx.h"
#include "EmRegsMediaQ11xxPacifiC.h"
#include "EmRegsPLDAtlantiC.h"
#include "EmRegsPLDPacifiC.h"
#include "EmRegsPLDPalmI705.h"
#include "EmRegsPLDPalmVIIEZ.h"
#include "EmRegsSED1375.h"
#include "EmRegsSED1376.h"
#include "EmRegsSZNaples.h"
#include "EmRegsSZRedwood.h"
#include "EmRegsSonyDSP.h"
#include "EmRegsUSBPhilipsPDIUSBD12.h"
#include "EmRegsUsbCLIE.h"
#include "EmRegsVZAcerS1x.h"
#include "EmRegsVZAtlantiC.h"
#include "EmRegsVZHandEra330.h"
#include "EmRegsVZHandEra330c.h"
#include "EmRegsVZPalmI705.h"
#include "EmRegsVZPalmM125.h"
#include "EmRegsVZPalmM130.h"
#include "EmRegsVZPalmM505.h"
#include "EmRegsVZPegModena.h"
#include "EmRegsVZPegN700C.h"
#include "EmRegsVZPegNasca.h"
#include "EmRegsVZPegVenice.h"
#include "EmRegsVZPegYellowStone.h"
#include "EmStructs.h"
#include "EmTRGCF.h"
#include "MemoryStick.h"
#include "Platform.h"  // _stricmp

// clang-format false
#include "PalmPack.h"
#define NON_PORTABLE
#include "HwrMiscFlags.h"  // hwrMiscFlagIDTouchdown, etc.

#ifndef hwrMiscFlagIDPalmIIIc
    #define hwrMiscFlagIDPalmIIIc (hwrMiscFlagID4 | hwrMiscFlagID1)
#endif

// The m100 ID has changed
#undef hwrMiscFlagIDm100
#define hwrMiscFlagIDm100 (hwrMiscFlagID3 | hwrMiscFlagID1)

#undef NON_PORTABLE
#include "PalmPackPop.h"
// clang-format true

enum  // DeviceType
{
    kDeviceUnspecified = 0,
    kDevicePilot,
    kDevicePalmPilot,
    kDevicePalmIII,
    kDevicePalmIIIc,
    kDevicePalmIIIe,
    kDevicePalmIIIx,
    kDevicePalmIIIxe,
    kDevicePalmV,
    kDevicePalmVx,
    kDevicePalmVII,
    kDevicePalmVIIEZ,
    kDevicePalmVIIx,
    kDevicePalmM100,
    kDevicePalmM105,
    kDevicePalmM125,
    kDevicePalmM130,
    kDevicePalmM500,
    kDevicePalmM505,
    kDevicePalmM515,
    kDevicePalmM520,
    kDevicePalmI705,
    kDevicePalmI710,
    kDevicePalmPacifiC,
    kDeviceARMRef,
    kDeviceSymbol1500,
    kDeviceSymbol1700,
    kDeviceSymbol1740,
    kDeviceTRGpro,
    kDeviceHandEra330,
    kDeviceHandEra330c,
    kDeviceVisor,
    kDeviceVisorPrism,
    kDeviceVisorPlatinum,
    kDeviceVisorEdge,
    kDevicePEGS300,
    kDevicePEGS320,
    kDevicePEGT400,
    kDevicePEGN600C,
    kDevicePEGT600,
    kDevicePEGN700C,
    kDeviceYSX1230,
    kDeviceYSX1100,
    kDevicePEGS500C,
    kDeviceAcerS11,
    kDeviceAcerS65,
    kDeviceLast
};

struct DeviceInfo {
    int fDeviceID;

    const char* fDeviceMenuName;  // This string appears in the Device menu.

    const char* fDeviceNames[8];  // These strings can be specified on the command line
                                  // to specify that this device be emulated.  The first
                                  // string in the array is also the "preferred" string,
                                  // in that it is used to identify the device in the
                                  // Preferences and Session files.

    uint32 fFlags;  // One or more of the flags below.

    RAMSizeType fMinRAMSize;  // Minimum RAM size (in K).  Used to prune back the
                              // RAM size menu.

    long fHardwareID;  // Some devices have a hardware ID they report via
                       // hardware register munging.

    long fHardwareSubID;  // Some devices have a hardware sub-ID they report via
                          // hardware registr munging.

    struct {
        uint32 fCompanyID;  // The "companyID" field in a v5 CardHeader.  This field
                            // is used to identify which ROMs can run on which devices.
                            // Set it to zero if the ROM for your device doesn't have
                            // a v5 CardHeader.

        uint32 fHalID;  // The "halID" field in a v5 CardHeader.  This field
                        // is used to identify which ROMs can run on which devices.
                        // Set it to zero if the ROM for your device doesn't have
                        // a v5 CardHeader.

    } fCardHeaderInfo[1];  // It's possible that in the future, more than one HAL
                           // could properly execute on this device.  Make this an
                           // array so that we can add more than one HAL ID.
};

// (None of these values are (is?) exposed, so they can be freely reordered.)

static const uint32 kSupports68328 = 0x00000001;
static const uint32 kSupports68EZ328 = 0x00000002;
static const uint32 kSupports68VZ328 = 0x00000004;
static const uint32 kSupports68SZ328 = 0x00000008;

static const uint32 kHasFlash = 0x00000100;

static const long hwrMiscFlagIDNone = 0xFF;
static const long hwrMiscFlagExtSubIDNone = 0xFF;

#define UNSUPPORTED 0

/* Miscellaneous symbols used to discriminate between ROMs. Do not assume that
   these are meant to convey actual ownership or documented interfaces. */

#define PALM_MANUF "Palm Computing"
#define HANDSPRING_MANUF "Handspring, Inc."
#define SYMBOL_DB "ScanMgr"
#define SYMBOL_1500_DB "ResetApp"      // But not available in 3.5 version of 1500 ROM
#define SYMBOL_1700_DB "ResetAppScan"  // But now also available in 3.5 version of 1500 ROM
#define SYMBOL_1740_DB "S24PowerDown"  // Only on 1740s, which have the Spectrum 24 wireless
#define PALM_VII_DB "WMessaging"
#define PALM_VII_IF1_DB "RAM NetIF"
#define PALM_VII_IF2_DB "RAM NetIFRM"
#define PALM_VII_EZ_NEW_DB "TuneUp"
#define PALM_VIIX_DB "iMessenger"
#define PALM_m100_DB "cclkPalmClock"

// Table used to describe the various hardware devices we support.
// Some good summary background information is at:
//
//	<http://www.palmos.com/dev/tech/hardware/compare.html>

static const DeviceInfo kDeviceInfo[] = {
    // ===== Palm Devices =====
    {kDevicePilot,
     "Pilot",
     {"Pilot", "Pilot1000", "Pilot5000", "TD", "Touchdown"},
     kSupports68328,
     1024,
     hwrMiscFlagIDTouchdown,
     hwrMiscFlagExtSubIDNone,
     {}},
    {kDevicePalmPilot,
     "PalmPilot",
     {"PalmPilot", "PalmPilotPersonal", "PalmPilotProfessional", "Striker"},
     kSupports68328,
     1024,
     hwrMiscFlagIDPalmPilot,
     hwrMiscFlagExtSubIDNone,
     {}},
    {kDevicePalmIII,
     "Palm III",
     {"PalmIII", "Rocky"},
     kSupports68328 + kHasFlash,
     2048,
     hwrMiscFlagIDRocky,
     hwrMiscFlagExtSubIDNone,
     {{'palm', 'rcky'}}},
    {kDevicePalmIIIc,
     "Palm IIIc",
     {"PalmIIIc", "Austin", "ColorDevice"},
     kSupports68EZ328 + kHasFlash,
     8192,
     hwrMiscFlagIDPalmIIIc,
     hwrMiscFlagExtSubIDNone,
     {{'palm', 'astn'}}},
    {kDevicePalmIIIe,
     "Palm IIIe",
     {"PalmIIIe", "Robinson"},
     kSupports68EZ328,
     2048,
     hwrMiscFlagIDBrad,
     hwrMiscFlagExtSubIDBrad,
     {}},
    {kDevicePalmIIIx,
     "Palm IIIx",
     {"PalmIIIx", "Brad"},
     kSupports68EZ328 + kHasFlash,
     4096,
     hwrMiscFlagIDBrad,
     hwrMiscFlagExtSubIDBrad,
     {{'palm', 'sumo'}}},
    {kDevicePalmIIIxe,
     "Palm IIIxe",
     {"PalmIIIxe"},
     kSupports68EZ328 + kHasFlash,
     8192,
     hwrMiscFlagIDBrad,
     hwrMiscFlagExtSubIDBrad,
     {{'palm', 'sumo'}}},
    {kDevicePalmV,
     "Palm V",
     {"PalmV", "Razor", "Sumo"},
     kSupports68EZ328 + kHasFlash,
     2048,
     hwrMiscFlagIDSumo,
     hwrMiscFlagExtSubIDSumo,
     {{'palm', 'sumo'}}},
    {kDevicePalmVx,
     "Palm Vx",
     {"PalmVx", "Cobra"},
     kSupports68EZ328 + kHasFlash,
     8192,
     hwrMiscFlagIDSumo,
     hwrMiscFlagExtSubIDCobra,
     {{'palm', 'sumo'}}},
    {kDevicePalmVII,
     "Palm VII",
     {"PalmVII", "Jerry", "Eleven"},
     kSupports68328 + kHasFlash,
     2048,
     hwrMiscFlagIDJerry,
     hwrMiscFlagExtSubIDNone,
     {}},
    {
        kDevicePalmVIIEZ,
        "Palm VII (EZ)",
        {"PalmVIIEZ", "Bonanza"},
        kSupports68EZ328 + kHasFlash,
        2048,
        hwrMiscFlagIDJerryEZ,
        0,
        {}
        // sub-id *should* be hwrMiscFlagExtSubIDNone, but
        // a bug in Rangoon (OS 3.5 for Palm VII EZ) has it
        // accidentally looking for and using the sub-id. In
        // order to get the hard keys to work, we need to
        // return zero here.
    },
    {
        kDevicePalmVIIx,
        "Palm VIIx",
        {"PalmVIIx", "Octopus"},
        kSupports68EZ328 + kHasFlash,
        8192,
        hwrMiscFlagIDJerryEZ,
        hwrMiscFlagExtSubIDBrad,
        {}
        // David Mai says that he *guesses* that the sub-id is zero
    },
    {kDevicePalmM100,
     "Palm m100",
     {"PalmM100", "m100", "Calvin"},
     kSupports68EZ328,
     2048,
     hwrMiscFlagIDm100,
     hwrMiscFlagExtSubIDBrad,
     {{'palm', 'clvn'}}},
    {kDevicePalmM105,
     "Palm m105",
     {"PalmM105", "m105"},
     kSupports68EZ328,
     8192,
     hwrMiscFlagIDm100,
     hwrMiscFlagExtSubIDBrad,
     {{'palm', 'clvn'}}},
    {kDevicePalmM125,
     "Palm m125",
     {"PalmM125", "m120", "m125", "m110", "m115", "JV", "Stu"},
     kSupports68VZ328 + kHasFlash,
     8192,
     hwrMiscFlagIDNone,
     hwrMiscFlagExtSubIDNone,
     {{'palm', 'vstu'}}},
    {kDevicePalmM130,
     "Palm m130",
     {"PalmM130", "m130", "Hobbes", "PalmM130"},
     kSupports68VZ328,
     8192,
     hwrMiscFlagIDNone,
     hwrMiscFlagExtSubIDNone,
     {{'palm', 'vhbs'}}},
    {kDevicePalmM500,
     "Palm m500",
     {"PalmM500", "m500", "Tornado"},
     kSupports68VZ328 + kHasFlash,
     8192,
     hwrMiscFlagIDNone,
     hwrMiscFlagExtSubIDNone,
     {{'palm', 'vtrn'}}},
    {kDevicePalmM505,
     "Palm m505",
     {"PalmM505", "m505", "EmeraldCity", "VZDevice"},
     kSupports68VZ328 + kHasFlash,
     8192,
     hwrMiscFlagIDNone,
     hwrMiscFlagExtSubIDNone,
     {{'palm', 'ecty'}}},
    {kDevicePalmM515,
     "Palm m515",
     {"PalmM515", "m515", "PalmM525", "m525", "Lighthouse", "GatorReef"},
     kSupports68VZ328 + kHasFlash,
     16384,
     hwrMiscFlagIDNone,
     hwrMiscFlagExtSubIDNone,
     {{'palm', 'vlit'}}},
    {kDevicePalmM520,
     "Palm m520",
     {"PalmM520", "m520"},
     kSupports68VZ328 + kHasFlash,
     16384,
     hwrMiscFlagIDNone,
     hwrMiscFlagExtSubIDNone,
     {{'palm', 'sfgo'}}},
    {kDevicePalmI705,
     "Palm i705",
     {"PalmI705", "i705", "Everest", "PalmI705"},
     kSupports68VZ328 + kHasFlash,
     8192,
     hwrMiscFlagIDNone,
     hwrMiscFlagExtSubIDNone,
     {{'palm', 'skyw'}}},
    {kDevicePalmI710,
     "Palm i710",
     {"PalmI710", "i710", "AtlantiC"},
     kSupports68VZ328 + kHasFlash,
     16384,
     hwrMiscFlagIDNone,
     hwrMiscFlagExtSubIDNone,
     {{'palm', 'atlc'}}},
    {kDevicePalmPacifiC,
     "PalmPacifiC",
     {"PalmPacifiC", "i710", "PacifiC"},
     kSupports68VZ328 + kHasFlash,
     16384,
     hwrMiscFlagIDNone,
     hwrMiscFlagExtSubIDNone,
     {{'palm', 'atlc'}}},
    // ===== Symbol devices =====
    {kDeviceSymbol1500,
     "Symbol 1500",
     {"Symbol1500"},
     kSupports68328 + kHasFlash,
     2048,
     hwrMiscFlagIDRocky,
     hwrMiscFlagExtSubIDNone,
     {}},
    {kDeviceSymbol1700,
     "Symbol 1700",
     {"Symbol1700"},
     kSupports68328 + kHasFlash,
     2048,
     hwrMiscFlagIDRocky,
     hwrMiscFlagExtSubIDNone,
     {}},
    {kDeviceSymbol1740,
     "Symbol 1740",
     {"Symbol1740"},
     kSupports68328 + kHasFlash,
     2048,
     hwrMiscFlagIDRocky,
     hwrMiscFlagExtSubIDNone,
     {}},
#if 0  // CSTODO
    // ===== TRG/HandEra devices =====

    {kDeviceTRGpro,
     "TRGpro",
     {"TRGpro"},
     kSupports68EZ328 + kHasFlash,
     8192,
     hwrOEMCompanyIDTRG,
     hwrTRGproID,
     {{'trgp', 'trg1'}}},
#endif
    {kDeviceHandEra330,
     "HandEra 330",
     {"HandEra330"},
     kSupports68VZ328 + kHasFlash,
     8192,
     1,
     1,
     {{'trgp', 'trg2'}}},
    {kDeviceHandEra330c,
     "HandEra 330C",
     {"HandEra330c"},
     kSupports68VZ328 + kHasFlash,
     16384,
     1,
     1,
     {{'trgp', 'trg3'}}},
    {kDevicePEGS300, "PEG-S300 series", {"PEG-S300"}, kSupports68EZ328, 8192, 0, 0, {}},
    {kDevicePEGS320,
     "PEG-S320 series",
     {"PEG-S320"},
     kSupports68VZ328,
     8192,
     UNSUPPORTED,
     UNSUPPORTED,
     {{'sony', 'nasc'}}},
    {kDevicePEGS300, "PEG-S300 series", {"PEG-S300"}, kSupports68EZ328, 8192, 0, 0, {}},
    {kDevicePEGS320,
     "PEG-S320 series",
     {"PEG-S320"},
     kSupports68VZ328,
     8192,
     UNSUPPORTED,
     UNSUPPORTED,
     {{'sony', 'nasc'}}},
    {kDevicePEGT400,
     "PEG-T400 series",
     {"PEG-T400"},
     kSupports68VZ328,
     8192,
     UNSUPPORTED,
     UNSUPPORTED,
     {{'sony', 'vnce'}}},
    {kDevicePEGN600C,
     "PEG-N600C/N610C series",
     {"PEG-N600C/N610C"},
     kSupports68VZ328,
     8192,
     UNSUPPORTED,
     UNSUPPORTED,
     {{'sony', 'ystn'}}},
    {kDevicePEGT600,
     "PEG-T600 series",
     {"PEG-T600"},
     kSupports68VZ328,
     8192,
     UNSUPPORTED,
     UNSUPPORTED,
     {{'sony', 'mdna'}}},
    {kDevicePEGN700C,
     "PEG-N700C/N710C series",
     {"PEG-N700C/N710C"},
     kSupports68VZ328,
     8192,
     UNSUPPORTED,
     UNSUPPORTED,
     {{'sony', 'ysmt'}}},
    {kDeviceYSX1230,
     "YSX1230 series",
     {"YSX1230"},
     kSupports68SZ328,
     16384,
     UNSUPPORTED,
     UNSUPPORTED,
     {{'sony', 'npls'}}},
    {kDeviceYSX1100,
     "NR70 series",
     {"NR70"},
     kSupports68SZ328,
     16384,
     UNSUPPORTED,
     UNSUPPORTED,
     {{'sony', 'rdwd'}}},
    {kDevicePEGS500C, "PEG-S500C series", {"PEG-S500C"}, kSupports68EZ328, 8192, 0, 0, {}},
    {kDeviceAcerS65, "Acer S65", {"Acer-S65"}, kSupports68VZ328, 16384, 0, 0, {{'acer', 'colr'}}},
    {kDeviceAcerS11, "Acer S11", {"Acer-S11"}, kSupports68VZ328, 16384, 0, 0, {{'acer', 'momo'}}},
#if 0
    // ===== Handspring devices =====
    {
        kDeviceVisor,
        "Visor",
        {"Visor", "Lego"},
        kSupports68EZ328,
        2048,
        halModelIDLego,
        0,  // Hardware ID for first Visor
    },
    {kDeviceVisorPlatinum,
     "Visor Platinum",
     {"VisorPlatinum"},
     kSupports68VZ328,
     8192,
     halModelIDVisorPlatinum,
     0},
    {kDeviceVisorPrism,
     "Visor Prism",
     {"VisorPrism"},
     kSupports68VZ328,
     8192,
     halModelIDVisorPrism,
     0},
    {kDeviceVisorEdge,
     "Visor Edge",
     {"VisorEdge"},
     kSupports68VZ328,
     8192,
     halModelIDVisorEdge,
     0}
#endif
};

/***********************************************************************
 *
 * FUNCTION:    EmDevice::EmDevice
 *
 * DESCRIPTION: Constructors
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

EmDevice::EmDevice(int id) : fDeviceID(id) { ConfigureMemoryRegions(); }

EmDevice::EmDevice(void) : EmDevice(kDeviceUnspecified) {}

EmDevice::EmDevice(const char* s) : EmDevice(this->GetDeviceID(s)) {}

EmDevice::EmDevice(const string& s) : EmDevice(this->GetDeviceID(s.c_str())) {}

EmDevice::EmDevice(const EmDevice& other) : EmDevice(other.fDeviceID) {}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::~EmDevice
 *
 * DESCRIPTION: Destructor
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

EmDevice::~EmDevice(void) {}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::operator==
 * FUNCTION:    EmDevice::operator!=
 *
 * DESCRIPTION: Destructor
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Nothing
 *
 ***********************************************************************/

bool EmDevice::operator==(const EmDevice& other) const { return fDeviceID == other.fDeviceID; }

bool EmDevice::operator!=(const EmDevice& other) const { return fDeviceID != other.fDeviceID; }

/***********************************************************************
 *
 * FUNCTION:    EmDevice::Supported
 *
 * DESCRIPTION: Returns whether or not this EmDevice represents a device
 *				we currently support emulating.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    True if so.
 *
 ***********************************************************************/

Bool EmDevice::Supported(void) const {
    const DeviceInfo* info = this->GetDeviceInfo();
    return info != NULL;
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::Supports68328
 *
 * DESCRIPTION: Returns whether or not this device is based on the
 *				MC68328.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    True if so.
 *
 ***********************************************************************/

Bool EmDevice::Supports68328(void) const {
    const DeviceInfo* info = this->GetDeviceInfo();
    return (info->fFlags & kSupports68328) != 0;
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::Supports68EZ328
 *
 * DESCRIPTION: Returns whether or not this device is based on the
 *				MC68EZ328.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    True if so.
 *
 ***********************************************************************/

Bool EmDevice::Supports68EZ328(void) const {
    const DeviceInfo* info = this->GetDeviceInfo();
    return (info->fFlags & kSupports68EZ328) != 0;
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::Supports68VZ328
 *
 * DESCRIPTION: Returns whether or not this device is based on the
 *				MC68VZ328.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    True if so.
 *
 ***********************************************************************/

Bool EmDevice::Supports68VZ328(void) const {
    const DeviceInfo* info = this->GetDeviceInfo();
    return (info->fFlags & kSupports68VZ328) != 0;
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::Supports68SZ328
 *
 * DESCRIPTION: Returns whether or not this device is based on the
 *				MC68SZ328.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    True if so.
 *
 ***********************************************************************/

Bool EmDevice::Supports68SZ328(void) const {
    const DeviceInfo* info = this->GetDeviceInfo();
    return (info->fFlags & kSupports68SZ328) != 0;
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::PrismPlatinumEdgeHack
 *
 * DESCRIPTION: Handspring's Prism, Platinum, and Edge have card headers
 *				saying they run on EZ's when they really run on VZ's.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    True if so.
 *
 ***********************************************************************/

Bool EmDevice::HasBogusEZFlag(void) const {
    switch (fDeviceID) {
        case kDeviceVisorPrism:
        case kDeviceVisorPlatinum:
        case kDeviceVisorEdge:
        case kDeviceHandEra330:
        case kDevicePEGN700C:
            return true;

        default:
            return false;
    }
}

/***********************************************************************
 *
 * FUNCTION:	EmDevice::EdgeHack
 *
 * DESCRIPTION:	Handspring's Edge uses bit 3 of port D for powered-cradle
 *				detection, so we need to apply a few hacks.
 *
 * PARAMETERS:	None
 *
 * RETURNED:	True if so.
 *
 ***********************************************************************/

Bool EmDevice::EdgeHack(void) const { return fDeviceID == kDeviceVisorEdge; }

/***********************************************************************
 *
 * FUNCTION:    EmDevice::SupportsROM
 *
 * DESCRIPTION: Returns whether or not this device can support the
 *				execution of a particular ROM.
 *
 * PARAMETERS:  A reference to an EmROMReader object. Hopefully one
 *				that has successfully acquired the details of a ROM.
 *
 * RETURNED:    True if ROM is supported by device.
 *
 ***********************************************************************/

Bool EmDevice::SupportsROM(const EmROMReader& ROM) const {
    /* If the ROM is recent enough, use the embedded hardware ID information
       to determine whether it will work with this device */

    if (ROM.GetCardVersion() >= 5) {
        const DeviceInfo* info = this->GetDeviceInfo();

        for (size_t ii = 0; ii < countof(info->fCardHeaderInfo); ++ii) {
            if (info->fCardHeaderInfo[ii].fCompanyID && info->fCardHeaderInfo[ii].fHalID &&
                info->fCardHeaderInfo[ii].fCompanyID == ROM.GetCompanyID() &&
                info->fCardHeaderInfo[ii].fHalID == ROM.GetHalID()) {
                return true;
            }
        }

        return false;
    }

    /* Otherwise, we will use miscellaneous pieces of data to heuristically
       discriminate between ROMs. Do not assume that any of these tests are
       meant to convey ownership or documented interfaces. */

    bool is7 = ROM.ContainsDB(PALM_VII_IF1_DB) || ROM.ContainsDB(PALM_VII_IF2_DB);
    bool is328 = (ROM.GetCardVersion() <= 2) || ROM.GetFlag328();
    bool isColor = ROM.GetSplashChunk() && (ROM.IsBitmapColor(*ROM.GetSplashChunk(), true) == 1);

    if (Supports68328() && !is328)
        return false;
    else if (Supports68EZ328() && !ROM.GetFlagEZ())
        return false;
    else if (Supports68VZ328() && !ROM.GetFlagVZ()) {
        /*	Make a hack for the Prism, Platinum and Edge below since
                Handspring seems to report the EZ bit in their ROMs. */

        if (!this->HasBogusEZFlag()) {
            return false;
        }
    }

    switch (fDeviceID) {
        case kDeviceUnspecified:
            return false;
            break;

        case kDevicePilot:
            if ((ROM.Version() < 0x020000) && (ROM.GetCardManufacturer() == PALM_MANUF))
                return true;
            break;

        case kDevicePalmPilot:
            if ((ROM.GetCardManufacturer() == PALM_MANUF) && (ROM.Version() >= 0x020000) &&
                (ROM.Version() < 0x030000))
                return true;
            break;

        case kDevicePalmIII:
            if ((ROM.GetCardManufacturer() == PALM_MANUF) && (ROM.Version() >= 0x030000) &&
                !ROM.ContainsDB(SYMBOL_DB) && !is7)
                return true;
            break;

        case kDevicePalmIIIc:
            if ((ROM.GetCardManufacturer() == PALM_MANUF) && !ROM.ContainsDB(PALM_m100_DB) &&
                !is7 && isColor)
                return true;
            break;

        case kDevicePalmIIIe:
            if ((ROM.GetCardManufacturer() == PALM_MANUF) && !ROM.ContainsDB(PALM_m100_DB) &&
                !is7 && !isColor && (ROM.Version() == 0x030100))
                return true;
            break;

        case kDevicePalmIIIx:
        case kDevicePalmIIIxe:
            if ((ROM.GetCardManufacturer() == PALM_MANUF) && !ROM.ContainsDB(PALM_m100_DB) &&
                !is7 && !isColor)
                return true;
            break;

        case kDevicePalmV:
            if ((ROM.GetCardManufacturer() == PALM_MANUF) && !ROM.ContainsDB(PALM_m100_DB) &&
                !is7 && !isColor)
                return true;
            break;

        case kDevicePalmVx:
            if ((ROM.GetCardManufacturer() == PALM_MANUF) && !ROM.ContainsDB(PALM_m100_DB) &&
                !is7 && !isColor && (ROM.Version() >= 0x030300))
                return true;
            break;

        case kDevicePalmVII:
            if ((ROM.GetCardManufacturer() == PALM_MANUF) && (ROM.Version() >= 0x030000) &&
                !ROM.ContainsDB(SYMBOL_DB) && is7)
                return true;
            break;

        case kDevicePalmVIIEZ:
            if ((ROM.GetCardManufacturer() == PALM_MANUF) && is7 &&
                (!ROM.ContainsDB(PALM_VIIX_DB) || ROM.ContainsDB(PALM_VII_EZ_NEW_DB)) && !isColor)
                return true;
            break;

        case kDevicePalmVIIx:
            if ((ROM.GetCardManufacturer() == PALM_MANUF) && is7 && ROM.ContainsDB(PALM_VIIX_DB) &&
                !ROM.ContainsDB(PALM_VII_EZ_NEW_DB) && !isColor)
                return true;
            break;

        case kDevicePalmM105:
        case kDevicePalmM100:
            if (ROM.ContainsDB(PALM_m100_DB)) return true;
            break;

#if 0

        case kDeviceSymbol1500:
            if ((ROM.GetCardManufacturer() == PALM_MANUF) && (ROM.Version() < 0x030500) &&
                ROM.ContainsDB(SYMBOL_1500_DB) && !ROM.ContainsDB(SYMBOL_1700_DB) &&
                !ROM.ContainsDB(SYMBOL_1740_DB))
                return true;
            break;

        case kDeviceSymbol1700:
            if ((ROM.GetCardManufacturer() == PALM_MANUF) && (ROM.Version() < 0x030500) &&
                !ROM.ContainsDB(SYMBOL_1500_DB) && ROM.ContainsDB(SYMBOL_1700_DB) &&
                !ROM.ContainsDB(SYMBOL_1740_DB))
                return true;
            break;

        case kDeviceSymbol1740:
            if ((ROM.GetCardManufacturer() == PALM_MANUF) && (ROM.Version() < 0x030500) &&
                !ROM.ContainsDB(SYMBOL_1500_DB) && ROM.ContainsDB(SYMBOL_1700_DB) &&
                ROM.ContainsDB(SYMBOL_1740_DB))
                return true;
            break;

        case kDeviceTRGpro:
#endif

        case kDeviceHandEra330:
            if (ROM.GetCardManufacturer() == "TRG Products") return true;
            break;

#if 0
        case kDeviceVisor:
            if ((ROM.GetCardManufacturer() == HANDSPRING_MANUF) && (ROM.Version() == 0x030100))
                return true;
            break;

        case kDeviceVisorPrism:
            if ((ROM.GetCardManufacturer() == HANDSPRING_MANUF) && (ROM.Version() >= 0x030500))
                return true;
            break;

        case kDeviceVisorPlatinum:
            if ((ROM.GetCardManufacturer() == HANDSPRING_MANUF) && (ROM.Version() >= 0x030500))
                return true;
            break;

        case kDeviceVisorEdge:
            if ((ROM.GetCardManufacturer() == HANDSPRING_MANUF) && (ROM.Version() >= 0x030500))
                return true;
            break;

#endif
        case kDevicePEGS500C:
            if (!(ROM.GetCardManufacturer().compare(0, 16, "SONY Corporation")) && isColor)
                return true;
            break;

        case kDevicePEGS300:
            if (!(ROM.GetCardManufacturer().compare(0, 16, "SONY Corporation")) && !isColor)
                return true;
            break;

        case kDevicePEGN700C: {
            if (!ROM.GetCardManufacturer().compare(0, 16, "SonyCorporation") ||
                !ROM.GetCardManufacturer().compare(0, 4, "Sony"))
                return true;
        } break;

        case kDeviceLast:
            EmAssert(false);
            break;

        default:
            break;
    }

    return false;
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::CreateCPU
 *
 * DESCRIPTION: Create the CPU object.
 *
 * PARAMETERS:  session - EmSession object used to initialize the CPU
 *					object.
 *
 * RETURNED:    Pointer to the created object.  Caller is responsible
 *				for destroying the object when it's done with it.
 *
 ***********************************************************************/

EmCPU* EmDevice::CreateCPU(EmSession* session) const {
    EmCPU* result = NULL;

    if (this->Supports68328() || this->Supports68EZ328() || this->Supports68VZ328() ||
        this->Supports68SZ328()) {
        result = new EmCPU68K(session);
    }

    EmAssert(result);

    return result;
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::CreateRegs
 *
 * DESCRIPTION: Create the register object that manages the Dragonball
 *				(or derivative) hardware registers.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Pointer to the created object.  Caller is responsible
 *				for destroying the object when it's done with it.
 *
 ***********************************************************************/

void EmDevice::CreateRegs(void) const {
    switch (fDeviceID) {
        case kDevicePilot:
            EmBankRegs::AddSubBank(new EmRegs328Pilot);
            break;

        case kDevicePalmPilot:
            EmBankRegs::AddSubBank(new EmRegs328PalmPilot);
            break;

        case kDevicePalmIII:
            EmBankRegs::AddSubBank(new EmRegs328PalmIII);
            break;

        case kDevicePalmIIIc: {
            EmBankRegs::AddSubBank(new EmRegsEZPalmIIIc);

            EmRegsFrameBuffer* framebuffer = new EmRegsFrameBuffer(sed1375BaseAddress);

            EmBankRegs::AddSubBank(
                new EmRegsSED1375(sed1375RegsAddr, sed1375BaseAddress, *framebuffer));
            EmBankRegs::AddSubBank(framebuffer);
            break;
        }

        case kDevicePalmIIIe:
            EmBankRegs::AddSubBank(new EmRegsEZPalmIIIe);
            break;

        case kDevicePalmIIIx:
        case kDevicePalmIIIxe:
            EmBankRegs::AddSubBank(new EmRegsEZPalmIIIx);
            break;

        case kDevicePalmV:
            EmBankRegs::AddSubBank(new EmRegsEZPalmV);
            break;

        case kDevicePalmVx:
            EmBankRegs::AddSubBank(new EmRegsEZPalmVx);
            break;

        case kDevicePalmVII:
            EmBankRegs::AddSubBank(new EmRegs328PalmVII);
            break;

        case kDevicePalmVIIEZ:
            EmBankRegs::AddSubBank(new EmRegsEZPalmVII);
            EmBankRegs::AddSubBank(new EmRegsPLDPalmVIIEZ);
            break;

        case kDevicePalmVIIx:
            EmBankRegs::AddSubBank(new EmRegsEZPalmVIIx);
            EmBankRegs::AddSubBank(new EmRegsPLDPalmVIIEZ);
            break;

        case kDevicePalmM105:
        case kDevicePalmM100:
            EmBankRegs::AddSubBank(new EmRegsEZPalmM100);
            break;

        case kDevicePalmM125:
            EmBankRegs::AddSubBank(new EmRegsVZPalmM125);
            EmBankRegs::AddSubBank(new EmRegsUSBPhilipsPDIUSBD12(0x1F000000));
            break;

        case kDevicePalmM130: {
            EmBankRegs::AddSubBank(new EmRegsVZPalmM130);

            EmRegsFrameBuffer* framebuffer = new EmRegsFrameBuffer(T_BASE);

            EmBankRegs::AddSubBank(new EmRegsMediaQ11xx(*framebuffer, MMIO_BASE, T_BASE));
            EmBankRegs::AddSubBank(framebuffer);
            break;
        }

        case kDevicePalmI710: {
            EmBankRegs::AddSubBank(new EmRegsVZAtlantiC());
            EmBankRegs::AddSubBank(new EmRegsPLDAtlantiC(0x10800000));

            EmRegsFrameBuffer* framebuffer = new EmRegsFrameBuffer(T_BASE);

            EmBankRegs::AddSubBank(new EmRegsMediaQ11xx(*framebuffer, MMIO_BASE, T_BASE));
            EmBankRegs::AddSubBank(framebuffer);

            break;
        }

        case kDevicePalmPacifiC: {
            EmBankRegs::AddSubBank(new EmRegsVZAtlantiC());
            EmBankRegs::AddSubBank(new EmRegsPLDPacifiC(0x10800000));

            EmRegsFrameBuffer* framebuffer = new EmRegsFrameBuffer(T_BASE);

            EmBankRegs::AddSubBank(new EmRegsMediaQ11xxPacifiC(*framebuffer, MMIO_BASE, T_BASE));
            EmBankRegs::AddSubBank(framebuffer);

            break;
        }

        case kDevicePalmM500:
            EmBankRegs::AddSubBank(new EmRegsVZPalmM500);
            EmBankRegs::AddSubBank(new EmRegsUSBPhilipsPDIUSBD12(0x10400000));
            break;

        case kDevicePalmM505: {
            EmBankRegs::AddSubBank(new EmRegsVZPalmM505);

            EmRegsFrameBuffer* framebuffer = new EmRegsFrameBuffer(sed1376VideoMemStart);

            EmBankRegs::AddSubBank(
                new EmRegsSED1376PalmGeneric(sed1376RegsAddr, sed1376VideoMemStart, *framebuffer));
            EmBankRegs::AddSubBank(framebuffer);
            EmBankRegs::AddSubBank(new EmRegsUSBPhilipsPDIUSBD12(0x10400000));
        } break;

        case kDevicePalmM515: {
            EmBankRegs::AddSubBank(new EmRegsVZPalmM505);

            EmRegsFrameBuffer* framebuffer = new EmRegsFrameBuffer(sed1376VideoMemStart);

            EmBankRegs::AddSubBank(
                new EmRegsSED1376PalmGeneric(sed1376RegsAddr, sed1376VideoMemStart, *framebuffer));
            EmBankRegs::AddSubBank(framebuffer);
            EmBankRegs::AddSubBank(new EmRegsUSBPhilipsPDIUSBD12(0x10400000));
        } break;

        case kDevicePalmM520: {
            EmBankRegs::AddSubBank(new EmRegsVZPalmM505);

            EmRegsFrameBuffer* framebuffer = new EmRegsFrameBuffer(T_BASE);

            EmBankRegs::AddSubBank(new EmRegsMediaQ11xx(*framebuffer, MMIO_BASE, T_BASE));
            EmBankRegs::AddSubBank(framebuffer);
            break;
        }

        case kDevicePalmI705:
            EmBankRegs::AddSubBank(new EmRegsVZPalmI705);
            EmBankRegs::AddSubBank(new EmRegsPLDPalmI705(0x11000000));
            EmBankRegs::AddSubBank(new EmRegsUSBPhilipsPDIUSBD12(0x1F000000));
            break;

#if 0

        case kDeviceARMRef:
            break;

        case kDeviceSymbol1500:
            EmBankRegs::AddSubBank(new EmRegs328PalmIII);
            break;

        case kDeviceSymbol1700:
            EmBankRegs::AddSubBank(new EmRegs328Symbol1700);
            EmBankRegs::AddSubBank(new EmRegsASICSymbol1700);
            break;

        case kDeviceSymbol1740:
            EmBankRegs::AddSubBank(new EmRegs328Symbol1700);
            EmBankRegs::AddSubBank(new EmRegsASICSymbol1700);
            break;

        case kDeviceTRGpro:
            OEMCreateTRGRegObjs(hwrTRGproID);
            break;
#endif

        case kDeviceHandEra330: {
            HandEra330PortManager* fPortMgr;
            EmRegsVZHandEra330* regsVZHandera330 = new EmRegsVZHandEra330(&fPortMgr);

            EmBankRegs::AddSubBank(regsVZHandera330);
            EmBankRegs::AddSubBank(new EmRegs330CPLD(kMemoryStartCPLD330, fPortMgr,
                                                     regsVZHandera330->GetSPISlaveSD(),
                                                     EmRegs330CPLD::Model::h330));
            EmBankRegs::AddSubBank(new EmRegsCFMemCard(&fPortMgr->CFBus));
        } break;

        case kDeviceHandEra330c: {
            HandEra330PortManager* fPortMgr;
            EmRegsVZHandEra330c* regsVZHandera330c = new EmRegsVZHandEra330c(&fPortMgr);

            EmBankRegs::AddSubBank(regsVZHandera330c);
            EmBankRegs::AddSubBank(new EmRegs330CPLD(kMemoryStartCPLD330c, fPortMgr,
                                                     regsVZHandera330c->GetSPISlaveSD(),
                                                     EmRegs330CPLD::Model::h330c));
            EmBankRegs::AddSubBank(new EmRegsCFMemCard(&fPortMgr->CFBus));
            EmBankRegs::AddSubBank(new EmRegs330CPLDMirror());

            EmRegsFrameBuffer* framebuffer = new EmRegsFrameBuffer(0x10c00000);

            EmBankRegs::AddSubBank(new EmRegsMediaQ11xx(*framebuffer, 0x10c40000, 0x10c00000));
            EmBankRegs::AddSubBank(framebuffer);
        } break;

        case kDevicePEGS300: {
            EmRegsMB86189* mb86189 = new EmRegsMB86189(0x10200000);

            EmBankRegs::AddSubBank(new EmRegsEzPegS300(*mb86189));
            EmBankRegs::AddSubBank(mb86189);
            // EmBankRegs::AddSubBank(new EmRegsExpCardCLIE());
            EmBankRegs::AddSubBank(new EmRegsUsbCLIE(0x00100000));
            break;
        }

        case kDevicePEGS320: {
            EmRegsMB86189* mb86189 = new EmRegsMB86189(0x10400000);

            EmBankRegs::AddSubBank(new EmRegsVzPegNasca(*mb86189));
            EmBankRegs::AddSubBank(mb86189);
            EmBankRegs::AddSubBank(new EmRegsUsbCLIE(0x11000000L, 0));
            break;
        }

        case kDevicePEGS500C: {
            EmRegsMB86189* mb86189 = new EmRegsMB86189(0x10400000);

            EmBankRegs::AddSubBank(new EmRegsEzPegS500C(*mb86189));

            EmRegsFrameBuffer* framebuffer = new EmRegsFrameBuffer(sed1375BaseAddress);
            EmBankRegs::AddSubBank(
                new EmRegsSED1375(sed1375RegsAddr, sed1375BaseAddress, *framebuffer));
            EmBankRegs::AddSubBank(framebuffer);

            EmBankRegs::AddSubBank(mb86189);
            EmBankRegs::AddSubBank(new EmRegsUsbCLIE(0x00100000));
        } break;

        case kDevicePEGT400: {
            EmRegsMB86189* mb86189 = new EmRegsMB86189(0x10800000);

            EmBankRegs::AddSubBank(new EmRegsVzPegVenice(*mb86189));
            EmBankRegs::AddSubBank(mb86189);
            EmBankRegs::AddSubBank(new EmRegsUsbCLIE(0x10900000L, 0));
            EmBankRegs::AddSubBank(new EmRegsFMSound(FMSound_BaseAddress));
            break;
        }

        case kDevicePEGN600C: {
            EmRegsMB86189* mb86189 = new EmRegsMB86189(0x10800000);
            EmBankRegs::AddSubBank(new EmRegsVzPegYellowStone(*mb86189));

            EmRegsFrameBuffer* framebuffer = new EmRegsFrameBuffer(T_BASE);
            EmBankRegs::AddSubBank(new EmRegsMediaQ11xx(*framebuffer, MMIO_BASE, T_BASE));
            EmBankRegs::AddSubBank(framebuffer);

            EmBankRegs::AddSubBank(mb86189);
            EmBankRegs::AddSubBank(new EmRegsUsbCLIE(0x10900000L, 0));
            break;
        }

        case kDevicePEGT600: {
            EmRegsMB86189* mb86189 = new EmRegsMB86189(0x10800000);
            EmBankRegs::AddSubBank(new EmRegsVzPegModena(*mb86189));

            EmRegsFrameBuffer* framebuffer = new EmRegsFrameBuffer(T_BASE);
            EmBankRegs::AddSubBank(new EmRegsMediaQ11xx(*framebuffer, MMIO_BASE, T_BASE));
            EmBankRegs::AddSubBank(framebuffer);

            EmBankRegs::AddSubBank(mb86189);
            EmBankRegs::AddSubBank(new EmRegsUsbCLIE(0x10900000L, 0));
            EmBankRegs::AddSubBank(new EmRegsFMSound(FMSound_BaseAddress));
            break;
        }

        case kDevicePEGN700C: {
            EmRegsSonyDSP* dsp = new EmRegsSonyDSP(0x10800000);

            EmBankRegs::AddSubBank(new EmRegsVzPegN700C(*dsp));
            EmRegsFrameBuffer* framebuffer = new EmRegsFrameBuffer(T_BASE);
            EmBankRegs::AddSubBank(new EmRegsMediaQ11xx(*framebuffer, MMIO_BASE, T_BASE));
            EmBankRegs::AddSubBank(framebuffer);

            EmBankRegs::AddSubBank(dsp);
            break;
        }

        case kDeviceYSX1100: {
            EmRegsSonyDSP* dsp = new EmRegsSonyDSP(0x11000000);

            EmBankRegs::AddSubBank(new EmRegsSzRedwood(*dsp));
            EmRegsFrameBuffer* framebuffer = new EmRegsFrameBuffer(T_BASE);
            EmBankRegs::AddSubBank(new EmRegsMQLCDControlT2(
                *framebuffer, MQ_LCDControllerT2_RegsAddr, MQ_LCDControllerT2_VideoMemStart));
            EmBankRegs::AddSubBank(framebuffer);
            EmBankRegs::AddSubBank(dsp);
            break;
        }

        case kDeviceYSX1230: {
            EmRegsSonyDSP* dsp = new EmRegsSonyDSP(0x11000000);

            EmBankRegs::AddSubBank(new EmRegsSzNaples(*dsp));
            EmRegsFrameBuffer* framebuffer = new EmRegsFrameBuffer(T_BASE);
            EmBankRegs::AddSubBank(new EmRegsMediaQ11xx(*framebuffer, MMIO_BASE, T_BASE));
            EmBankRegs::AddSubBank(framebuffer);
            EmBankRegs::AddSubBank(dsp);
            break;
        }

        case kDeviceAcerS65: {
            EmRegsMB86189* mb86189 = new EmRegsMB86189(0x10c00000);
            EmBankRegs::AddSubBank(new EmRegsVZPalmM505());

            EmRegsFrameBuffer* framebuffer = new EmRegsFrameBuffer(T_BASE);
            EmBankRegs::AddSubBank(new EmRegsMediaQ11xx(*framebuffer, MMIO_BASE, T_BASE));
            EmBankRegs::AddSubBank(framebuffer);

            EmBankRegs::AddSubBank(mb86189);
            break;
        }

        case kDeviceAcerS11: {
            EmRegsMB86189* mb86189 = new EmRegsMB86189(0x10c00000);
            EmBankRegs::AddSubBank(new EmRegsVZAcerS1x(*mb86189));
            EmBankRegs::AddSubBank(new EmRegsAcerUSBStub());
            EmBankRegs::AddSubBank(new EmRegsAcerDSPStub());

            EmBankRegs::AddSubBank(mb86189);
            break;
        }

#if 0
        case kDeviceVisor:
            EmBankRegs::AddSubBank(new EmRegsEZVisor);
            EmBankRegs::AddSubBank(new EmRegsUSBVisor);
            break;

        case kDeviceVisorPrism:
            EmBankRegs::AddSubBank(new EmRegsVZVisorPrism);
            EmBankRegs::AddSubBank(new EmRegsSED1376VisorPrism(0x11800000, 0x11820000));
            EmBankRegs::AddSubBank(new EmRegsFrameBuffer(0x11820000, 80 * 1024L));
            EmBankRegs::AddSubBank(new EmRegsUSBVisor);
            break;

        case kDeviceVisorPlatinum:
            EmBankRegs::AddSubBank(new EmRegsVZVisorPlatinum);
            EmBankRegs::AddSubBank(new EmRegsUSBVisor);
            break;

        case kDeviceVisorEdge:
            EmBankRegs::AddSubBank(new EmRegsVZVisorEdge);
            EmBankRegs::AddSubBank(new EmRegsUSBVisor);
            break;

#endif

        default:
            break;
    }

    EmHAL::EnsureCoverage();
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::HasFlash
 *
 * DESCRIPTION: Returns whether or not this device uses Flash RAM.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    True if so.
 *
 ***********************************************************************/

Bool EmDevice::HasFlash(void) const {
    const DeviceInfo* info = this->GetDeviceInfo();
    return (info->fFlags & kHasFlash) != 0;
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::HardwareID
 *
 * DESCRIPTION: Returns the hardware ID of this device.  This ID is the
 *				one that eventually gets put into GHwrMiscGFlags by
 *				HwrIdentifyFeatures.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    The device's hardware ID.
 *
 ***********************************************************************/

long EmDevice::HardwareID(void) const {
    const DeviceInfo* info = this->GetDeviceInfo();

    //	EmAssert (info->fHardwareID != hwrMiscFlagIDNone);

    return info->fHardwareID;
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::HardwareSubID
 *
 * DESCRIPTION: Returns the hardware ID of this device.  This ID is the
 *				one that eventually gets put into GHwrMiscGFlags by
 *				HwrIdentifyFeatures.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    The device's hardware ID.
 *
 ***********************************************************************/

long EmDevice::HardwareSubID(void) const {
    const DeviceInfo* info = this->GetDeviceInfo();

    EmAssert(info->fHardwareSubID != hwrMiscFlagExtSubIDNone);

    return info->fHardwareSubID;
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::GetMenuString
 *
 * DESCRIPTION: .
 *
 * PARAMETERS:  None
 *
 * RETURNED:    .
 *
 ***********************************************************************/

string EmDevice::GetMenuString(void) const {
    const DeviceInfo* info = this->GetDeviceInfo();

    return string(info->fDeviceMenuName);
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::GetIDString
 *
 * DESCRIPTION: .
 *
 * PARAMETERS:  None
 *
 * RETURNED:    .
 *
 ***********************************************************************/

string EmDevice::GetIDString(void) const {
    const DeviceInfo* info = this->GetDeviceInfo();

    return string(info->fDeviceNames[0]);
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::GetIDStrings
 *
 * DESCRIPTION: .
 *
 * PARAMETERS:  None
 *
 * RETURNED:    .
 *
 ***********************************************************************/

StringList EmDevice::GetIDStrings(void) const {
    StringList result;

    const DeviceInfo* info = this->GetDeviceInfo();

    size_t ii = 0;
    while (ii < countof(info->fDeviceNames) && info->fDeviceNames[ii]) {
        result.push_back(string(info->fDeviceNames[ii]));
        ++ii;
    }

    return result;
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::GetDeviceList
 *
 * DESCRIPTION: Return the list of devices Poser emulates.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Collection containing the results.
 *
 ***********************************************************************/

EmDeviceList EmDevice::GetDeviceList(void) {
    EmDeviceList result;

    for (size_t ii = 0; ii < countof(kDeviceInfo); ++ii) {
        const DeviceInfo* info = &kDeviceInfo[ii];

        result.push_back(EmDevice(info->fDeviceID));
    }

    return result;
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::GetDeviceInfo
 *
 * DESCRIPTION: Look up and return the DeviceInfo block for the
 *				managed device.
 *
 * PARAMETERS:  None
 *
 * RETURNED:    Const pointer to the DeviceInfo block.
 *
 ***********************************************************************/

const DeviceInfo* EmDevice::GetDeviceInfo(void) const {
    for (size_t ii = 0; ii < countof(kDeviceInfo); ++ii) {
        const DeviceInfo* info = &kDeviceInfo[ii];

        if (info->fDeviceID == fDeviceID) return info;
    }

    return NULL;
}

/***********************************************************************
 *
 * FUNCTION:    EmDevice::GetDeviceID
 *
 * DESCRIPTION: Look up and return the unique ID for the
 *				managed device.
 *
 * PARAMETERS:  s - a string identifying the device.  Can be any of the
 *					strings in the fDeviceNames in the DeviceInfo array.
 *					The comparison to this string is case-insensitive.
 *
 * RETURNED:    The device's unique ID.  kDeviceUnspecified if a match
 *				could not be found.
 *
 ***********************************************************************/

int EmDevice::GetDeviceID(const char* s) const {
    // Iterate over each entry in the kDeviceInfo array.

    for (size_t ii = 0; ii < countof(kDeviceInfo); ++ii) {
        const DeviceInfo* info = &kDeviceInfo[ii];

        // Iterate over each string in the fDeviceNames array.

        size_t jj = 0;
        while (jj < countof(info->fDeviceNames) && info->fDeviceNames[jj]) {
            // If we find a match, return the unique ID.

            if (strcasecmp(info->fDeviceNames[jj], s) == 0) {
                return info->fDeviceID;
            }

            ++jj;
        }
    }

    // No match.

    return kDeviceUnspecified;
}

void EmDevice::ConfigureMemoryRegions() {
    memoryRegionMap.AllocateRegion(MemoryRegion::ram, MinRAMSize() * 1024);

    switch (fDeviceID) {
        case kDevicePEGS500C:
        case kDevicePalmIIIc:
            memoryRegionMap.AllocateRegion(MemoryRegion::framebuffer,
                                           EmRegsSED1375::FRAMEBUFFER_SIZE);
            break;

        case kDevicePalmM505:
        case kDevicePalmM515:
        case kDeviceVisorPrism:
            memoryRegionMap.AllocateRegion(MemoryRegion::framebuffer,
                                           EmRegsSED1376::FRAMEBUFFER_SIZE);
            break;

        case kDevicePalmPacifiC:
        case kDevicePalmI710:
        case kDevicePalmM130:
        case kDevicePalmM520:
        case kDeviceHandEra330c:
        case kDevicePEGN600C:
        case kDevicePEGT600:
        case kDevicePEGN700C:
        case kDeviceYSX1230:
        case kDeviceAcerS65:
            memoryRegionMap.AllocateRegion(MemoryRegion::framebuffer,
                                           EmRegsMediaQ11xx::FRAMEBUFFER_SIZE);
            break;

        case kDeviceYSX1100:
            memoryRegionMap.AllocateRegion(MemoryRegion::framebuffer,
                                           EmRegsMQLCDControlT2::FRAMEBUFFER_SIZE);
            break;
    }

    if (IsClie() || fDeviceID == kDeviceAcerS11 || fDeviceID == kDeviceAcerS65)
        memoryRegionMap.AllocateRegion(MemoryRegion::memorystick, MemoryStick::BLOCK_MAP_SIZE);

    switch (fDeviceID) {
        case kDevicePEGN700C:
        case kDeviceYSX1230:
        case kDeviceYSX1100:
            memoryRegionMap.AllocateRegion(MemoryRegion::sonyDsp,
                                           EmRegsSonyDSP::ADDRESS_SPACE_SIZE);
            break;
    }
}

bool EmDevice::IsValid() const { return fDeviceID != kDeviceUnspecified; }

RAMSizeType EmDevice::MinRAMSize(void) const {
    const DeviceInfo* info = this->GetDeviceInfo();

    return info ? info->fMinRAMSize : 0;
}

uint32 EmDevice::FramebufferSize() const {
    uint32 framebufferSizeBytes = memoryRegionMap.GetRegionSize(MemoryRegion::framebuffer);
    EmAssert(framebufferSizeBytes % 1024 == 0);

    return framebufferSizeBytes / 1024;
}

uint32 EmDevice::TotalMemorySize() const {
    uint32 sizeBytes = memoryRegionMap.GetTotalSize();
    EmAssert(sizeBytes % 1024 == 0);

    return sizeBytes / 1024;
}

MemoryRegionMap& EmDevice::GetMemoryRegionMap() { return memoryRegionMap; }

bool EmDevice::EmulatesDockStatus() const {
    if (Supports68328()) return false;

    switch (fDeviceID) {
        case kDevicePalmM130:
        case kDevicePalmI705:
        case kDevicePalmI710:
        case kDevicePalmPacifiC:
        case kDeviceAcerS11:
        case kDeviceAcerS65:
            return false;

        default:
            return true;
    }
}

bool EmDevice::NeedsSDCTLHack() const { return fDeviceID == kDevicePalmM515; }

bool EmDevice::HasCustomDigitizerTransform() const {
    return fDeviceID == kDeviceHandEra330 || fDeviceID == kDeviceHandEra330c ||
           fDeviceID == kDeviceYSX1100;
}

bool EmDevice::IsClie() const {
    switch (fDeviceID) {
        case kDevicePEGS300:
        case kDevicePEGS320:
        case kDevicePEGT400:
        case kDevicePEGN600C:
        case kDevicePEGT600:
        case kDevicePEGN700C:
        case kDeviceYSX1230:
        case kDeviceYSX1100:
        case kDevicePEGS500C:
            return true;

        default:
            return false;
    }
}

bool EmDevice::NeedsBatteryPatch() const { return Supports68328() || IsClie(); }

bool EmDevice::NeedsBatteryGlobalsHack() const {
    switch (fDeviceID) {
        case kDeviceYSX1230:
        case kDeviceYSX1100:
        case kDevicePEGN700C:
        case kDeviceAcerS65:
        case kDeviceAcerS11:
            return true;

        default:
            return false;
    }
}

int EmDevice::DigitizerScale() const {
    switch (fDeviceID) {
        case kDevicePEGT400:
        case kDevicePEGN600C:
        case kDevicePEGT600:
        case kDevicePEGN700C:
        case kDeviceYSX1230:
        case kDeviceYSX1100:
            return 2;

        default:
            return 1;
    }
}

bool EmDevice::BuggySleep() const {
    switch (fDeviceID) {
        case kDeviceYSX1100:
        case kDevicePalmPacifiC:
        case kDevicePalmI710:
        case kDeviceYSX1230:
            return true;

        default:
            return false;
    }
}

ScreenDimensions::Kind EmDevice::GetScreenDimensions() const {
    switch (fDeviceID) {
        case kDevicePalmI710:
        case kDevicePalmPacifiC:
        case kDevicePEGT400:
        case kDevicePalmM520:
        case kDevicePEGN600C:
        case kDevicePEGT600:
        case kDevicePEGN700C:
        case kDeviceYSX1230:
        case kDeviceAcerS65:
            return ScreenDimensions::screen320x320;

        case kDeviceHandEra330:
        case kDeviceHandEra330c:
            return ScreenDimensions::screen240x320;

        case kDeviceYSX1100:
            return ScreenDimensions::screen320x480;

        default:
            return ScreenDimensions::screen160x160;
    }
}

bool EmDevice::SupportsHardBtnCradle() const {
    switch (fDeviceID) {
        case kDeviceYSX1100:
        case kDeviceYSX1230:
            return false;

        default:
            return true;
    }
}
