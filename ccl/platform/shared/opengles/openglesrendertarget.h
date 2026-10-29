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
// Filename    : ccl/platform/shared/opengles/openglesrendertarget.h
// Description : Skia Render Target using OpenGL ES
//
//************************************************************************************************

#ifndef _openglesrendertarget_h
#define _openglesrendertarget_h

#include "ccl/platform/shared/opengles/opengles3dsupport.h"

namespace CCL {

class MutableRegion;
class Window;

//************************************************************************************************
// OpenGLESRenderTarget
//************************************************************************************************

class OpenGLESRenderTarget
{
public:
	OpenGLESRenderTarget ();
	virtual ~OpenGLESRenderTarget ();
	
	virtual float getScaleFactor () const { return 1.f; }
	virtual float getOpacity () const { return 1.f; }
	virtual bool isTranslucent () const { return false; }

	PROPERTY_AUTO_POINTER (OpenGLES3DGraphicsContext, graphicsContext3D, GraphicsContext3D)
	
protected:
	OpenGLESImage texture;
	Point surfaceExtent;
	sk_sp<SkSurface> targetSurface;
	sk_sp<SkImage> textureImage;
	
	Vector<OpenGLES3DSurface*> surfaces;

	virtual GrDirectContext* getGPUContext () { return nullptr; }
	virtual sk_sp<SkSurface> getSurface () { return nullptr; }
	virtual void setSurface (sk_sp<SkSurface> surface) {}
	virtual SkCanvas* getSkiaCanvas ();
	void render3DContent ();
	void compose3DContent ();
	bool flushSurface ();
	virtual void makeCurrent () {}
	virtual void addOpenGLES3DSurface (Native3DSurface* surface);
	virtual void removeOpenGLES3DSurface (Native3DSurface* surface);
	virtual void blendLayers (MutableRegion& invalidateRegion);
	void renderWindow (Window& window, MutableRegion& invalidateRegion);
};

} // namespace CCL

#endif // _openglesrendertarget_h

