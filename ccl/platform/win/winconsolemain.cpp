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
// Filename    : ccl/platform/win/winconsolemain.cpp
// Description : Windows Console Application Entry
//
//************************************************************************************************

#include "ccl/main/cclargs.h"
#include "ccl/platform/win/cclwindows.h"

extern int __ccl_main (CCL::ModuleRef module, const CCL::PlatformArgs& args);

//////////////////////////////////////////////////////////////////////////////////////////////////
// main/wmain
//////////////////////////////////////////////////////////////////////////////////////////////////

#if UNICODE
int wmain (int argc, wchar_t* argv[])
{
	#if DEBUG
	::_set_error_mode (_OUT_TO_MSGBOX); // display assert dialog box instead of aborting the process
	#endif

	HINSTANCE hInstance = ::GetModuleHandle (nullptr);
	return __ccl_main (hInstance, CCL::PlatformArgs (argc, argv));
}

#else
int main (int argc, char* argv[])
{
	#if DEBUG
	::_set_error_mode (_OUT_TO_MSGBOX); // display assert dialog box instead of aborting the process
	#endif

	HINSTANCE hInstance = ::GetModuleHandle (nullptr);
	return __ccl_main (hInstance, CCL::PlatformArgs (argc, argv));
}
#endif
