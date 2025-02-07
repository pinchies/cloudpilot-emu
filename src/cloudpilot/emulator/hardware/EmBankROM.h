/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
        Copyright (c) 1998-2001 Palm, Inc. or its subsidiaries.
        All rights reserved.

        This file is part of the Palm OS Emulator.

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2 of the License, or
        (at your option) any later version.
\* ===================================================================== */

#ifndef EmBankROM_h
#define EmBankROM_h

#include "EmCommon.h"

class EmStream;
const emuptr kDefaultROMMemoryStart = 0x10C00000;

class EmBankROM {
   public:
    static bool Initialize(size_t len, const uint8* buffer);
    static void Reset(Bool hardwareReset);
    static void Dispose(void);

    static void SetBankHandlers(void);

    static uint32 GetLong(emuptr address);
    static uint32 GetWord(emuptr address);
    static uint32 GetByte(emuptr address);
    static void SetLong(emuptr address, uint32 value);
    static void SetWord(emuptr address, uint32 value);
    static void SetByte(emuptr address, uint32 value);
    static int ValidAddress(emuptr address, uint32 size);
    static uint8* GetRealAddress(emuptr address);
    static uint8* GetMetaAddress(emuptr address);
    static void AddOpcodeCycles(void);

    static emuptr GetMemoryStart(void) { return gROMMemoryStart; }
    static uint32 GetRomSize();

   private:
    static void AddressError(emuptr address, long size, Bool forRead);
    static void InvalidAccess(emuptr address, long size, Bool forRead);
    static bool LoadROM(size_t len, const uint8* buffer);

    static emuptr gROMMemoryStart;
};

class EmBankFlash {
   public:
    static void Initialize(void);
    static void Reset(Bool hardwareReset);
    static void Dispose(void);

    static void SetBankHandlers(void);

    static uint32 GetLong(emuptr address);
    static uint32 GetWord(emuptr address);
    static uint32 GetByte(emuptr address);
    static void SetLong(emuptr address, uint32 value);
    static void SetWord(emuptr address, uint32 value);
    static void SetByte(emuptr address, uint32 value);
    static int ValidAddress(emuptr address, uint32 size);
    static uint8* GetRealAddress(emuptr address);
    static uint8* GetMetaAddress(emuptr address);
    static void AddOpcodeCycles(void);

    static emuptr GetMemoryStart(void) { return kDefaultROMMemoryStart; }
};

#endif /* EmBankROM_h */
