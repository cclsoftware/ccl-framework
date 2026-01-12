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
// Filename    : ccl/gui/windows/systemwindow.h
// Description : System Window
//
//************************************************************************************************

#ifndef _ccl_systemwindow_h
#define _ccl_systemwindow_h

#include "ccl/gui/windows/nativewindow.h"

namespace CCL {

//************************************************************************************************
// SystemWindow
/** Represents an existing OS window. */
//************************************************************************************************

class SystemWindow: public NativeWindow
{
public:
	DECLARE_CLASS (SystemWindow, NativeWindow)

	SystemWindow (void* nativeHandle = nullptr);
};

//************************************************************************************************
// ModalSystemWindow
/** Represents an existing modal OS window. */
//************************************************************************************************

class ModalSystemWindow: public SystemWindow
{
public:
	ModalSystemWindow (void* nativeHandle = nullptr);
	~ModalSystemWindow ();
};

} // namespace CCL

#endif // _ccl_systemwindow_h
