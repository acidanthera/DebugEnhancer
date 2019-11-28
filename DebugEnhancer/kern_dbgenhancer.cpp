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
	callbackDBGENH->kern_vprintf(fmt, ap);
	va_end(ap);
	return 0;
}

//==============================================================================

void DBGENH::kprintf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	callbackDBGENH->kern_vprintf(fmt, ap);
	va_end(ap);
}

//==============================================================================

uint32_t DBGENH::hibernate_write_image(void)
{
	unsigned int value = 0;
	if (callbackDBGENH->kernel_debug_entry_count) {
		value = *callbackDBGENH->kernel_debug_entry_count;
		*callbackDBGENH->kernel_debug_entry_count = 1;
	}
	uint32_t result = FunctionCast(hibernate_write_image, callbackDBGENH->org_hibernate_write_image)();
	if (callbackDBGENH->kernel_debug_entry_count)
		*callbackDBGENH->kernel_debug_entry_count = value;
	return result;
}

//==============================================================================

void DBGENH::processKernel(KernelPatcher &patcher)
{
	if (!(progressState & ProcessingState::KernelRouted))
	{
		kernel_debug_entry_count = reinterpret_cast<unsigned int *>(patcher.solveSymbol(KernelPatcher::KernelID, "_kernel_debugger_entry_count"));
		if (!kernel_debug_entry_count)
			SYSLOG("DBGENH", "Symbol _kernel_debugger_entry_count cannot be resolved with error %d", patcher.getError());
		patcher.getError();
		
		kern_vprintf = reinterpret_cast<t_kern_vprintf>(patcher.solveSymbol(KernelPatcher::KernelID, "_vprintf"));
		if (kern_vprintf) {
			KernelPatcher::RouteRequest requests[] = {
				{"_kdb_printf", kdb_printf},
				{"_kprintf", kprintf},
				{"_hibernate_write_image", hibernate_write_image, org_hibernate_write_image},
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
