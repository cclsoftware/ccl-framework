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
// Filename    : ccl/platform/win/gui/nativegraphics.win.cpp
// Description : Win32 Native Graphics
//
//************************************************************************************************

#include "ccl/platform/win/direct2d/d2dengine.h"
#include "ccl/platform/win/direct2d/dxgiengine.h"

#include "ccl/base/storage/configuration.h"

using namespace CCL;
using namespace Win32;

//////////////////////////////////////////////////////////////////////////////////////////////////

static Configuration::BoolValue direct2DGdiCompatible ("CCL.Win32.Direct2D", "GdiCompatible", false);
static Configuration::BoolValue direct2DFlipModelEnabled ("CCL.Win32.Direct2D", "FlipModelEnabled", true);

//************************************************************************************************
// NativeGraphicsEngine
//************************************************************************************************

NativeGraphicsEngine& NativeGraphicsEngine::instance ()
{
	static AutoPtr<NativeGraphicsEngine> theEngine;
	if(theEngine == nullptr)
	{
		theEngine = NEW Direct2DEngine;
		DXGIEngine::instance ().setGdiCompatible (direct2DGdiCompatible);
		DXGIEngine::instance ().setFlipModelEnabled (direct2DFlipModelEnabled);	
	}
	return *theEngine;
}
