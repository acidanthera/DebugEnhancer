//
//  kern_start.cpp
//  BT4LEContinuityFixup
//
//  Copyright © 2017 lvs1974. All rights reserved.
//

#include <Headers/plugin_start.hpp>
#include <Headers/kern_api.hpp>

#include "kern_dbgenhancer.hpp"

static BT4LEFX bt4lefx;

static const char *bootargOff[] {
	"-bt4lefxoff"
};

static const char *bootargDebug[] {
	"-bt4lefxdbg"
};

static const char *bootargBeta[] {
	"-bt4lefxbeta"
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
	KernelVersion::Catalina,
	[]() {
		bt4lefx.init();
	}
};
