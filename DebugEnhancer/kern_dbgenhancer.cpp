//
//  kern_dbgenhancer.cpp
//  DebugEnhancer
//
//  Copyright © 2019 lvs1974. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>

#include "kern_dbgenhancer.hpp"

static DBGENH *callbackDBGENH = nullptr;


bool DBGENH::init()
{
	callbackDBGENH = this;

	lilu.onPatcherLoadForce(
	[](void *user, KernelPatcher &patcher) {
		callbackDBGENH->processKernel(patcher);
	}, this);

	return true;
}

void DBGENH::deinit()
{
}

//==============================================================================

int DBGENH::kdb_printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	callbackDBGENH->vprintf(fmt, ap);
	va_end(ap);
	return 0;
}

//==============================================================================

void DBGENH::kprintf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	callbackDBGENH->vprintf(fmt, ap);
	va_end(ap);
}

//==============================================================================

void DBGENH::processKernel(KernelPatcher &patcher)
{
	if (!(progressState & ProcessingState::KernelRouted))
	{
		vprintf = reinterpret_cast<t_vprintf>(patcher.solveSymbol(KernelPatcher::KernelID, "_vprintf"));
		if (vprintf) {
			KernelPatcher::RouteRequest requests[] = {
				{"_kdb_printf", kdb_printf},
				{"_kprintf", kprintf}
			};
			if (!patcher.routeMultiple(KernelPatcher::KernelID, requests, arrsize(requests)))
				SYSLOG("DBGENH", "patcher.routeMultiple for %s is failed with error %d", requests[0].symbol, patcher.getError());
		} else
			SYSLOG("DBGENH", "Symbol _vprintf cannot be resolved with error %d", patcher.getError());
		
		progressState |= ProcessingState::KernelRouted;
	}

	// Ignore all the errors for other processors
	patcher.clearError();
}
