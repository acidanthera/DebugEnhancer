//
//  kern_bt4lefx.cpp
//  BT4LEContinuityFixup
//
//  Copyright © 2017 lvs1974. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>

#include "kern_dbgenhancer.hpp"

static BT4LEFX *callbackBT4LEFX = nullptr;

static const char *kextIOBluetoothFamily[] { "/System/Library/Extensions/IOBluetoothFamily.kext/Contents/MacOS/IOBluetoothFamily" };

static KernelPatcher::KextInfo kextIOBluetooth { "com.apple.iokit.IOBluetoothFamily", kextIOBluetoothFamily, 1, {true, true}, {}, KernelPatcher::KextInfo::Unloaded };

bool BT4LEFX::init()
{
	callbackBT4LEFX = this;

	lilu.onKextLoadForce(&kextIOBluetooth, 1,
	[](void *user, KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
		callbackBT4LEFX->processKext(patcher, index, address, size);
	}, this);

	return true;
}

void BT4LEFX::deinit()
{
}

int BT4LEFX::AppleBroadcomBluetoothHostController_SetControllerFeatureFlags(void *that, unsigned int)
{
	DBGLOG("bt4lefx", "AppleBroadcomBluetoothHostController::SetControllerFeatureFlags is called");
	int result = FunctionCast(AppleBroadcomBluetoothHostController_SetControllerFeatureFlags,
							  callbackBT4LEFX->orgIOBluetoothHostController_SetControllerFeatureFlags)(that, 0x0F);
	DBGLOG("bt4lefx", "IOBluetoothHostController::SetControllerFeatureFlags returned %d", result);

	return result;
}

void BT4LEFX::processKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size)
{
	if (kextIOBluetooth.loadIndex == index) {
		DBGLOG("bt4lefx", "%s", kextIOBluetooth.id);

		if (getKernelVersion() > KernelVersion::Yosemite) {
			orgIOBluetoothHostController_SetControllerFeatureFlags = patcher.solveSymbol(index, "__ZN25IOBluetoothHostController25SetControllerFeatureFlagsEj", address, size);
			if (orgIOBluetoothHostController_SetControllerFeatureFlags) {
				DBGLOG("bt4lefx", "obtained __ZN25IOBluetoothHostController25SetControllerFeatureFlagsEj");

				KernelPatcher::RouteRequest request {
					"__ZN36AppleBroadcomBluetoothHostController25SetControllerFeatureFlagsEj",
					AppleBroadcomBluetoothHostController_SetControllerFeatureFlags
				};
				patcher.routeMultiple(index, &request, 1, address, size);
			} else {
				SYSLOG("bt4lefx", "failed to resolve __ZN25IOBluetoothHostController25SetControllerFeatureFlagsEj %d", patcher.getError());
				patcher.clearError();
			}
		}
		else {
			KernelPatcher::RouteRequest request {
				"__ZN24IOBluetoothHCIController25SetControllerFeatureFlagsEj",
				AppleBroadcomBluetoothHostController_SetControllerFeatureFlags,
				orgIOBluetoothHostController_SetControllerFeatureFlags
			};
			patcher.routeMultiple(index, &request, 1, address, size);
			if (patcher.getError() == KernelPatcher::Error::NoError) {
				DBGLOG("bt4lefx", "routed __ZN24IOBluetoothHCIController25SetControllerFeatureFlagsEj");
			} else {
				SYSLOG("bt4lefx", "failed to resolve __ZN24IOBluetoothHCIController25SetControllerFeatureFlagsEj %d", patcher.getError());
				patcher.clearError();
			}
		}
	}
}
