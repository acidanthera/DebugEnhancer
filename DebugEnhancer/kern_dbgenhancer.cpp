//
//  kern_dbgenhancer.cpp
//  DebugEnhancer
//
//  Copyright © 2019 lvs1974. All rights reserved.
//

#include <Headers/kern_api.hpp>
#include <Headers/kern_util.hpp>

#include <IOKit/IOKitDebug.h>

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

void DBGENH::IOLog(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	callbackDBGENH->kern_vprintf(fmt, ap);
	va_end(ap);
}

//==============================================================================

void DBGENH::processKernel(KernelPatcher &patcher)
{
	if (!(progressState & ProcessingState::KernelRouted))
	{
		kernel_debug_entry_count = reinterpret_cast<unsigned int *>(patcher.solveSymbol(KernelPatcher::KernelID, "_kernel_debugger_entry_count"));
		if (!kernel_debug_entry_count)
			SYSLOG("DBGENH", "Symbol _kernel_debugger_entry_count cannot be resolved with error %d", patcher.getError());
		patcher.clearError();
		
		gIOKitDebug = reinterpret_cast<SInt64*>(patcher.solveSymbol(KernelPatcher::KernelID, "_gIOKitDebug"));
		if (gIOKitDebug) {
			*gIOKitDebug |= kIOLogPMRootDomain | kIOLogHibernate;
		}
		else
			SYSLOG("DBGENH", "Symbol _gIOKitDebug cannot be resolved with error %d", patcher.getError());
		patcher.clearError();
				
		kern_vprintf = reinterpret_cast<t_kern_vprintf>(patcher.solveSymbol(KernelPatcher::KernelID, "_vprintf"));
		if (kern_vprintf) {
			KernelPatcher::RouteRequest requests[] = {
				{"_kdb_printf", kdb_printf},
				{"_kprintf", kprintf},
				{"_hibernate_write_image", hibernate_write_image, org_hibernate_write_image},
				{"_IOHibernateSystemSleep", IOHibernateSystemSleep, orgIOHibernateSystemSleep},
				{"_IOLog", IOLog, orgIOLog}
			};
			
			bool route_iolog = checkKernelArgument("-dbgenhiolog");
			size_t size = arrsize(requests) - (route_iolog ? 0 : 1);
			if (!patcher.routeMultipleLong(KernelPatcher::KernelID, requests, size))
				SYSLOG("DBGENH", "patcher.routeMultiple for %s is failed with error %d", requests[0].symbol, patcher.getError());
		} else
			SYSLOG("DBGENH", "Symbol _vprintf cannot be resolved with error %d", patcher.getError());
		
		log_setsize = reinterpret_cast<t_log_setsize>(patcher.solveSymbol(KernelPatcher::KernelID, "_log_setsize"));
		if (log_setsize) {
			const uint8_t find[]    = {0x3D, 0xFF, 0xFF, 0x0F, 0x00, 0x0F};
			const uint8_t replace[] = {0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F};
			KernelPatcher::LookupPatch patch = {nullptr, find, replace, sizeof(find), 1};
			DBGLOG("DBGENH", "applying kernel patch");
			patcher.clearError();
			patcher.applyLookupPatch(&patch, reinterpret_cast<uint8_t *>(log_setsize), 50);
			int new_logsize = MAX_MSG_BSIZE;
			if (patcher.getError() == KernelPatcher::Error::NoError)
				new_logsize = MAX_MSG_BSIZE*10;	// 10 MBytes
			else
				SYSLOG("DBGENH", "applyLookupPatch failed with error %d", patcher.getError());
			
			int error = 0;
			if ((error = log_setsize(new_logsize)))
				SYSLOG("DBGENH", "log_setsize could not change dmesg buffer size to %d, error: %d", new_logsize, error);
		}
		else
		{
			patcher.clearError();
			kalloc_data = reinterpret_cast<t_kalloc_data>(patcher.solveSymbol(KernelPatcher::KernelID, "_kalloc_data"));
			bsd_log_lock_safe = reinterpret_cast<t_bsd_log_lock_safe>(patcher.solveSymbol(KernelPatcher::KernelID, "_bsd_log_lock_safe"));
			bsd_log_unlock = reinterpret_cast<t_bsd_log_unlock>(patcher.solveSymbol(KernelPatcher::KernelID, "_bsd_log_unlock"));
			struct msgbuf *msgbufp = reinterpret_cast<struct msgbuf *>(patcher.solveSymbol(KernelPatcher::KernelID, "_msgbuf"));
			if (kalloc_data && bsd_log_lock_safe && bsd_log_unlock && msgbufp)
			{
				int new_logsize = MAX_MSG_BSIZE*10; // 10 MBytes
				char *new_logdata = kalloc_data(new_logsize, Z_WAITOK | Z_ZERO);
				if (new_logdata != nullptr)
				{
					int i = 0, count = 0;
					char *p = nullptr;

					bsd_log_lock_safe();
					
					PANIC_COND(msgbufp->msg_magic != MSG_MAGIC, "DBGENH", "msgbufp->msg_magic has a wrong magic value");
					
					char *old_logdata = msgbufp->msg_bufc;
					int old_logsize = msgbufp->msg_size;
					int old_bufr = msgbufp->msg_bufr;
					int old_bufx = msgbufp->msg_bufx;

					// start "new_logsize" bytes before the write pointer
					if (new_logsize <= old_bufx) {
						count = new_logsize;
						p = old_logdata + old_bufx - count;
					} else {
						 // if new buffer is bigger, copy what we have and let the bzero above handle the difference
						count = MIN(new_logsize, old_logsize);
						p = old_logdata + old_logsize - (count - old_bufx);
					}

					for (i = 0; i < count; i++) {
						if (p >= old_logdata + old_logsize)
							p = old_logdata;
						new_logdata[i] = *p++;
					}

					int new_bufx = i;
					if (new_bufx >= new_logsize)
						new_bufx = 0;
					msgbufp->msg_bufx = new_bufx;

					int new_bufr = old_bufx - old_bufr; // how much were we trailing bufx by?
					if (new_bufr < 0)
						new_bufr += old_logsize;
					
					new_bufr = new_bufx - new_bufr; // now relative to oldest data in new buffer
					if (new_bufr < 0)
						new_bufr += new_logsize;
					
					msgbufp->msg_bufr = new_bufr;
					msgbufp->msg_size = new_logsize;
					msgbufp->msg_bufc = new_logdata;

					bsd_log_unlock();
					
					// This memory is now dead - clear it so that it compresses better
					// in case of suspend to disk etc.
					bzero(old_logdata, old_logsize);
				}
				else
					SYSLOG("DBGENH", "kalloc_data could not allocate memory");
			}
			else
			{
				SYSLOG("DBGENH", "Symbol _log_setsize/_kalloc_data/_bsd_log_lock_safe/_bsd_log_unlock/_msgbuf cannot be resolved with error %d", patcher.getError());
			}
		}

		progressState |= ProcessingState::KernelRouted;
	}

	// Ignore all the errors for other processors
	patcher.clearError();
}
