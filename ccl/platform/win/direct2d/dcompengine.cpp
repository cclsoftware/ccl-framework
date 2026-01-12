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
// Filename    : ccl/platform/win/direct2d/dcompengine.cpp
// Description : DirectComposition Engine
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "dcompengine.h"
#include "dxgiengine.h"
#include "d2dengine.h"
#include "d2ddevice.h"

#include "ccl/base/message.h"
#include "ccl/base/math/mathregion.h"

#include "ccl/platform/win/gui/win32graphics.h"

#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/public/gui/graphics/dpiscale.h"

#include "ccl/public/gui/framework/iwindow.h"
#if DEBUG
#include "ccl/public/gui/framework/iview.h"
#endif

#pragma comment (lib, "Dcomp.lib")

namespace CCL {
namespace Win32 {

//************************************************************************************************
// AnimationWriter
//************************************************************************************************

struct AnimationWriter
{
	IDCompositionAnimation& animation;
	double currentOffset;

	AnimationWriter (IDCompositionAnimation& animation)
	: animation (animation),
	  currentOffset (0.)
	{}

	HRESULT addConstant (double value, double duration)
	{
		HRESULT hr = animation.AddCubic (currentOffset, (float)value, 0.f, 0.f, 0.f);
		currentOffset += duration;
		return hr;
	}

	HRESULT addLinear (double startValue, double endValue, double duration)
	{
		double linearCoefficient = (endValue - startValue) / duration;
		HRESULT hr = animation.AddCubic (currentOffset, (float)startValue, (float)linearCoefficient, 0.f, 0.f);
		currentOffset += duration;
		return hr;
	}

	HRESULT addToggle (double startValue, double endValue, double duration)
	{
		addConstant (startValue, duration / 2.);
		return addConstant (endValue, duration / 2.);
	}

	HRESULT addRepeat (double duration)
	{
		HRESULT hr = animation.AddRepeat (currentOffset, duration);
		currentOffset += duration;
		return hr;
	}

