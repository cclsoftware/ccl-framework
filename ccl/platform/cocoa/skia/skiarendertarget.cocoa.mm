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
// Filename    : ccl/platform/cocoa/skia/skiarendertarget.cocoa.mm
// Description : Skia Render Target for Mac and iOS using Metal
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/skia/skiarendertarget.cocoa.h"

#include "ccl/platform/shared/skia/skiadevice.h"
#include "ccl/platform/cocoa/skia/skiaengine.cocoa.h"
#include "ccl/platform/cocoa/metal/metalclient.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/iapplication.h"

#include "ccl/public/base/ccldefpush.h"
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include <QuartzCore/CATransaction.h>
#include <QuartzCore/CAAnimation.h>

using namespace CCL;

//************************************************************************************************
// LayerDelegate
//************************************************************************************************

@implementation CCL_ISOLATED (LayerDelegate)

- (id<CAAction>)actionForLayer:(CALayer*)layer forKey:(NSString*)event
{
	// disable all implicit animations
	return (id<CAAction>)[NSNull null];
}

@end

//************************************************************************************************
// MetalUpdater
//************************************************************************************************

MetalUpdater::MetalUpdater ()
: suspended (false)
{
	ISubject::addObserver (&System::GetGUI (), this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetalUpdater::~MetalUpdater ()
{
	ASSERT (!hasClients ())
	ISubject::removeObserver (&System::GetGUI (), this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetalUpdater::hasClients () const
{
	return !(targets.isEmpty () && layers.isEmpty () && targets.isEmpty ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalUpdater::addTarget (MetalWindowRenderTarget* target)
{
	if(!hasClients ())
		System::GetGUI ().addIdleTask (this);
	targets.add (target);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalUpdater::removeTarget (MetalWindowRenderTarget* target)
{
	targets.remove (target);
	if(!hasClients ())
		System::GetGUI ().removeIdleTask (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalUpdater::addLayer (IGraphicsLayer* layer)
{
	if(!hasClients ())
		System::GetGUI ().addIdleTask (this);
	layers.add (layer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalUpdater::removeLayer (IGraphicsLayer* layer)
{
	removedLayers.add (layer);
	if(!hasClients ())
		System::GetGUI ().removeIdleTask (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalUpdater::removePendingLayers ()
{
	VectorForEachFast (removedLayers, IGraphicsLayer*, layer)
		layers.remove (layer);
	EndFor
	removedLayers.removeAll ();
	if(!hasClients ())
		System::GetGUI ().removeIdleTask (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalUpdater::addSurface (Metal3DSurface* surface)
{
	if(!hasClients ())
		System::GetGUI ().addIdleTask (this);
	surfaces.add (surface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalUpdater::removeSurface (Metal3DSurface* surface)
{
	surfaces.remove (surface);
	if(!hasClients ())
	{
		System::GetGUI ().removeIdleTask (this);
		removePendingLayers ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MetalUpdater::onTimer (ITimer* timer)
{
	if(!suspended)
	{
		VectorForEachFast (targets, MetalWindowRenderTarget*, target)
			target->onPresent ();
		EndFor
		VectorForEachFast (layers, IGraphicsLayer*, layer)
			if(!removedLayers.contains (layer))
				layer->flush ();
		EndFor
		VectorForEachFast (surfaces, Metal3DSurface*, surface)
			surface->draw ();
		EndFor
		removePendingLayers ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MetalUpdater::notify (ISubject* subject, MessageRef msg)
{
	#if CCL_PLATFORM_IOS
	if(msg == IApplication::kAppTerminates || msg == IApplication::kAppSuspended || msg == IApplication::kAppDeactivated)
		suspended = true;
	else if(msg == IApplication::kAppActivated)
	{
		VectorForEachFast (targets, MetalWindowRenderTarget*, target)
			target->reset ();
		EndFor
		suspended = false;
	}
	#elif CCL_PLATFORM_MAC
	if(msg == IApplication::kAppTerminates)
		removePendingLayers ();
	#endif
}

//************************************************************************************************
// MetalRenderTarget
//************************************************************************************************

MetalRenderTarget::MetalRenderTarget ()
: metalLayer (nil),
  delegate (nil),
  currentDrawable (nil),
  maxSize (0),
  context (nil)
{
    metalDevice = MetalClient::instance ().getDevice ();
	metalQueue = MetalClient::instance ().getQueue ();
	
	// reference https://developer-rno.apple.com/metal/Metal-Feature-Set-Tables.pdf
	maxSize = 16384;
	#if CCL_PLATFORM_IOS
	if(![metalDevice supportsFamily:MTLGPUFamilyApple3])
		maxSize = 8192;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetalRenderTarget::~MetalRenderTarget ()
{
	if(currentDrawable)
		[currentDrawable release];
	if(metalLayer)
		[metalLayer release];
	if(delegate)
		[delegate release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalRenderTarget::presentDrawable ()
{
	if(currentDrawable)
	{
		if(id<MTLCommandBuffer> commandBuffer = [metalQueue commandBuffer])
		{
			[commandBuffer presentDrawable:currentDrawable];
			[commandBuffer commit];
		}
		[currentDrawable release];
		currentDrawable = nil;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetalRenderTarget::checkSize (const CGSize& size) const
{
	return (size.width <= maxSize && size.height <= maxSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalRenderTarget::addMetal3DSurface (Native3DSurface* surface)
{
	if(Metal3DSurface* metal3DSurface = ccl_cast<Metal3DSurface> (surface))
		surfaces.add (metal3DSurface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalRenderTarget::removeMetal3DSurface (Native3DSurface* surface)
{
	surfaces.remove (ccl_cast<Metal3DSurface> (surface));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

id<CAMetalDrawable> MetalRenderTarget::getCurrentDrawable () const
{
	return currentDrawable;
}

//************************************************************************************************
// MetalLayerRenderTarget
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MetalLayerRenderTarget, Object)

MetalLayerRenderTarget::MetalLayerRenderTarget (CAMetalLayer* layer, float _contentScaleFactor)
{
	metalLayer = layer;
	[metalLayer retain];
	metalLayer.framebufferOnly = NO;
	delegate = [[CCL_ISOLATED (LayerDelegate) alloc] init];
	metalLayer.delegate = delegate;
	metalLayer.device = metalDevice;
	metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
	setContentScaleFactor (_contentScaleFactor);
	SkiaEngine* engine = SkiaEngine::getInstance ();
	context = engine->getGPUContext ();

	ASSERT(context)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float MetalLayerRenderTarget::getContentScaleFactor () const
{
	return metalLayer ? float(metalLayer.contentsScale) : 1;

}
//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalLayerRenderTarget::setContentScaleFactor (float factor)
{
	metalLayer.contentsScale = factor;
	CGSize pixelSize = metalLayer.bounds.size;
	pixelSize.width *= factor;
	pixelSize.height *= factor;
	metalLayer.drawableSize = pixelSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* MetalLayerRenderTarget::getCanvas ()
{
	if(!surface)
	{
		presentDrawable ();
		currentDrawable = [metalLayer nextDrawable];
		ASSERT (currentDrawable);
		[currentDrawable retain];
		
		GrMtlTextureInfo textureInfo;
		textureInfo.fTexture.retain ((__bridge const void*)(currentDrawable.texture));
		GrBackendTexture backendTexture = GrBackendTextures::MakeMtl (int(metalLayer.drawableSize.width), int(metalLayer.drawableSize.height), skgpu::Mipmapped::kNo, textureInfo);
		surface = SkSurfaces::WrapBackendTexture (context, backendTexture, kTopLeft_GrSurfaceOrigin, 1, kBGRA_8888_SkColorType, 0, 0);
		if(!surface)
			return nullptr;
			
		SkCanvas* canvas = surface->getCanvas ();
		if(canvas)
		{
			canvas->scale (getContentScaleFactor (), getContentScaleFactor ());
			canvas->clear (SkColorSetARGB (0x00, 0x00, 0x00, 0x00));
		}
	}
	else if(!currentDrawable)
	{
		currentDrawable = [metalLayer nextDrawable];
		ASSERT (currentDrawable);
		[currentDrawable retain];
				
		GrMtlTextureInfo textureInfo;
		textureInfo.fTexture.retain ((__bridge const void*)(currentDrawable.texture));
		GrBackendTexture backendTexture = GrBackendTextures::MakeMtl (int(metalLayer.drawableSize.width), int(metalLayer.drawableSize.height), skgpu::Mipmapped::kNo, textureInfo);
		surface->replaceBackendTexture (backendTexture, kTopLeft_GrSurfaceOrigin);
	}
	
	return SkiaRenderTarget::getCanvas ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalLayerRenderTarget::onRender ()
{
	if(surface)
	{
		if(auto directContext = GrAsDirectContext (surface->getCanvas ()->recordingContext())) {
			directContext->flushAndSubmit();
		}
		presentDrawable ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalLayerRenderTarget::presentDrawable ()
{
	if(currentDrawable)
	{
		[currentDrawable present];
		[currentDrawable release];
		currentDrawable = nil;
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalLayerRenderTarget::onSize ()
{
	surface = nullptr;
}

//************************************************************************************************
// MetalWindowRenderTarget
//************************************************************************************************

MetalWindowRenderTarget::MetalWindowRenderTarget (Window& window)
: SkiaWindowRenderTarget (window),
  lastTexture (nil)
{
	SkiaEngine* engine = SkiaEngine::getInstance ();
	context = engine->getGPUContext ();
	invalidateRegion.addRect (Rect (0, 0, window.getWidth (), window.getHeight ()));
	ASSERT(context)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetalWindowRenderTarget::~MetalWindowRenderTarget ()
{
	MetalUpdater::instance ().removeTarget (this);
	if(lastTexture)
		[lastTexture release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalWindowRenderTarget::reset ()
{
	[lastTexture release];
	lastTexture = nil;
	invalidateRegion.addRect (Rect (0, 0, window.getWidth (), window.getHeight ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalWindowRenderTarget::initialize ()
{
	MetalUpdater::instance ().addTarget (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetalWindowRenderTarget::shouldCollectUpdates ()
{
	return isTranslucent ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetalWindowRenderTarget::isTranslucent () const
{
	return window.shouldBeTranslucent () || window.getOpacity () < 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float MetalWindowRenderTarget::getContentScaleFactor () const
{
	return window.getContentScaleFactor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalWindowRenderTarget::onSize ()
{
    PixelPoint currentSize = size;
    size = PixelPoint (Point (window.getWidth (), window.getHeight ()), window.getContentScaleFactor ());
	CCL_PRINTF ("MetalRenderTarget::onSize window size = %d x %d, factor =%f\n", window.getWidth (), window.getHeight (), window.getContentScaleFactor ())
	
	if(size == currentSize && metalLayer && metalLayer.contentsScale == window.getContentScaleFactor ())
        return;
	
	if(metalLayer)
	{
		presentDrawable ();
		surface = nullptr;
		metalLayer.bounds = CGRectMake (0, 0, window.getWidth (), window.getHeight ());
		metalLayer.drawableSize = CGSizeMake (size.x, size.y);
		metalLayer.contentsScale = window.getContentScaleFactor ();
		
		if(lastTexture)
		{
			[lastTexture release];
			lastTexture = nil;
		}
		
		updateRegion.setEmpty ();
		invalidateRegion.setEmpty ();
	}
	else
		initialize ();

    invalidateRegion.addRect (Rect (0, 0, window.getWidth (), window.getHeight ()));
	onPresent ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* MetalWindowRenderTarget::getCanvas ()
{
	if(!surface)
	{
		if(!metalLayer)
			initialize ();

		currentDrawable = [metalLayer nextDrawable];
		ASSERT (currentDrawable);
		if(!currentDrawable)
			return nullptr;
			
		[currentDrawable retain];
		bool needsClear = false;
		if(lastTexture)
		{
			// copy contents of previous texture to new one
			if(id<MTLCommandBuffer> commandBuffer = [metalQueue commandBuffer])
			{
				if(id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder])
				{
					if(lastTexture.width > 0 && lastTexture.height > 0 && lastTexture.depth >0)
					{
						MTLSize size = MTLSizeMake (lastTexture.width, lastTexture.height, lastTexture.depth);
						[blit copyFromTexture:lastTexture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake (0, 0, 0) sourceSize:size toTexture:currentDrawable.texture destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake (0, 0, 0)];
					}
					[blit endEncoding];
				}
				[commandBuffer commit];
			}
			
			[lastTexture release];
			lastTexture = nil;
		}
		else
			needsClear = true;

		SkSurfaceProps props;

		GrMtlTextureInfo textureInfo;
		textureInfo.fTexture.retain ((__bridge const void*)(currentDrawable.texture));

		GrBackendRenderTarget backendRT = GrBackendRenderTargets::MakeMtl (int(metalLayer.drawableSize.width), int(metalLayer.drawableSize.height), textureInfo);
		surface = SkSurfaces::WrapBackendRenderTarget (context, backendRT, kTopLeft_GrSurfaceOrigin, kBGRA_8888_SkColorType, 0,  &props);
		if(!surface)
			return nullptr;
		
		SkCanvas* canvas = surface->getCanvas ();
		if(!canvas)
			return nullptr;
			
		canvas->scale (window.getContentScaleFactor (), window.getContentScaleFactor ());
		if(needsClear)
			canvas->clear (SkColorSetARGB (0, 0, 0, 0));
	}

	return SkiaRenderTarget::getCanvas ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalWindowRenderTarget::onRender ()
{
	if(!invalidateRegion.getRects ().isEmpty ())
	{
		AutoPtr<NativeGraphicsDevice> nativeDevice = ensureGraphicsDevice (NativeGraphicsEngine::instance ().createWindowDevice (&window));
		AutoPtr<GraphicsDevice> graphicsDevice = NEW WindowGraphicsDevice (window, nativeDevice);
		window.setGraphicsDevice (graphicsDevice);
		
		RectRef& updateRect = updateRegion.getBoundingBox ();
		
		CCL_PROFILE_START(draw)
		for(int i = 0; i < invalidateRegion.getRects ().count (); i++)
		{
			RectRef invalidateRect = invalidateRegion.getRects ().at (i);
			CCL_PRINTF ("invalidate : %4d %4d %4d %4d\n", invalidateRect.left, invalidateRect.top, invalidateRect.getWidth (), invalidateRect.getHeight ())

			graphicsDevice->saveState ();
			graphicsDevice->addClip (invalidateRect);
			graphicsDevice->clearRect (invalidateRect);

			window.setInDrawEvent (true);
			window.draw (UpdateRgn (invalidateRect, &invalidateRegion));
			window.setInDrawEvent (false);
			graphicsDevice->restoreState ();
		}
		invalidateRegion.setEmpty ();
		CCL_PROFILE_STOP(draw)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalWindowRenderTarget::onPresent ()
{
	onRender ();
	if(surface)
	{
		if(auto directContext = GrAsDirectContext (surface->getCanvas ()->recordingContext ()))
			directContext->flushAndSubmit ();
		lastTexture = currentDrawable.texture;
		[lastTexture retain];
		presentDrawable ();
		surface = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalWindowRenderTarget::onScroll (RectRef rect, PointRef delta)
{
	if(surface)
		if(auto directContext = GrAsDirectContext (surface->getCanvas ()->recordingContext ()))
			directContext->flushAndSubmit ();

    CCL_PRINTF ("MetalRenderTarget::onScroll rect = %d %d %d %d delta = %d %d\n", rect.left, rect.top, rect.getWidth (), rect.getHeight (), delta.x, delta.y)
 
    if(delta.x == 0 && delta.y == 0 || rect.getWidth () <= 0 || rect.getHeight () <= 0)
        return;

    getCanvas ();
	ASSERT(currentDrawable)
	ASSERT(currentDrawable.texture)
	
    id<MTLCommandBuffer> commandBuffer = [metalQueue commandBuffer];
	ASSERT(commandBuffer)

	id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder];
	ASSERT(blit)

    PixelRect pixelRect (rect, (float)metalLayer.contentsScale);
    PixelPoint pixelDelta (delta, (float)metalLayer.contentsScale);
	MTLSize pixelSize = MTLSizeMake (pixelRect.getWidth (), pixelRect.getHeight (), 1);

	id<MTLBuffer> pixelBuffer = [metalDevice newBufferWithLength:pixelSize.width * pixelSize.height * sizeof(uint32) options:MTLResourceStorageModePrivate];
	ASSERT(pixelBuffer)
	
	if(currentDrawable.texture && pixelBuffer)
	{
		[blit copyFromTexture:currentDrawable.texture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake (pixelRect.left, pixelRect.top, 0) sourceSize:pixelSize toBuffer:pixelBuffer destinationOffset:0 destinationBytesPerRow:pixelSize.width * sizeof(uint32) destinationBytesPerImage:0];
		[blit copyFromBuffer:pixelBuffer sourceOffset:0 sourceBytesPerRow:pixelSize.width * sizeof(uint32) sourceBytesPerImage:0 sourceSize:pixelSize toTexture:currentDrawable.texture destinationSlice:0 destinationLevel:0 destinationOrigin:MTLOriginMake (pixelRect.left + pixelDelta.x, pixelRect.top + pixelDelta.y, 0)];
	}
	[blit endEncoding];
    [commandBuffer commit];
    [pixelBuffer release];

	if(invalidateRegion.rectVisible (rect))
	{
		Rect r (rect);
		r.offset (delta);
		r.join (rect);
		invalidateRegion.addRect (rect);
	}

    Rect r1, r2;
    if(delta.y < 0)
        r1 (rect.left, rect.top + delta.y, rect.right, rect.bottom);
    else
        r1 (rect.left, rect.top, rect.right, rect.bottom + delta.y);

    if(delta.x < 0)
        r2 (rect.left + delta.x, rect.top, rect.right, rect.bottom);
    else
        r2 (rect.left, rect.top, rect.right + delta.x, rect.bottom);
        
    if(!r1.isEmpty ())
        updateRegion.addRect (r1);
    if(!r2.isEmpty ())
        updateRegion.addRect (r2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalWindowRenderTarget::add3DSurface (Native3DSurface* surface)
{
	addMetal3DSurface (surface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalWindowRenderTarget::remove3DSurface (Native3DSurface* surface)
{
	removeMetal3DSurface (surface);
}
