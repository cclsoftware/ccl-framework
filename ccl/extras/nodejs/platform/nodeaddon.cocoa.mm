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
// Filename    : ccl/extras/nodejs/platform/nodeaddon.cocoa.cpp
// Description : Node addon platform code
//
//************************************************************************************************

#include "ccl/extras/nodejs/nodeaddon.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/storage/iurl.h"
#include "ccl/platform/cocoa/macutils.h"

#include <CoreFoundation/CoreFoundation.h>

using namespace CCL;
using namespace NodeJS;

//*************************************************************************************************
// NodeAddon
//*************************************************************************************************

ModuleRef NodeAddon::initPlatformModule ()
{
	if(!moduleId)
		return 0;

	CFObj<CFStringRef> moduleIdStr = CFStringCreateWithCString (0, moduleId.str (), kCFStringEncodingASCII); // The bundle ID string must contain only alphanumeric characters (A-Z, a-z, 0-9), hyphen (-), and period (.)
	return CFBundleGetBundleWithIdentifier (moduleIdStr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NodeAddon::getPlatformPluginsFolder (IUrl& url) const
{
	CFBundleRef module = (CFBundleRef)System::GetCurrentModuleRef ();

	// "Contents/PlugIns" folder in bundle
	CFObj<CFURLRef> bundleUrl = CFBundleCopyBuiltInPlugInsURL (module);

	MacUtils::urlFromCFURL (url, bundleUrl, IUrl::kFolder);
}
