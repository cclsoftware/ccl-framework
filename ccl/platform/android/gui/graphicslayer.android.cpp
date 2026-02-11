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
// Filename    : ccl/platform/android/gui/graphicslayer.android.cpp
// Description : Android Graphixs Layer implementation
//
//************************************************************************************************

#include "ccl/platform/android/gui/graphicslayer.android.h"

#include "ccl/platform/android/gui/androidview.h"
#include "ccl/platform/android/gui/frameworkactivity.h"
#include "ccl/platform/android/gui/frameworkview.h"
#include "ccl/platform/android/gui/window.android.h"

#include "ccl/platform/android/graphics/androidbitmap.h"
#include "ccl/platform/android/graphics/frameworkgraphics.h"

#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/graphicshelper.h"

#include "ccl/gui/system/animation.h"

#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/gui/graphics/updatergn.h"

#define USE_BITMAP_CONTENT 1

using namespace CCL;

//************************************************************************************************
// JNI classes
//************************************************************************************************

namespace CCL {
namespace Android {

//************************************************************************************************
// GraphicsLayerView
//************************************************************************************************

DECLARE_JNI_CLASS (GraphicsLayerView, CCLGUI_CLASS_PREFIX "GraphicsLayerView")
	DECLARE_JNI_CONSTRUCTOR (construct, JniIntPtr, jobject, int, int)
	DECLARE_JNI_METHOD (void, setSize, int, int)
	DECLARE_JNI_METHOD (void, setMode, bool, bool)
	DECLARE_JNI_METHOD (void, setBackground, jobject)
	DECLARE_JNI_METHOD (void, setTransform, float, float, float, float, float, float)
	DECLARE_JNI_METHOD (void, addTransformAnimation, int64, int, int, bool, JniIntPtr, float, float, float, float, float, float)
	DECLARE_JNI_METHOD (void, addAlphaAnimation, int64, int, int, bool, JniIntPtr, float)
	DECLARE_JNI_METHOD (void, addOffsetXAnimation, int64, int, int, bool, JniIntPtr, float)
	DECLARE_JNI_METHOD (void, addOffsetYAnimation, int64, int, int, bool, JniIntPtr, float)
	DECLARE_JNI_METHOD (void, addOffsetAnimation, int64, int, int, bool, JniIntPtr, int, int)
	DECLARE_JNI_METHOD (void, removeAnimation)
	DECLARE_JNI_METHOD (void, setUpdateNeeded)
	DECLARE_JNI_FIELD (bool, isSprite)
END_DECLARE_JNI_CLASS (GraphicsLayerView)

DEFINE_JNI_CLASS (GraphicsLayerView)
	DEFINE_JNI_CONSTRUCTOR (construct, "(JLandroid/content/Context;II)V")
	DEFINE_JNI_METHOD (setSize, "(II)V")
	DEFINE_JNI_METHOD (setMode, "(ZZ)V")
	DEFINE_JNI_METHOD (setBackground, "(Landroid/graphics/Bitmap;)V")
	DEFINE_JNI_METHOD (setTransform, "(FFFFFF)V")
	DEFINE_JNI_METHOD (addTransformAnimation, "(JIIZJFFFFFF)V")
	DEFINE_JNI_METHOD (addAlphaAnimation, "(JIIZJF)V")
	DEFINE_JNI_METHOD (addOffsetXAnimation, "(JIIZJF)V")
	DEFINE_JNI_METHOD (addOffsetYAnimation, "(JIIZJF)V")
	DEFINE_JNI_METHOD (addOffsetAnimation, "(JIIZJII)V")
	DEFINE_JNI_METHOD (removeAnimation, "()V")
	DEFINE_JNI_METHOD (setUpdateNeeded, "()V")
	DEFINE_JNI_FIELD (isSprite, "Z")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// AndroidGraphicsLayer::AnimationListener
//************************************************************************************************

class AndroidGraphicsLayer::AnimationListener: public JniCast<AnimationListener>
{
public:
	AnimationListener (IAnimationCompletionHandler* completionHandler)
	: completionHandler (completionHandler)
	{}

