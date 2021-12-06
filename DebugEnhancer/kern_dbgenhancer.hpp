//
//  kern_dbgenhancer.hpp
//  DebugEnhancer
//
//  Copyright © 2019 lvs1974. All rights reserved.
//

#ifndef kern_dbgenhancer_hpp
#define kern_dbgenhancer_hpp

#include <Headers/kern_patcher.hpp>

class DBGENH {
public:
	bool init();
	void deinit();
	
private:
	/**
	 *  Patch kernel
	 *
	 *  @param patcher KernelPatcher instance
	 */
	void processKernel(KernelPatcher &patcher);

	/**
	 *  Hooked methods / callbacks
	 */
	static int 			kdb_printf(const char *fmt, ...);
	static void			kprintf(const char *fmt, ...);
	static IOReturn     IOHibernateSystemSleep(void);
	static uint32_t	    hibernate_write_image(void);
	static void         IOLog(const char *fmt, ...);
	
	/**
	 *  Trampolines for original method invocations
	 */
	
	mach_vm_address_t orgIOHibernateSystemSleep {};
	mach_vm_address_t org_hibernate_write_image {};
	mach_vm_address_t orgIOLog {};
	
	/**
	 *  Original method
	 */
	using t_kern_vprintf = int (*) (const char *fmt, va_list ap);
	t_kern_vprintf kern_vprintf {nullptr};
	
	unsigned int *kernel_debug_entry_count {nullptr};
	SInt64 *gIOKitDebug {nullptr};
	
	using t_log_setsize = int (*) (int);
	t_log_setsize log_setsize {nullptr};
	/**
	 *  Current progress mask
	 */
	struct ProcessingState {
		enum {
			NothingReady = 0,
			KernelRouted = 1,
			EverythingDone = KernelRouted,
		};
	};
	int progressState {ProcessingState::NothingReady};
};

#endif /* kern_dbgenhancer_hpp */
