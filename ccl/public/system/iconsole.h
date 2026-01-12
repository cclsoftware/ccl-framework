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
// Filename    : ccl/public/system/iconsole.h
// Description : Console Interface
//
//************************************************************************************************

#ifndef _ccl_iconsole_h
#define _ccl_iconsole_h

#include "ccl/public/system/alerttypes.h"

namespace CCL {
namespace System {

//************************************************************************************************
// System::IConsole
/** Console interface for character-mode applications.
	\ingroup ccl_system */
//************************************************************************************************

interface IConsole: Alert::IReporter
{	
	/** Redirect to other console. */
	virtual tbool CCL_API redirect (IConsole* console) = 0;

	/** Write Unicode String. */
	virtual tbool CCL_API writeLine (StringRef text) = 0;
		
	/** Write ASCII-encoded C-String. */
	virtual tbool CCL_API writeLine (const char* text) = 0;

	/** Read Unicode String. */
	virtual tbool CCL_API readLine (String& text) = 0;

	DECLARE_IID (IConsole)
};

DEFINE_IID (IConsole, 0x9f1255e0, 0x9bbc, 0x4d81, 0x8e, 0x6e, 0xdc, 0x42, 0xb8, 0xae, 0x4a, 0x6e)

} // namespace System
} // namespace CCL

#endif // _ccl_iconsole_h
