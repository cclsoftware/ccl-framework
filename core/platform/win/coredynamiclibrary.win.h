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
// Filename    : core/platform/win/coredynamiclibrary.win.h
// Description : Dynamic library Windows implementation
//
//************************************************************************************************

#ifndef _coredynamiclibrary_win_h
#define _coredynamiclibrary_win_h

#include "core/platform/shared/coreplatformdynamiclibrary.h"

#include "core/public/corevector.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// Win32DynamicLibrary
//************************************************************************************************

class Win32DynamicLibrary: public IDynamicLibrary
{
public:
	Win32DynamicLibrary ();
	~Win32DynamicLibrary ();

	/**
 	* \brief Adds a directory to the dll search path used for the next call of \a load.
	* \param dir The directory to add
	*/
	void addSearchPath (CStringPtr dir);

	// IDynamicLibrary
	ModuleRef getNativeReference () const override;
	void* getFunctionPointer (CStringPtr name) const override;
	void load (CStringPtr library) override;
	void unload () override;

protected:
	ModuleRef nativeRef;
	Vector<void*> directoryCookies;
};

typedef Win32DynamicLibrary DynamicLibrary;

} // namespace Platform
} // namespace Core

#endif // _coredynamiclibrary_win_h
