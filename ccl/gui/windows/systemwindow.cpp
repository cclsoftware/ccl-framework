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
// Filename    : ccl/gui/windows/systemwindow.cpp
// Description : System Window
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/systemwindow.h"
#include "ccl/gui/windows/desktop.h"

using namespace CCL;

//************************************************************************************************
// SystemWindow
//************************************************************************************************

DEFINE_CLASS (SystemWindow, NativeWindow)

//////////////////////////////////////////////////////////////////////////////////////////////////

SystemWindow::SystemWindow (void* nativeHandle)
: NativeWindow ()
{
	handle = nativeHandle;

	// could fill the various Window members (size, title, style, ...)
	if(nativeHandle != nullptr)
		fromNativeWindow (nativeHandle);
}

//************************************************************************************************
// ModalSystemWindow
//************************************************************************************************

ModalSystemWindow::ModalSystemWindow (void* nativeHandle)
: SystemWindow (nativeHandle)
{
	Desktop.addWindow (this, kDialogLayer);

	#if CCL_PLATFORM_WINDOWS
	// If we are the first modal window to open, we disable other windows
	Win32Dialog::beginModalMode (this, true);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ModalSystemWindow::~ModalSystemWindow ()
{
	#if CCL_PLATFORM_WINDOWS
	// If we are the last modal window to close, we enable other windows
	Win32Dialog::beginModalMode (this, false);
	#endif

	Desktop.removeWindow (this);
}
