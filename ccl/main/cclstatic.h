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
// Filename    : cclstatic.h
// Description : Framework Initialization (static linkage)
//
//************************************************************************************************

#ifndef _ccl_cclstatic_h
#define _ccl_cclstatic_h

#if !CCL_STATIC_LINKAGE
#error "This file is for static CCL linkage only!"
#endif

namespace CCL {

class ClassFactory;

//************************************************************************************************
// FrameworkInitializerStatic
//************************************************************************************************

class FrameworkInitializerStatic
{
public:
	FrameworkInitializerStatic ();

	bool initializeFrameworkLevel ();
	void terminateApplicationLevel ();
	void terminateFrameworkLevel ();
	
protected:
	ClassFactory* classFactory;
	bool success;

	void registerClasses ();
	void unregisterClasses ();
};

} // namespace CCL

#endif // _ccl_cclstatic_h
