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
// Filename    : ccl/platform/cocoa/quartz/quartzrendertarget.h
// Description : Quartz Engine
//
//************************************************************************************************

#ifndef _ccl_quartzrendertarget_h
#define _ccl_quartzrendertarget_h

#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/mutableregion.h"
#include "ccl/platform/cocoa/gui/window.mac.h"
#include "ccl/platform/cocoa/metal/metal3dsupport.h"

#include "ccl/public/base/ccldefpush.h"
#include <CoreGraphics/CGContext.h>
#include "ccl/public/base/ccldefpop.h"

namespace CCL {
namespace MacOS {

class NativeView;
class QuartzBitmap;
class QuartzBitmapRenderTarget;

//************************************************************************************************
// QuartzRenderTarget
//************************************************************************************************
	
class QuartzRenderTarget
{
public:
    QuartzRenderTarget (CGContextRef context = 0);
    
	virtual CGContextRef getContext () = 0;
    virtual void flush () = 0;
	virtual float getContentScaleFactor () const = 0;
	
protected:
    CGContextRef context;
	Vector<Metal3DSurface*> surfaces;
	
	virtual void addMetal3DSurface (Native3DSurface* surface);
	virtual void removeMetal3DSurface (Native3DSurface* surface);
};

//************************************************************************************************
// QuartzLayerRenderTarget
//************************************************************************************************
	
class QuartzLayerRenderTarget: public Object,
							   public QuartzRenderTarget
{
public:
	DECLARE_CLASS_ABSTRACT (QuartzLayerRenderTarget, Object)
	
	QuartzLayerRenderTarget (CGContextRef _context, float contentScaleFactor);
	
	// QuartzRenderTarget
	CGContextRef getContext () override { return context; }
    void flush () override { ; }
	float getContentScaleFactor () const override { return contentScaleFactor; }
	
protected:
	CGContextRef context;
	float contentScaleFactor;
};

//************************************************************************************************
// NativeViewUpdateRegion
//************************************************************************************************

class NativeViewUpdateRegion: public MutableRegion
{
public:
	NativeViewUpdateRegion () { ; }
	
	void setNativeView (NativeView* _nativeView) { nativeView = _nativeView; }

	// MutableRegion
	void CCL_API addRect (RectRef rect);
		
protected:
	NativeView* nativeView;
};

#if CCL_PLATFORM_MAC

//************************************************************************************************
// QuartzOSXWindowRenderTarget
//************************************************************************************************

class QuartzOSXWindowRenderTarget: public NativeWindowRenderTarget,
								   public QuartzRenderTarget
{
public:
	DECLARE_CLASS_ABSTRACT (QuartzOSXWindowRenderTarget, NativeWindowRenderTarget)
	
	QuartzOSXWindowRenderTarget (Window& window);
    ~QuartzOSXWindowRenderTarget ();
	
	// NativeWindowRenderTarget
	bool shouldCollectUpdates () override;
    IMutableRegion* getUpdateRegion () override;
	IMutableRegion* getInvalidateRegion () override { return &invalidateRegion; }
	void onRender () override;
	void onSize () override {;}
	void onScroll (RectRef rect, PointRef delta) override;
	void add3DSurface (Native3DSurface* surface) override;
	void remove3DSurface (Native3DSurface* surface) override;
	
	// QuartzRenderTarget
    float getContentScaleFactor () const override;
    CGContextRef getContext () override;
    void flush () override;
	
protected:
	SharedPtr<NativeView> nativeView;
	AutoPtr<QuartzBitmap> offscreen;
	AutoPtr<QuartzBitmapRenderTarget> offscreenTarget;
	NativeViewUpdateRegion invalidateRegion;
    bool didLockFocus;
    
    void createContext ();
    void releaseContext ();
};
#endif

#if CCL_PLATFORM_IOS
//************************************************************************************************
// QuartzIOSWindowRenderTarget
//************************************************************************************************
	
class QuartzIOSWindowRenderTarget: public NativeWindowRenderTarget,
								   public QuartzRenderTarget
{
public:
	DECLARE_CLASS_ABSTRACT (QuartzIOSWindowRenderTarget, NativeWindowRenderTarget)
	
	QuartzIOSWindowRenderTarget (Window& window);
	~QuartzIOSWindowRenderTarget ();
	
	// NativeWindowRenderTarget
	bool shouldCollectUpdates () override;
	IMutableRegion* getUpdateRegion () override;
	IMutableRegion* getInvalidateRegion ()  override { return &invalidateRegion; }
	void onRender () override;
	void onSize () override;
	void onScroll (RectRef rect, PointRef delta) override;
	void add3DSurface (Native3DSurface* surface) override;
	void remove3DSurface (Native3DSurface* surface) override;
	
	// QuartzRenderTarget
	float getContentScaleFactor () const override;
	CGContextRef getContext () override;
    void flush () override { ; }
	
protected:
	SharedPtr<NativeView> nativeView;
	NativeViewUpdateRegion invalidateRegion;
};

#endif

} // namespace MacOS
} // namespace CCL

#endif // ccl_quartzrendertarget_h
