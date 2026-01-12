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
// Filename    : ccl/platform/linux/skia/rasterrendertarget.h
// Description : Skia Render Target for Linux using Software Rendering
//
//************************************************************************************************

#ifndef _rasterrendertarget_h
#define _rasterrendertarget_h

#include "ccl/platform/linux/wayland/waylandrendertarget.h"
#include "ccl/platform/linux/wayland/waylandbuffer.h"

#include "ccl/platform/linux/skia/skiarendertarget.linux.h"

namespace CCL {
class LinuxWindow;
	
namespace Linux {

//************************************************************************************************
// RasterRenderTarget
//************************************************************************************************

class RasterRenderTarget: public WaylandRenderTarget
{
public:
	RasterRenderTarget ();
	
	virtual float getScaleFactor () const { return 1.f; }
	
protected:
	sk_sp<SkSurface> lastSurface;
	
	WaylandBuffer buffers[5];
	int currentBuffer;
	bool resized;
	
	sk_sp<SkSurface> getSurface (PointRef size);
};

//************************************************************************************************
// RasterWindowRenderTarget
//************************************************************************************************

class RasterWindowRenderTarget: public RasterRenderTarget,
								public SkiaWindowRenderTarget
{
public:
	RasterWindowRenderTarget (Window& window);
	
	// SkiaWindowRenderTarget
	void onSize () override;
	void onRender () override;
	SkCanvas* getCanvas () override;
	IMutableRegion* getUpdateRegion () override { return nullptr; }
	IMutableRegion* getInvalidateRegion () override  { return &invalidateRegion; }
	bool shouldCollectUpdates () override { return true; }
	void onScroll (RectRef rect, PointRef delta) override {};
	
	// RasterRenderTarget
	bool onFrameCallback () override;
	float getScaleFactor () const override { return getContentScaleFactor (); }
	
protected:
	LinuxWindow* linuxWindow;
	
	void onPresent ();
};

//************************************************************************************************
// RasterLayerRenderTarget
//************************************************************************************************

class RasterLayerRenderTarget: public LinuxLayerRenderTarget,
							   public RasterRenderTarget,
							   public SkiaRenderTarget
{
public:
	RasterLayerRenderTarget (wl_surface* surface, NativeGraphicsLayer& layer);
	
	// LinuxLayerRenderTarget
	virtual SkiaRenderTarget* getSkiaRenderTarget () override { return this; }
	void resize (RectRef size) override;
	RectRef getSize () const override;
	void setContentScaleFactor (float factor) override;
	void onPresent () override;
	
	// RasterRenderTarget
	float getScaleFactor () const override { return getContentScaleFactor (); }
	
	// SkiaRenderTarget
	void onSize () override;
	float getContentScaleFactor () const override;
    SkCanvas* getCanvas () override;
};

} // namespace Linux
} // namespace CCL

#endif // _rasterrendertarget_h

