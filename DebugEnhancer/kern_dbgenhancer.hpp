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
	
	/**
	 *  Original method
	 */
	using t_vprintf = int (*) (const char *fmt, va_list ap);
	t_vprintf vprintf {nullptr};
	
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
