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
// Filename    : ccl/platform/shared/opengles/eglopenglesrendertarget.h
// Description : Skia Render Target using OpenGL ES and EGL
//
//************************************************************************************************

#ifndef _eglopenglesrendertarget_h
#define _eglopenglesrendertarget_h

#include "ccl/platform/shared/opengles/openglesrendertarget.h"
#include "ccl/platform/shared/opengles/openglesclient.h"

namespace CCL {

//************************************************************************************************
// EGLOpenGLESRenderTarget
//************************************************************************************************

class EGLOpenGLESRenderTarget: public OpenGLESRenderTarget
{
public:
	EGLOpenGLESRenderTarget ();
	~EGLOpenGLESRenderTarget ();
	
	void initializeSurface (EGLNativeWindowType window);
	
protected:
	EGLSurface eglSurface;
	
	void presentFrame ();

	// OpenGLESRenderTarget
	GrDirectContext* getGPUContext () override;
	SkCanvas* getSkiaCanvas () override;
	void makeCurrent () override;
	void addOpenGLES3DSurface (Native3DSurface* surface) override;
};

} // namespace CCL

#endif // _openglesrendertarget_h