	SharedPtr<IAnimationCompletionHandler> completionHandler;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, LayerAnimation, onAnimationEndNative, JniIntPtr nativeListenerPtr)
{
	if(AndroidGraphicsLayer::AnimationListener* listener = JniCast<AndroidGraphicsLayer::AnimationListener>::fromIntPtr (nativeListenerPtr))
	{	
		if(listener->completionHandler)
			listener->completionHandler->onAnimationFinished ();
		delete listener;
	}
}

//************************************************************************************************
// AndroidGraphicsLayer::AnimationHelper
//************************************************************************************************

class AndroidGraphicsLayer::AnimationHelper
{
public:
	typedef const JniMethodTyped<void, float>& JniSetPropertyMethodRef;
	typedef const JniMethodTyped<void, int64, int, int, bool, JniIntPtr, float>& JniAnimatePropertyMethodRef;

	AnimationHelper (const Animation& animation)
	: duration (jlong(animation.getDuration () * 1000)),
  	  timing (animation.getTimingType ()),
	  repeatCount (animation.getRepeatCount ()),
	  autoReverse (animation.isAutoReverse ()),
	  listener (0)
	{
		if(animation.getCompletionHandler ())
			listener = (NEW AnimationListener (animation.getCompletionHandler ()))->asIntPtr ();
	}

	void addBasicAnimation (jobject layerView, JniSetPropertyMethodRef setProperty, JniAnimatePropertyMethodRef& animateProperty, float startValue, float endValue)
	{
		// apply startValue, animate to endValue
		setProperty (layerView, startValue);
		animateProperty (layerView, duration, timing, repeatCount, autoReverse, listener, endValue);
	}

