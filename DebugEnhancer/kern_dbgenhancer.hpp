//
//  kern_dbgenhancer.hpp
//  DebugEnhancer
//
//  Copyright © 2019 lvs1974. All rights reserved.
//

#ifndef kern_dbgenhancer_hpp
#define kern_dbgenhancer_hpp

#include <Headers/kern_patcher.hpp>

#define MAX_MSG_BSIZE   (1*1024*1024)

enum zalloc_flags_t {
	Z_WAITOK        = 0x0000,
	Z_NOWAIT        = 0x0001,
	Z_NOPAGEWAIT    = 0x0002,
	Z_ZERO          = 0x0004,
    Z_NOFAIL        = 0x8000
};

#pragma pack(push,1)

struct  msgbuf {
#define MSG_MAGIC       0x063061
	int             msg_magic;
	int             msg_size;
	int             msg_bufx;               /* write pointer */
	int             msg_bufr;               /* read pointer */
	char           *msg_bufc;               /* buffer */
};

#pragma pack(pop)

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
	
	using t_kalloc_data = char * (*)(vm_size_t size, uint32_t flags);
	t_kalloc_data kalloc_data {nullptr};
	
	using t_bsd_log_lock_safe = void (*) ();
	t_bsd_log_lock_safe bsd_log_lock_safe {nullptr};

	using t_bsd_log_unlock = void (*) ();
	t_bsd_log_unlock bsd_log_unlock {nullptr};
	
	using t_kernel_sysctlbyname = int (*)(const char *, void *, size_t *, void *, size_t);
	t_kernel_sysctlbyname kernel_sysctlbyname {nullptr};
	
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
