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
// Filename    : ccl/platform/cocoa/skia/skiarendertarget.cocoa.h
// Description : Skia Render Target for Mac and iOS using Metal
//
//************************************************************************************************

#ifndef _skiarendertarget_cocoa_h
#define _skiarendertarget_cocoa_h

#include "ccl/platform/shared/skia/skiarendertarget.h"
#include "ccl/public/gui/framework/itimer.h"
#include "ccl/gui/graphics/mutableregion.h"
#include "ccl/platform/cocoa/metal/metal3dsupport.h"

#include "ccl/public/base/ccldefpush.h"

#include <QuartzCore/CALayer.h>

@class CAMetalLayer;
@protocol MTLDevice;
@protocol MTLCommandQueue;
@protocol MTLTexture;
@protocol MTLRenderCommandEncoder;

@protocol CAMetalDrawable;

//************************************************************************************************
// LayerDelegate
//************************************************************************************************

@interface CCL_ISOLATED (LayerDelegate) : NSObject<CALayerDelegate>
@end


namespace CCL {

//************************************************************************************************
// MetalRenderTarget
//************************************************************************************************

class MetalRenderTarget
{
public:
	MetalRenderTarget ();
	~MetalRenderTarget ();

	bool checkSize (const CGSize& size) const;
	id<CAMetalDrawable> getCurrentDrawable () const;
		
protected:
	CAMetalLayer* metalLayer;
	CCL_ISOLATED (LayerDelegate)* delegate;
  	id<MTLDevice> metalDevice;
    id<MTLCommandQueue> metalQueue;
    id<CAMetalDrawable> currentDrawable;
	CGFloat maxSize;
	Vector<Metal3DSurface*> surfaces;
	GrRecordingContext* context;
	
	virtual void presentDrawable ();
	virtual void addMetal3DSurface (Native3DSurface* surface);
	virtual void removeMetal3DSurface (Native3DSurface* surface);
};

//************************************************************************************************
// MetalWindowRenderTarget
//************************************************************************************************

class MetalWindowRenderTarget: public MetalRenderTarget,
							   public SkiaWindowRenderTarget
{
public:
	MetalWindowRenderTarget (Window& window);
	~MetalWindowRenderTarget ();
	
	void reset ();
    void onPresent ();
    
	// SkiaWindowRenderTarget
	virtual void onSize () override;
	virtual void onRender () override;
    float getContentScaleFactor () const override;
    void onScroll (RectRef rect, PointRef delta) override;
    SkCanvas* getCanvas () override;

	IMutableRegion* getUpdateRegion () override { return &updateRegion; }
	IMutableRegion* getInvalidateRegion () override { return &invalidateRegion; }
	bool shouldCollectUpdates () override;
	
	void add3DSurface (Native3DSurface* surface) override;
	void remove3DSurface (Native3DSurface* surface) override;
	
protected:
	id<MTLTexture> lastTexture;
	MutableRegion updateRegion;
	MutableRegion invalidateRegion;

	virtual void initialize ();
    bool isTranslucent () const;
};

//************************************************************************************************
// MetalLayerRenderTarget
//************************************************************************************************

class MetalLayerRenderTarget: public Object,
							  public MetalRenderTarget,
							  public SkiaRenderTarget
{
public:
	DECLARE_CLASS_ABSTRACT (MetalLayerRenderTarget, Object)
	
	MetalLayerRenderTarget (CAMetalLayer* metalLayer, float contentScaleFactor);
	
	void setContentScaleFactor (float factor);
    void onRender ();

	// MetalRenderTarget
	void presentDrawable () override;

    // SkiaRenderTarget
	float getContentScaleFactor () const override;	
	void onSize () override;
	SkCanvas* getCanvas () override;
};

//************************************************************************************************
// MetalUpdater
//************************************************************************************************

class MetalUpdater: public Object,
					public ITimerTask,
					public StaticSingleton<MetalUpdater>
{
public:
	MetalUpdater ();
	~MetalUpdater ();

	void addTarget (MetalWindowRenderTarget* target);
	void removeTarget (MetalWindowRenderTarget* target);
	void addLayer (IGraphicsLayer* layer);
	void removeLayer (IGraphicsLayer* layer);
	void addSurface (Metal3DSurface* surface);
	void removeSurface (Metal3DSurface* surface);

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// ITimerTask
	void CCL_API onTimer (ITimer* timer) override;

	CLASS_INTERFACE (ITimerTask, Object)

protected:
	Vector<MetalWindowRenderTarget*> targets;
	Vector<IGraphicsLayer*> layers;
	Vector<IGraphicsLayer*> removedLayers;
	Vector<Metal3DSurface*> surfaces;
	bool suspended;

	bool hasClients () const;
	void removePendingLayers ();
};


}; // namespace CCL

#include "ccl/public/base/ccldefpop.h"

#endif // _skiarendertarget_cocoa_h
