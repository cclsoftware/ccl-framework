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
// Filename    : ccl/platform/cocoa/quartz/quartzlayer.mm
// Description : CoreAnimation Graphics Layer for Quartz content
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/quartz/quartzlayer.h"
#include "ccl/platform/cocoa/gui/cagraphicslayer.cocoa.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/tiledimage.h"
#include "ccl/gui/graphics/graphicshelper.h"
#include "ccl/gui/system/animation.h"
#include "ccl/gui/controls/editbox.h"

#include "ccl/platform/cocoa/quartz/quartzbitmap.h"
#include "ccl/platform/cocoa/quartz/device.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"
#include "ccl/platform/cocoa/cclcocoa.h"

#if CCL_PLATFORM_MAC
#include "ccl/platform/cocoa/gui/platformwindow.mac.h"
#include "ccl/platform/cocoa/gui/window.mac.h"
#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#elif CCL_PLATFORM_IOS
#include "ccl/platform/cocoa/gui/window.ios.h"
#include "ccl/platform/cocoa/gui/nativeview.ios.h"
#endif

#include "ccl/public/systemservices.h"

#include "ccl/public/base/ccldefpush.h"

#include <QuartzCore/CALayer.h>
#include <QuartzCore/CATiledLayer.h>
#include <QuartzCore/CAAnimation.h>
#include <QuartzCore/CAMediaTimingFunction.h>
#include <QuartzCore/CATransaction.h>

#if CCL_PLATFORM_MAC
#define COLORTYPE NSColor
#elif CCL_PLATFORM_IOS
#define COLORTYPE UIColor
#endif

#define LOG_INVALIDATE	 0 && DEBUG_LOG
#define LOG_DRAW		 0 && DEBUG_LOG
#define HIGHLIGHT_LAYERS 0 && DEBUG

using namespace CCL;
using namespace MacOS;

namespace CCL {
namespace MacOS {

class CocoaQuartzLayer;
class CocoaQuartzTiledLayer;

} // namespace MacOS
} // namespace CCL

//************************************************************************************************
// CATiledLayerNoFade
//************************************************************************************************

@interface CCL_ISOLATED (CATiledLayerNoFade) : CATiledLayer
@end

//************************************************************************************************
// LayerContentDelegate
//************************************************************************************************

@interface CCL_ISOLATED (LayerContentDelegate) : NSObject<CALayerDelegate>
{
	CocoaQuartzLayer* layer;
}

- (id)init:(CocoaQuartzLayer*)caLayer;
- (void)setLayer:(CocoaQuartzLayer*)caLayer;

@end

//************************************************************************************************
// TiledLayerContentDelegate
//************************************************************************************************

@interface CCL_ISOLATED (TiledLayerContentDelegate) : CCL_ISOLATED (LayerContentDelegate)
{
	AutoPtr<Threading::ILockable> lock;
}

- (id)init:(CocoaQuartzLayer*)caLayer;
- (Threading::ILockable*)getLock;

@end

//************************************************************************************************
// ClippedLayerContentDelegate
//************************************************************************************************

