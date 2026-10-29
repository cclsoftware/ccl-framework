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
// Filename    : ccl/platform/linux/wasm/openglesrendertarget.wasm.cpp
// Description : Skia Render Target using WebGL
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/wasm/opengles/openglesrendertarget.wasm.h"
#include "ccl/platform/wasm/gui/window.wasm.h"

#include "ccl/platform/shared/skia/skialayer.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <emscripten/html5.h>

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// WebGLWindowRenderTarget::WebGLMutableRegion
//************************************************************************************************

WebGLWindowRenderTarget::WebGLMutableRegion::WebGLMutableRegion (WebGLWindowRenderTarget& target)
: target (target),
  updatesSuspended (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebGLWindowRenderTarget::WebGLMutableRegion::addRect (RectRef rect)
{
	MutableRegion::addRect (rect);
	if(!updatesSuspended)
		target.requestFrame ();
}

//************************************************************************************************
// WebGLWindowRenderTarget
//************************************************************************************************

WebGLWindowRenderTarget::WebGLWindowRenderTarget (Window& window)
: SkiaWindowRenderTarget (window),
  webglContext (0),
  updateRegion (*this),
  invalidateRegion (*this),
  callbackId (0),
  scaleFactor (1.f),
  contextLost (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebGLWindowRenderTarget::~WebGLWindowRenderTarget ()
{
	if(callbackId != 0)
		emscripten_cancel_animation_frame (callbackId);

	makeCurrent ();
	gpuContext.reset ();
	surface.reset ();
	if(webglContext != 0)
		emscripten_webgl_destroy_context (webglContext);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebGLWindowRenderTarget::initialize ()
{
	EmscriptenWebGLContextAttributes attributes;
	emscripten_webgl_init_context_attributes (&attributes);
	attributes.alpha = true;
	attributes.depth = false;
	attributes.stencil = false;
	attributes.antialias = false;
	attributes.premultipliedAlpha = true;
	attributes.preserveDrawingBuffer = false;
	attributes.majorVersion = 2;
	attributes.minorVersion = 0;
	attributes.enableExtensionsByDefault = true;
	attributes.explicitSwapControl = false;

	WasmWindow* wasmWindow = WasmWindow::cast (&window);
	wasmWindow->getFrameId (frameId);
	wasmWindow->getContentId (contentId);

	webglContext = emscripten_webgl_create_context (contentId, &attributes);
	if(webglContext == 0)
		return false;

	makeCurrent ();

	sk_sp<const GrGLInterface> nativeInterface = GrGLInterfaces::MakeWebGL ();
	GrContextOptions contextOptions;
	gpuContext = GrDirectContexts::MakeGL (nativeInterface, contextOptions);
	if(gpuContext == nullptr)
		CCL_WARN ("%s\n", "Failed to initialize Skia GL bindings");

	emscripten_set_webglcontextlost_callback (contentId, this, false, onContextLost);
	emscripten_set_webglcontextrestored_callback (contentId, this, false, onContextRestored);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebGLWindowRenderTarget::requestFrame ()
{
	if(callbackId == 0 && !contextLost)
		callbackId = emscripten_request_animation_frame (onFrame, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebGLWindowRenderTarget::onFrame (double time, void* data)
{
	WebGLWindowRenderTarget* This = static_cast<WebGLWindowRenderTarget*> (data);
	This->onRender ();
	emscripten_webgl_commit_frame ();
	This->callbackId = 0;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebGLWindowRenderTarget::onContextLost (int eventType, const void* reserved, void* userData)
{
	WebGLWindowRenderTarget* This = static_cast<WebGLWindowRenderTarget*> (userData);

	CCL_WARN ("WebGL context lost for %s\n", This->frameId.str ());

	if(This->callbackId != 0)
	{
		emscripten_cancel_animation_frame (This->callbackId);
		This->callbackId = 0;
	}

	if(SkiaLayer* layer = unknown_cast<SkiaLayer> (This->window.getGraphicsLayer ()))
		layer->resetGPUState ();

	// the underlying GL resources are already gone; abandon rather than release them
	if(This->gpuContext)
		This->gpuContext->abandonContext ();
	This->gpuContext.reset ();
	This->surface = nullptr;
	This->contextLost = true;

	return true; // allow the browser to attempt restoration
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebGLWindowRenderTarget::onContextRestored (int eventType, const void* reserved, void* userData)
{
	WebGLWindowRenderTarget* This = static_cast<WebGLWindowRenderTarget*> (userData);

	CCL_WARN ("WebGL context restored for %s\n", This->frameId.str ());

	This->makeCurrent ();

	sk_sp<const GrGLInterface> nativeInterface = GrGLInterfaces::MakeWebGL ();
	GrContextOptions contextOptions;
	This->gpuContext = GrDirectContexts::MakeGL (nativeInterface, contextOptions);
	if(This->gpuContext == nullptr)
		CCL_WARN ("%s\n", "Failed to reinitialize Skia GL bindings after context restoration");

	This->contextLost = false;

	// all GPU resources were lost along with the context; force a full repaint
	This->clear ();
	This->requestFrame ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebGLWindowRenderTarget::makeCurrent ()
{
	if(webglContext != 0)
		emscripten_webgl_make_context_current (webglContext);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebGLWindowRenderTarget::clear ()
{
	updateRegion.setEmpty ();
	invalidateRegion.setEmpty ();
	invalidateRegion.addRect (Rect (0, 0, size.x, size.y));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebGLWindowRenderTarget::onSize ()
{
	if(scaleFactor != window.getContentScaleFactor ())
		scaleFactor = window.getContentScaleFactor ();

	if(contentId.isEmpty () || webglContext == 0)
		initialize ();

	surface = nullptr;
	size = PixelPoint (Point (window.getWidth (), window.getHeight ()), scaleFactor);
	surfaceExtent = size;
	if(!window.isMaximized ())
	{
		Rect frameSize;
		window.getFrameSize (frameSize);
		emscripten_set_element_css_size (frameId, frameSize.getWidth (), frameSize.getHeight ());
	}
	emscripten_set_canvas_element_size (contentId, size.x, size.y);
	CCL_PRINTF ("%s onSize: (%d,%d) * %f = (%d,%d)\n", id.str (), window.getWidth (), window.getHeight (), scaleFactor, surfaceExtent.x, surfaceExtent.y)
	clear ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* WebGLWindowRenderTarget::getCanvas ()
{
	makeCurrent ();

	if(contentId.isEmpty () || webglContext == 0)
		initialize ();

	return OpenGLESRenderTarget::getSkiaCanvas ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float WebGLWindowRenderTarget::getOpacity () const
{
	return window.getOpacity ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebGLWindowRenderTarget::isTranslucent () const
{
	return window.getStyle ().isTranslucent () || getOpacity () < 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebGLWindowRenderTarget::onRender ()
{
	if(contextLost || gpuContext == nullptr)
		return;

	CCL_PRINTF ("%s onRender: (%d,%d)\n", id.str (), surfaceExtent.x, surfaceExtent.y)

	invalidateRegion.setUpdatesSuspended (true);
	renderWindow (window, invalidateRegion);
	invalidateRegion.setUpdatesSuspended (false);

	if(surface)
		flushSurface ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebGLWindowRenderTarget::flushSurface ()
{
	if(!OpenGLESRenderTarget::flushSurface ())
		return false;

	if(SkiaLayer* layer = unknown_cast<SkiaLayer> (window.getGraphicsLayer ()))
		layer->onFrame (targetSurface.get ());	
	targetSurface->recordingContext ()->asDirectContext ()->flushAndSubmit (targetSurface.get ());

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebGLWindowRenderTarget::onScroll (RectRef rect, PointRef delta)
{
	invalidateRegion.addRect (Rect (0, 0, window.getWidth (), window.getHeight ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMutableRegion* WebGLWindowRenderTarget::getUpdateRegion ()
{
	return &updateRegion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMutableRegion* WebGLWindowRenderTarget::getInvalidateRegion ()
{
	return &invalidateRegion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebGLWindowRenderTarget::add3DSurface (Native3DSurface* surface)
{
	addOpenGLES3DSurface (surface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebGLWindowRenderTarget::remove3DSurface (Native3DSurface* surface)
{
	removeOpenGLES3DSurface (surface);
}
