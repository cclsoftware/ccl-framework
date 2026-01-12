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
// Filename    : ccl/main/cclmodmain.h
// Description : Module Main Definitions
//
//************************************************************************************************

#ifndef _ccl_modmain_h
#define _ccl_modmain_h

#include "ccl/public/base/platform.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// CCLModuleMain
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Reason code for CCLModuleMain. */
DEFINE_ENUM (ModuleEntryReason)
{
	kModuleInit = 1,	///< Module loaded by host process
	kModuleExit			///< Module unloaded by host process
};

extern "C"
{	
	/** "CCLModuleMain": module has been loaded or unloaded (mandatory!). */
	typedef tbool (CCL_API *CCLModuleMainProc) (ModuleRef module, ModuleEntryReason reason);
}

} // namespace CCL

#endif // _ccl_modmain_h
