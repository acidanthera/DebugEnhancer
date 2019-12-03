//
//  kern_dbgenhancer.cpp
//  DebugEnhancer
//
//  Copyright © 2019 lvs1974. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>

#include "kern_dbgenhancer.hpp"
#include "osx_defines.h"

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

IOReturn DBGENH::IOHibernateSystemSleep(void)
{
	unsigned int value = 0;
	if (callbackDBGENH->kernel_debug_entry_count) {
		value = *callbackDBGENH->kernel_debug_entry_count;
		*callbackDBGENH->kernel_debug_entry_count = 1;
	}
	
	IOReturn result = FunctionCast(IOHibernateSystemSleep, callbackDBGENH->orgIOHibernateSystemSleep)();
	SYSLOG("DBGENH", "IOHibernateSystemSleep is called, result is: %x", result);
		
	if (callbackDBGENH->kernel_debug_entry_count)
		*callbackDBGENH->kernel_debug_entry_count = value;
	return result;
}

//==============================================================================

uint32_t DBGENH::hibernate_write_image(void)
{
	unsigned int value = 0;
	if (callbackDBGENH->kernel_debug_entry_count) {
		value = *callbackDBGENH->kernel_debug_entry_count;
		*callbackDBGENH->kernel_debug_entry_count = 1;
	}
	printf("DBGENH @ hibernate_write_image is called\n");
	uint32_t result = FunctionCast(hibernate_write_image, callbackDBGENH->org_hibernate_write_image)();
	printf("DBGENH @ hibernate_write_image result is: %x\n", result);
	
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
		
		gIOKitDebug = reinterpret_cast<SInt64*>(patcher.solveSymbol(KernelPatcher::KernelID, "_gIOKitDebug"));
		if (gIOKitDebug) {
			*gIOKitDebug |= kIOLogPMRootDomain | kIOLogHibernate;
		}
		else
			SYSLOG("DBGENH", "Symbol _gIOKitDebug cannot be resolved with error %d", patcher.getError());
		patcher.getError();
				
		kern_vprintf = reinterpret_cast<t_kern_vprintf>(patcher.solveSymbol(KernelPatcher::KernelID, "_vprintf"));
		if (kern_vprintf) {
			KernelPatcher::RouteRequest requests[] = {
				{"_kdb_printf", kdb_printf},
				{"_kprintf", kprintf},
				{"_hibernate_write_image", hibernate_write_image, org_hibernate_write_image},
				{"_IOHibernateSystemSleep", IOHibernateSystemSleep, orgIOHibernateSystemSleep}
			};
			if (!patcher.routeMultiple(KernelPatcher::KernelID, requests, arrsize(requests)))
				SYSLOG("DBGENH", "patcher.routeMultiple for %s is failed with error %d", requests[0].symbol, patcher.getError());
		} else
			SYSLOG("DBGENH", "Symbol _vprintf cannot be resolved with error %d", patcher.getError());
		
		log_setsize = reinterpret_cast<t_log_setsize>(patcher.solveSymbol(KernelPatcher::KernelID, "_log_setsize"));
		if (log_setsize) {
			int error = 0;
			if ((error = log_setsize(1*1024*1024)))
				SYSLOG("DBGENH", "log_setsize could not change dmesg buffer size to 1*1024*1024, error: %d", error);
			
			// enable higher limit in log_setsize
			const uint8_t find[]    = {0x3D, 0xFF, 0xFF, 0x0F, 0x00, 0x0F};
			const uint8_t replace[] = {0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F};
			KernelPatcher::LookupPatch patch = {nullptr, find, replace, sizeof(find), 1};
			DBGLOG("DBGENH", "applying kernel patch");
			patcher.clearError();
			patcher.applyLookupPatch(&patch, reinterpret_cast<uint8_t *>(log_setsize), 50);
			if (patcher.getError() != KernelPatcher::Error::NoError)
				SYSLOG("DBGENH", "applyLookupPatch failed with error %d", patcher.getError());
			else if ((error = log_setsize(10*1024*1024)))
				SYSLOG("DBGENH", "log_setsize could not change dmesg buffer size to 10*1024*1024, erro %d", error);
		}
		else
			SYSLOG("DBGENH", "Symbol _log_setsize cannot be resolved with error %d", patcher.getError());

		progressState |= ProcessingState::KernelRouted;
	}

	// Ignore all the errors for other processors
	patcher.clearError();
}
