//
//  osx_defines.h
//  DebugEnhancer
//
//  Created by Sergey Lvov on 02.12.19.
//  Copyright © 2019 vit9696. All rights reserved.
//

#ifndef osx_defines_h
#define osx_defines_h

enum {
    // loggage
    kIOLogAttach        =         0x00000001ULL,
    kIOLogProbe         =         0x00000002ULL,
    kIOLogStart         =         0x00000004ULL,
    kIOLogRegister      =         0x00000008ULL,
    kIOLogMatch         =         0x00000010ULL,
    kIOLogConfig        =         0x00000020ULL,
    kIOLogYield         =         0x00000040ULL,
    kIOLogPower         =         0x00000080ULL,
    kIOLogMapping       =         0x00000100ULL,
    kIOLogCatalogue     =         0x00000200ULL,
	kIOLogTracePower    =         0x00000400ULL,  // Obsolete: Use iotrace=0x00000400ULL to enable now
    kIOLogDebugPower    =         0x00000800ULL,
    kIOLogServiceTree   =         0x00001000ULL,
    kIOLogDTree         =         0x00002000ULL,
    kIOLogMemory        =         0x00004000ULL,
    kIOLogKextMemory    =         0x00008000ULL,
    kOSLogRegistryMods  =         0x00010000ULL,  // Log attempts to modify registry collections
    kIOLogPMRootDomain  =         0x00020000ULL,
    kOSRegistryModsMode =         0x00040000ULL,  // Change default registry modification handling - panic vs. log
//    kIOTraceIOService   =         0x00080000ULL,  // Obsolete: Use iotrace=0x00080000ULL to enable now
    kIOLogHibernate     =         0x00100000ULL,
    kIOStatistics       =         0x04000000ULL,
    kIOSleepWakeWdogOff =         0x40000000ULL,
    kIOKextSpinDump     =         0x80000000ULL,

    // debug aids - change behaviour
    kIONoFreeObjects    =         0x00100000ULL,
//    kIOLogSynchronous   =         0x00200000ULL,  // IOLog completes synchronously -- obsolete
    kIOTracking         =         0x00400000ULL,
    kIOWaitQuietPanics  =         0x00800000ULL,
    kIOWaitQuietBeforeRoot =      0x01000000ULL,
    kIOTrackingBoot     =         0x02000000ULL,

    _kIODebugTopFlag    = 0x8000000000000000ULL   // force enum to be 64 bits
};
#endif /* osx_defines_h */
