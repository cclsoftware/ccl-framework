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
// Filename    : ccl/platform/linux/gui/nativegraphics.linux.cpp
// Description : Linux Native Graphics
//
//************************************************************************************************

#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/platform/linux/skia/skiaengine.linux.h"

using namespace CCL;

//************************************************************************************************
// NativeGraphicsEngine
//************************************************************************************************

NativeGraphicsEngine& NativeGraphicsEngine::instance ()
{
	static AutoPtr<NativeGraphicsEngine> theEngine;
	if(theEngine == nullptr)
	{
		theEngine = NEW LinuxSkiaEngine;
	}
	return *theEngine;
}
