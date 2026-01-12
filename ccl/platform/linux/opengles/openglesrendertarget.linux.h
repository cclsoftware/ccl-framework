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
// Filename    : ccl/platform/linux/opengles/openglesrendertarget.linux.h
// Description : Skia Render Target using OpenGL ES and Wayland
//
//************************************************************************************************

#ifndef _openglesrendertarget_linux_h
#define _openglesrendertarget_linux_h

#include "ccl/platform/linux/wayland/waylandrendertarget.h"

#include "ccl/platform/linux/skia/skiarendertarget.linux.h"

#include "ccl/platform/shared/opengles/openglesrendertarget.h"

struct wl_egl_window;

namespace CCL {
class LinuxWindow;

namespace Linux {

//************************************************************************************************
// LinuxOpenGLESRenderTarget
//************************************************************************************************

class LinuxOpenGLESRenderTarget: public OpenGLESRenderTarget,
				 				 public WaylandRenderTarget
{
public:
	LinuxOpenGLESRenderTarget ();
	~LinuxOpenGLESRenderTarget ();
	
protected:
	wl_egl_window* eglWindow;

	bool initialize (wl_surface* surface);
};

//************************************************************************************************
// OpenGLESWindowRenderTarget
//************************************************************************************************

class OpenGLESWindowRenderTarget: public LinuxOpenGLESRenderTarget,
								  public SkiaWindowRenderTarget
{
public:
	OpenGLESWindowRenderTarget (Window& window);
		
	// LinuxOpenGLESRenderTarget
	float getOpacity () const override;
	bool isTranslucent () const override;
	bool onFrameCallback () override;
	float getScaleFactor () const override { return getContentScaleFactor (); }

	// SkiaWindowRenderTarget
	void onSize () override;
	void onRender () override;
	void onScroll (RectRef rect, PointRef delta) override;
	SkCanvas* getCanvas () override;
	IMutableRegion* getUpdateRegion () override;
	IMutableRegion* getInvalidateRegion () override;
	bool shouldCollectUpdates () override { return true; }
	void add3DSurface (Native3DSurface* surface) override;
	void remove3DSurface (Native3DSurface* surface) override;
	
private:
	LinuxWindow* linuxWindow;
	
	void applySize ();
	void onPresent ();

	// LinuxOpenGLESRenderTarget
	bool initialize () override;
	void clear () override;
	sk_sp<SkSurface> getSurface () override { return surface; }
	void setSurface (sk_sp<SkSurface> newSurface) override { surface = newSurface; }
};

//************************************************************************************************
// OpenGLESLayerRenderTarget
//************************************************************************************************

class OpenGLESLayerRenderTarget: public LinuxLayerRenderTarget,
								 public LinuxOpenGLESRenderTarget,
								 public SkiaRenderTarget
{
public:
	OpenGLESLayerRenderTarget (wl_surface* surface, NativeGraphicsLayer& layer);
	
	// LinuxLayerRenderTarget
	virtual SkiaRenderTarget* getSkiaRenderTarget () override { return this; }
	void resize (RectRef size) override;
	RectRef getSize () const override;
	void setContentScaleFactor (float factor) override;
	void onRender () override;
	void onPresent () override;
	
	// LinuxOpenGLESRenderTarget
	float getScaleFactor () const override { return getContentScaleFactor (); }

	// SkiaRenderTarget
	void onSize () override;
	float getContentScaleFactor () const override;
	SkCanvas* getCanvas () override;
	
private:
	Point pixelSize;
	bool sizeChanged;

	void applySize ();
	
	// LinuxOpenGLESRenderTarget
	bool initialize () override;
	void clear () override;
	sk_sp<SkSurface> getSurface () override { return surface; }
	void setSurface (sk_sp<SkSurface> newSurface) override { surface = newSurface; }
};

} // namespace Linux
} // namespace CCL

#endif // _openglesrendertarget_linux_h

