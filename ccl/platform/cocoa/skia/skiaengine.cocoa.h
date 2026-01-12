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
// Filename    : ccl/platform/cocoa/skia/skiaengine.cocoa.h
// Description : Cocoa Skia Engine
//
//************************************************************************************************

#ifndef _ccl_skia_engine_cocoa_h
#define _ccl_skia_engine_cocoa_h

#include "ccl/platform/shared/skia/skiaengine.h"

#include "ccl/base/singleton.h"
#include "ccl/public/gui/framework/imacosspecifics.h"

namespace CCL {
namespace MacOS {

//************************************************************************************************
// CocoaSkiaEngine
//************************************************************************************************

class CocoaSkiaEngine: public SkiaEngine
{
public:	
	// SkiaEngine
	const SkShaper& getShaper ();
	GrRecordingContext* getGPUContext ();
	NativeBitmap* createBitmap (int width, int height, IBitmap::PixelFormat pixelFormat, float contentScaleFactor = 1.f);
	NativeBitmap* loadBitmap (IStream& stream, const FileType& format);
	bool hasGraphicsLayers ();
	IGraphicsLayer* createGraphicsLayer (UIDRef classID);
	NativeBitmap* createScreenshotFromWindow (Window* window);
	Object* createPrintJob ();
	INative3DSupport* get3DSupport ();
	
protected:
	sk_sp<GrRecordingContext> context;
};

//************************************************************************************************
// MetalGraphicsInfo
//************************************************************************************************

class MetalGraphicsInfo: public Object,
						 public Singleton<MetalGraphicsInfo>,
						 public IMetalGraphicsInfo
{
public:
	DECLARE_CLASS (MetalGraphicsInfo, Object)
	
	bool isSkiaEnabled () const;
	
	// IMetalGraphicsInfo
	tbool CCL_API isMetalAvailable () const override;
	tbool CCL_API isMetalEnabled () const override;
	void CCL_API setMetalEnabled (tbool state) override;
	
	CLASS_INTERFACE (IMetalGraphicsInfo, Object)
};

} // namespace MacOS
} // namespace CCL

#endif // _ccl_skia_engine_cocoa_h