	jlong duration;
	jint timing;
	jint repeatCount;
	jboolean autoReverse;
	JniIntPtr listener;
};

//************************************************************************************************
// AndroidGraphicsLayer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AndroidGraphicsLayer, NativeGraphicsLayer)

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, GraphicsLayerView, onViewCreatedNative, JniIntPtr nativeLayerPtr, jobject graphics)
{
	AndroidGraphicsLayer* layer = JniCast<AndroidGraphicsLayer>::fromIntPtr (nativeLayerPtr);
	if(layer && graphics)
		layer->setGraphics (NEW FrameworkGraphics (env, graphics));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, GraphicsLayerView, redrawNative, JniIntPtr nativeLayerPtr)
{
	if(AndroidGraphicsLayer* layer = JniCast<AndroidGraphicsLayer>::fromIntPtr (nativeLayerPtr))
		layer->redraw ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidGraphicsLayer::AndroidGraphicsLayer ()
: graphics (nullptr),
  contentScaleFactor (1.f)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidGraphicsLayer::~AndroidGraphicsLayer ()
{
	delete graphics;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidGraphicsLayer::construct (IUnknown* _content, RectRef bounds, int mode, float contentScaleFactor)
{
	ASSERT (!content.isValid ()) // must be called only once
	if(content.isValid ())
		return kResultUnexpected;

	size (0, 0, bounds.getWidth (), bounds.getHeight ());
	#if USE_LAYER_DIRTY_REGION
	dirtyRegion.join (size);
	#endif

	PixelPoint pixelSize (size.getSize (), contentScaleFactor);

	ASSERT (!layerView)
	if(!layerView)
	{
		JniAccessor jni;
		layerView.assign (jni, jni.newObject (GraphicsLayerView, GraphicsLayerView.construct, asIntPtr (), FrameworkActivity::getCurrentActivity ()->getJObject (), pixelSize.x, pixelSize.y));
	}
	
	setMode (mode);
	setContentScaleFactor (contentScaleFactor);	

	setOffset (bounds.getLeftTop ());
	setContent (_content);
	
	#if USE_LAYER_DIRTY_REGION
	setUpdateNeeded ();
	#endif
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGraphicsLayer::isSprite (bool state)
{
	JniAccessor jni;
	jni.setField (layerView, GraphicsLayerView.isSprite, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGraphicsLayer::setGraphics (FrameworkGraphics* _graphics)
{
	graphics = _graphics;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidGraphicsLayer::setContent (IUnknown* _content)
{
	content = _content;
	
	if(Bitmap* bitmap = unknown_cast<Bitmap> (content))
	{
		#if USE_BITMAP_CONTENT
		if(AndroidBitmap* androidBitmap = ccl_cast<AndroidBitmap> (bitmap->getNativeBitmap ()))
			GraphicsLayerView.setBackground (layerView, *androidBitmap->getJavaBitmap ());
		#endif
	}
	else
	{
		UnknownPtr<IGraphicsLayerContent> layerContent (content);
		if(layerContent)
			setUpdateNeeded ();
	}
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidGraphicsLayer::redraw ()
{
	FrameworkGraphics::ScaleHelper scaler (graphics, contentScaleFactor);

	#if 0 && DEBUG
	if(graphics)
		graphics->fillEllipse (Rect (0, 0, size.getSize ()), SolidBrush (Color (0,0,0xff,40)));
	#endif
	
	if(Bitmap* bitmap = unknown_cast<Bitmap> (content))
	{
		#if !USE_BITMAP_CONTENT
		AndroidBitmap* androidBitmap = ccl_cast<AndroidBitmap> (bitmap->getNativeBitmap ());
		if(androidBitmap && graphics)
			androidBitmap->draw (*graphics, Point ());
		#endif
		return;
	}

	UnknownPtr<IGraphicsLayerContent> layerContent (content);
	if(!layerContent || ! graphics)
		return;

	IGraphicsLayerContent::LayerHint layerHint = layerContent->getLayerHint ();
	if(layerHint == kGraphicsContentEmpty)
		return;
	
	GraphicsDevice device;
	device.setNativeDevice (graphics);
	#if USE_LAYER_DIRTY_REGION
	if(!dirtyRegion.isEmpty ())
	{
		ForEachRectFast (dirtyRegion, Rect, r)
			layerContent->drawLayer (device, UpdateRgn (r), Point ());
		EndFor
		dirtyRegion.setEmpty ();
	}
	#else
	Rect dirtyRect (0, 0, size.getSize ());
	graphics->getClipBounds (dirtyRect);
	layerContent->drawLayer (device, UpdateRgn (dirtyRect), Point ());
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsLayer::setOffset (PointRef offset)
{
	if(offset != size.getLeftTop ())
	{
		size.moveTo (offset);

		PixelPointF pixelPoint (offset, contentScaleFactor);

		AndroidView.setX (layerView, pixelPoint.x);
		AndroidView.setY (layerView, pixelPoint.y);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsLayer::setOffsetX (float offsetX)
{
	if(offsetX != size.left)
	{
		size.moveTo (Point (coordFToInt (offsetX), size.top));

		AndroidView.setX (layerView, DpiScale::coordFToPixelF (offsetX, contentScaleFactor));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsLayer::setOffsetY (float offsetY)
{
	if(offsetY != size.top)
	{
		size.moveTo (Point (size.left, coordFToInt (offsetY)));

		AndroidView.setY (layerView, DpiScale::coordFToPixelF (offsetY, contentScaleFactor));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsLayer::setSize (Coord width, Coord height)
{
	Point newSize (width, height);
	if(newSize != size.getSize ())
	{
		size.setSize (newSize);
		#if USE_LAYER_DIRTY_REGION
		dirtyRegion.join (Rect (0, 0, width, height));
		#endif

		PixelPoint pixelSize (newSize, contentScaleFactor);

		GraphicsLayerView.setSize (layerView, pixelSize.x, pixelSize.y);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsLayer::setMode (int mode)
{
	bool ignoreAlpha = mode & kIgnoreAlpha;
	bool clipChildren = mode & kClipToBounds;

	GraphicsLayerView.setMode (layerView, ignoreAlpha, clipChildren);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsLayer::setOpacity (float opacity)
{
	AndroidView.setAlpha (layerView, opacity);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsLayer::setTransform (TransformRef t)
{
	// add layer offset
	PixelPointF offset (size.getLeftTop (), contentScaleFactor);

	GraphicsLayerView.setTransform (layerView, t.a0, t.a1, t.b0, t.b1, t.t0 + offset.x, t.t1 + offset.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsLayer::setContentScaleFactor (float factor)
{
	contentScaleFactor = factor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsLayer::setUpdateNeeded ()
{
	if(!unknown_cast<Image> (content))
	{
		#if USE_LAYER_DIRTY_REGION
		dirtyRegion.join (Rect (0, 0, size.getWidth (), size.getHeight ()));
		#endif

		GraphicsLayerView.setUpdateNeeded (layerView);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsLayer::setUpdateNeeded (RectRef rect)
{
	if(!unknown_cast<Image> (content))
	{
		#if USE_LAYER_DIRTY_REGION
		dirtyRegion.join (Rect (rect.left, rect.top, rect.right, rect.bottom));
		#endif

		GraphicsLayerView.setUpdateNeeded (layerView);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidGraphicsLayer::addSublayer (IGraphicsLayer* layer)
{
	tresult tr = SuperClass::addSublayer (layer);
	if(tr == kResultOk)
		if(AndroidGraphicsLayer* androidSubLayer = unknown_cast<AndroidGraphicsLayer> (layer))
			ViewGroup.addView (layerView, androidSubLayer->layerView);

	return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidGraphicsLayer::removeSublayer (IGraphicsLayer* layer)
{
	tresult tr = SuperClass::removeSublayer (layer);
	if(tr == kResultOk)
		if(AndroidGraphicsLayer* androidSubLayer = unknown_cast<AndroidGraphicsLayer> (layer))
			ViewGroup.removeView (layerView, androidSubLayer->layerView);

	return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidGraphicsLayer::placeAbove (IGraphicsLayer* layer, IGraphicsLayer* sibling)
{
    tresult tr = SuperClass::placeAbove (layer, sibling);
    if(tr == kResultOk)
        return moveLayerView (layer, sibling, true);

    return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidGraphicsLayer::placeBelow (IGraphicsLayer* layer, IGraphicsLayer* sibling)
{
    tresult tr = SuperClass::placeBelow (layer, sibling);
    if(tr == kResultOk)
        return moveLayerView (layer, sibling, false);

    return tr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AndroidGraphicsLayer::moveLayerView (IGraphicsLayer* layer, IGraphicsLayer* sibling, bool above)
{
    auto* androidLayer= unknown_cast<AndroidGraphicsLayer> (layer);
    auto* androidSibling= unknown_cast<AndroidGraphicsLayer> (sibling);
    if(androidLayer && androidSibling)
    {
        int currentIndex = ViewGroup.indexOfChild (layerView, androidLayer->layerView);
        int siblingIndex = ViewGroup.indexOfChild (layerView, androidSibling->layerView);
        if(siblingIndex < 0 || currentIndex < 0)
            return kResultFailed;

        int insertIndex = siblingIndex;
        if(above)
            insertIndex++;
        if(currentIndex < siblingIndex)
            insertIndex += 1;

        if(currentIndex != insertIndex)
        {
            ViewGroup.removeView (layerView, androidLayer->layerView);
            ViewGroup.addViewAt (layerView, androidLayer->layerView, insertIndex);
        }
        return kResultOk;
    }
    return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidGraphicsLayer::addAnimation (StringID propertyId, const IAnimation* _animation)
{
	const Animation* animation = Animation::cast<Animation> (_animation);
	if(!animation)
		return kResultInvalidArgument;
	
	if(const BasicAnimation* basicAnimation = ccl_cast<BasicAnimation> (animation))
	{
		if(basicAnimation->getValueType () == UIValue::kNil) // scalar value
		{
			float start = basicAnimation->getStartValue ();
			float end = basicAnimation->getEndValue ();
			AnimationHelper a (*animation);

			if(propertyId == kOpacity)
				a.addBasicAnimation (layerView, AndroidView.setAlpha, GraphicsLayerView.addAlphaAnimation, start, end);
			else if(propertyId == kOffsetX)
			{
				start = DpiScale::coordToPixelF (start, contentScaleFactor);
				end = DpiScale::coordToPixelF (end, contentScaleFactor);
	
				a.addBasicAnimation (layerView, AndroidView.setX, GraphicsLayerView.addOffsetXAnimation, start, end);
			}
			else if(propertyId == kOffsetY)
			{
				start = DpiScale::coordToPixelF (start, contentScaleFactor);
				end = DpiScale::coordToPixelF (end, contentScaleFactor);

				a.addBasicAnimation (layerView, AndroidView.setY, GraphicsLayerView.addOffsetYAnimation, start, end);
			}
			return kResultOk;
		}
		else if(propertyId == kOffset)
		{
			Point startPoint, endPoint;
			if(IUIValue* value = IUIValue::toValue (basicAnimation->getStartValue ()))
				value->toPoint (startPoint);
			if(IUIValue* value = IUIValue::toValue (basicAnimation->getEndValue ()))
				value->toPoint (endPoint);
			
			setOffset (startPoint);

			AnimationHelper a (*animation);
			GraphicsLayerView.addOffsetAnimation (layerView, a.duration, a.timing, a.repeatCount, a.autoReverse, a.listener, endPoint.x, endPoint.y);
		}
	}
	else if(const TransformAnimation* transformAnimation = ccl_cast<TransformAnimation> (animation))
	{
		Transform t1, t2;
		transformAnimation->getStartTransform (t1);
		transformAnimation->getEndTransform (t2);

		// scale values
		t1.t0 *= contentScaleFactor;
		t1.t1 *= contentScaleFactor;
		t2.t0 *= contentScaleFactor;
		t2.t1 *= contentScaleFactor;

		// add layer offset
		PixelPointF offset (size.getLeftTop (), contentScaleFactor);
		t1.t0 += offset.x;
		t1.t1 += offset.y;
		t2.t0 += offset.x;
		t2.t1 += offset.y;

		AnimationHelper a (*animation);
		GraphicsLayerView.setTransform (layerView, t1.a0, t1.a1, t1.b0, t1.b1, t1.t0, t1.t1); // transform view to initial state
		GraphicsLayerView.addTransformAnimation (layerView, a.duration, a.timing, a.repeatCount, a.autoReverse, a.listener, t2.a0, t2.a1, t2.b0, t2.b1, t2.t0, t2.t1);
		return kResultOk;
	}
	return kResultInvalidArgument;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidGraphicsLayer::removeAnimation (StringID propertyId)
{
	GraphicsLayerView.removeAnimation (layerView); // transform view to initial state
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidGraphicsLayer::flush ()
{
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidGraphicsLayer::suspendTiling (tbool suspend, const Rect* visibleRect)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AndroidGraphicsLayer::getPresentationProperty (Variant& value, StringID propertyId) const
{
	if(propertyId == kOffsetX)
	{
		value = DpiScale::pixelToCoordF (AndroidView.getX (layerView), contentScaleFactor);
		return true;
	}
	else if(propertyId == kOffsetY)
	{
		value = DpiScale::pixelToCoordF (AndroidView.getY (layerView), contentScaleFactor);
		return true;
	}
	else if(propertyId == kOpacity)
	{
		value = AndroidView.getAlpha (layerView);
		return true;
	}
	return false;
}

//************************************************************************************************
// AndroidRootLayer
//************************************************************************************************

AndroidRootLayer::AndroidRootLayer ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidRootLayer::~AndroidRootLayer ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidRootLayer::construct (IUnknown* content, RectRef bounds, int mode, float contentScaleFactor)
{
	AndroidWindow* window = AndroidWindow::cast (unknown_cast<Window> (content));
	ASSERT (window)
	if(!window)
		return kResultInvalidArgument;

	if(!window->getFrameworkView ())
		return kResultUnexpected;

	ASSERT (!layerView)
	if(!layerView)
	{
		JniAccessor jni;
		layerView.assign (jni, *window->getFrameworkView ());
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidRootLayer::setOffset (PointRef offset)
{
	// root layer is automaticaly sized
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidRootLayer::setOffsetX (float offsetX)
{
	// root layer is automaticaly sized
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidRootLayer::setOffsetY (float offsetY)
{
	// root layer is automaticaly sized
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidRootLayer::setSize (Coord width, Coord height)
{
	// root layer is automaticaly sized
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidRootLayer::setUpdateNeeded ()
{
	AndroidView.invalidate (layerView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidRootLayer::setUpdateNeeded (RectRef rect)
{
	AndroidView.invalidate (layerView);
}
