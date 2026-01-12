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
// Filename    : ccl/extras/nodejs/platform/nodeaddon.win.cpp
// Description : Node addon platform code
//
//************************************************************************************************

#include "ccl/extras/nodejs/nodeaddon.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/systemservices.h"

#include "ccl/platform/win/cclwindows.h"

using namespace CCL;
using namespace NodeJS;

//*************************************************************************************************
// NodeAddon
//*************************************************************************************************

ModuleRef NodeAddon::initPlatformModule ()
{
	if(moduleId.isEmpty ())
		return nullptr;

	static int g_dummy = 0;

	HMODULE hModule = NULL;
	if(!::GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(&g_dummy), &hModule))
		return nullptr;

	return hModule;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void NodeAddon::getPlatformPluginsFolder (IUrl& url) const
{
	AutoPtr<IExecutableImage>image = System::GetExecutableLoader ().createImage (System::GetCurrentModuleRef ());
	image->getPath (url);
	url.ascend ();
	url.descend (CCLSTR ("Plugins"), IUrl::kFolder);
}
