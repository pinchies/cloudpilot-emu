/* -*- mode: C++; tab-width: 4 -*- */
/* ===================================================================== *\
        Copyright (c) 2000-2001 Palm, Inc. or its subsidiaries.
        All rights reserved.

        This file is part of the Palm OS Emulator.

        This program is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 2 of the License, or
        (at your option) any later version.
\* ===================================================================== */

#ifndef EmHAL_h
#define EmHAL_h

#include <algorithm>

#include "ButtonEvent.h"
#include "CardImage.h"
#include "EmCommon.h"
#include "EmEvent.h"

class EmHAL;
class EmPixMap;
struct Frame;

enum { kLEDOff = 0x00, kLEDGreen = 0x01, kLEDRed = 0x02 };

enum EmUARTDeviceType {
    kUARTBegin = 0,

    kUARTSerial = kUARTBegin,
    kUARTIR,
    kUARTBluetooth,
    kUARTMystery,
    kUARTNone,

    kUARTEnd
};

DEFINE_SCALAR_MODIFIERS(EmUARTDeviceType)

class EmHALHandler;

class EmHAL {
   public:
    enum class Slot { none = -1, sdcard = 0, memorystick = 1 };
    constexpr static Slot MAX_SLOT = Slot::memorystick;

   public:
    typedef void (*CycleHandler)(void*, uint64, bool);

   public:
    static void AddHandler(EmHALHandler*);
    static void RemoveHandler(EmHALHandler*);
    static void EnsureCoverage(void);

    static void CycleSlowly(Bool sleeping);

    static void DispatchButtonEvent(ButtonEvent evt);
    static void TurnSoundOff(void);
    static void ResetTimer(void);
    static void ResetRTC(void);

    static int32 GetInterruptLevel(void);
    static int32 GetInterruptBase(void);

    static Bool GetLCDScreenOn(void);
    static Bool GetLCDBacklightOn(void);
    static Bool GetLCDHasFrame(void);
    static bool CopyLCDFrame(Frame& frame, bool fullRefresh = false);
    static uint16 GetLCD2bitMapping();

    static int32 GetDynamicHeapSize(void);
    static int32 GetROMSize(void);
    static emuptr GetROMBaseAddress(void);
    static Bool ChipSelectsConfigured(void);
    static int32 GetSystemClockFrequency(void);
    static Bool GetAsleep(void);

    static uint8 GetPortInputValue(int);
    static uint8 GetPortInternalValue(int);
    static void PortDataChanged(int, uint8, uint8);
    static void GetKeyInfo(int* numRows, int* numCols, uint16* keyMap, Bool* rows);

    static void LineDriverChanged(EmUARTDeviceType);
    static Bool GetLineDriverState(EmUARTDeviceType);
    static EmUARTDeviceType GetUARTDevice(int uartNum);

    static void GetLineDriverStates(Bool* states);
    static void CompareLineDriverStates(const Bool* oldStates);

    static Bool GetDTR(int uartNum);
    static void DTRChanged(int uartNum);

    static Bool GetVibrateOn(void);
    static uint16 GetLEDState(void);

    static uint32 CyclesToNextInterrupt(uint64 systemCycles);
    static bool EnableRAM();

    static EmEvent<> onSystemClockChange;
    static EmEvent<double, double> onPwmChange;
    static EmEvent<> onDayRollover;

    static void AddCycleConsumer(CycleHandler handler, void* context);
    static void RemoveCycleConsumer(CycleHandler handler, void* context);
    static void DispatchCycle(uint64 cycles, bool sleeping);

    static bool SupportsImageInSlot(Slot slot, uint32 blocksTotal);
    static bool SupportsImageInSlot(Slot slot, const CardImage& cardImage);
    static void Mount(Slot slot, CardImage& cardImage);
    static void Unmount(Slot slot);
    static void Remount(Slot slot, CardImage& cardImage);

   private:
    struct CycleConsumer {
        CycleHandler handler;
        void* context;
    };

   private:
    static EmHALHandler* GetRootHandler(void) { return fgRootHandler; }
    static EmHALHandler* fgRootHandler;

    static vector<CycleConsumer> cycleConsumers;
};

class EmHALHandler {
   public:
    EmHALHandler(void);
    virtual ~EmHALHandler(void);

    virtual void CycleSlowly(Bool sleeping);

    virtual void DispatchButtonEvent(ButtonEvent evt);
    virtual void TurnSoundOff(void);
    virtual void ResetTimer(void);
    virtual void ResetRTC(void);

    virtual int32 GetInterruptLevel(void);
    virtual int32 GetInterruptBase(void);

    virtual Bool GetLCDScreenOn(void);
    virtual Bool GetLCDBacklightOn(void);
    virtual Bool GetLCDHasFrame(void);
    virtual bool CopyLCDFrame(Frame& frame, bool fullRefresh);
    virtual uint16 GetLCD2bitMapping();

    virtual int32 GetDynamicHeapSize(void);
    virtual int32 GetROMSize(void);
    virtual emuptr GetROMBaseAddress(void);
    virtual Bool ChipSelectsConfigured(void);
    virtual int32 GetSystemClockFrequency(void);
    virtual Bool GetAsleep(void);

    virtual uint8 GetPortInputValue(int);
    virtual uint8 GetPortInternalValue(int);
    virtual void PortDataChanged(int, uint8, uint8);
    virtual void GetKeyInfo(int* numRows, int* numCols, uint16* keyMap, Bool* rows);

    virtual Bool GetLineDriverState(EmUARTDeviceType);
    virtual EmUARTDeviceType GetUARTDevice(int uartNum);

    virtual Bool GetDTR(int uartNum);

    virtual Bool GetVibrateOn(void);
    virtual uint16 GetLEDState(void);

    virtual uint32 CyclesToNextInterrupt(uint64 systemCycles);
    virtual bool EnableRAM();

    virtual bool SupportsImageInSlot(EmHAL::Slot slot, uint32 blocksTotal);
    virtual void Mount(EmHAL::Slot slot, CardImage& cardImage);
    virtual void Unmount(EmHAL::Slot slot);
    virtual void Remount(EmHAL::Slot slot, CardImage& cardImage);

   protected:
    EmHALHandler* GetNextHandler(void) { return fNextHandler; }

   private:
    EmHALHandler* fNextHandler;
    EmHALHandler* fPrevHandler;

    friend class EmHAL;
};

#endif /* EmHAL_h */