@interface CCL_ISOLATED (ClippedLayerContentDelegate) : CCL_ISOLATED (LayerContentDelegate)
{
@public
	CocoaQuartzTiledLayer* tiledLayer;
}

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CCL {
namespace MacOS {

//************************************************************************************************
// CocoaQuartzLayer
//************************************************************************************************

class CocoaQuartzLayer: public CoreAnimationLayer
{
public:
	DECLARE_CLASS (CocoaQuartzLayer, CoreAnimationLayer)
	
	CocoaQuartzLayer ();
	~CocoaQuartzLayer ();
	
    PROPERTY_BOOL (suspended, Suspended)

	CALayer* getNativeLayer () const { return nativeLayer; }
	void setContentShift (PointRef offset) { contentShift = offset; }
	PointRef getContentShift () const { return contentShift; }

	virtual void draw (void* context);
	virtual void drawWithShift (void* context, PointRef offset, bool onForeignLayer = false);

	// CoreAnimationLayer
	virtual tresult CCL_API construct (IUnknown* content, RectRef bounds = Rect (), int mode = 0, float contentScaleFactor = 1.f) override;
	virtual tresult CCL_API setContent (IUnknown* content) override;
	virtual void CCL_API setUpdateNeeded () override;
	virtual void CCL_API setUpdateNeeded (RectRef rect) override;
	virtual void CCL_API suspendTiling (tbool suspend, const Rect* visibleRect) override { ; }
	virtual tresult CCL_API flush () override { return kResultOk; }
	
	#if DEBUG
	PROPERTY_MUTABLE_CSTRING (name, Name)
	#endif

protected:
	CCL_ISOLATED (LayerContentDelegate)* contentDelegate;
	Point contentShift;
	
	virtual CCL_ISOLATED (LayerContentDelegate)* createContentDelegate ();
};

//************************************************************************************************
// CocoaQuartzRootLayer
//************************************************************************************************

class CocoaQuartzRootLayer: public CocoaQuartzLayer
{
public:
	DECLARE_CLASS (CocoaQuartzRootLayer, CocoaQuartzLayer)
	
	CocoaQuartzRootLayer ();
	~CocoaQuartzRootLayer ();

	// CocoaQuartzLayer
	tresult CCL_API construct (IUnknown* content, RectRef bounds = Rect (), int mode = 0, float contentScaleFactor = 1.f) override;
	void CCL_API setOffset (PointRef offset) override;
	void CCL_API setSize (Coord width, Coord height) override;

protected:
	#if CCL_PLATFORM_MAC
	NSView* layerHost;
	#endif
};

//************************************************************************************************
// CocoaQuartzTiledLayer
//************************************************************************************************
	
class CocoaQuartzTiledLayer: public CocoaQuartzLayer
{
public:
	DECLARE_CLASS (CocoaQuartzTiledLayer, CocoaQuartzLayer)
	
	CocoaQuartzTiledLayer ();
	~CocoaQuartzTiledLayer ();
	
	ObjectList& getSubLayers () { return sublayers; }
	
protected:
	AutoPtr<CocoaQuartzLayer> clippedLayer;
	ObservedPtr<IGraphicsLayer> savedParent;

	void removeClippedLayer ();
	void checkClippedLayerSize ();
	
	// CocoaQuartzLayer
	CCL_ISOLATED (LayerContentDelegate)* createContentDelegate () override;
	CALayer* createNativeLayer () override;
	void CCL_API setOffset (PointRef offset) override;
	void CCL_API setSize (Coord width, Coord height) override;
	void CCL_API setContentScaleFactor (float factor) override;
	void CCL_API suspendTiling (tbool suspend, const Rect* visibleRect) override;
	void CCL_API setUpdateNeeded () override;
	void CCL_API setUpdateNeeded (RectRef rect) override;
	void CCL_API setTileSize (int size) override;

	void notify (ISubject* subject, MessageRef msg) override;

	int tileSize;
	SignalSink signalSink;
};

//************************************************************************************************
// CocoaQuartzClippedLayer
//************************************************************************************************

class CocoaQuartzClippedLayer: public CocoaQuartzLayer
{
public:
	CocoaQuartzClippedLayer (CocoaQuartzTiledLayer* tiledLayer)
	: tiledLayer (tiledLayer)
	{}
	
	CCL_ISOLATED (LayerContentDelegate)* createContentDelegate ()
	{
		CCL_ISOLATED (ClippedLayerContentDelegate)* d = [[CCL_ISOLATED (ClippedLayerContentDelegate) alloc] init:this];
		d->tiledLayer = tiledLayer;
		return d;
	}
	
	CocoaQuartzTiledLayer* tiledLayer;
};

} // namespace MacOS
} // namespace CCL

//************************************************************************************************
// CATiledLayerNoFade
//************************************************************************************************

@implementation CCL_ISOLATED (CATiledLayerNoFade)
+ (CFTimeInterval)fadeDuration
{
	return 0.;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)init
{
	if(self = [super init])
	{
		#if HIGHLIGHT_LAYERS
		[self setBorderColor: [[COLORTYPE greenColor] CGColor]];
		[self setBorderWidth:1.0];
		#endif
	}
	return self;
}

@end

//************************************************************************************************
// LayerContentDelegate
//************************************************************************************************

@implementation CCL_ISOLATED (LayerContentDelegate)

