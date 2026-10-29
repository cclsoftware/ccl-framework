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
// Filename    : ccl/platform/linux/gui/fontresource.linux.cpp
// Description : Linux Font Resource
//
//************************************************************************************************

#include "ccl/platform/shared/skia/skiafontmanager.h"

namespace CCL {

//************************************************************************************************
// LinuxFontManager
//************************************************************************************************

class LinuxFontManager: public SkiaFontManager 
{
public:
	LinuxFontManager ();
	
	static sk_sp<SkFontMgr> create ();
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// SkiaFontManagerFactory
//************************************************************************************************

sk_sp<SkFontMgr> SkiaFontManagerFactory::createFontManager ()
{
	static sk_sp<SkFontMgr> theManager = LinuxFontManager::create ();
	return theManager;
}

//************************************************************************************************
// LinuxFontManager
//************************************************************************************************

LinuxFontManager::LinuxFontManager ()
{
	fontManager = SkFontMgr_New_FontConfig (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkFontMgr> LinuxFontManager::create ()
{
	return sk_make_sp<LinuxFontManager> ();
}
