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
// Filename    : ccl/platform/cocoa/quartz/quartzlayer.h
// Description : CoreAnimation Graphics Layer for Quartz content
//
//************************************************************************************************

#ifndef _quartzlayer_h
#define _quartzlayer_h

#include "ccl/public/base/uid.h" 

namespace CCL {

interface IGraphicsLayer;

namespace MacOS {

//************************************************************************************************
// CocoaQuartzLayerFactory
//************************************************************************************************

class CocoaQuartzLayerFactory
{
public:
	static IGraphicsLayer* createLayer (UIDRef classID);
};

} // namespace MacOS
} // namespace CCL

#endif // _quartzlayer_h
