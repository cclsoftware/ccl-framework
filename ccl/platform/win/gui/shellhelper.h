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
// Filename    : ccl/platform/win/gui/shellhelper.h
// Description : Shell Helper
//
//************************************************************************************************

#ifndef _ccl_shellhelper_h
#define _ccl_shellhelper_h

#include "ccl/platform/win/system/cclcom.h"

#include <shobjidl.h>

namespace CCL {
namespace Win32 {

//************************************************************************************************
// ShellHelper
//************************************************************************************************

class ShellHelper
{
public:
	static void initialize ();

	static String getAppUserModelID ();
	
	static IShellLink* createLink (StringRef path, StringRef arguments, StringRef title);
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_shellhelper_h
