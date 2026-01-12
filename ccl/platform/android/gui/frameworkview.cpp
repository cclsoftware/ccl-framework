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
// Filename    : ccl/platform/android/gui/frameworkview.cpp
// Description : Framework View (native)
//
//************************************************************************************************

#include "frameworkview.h"
#include "frameworkactivity.h"
#include "androidview.h"

#include "ccl/platform/android/gui/accessibility.android.h"

#include "ccl/platform/android/graphics/androidbitmap.h"
#include "ccl/platform/android/graphics/frameworkgraphics.h"

#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/windows/childwindow.h"
#include "ccl/gui/windows/windowmanager.h"

#include "ccl/gui/graphics/graphicsdevice.h"

#include "ccl/gui/theme/thememanager.h"

#include "ccl/gui/touch/touchinput.h"
#include "ccl/gui/touch/touchcollection.h"
#include "ccl/gui/touch/gesturemanager.h"
#include "ccl/gui/gui.h"

#include "ccl/base/math/mathcurve.h"

#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/gui/iapplication.h"
#include "ccl/public/systemservices.h"

#include <android/input.h>

#define USE_OFFSCREEN 1
#define USE_DIRTY_REGION (1 && USE_OFFSCREEN)
#define USE_CLIP_BOUNDS 0

namespace CCL {
namespace Android {

//************************************************************************************************
// dev.ccl.FrameworkView
//************************************************************************************************

DEFINE_JNI_CLASS (FrameworkViewClass)
	DEFINE_JNI_CONSTRUCTOR (construct, "(Landroid/content/Context;)V")
	DEFINE_JNI_METHOD (destruct, "()V")
	DEFINE_JNI_METHOD (getNativeViewPtr, "()J")
	DEFINE_JNI_METHOD (getRectOnScreen, "()Landroid/graphics/Rect;")
	DEFINE_JNI_METHOD (setSize, "(IIII)V")
	DEFINE_JNI_METHOD (accessibilityContentChanged, "(I)V")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// AndroidAppWindow
//************************************************************************************************

class AndroidAppWindow: public ChildWindow
{
public:
    AndroidAppWindow (RectRef size)
	: ChildWindow (kWindowModeEmbedding, size)
	{}
   
	// AndroidWindow
	bool isAppWindow () const override
	{
		return true;
	}

