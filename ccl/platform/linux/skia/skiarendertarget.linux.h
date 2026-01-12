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
// Filename    : ccl/platform/linux/skia/skiarendertarget.linux.h
// Description : Skia Render Target for Linux
//
//************************************************************************************************

#ifndef _skiarendertarget_linux_h
#define _skiarendertarget_linux_h

#include "ccl/platform/shared/skia/skiarendertarget.h"
#include "ccl/platform/shared/vulkan/vulkanskiarendertarget.h"

struct wl_surface;

namespace CCL {

//************************************************************************************************
// LinuxLayerRenderTarget
//************************************************************************************************

class LinuxLayerRenderTarget: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (LinuxLayerRenderTarget, Object)
	
	LinuxLayerRenderTarget (wl_surface* surface, NativeGraphicsLayer& layer)
	: layer (layer),
	  contentScaleFactor (1)
	{}
	
	static LinuxLayerRenderTarget* create (wl_surface* surface, NativeGraphicsLayer& layer);
	
	virtual SkiaRenderTarget* getSkiaRenderTarget () { return nullptr; }
	
	virtual void onRender () {}
	virtual void onPresent () {}
	
	virtual void resize (RectRef size) {}
	virtual RectRef getSize () const { return size; }
	virtual void setContentScaleFactor (float factor) {}
	
protected:
	NativeGraphicsLayer& layer;
	float contentScaleFactor;
	Rect size;
};

} // namespace CCL

#endif // _skiarendertarget_linux_h
