//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/linux/wasm/openglesrendertarget.wasm.h
// Description : Skia Render Target using WebGL
//
//************************************************************************************************

#ifndef _openglesrendertarget_wasm_h
#define _openglesrendertarget_wasm_h

#include "ccl/platform/shared/skia/skiarendertarget.h"

#include "ccl/platform/shared/opengles/openglesrendertarget.h"

#include "ccl/gui/graphics/mutableregion.h"

#include <emscripten/html5.h>

namespace CCL {
namespace Wasm {

//************************************************************************************************
// WebGLWindowRenderTarget
//************************************************************************************************

class WebGLWindowRenderTarget: public SkiaWindowRenderTarget,
							   public OpenGLESRenderTarget
{
public:
	WebGLWindowRenderTarget (Window& window);
	~WebGLWindowRenderTarget ();

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
	class WebGLMutableRegion: public MutableRegion
	{
	public:
		WebGLMutableRegion (WebGLWindowRenderTarget& target);
		
		PROPERTY_BOOL (updatesSuspended, UpdatesSuspended)

		// MutableRegion
		void CCL_API addRect (RectRef rect);
		
	private:
		WebGLWindowRenderTarget& target;
	};
	friend class WebGLMutableRegion;
	
	sk_sp<GrDirectContext> gpuContext;
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webglContext;
	MutableCString frameId;
	MutableCString contentId;

	WebGLMutableRegion updateRegion;
	WebGLMutableRegion invalidateRegion;
	long callbackId;

	float scaleFactor;
	bool contextLost;

	static bool onFrame (double time, void* data);
	static bool onContextLost (int eventType, const void* reserved, void* userData);
	static bool onContextRestored (int eventType, const void* reserved, void* userData);

	bool initialize ();
	void clear ();
	void requestFrame ();
	bool flushSurface ();

	// OpenGLESRenderTarget
	sk_sp<SkSurface> getSurface () override { return surface; }
	void setSurface (sk_sp<SkSurface> newSurface) override { surface = newSurface; }
	void makeCurrent () override;
	GrDirectContext* getGPUContext () override { return gpuContext.get (); }
	float getScaleFactor () const override { return scaleFactor; }
	float getOpacity () const override;
	bool isTranslucent () const override;
};

} // namespace Linux
} // namespace CCL

#endif // _openglesrendertarget_wasm_h