	void onChildLimitsChanged (View* child) override
	{
		// supress deferred "checkSizeLimits" in Window::onChildLimitsChanged
		View::onChildLimitsChanged (child);
	}
};

//************************************************************************************************
// FrameworkView::OffscreenState
//************************************************************************************************

FrameworkView::OffscreenState::OffscreenState ()
: scaler (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkView::OffscreenState::~OffscreenState ()
{
	delete scaler;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkView::OffscreenState::init (PointRef sizeInPixel, float contentScaleFactor)
{
	delete scaler;
	scaler = nullptr;
	graphics.release ();
	bitmap.release ();

	bitmap = gGraphicsFactory->createBitmap (sizeInPixel, false);
	if(bitmap)
	{
		graphics = gGraphicsFactory->createBitmapGraphics (*bitmap);
		scaler = NEW FrameworkGraphics::ScaleHelper (graphics, contentScaleFactor);
	}
	else
		CCL_WARN ("Could not allocate offscreen %d x %d", sizeInPixel.x, sizeInPixel.y)
}

//************************************************************************************************
// AndroidOffscreenGraphics
//************************************************************************************************

class AndroidOffscreenGraphics: public FrameworkBitmapGraphics
{
public:
	AndroidOffscreenGraphics (JNIEnv* jni, jobject graphics, FrameworkView* view, AndroidBitmap& offscreenBitmap)
	: FrameworkBitmapGraphics (jni, graphics, offscreenBitmap),
	  scaler (FrameworkGraphics::ScaleHelper (this, view->getContentScaleFactor ())),
	  view (view)
	{}
	
	~AndroidOffscreenGraphics ()
	{
		// invalidate FrameworkView to trigger offscreen transfer
		AndroidView.invalidate (*view);
	}

private:
	FrameworkView* view;
	FrameworkGraphics::ScaleHelper scaler;
};

//************************************************************************************************
// FrameworkView
//************************************************************************************************

bool FrameworkView::isOffscreenEnabled ()
{
#if USE_OFFSCREEN
	return true;
#else
	return false;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkView* FrameworkView::createWithContext (jobject context)
{
	FrameworkView* nativeView = nullptr;

	JniAccessor jni;
	LocalRef javaView (jni, jni.newObject (FrameworkViewClass, FrameworkViewClass.construct, FrameworkActivity::getCurrentActivity ()->getJObject ()));
	if(javaView)
	{
		JniIntPtr viewPtr = FrameworkViewClass.getNativeViewPtr (javaView);
		nativeView = FrameworkView::fromIntPtr (viewPtr);
	}
	return nativeView;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkView::FrameworkView (JNIEnv* jni, jobject object, jobject _graphics)
: JniObject (jni, object),
  graphics (NEW FrameworkGraphics (jni, _graphics)),
  parentView (nullptr),
  contentScaleFactor (1.f),
  resizing (false),
  mouseDownEventSent (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkView::~FrameworkView ()
{
	CCL_PRINTF ("~FrameworkView %x\n", this)
	delete graphics;

	if(window)
	{
		window->getTouchInputState ().discardHoverTouches ();
		window->cancelDragSession ();
		window->removed (nullptr);
		window->setFrameworkView (nullptr);
	}

	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkView::createApplicationView ()
{
	ASSERT (window == nullptr)

	// create application view
	Rect rect;
	getSize (rect);
	// note: empty size here

	contentScaleFactor = FrameworkActivity::getCurrentActivity ()->getDensityFactor ();
	graphics->setContentScaleFactor (contentScaleFactor);

	IView* appView = WindowManager::instance ().createApplicationView (rect);
	if(View* view = unknown_cast<View> (appView))
	{
		Desktop.getMonitorSize (rect, Desktop.getMainMonitor (), true);

		view->setSizeMode (View::kAttachAll);
		view->setSize (rect);

		window = NEW AndroidAppWindow (rect);
		window->addView (view);
		window->setController (GUI.getApplication ());
		if(view->getStyle ().isTransparent ())
			View::StyleModifier (*window).setCommonStyle (Styles::kTransparent);

		window->setFrameworkView (this);
		window->addToDesktop ();
		window->attached (nullptr);

		window->getTouchInputState ().setGestureManager (NEW CustomGestureManager (*window));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkView::initWithWindow (AndroidWindow* _window)
{
	ASSERT (window == nullptr)
	window.share (_window);
	window->setFrameworkView (this);

	contentScaleFactor = FrameworkActivity::getCurrentActivity ()->getDensityFactor ();
	graphics->setContentScaleFactor (contentScaleFactor);
	
	window->getTouchInputState ().setGestureManager (NEW CustomGestureManager (*window));

	window->attached (nullptr);
	window->onActivate (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameworkView::addView (FrameworkView* child)
{
	ASSERT (child != this)
	if(child->getParentView ())
		return false;

	child->parentView = this;
	children.append (child);

	ViewGroup.addView (*this, *child);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameworkView::removeView (FrameworkView* child)
{
	if(child->getParentView () != this)
		return false;

	children.remove (child);
	child->parentView = nullptr;

	ViewGroup.removeView (*this, *child);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkView::onSizeChanged (PointRef sizeInPixel)
{
	Point size (sizeInPixel);
	DpiScale::toCoordPoint (size, contentScaleFactor);

	if(window && !size.isNull ())
	{
		#if USE_OFFSCREEN
		offscreen.init (sizeInPixel, getContentScaleFactor ());
		#endif

		Rect rect (window->getSize ());
		rect.setSize (size);

		ScopedVar<bool> scope (resizing, true);
		window->setSize (rect);
	}

	#if USE_DIRTY_REGION
	dirtyRegion.join (size);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& FrameworkView::getSize (Rect& size) const
{
	size.left = AndroidView.getLeft (*this);
	size.top = AndroidView.getTop (*this);
	size.setWidth (AndroidView.getWidth (*this));
	size.setHeight (AndroidView.getHeight (*this));
	DpiScale::toCoordRect (size, contentScaleFactor);
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float FrameworkView::getContentScaleFactor () const
{
	return contentScaleFactor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidWindow* FrameworkView::getWindow () const
{
	return window;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkView::invalidate (RectRef rect, bool addToDirtyRegion)
{
	#if USE_DIRTY_REGION
	if(addToDirtyRegion && !rect.isEmpty ())
		dirtyRegion.join (rect);
	#endif

	AndroidView.invalidate (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkView::redraw ()
{
	if(window)
	{
		#if (0 && DEBUG)
		bool isAccelerated = drawDevice->isHardwareAccelerated ();
		#endif

		Rect boundsPixel;
		#if USE_CLIP_BOUNDS
		graphics->getClipBounds (boundsPixel);
		if(boundsPixel.isEmpty ())
			return;
		#else
		Rect boundsCoords;
		window->getClientRect (boundsCoords);
		boundsPixel = boundsCoords;
		DpiScale::toPixelRect (boundsPixel, getContentScaleFactor ());
		#endif

		#if USE_OFFSCREEN
		if(offscreen.graphics)
		{
			WindowGraphicsDevice graphicsDevice (*window, offscreen.graphics);
			window->setGraphicsDevice (&graphicsDevice);
			window->setInDrawEvent (true);

			#if USE_DIRTY_REGION
			if(!dirtyRegion.isEmpty ())
			{
				ForEachRectFast (dirtyRegion, Rect, r)
					offscreen.graphics->saveStateAndClip (r);

					window->draw (UpdateRgn (r));

					offscreen.graphics->restoreState ();
				EndFor
				dirtyRegion.setEmpty ();
			}
			#else
			window->draw (UpdateRgn (boundsCoords));
			#endif	

			window->setInDrawEvent (false);
			offscreen.bitmap->drawDirect (*graphics, boundsPixel);
			return;
		}
		#endif
		
		FrameworkGraphics::ScaleHelper scaler (graphics, getContentScaleFactor ());

		WindowGraphicsDevice graphicsDevice (*window, graphics);
		window->setGraphicsDevice (&graphicsDevice);
		window->setInDrawEvent (true);

		window->draw (UpdateRgn (boundsCoords));

		window->setInDrawEvent (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameworkGraphics* FrameworkView::createOffscreenDevice ()
{
	if(!offscreen.bitmap)
		return 0;

	JniAccessor jni;

	LocalRef graphics (jni, jni.newObject (FrameworkGraphicsClass, FrameworkGraphicsClass.constructWithBitmap, offscreen.bitmap->getJavaBitmap ()->getJObject ()));
	if(jni.checkException () || graphics == 0)
		return 0;

	return NEW AndroidOffscreenGraphics (jni, graphics, this, *offscreen.bitmap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkView::onTouchEvent (int actionCode, int actionId, int toolType, int buttonState, int metaState, jintArray pointerIds, jfloatArray pointerCoords, float pressure, float orientation, int source)
{
	if(!window)
		return;

	auto getEventType = [] (int actionCode)
	{
		switch(actionCode)
		{
		case AMOTION_EVENT_ACTION_DOWN:
			return TouchEvent::kBegin;

		case AMOTION_EVENT_ACTION_UP:
			return TouchEvent::kEnd;

		case AMOTION_EVENT_ACTION_HOVER_ENTER:
			return TouchEvent::kEnter;

		case AMOTION_EVENT_ACTION_HOVER_MOVE:
			return TouchEvent::kHover;

		case AMOTION_EVENT_ACTION_HOVER_EXIT:
			return TouchEvent::kLeave;

		case AMOTION_EVENT_ACTION_CANCEL:
			return TouchEvent::kCancel;

		default:
		case AMOTION_EVENT_ACTION_MOVE:
			return TouchEvent::kMove;
		}
	};

	auto getTouchType = [&] (int actionCode)
	{
		switch(actionCode)
		{
		case AMOTION_EVENT_ACTION_POINTER_DOWN:
			return TouchEvent::kBegin;

		case AMOTION_EVENT_ACTION_POINTER_UP:
			return TouchEvent::kEnd;

		default:
			return getEventType (actionCode);
		}
	};

	auto getButtons = [] (int buttonState, int metaState)
	{
		enum { AMOTION_EVENT_BUTTON_STYLUS_PRIMARY = 0x20, AMOTION_EVENT_BUTTON_STYLUS_SECONDARY = 0x40 };

		int keys = 0;
		if(buttonState & AMOTION_EVENT_BUTTON_STYLUS_PRIMARY)
			keys |= KeyState::kPenBarrel;
		if(buttonState & AMOTION_EVENT_BUTTON_STYLUS_SECONDARY)
			keys |= KeyState::kPenEraser; // to be tested

		if(metaState & AMETA_SHIFT_ON)
			keys |= KeyState::kShift;
		if(metaState & AMETA_CTRL_ON)
			keys |= KeyState::kCommand;
		if(metaState & AMETA_ALT_ON)
			keys |= KeyState::kOption;

		return keys;
	};

	auto makeTouchID = [] (jint id) -> TouchID
	{
		return id + 1; // platform IDs start at 0, avoid conflict with our TouchEvent::kNoTouchId 
	};

	int eventType = getEventType (actionCode);
	double eventTime = System::GetProfileTime ();
	int64 touchTime = eventTime * 1000;

	static Vector<jint> pointerIdData; // keep allocated
	static Vector<jfloat> pointerCoordsData;

	JniAccessor jni;
	Core::Java::JniIntArray pointerIdArray (jni, pointerIds);
	Core::Java::JniFloatArray coordsArray (jni, pointerCoords);
	pointerIdArray.getData (pointerIdData);
	coordsArray.getData (pointerCoordsData);

	ASSERT (pointerCoordsData.count () == 2 * pointerIdData.count ())

	TouchCollection touches;

	int coordIndex = 0;
	for(auto id : pointerIdData)
	{
		CoordF x = pointerCoordsData[coordIndex++];
		CoordF y = pointerCoordsData[coordIndex++];
		PointF p (x, y);

		DpiScale::toCoordPointF (p, contentScaleFactor);

		int type = (id == actionId) ? getTouchType (actionCode) : eventType;
		touches.add (TouchInfo (type, makeTouchID (id), p, touchTime));
	}

	TouchEvent touchEvent (touches, eventType);
	touchEvent.eventTime = eventTime;
	touchEvent.touchID = makeTouchID (actionId);
	touchEvent.keys.keys = getButtons (buttonState, metaState);

	// scale pressure values to conform with other platforms (seen values much closer to 0 on Android then on Windows/iOS)
	static Math::LinearCurve pressureScaler (0.9, 0.1);

	switch(toolType)
	{
	case AMOTION_EVENT_TOOL_TYPE_FINGER:
		touchEvent.inputDevice = TouchEvent::kTouchInput;
		// set original input device (e.g. for emulated touches from trackpad two-finger swipe)
		if(source == AINPUT_SOURCE_MOUSE)
			touchEvent.inputDevice = TouchEvent::kMouseInput;
		break;

	case AMOTION_EVENT_TOOL_TYPE_ERASER:
		touchEvent.keys.keys |= KeyState::kPenEraser;
		// through:
	case AMOTION_EVENT_TOOL_TYPE_STYLUS:
		touchEvent.inputDevice = TouchEvent::kPenInput;
		touchEvent.penInfo.pressure = (float)pressureScaler.getY ((double)pressure);
		touchEvent.penInfo.tiltX = Math::radToDegrees (orientation);
		break;
	}

	window->getTouchInputState ().processTouches (touchEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkView::onMouseEvent (int actionCode, int buttonState, int metaState, float posX, float posY, float hscroll, float vscroll)
{
	if(!window)
		return;

	auto getButtons = [] (int buttonState, int metaState)
	{
		int keys = 0;
		if(buttonState & AMOTION_EVENT_BUTTON_PRIMARY)
			keys |= KeyState::kLButton;
		if(buttonState & AMOTION_EVENT_BUTTON_SECONDARY)
			keys |= KeyState::kRButton;
		if(buttonState & AMOTION_EVENT_BUTTON_TERTIARY)
			keys |= KeyState::kMButton;

		if(metaState & AMETA_SHIFT_ON)
			keys |= KeyState::kShift;
		if(metaState & AMETA_CTRL_ON)
			keys |= KeyState::kCommand;
		if(metaState & AMETA_ALT_ON)
			keys |= KeyState::kOption;

		return keys;
	};

	Point where (posX, posY);

	DpiScale::toCoordPoint (where, contentScaleFactor);

	int buttons = getButtons (buttonState, metaState);
	double eventTime = System::GetProfileTime ();

	static const int kDragDistance = 2; // in coord units
	static const int kDragTimeout = 200; // in ms

	static const double kDoubleClickTime = 0.5; // in seconds

	switch(actionCode)
	{
	case AMOTION_EVENT_ACTION_BUTTON_PRESS:
		// check for double click
		if(mouseDownEvent.eventTime >= eventTime - kDoubleClickTime && mouseDownEvent.keys == buttons)
		{
			mouseDownEvent = MouseEvent (MouseEvent::kMouseDown, where, buttons, eventTime);
			mouseDownEvent.doubleClicked = 1;

			sendMouseDownEvent (false);
			return;
		}

		// save mouse down event to allow detecting drag and double click
		mouseDownEvent = MouseEvent (MouseEvent::kMouseDown, where, buttons, eventTime);
		mouseDownEventSent = false;

		if(buttons & KeyState::kLButton)
			(NEW Message ("dragTimerElapsed"))->post (this, kDragTimeout);
		else
			sendMouseDownEvent (false);

		return;

	case AMOTION_EVENT_ACTION_BUTTON_RELEASE:
		if(!mouseDownEventSent)
			sendMouseDownEvent (false);

		window->onMouseUp (MouseEvent (MouseEvent::kMouseUp, where, buttons, eventTime));
		if(mouseDownEvent.keys == KeyState::kRButton)
			window->popupContextMenu (where, false);
		return;

	case AMOTION_EVENT_ACTION_MOVE:
		if(!mouseDownEventSent && !Rect (mouseDownEvent.where, mouseDownEvent.where).expand (kDragDistance).pointInside (where))
			sendMouseDownEvent (true);

		window->onMouseMove (MouseEvent (MouseEvent::kMouseMove, where, buttons, eventTime));
		return;

	default:
	case AMOTION_EVENT_ACTION_HOVER_MOVE:
		window->onMouseMove (MouseEvent (MouseEvent::kMouseMove, where, buttons, eventTime));
		return;

	case AMOTION_EVENT_ACTION_SCROLL:
		if(vscroll > 0)
			window->onMouseWheel (MouseWheelEvent (MouseWheelEvent::kWheelUp, where, buttons, vscroll));
		else if(vscroll < 0)
			window->onMouseWheel (MouseWheelEvent (MouseWheelEvent::kWheelDown, where, buttons, vscroll));
		else if(hscroll > 0)
			window->onMouseWheel (MouseWheelEvent (MouseWheelEvent::kWheelRight, where, buttons, -hscroll));
		else if(hscroll < 0)
			window->onMouseWheel (MouseWheelEvent (MouseWheelEvent::kWheelLeft, where, buttons, -hscroll));

		return;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkView::sendMouseDownEvent (bool dragged)
{
	if(dragged)
		mouseDownEvent.dragged = 1;

	mouseDownEventSent = true;
	window->onMouseDown (mouseDownEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkView::fillAccessibilityNodeInfo (int virtualViewId, jobject info) const
{
	if(!window)
		return;

	if(AccessibilityElementProvider* provider = AccessibilityElementProvider::toPlatformProvider (window->getAccessibilityProvider ()))
		provider->fillAccessibilityNodeInfo (*this, virtualViewId, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int FrameworkView::getVirtualViewAt (PointRef pos) const
{
	if(!window)
		return -1;

	if(AccessibilityElementProvider* provider = AccessibilityElementProvider::toPlatformProvider (window->getAccessibilityProvider ()))
		return provider->getVirtualViewAt (pos);
	else
		return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameworkView::accessibilityContentChanged (int virtualViewId)
{
	FrameworkViewClass.accessibilityContentChanged (*this, virtualViewId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FrameworkView::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "dragTimerElapsed")
		if(!mouseDownEventSent)
			sendMouseDownEvent (false);
}

//************************************************************************************************
// FrameworkView Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD_CCLGUI (JniIntPtr, FrameworkView, constructNative, jobject graphics)
{
	FrameworkView* nativeView = NEW FrameworkView (env, This, graphics);
	return nativeView->asIntPtr ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, FrameworkView, destructNative, JniIntPtr nativeViewPtr)
{
	if(FrameworkView* nativeView = FrameworkView::fromIntPtr (nativeViewPtr))
		delete nativeView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, FrameworkView, onLayoutNative, JniIntPtr nativeViewPtr)
{
	if(FrameworkView* nativeView = FrameworkView::fromIntPtr (nativeViewPtr))
		if(AndroidWindow* window = nativeView->getWindow ())
			window->updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, FrameworkView, onSizeChangedNative, JniIntPtr nativeViewPtr, int width, int height)
{
	if(FrameworkView* nativeView = FrameworkView::fromIntPtr (nativeViewPtr))
		nativeView->onSizeChanged (Point (width, height));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, FrameworkView, onApplyWindowInsetsNative, JniIntPtr nativeViewPtr, jobject insets)
{
	ThemeManager::instance ().onSystemMetricsChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, FrameworkView, redrawNative, JniIntPtr nativeViewPtr)
{
	if(FrameworkView* nativeView = FrameworkView::fromIntPtr (nativeViewPtr))
		nativeView->redraw ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, FrameworkView, onTouchEventNative, JniIntPtr nativeViewPtr, int actionCode, int actionId, int toolType, int buttonState, int metaState, jintArray pointerIds, jfloatArray pointerCoords, float pressure, float orientation, int source)
{
	if(FrameworkView* nativeView = FrameworkView::fromIntPtr (nativeViewPtr))
		nativeView->onTouchEvent (actionCode, actionId, toolType, buttonState, metaState, pointerIds, pointerCoords, pressure, orientation, source);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, FrameworkView, onMouseEventNative, JniIntPtr nativeViewPtr, int actionCode, int buttonState, int metaState, float posX, float posY, float hscroll, float vscroll)
{
	if(FrameworkView* nativeView = FrameworkView::fromIntPtr (nativeViewPtr))
		nativeView->onMouseEvent (actionCode, buttonState, metaState, posX, posY, hscroll, vscroll);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, FrameworkView, fillAccessibilityNodeInfoNative, JniIntPtr nativeViewPtr, int virtualViewId, jobject node)
{
	if(FrameworkView* nativeView = FrameworkView::fromIntPtr (nativeViewPtr))
		nativeView->fillAccessibilityNodeInfo (virtualViewId, node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, FrameworkView, getVirtualViewAtNative, JniIntPtr nativeViewPtr, int x, int y)
{
	if(FrameworkView* nativeView = FrameworkView::fromIntPtr (nativeViewPtr))
		nativeView->getVirtualViewAt (Point (x, y));
}