	HRESULT end (double endValue)
	{
		return animation.End (currentOffset, (float)endValue);
	}
};

//************************************************************************************************
// AnimationSegmentSink - can be passed to IUIAnimationVariable2::getCurve()
//************************************************************************************************

class AnimationSegmentSink: public Object,
							public IDCompositionAnimation
{
public:
	// IUnknown
	DELEGATE_COM_IUNKNOWN
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override
	{
		QUERY_COM_INTERFACE (IDCompositionAnimation)
		return Object::queryInterface (iid, ptr);
	}

	// IDCompositionAnimation
	HRESULT STDMETHODCALLTYPE Reset () override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE AddCubic (double beginOffset, float constantCoefficient, float linearCoefficient, float quadraticCoefficient, float cubicCoefficient) override
	{
		CCL_PRINTF ("AddCubic offset = %lf const = %lf linear = %lf quad = %lf cubic = %lf\n", beginOffset, constantCoefficient, linearCoefficient, quadraticCoefficient, cubicCoefficient)
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE SetAbsoluteBeginTime (LARGE_INTEGER beginTime) override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE AddSinusoidal (double beginOffset, float bias, float amplitude, float frequency, float phase) override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE AddRepeat (double beginOffset, double durationToRepeat) override
	{
		CCL_PRINTF ("AddRepeat %lf %lf\n", beginOffset, durationToRepeat)
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE End (double endOffset, float endValue) override
	{
		CCL_PRINTF ("End %lf %lf\n", endOffset, endValue)
		return S_OK;
	}
};

} // namespace Win32
} // namespace CCL

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// DirectCompositionEngine
//************************************************************************************************

DirectCompositionEngine::DirectCompositionEngine ()
: updatesSuspended (false),
  commitPending (false),
  waitForCompletionPending (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DirectCompositionEngine::startup ()
{
	IDXGIDevice* dxgiDevice = DXGIEngine::instance ().getDXGIDevice ();
	ASSERT (dxgiDevice != nullptr)

	// *** Create DirectComposition device ***
	HRESULT hr = ::DCompositionCreateDevice (dxgiDevice, __uuidof(IDCompositionDevice), directCompositionDevice);
	if(FAILED (hr))
		return false;

	// *** Create Animation Manager ***
	animationManager = com_new<IUIAnimationManager2> (CLSID_UIAnimationManager2);
	transitionLibrary = com_new<IUIAnimationTransitionLibrary2> (CLSID_UIAnimationTransitionLibrary2);
	if(!animationManager || !transitionLibrary)
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DirectCompositionEngine::shutdown ()
{
	ASSERT (!updatesSuspended && !commitPending && !waitForCompletionPending)

	animationManager.release ();
	transitionLibrary.release ();

	// commit any pending commands
	// such as removal of layers
	flush ();

	directCompositionDevice.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DirectCompositionEngine::checkDeviceState ()
{
	BOOL valid = TRUE;
	HRESULT hr = directCompositionDevice->CheckDeviceState (&valid);
	ASSERT (SUCCEEDED (hr))
	return valid != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DirectCompositionEngine::handleDeviceLost ()
{
	removeAnimations ();

	ForEach (rootLayers, DCRootLayer, rootLayer)
		rootLayer->handleDeviceLost (true);
	EndFor

	shutdown ();
	if(!startup ())
		return;

	ForEach (rootLayers, DCRootLayer, rootLayer)
		rootLayer->handleDeviceLost (false);
	EndFor

	onTimer (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* DirectCompositionEngine::createLayer (UIDRef classID)
{
	if(classID == ClassID::RootLayer)
		return NEW DCRootLayer;
	if(classID == ClassID::GraphicsLayer)
		return NEW DCGraphicsLayer;
	if(classID == ClassID::TiledLayer)
		return NEW DCTiledGraphicsLayer;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DirectCompositionEngine::flush (bool force)
{
	if(updatesSuspended)
	{
		commitPending = true;
		if(force)
			waitForCompletionPending = true;
	}
	else
	{
		CCL_PRINTF ("DirectCompositionEngine::flush (%d): Commit %s\n", force, (force || waitForCompletionPending) ? "- WaitForCommitCompletion" : "")
		directCompositionDevice->Commit ();

		if(force || waitForCompletionPending)
			directCompositionDevice->WaitForCommitCompletion ();

		commitPending = false;
		waitForCompletionPending = false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DirectCompositionEngine::suspendUpdates (bool suspend)
{
	bool oldState = updatesSuspended;
	if(updatesSuspended != suspend)
	{
		updatesSuspended = suspend;

		if(!updatesSuspended)
		{
			#if DEBUG_LOG
			if(commitPending || waitForCompletionPending)
				CCL_PRINTF ("DirectCompositionEngine::suspendUpdates (false): %s %s\n", commitPending ? "Commit" : "", waitForCompletionPending ? "- WaitForCommitCompletion" : "")
			#endif

			if(commitPending)
			{
				directCompositionDevice->Commit ();
				commitPending = false;
				GraphicsLayerEngine::flushNeeded = false;
			}
			
			if(waitForCompletionPending)
			{
				directCompositionDevice->WaitForCommitCompletion ();
				waitForCompletionPending = false;
			}
		}
	}
	return oldState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double DirectCompositionEngine::getNextEstimatedFrameTime () const
{
	DCOMPOSITION_FRAME_STATISTICS frameStatistics = {0};
	HRESULT hr = directCompositionDevice->GetFrameStatistics (&frameStatistics);
	ASSERT (SUCCEEDED (hr))
	return (double)frameStatistics.nextEstimatedFrameTime.QuadPart / (double)frameStatistics.timeFrequency.QuadPart;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDCompositionAnimation* DirectCompositionEngine::createAnimation (const BasicAnimation& animation, float scaleFactor)
{
	ASSERT (animation.getValueType () == IUIValue::kNil) // must be a scalar value!

	HRESULT hr = S_OK;
	IDCompositionAnimation* directAnimation = nullptr; // TEST: NEW AnimationSegmentSink;
	hr = directCompositionDevice->CreateAnimation (&directAnimation);
	ASSERT (SUCCEEDED (hr))

	AnimationTimingType timingType = animation.getTimingType ();
	double start = animation.getStartValue ();
	double end = animation.getEndValue ();
	double duration = animation.getDuration ();
	int repeatCount = animation.getRepeatCount ();

	if(scaleFactor != 1.f)
	{
		start *= scaleFactor;
		end *= scaleFactor;
	}

	if(timingType == kTimingLinear || timingType == kTimingToggle)
	{
		AnimationWriter writer (*directAnimation);
		if(timingType == kTimingLinear)
		{
			hr = writer.addLinear (start, end, duration);
			if(animation.isAutoReverse ())
				hr = writer.addLinear (end, start, duration);
		}
		else
		{
			hr = writer.addToggle (start, end, duration);
			if(animation.isAutoReverse ())
				hr = writer.addToggle (end, start, duration);
		}

		double finalValue = animation.isAutoReverse () ? start : end;
		if(repeatCount > 1)
		{
			double toRepeat = writer.currentOffset;
			hr = writer.addRepeat (toRepeat);
			if(repeatCount != Animation::kRepeatForever)
			{
				writer.currentOffset += (double)(repeatCount-1) * toRepeat;
				hr = writer.end (finalValue);
			}
		}
		else
			hr = writer.end (finalValue);
	}
	else
	{
		AnimationControlPoints values;
		if(timingType == kTimingCubicBezier)
			values = animation.getControlPoints ();
		else if(const ITimingFunction* function = Animation::getStandardTimingFunction (timingType))
			function->getControlPoints (values);
		#if DEBUG
		else
			CCL_DEBUGGER ("Unknown timing type!\n")
		#endif

		ComPtr<IUIAnimationVariable2> variable;
		hr = animationManager->CreateAnimationVariable (start, variable);

		ComPtr<IUIAnimationTransition2> transition;
		hr = transitionLibrary->CreateCubicBezierLinearTransition (duration, end, values.c1x, values.c1y, values.c2x, values.c2y, transition);

		ComPtr<IUIAnimationStoryboard2> storyboard;
		hr = animationManager->CreateStoryboard (storyboard);
		hr = storyboard->AddTransition (variable, transition);

		UI_ANIMATION_KEYFRAME endKeyFrame = NULL;
		if(animation.isAutoReverse ())
		{
			ComPtr<IUIAnimationTransition2> reverseTransition;
			hr = transitionLibrary->CreateCubicBezierLinearTransition (duration, start, values.c1x, values.c1y, values.c2x, values.c2y, reverseTransition);
			hr = storyboard->AddTransition (variable, reverseTransition);
			hr = storyboard->AddKeyframeAfterTransition (reverseTransition, &endKeyFrame);
		}
		else
			hr = storyboard->AddKeyframeAfterTransition (transition, &endKeyFrame);

		if(repeatCount > 1)
		{
			double count = repeatCount == Animation::kRepeatForever ? UI_ANIMATION_REPEAT_INDEFINITELY : repeatCount;
			hr = storyboard->RepeatBetweenKeyframes (UI_ANIMATION_KEYFRAME_STORYBOARD_START, endKeyFrame, count, UI_ANIMATION_REPEAT_MODE_NORMAL, nullptr, 0, FALSE);
		}

		// Synchronize WAM with DirectComposition time
		UI_ANIMATION_SECONDS nextEstimatedFrameTime = getNextEstimatedFrameTime ();
		hr = animationManager->Update (nextEstimatedFrameTime);
		hr = storyboard->Schedule (nextEstimatedFrameTime);

		hr = variable->GetCurve (directAnimation);
		ASSERT (SUCCEEDED (hr))
	}

	return directAnimation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDCompositionAnimation* DirectCompositionEngine::createAnimation (const AnimationDescription& description, double startValue, double endValue, float scaleFactor)
{
	BasicAnimation basicAnimation;
	basicAnimation.setDescription (description);
	basicAnimation.setStartValue (startValue);
	basicAnimation.setEndValue (endValue);
	return createAnimation (basicAnimation, scaleFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDCompositionTransform* DirectCompositionEngine::createTransform (const TransformAnimation& transformAnimation, PointRef center, float scaleFactor)
{
	AnimationDescription description;
	transformAnimation.getDescription (description);

	auto needsScaling = [] (TransformAnimation::MatrixOpType type)
	{
		return type == TransformAnimation::kTranslateX || type == TransformAnimation::kTranslateY;
	};
	
	HRESULT hr = S_OK;
	Core::FixedSizeVector<IDCompositionTransform*, TransformAnimation::kMaxMatrixOpCount> transforms;
	VectorForEach (transformAnimation.getOperations (), TransformAnimation::MatrixOp, op)
		// create animation for current matrix operation
		float opScaleFactor = needsScaling (op.type) ? scaleFactor : 1.f;
		ComPtr<IDCompositionAnimation> directAnimation = createAnimation (description, op.startValue, op.endValue, opScaleFactor);
		ASSERT (directAnimation != 0)

		switch(op.type)
		{
		case TransformAnimation::kTranslateX :
		case TransformAnimation::kTranslateY :
			{
				IDCompositionTranslateTransform* translateTransform = nullptr;
				hr = directCompositionDevice->CreateTranslateTransform (&translateTransform);
				if(op.type == TransformAnimation::kTranslateX)
					translateTransform->SetOffsetX (directAnimation);
				else
					translateTransform->SetOffsetY (directAnimation);
				transforms.add (translateTransform);
			}
			break;

		case TransformAnimation::kScaleX :
		case TransformAnimation::kScaleY :
			{
				IDCompositionScaleTransform* scaleTransform = nullptr;
				hr = directCompositionDevice->CreateScaleTransform (&scaleTransform);
				hr = scaleTransform->SetCenterX (DpiScale::coordToPixelF (center.x, scaleFactor));
				hr = scaleTransform->SetCenterY (DpiScale::coordToPixelF (center.y, scaleFactor));
				if(op.type == TransformAnimation::kScaleX)
					hr = scaleTransform->SetScaleX (directAnimation);
				else
					hr = scaleTransform->SetScaleY (directAnimation);
				transforms.add (scaleTransform);
			}
			break;

		case TransformAnimation::kRotate :
			{
				IDCompositionRotateTransform* rotateTransform = nullptr;
				hr = directCompositionDevice->CreateRotateTransform (&rotateTransform);
				hr = rotateTransform->SetCenterX (DpiScale::coordToPixelF (center.x, scaleFactor));
				hr = rotateTransform->SetCenterY (DpiScale::coordToPixelF (center.y, scaleFactor));
				hr = rotateTransform->SetAngle (directAnimation);
				transforms.add (rotateTransform);
			}
			break;

		case TransformAnimation::kSkewX :
		case TransformAnimation::kSkewY :
			{
				IDCompositionSkewTransform* skewTransform = nullptr;
				hr = directCompositionDevice->CreateSkewTransform (&skewTransform);
				skewTransform->SetCenterX (DpiScale::coordToPixelF (center.x, scaleFactor));
				skewTransform->SetCenterY (DpiScale::coordToPixelF (center.y, scaleFactor));
				if(op.type == TransformAnimation::kSkewX)
					skewTransform->SetAngleX (directAnimation);
				else
					skewTransform->SetAngleY (directAnimation);
				transforms.add (skewTransform);
			}
			break;

		default :
			CCL_DEBUGGER ("Unknown matrix operation!\n")
			break;
		}
	EndFor

	if(transforms.isEmpty ())
		return nullptr;
	if(transforms.count () == 1)
		return transforms[0];

	// create transform group
	IDCompositionTransform* transformGroup = nullptr;
	hr = directCompositionDevice->CreateTransformGroup (transforms, transforms.count (), &transformGroup);
	ASSERT (SUCCEEDED (hr))

	VectorForEach (transforms, IDCompositionTransform*, t)
		t->Release (); // now owned by group
	EndFor

	return transformGroup;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDCompositionClip* DirectCompositionEngine::createClip (const BasicAnimation& animation, float scaleFactor)
{
	AnimationDescription description;
	animation.getDescription (description);

	Rect startRect, endRect;
	if(IUIValue* start = IUIValue::toValue (animation.getStartValue ()))
		start->toRect (startRect);
	if(IUIValue* end = IUIValue::toValue (animation.getEndValue ()))
		end->toRect (endRect);

	DpiScale::toPixelRect (startRect, scaleFactor);
	DpiScale::toPixelRect (endRect, scaleFactor);

	IDCompositionRectangleClip* clip = nullptr;
	HRESULT hr = directCompositionDevice->CreateRectangleClip (&clip);

	// left
	if(startRect.left == endRect.left)	
		hr = clip->SetLeft ((float)startRect.left);
	else
		hr = clip->SetLeft (ComPtr<IDCompositionAnimation> (createAnimation (description, startRect.left, endRect.left, 1.f)));

	// top
	if(startRect.top == endRect.top)
		hr = clip->SetTop ((float)startRect.top);
	else
		hr = clip->SetTop (ComPtr<IDCompositionAnimation> (createAnimation (description, startRect.top, endRect.top, 1.f)));

	// right
	if(startRect.right == endRect.right)
		hr = clip->SetRight ((float)startRect.right);
	else
		hr = clip->SetRight (ComPtr<IDCompositionAnimation> (createAnimation (description, startRect.right, endRect.right, 1.f)));

	// bottom
	if(startRect.bottom == endRect.bottom)
		hr = clip->SetBottom ((float)startRect.bottom);
	else
		hr = clip->SetBottom (ComPtr<IDCompositionAnimation> (createAnimation (description, startRect.bottom, endRect.bottom, 1.f)));

	return clip;
}

//************************************************************************************************
// DCRootLayer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DCRootLayer, DCGraphicsLayer)

//////////////////////////////////////////////////////////////////////////////////////////////////

DCRootLayer::DCRootLayer ()
: windowHandle (NULL)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DCRootLayer::~DCRootLayer ()
{
	CCL_PRINTLN ("DCRootLayer dtor")

	if(!target.isValid ())
		return;

	engine.removeRootLayer (this);

	target->SetRoot (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect DCRootLayer::getBounds () const
{
	if(!windowHandle)
		return Rect ();

	RECT clientRect = {0};
	::GetClientRect (windowHandle, &clientRect);
	
	Rect r;
	GdiInterop::fromSystemRect (r, clientRect);
	DpiScale::toCoordRect (r, contentScaleFactor);
	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DCRootLayer::setContentScaleFactor (float factor)
{
	// nothing here
	contentScaleFactor = factor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DCRootLayer::handleDeviceLost (bool begin)
{
	if(begin)
	{
		handleDeviceLostForSublayers (true);

		if(target)
			target->SetRoot (nullptr);
		visual.release ();
		target.release ();
	}
	else
	{
		ASSERT (windowHandle != NULL)
		HRESULT hr = getDevice ()->CreateTargetForHwnd (windowHandle, FALSE, target);
		ASSERT (SUCCEEDED (hr))
		if(!target)
			return;

		hr = getDevice ()->CreateVisual (visual);
		ASSERT (SUCCEEDED (hr))

		hr = target->SetRoot (visual);
		ASSERT (SUCCEEDED (hr))
		
		handleDeviceLostForSublayers (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DCRootLayer::construct (IUnknown* content, RectRef, int, float contentScaleFactor)
{
	ASSERT (!target.isValid ()) // must be called only once
	if(target.isValid ())
		return kResultUnexpected;

	UnknownPtr<IWindow> window (content);
	ASSERT (window.isValid ())
	if(!window)
		return kResultInvalidArgument;

	HWND hwnd = (HWND)window->getSystemWindow ();
	HRESULT hr = getDevice ()->CreateTargetForHwnd (hwnd, FALSE, target);
	ASSERT (SUCCEEDED (hr))
	if(FAILED (hr))
		return kResultFailed;

	windowHandle = hwnd;
	hr = target->SetRoot (visual);
	ASSERT (SUCCEEDED (hr))

	engine.addRootLayer (this);
	
	this->contentScaleFactor = contentScaleFactor;

	CCL_PRINTF ("Construct Root Layer for HWND %p\n", hwnd)
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DCRootLayer::suspendUpdates (tbool suspend)
{
	return static_cast<DirectCompositionEngine&> (engine).suspendUpdates (suspend != 0);
}

//************************************************************************************************
// DCTiledGraphicsLayer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DCTiledGraphicsLayer, DCGraphicsLayer)

//////////////////////////////////////////////////////////////////////////////////////////////////

IDCompositionSurface* DCTiledGraphicsLayer::createSurface () const
{
	IDCompositionVirtualSurface* surface = nullptr;
	DXGI_ALPHA_MODE alphaMode = isIgnoreAlpha () ? DXGI_ALPHA_MODE_IGNORE : DXGI_ALPHA_MODE_PREMULTIPLIED;
	PixelPoint sizeInPixel (size, contentScaleFactor);
	HRESULT hr = getDevice ()->CreateVirtualSurface (sizeInPixel.x, sizeInPixel.y, DXGI_FORMAT_B8G8R8A8_UNORM, alphaMode, &surface);
	ASSERT (SUCCEEDED (hr))
	return surface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DCTiledGraphicsLayer::setSize (Coord width, Coord height)
{
	ComPtr<IDCompositionVirtualSurface> virtualSurface;
	surface.as (virtualSurface);
	if(virtualSurface && width > 0  && height > 0)
	{
		Point newSize (width, height);
		if(size != newSize)
		{
			CCL_PRINTF ("Tiled layer %s resize to %d x %d\n", debugName.str (), width, height)
			size = newSize;
			
			PixelPoint sizeInPixel (Point (width, height), contentScaleFactor);
			HRESULT hr = virtualSurface->Resize (sizeInPixel.x, sizeInPixel.y);
			ASSERT (SUCCEEDED (hr))
			updateClip ();
		}
	}
	else
		SuperClass::setSize (width, height);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DCTiledGraphicsLayer::setUpdateNeeded (RectRef _rect)
{
	// Updates need to be trimmed to area displayed by the application,
	// because IDCompositionSurface::BeginDraw() allocates the pixels!

	Rect rect (_rect);
	rect.bound (visibleRect);
	if(!rect.isEmpty ())
		dirtyRect.join (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DCTiledGraphicsLayer::snapToTiles (Rect& r) const
{
	const Coord kTileSize = 512;

	auto snap = [kTileSize] (Coord& c, int inc)
	{
		c = (c / kTileSize + inc) * kTileSize;
	};

	snap (r.left, 0);
	snap (r.top, 0);
	snap (r.right, 1);
	snap (r.bottom, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DCTiledGraphicsLayer::updateAll (bool& updateDone)
{
	// check if visible area has changed
	Rect newRect;
	if(getVisibleClient (newRect))
	{
		snapToTiles (newRect);
		newRect.bound (Rect (size));
	}

	if(newRect != visibleRect)
	{
		updateDone = true; // flush is needed

		#if DEBUG_LOG
		dumpRect (newRect, "Tiled Layer visibleRect changed to:");
		#endif

		auto toRegionSegment = [] (RectRef r)
		{
			return Math::Region::Segment (r.left, r.top, r.right, r.bottom);
		};

		auto toRect = [] (const Math::Region::Segment& s)
		{
			return Rect ((Coord)s.x1, (Coord)s.y1, (Coord)s.x2, (Coord)s.y2);
		};

		// TODO: use GdiClipRegion class???
		Math::Region region;
		if(!newRect.isEmpty ())
			region.include (toRegionSegment (newRect));
		if(!visibleRect.isEmpty ())
			region.exclude (toRegionSegment (visibleRect));
		visibleRect = newRect;

		ComPtr<IDCompositionVirtualSurface> virtualSurface;
		surface.as (virtualSurface);
		if(virtualSurface)
		{
			if(visibleRect.isEmpty ())
			{
				// no pixels are kept
				HRESULT hr = virtualSurface->Trim (nullptr, 0);
				ASSERT (SUCCEEDED (hr))
			}
			else
			{
				PixelRect visibleRectInPixel (visibleRect, contentScaleFactor);
				RECT rectangles[1] = {};
				GdiInterop::toSystemRect (rectangles[0], visibleRectInPixel);
				HRESULT hr = virtualSurface->Trim (rectangles, 1);
				ASSERT (SUCCEEDED (hr))
			}
		}

		Math::RegionIterator iter (region);
		Math::Region::Segment currentSegment;
		while(iter.next (currentSegment))
		{
			ScopedVar<Rect> scope (dirtyRect, toRect (currentSegment));
			//dirtyRect.bound (Rect (size));
			#if DEBUG_LOG
			dumpRect (dirtyRect, "Updating Tiled Layer rect:");
			#endif
			SuperClass::updateContent ();
		}
	}

	SuperClass::updateAll (updateDone);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DCTiledGraphicsLayer::updateContent ()
{
	// see also setUpdateNeeded()
	dirtyRect.bound (visibleRect);
	if(dirtyRect.isEmpty ())
		return;

	SuperClass::updateContent ();
}

//************************************************************************************************
// DCGraphicsLayer
//************************************************************************************************

IDCompositionDevice* DCGraphicsLayer::getDevice ()
{
	return DirectCompositionEngine::instance ().getDevice ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (DCGraphicsLayer, GraphicsLayer)

//////////////////////////////////////////////////////////////////////////////////////////////////

DCGraphicsLayer::DCGraphicsLayer ()
: GraphicsLayer (DirectCompositionEngine::instance ()),
  mode (0)
{
	HRESULT hr = getDevice ()->CreateVisual (visual);
	ASSERT (SUCCEEDED (hr))
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DCGraphicsLayer::~DCGraphicsLayer ()
{
	CCL_PRINTLN ("DCGraphicsLayer dtor")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DCGraphicsLayer::handleDeviceLostForSublayers (bool begin)
{
	ListForEachObject (sublayers, DCGraphicsLayer, layer)
		if(begin)
			attachSublayer (layer, false);

		layer->handleDeviceLost (begin);

		if(!begin)
			attachSublayer (layer, true);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DCGraphicsLayer::handleDeviceLost (bool begin)
{
	if(begin)
	{
		handleDeviceLostForSublayers (true);
		
		HRESULT hr = visual->SetEffect (nullptr);
		ASSERT (SUCCEEDED (hr))
		effects.release ();

		hr = visual->SetContent (nullptr);
		ASSERT (SUCCEEDED (hr))
		surface.release ();

		visual.release ();
	}
	else
	{
		HRESULT hr = getDevice ()->CreateVisual (visual);
		ASSERT (SUCCEEDED (hr))

		reconstruct ();
		setUpdateNeeded ();

		handleDeviceLostForSublayers (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DCGraphicsLayer::reconstruct ()
{
	// similar to construct():
	Point oldSize (size);
	size (0, 0);
	setSize (oldSize.x, oldSize.y);
	initContent ();
	updateClip ();

	// restore properties
	applyProperty (kAnimateOffset);
	applyProperty (kAnimateOpacity);
	applyProperty (kAnimateTransform);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DCGraphicsLayer::setContentScaleFactor (float factor)
{
	if(factor != contentScaleFactor)
	{
		CCL_PRINTF ("Set content scale factor from %.2f to %.2f\n", contentScaleFactor, factor)
		contentScaleFactor = factor;
		reconstruct ();
		setUpdateNeeded ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DCGraphicsLayer::construct (IUnknown* _content, RectRef bounds, int _mode, float contentScaleFactor)
{
	ASSERT (!content.isValid ()) // must be called only once
	if(content.isValid () == true)
		return kResultUnexpected;

	mode = _mode;
	content = _content;
	this->contentScaleFactor = contentScaleFactor;
	setSize (bounds.getWidth (), bounds.getHeight ());
	setOffset (bounds.getLeftTop ());
	initContent ();
	updateClip ();

	CCL_PRINTF ("Construct Graphics Layer for %s %d x %d\n", debugName.str (), bounds.getWidth (), bounds.getHeight ())

	setUpdateNeeded ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DCGraphicsLayer::setContent (IUnknown* _content)
{
	content = _content;
	initContent ();
	makeSurface ();
	setUpdateNeeded ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DCGraphicsLayer::setSize (Coord width, Coord height)
{
	Point newSize (width, height);
	if(size != newSize)
	{
		size = newSize;
		makeSurface ();
		updateClip ();
		setFlushNeeded ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DCGraphicsLayer::setMode (int _mode)
{
	if(mode != _mode)
	{
		mode = _mode;
		makeSurface ();
		updateClip ();
		setFlushNeeded ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DCGraphicsLayer::updateClip ()
{
	if(mode & kClipToBounds)
	{
		Rect clipRect (0, 0, size.x, size.y);
		PixelRect clipRectInPixel (clipRect, contentScaleFactor);
		HRESULT hr = visual->SetClip (D2DInterop::toRectF (clipRectInPixel));
		ASSERT (SUCCEEDED (hr))
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDCompositionSurface* DCGraphicsLayer::createSurface () const
{
	IDCompositionSurface* surface = nullptr;
	DXGI_ALPHA_MODE alphaMode = isIgnoreAlpha () ? DXGI_ALPHA_MODE_IGNORE : DXGI_ALPHA_MODE_PREMULTIPLIED;
	PixelPoint sizeInPixel (size, contentScaleFactor);
	CCL_PRINTF ("Create surface with pixel size %d x %d\n", sizeInPixel.x, sizeInPixel.y)
	HRESULT hr = getDevice ()->CreateSurface (sizeInPixel.x, sizeInPixel.y, DXGI_FORMAT_B8G8R8A8_UNORM, alphaMode, &surface);
	ASSERT (SUCCEEDED (hr))
	return surface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDCompositionEffectGroup* DCGraphicsLayer::getEffects ()
{
	if(!effects)
	{
		getDevice ()->CreateEffectGroup (effects);
		visual->SetEffect (effects);
	}
	return effects;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DCGraphicsLayer::makeSurface ()
{
	if(surface)
	{
		visual->SetContent (nullptr);
		surface.release ();
	}

	if(size.x <= 0 || size.y <= 0 || !content.isValid ())
		return;

	surface = createSurface ();
	HRESULT hr = visual->SetContent (surface);
	ASSERT (SUCCEEDED (hr))
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DCGraphicsLayer::initContent ()
{
	if(Image* image = unknown_cast<Image> (content))
	{
		if(Bitmap* bitmap = Bitmap::getOriginalBitmap (contentRect, image)) // <-- init content rect
			content = static_cast<IImage*> (bitmap);
		else
		{
			CCL_DEBUGGER ("Layer content image must be of type bitmap!\n")
			content.release ();
		}
	}

	#if DEBUG
	debugName = content ? UnknownPtr<IObject> (content)->getTypeInfo ().getClassName () : "";
	if(UnknownPtr<IView> view = content)
	{
		Variant v;
		view->getViewAttribute (v, IView::kName);
		MutableCString viewName (v.asString ());
		if(!viewName.isEmpty ())
			debugName += " ",
			debugName += viewName;
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DCGraphicsLayer::updateContent ()
{
	//ASSERT (surface && content)
	if(!surface || !content)
		return;

	RECT updateRect = {0};
	PixelRect dirtyRectInPixel (dirtyRect, contentScaleFactor);
	GdiInterop::toSystemRect (updateRect, dirtyRectInPixel);

	//ASSERT (dirtyRect.left >= 0 && dirtyRect.right <= size.x && dirtyRect.top >= 0 && dirtyRect.bottom <= size.y, "Unexpected dirty rect!\n")

	POINT updateOffset = {0};
	ComPtr<IDXGISurface1> dxgiSurface;
	HRESULT hr = surface->BeginDraw (&updateRect, __uuidof(IDXGISurface1), dxgiSurface, &updateOffset);
	//ASSERT (SUCCEEDED (hr))
	if(FAILED (hr))
	{
		Rect fullRect (0, 0, size.x, size.y);
		PixelRect fullRectInPixel (fullRect, contentScaleFactor);
		GdiInterop::toSystemRect (updateRect, fullRectInPixel);
		hr = surface->BeginDraw (&updateRect, __uuidof(IDXGISurface1), dxgiSurface, &updateOffset);
		ASSERT (SUCCEEDED (hr))
		if(FAILED (hr))
			return;

		dirtyRect = fullRect;
	}

	Rect updateRectInPixel (0, 0, updateRect.right - updateRect.left, updateRect.bottom - updateRect.top);
	updateRectInPixel.offset (updateOffset.x, updateOffset.y); // this is the position we should draw at

	#if (0 && DEBUG_LOG)
	CCL_PRINTF ("Graphics layer %s redraw (%d,%d,%d,%d) update offset (pixel) = %d,%d\n", debugName.str (), 
		dirtyRect.left, dirtyRect.top, dirtyRect.right, dirtyRect.bottom,
		updateRectInPixel.left, updateRectInPixel.top)
	#endif

	drawContentDirect2D (dxgiSurface, updateRectInPixel);

	dxgiSurface.release ();
	hr = surface->EndDraw ();
	ASSERT (SUCCEEDED (hr))

	// reset dirty region
	dirtyRect.setReallyEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DCGraphicsLayer::drawContentDirect2D (IDXGISurface1* dxgiSurface, RectRef updateRectInPixel)
{
	D2D1_ALPHA_MODE alphaMode = isIgnoreAlpha () ? D2D1_ALPHA_MODE_IGNORE : D2D1_ALPHA_MODE_PREMULTIPLIED;
	D2D1_PIXEL_FORMAT pixelFormat = D2D1::PixelFormat (DXGI_FORMAT_B8G8R8A8_UNORM, alphaMode);

	FLOAT dpi = DpiScale::getDpi (contentScaleFactor);
	const D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1 (
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		pixelFormat,
		dpi, dpi);

	ComPtr<ID2D1Bitmap1> surfaceBitmap;
	HRESULT hr = DXGIEngine::instance ().getDirect2dDeviceContext ()->CreateBitmapFromDxgiSurface (dxgiSurface, bitmapProperties, surfaceBitmap);
	ASSERT (SUCCEEDED (hr))
	if(FAILED (hr))
		return;

	Point updateOffsetInPixel (updateRectInPixel.getLeftTop ());
	Point originPoint (updateOffsetInPixel);
	DpiScale::toCoordPoint (originPoint, contentScaleFactor);

	// ATTENTION: Since we don't support floating point coordinates, we can only draw directly 
	// if pixel-point-pixel conversion is non-ambiguous, otherwise we get artifacts on screen!
	bool copyNeeded = PixelPoint (originPoint, contentScaleFactor) != updateOffsetInPixel;

	if(!copyNeeded && !DpiScale::isIntAligned (contentScaleFactor))
	{
		// for fractional scaling, also check if the updateRect's "end point" is non-ambiguous (fixes shrinking or even disappearing sprites)
		Point endInPixel (updateRectInPixel.getRightBottom ());
		Point endPoint (endInPixel);
		DpiScale::toCoordPoint (endPoint, contentScaleFactor);

		copyNeeded = PixelPoint (endPoint, contentScaleFactor) != endInPixel;
	}

	ID2D1Bitmap1* scratchBitmap = nullptr;
	if(copyNeeded == true)
	{
		D2D_SIZE_U size = D2D1::SizeU (updateRectInPixel.getWidth (), updateRectInPixel.getHeight ());
		scratchBitmap = DXGIEngine::instance ().getScratchBitmap (size, pixelFormat);
		ASSERT (scratchBitmap != nullptr)
		if(scratchBitmap == nullptr)
		{
			DXGIEngine::instance ().reportError ("DirectComposition scratch bitmap failed", DXGIEngine::instance ().getLastError (), true);
			return;
		}

		originPoint = Point ();
	}

	class D2DLayerRenderTarget: public Object,
								public D2DRenderTarget
	{
	public:
		D2DLayerRenderTarget (ID2D1Bitmap1* surfaceBitmap, bool alphaChannelUsed, float contentScaleFactor)
		: alphaChannelUsed (alphaChannelUsed),
		  contentScaleFactor (contentScaleFactor)
		{
			outputImage.share (surfaceBitmap);
		}

		// D2DRenderTarget
		bool isAlphChannelUsed () const override { return alphaChannelUsed; }
		float getContentScaleFactor () const override { return contentScaleFactor; }

	protected:
		bool alphaChannelUsed;
		float contentScaleFactor;
	};

	{
		D2DLayerRenderTarget renderTarget (copyNeeded ? scratchBitmap : surfaceBitmap, !isIgnoreAlpha (), contentScaleFactor);
		D2DScopedGraphicsDevice d2dDevice (renderTarget, /*static_cast<Unknown*> (&renderTarget)*/nullptr);
		drawContent (d2dDevice, originPoint);
	}

	// copy back from scratch bitmap
	if(copyNeeded == true)
	{
		D2D1_POINT_2U dstPoint = D2D1::Point2U (updateOffsetInPixel.x, updateOffsetInPixel.y);
		D2D1_RECT_U srcRect = D2D1::RectU (0, 0, updateRectInPixel.getWidth (), updateRectInPixel.getHeight ());
		hr = surfaceBitmap->CopyFromBitmap (&dstPoint, scratchBitmap, &srcRect);
		ASSERT (SUCCEEDED (hr))
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DCGraphicsLayer::drawContent (NativeGraphicsDevice& nativeDevice, PointRef originPoint)
{
	Point delta;
	delta.x = originPoint.x - dirtyRect.left;
	delta.y = originPoint.y - dirtyRect.top;
	Rect clipRect (dirtyRect);
	clipRect.offset (delta);

	GraphicsDevice graphics;
	graphics.setNativeDevice (&nativeDevice);
	graphics.addClip (clipRect);

	// clear background
	if(isIgnoreAlpha ())
		graphics.fillRect (clipRect, SolidBrush (Colors::kBlack));
	else
		graphics.clearRect (clipRect);

	if(Bitmap* bitmap = unknown_cast<Bitmap> (content))
	{
		Rect dst (0, 0, contentRect.getWidth (), contentRect.getHeight ()); // TODO: stretch option here?
		dst.moveTo (originPoint);
		graphics.drawImage (bitmap, contentRect, dst);
	}
	else if(UnknownPtr<IGraphicsLayerContent> layerContent = content)
	{
		layerContent->drawLayer (graphics, UpdateRgn (dirtyRect), delta);
	}
	else
	{
		CCL_DEBUGGER ("Unknown layer content!\n")
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DCGraphicsLayer::attachSublayer (IGraphicsLayer* layer, bool state, IGraphicsLayer* _sibling, bool below)
{
	IDCompositionVisual* other = unknown_cast<DCGraphicsLayer> (layer)->visual;
	IDCompositionVisual* sibling = _sibling ? unknown_cast<DCGraphicsLayer> (_sibling)->visual : nullptr;
	
	// PLEASE NOTE: insertAbove=FALSE with sibling=nullptr => render visual below no sibling, meaning above all of its siblings.
	BOOL insertAbove = sibling == nullptr ? (below ? TRUE : FALSE) : (below ? FALSE : TRUE);

	HRESULT hr = state ? visual->AddVisual (other, insertAbove, sibling) : visual->RemoveVisual (other); 
	ASSERT (SUCCEEDED (hr))		
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DCGraphicsLayer::applyProperty (int id)
{
	auto modelToVisual = [&] (float modelValue)
	{
		return modelValue * contentScaleFactor;
		//return (float)DpiScale::coordToPixel ((int)modelValue, contentScaleFactor);
	};

	HRESULT hr = E_FAIL;
	switch(id)
	{
	case kAnimateOffsetX :
		hr = visual->SetOffsetX (modelToVisual (modelState.offsetX));
		break;

	case kAnimateOffsetY :
		hr = visual->SetOffsetY (modelToVisual (modelState.offsetY));
		break;

	case kAnimateOffset :
		hr = visual->SetOffsetX (modelToVisual (modelState.offsetX));
		hr = visual->SetOffsetY (modelToVisual (modelState.offsetY));
		break;

	case kAnimateOpacity :
		hr = getEffects ()->SetOpacity (modelState.opacity);
		break;

	case kAnimateTransform :
		// TODO: respect scale factor for x/y translation (but don't scale matrix)!
		hr = visual->SetTransform (D2DInterop::toMatrix (modelState.transform));
		break;
	}

	ASSERT (SUCCEEDED (hr))
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DCGraphicsLayer::applyAnimation (int id, const Animation* _animation)
{
	DirectCompositionEngine& engine = DirectCompositionEngine::instance ();
	bool done = false;

	if(const BasicAnimation* animation = ccl_cast<BasicAnimation> (_animation))
	{
		if(animation->getValueType () == IUIValue::kNil) // scalar value
		{
			auto needsScaling = [] (int id)
			{
				return id == kAnimateOffsetX || id == kAnimateOffsetY;
			};

			done = true;
			float scaleFactor = needsScaling (id) ? contentScaleFactor : 1.f;
			ComPtr<IDCompositionAnimation> directAnimation = engine.createAnimation (*animation, scaleFactor);

			if(id == kAnimateOpacity)
				getEffects ()->SetOpacity (directAnimation);
			else if(id == kAnimateOffsetX)
				visual->SetOffsetX (directAnimation);				
			else if(id == kAnimateOffsetY)
				visual->SetOffsetY (directAnimation);
			else
				done = false;
		}
		else if(id == kAnimateOffset)
		{
			Point startPoint, endPoint;
			if(IUIValue* start = IUIValue::toValue (animation->getStartValue ()))
				start->toPoint (startPoint);
			if(IUIValue* end = IUIValue::toValue (animation->getEndValue ()))
				end->toPoint (endPoint);

			AnimationDescription description;
			animation->getDescription (description);
			ComPtr<IDCompositionAnimation> xAnimation = engine.createAnimation (description, startPoint.x, endPoint.x, contentScaleFactor);
			ComPtr<IDCompositionAnimation> yAnimation = engine.createAnimation (description, startPoint.y, endPoint.y, contentScaleFactor);

			visual->SetOffsetX (xAnimation);
			visual->SetOffsetY (yAnimation);
			done = true;
		}
	}
	else if(const TransformAnimation* animation = ccl_cast<TransformAnimation> (_animation))
	{
		if(id == kAnimateTransform)
		{
			Point center;
			#if 0
			center.x = size.x / 2;
			center.y = size.y / 2;
			#endif
			ComPtr<IDCompositionTransform> directTransform = engine.createTransform (*animation, center, contentScaleFactor);
			visual->SetTransform (directTransform);
			done = true;
		}
	}

	return done;
}
