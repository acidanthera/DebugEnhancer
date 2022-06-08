//
//  kern_start.cpp
//  DebugEnhancer
//
//  Copyright © 2019 lvs1974. All rights reserved.
//

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>

#include "kern_dbgenhancer.hpp"

static DBGENH dbgenh;

static const char *bootargOff[] {
	"-dbgenhoff"
};

static const char *bootargDebug[] {
	"-dbgenhdbg"
};

static const char *bootargBeta[] {
	"-dbgenhbeta"
};

PluginConfiguration ADDPR(config) {
	xStringify(PRODUCT_NAME),
	parseModuleVersion(xStringify(MODULE_VERSION)),
	LiluAPI::AllowNormal | LiluAPI::AllowInstallerRecovery | LiluAPI::AllowSafeMode,
	bootargOff,
	arrsize(bootargOff),
	bootargDebug,
	arrsize(bootargDebug),
	bootargBeta,
	arrsize(bootargBeta),
	KernelVersion::MountainLion,
	KernelVersion::Ventura,
	[]() {
		dbgenh.init();
	}
};
