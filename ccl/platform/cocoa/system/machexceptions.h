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
// Filename    : ccl/platform/cocoa/system/machexceptions.h
// Description : Mach Exception Handler (based on Mac OS X Internals by Amit Singh, Chapter 9.7)
//
//************************************************************************************************

#ifndef _ccl_machexceptions_h
#define _ccl_machexceptions_h

#include "ccl/public/text/cclstring.h"

namespace MacOS {

//************************************************************************************************
// MachExceptionHandler
//************************************************************************************************

class MachExceptionHandler
{
public:
	static void install ();
	static void remove ();
	
	static void setMessageContext (CCL::StringRef message);

protected:
	static CCL::String messageContext;
};

} // namespace MacOS

#endif // _ccl_machexceptions_h
