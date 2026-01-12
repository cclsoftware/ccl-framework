//********************************************************************************************manager//
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
// Filename    : ccl/platform/shared/skia/skiafontmanager.h
// Description : Skia Font Manager Factory
//
//************************************************************************************************

#ifndef _ccl_skia_fontmanager_h
#define _ccl_skia_fontmanager_h

#include "ccl/platform/shared/skia/skiaglue.h"

namespace CCL {

//************************************************************************************************
// SkiaFontManagerFactory
//************************************************************************************************

class SkiaFontManagerFactory
{
public:
	static sk_sp<SkFontMgr> createFontManager ();
};

} // namespace CCL

#endif // _ccl_skia_fontmanager_h
