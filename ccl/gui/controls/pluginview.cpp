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
// Filename    : ccl/gui/controls/pluginview.cpp
// Description : Plugin View
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/controls/pluginview.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/windows/childwindow.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/views/mousehandler.h"

#include "ccl/base/singleton.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/gui/framework/iwin32specifics.h"
#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// ChildWindowDelegate
//************************************************************************************************

class ChildWindowDelegate: public View
{
public:
	ChildWindowDelegate (View* owner, RectRef size);

	PROPERTY_POINTER (View, owner, Owner)

	// View
	void onActivate (bool state) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	bool onFocus (const FocusEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onKeyUp (const KeyEvent& event) override;
};

//************************************************************************************************
// PlugInViewStatics
//************************************************************************************************

class PlugInViewStatics: public Object,
						 public IPlugInViewStatics,
						 public Singleton<PlugInViewStatics>
{
public:
	DECLARE_CLASS (PlugInViewStatics, Object)

	// IPlugInViewStatics
	tbool CCL_API isSystemScalingAvailable () const override;
	tresult CCL_API setManagementInterface (IPlugInViewManagement* plugInViewManagement) override;

	CLASS_INTERFACE (IPlugInViewStatics, Object)
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

#if CCL_PLATFORM_WINDOWS // access to IDpiInfo singleton from platform code
static AutoPtr<Win32::IDpiInfo> dpiInfo;
static Win32::IDpiInfo& getDpiInfo ()
{
	if(dpiInfo == nullptr)
		dpiInfo = ccl_new<Win32::IDpiInfo> (Win32::ClassID::DpiInfo);
	ASSERT (dpiInfo.isValid ())
	return *dpiInfo;

}
CCL_KERNEL_TERM_LEVEL (PlugInView, kFrameworkLevelFirst)
{
	dpiInfo.release ();
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IView* CCL_API System::CCL_ISOLATED (CreateFrameworkView) (IUnknown* plugIn, StringID name)
{
	// try to create framework view directly...
	UnknownPtr<IViewFactory> viewFactory (plugIn);
	if(viewFactory)
	{
		IView* view = viewFactory->createView (name, Variant (), Rect ());
		if(view)
			return view;
	}

	// ...or try plug-in view
	return PlugInView::createPlugInView (plugIn, name);
}

//************************************************************************************************
// PlugInView::PlugInCallScope
//************************************************************************************************

struct PlugInView::PlugInCallScope
{
	#if CCL_PLATFORM_WINDOWS
	Win32::DpiAwarenessContext oldContext;
	bool changed;

	PlugInCallScope (const PlugInView& view)
	: oldContext (Win32::kDpiContextDefault),
	  changed (false)
	{
		if(view.hostingMode == kSystemScaledHosting)
		{
			oldContext = getDpiInfo ().getCurrentDpiAwarenessContext ();
			if(oldContext != Win32::kDpiContextUnaware)
			{
				getDpiInfo ().switchToDpiAwarenessContext (Win32::kDpiContextUnaware);
				changed = true;
			}
		}
	}

	~PlugInCallScope ()
	{
		if(changed)
			getDpiInfo ().switchToDpiAwarenessContext (oldContext);
	}
	#else
	PlugInCallScope (const PlugInView& view) {} // nothing here
	#endif
};

//************************************************************************************************
// ChildWindowDelegate
//************************************************************************************************

ChildWindowDelegate::ChildWindowDelegate (View* owner, RectRef size)
: View (size),
  owner (owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ChildWindowDelegate::onActivate (bool state)
{
	//CCL_ADD_INDENT (2)
	//CCL_PRINTF ("%sChildWindowDelegate::onActivate (%s)\n", CCL_INDENT, state ? "true" : "false");

	View::onActivate (state);

	// activated via ChildWindow (e.g. mouse click): activate WindowBase of owner view
	if(owner && state)
		if(WindowBase* windowBase = owner->getParent<WindowBase> ())
			windowBase->activate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ChildWindowDelegate::onMouseWheel (const MouseWheelEvent& event)
{
	if(owner)
		return owner->onMouseWheel (event);
	return View::onMouseWheel (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ChildWindowDelegate::onFocus (const FocusEvent& event)
{
	if(owner)
		return owner->onFocus (event);
	return View::onFocus (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ChildWindowDelegate::onKeyDown (const KeyEvent& event)
{
	if(owner)
		return owner->onKeyDown (event);
	return View::onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ChildWindowDelegate::onKeyUp (const KeyEvent& event)
{
	if(owner)
		return owner->onKeyUp (event);
	return View::onKeyUp (event);
}

//************************************************************************************************
// PlugInViewStatics
//************************************************************************************************

DEFINE_SINGLETON_CLASS (PlugInViewStatics, Object)
DEFINE_CLASS_UID (PlugInViewStatics, 0xb6035e5e, 0xfe1b, 0x4f61, 0x95, 0x2a, 0xe3, 0x5e, 0x7d, 0x15, 0xa4, 0xf0)
DEFINE_SINGLETON (PlugInViewStatics)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInViewStatics::isSystemScalingAvailable () const
{
	return PlugInView::isSystemScalingAvailable ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInViewStatics::setManagementInterface (IPlugInViewManagement* plugInViewManagement)
{
	return PlugInView::setManagementInterface (plugInViewManagement) ? kResultOk : kResultFailed;
}

//************************************************************************************************
// PlugInView
//************************************************************************************************

PlugInView* PlugInView::createPlugInView (IUnknown* plug, StringID name)
{
	UnknownPtr<IPlugInViewFactory> factory (plug);
	if(!factory)
		return nullptr;

	IPlugInView* plugView = nullptr;
	if(factory->createPlugInView (plugView, name) != kResultOk || !plugView)
		return nullptr;

	// determine hosting mode
	HostingMode hostingMode = kDefaultHosting;
	int style = plugView->getStyle ();
	bool unitIsPixels = (style & IPlugInView::kUnitIsPixels) != 0;
	if(unitIsPixels == true)
	{
		auto shouldEnableSystemScaling = [&] ()
		{
			if(plugInViewManagement)
			{
				const IClassDescription* description = ccl_classof (plugView->getOwner ());
				if(description)
					return plugInViewManagement->isSystemScalingEnabled (description->getClassID ()) != 0;
			}
			return false;
		};

		bool canScale = (style & IPlugInView::kCanScale) != 0;
		bool scalingAware = (style & IPlugInView::kSystemScalingAware) != 0;
		if(canScale == false && isSystemScalingAvailable () && (scalingAware || shouldEnableSystemScaling ()))
			hostingMode = kSystemScaledHosting;
		else
			hostingMode = kPixelUnitHosting;
	}

	PlugInView* view = NEW PlugInView;
	view->setHostingMode (hostingMode);
	view->setView (plugView);
	plugView->release ();

	// init mode if view is sizable
	if(view->isSizable ())
		view->setSizeMode (View::kAttachAll);

	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInView::isSystemScalingAvailable ()
{
	#if CCL_PLATFORM_WINDOWS
	static int scalingAvailibility = -1;
	if(scalingAvailibility == -1)
	{
		Win32::IDpiInfo& dpiInfo = getDpiInfo ();
		scalingAvailibility = dpiInfo.canSwitchDpiHostingBehavior () &&
							  dpiInfo.canSwitchDpiAwarenessContext () ? 1 : 0;
	}
	return scalingAvailibility == 1;
	#else
	return false;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPlugInViewManagement* PlugInView::plugInViewManagement = nullptr;
bool PlugInView::setManagementInterface (IPlugInViewManagement* _plugInViewManagement)
{
	plugInViewManagement = _plugInViewManagement;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInView* PlugInView::attachingView = nullptr;
PlugInView* PlugInView::getAttachingView ()
{
	return attachingView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (PlugInView, View)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (PlugInView::customStyles)
	{"focus", Styles::kPlugInViewBehaviorFocus},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInView::PlugInView (const Rect& size, StyleRef style, StringRef title)
: View (size, style, title),
  hostingMode (kDefaultHosting),
  plugView (nullptr),
  childWindow (nullptr),
  initialScaleFactor (1),
  plugViewOnSizePending (false),
  childWindowSizeChanging (0)
  #if DEBUG
  ,insideScalingChanged (false)
  #endif
{
	wantsFocus (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInView::~PlugInView ()
{
	setView (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInView::queryInterface (UIDRef iid, void** ptr)
{
	// delegate to PlugInView
	if(iid == ccl_iid<IPlugInView> () && plugView)
		return plugView->queryInterface (iid, ptr);

	QUERY_INTERFACE (IPlugInViewFrame)
	QUERY_INTERFACE (IPlugInViewParamFinder) // <-- use this for pixel/point conversion
	QUERY_INTERFACE (IPlugInViewRendererFrame)
	QUERY_INTERFACE (ITimerTask)
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInView::queryWindowInterface (UIDRef iid, void** ptr)
{
	if(childWindow)
		return childWindow->queryInterface (iid, ptr);
	return kResultNoInterface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInView::isSizable () const
{
	// in addition to the kSizable flag, the limits must allow sizing in at least one direction
	return plugView && (plugView->getStyle () & IPlugInView::kSizable)
		&& (!const_cast<PlugInView*> (this)->getSizeLimits ().isValid () || (sizeLimits.minWidth < sizeLimits.maxWidth || sizeLimits.minHeight < sizeLimits.maxHeight));

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInView::wantsExtendedInput () const
{
	return plugView && (plugView->getStyle () & IPlugInView::kWantsExtendedInput);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float PlugInView::getContentScaleFactor () const
{
	if(Window* w = const_cast<PlugInView&> (*this).getWindow ())
		return w->getContentScaleFactor ();

	// fallback to primary monitor
	return Desktop.getMonitorScaleFactor (Desktop.getMainMonitor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& PlugInView::toPlugInPoint (Point& p) const
{
	if(hostingMode == kPixelUnitHosting)
		DpiScale::toPixelPoint (p, getContentScaleFactor ());
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& PlugInView::toPlugInRect (Rect& r) const
{
	if(hostingMode == kPixelUnitHosting)
		DpiScale::toPixelRect (r, getContentScaleFactor ());
	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& PlugInView::fromPlugInRect (Rect& r) const
{
	if(hostingMode == kPixelUnitHosting)
		DpiScale::toCoordRect (r, getContentScaleFactor ());
	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInView::setHostingMode (HostingMode mode)
{
	hostingMode = mode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInView::setView (IPlugInView* _plugView)
{
	ASSERT (isAttached () == false)

	if(plugView)
	{
		plugView->setFrame (nullptr);
		plugView->release ();

		GUI.removeIdleTask (this);
	}

	plugView = _plugView;

	if(plugView)
	{
		// init scaling and size
		initialScaleFactor = getContentScaleFactor ();		
		Rect rect;
		{
			PlugInCallScope scope (*this);
			plugView->onViewEvent (DisplayChangedEvent (initialScaleFactor));
			plugView->getSize (rect);
		}

        if(!rect.isEmpty ())
		    View::setSize (fromPlugInRect (rect));

		plugView->setFrame (this);
		plugView->retain ();

		GUI.addIdleTask (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPlugInView* PlugInView::getView () const
{
	return plugView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInView::calcSizeLimits ()
{
	if(plugView)
	{
		{
			PlugInCallScope scope (*this);
			plugView->getSizeLimits (sizeLimits);
		}		
		if(hostingMode == kPixelUnitHosting)
		{
			float scaleFactor = getContentScaleFactor ();
			if(sizeLimits.minWidth > 0)
				sizeLimits.minWidth = DpiScale::pixelToCoord (sizeLimits.minWidth, scaleFactor);
			if(sizeLimits.minHeight > 0)
				sizeLimits.minHeight = DpiScale::pixelToCoord (sizeLimits.minHeight, scaleFactor);
			if(sizeLimits.maxWidth < kMaxCoord)
				sizeLimits.maxWidth = DpiScale::pixelToCoord (sizeLimits.maxWidth, scaleFactor);
			if(sizeLimits.maxHeight < kMaxCoord)
				sizeLimits.maxHeight = DpiScale::pixelToCoord (sizeLimits.maxHeight, scaleFactor);
		}

		ccl_lower_limit (sizeLimits.minWidth, 1);
		ccl_lower_limit (sizeLimits.minHeight, 1);

		if((plugView->getStyle () & IPlugInView::kSizable)
			&& (!sizeLimits.isValid () || sizeLimits.minWidth < sizeLimits.maxWidth || sizeLimits.minHeight < sizeLimits.maxHeight))
				privateFlags |= kExplicitSizeLimits;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInView::constrainSize (Rect& rect) const
{
	if(plugView && !rect.isEmpty ())
	{
		Rect suggestedRect (rect);
		Point suggestedSize (suggestedRect.getSize ());

		toPlugInRect (rect);
		{
			PlugInCallScope scope (*this);
			plugView->constrainSize (rect);
		}
		fromPlugInRect (rect);

		Point constrainedSize (rect.getSize ());
		CCL_PRINTF ("%sPlugInView::constrainSize: %d x %d %s\n", CCL_INDENT, suggestedSize.x, suggestedSize.y, suggestedSize == constrainedSize ? "(accepted)" : MutableCString ().appendFormat ("-> %d x %d (diff: %d x %d)", constrainedSize.x, constrainedSize.y, constrainedSize.x - suggestedSize.x, constrainedSize.y - suggestedSize.y).str ())

		if(constrainedSize != suggestedSize)
		{
			// adjust size limits to allow this size returned by the plug-in
			SizeLimit limits (ccl_const_cast (this)->getSizeLimits ());
			if(!limits.isAllowed (constrainedSize))
			{
				if(constrainedSize.x < limits.minWidth)
					limits.minWidth = constrainedSize.x;
				if(constrainedSize.y < limits.minHeight)
					limits.minHeight = constrainedSize.y;
				if(constrainedSize.x > limits.maxWidth)
					limits.maxWidth = constrainedSize.x;
				if(constrainedSize.y > limits.maxHeight)
					limits.maxHeight = constrainedSize.y;
				ccl_const_cast (this)->setSizeLimits (limits);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInView::calcPlugViewRect (Rect& rect) const
{
    getClientRect (rect);
	constrainSize (rect);

    Point pos;
    clientToWindow (pos);
    rect.offset (pos);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInView::attach ()
{
	Window* w = getWindow ();
	if(w == nullptr)
		return;

	if(childWindow != nullptr)
	{
		if(w->isSystemWindowValid ())
			plugView->attached (childWindow->getSystemWindow ());
	}
	else
	{
		plugView->attached (w->getSystemWindow ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInView::attached (View* parent)
{
	CCL_LOGSCOPE ("PlugInView::attached")
	SuperClass::attached (parent);

	if(!plugView)
		return;

	ScopedVar<PlugInView*> attachScope (attachingView, this);
	Window* w = getWindow ();
	w->addObserver (this);

	#if CCL_PLATFORM_WINDOWS // switch DPI context if needed (Windows only)
	bool dpiContextChanged = false;
	if(hostingMode == kSystemScaledHosting)
	{
		bool supportedByParent = w->getStyle ().isCustomStyle (Styles::kWindowBehaviorPluginViewHost);
		ASSERT (supportedByParent == true)
		if(supportedByParent == false) // unexpected fallback!
			hostingMode = kPixelUnitHosting;
		else
		{
			// switch per-thread DPI context to "unaware" to enable system scaling
			getDpiInfo ().switchToDpiAwarenessContext (Win32::kDpiContextUnaware);
			dpiContextChanged = true;
		}
	}
	else
	{
		Win32::DpiAwarenessContext oldContext = getDpiInfo ().getCurrentDpiAwarenessContext ();
		CCL_PRINTF ("%sPlugInView::attached: previous DPI Awareness Context: %s\n", CCL_INDENT, oldContext == Win32::kDpiContextDefault ? "Default" : (oldContext ==  Win32::kDpiContextUnaware ? "Unaware" : "SystemAware"))

		// switch back to default when reusing a window that contained another plug-in
		if(oldContext != Win32::kDpiContextDefault)
			getDpiInfo ().switchToDpiAwarenessContext (Win32::kDpiContextDefault);
	}
	#endif

	Rect rect;
	calcPlugViewRect (rect);

	// check if plug-in view is using bitmap rendering
	renderer = plugView;
	if(renderer)
	{
		tbool renderTypeSupported = renderer->isRenderingTypeSupported (ccl_iid<IBitmap> (), kDefaultBitmapFormat);
		ASSERT (renderTypeSupported)
		if(!renderTypeSupported)
		{
			CCL_WARN ("Plug-in renderer does not support default bitmap type and pixel format!\n", 0)
			renderer.release ();
		}

		plugView->attached (nullptr);
	}
	else
	{
		// Use child window on Windows, Linux, and sometimes Mac platforms.
		// Assume other platforms attach to the parent window directly.
	
		bool useChildWindow = false;
		#if CCL_PLATFORM_WINDOWS || CCL_PLATFORM_LINUX || CCL_PLATFORM_MAC
		useChildWindow = true;
		#endif
		
		if(useChildWindow)
		{
			ASSERT (childWindow == nullptr)
			childWindow = NEW ChildWindow (Window::kWindowModeHosting, rect, StyleFlags (Styles::kTransparent));
			childWindow->makeNativeWindow (w->getSystemWindow ());
			View* delegateView = NEW ChildWindowDelegate (this, Rect (0, 0, rect.getWidth (), rect.getHeight ()));
			delegateView->setSizeMode (kAttachAll);
			childWindow->addView (delegateView);
			childWindow->addObserver (this);
			attach ();
			childWindow->show ();
		}
		else
		{
			attach ();
		}
	}

	// check if window opened on another monitor
	float scaleFactor = w->getContentScaleFactor ();
	if(scaleFactor != initialScaleFactor)
	{
		initialScaleFactor = scaleFactor;

		if(hostingMode == kPixelUnitHosting && isSizable () == false)
		{
			Rect rect;
			plugView->getSize (rect);
			if(!rect.isEmpty ())
			{
				// add offset from window (see calcPlugViewRect)
				Point pos;
				clientToWindow (pos);
				rect.offset (pos);

				View::setSize (fromPlugInRect (rect));
			}
		}

		onDisplayPropertiesChanged (DisplayChangedEvent (scaleFactor));
	}

	if(plugViewOnSizePending)
	{
		plugViewOnSizePending = false;
		plugView->onSize (toPlugInRect (rect));
	}

	#if CCL_PLATFORM_WINDOWS
	if(dpiContextChanged)
		getDpiInfo ().switchToDpiAwarenessContext (Win32::kDpiContextDefault);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInView::removed (View* parent)
{
	Window* w = getWindow ();
	if(w)
		w->removeObserver (this);

	SuperClass::removed (parent);

	if(plugView)
	{
		UserInterface::TimerBlocker timerBlocker;
		PlugInCallScope scope (*this);
		plugView->removed ();
	}

	if(childWindow)
	{
		childWindow->removeObserver (this);
		childWindow->close ();
		childWindow = nullptr;
	}

	renderer.release ();
	renderBitmap.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInView::draw (const UpdateRgn& updateRgn)
{
	if(renderer)
	{
		Rect clientRect;
		getClientRect (clientRect);

		Point sizeInPoint = clientRect.getSize ();
		if(sizeInPoint.x > 0 && sizeInPoint.y > 0)
		{
			bool initialRedraw = false;
			bool unitIsPixels = (plugView->getStyle () & IPlugInView::kUnitIsPixels) != 0;
			float bitmapScaling = getContentScaleFactor ();
			
			PixelPoint sizeInPixel (sizeInPoint, bitmapScaling);	
			if(!renderBitmap || renderBitmap->getPixelSize () != sizeInPixel)
			{
				renderBitmap.release ();
				renderBitmap = NEW Bitmap (sizeInPoint.x, sizeInPoint.y, kDefaultBitmapFormat, bitmapScaling);
				initialRedraw = true;
			}

			UpdateRgn renderRegion (initialRedraw ? UpdateRgn (clientRect) : updateRgn);
			Rect bitmapBounds (renderRegion.bounds);
			if(hostingMode == kPixelUnitHosting && bitmapScaling != 1.f)
			{
				Rect regionRect (renderRegion.bounds);
				toPlugInRect (regionRect);
				renderRegion = UpdateRgn (regionRect);
			}
			renderer->draw (renderBitmap->asUnknown (), renderRegion);

			GraphicsPort port (this);
			port.drawImage (renderBitmap, bitmapBounds, bitmapBounds);
		}
	}
	SuperClass::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInView::repairPlugViewSize ()
{
	CCL_PRINTLN ("Attempting plug-in view repair...")
	if(hostingMode == kSystemScaledHosting)
		if(UnknownPtr<IPlugInViewRepair> viewRepair = plugView)
		{
			PlugInCallScope scope (*this);
			Rect detectedSize;
			if(viewRepair->detectSize (detectedSize))
			{
				Coord detectedWidth = detectedSize.getWidth ();
				Coord detectedHeight = detectedSize.getHeight ();
				Coord expectedWidth = childWindow->getWidth ();
				Coord expectedHeight = childWindow->getHeight ();
				bool mismatch = detectedWidth != expectedWidth || detectedHeight != expectedHeight;
				if(mismatch == true)
				{
					CCL_PRINTLN ("Plug-in view size mismatch detected.")
					Rect expectedSize (0, 0, expectedWidth, expectedHeight);
					viewRepair->repairSize (expectedSize);
				}
			}
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInView::notify (ISubject* subject, MessageRef msg)
{
	if(subject == childWindow)
	{
		if(msg == kSizeChanged)
		{
			if(childWindowSizeChanging >= 10) // quick fix: avoid recursion caused by pixel/point rounding errors in child window
				return;
			childWindowSizeChanging++;

			Rect rect (childWindow->getSize ());
			rect.moveTo (getSize ().getLeftTop ());

			if(plugView && !isSizable ())
			{
				SizeLimit limits;
				limits.setFixed (rect.getSize ());
				setSizeLimits (limits);
			}
			else
				resetSizeLimits ();

			setSize (rect);

			if(parent && !isResizing ())
				parent->onChildLimitsChanged (this);

			childWindowSizeChanging--;
		}
		else if(msg == kDestroyed)
		{
			childWindow->removeObserver (this);
			childWindow = nullptr;
		}
	}
	else if(subject == getWindow ())
	{
		if(msg == IWindow::kSystemWindowChanged)
			attach ();
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInView::onSize (const Point& delta)
{
	CCL_PRINTF ("%sPlugInView::onSize (%d, %d) -> now %d x %d\n", CCL_INDENT, delta.x, delta.y, getSize ().getWidth (), getSize ().getHeight ())
	View::onSize (delta);

    Rect rect;
	calcPlugViewRect (rect);

	if(childWindow)
		childWindow->setSize (rect);

	if(plugView)
	{
		if(isAttached ())
		{
			plugViewOnSizePending = false;
			PlugInCallScope scope (*this);
			plugView->onSize (toPlugInRect (rect));
		}
		else
			plugViewOnSizePending = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInView::onMove (const Point& delta)
{
    Rect rect;
	calcPlugViewRect (rect);

	if(childWindow)
		childWindow->setSize (rect);

	if(plugView)
	{
		if(isAttached ())
		{
			plugViewOnSizePending = false;
			PlugInCallScope scope (*this);
			plugView->onSize (toPlugInRect (rect));
		}
		else
			plugViewOnSizePending = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInView::getFrameSize (Rect& size) const
{
	size = getSize ();

	Point pos;
	clientToWindow (pos);
	size.moveTo (pos);

	toPlugInRect (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInView::setFrameSize (const Rect& size)
{
	CCL_PRINTF ("%sPlugInView::setFrameSize %d %d %d %d %s\n", CCL_INDENT, size.left, size.top, size.right, size.bottom,
															 insideScalingChanged ? "inside DPI change" : "")
	Rect rect = size;
	fromPlugInRect (rect);

	rect.moveTo (getSize ().getLeftTop ());

	if(!isSizable ())
	{
		SizeLimit limits;
		limits.setFixed (rect.getSize ());
		setSizeLimits (limits);
	}
	else
		resetSizeLimits ();

	if(this->size == rect && isAttached ())
	{
		// send callback also when view size did not change
		calcPlugViewRect (rect);
		plugViewOnSizePending = false;
		PlugInCallScope scope (*this);

		Rect plugRect (rect);
		plugView->onSize (toPlugInRect (plugRect));

		if(childWindow)
			childWindow->setSize (rect);
	}
	else
		View::setSize (rect);

	if(parent)
		parent->onChildLimitsChanged (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInView::enableParentProtection (tbool state, void*& protectedData)
{
	#if CCL_PLATFORM_WINDOWS
	if(childWindow)
		Win32Window::cast (childWindow)->suspendParent (state != 0, protectedData);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInView::onPluginViewActivated ()
{
	auto windowBase = getParent<WindowBase> ();
	if(windowBase)
		windowBase->activate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInView::invalidateFrame (const Rect& _rect)
{
	Rect rect (_rect);
	fromPlugInRect (rect);
	invalidate (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API PlugInView::createParameterIdentity (const Point& p)
{
	if(UnknownPtr<IPlugInViewParamFinder> finder = plugView)
	{
		PlugInCallScope scope (*this);
		if(hostingMode == kPixelUnitHosting)
		{
			PixelPoint pixelPoint (p, getContentScaleFactor ());
			return finder->createParameterIdentity (pixelPoint);
		}
		else
			return finder->createParameterIdentity (p);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInView::onIdle ()
{
	GUI.flushUpdates (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInView::onTimer (ITimer* timer)
{
	if(plugView)
	{
		// EXPERIMENTAL:
		//repairPlugViewSize ();

		PlugInCallScope scope (*this);
		plugView->onIdle ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInView::onDisplayPropertiesChanged (const DisplayChangedEvent& event)
{
	CCL_PRINTF ("%sPlugInView::onDisplayPropertiesChanged: scale factor %f %s\n", CCL_INDENT, event.contentScaleFactor, insideScalingChanged ? "RECURSIVE!" : "")
	#if DEBUG
	ScopedVar<bool> scope (insideScalingChanged, true);
	#endif

	if(plugView)
	{
		#if CCL_PLATFORM_WINDOWS
		if(childWindow)
		{
			Point sizeInPoint (getWidth (), getHeight ());
			PixelPoint oldPixelSize (sizeInPoint, childWindow->getContentScaleFactor ());
			PixelPoint newPixelSize (sizeInPoint, event.contentScaleFactor);

			Rect newPixelRect;
			bool resizeNeeded = false;
			if(hostingMode == kPixelUnitHosting)
			{
				bool canScale = (plugView->getStyle () & IPlugInView::kCanScale) != 0;
				if(isSizable () == true && canScale == false) // only if view doesn't support DPI scaling
				{
					newPixelRect = Rect (newPixelSize);
					resizeNeeded = true;
				}
				else if(canScale)
					newPixelRect = Rect (newPixelSize); // plug-in will scale to the new pixel size (but no resize needed, size in coords stays the same)
				else
					newPixelRect = Rect (oldPixelSize);
			}
			else
				newPixelRect = Rect (newPixelSize);

			Point offsetInPoint;
			clientToWindow (offsetInPoint);
			PixelPoint newPixelOffset (offsetInPoint, event.contentScaleFactor);
			newPixelRect.moveTo (newPixelOffset);

			CCL_PRINTF ("%sHost adjusts child window to %d x %d pixel\n", CCL_INDENT, newPixelRect.getWidth (), newPixelRect.getHeight ())
			childWindow->onDpiChanged (event.contentScaleFactor, newPixelRect, true); // suppress further pixel size adjustment!

			PlugInCallScope scope (*this);
			if(resizeNeeded == true)
			{
				CCL_PRINTF ("%sHost calls IPlugInView::onSize()\n", CCL_INDENT)
				plugView->onSize (newPixelRect);
			}
		}
		#endif // CCL_PLATFORM_WINDOWS

		CCL_PRINTF ("%sHost calls IPlugInView::onViewEvent() with new scaling factor %f\n", CCL_INDENT, event.contentScaleFactor)
		plugView->onViewEvent (event);
	}

	SuperClass::onDisplayPropertiesChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* PlugInView::createMouseHandler (const MouseEvent& event)
{
	class Handler: public MouseHandler
	{
	public:
		Handler (PlugInView* view)
		: MouseHandler (view)
		{}

		PlugInView* getPlugInView () { return static_cast<PlugInView*> (view); }
		IPlugInView* getIPlugInView () { return getPlugInView ()->getView (); }

		// MouseHandler
		void onBegin () override
		{
			PlugInCallScope scope (*getPlugInView ());
			getIPlugInView ()->onViewEvent (current);
		}

		bool onMove (int moveFlags) override
		{
			PlugInCallScope scope (*getPlugInView ());
			return getIPlugInView ()->onViewEvent (current) != 0;
		}

		void onRelease (bool canceled) override
		{
			PlugInCallScope scope (*getPlugInView ());
			getIPlugInView ()->onViewEvent (current);
		}
	};
	
	return wantsExtendedInput () ? NEW Handler (this) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInView::onMouseDown (const MouseEvent& event)
{
	if(wantsExtendedInput ())
	{
		if(!event.keys.isSet (KeyState::kLButton))
		{
			MouseEvent e2 (event);
			toPlugInPoint (e2.where);

			PlugInCallScope scope (*this);
			return plugView->onViewEvent (e2) != 0;
		}
	}
	return SuperClass::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInView::onMouseUp (const MouseEvent& event)
{
	if(wantsExtendedInput ())
	{
		MouseEvent e2 (event);
		toPlugInPoint (e2.where);

		PlugInCallScope scope (*this);
		return plugView->onViewEvent (e2) != 0;
	}
	else
		return SuperClass::onMouseUp (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInView::onMouseEnter (const MouseEvent& event)
{
	if(wantsExtendedInput ())
	{
		MouseEvent e2 (event);
		toPlugInPoint (e2.where);
		
		PlugInCallScope scope (*this);
		return plugView->onViewEvent (e2) != 0;
	}
	else
		return SuperClass::onMouseEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInView::onMouseMove (const MouseEvent& event)
{
	if(wantsExtendedInput ())
	{
		MouseEvent e2 (event);
		toPlugInPoint (e2.where);

		PlugInCallScope scope (*this);
		return plugView->onViewEvent (e2) != 0;
	}
	else
		return SuperClass::onMouseMove (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInView::onMouseLeave (const MouseEvent& event)
{
	if(wantsExtendedInput ())
	{
		MouseEvent e2 (event);
		toPlugInPoint (e2.where);

		PlugInCallScope scope (*this);
		return plugView->onViewEvent (e2) != 0;
	}
	else
		return SuperClass::onMouseLeave (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInView::onMouseWheel (const MouseWheelEvent& event)
{
	if(plugView)
	{
		MouseWheelEvent e2 (event);
		toPlugInPoint (e2.where);

		PlugInCallScope scope (*this);
		return plugView->onViewEvent (e2) != 0;
	}
	else
		return SuperClass::onMouseWheel (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInView::onFocus (const FocusEvent& event)
{
	if(plugView)
	{
		PlugInCallScope scope (*this);
		plugView->onViewEvent (event);
	}

	return SuperClass::onFocus (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInView::onKeyDown (const KeyEvent& event)
{
	CCL_PRINTLN ("PlugInView::onKeyDown")
	if(plugView)
	{
		PlugInCallScope scope (*this);
		return plugView->onViewEvent (event) != 0;
	}
	else
		return SuperClass::onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInView::onKeyUp (const KeyEvent& event)
{
	CCL_PRINTLN ("PlugInView::onKeyUp")
	if(plugView)
	{
		PlugInCallScope scope (*this);
		return plugView->onViewEvent (event) != 0;
	}
	else
		return SuperClass::onKeyUp (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInView::onActivate (bool state)
{
	//CCL_ADD_INDENT (2)
	//CCL_PRINTF ("%sPlugInView::onActivate (%s)\n", CCL_INDENT, state ? "true" : "false");

	SuperClass::onActivate (state);

	// forward activation to ChildWindow
	if(childWindow)
		childWindow->onActivate (state);

	if(state && getStyle ().isCustomStyle (Styles::kPlugInViewBehaviorFocus))
		takeFocus ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInView::delegateEvent (const GUIEvent& event)
{
	switch(event.eventClass)
	{
	case GUIEvent::kKeyEvent :
		switch(event.eventType)
		{
		case KeyEvent::kKeyDown :
			return onKeyDown (static_cast<const KeyEvent&> (event));

		case KeyEvent::kKeyUp :
			return onKeyUp (static_cast<const KeyEvent&> (event));
		}
		break;
	}
	return SuperClass::delegateEvent (event);
}
