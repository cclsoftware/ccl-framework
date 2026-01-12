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
// Filename    : ccl/platform/android/androidmain.h
// Description : Android Application Entry
//
//************************************************************************************************

#ifndef _ccl_androidmain_h
#define _ccl_androidmain_h

#include "ccl/public/base/platform.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// CCLAndroidMain
//////////////////////////////////////////////////////////////////////////////////////////////////

extern "C"
{	
	/** "CCLAndroidMain": main function in application module. */
	typedef int (CCL_API *CCLAndroidMain) (ModuleRef module, tbool startup);
}

} // namespace CCL

#endif // _ccl_androidmain_h
