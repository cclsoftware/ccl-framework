//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/base/initterm.h
// Description : Initializer/Terminator
//
//************************************************************************************************

#ifndef _ccl_initterm_h
#define _ccl_initterm_h

#include "ccl/public/base/platform.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization/Termination Macros
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Define initializer function. */
#define CCL_KERNEL_INIT(Name) \
bool kernelInit##Name (); \
CCL::KernelInitializer __##Name##KernelInit (kernelInit##Name, #Name); \
bool kernelInit##Name ()

/** Define initializer function with level. */
#define CCL_KERNEL_INIT_LEVEL(Name, level) \
bool kernelInit##Name (); \
CCL::KernelInitializer __##Name##KernelInit (kernelInit##Name, #Name, level); \
bool kernelInit##Name ()

/** Define termination function. */
#define CCL_KERNEL_TERM(Name) \
void kernelTerm##Name (); \
CCL::KernelTerminator __##Name##KernelTerm (kernelTerm##Name); \
void kernelTerm##Name ()

/** Define termination function with level. */
#define CCL_KERNEL_TERM_LEVEL(Name, level) \
void kernelTerm##Name (); \
CCL::KernelTerminator __##Name##KernelTerm (kernelTerm##Name, level); \
void kernelTerm##Name ()

//////////////////////////////////////////////////////////////////////////////////////////////////
// Run Levels
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Levels of initialization/termination phase. */
enum RunLevel
{
	kFrameworkLevelFirst   = 0,	///< (reserved for framework initialization)
	kFrameworkLevelSecond = 10,
	kFrameworkLevelLast   = 20,

	kFirstRun	= 100,		///< e.g. for loose binding of objects
	kSecondRun	= 1000,		///< default level
	kLastRun	= 10000,	///< after everything important is done

	kSetupLevel = kSecondRun - 200,	///< before application components are initialized
	kAppLevel   = kSecondRun - 100	///< application component level
};

//************************************************************************************************
// KernelInitializer
/** Define initialization function. */
//************************************************************************************************

struct KernelInitializer
{
	KernelInitializer (bool (*func)(), CStringPtr name, int level = kSecondRun);
	KernelInitializer ();

	bool (*func)();
	int level;
	CStringPtr name;
	
	bool operator == (const KernelInitializer&) const;
	bool operator > (const KernelInitializer&) const;
};

//************************************************************************************************
// KernelTerminator
/** Define termination function. */
//************************************************************************************************

struct KernelTerminator
{
	KernelTerminator (void (*func)(), int level = kSecondRun);
	KernelTerminator ();

	void (*func)();
	int level;

	bool operator == (const KernelTerminator&) const;
	bool operator > (const KernelTerminator&) const;
};

} // namespace CCL

#endif // _ccl_initterm_h