- (id)init:(CocoaQuartzLayer*)caLayer
{
	if(self = [super init])
		layer = caLayer;
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setLayer:(CocoaQuartzLayer*)caLayer
{
	layer = caLayer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id<CAAction>)actionForLayer:(CALayer*)layer forKey:(NSString*)event
{
	// disable all implicit animations
	return (id<CAAction>)[NSNull null];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)drawLayer:(CALayer*)_layer inContext:(CGContextRef)_context
{
	if(layer)
		layer->draw (_context);
}

@end

//************************************************************************************************
// TiledLayerContentDelegate
//************************************************************************************************

@implementation CCL_ISOLATED (TiledLayerContentDelegate)

- (id)init:(CocoaQuartzLayer*)caLayer
{
	if(self = [super init:caLayer])
		lock = System::CreateAdvancedLock (ClassID::ReadWriteLock);

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (Threading::ILockable*)getLock
{
	return lock;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)drawLayer:(CALayer*)_layer inContext:(CGContextRef)_context
{
	Threading::AutoLock autoLock (lock, Threading::ILockable::kRead);
	if(layer == nil)
		return;

    if(layer->isSuspended ())
        return;

	ASSERT (layer->getNativeLayer () == _layer)
	(NEW Message ("draw", Variant ().setIntPointer ((UIntPtr)_context)))->postBlocking (layer);
}

@end

//************************************************************************************************
// ClippedLayerContentDelegate
//************************************************************************************************

@implementation CCL_ISOLATED (ClippedLayerContentDelegate)

- (void)drawLayer:(CALayer*)_layer inContext:(CGContextRef)_context
{
	if(layer)
	{
		// draw clippedLayer
		layer->draw (_context);
		
		// draw subLayers of tiledLayer on top
		ForEach (tiledLayer->getSubLayers (), CocoaQuartzLayer, subLayer)
		
			// translate coords from subLayer coords to tiledLayer (parent)
			CGPoint subLayerPos (subLayer->getNativeLayer ().frame.origin);
			CCL::Point p ((int)subLayerPos.x, (int)subLayerPos.y);
		
			// translate from tiledLayer to clippedLayer
			p.offset (layer->getContentShift ());

			// clip to Sublayer bounds
			::CGContextSaveGState (_context);
			CGSize size (subLayer->getNativeLayer ().frame.size);
			CCL::Rect rect (0, 0, (int)size.width, (int)size.height);
			rect.moveTo (p);
			CGRect rect2;
			toCGRect (rect2, rect);
			::CGContextClipToRect (_context, rect2);

			subLayer->drawWithShift (_context, p, true);
			::CGContextRestoreGState (_context);
		EndFor
	}
}

@end

//************************************************************************************************
// CocoaQuartzLayerFactory
//************************************************************************************************

IGraphicsLayer* CocoaQuartzLayerFactory::createLayer (UIDRef classID)
{
	if(classID == ClassID::RootLayer)
		return NEW CocoaQuartzRootLayer;
	if(classID == ClassID::GraphicsLayer)
		return NEW CocoaQuartzLayer;
	if(classID == ClassID::TiledLayer)
		return NEW CocoaQuartzTiledLayer;
	return nullptr;
}

//************************************************************************************************
// CocoaQuartzLayer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CocoaQuartzLayer, CoreAnimationLayer)
	
//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaQuartzLayer::CocoaQuartzLayer ()
: contentDelegate (nil),
  suspended (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaQuartzLayer::~CocoaQuartzLayer ()
{
	if(nativeLayer)
		nativeLayer.delegate = nullptr;

	if(contentDelegate)
		[contentDelegate release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaQuartzLayer::construct (IUnknown* _content, RectRef bounds, int mode, float contentScaleFactor)
{
	ASSERT (!content.isValid ()) // must be called only once
	if(content.isValid ())
		return kResultUnexpected;

	ASSERT (!nativeLayer)
	if(!nativeLayer)
	{
		nativeLayer = createNativeLayer ();
		
		#if CCL_PLATFORM_MAC
		nativeLayer.anchorPoint = CGPointMake (0.f, 1.f);
		#else
		nativeLayer.anchorPoint = CGPointMake (0.f, 0.f);		
		#endif
		
		nativeLayer.masksToBounds = (mode & kClipToBounds) ? YES : NO;	// clip sublayers
		nativeLayer.opaque = (mode & kIgnoreAlpha) ? YES : NO;
	}
	
	setSize (bounds.getWidth (), bounds.getHeight ());
	setOffset (bounds.getLeftTop ());
	setContent (_content);
	setContentScaleFactor (contentScaleFactor);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaQuartzLayer::setContent (IUnknown* _content)
{
	content = _content;
	
	// if it's an image, just set the image to be the layer's contents...
	if(Image* image = unknown_cast<Image> (content))
	{
		Rect originalRect;
		if(Bitmap* originalBitmap = Bitmap::getOriginalBitmap (originalRect, image))
		{
			QuartzBitmap* quartzBitmap = nullptr;
			MultiResolutionBitmap* multiBitmap = ccl_cast<MultiResolutionBitmap> (originalBitmap);
			if(Bitmap::isHighResolutionScaling (getContentScaleFactor ()) && multiBitmap)
				quartzBitmap = ccl_cast<QuartzBitmap>(multiBitmap->getNativeBitmap2x ());
			else
				quartzBitmap = ccl_cast<QuartzBitmap>(originalBitmap->getNativeBitmap ());

			if(quartzBitmap)
			{
				CGImageRef img = quartzBitmap->getCGImage ();
				if(img)
				{
					// disable CoreAnimation default animations
					[CATransaction begin]; 
					[CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
					CGRect layerRect = nativeLayer.frame;
					layerRect.size = CGSizeMake (image->getWidth (), image->getHeight ());
					nativeLayer.frame = layerRect;
					nativeLayer.contents =  (id)img;
					PRINT_CGRECT ("layer::setContent ", nativeLayer.frame)
					#if DEBUG
					this->name.appendFormat("%s %d x %d", image->myClass ().getPersistentName (), image->getWidth (), image->getHeight ());
					#endif

					if(originalBitmap->getHeight () != image->getHeight () || originalBitmap->getWidth () != image->getWidth ())
					{
						CGFloat w = originalBitmap->getWidth ();
						CGFloat h = originalBitmap->getHeight ();
						if(w != 0. && h != 0.)
							nativeLayer.contentsRect = CGRectMake (originalRect.left / w, originalRect.top / h, originalRect.getWidth () / w, originalRect.getHeight () / h);
						CCL_PRINTF ("original bmp: %d x %d, used: %d, %d, %d, %d\n", originalBitmap->getWidth (), originalBitmap->getHeight (), originalRect.left, originalRect.top, originalRect.right, originalRect.bottom)
					}

					if(TiledImage* tiledImage = ccl_cast<TiledImage> (image))
					{
						// note: contentsCenter does not tile, but stretch the bitmap...
						Rect m (tiledImage->getMargins ());
						CGFloat width = tiledImage->getWidth ();
						CGFloat height = tiledImage->getHeight ();
						CGRect center = CGRectMake (m.left / width, m.top / height, (width - m.left - m.right) / width, (height - m.top - m.bottom) / height);
						nativeLayer.contentsCenter = center;
						CCL_PRINTF ("TiledImage contentsCenter: %f, %f, (%f x %f)\n", center.origin.x, center.origin.y, center.size.width, center.size.height)
					}
					[CATransaction commit];
					// don't call setNeedsDisplay, it'll obliterate our contents!
				}
			}
		}
	}
	else
	{
		UnknownPtr<IGraphicsLayerContent> layerContent (content);
		if(layerContent)
		{
			#if DEBUG
			UnknownPtr<IView> view (content);
			if(view)
			{
				UnknownPtr<IObject> object (view);
				if(object)
					this->name = object->getTypeInfo ().getClassName ();
				
				Variant name;
				view->getViewAttribute (name, IView::kName);
				if(!name.asString ().isEmpty ())
					(this->name += " ") += name.asString ();
			}
			#endif
			if(!contentDelegate)
				contentDelegate = createContentDelegate ();
			
			nativeLayer.delegate = contentDelegate;
			nativeLayer.needsDisplayOnBoundsChange = YES;

			setUpdateNeeded ();
		}
	}
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaQuartzLayer::draw (void* context)
{
	drawWithShift (context, contentShift);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaQuartzLayer::drawWithShift (void* context, PointRef offset, bool onForeignLayer)
{
	UnknownPtr<IGraphicsLayerContent> layerContent (content);
	if(!layerContent)
		return;

	IGraphicsLayerContent::LayerHint layerHint = layerContent->getLayerHint ();
	if(layerHint == kGraphicsContentEmpty)
		return;

	CGRect boundingBox = CGContextGetClipBoundingBox ((CGContextRef)context);
	if(!onForeignLayer && layerHint == kGraphicsContentTranslucent) // don't clear when drawing onto a another layer
		CGContextClearRect ((CGContextRef)context, boundingBox);
	
	CCL::Rect clipRect;
	fromCGRect (clipRect, boundingBox);
	clipRect.offset (-offset.x, -offset.y);

	#if LOG_DRAW
	CCL_PRINT("drawLayer ") PRINT_CGRECT (getName ().str (), boundingBox)
	#endif
	
	QuartzLayerRenderTarget renderTarget ((CGContextRef)context, getContentScaleFactor ());
	QuartzScopedGraphicsDevice nativeDevice (renderTarget, static_cast<Unknown&> (renderTarget));
	GraphicsDevice device;
	device.setNativeDevice (&nativeDevice);
	
	layerContent->drawLayer (device, UpdateRgn (clipRect), offset);

	#if HIGHLIGHT_LAYERS
	if([nativeLayer isKindOfClass:[CATiledLayer class]])
		device.drawRect (clipRect, Pen (Colors::kRed));
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaQuartzLayer::setUpdateNeeded ()
{
	if(!unknown_cast<Image> (content))
	{
		[nativeLayer setNeedsDisplay];

		#if LOG_INVALIDATE
		CCL_PRINTF ("layer \"%s\" ", name.str ()) PRINT_CGRECT ("setUpdateNeeded: (full) ", nativeLayer.bounds)
		#endif
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaQuartzLayer::setUpdateNeeded (RectRef rect)
{
	if(!unknown_cast<Image> (content))
	{
		CGRect r;
		toCGRect(r, rect);
		[nativeLayer setNeedsDisplayInRect:r];

		#if LOG_INVALIDATE
		CCL_PRINTF ("layer \"%s\" ", name.str ()) PRINT_CGRECT ("setUpdateNeeded: ", r)
		#endif
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_ISOLATED (LayerContentDelegate)* CocoaQuartzLayer::createContentDelegate ()
{
	return [[CCL_ISOLATED (LayerContentDelegate) alloc] init:this];
}

//************************************************************************************************
// CocoaQuartzRootLayer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CocoaQuartzRootLayer, CocoaQuartzLayer)

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaQuartzRootLayer::CocoaQuartzRootLayer ()
#if CCL_PLATFORM_MAC
: layerHost (nil)
#endif
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaQuartzRootLayer::~CocoaQuartzRootLayer ()
{
	#if CCL_PLATFORM_MAC
	if(layerHost)
	{
		[layerHost removeFromSuperview];
		[layerHost release];
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaQuartzRootLayer::construct (IUnknown* _content, RectRef bounds, int mode, float contentScaleFactor)
{
	Window* window = unknown_cast<Window> (_content);
	// do not assign _content to content, this object is owned by the window
	ASSERT (window)
	if(!window)
		return kResultInvalidArgument;
	
	#if CCL_PLATFORM_IOS
	NativeView* nativeView = IOSWindow::cast (window)->getNativeView ();
	if(!nativeView)
		return kResultInvalidArgument;
	nativeLayer = nativeView->getView ().layer;
	
	#else
	
	OSXWindow* osxWindow = OSXWindow::cast (window);
	ASSERT (osxWindow)
	RectRef windowSize = osxWindow->getSize ();
	layerHost = [[CCL_ISOLATED (FlippedView) alloc] initWithFrame:CGRectMake (0, 0, windowSize.getWidth (), windowSize.getHeight ())];
	[layerHost setWantsLayer:YES];
	[layerHost setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	NativeView layerView (layerHost);
	osxWindow->embed (&layerView);

	nativeLayer = layerHost.layer;
	ASSERT (!contentDelegate)
	if(!contentDelegate)
		contentDelegate = [[CCL_ISOLATED (LayerContentDelegate) alloc] init];
	nativeLayer.delegate = contentDelegate;
	#endif
	
	[nativeLayer retain];
	
	setContentScaleFactor (contentScaleFactor);

	#if DEBUG
	name ="ROOT";
	#endif
	PRINT_CGRECT ("ROOT layer: ", nativeLayer.bounds)
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaQuartzRootLayer::setOffset (PointRef offset)
{
	// root layer is automaticaly sized
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaQuartzRootLayer::setSize (Coord width, Coord height)
{
	// root layer is automaticaly sized
}

//************************************************************************************************
// CocoaQuartzTiledLayer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CocoaQuartzTiledLayer, CocoaQuartzLayer)

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaQuartzTiledLayer::CocoaQuartzTiledLayer ()
: clippedLayer (nullptr),
  tileSize (256),
  signalSink (Signals::kNativeTextControl)
{
	signalSink.setObserver (this);
	signalSink.enable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
CocoaQuartzTiledLayer::~CocoaQuartzTiledLayer ()
{
	setSuspended (true);
	removeClippedLayer ();
	cancelSignals ();
	signalSink.enable (false);
	if(contentDelegate)
	{
		Threading::AutoLock autoLock ([(CCL_ISOLATED (TiledLayerContentDelegate)*)contentDelegate getLock], Threading::ILockable::kWrite);
		[contentDelegate setLayer:nil];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CALayer* CocoaQuartzTiledLayer::createNativeLayer ()
{
	return [[CCL_ISOLATED (CATiledLayerNoFade) alloc] init];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_ISOLATED (LayerContentDelegate)* CocoaQuartzTiledLayer::createContentDelegate ()
{
	return [[CCL_ISOLATED (TiledLayerContentDelegate) alloc] init:this];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaQuartzTiledLayer::setContentScaleFactor (float factor)
{
	CoreAnimationLayer::setContentScaleFactor (factor);
	if(CATiledLayer* tiledLayer = (CATiledLayer*)nativeLayer)
		tiledLayer.tileSize =  CGSizeMake (tileSize * factor, tileSize * factor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaQuartzTiledLayer::suspendTiling (tbool suspend, const Rect* visibleRect)
{
	if(isSuspended () == suspend)
		return;

	setSuspended (suspend);
	CCL_PRINTF ("%s (%p) : suspend tiling: %d\n", name.str (), this, suspend);
	if(suspend)
	{
		removeClippedLayer ();
		clippedLayer = NEW CocoaQuartzClippedLayer (this);
		clippedLayer->construct (content);
		clippedLayer->setContentScaleFactor (getContentScaleFactor ());
		
		Point offset ((Coord)nativeLayer.frame.origin.x, (Coord)nativeLayer.frame.origin.y);
		clippedLayer->setContentShift (offset);

		if(visibleRect)
			clippedLayer->setSize (visibleRect->getWidth (), visibleRect->getHeight ());
		else if(CoreAnimationLayer* parent = unknown_cast<CoreAnimationLayer> (getParentLayer ()))
		{
			Coord width = 0;
			Coord height = 0;
			parent->getSize (width, height);
			clippedLayer->setSize (width, height);
		}

		getParentLayer ()->addSublayer (clippedLayer);

		// iOS crashes internally when a UITextView closes and a CATiledLayer is active
		// temporary workaround: the application suspends tiling during text input and the tiled layer is disconnected here
		if(NativeTextControl::isNativeTextControlPresent ())
		{
			savedParent = getParentLayer ();
			getParentLayer ()->removeSublayer (this);
		}

		#if HIGHLIGHT_LAYERS
		CALayer* layer = clippedLayer->getNativeLayer ();
		[layer setBorderColor: [[COLORTYPE blueColor] CGColor]];
		[layer setBorderWidth:1.0];
		#endif
	}
	else
	{
		setUpdateNeeded ();

		if(savedParent)
		{
			savedParent->addSublayer (this);
			savedParent = nullptr;
		}

		// defer removing the clippedLayer, to give the tiled layer time to get drawn
		if(clippedLayer)
			(NEW Message ("removeClippedLayer", clippedLayer->asUnknown ()))->post (this, 300);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaQuartzTiledLayer::removeClippedLayer ()
{
	if(clippedLayer)
	{
		CCL_PRINTF ("removeClippedLayer %p retainCount=%d\n", (CoreAnimationLayer*)clippedLayer, clippedLayer->getRetainCount ())
		if(IGraphicsLayer* parent = clippedLayer->getParentLayer ())
			parent->removeSublayer (clippedLayer);
		clippedLayer = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaQuartzTiledLayer::setOffset (PointRef offset)
{
	SuperClass::setOffset (offset);
	if(clippedLayer)
	{
		clippedLayer->setContentShift (offset);
		clippedLayer->setUpdateNeeded ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaQuartzTiledLayer::setSize (Coord width, Coord height)
{
	CGSize oldSize = nativeLayer.frame.size;
	if((oldSize.width > 0 && width > oldSize.width) || (oldSize.height > 0 && height > oldSize.height))
		(NEW Message ("invalidate"))->post (this, 100);

	SuperClass::setSize (width, height);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaQuartzTiledLayer::setUpdateNeeded ()
{
	SuperClass::setUpdateNeeded ();
	if(clippedLayer)
	{
		checkClippedLayerSize ();
		clippedLayer->setUpdateNeeded ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaQuartzTiledLayer::setUpdateNeeded (RectRef rect)
{
	SuperClass::setUpdateNeeded (rect);
	if(clippedLayer)
	{
		checkClippedLayerSize ();
		clippedLayer->setUpdateNeeded (rect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaQuartzTiledLayer::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "draw")
	{
		void* context = (void*)msg[0].asIntPointer ();
		draw (context);
	}
	else if(msg == "removeClippedLayer" && msg[0].asUnknown () == clippedLayer->asUnknown ())
		removeClippedLayer ();
	else if(msg == "invalidate")
		setUpdateNeeded ();
	else if(msg == Signals::kNativeTextControlCreated || msg == Signals::kNativeTextControlDestroyed)
	{
		bool needsSuspend = NativeTextControl::isNativeTextControlPresent ();
		if(isSuspended () != needsSuspend)
			suspendTiling (needsSuspend, nullptr);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaQuartzTiledLayer::setTileSize (int size)
{
	tileSize = size;
	if(CATiledLayer* tiledLayer = (CATiledLayer*)nativeLayer)
		tiledLayer.tileSize = CGSizeMake (tileSize * getContentScaleFactor (), tileSize * getContentScaleFactor ());
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CocoaQuartzTiledLayer::checkClippedLayerSize ()
{
	if(clippedLayer)
		if(auto parent = static_cast<CocoaQuartzLayer*> (parentLayer))
		{
			// the clipped layer must completely cover the visible part of the tiled layer
			// if this visible part has grown (because the parent layer has grown), we must enlarge the clipped layer as well
			CGSize parentSize (parent->getNativeLayer ().frame.size);
			Rect parentRect (0, 0, (int)parentSize.width, (int)parentSize.height);

			Rect visibleRect;
			fromCGRect (visibleRect, getNativeLayer ().frame);
			visibleRect.bound (parentRect);

			Rect clippedRect;
			fromCGRect (clippedRect, clippedLayer->getNativeLayer ().frame);
			clippedRect.bound (parentRect);

			Point uncovered (visibleRect.getSize () - clippedRect.getSize ());
			if(uncovered.x > 0 || uncovered.y > 0)
			{
				constexpr Coord kGrowStep = 40; // avoid resizing too often (clipped by parent layer anyway)
				if(uncovered.x > 0)
					uncovered.x = (uncovered.x / kGrowStep + 1) * kGrowStep;
				if(uncovered.y > 0)
					uncovered.y = (uncovered.y / kGrowStep + 1) * kGrowStep;

				Point newSize = clippedRect.getSize () + uncovered;
				clippedLayer->setSize (newSize.x, newSize.y);
				CCL_PRINTF ("resize clippedLayer by %d, %d: \t%d %d\n", uncovered.x, uncovered.y, newSize.x, newSize.y)
			}
		}
	}
