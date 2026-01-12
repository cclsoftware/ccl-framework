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
// Filename    : ccl/platform/cocoa/skia/skiarendertarget.mac.h
// Description : Skia Render Target for Mac using Metal
//
//************************************************************************************************

#include "ccl/platform/cocoa/skia/skiarendertarget.cocoa.h"

@class NSView;

namespace CCL {

//************************************************************************************************
// MetalMacWindowRenderTarget
//************************************************************************************************

class MetalMacWindowRenderTarget: public MetalWindowRenderTarget
{
public:
	MetalMacWindowRenderTarget (Window& window);
	~MetalMacWindowRenderTarget ();

	// MetalWindowRenderTarget
	void onSize () override;
	void onRender () override;
	void addMetal3DSurface (Native3DSurface* surface) override;
	void removeMetal3DSurface (Native3DSurface* surface) override;
	
protected:
	NSView* hostView;
	id<NSObject> sizeObserver;
	id<NSObject> scaleObserver;

    // MetalWindowRenderTarget
	void initialize () override;
};

}; // namespace CCL


