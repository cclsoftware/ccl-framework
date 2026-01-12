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
// Filename    : ccl/platform/cocoa/skia/skialayer.cocoa.h
// Description : CoreAnimation Graphics Layer for Skia content
//
//************************************************************************************************

#ifndef _skialayer_cocoa_h
#define _skialayer_cocoa_h

#include "ccl/public/base/uid.h" 

namespace CCL {

interface IGraphicsLayer;

namespace MacOS {

//************************************************************************************************
// CocoaSkiaLayerFactory
//************************************************************************************************

class CocoaSkiaLayerFactory
{
public:
	static IGraphicsLayer* createLayer (UIDRef classID);
};

} // namespace MacOS

} // namespace CCL

#endif // _skialayer_cocoa_h
