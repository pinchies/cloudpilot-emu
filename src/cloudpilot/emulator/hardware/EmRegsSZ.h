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

#ifndef _EM_REGS_SZ_H_
#define _EM_REGS_SZ_H_

#include "ButtonEvent.h"
#include "EmEvent.h"
#include "EmHAL.h"             // EmHALHandler
#include "EmRegs.h"            // EmRegs
#include "EmStructs.h"         // RGBList
#include "EmUARTDragonball.h"  // EmUARTDragonball::State

class EmSPISlave;

class EmRegsSZ : public EmRegs, public EmHALHandler {
   public:
    EmRegsSZ(void);
    virtual ~EmRegsSZ(void);

    // EmRegs overrides
    virtual void Initialize(void);
    virtual void Reset(Bool hardwareReset);
    virtual void Dispose(void);

    virtual void Save(Savestate&);
    virtual void Save(SavestateProbe&);
    virtual void Load(SavestateLoader&);

    virtual void SetSubBankHandlers(void);
    virtual uint8* GetRealAddress(emuptr address);
    virtual emuptr GetAddressStart(void);
    virtual uint32 GetAddressRange(void);

    // EmHALHandler overrides
    virtual void CycleSlowly(Bool sleeping);

    virtual void DispatchButtonEvent(ButtonEvent evt);
    virtual void TurnSoundOff(void);
    virtual void ResetTimer(void);
    virtual void ResetRTC(void);

    virtual int32 GetInterruptLevel(void);
    virtual int32 GetInterruptBase(void);

    virtual Bool GetLCDScreenOn(void) = 0;
    virtual Bool GetLCDBacklightOn(void) = 0;
    virtual Bool GetLCDHasFrame(void);

    virtual int32 GetDynamicHeapSize(void);
    virtual int32 GetROMSize(void);
    virtual emuptr GetROMBaseAddress(void);
    virtual Bool ChipSelectsConfigured(void);
    virtual int32 GetSystemClockFrequency(void);
    virtual Bool GetAsleep(void);

    virtual uint8 GetPortInputValue(int);
    virtual uint8 GetPortInternalValue(int);
    virtual void PortDataChanged(int, uint8, uint8);
    virtual void GetKeyInfo(int* numRows, int* numCols, uint16* keyMap, Bool* rows) = 0;

    virtual uint32 CyclesToNextInterrupt(uint64 systemCycles);
    virtual bool EnableRAM();
    inline void Cycle(uint64 systemCycles, Bool sleeping);

   protected:
    void HotSyncEvent(Bool buttonIsDown);

    virtual uint8 GetKeyBits(void);
    virtual uint16 ButtonToBits(ButtonEvent::Button btn);
    virtual EmSPISlave* GetSPISlave(void);

    void UpdateInterrupts(void);
    virtual void UpdatePortXInterrupts(char);
    void UpdateRTCInterrupts(void);

    void UARTStateChanged(Bool sendTxData, int uartNum);
    void UpdateUARTState(Bool refreshRxData, int uartNum);
    void UpdateUARTInterrupts(const EmUARTDragonball::State& state, int uartNum);
    void MarshalUARTState(EmUARTDragonball::State& state, int uartNum);
    void UnmarshalUARTState(const EmUARTDragonball::State& state, int uartNum);

    int GetPortFromAddress(emuptr address);
    emuptr GetAddressFromPort(int port);

   private:
    uint32 portXDataRead(emuptr address, int size);
    uint32 tmr1StatusRead(emuptr address, int size);
    uint32 tmr2StatusRead(emuptr address, int size);
    uint32 tmrRegisterRead(emuptr address, int size);
    uint32 uart1Read(emuptr address, int size);
    uint32 uart2Read(emuptr address, int size);
    uint32 rtcHourMinSecRead(emuptr address, int size);
    uint32 rtcDayRead(emuptr address, int size);

    void portDIntReqEnWrite(emuptr address, int size, uint32 value);
    void csControl1Write(emuptr address, int size, uint32 value);
    void csASelectWrite(emuptr address, int size, uint32 value);
    void csESelectWrite(emuptr address, int size, uint32 value);
    void intMaskHiWrite(emuptr address, int size, uint32 value);
    void intMaskLoWrite(emuptr address, int size, uint32 value);
    void intStatusHiWrite(emuptr address, int size, uint32 value);
    void portXDataWrite(emuptr address, int size, uint32 value);
    void portXIntMaskWrite(emuptr address, int size, uint32 value);
    void portXIntStatusWrite(emuptr address, int size, uint32 value);
    void tmr1StatusWrite(emuptr address, int size, uint32 value);
    void tmr2StatusWrite(emuptr address, int size, uint32 value);
    void spiCont1Write(emuptr address, int size, uint32 value);
    void spiMasterControlWrite(emuptr address, int size, uint32 value);
    void tmrRegisterWrite(emuptr address, int size, uint32 value);
    void uart1Write(emuptr address, int size, uint32 value);
    void uart2Write(emuptr address, int size, uint32 value);
    void rtcControlWrite(emuptr address, int size, uint32 value);
    void rtcIntStatusWrite(emuptr address, int size, uint32 value);
    void rtcIntEnableWrite(emuptr address, int size, uint32 value);
    void pllRegisterWrite(emuptr address, int size, uint32 value);
    void adcIntControlWrite(emuptr address, int size, uint32 value);
    void sdramControlEWrite(emuptr address, int size, uint32 value);
    void pwmc1Write(emuptr address, int size, uint32 value);
    void pwms1Write(emuptr address, int size, uint32 value);
    void pwmp1Write(emuptr address, int size, uint32 value);

    void UpdateTimers();
    void HandleDayRollover();
    void DispatchPwmChange();
    int32 GetSysClk();

    template <typename T>
    void DoSave(T& savestate);

    template <typename T>
    void DoSaveLoad(T& helper, uint32 version);

   protected:
    HwrM68SZ328Type f68SZ328Regs;
    bool fHotSyncButtonDown;
    uint16 fKeyBits;
    uint16 fLastTmr1Status;
    uint16 fLastTmr2Status;
    uint8 fPortXEdge['R' - 'D' + 1];
    uint32 fPortDDataCount;

    EmEvent<>::HandleT onDayRolloverHandle;

    double tmr1LastProcessedSystemCycles;
    double tmr2LastProcessedSystemCycles;
    uint64 nextTimerEventAfterCycle;
    uint64 systemCycles;

    int32 lastRtcAlarmCheck{-1};

    bool pwmActive{false};
    bool afterLoad{false};
    bool powerOffCached{false};

    EmUARTDragonball* fUART[2];
    EmSPISlave* fSPISlaveADC{nullptr};
};

#endif  // _EM_REGS_SZ_H_
