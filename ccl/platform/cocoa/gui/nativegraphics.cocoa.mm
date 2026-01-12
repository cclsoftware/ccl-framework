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
// Filename    : nativegraphics.cocoa.mm
// Description : Mac/iOS Native Graphics
//
//************************************************************************************************

#include "ccl/platform/cocoa/interfaces/iquartzbitmap.h"
#include "ccl/platform/cocoa/quartz/engine.h"
#include "ccl/platform/cocoa/skia/skiaengine.cocoa.h"
#include "ccl/platform/cocoa/metal/metalclient.h"

#include "ccl/platform/cocoa/cclcocoa.h"

using namespace CCL;
using namespace MacOS;

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MetalGraphicsInfo::isMetalAvailable () const
{	
	return MetalClient::instance ().isSupported ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MetalGraphicsInfo::isMetalEnabled () const
{
	NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
	// inverted logic: key missing yields "NO", so the default is Metal enabled
	return ![defaults boolForKey:@"MetalDisabled"];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MetalGraphicsInfo::setMetalEnabled (tbool state)
{
	NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
	[defaults setBool:state ? NO : YES forKey:@"MetalDisabled"];
    [defaults synchronize];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IQuartzBitmap, 0xED4FD1E3, 0x97D2, 0xA04E, 0x9B, 0xA9, 0xA2, 0x62, 0xC0, 0xB4, 0x31, 0xCB)

//************************************************************************************************
// NativeGraphicsEngine
//************************************************************************************************

NativeGraphicsEngine& NativeGraphicsEngine::instance ()
{
	static AutoPtr<NativeGraphicsEngine> theEngine;
	if(theEngine == nullptr)
	{
		if(MetalGraphicsInfo::instance ().isSkiaEnabled ())
            theEngine = NEW CocoaSkiaEngine;
		if(theEngine == nullptr)
			theEngine = NEW QuartzEngine;
	}
	ASSERT (theEngine)
	
	return *theEngine;
}


