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
// Filename    : ccl/gui/windows/window.cpp
// Description : Window class
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/nativewindow.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/transparentwindow.h"
#include "ccl/gui/popup/contextmenu.h"
#include "ccl/gui/popup/menu.h"
#include "ccl/gui/theme/themerenderer.h"
#include "ccl/gui/touch/touchinput.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/system/dragndrop.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/settings.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/views/sprite.h"
#include "ccl/gui/views/viewaccessibility.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/public/gui/iviewstate.h"
#include "ccl/public/math/mathprimitives.h"

namespace CCL {

//************************************************************************************************
// WindowSettings
//************************************************************************************************

class WindowSettings: public XmlSettings,
					  public ILayoutStateProvider
{
public:
	WindowSettings ()
	: XmlSettings (CCLSTR ("WindowState"))
	{}

	// ILayoutStateProvider
	IAttributeList* CCL_API getLayoutState (StringID id, tbool create = false) override
	{
		String path (String () << "Layout/" << id);
		if(Section* s = getSection (path, create != 0))
			return &s->getAttributes ();
		return nullptr;
	}

	CLASS_INTERFACE (ILayoutStateProvider, XmlSettings)
};

//////////////////////////////////////////////////////////////////////////////////////////////////

#if 1 && DEBUG_LOG
static void logFocus (Window* window, const char* text)
{
	View* focus = window->getFocusView ();
	View* saved = window->getSavedFocusView ();

	const char* str1 = focus ? focus->myClass ().getPersistentName () : "0";
	const char* str2 = saved ? saved->myClass ().getPersistentName () : "0";
	CCL_PRINTF ("%s%s (focusView: %s,  savedFocusView: %s)\n", CCL_INDENT, text, str1, str2);
}
#define LOG_FOCUS(window,text) logFocus (window, text);
#else
#define LOG_FOCUS(window,text)
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Default Window Styles
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Styles 
{
	StyleFlags defaultWindowStyle (0, kWindowCombinedStyleDefault);
	StyleFlags sizableWindowStyle (0, kWindowCombinedStyleSizable);
	StyleFlags panelWindowStyle (0, kWindowCombinedStylePanel);
	StyleFlags dialogWindowStyle (0, kWindowCombinedStyleDialog);
}

//************************************************************************************************
// WindowGraphicsDevice
//************************************************************************************************

WindowGraphicsDevice::WindowGraphicsDevice (Window& window, NativeGraphicsDevice* nativeDevice)
: window (window)
{
	if(nativeDevice)
		setNativeDevice (nativeDevice);
	else
	{
		// allocate device for painting asynchronously (not allowed on all platforms!)
		nativeDevice = ensureGraphicsDevice (NativeGraphicsEngine::instance ().createWindowDevice (&window));
		setNativeDevice (nativeDevice);
		nativeDevice->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowGraphicsDevice::~WindowGraphicsDevice ()
{
	window.setGraphicsDevice (nullptr);
}

//************************************************************************************************
// Window::ContextMenuBuilder
//************************************************************************************************

class Window::ContextMenuBuilder
{
public:
	ContextMenuBuilder (const Window& window, PointRef where, bool wasKeyPressed);

	void buildMenu ();
	void popup (Window& window);

	bool hasMenuItems () const;
	const ContextMenuEvent& getEvent () const;

private:
	AutoPtr<ContextMenu> contextMenu;
	ContextMenuEvent event;
	ObjectList views;
	LinkedList<IContextMenuHandler*> testedHandlers;
	View* deepestContributor;

	bool visitView (View* view);
};

//************************************************************************************************
// Window::ContextMenuBuilder
//************************************************************************************************

Window::ContextMenuBuilder::ContextMenuBuilder (const Window& window, PointRef where, bool wasKeyPressed)
: contextMenu (NEW ContextPopupMenu), // may be kept by script world!
  event (*contextMenu, where, wasKeyPressed),
  deepestContributor (nullptr)
{
	if(wasKeyPressed)
	{
		event.where (0, 0);
		if(View* focusView = window.getFocusView ())
			focusView->clientToWindow (event.where);
	}
	window.findAllViews (views, event.where, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ContextMenuEvent& Window::ContextMenuBuilder::getEvent () const
{
	return event;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::ContextMenuBuilder::buildMenu ()
{
	ListForEachObjectReverse (views, View, view)
		int itemsBefore = contextMenu->countItems ();

		bool result = visitView (view);

		int itemsNow = contextMenu->countItems ();
		if(itemsNow > itemsBefore)
			contextMenu->addSeparatorItem ();

		if(!deepestContributor && contextMenu->countItems () > 0 && !view->noFocusOnContextMenu ())
			deepestContributor = view;
		if(result)
			break;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::ContextMenuBuilder::hasMenuItems () const
{
	ListForEachObjectReverse (views, View, view)
		bool result = ccl_const_cast (this)->visitView (view);

		if(contextMenu->countItems () > 0)
			return true;
		if(result)
			break;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::ContextMenuBuilder::visitView (View* view)
{
	view->windowToClient (event.where);
	bool result = view->onContextMenu (event);
	view->clientToWindow (event.where);

	if(!result)
	{
		UnknownPtr<IContextMenuHandler> handler (view->getController ());
		if(handler && !testedHandlers.contains (handler))
		{
			result = handler->appendContextMenu (event.contextMenu) == kResultOk;
			testedHandlers.append (handler);
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::ContextMenuBuilder::popup (Window& window)
{
	if(deepestContributor)
		window.setFocusView (deepestContributor);

	contextMenu->popup (event.where, &window);
}

//************************************************************************************************
// Window
//************************************************************************************************

CCL_KERNEL_TERM_LEVEL (Window, kFrameworkLevelFirst)
{
	Window::flushSettings ();
	Window::cleanupSettings ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings* Window::windowSettings = nullptr;
Settings& Window::getWindowSettings ()
{
	if(windowSettings == nullptr)
	{
		windowSettings = NEW WindowSettings;
		windowSettings->isAutoSaveEnabled (true);
		windowSettings->isBackupEnabled (true);
		windowSettings->enableSignals (true);
		windowSettings->restore ();
	}
	return *windowSettings;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::cleanupSettings ()
{
	if(windowSettings)
	{
		windowSettings->release ();
		windowSettings = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::flushSettings ()
{
	if(windowSettings)
		windowSettings->flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (Window::windowStyles)
	{"floating",		Styles::kWindowBehaviorFloating},
	{"titlebar",		Styles::kWindowAppearanceTitleBar},
	{"sizable",			Styles::kWindowBehaviorSizable},
    {"maximize",		Styles::kWindowBehaviorMaximizable},
	{"topmost",			Styles::kWindowBehaviorTopMost},
	{"shadow",			Styles::kWindowAppearanceDropShadow},
	{"seethru",			Styles::kWindowBehaviorAutoSeeThru},
	{"center",			Styles::kWindowBehaviorCenter},
	{"inflate",			Styles::kWindowBehaviorInflate},
	{"restoresize",		Styles::kWindowBehaviorRestoreSize},
	{"restorepos",		Styles::kWindowBehaviorRestorePosition},
	{"restorecenter",	Styles::kWindowBehaviorRestoreCenter},
	{"customframe",		Styles::kWindowAppearanceCustomFrame},
	{"intermediate",	Styles::kWindowBehaviorIntermediate},
	{"pluginhost",		Styles::kWindowBehaviorPluginViewHost},
	{"fullscreen",		Styles::kWindowBehaviorFullscreen},
	{"sheetstyle",	    Styles::kWindowBehaviorSheetStyle},
	{"roundedcorners",	Styles::kWindowAppearanceRoundedCorners},
		
	// combined styles:
	{"windowstyle",		Styles::kWindowCombinedStyleDefault},
	{"panelstyle",		Styles::kWindowCombinedStylePanel},
	{"dialogstyle",		Styles::kWindowCombinedStyleDialog},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (Window, WindowBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

Window::Window (const Rect& size, StyleRef style, StringRef title)
: WindowBase (size, style, title),
  menuBar (nullptr),
  mouseHandler (nullptr),
  controller (nullptr),
  handle (nullptr),
  opacity (1.f),
  backgroundRenderer (nullptr),
  focusView (nullptr),
  savedFocusView (nullptr),
  inDrawEvent (false),
  inContextMenu (false),
  inMenuLoop (false),
  inMoveLoop (false),
  inCloseEvent (false),
  inDestroyEvent (false),
  collectUpdates (false),
#if CCL_DEBUG_INTERNAL
  inRedrawView (false),
#endif
  layer (kWindowLayerBase),
  renderTarget (nullptr),
  touchInputState (nullptr),
  windowMode (kWindowModeRegular)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window::~Window ()
{
	// destruct() has to be called by derived class while vtable points to it!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::destruct ()
{
	inDestroyEvent = true; // (if not already set on platform event)

	Desktop.removeWindow (this);

	setController (nullptr);
	setMouseHandler (nullptr);
	setFocusView (nullptr);
	saveFocusView (nullptr);

	delete touchInputState;

	// remove views while attached to this 
	removeAll ();

	ForEach (transparentWindows, TransparentWindow, w)
		w->release (); // dtor should remove it!
	EndFor
	ASSERT (transparentWindows.isEmpty () == true)

	cancelSignals ();

	if(menuBar)
		menuBar->release ();

	if(backgroundRenderer)
		backgroundRenderer->release ();

	//ASSERT (renderTarget == 0) // should be done by onDestroy() - no, not anymore.
	safe_release (renderTarget);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeWindowRenderTarget* Window::getRenderTarget ()
{
	if(!renderTarget)
	{
		ASSERT (!isInDestroyEvent ()) // avoid recreation
		renderTarget = NativeGraphicsEngine::instance ().createRenderTarget (this);
	}
	return renderTarget;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GraphicsDevice* Window::getGraphicsDevice (Point& offset)
{
	AutoPtr<GraphicsDevice> releaser;
	if(!graphicsDevice)
	{
		graphicsDevice = NEW WindowGraphicsDevice (*this);
		releaser = graphicsDevice;
	}

	// always call base class to add additional offset via DrawViewContext!
	return View::getGraphicsDevice (offset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::setMenuBar (MenuBar* newMenu)
{	
	MenuBar* oldMenu = menuBar;

	menuBar = newMenu;
	if(menuBar)
	{
		ASSERT (menuBar->window == nullptr)
		menuBar->window = this;
	}

	updateMenuBar ();

	if(oldMenu)
	{
		oldMenu->window = nullptr;
		oldMenu->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::finishMouseHandler (MouseEvent& event, bool canceled)
{
	if(mouseHandler)
	{		
		CCL_PRINTF ("Handler finished\n", 0)

		SharedPtr<MouseHandler> handler (mouseHandler);
		mouseHandler = nullptr; // reset pointer before releasing mouse capture (finishMouseHandler could be called again while in captureMouse)

		#if CCL_PLATFORM_WINDOWS
		captureMouse (false);
		#endif

		handler->finish (event, canceled);
		handler->onRelease (canceled);
		handler->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::setMouseHandler (MouseHandler* handler)
{
	// cancel old handler first
	MouseEvent me;
	finishMouseHandler (me, true);

	mouseHandler = handler;

	if(mouseHandler)
	{
		GUI.hideTooltip ();

		#if CCL_PLATFORM_WINDOWS
		captureMouse (true);
		#endif
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* Window::detachMouseHandler ()
{
	MouseHandler* handler = mouseHandler;
	mouseHandler = nullptr;
	return handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::captureMouse (bool state)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::canPopupContextMenu (PointRef where, bool wasKeyPressed) const
{
	return ContextMenuBuilder (*this, where, wasKeyPressed).hasMenuItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Window::popupContextMenu (PointRef _where, tbool wasKeyPressed)
{
	if(mouseHandler)
		return;

	GUI.hideTooltip ();

	ContextMenuBuilder builder (*this, _where, wasKeyPressed);
	builder.buildMenu ();

	GUI.trackUserInput (builder.getEvent ());

	ScopedVar<bool> scope (inContextMenu, true);
	builder.popup (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::saveFocusView (View* view)
{
	CCL_PRINTF ("%ssaveFocusView (%s)\n", CCL_INDENT, view ? view->myClass ().getPersistentName () : "0")
	CCL_ADD_INDENT (2)

	if(savedFocusView)
		savedFocusView->removeObserver (this);
	
	if(inDestroyEvent // don't save if beeing destroyed
		|| (view && !view->isAttached ())) // or if not attached anymore (e.g. in a VariantView)
		view = nullptr;

	savedFocusView = view;

	if(savedFocusView)
		savedFocusView->addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::setFocusView (View* view, bool directed)
{
	if(view != focusView)
	{
		CCL_PRINTF ("\n%s%s::setFocusView (%s)", CCL_INDENT, myClass ().getPersistentName (), view ? MutableCString (view->myClass ().getPersistentName ()).append (" ").append (view->getName ()).str () : "0")
		LOG_FOCUS (this, "   before:")
		CCL_ADD_INDENT (2)

		if(view == this)
			return false; // a window cannot be the focus view (endless recursion)!

		if(view && !view->isAttached ()) // might have been removed meanwhile (e.g. savedFocusView, in a VariantView)
			return false;

		if(privateFlags & kInSetFocus)
			return false; // prevent recursion

		ScopedFlag<kInSetFocus> guard (privateFlags);

		// when no view is focused, save the last focusView
		View* save = view ? nullptr : focusView;
		if(save != savedFocusView)
			saveFocusView (save);

		if(focusView)
		{
			// let parent WindowBase remember the focusView
			WindowBase* windowBase = focusView->getParent<WindowBase> ();
			if(windowBase)
				windowBase->setLastFocusView (focusView);

			focusView->onFocus (FocusEvent (FocusEvent::kKillFocus, directed));
			if(focusView) // might be re-entered!
				focusView->removeObserver (this);
		}
		
		focusView = view;
		LOG_FOCUS (this, "new focusview:")

		if(focusView)
		{
			// activate parent WindowBase
			WindowBase* windowBase = focusView->getParent<WindowBase> ();
			if(windowBase && !isInContextMenu ()) // prevent flicker when still inside context menu
			{
				windowBase->setLastFocusView (nullptr);
				windowBase->activate ();
			}

			if(focusView) // might be re-entered!
			{
				focusView->onFocus (FocusEvent (FocusEvent::kSetFocus, directed));
				if(focusView) // might be re-entered!
					focusView->addObserver (this);
			}
		}

		deferSignal (NEW Message (kFocusViewChanged));

		LOG_FOCUS (this, "end of setFocusView")
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::killFocusView (bool permanent)
{
	WindowBase* focusWindowBase = nullptr;
	if(focusView)
		if(WindowBase* windowBase = focusView->getParent<WindowBase> ())
			if(windowBase->getLastFocusView () != focusView)
				focusWindowBase = windowBase;

	setFocusView (nullptr);
	if(permanent)
		saveFocusView (nullptr);

	if(focusWindowBase)
		focusWindowBase->setLastFocusView (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* Window::getWindow ()
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* Window::getWindowForUpdate (WindowUpdateInfo& updateInfo)
{
	if(shouldCollectUpdates () || (renderTarget && renderTarget->shouldCollectUpdates ()))
		updateInfo.collectUpdates = true;
	else if(renderTarget)
		updateInfo.region = renderTarget->getUpdateRegion ();

	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::isAttached ()
{
	return handle != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Window::setSize (RectRef size, tbool invalidate)
{
	if(collectResize ())
	{
		resizeDeferred (true);
		SuperClass::setSize (size, invalidate);
	}
	else
	{
		if(size.isEmpty ())
			return;
		Rect s (size);
		setWindowSize (s); // may limit s to screen size
		SuperClass::setSize (s, invalidate);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::limitSizeToScreen (Rect& windowRect)
{
	Rect screen;
	Desktop.getVirtualScreenSize (screen, true);

	const SizeLimit& limits = getSizeLimits ();
	
	// limit window size to screen size, if sizeLimits allow it
	if(windowRect.getWidth () > screen.getWidth ())
		windowRect.setWidth (ccl_max (limits.minWidth, screen.getWidth ()));
	if(windowRect.getHeight () > screen.getHeight ())
		windowRect.setHeight (ccl_max (limits.minHeight, screen.getHeight ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::moveWindowRectInsideScreen (Rect& windowRect)
{
	if(Desktop.isRectVisible (windowRect))
		return;
		
	// move hidden window into center of nearest monitor
	int monitor = Desktop.findNearestMonitor (windowRect);	
	Rect monitorSize;
	Desktop.getMonitorSize (monitorSize, monitor, true);
	windowRect.center (monitorSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::onSize (const Point& delta)
{
	if(delta.x < 0)
	{
		// fix me!
	}
	if(delta.y < 0)
	{
		// fix me!
	}
	SuperClass::onSize (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::onChildSized (View* child, const Point& delta)
{
	CCL_PRINTF ("Window::onChildSized: %s (%d,%d)\n", child->myClass ().getPersistentName (), delta.x, delta.y)
	if(!isResizing ())
	{		
		if(views.count () == 1)
		{
			// resize by the same amount
			Rect rect (getSize ());
			rect.right += delta.x;
			rect.bottom += delta.y;

			disableSizeMode (true);
			setSize (rect);
			disableSizeMode (false);
		}

		// recalculate sizeLimits
		if((privateFlags & kExplicitSizeLimits) == 0)
		{
			privateFlags &= ~kSizeLimitsValid;
			getSizeLimits ();
			ASSERT (sizeLimits.maxWidth >= sizeLimits.minWidth)
			ASSERT (sizeLimits.maxHeight >= sizeLimits.minHeight)
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::onChildLimitsChanged (View* child)
{
	if(inDestroyEvent)
		return;

	CCL_PRINTF ("Window::onChildLimitsChanged (%s): H (%d, %d) window height: %d\n", child->myClass ().getPersistentName (), child->getSizeLimits ().minHeight, child->getSizeLimits ().maxHeight, getHeight ());

	SuperClass::onChildLimitsChanged (child);

	// check size limits deferred
	(NEW Message ("checkSizeLimits"))->post (this, -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::constrainSize (Rect& rect) const
{
	CCL_PRINTF ("Window::constrainSize: %d x %d (current: %d x %d)\n", rect.getWidth (), rect.getHeight (), getSize ().getWidth (), getSize ().getHeight ())
	CCL_ADD_INDENT (2)

	auto getDiagonalLength = [] (PointRef size) { return sqrtf (powf (size.x, 2.f) + powf (size.y, 2.f)); };

	Rect suggestedRect (rect);
	Point suggestedSize (suggestedRect.getSize ());

	SuperClass::constrainSize (rect);

	if(rect != suggestedRect && !resizeStartSize.isNull ())
	{
		Point currentSize (getSize ().getSize ());
		Point constrainedSize (rect.getSize ());

		float currentDiagonal = getDiagonalLength (currentSize);
		float suggestedDiagonal = getDiagonalLength (suggestedSize);
		float constrainedDiagonal = getDiagonalLength (constrainedSize);

		CCL_PRINTF ("        constrained 1: %d x %d \t-> %d x %d \t(diff: %d x %d)\n", suggestedSize.x, suggestedSize.y, constrainedSize.x, constrainedSize.y, constrainedSize.x - suggestedSize.x, constrainedSize.x - suggestedSize.y)

		if(constrainedSize == currentSize // stuck at previous size
			|| (constrainedDiagonal < currentDiagonal && suggestedDiagonal > currentDiagonal) // shrinked despite suggestion to grow
			|| resizeKeepRatio ())
		{
			// 2nd try to resolve a "locked" situation when the constrained size is the old window size again:
			// offer resizing to a rect with the same aspect ratio as resizeStartSize, based on the diagonal growth of the suggested rect
			Point delta (suggestedSize - currentSize);
			float startDiagonal = getDiagonalLength (resizeStartSize);
			if(!delta.isNull () && startDiagonal > 0)
			{
				float suggestedGrow = suggestedDiagonal / startDiagonal; // from resizeStartSize

				Rect scaledRect (suggestedRect);
				scaledRect.setSize (resizeStartSize * suggestedGrow); // same aspect ratio as resizeStartSize
				if(suggestedGrow > 1)
				{
					// avoid getting stuck when scaling has rounded down
					if(scaledRect.getWidth () == resizeStartSize.x)
						scaledRect.right++;
					if(scaledRect.getHeight () == resizeStartSize.y)
						scaledRect.bottom++;
				}

				Rect constrainedScaledRect (scaledRect);
				SuperClass::constrainSize (constrainedScaledRect);

				// choose the size that is closer to the suggested rect
				Point constrainedScaledSize (constrainedScaledRect.getSize ());
				float constrainedScaledDiagonal = getDiagonalLength (constrainedScaledSize);
				if(ccl_abs (constrainedScaledDiagonal - suggestedDiagonal) < ccl_abs (constrainedDiagonal - suggestedDiagonal))
				{
					rect = constrainedScaledRect;
					ccl_const_cast (this)->resizeKeepRatio (true); // once we choose this path, we must always try it to avoid jumps

					CCL_PRINTF ("        constrained 2: %d x %d \t-> %d x %d \t(diff: %d x %d)\n", scaledRect.getWidth (), scaledRect.getHeight (), constrainedScaledSize.x, constrainedScaledSize.y, constrainedScaledSize.x - scaledRect.getWidth (), constrainedScaledSize.y - scaledRect.getHeight ())
				}
			}
		}
		ccl_const_cast (this)->getSizeLimits ().makeValid (rect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::onResizing (bool begin)
{
	if(begin)
		resizeStartSize = getSize ().getSize ();
	else
	{
		resizeStartSize = Point ();
		resizeKeepRatio (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::setTitle (StringRef title)
{
	SuperClass::setTitle (title);
	setWindowTitle (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::addTransparentWindow (TransparentWindow* w)
{
	transparentWindows.add (w);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::removeTransparentWindow (TransparentWindow* w)
{
	transparentWindows.remove (w);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* Window::getTransparentWindows () const
{
	return transparentWindows.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TransparentWindow* Window::getFirstTransparentWindow () const
{
	return (TransparentWindow*)transparentWindows.getFirst ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Window::getZIndex () const
{
	return Desktop.getZIndex (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowLayer Window::getLayer () const
{
	return layer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::setLayer (WindowLayer _layer)
{
	layer = _layer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::redrawView (View* view)
{
	Rect r;
	view->getClientRect (r);
	redrawView (view, r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::redrawView (View* view, RectRef rect)
{
	#if CCL_DEBUG_INTERNAL
	ScopedVar<bool> scope (inRedrawView, true);
	#endif

	if(inDestroyEvent)
		return;

	Point p;
	Rect r (rect);
	view->clientToWindow (p);
	r.offset (p);
	
	GraphicsPort port (this);
	port.addClip (r); // need this to set the clipping correctly
	draw (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::draw (const UpdateRgn& updateRgn)
{
	hasBeenDrawn (true);

#if DEBUG_REDRAW
	CCL_PRINTF ("==================================================================\nUpdate %d %d %d %d\n", 
				 updateRgn.bounds.left, updateRgn.bounds.top, updateRgn.bounds.getWidth (), updateRgn.bounds.getHeight ())
#endif
	
	// *** Draw Background ***
	drawBackground (updateRgn);
	
	// *** Draw Content ***
	SuperClass::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::drawBackground (const UpdateRgn& updateRgn)
{
	// Note: Default behavior of a window if no visual style is assigned, is to draw the theme background.
	// If you want to avoid any overlain drawing, the transparent option has to be set.
	if(style.isOpaque ())
	{
		if(backgroundRenderer == nullptr)
			backgroundRenderer = getTheme ().createRenderer (ThemePainter::kBackgroundRenderer, visualStyle);
		if(backgroundRenderer)
			backgroundRenderer->draw (this, updateRgn);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Window::scrollClient (RectRef rect, PointRef delta)
{
    Rect r (rect);
    r.offset (delta);
    r.join (rect);
    invalidate (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGraphicsLayer* CCL_API Window::getParentLayer (Point& offset) const
{
	if(graphicsLayer == nullptr)
	{
		IGraphicsLayer* rootLayer = NativeGraphicsEngine::instance ().createGraphicsLayer (ClassID::RootLayer);
		if(rootLayer)
		{
			Window* This = const_cast<Window*> (this);
			rootLayer->construct (This->asUnknown (), Rect (), 0, getContentScaleFactor ());
			This->graphicsLayer = rootLayer;
		}
	}
	return graphicsLayer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::onDisplayPropertiesChanged (const DisplayChangedEvent& event)
{
	if(event.eventType == DisplayChangedEvent::kResolutionChanged)
	{
		NativeGraphicsLayer* rootLayer = unknown_cast<NativeGraphicsLayer> (graphicsLayer);
		ASSERT (!graphicsLayer || rootLayer)
		if(rootLayer)
		{
			rootLayer->setContentScaleFactorDeep (event.contentScaleFactor);
			rootLayer->flush ();
		}
	}

	SuperClass::onDisplayPropertiesChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	if(style.isOpaque ())
		if(getVisualStyle ().hasReferences (event.scheme))
		{
			safe_release (backgroundRenderer);
			updateBackgroundColor ();
		}
	
	SuperClass::onColorSchemeChanged (event);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::onActivate (bool state)
{
	CCL_PRINTF ("Window::%s (%s) %s\n", state ? "onActivate" : "onDEACTIVATE", myClass ().getPersistentName (), MutableCString (getTitle ()).str ())
	CCL_ADD_INDENT (2)
	
	Desktop.onActivateWindow (this, state);
	GUI.onActivateWindow (this, state);

	if(style.isCustomStyle (Styles::kWindowBehaviorAutoSeeThru))
		setOpacity (state ? 1.f : .8f);

	// notify event handlers
	WindowEvent activateEvent (*this, state ? WindowEvent::kActivate : WindowEvent::kDeactivate);
	signalWindowEvent (activateEvent);

	// don't deactivate subViews if deactivation was caused by a PopupSelector
	if(state == false && Desktop.isPopupActive ())
		return;

	SuperClass::onActivate (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::onClose ()
{
	ASSERT (inCloseEvent == false)

	bool isQuit = GUI.isQuitting () != 0; // handler can not stop application shutdown

	WindowEvent closeEvent (*this, WindowEvent::kClose);
	ListForEach (handlers, IWindowEventHandler*, handler)
		if(!handler->onWindowEvent (closeEvent) && !isQuit)
			return false;
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::onDestroy ()
{
	//safe_release (renderTarget); - no, might still be needed when views are being detached

	if(shouldStoreSize ())
		storeSize ();

	WindowEvent destroyEvent (*this, WindowEvent::kDestroy);
	signalWindowEvent (destroyEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API Window::getSystemWindow () const
{
	return handle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Window::isSystemWindowValid () const
{
	return handle != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Window::setCollectGraphicUpdates (tbool state)
{
	bool oldState = collectUpdates; 
	collectUpdates = state != 0;
	return oldState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Window::setCollectSizeUpdates (tbool state)
{
	bool oldState = collectResize ();
	collectResize (state != 0);
	if(collectResize () == false && resizeDeferred ())
	{
		resizeDeferred (false);
		disableSizeMode (true);
		setSize (getSize ());
		disableSizeMode (false);
	}
	return oldState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API Window::getWindowTitle () const
{
	return getTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::inflate ()
{
	Rect monitorSize;
	Desktop.getMonitorSize (monitorSize, Desktop.getMainMonitor (), true);

	Rect frameSize;
	getFrameSize (frameSize);
	Coord ncWidth = frameSize.getWidth () - size.getWidth ();
	Coord ncHeight = frameSize.getHeight () - size.getHeight ();

	// almost fill the main screen, but with some margin
	enum { kHMargin = 50, kVMargin = 20 };
	Rect r (monitorSize);
	r.left   += kHMargin;
	r.right  -= kHMargin;
	r.top    += kVMargin;
	r.bottom -= kVMargin;

	// translate to View coordinates
	r.right -= ncWidth;
	r.bottom -= ncHeight;

	getSizeLimits ();
	sizeLimits.makeValid (r);

	// center frame rect on screen
	r.right += ncWidth;
	r.bottom += ncHeight;
	r.center (monitorSize);
	r.right -= ncWidth;
	r.bottom -= ncHeight;
	setSize (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::cancelDragSession ()
{
	if(DragSession* dragSession = DragSession::getActiveSession ())
	{
		DragEvent dragEvent (*dragSession, DragEvent::kDragLeave);
		dragSession->setCanceled (true);
		onDragLeave (dragEvent);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::initSize ()
{
	sizeRestored (false);
	if(shouldStoreSize ())
		sizeRestored (restoreSize ());

	if(!sizeRestored ())
	{
		if(style.isCustomStyle (Styles::kWindowBehaviorInflate))
			inflate ();
		else if(style.isCustomStyle (Styles::kWindowBehaviorCenter))
			center ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Window::show ()
{
	initSize ();

	GUI.hideTooltip ();

	showWindow (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Window::hide ()
{
	showWindow (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Window::addHandler (IWindowEventHandler* handler)
{
	handlers.append (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Window::removeHandler (IWindowEventHandler* handler)
{
	handlers.remove (handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::signalWindowEvent (WindowEvent& windowEvent)
{
	ListForEach (handlers, IWindowEventHandler*, handler)
		handler->onWindowEvent (windowEvent);
	EndFor

	// notify controller without registration
	UnknownPtr<IWindowEventHandler> handler (getController ());
	if(handler && !handlers.contains (handler))
		handler->onWindowEvent (windowEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Window::getController () const
{
	return controller;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Window::setController (IUnknown* c)
{
	take_shared<IUnknown> (controller, c);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::deferActivate ()
{
	Message* m = NEW Message ("activate");
	m->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::deferClose ()
{
	Message* m = NEW Message ("close");
	m->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::addToDesktop ()
{
	Desktop.addWindow (this, getLayer ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "activate")
	{
		activate ();
	}
	else if(msg == "close")
	{
		close ();
	}
	else if(subject == focusView && msg == kDestroyed)
	{
		CCL_PRINTF ("%s focusView destroyed\n", CCL_INDENT)
		CCL_ADD_INDENT (2)
		setFocusView (nullptr);
		saveFocusView (nullptr);
	}
	else if(subject == savedFocusView && savedFocusView && msg == kDestroyed)
	{
		CCL_PRINTF ("%s savedFocusView destroyed\n", CCL_INDENT)
		CCL_ADD_INDENT (2)
		saveFocusView (nullptr);
	}
	else if (msg == "checkSizeLimits")
	{
		if(views.count () == 1)
		{
			View* child = getFirst ();
			if(child && hasExplicitSizeLimits ())
			{
				// set new explicit limits including the new child limits (-> limits only get stricter, never softer!)
				SizeLimit limits (getSizeLimits ());
				limits.include (child->getSizeLimits ());
				setSizeLimits (limits);
			}

			// enforce size limits
			Rect s (getSize ());
			Rect s2 (s);
			getSizeLimits ().makeValid (s);
			if(s != s2)
				setSize (s);
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::onMouseDown (const MouseEvent& event)
{
	beforeMouseDown (event);

	GUI.resetDoubleClick ();
	GUI.setLastKeyState (event.keys);
	GUI.trackUserInput (event);

	if(NonModalPopupSelectorWindow::processForeignEvent (event, this))
		return true;

	// kill focus if clicked outside...
	if(focusView)
	{
		Point offset;
		focusView->clientToWindow (offset);
		Point where (event.where);
		where.offset (-offset.x, -offset.y);

		if(!focusView->isInsideClient (where))
		{
			// but not if the clicked view ignores focus (e.g. scrollbar, divider)
			View* clickedView = findView (event.where, true);
			if(!clickedView || !clickedView->ignoresFocus ())
			{
				killFocusView ();
				saveFocusView (nullptr); // prevent coming back on window reactivation!
			}
		}
	}

	// try to find new focus...
	if(event.keys.isSet (KeyState::kLButton))
	{
		View* newFocus = findFocusView (event);
		if(newFocus && !newFocus->ignoresFocus ())
		{
			View* currentFocus = getFocusView ();
			if(!currentFocus || (currentFocus && newFocus != currentFocus /*&& !currentFocus->isChild (newFocus, true)*/))
				setFocusView (newFocus);
		}
	}

	return SuperClass::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::onMouseMove (const MouseEvent& event)
{
	GUI.setLastKeyState (event.keys);
	GUI.trackUserInput (event);

	if(mouseHandler)
	{
		MouseEvent e2 (event);
		mouseHandler->getView ()->windowToClient (e2.where);
		if(!mouseHandler->trigger (e2))
			setMouseHandler (nullptr);

		return true; // do not trigger mouse enter/leave while handler active
	}
	
	return GUI.onMouseMove (this, event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::onMouseUp (const MouseEvent& event)
{
	GUI.setLastKeyState (event.keys);
	GUI.trackUserInput (event);

	if(mouseHandler)
	{
        // keep mouse handler alive if it was started on a double click and the mouse button is still pressed
        // (ignore first mouseUp, delivered after second mouseDown)
        if(mouseHandler->hasStartedOnDoubleClick ())
        {
            KeyState keys;
            GUI.getKeyState (keys);
            if(keys.isSet (KeyState::kLButton))
                return false;
        }

        MouseEvent e2 (event);
		mouseHandler->getView ()->windowToClient (e2.where);
		finishMouseHandler (e2, false);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::onMouseWheel (const MouseWheelEvent& event)
{
	GUI.trackUserInput (event);
	return SuperClass::onMouseWheel (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchInputState& Window::getTouchInputState ()
{
	if(!touchInputState)
		touchInputState = NEW TouchInputState (*this);

	return *touchInputState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::onFocus (const FocusEvent& event)
{
	CCL_PRINTF ("Window::onFocus %s (%s)\n", event.eventType == FocusEvent::kSetFocus ? "setFocus" : "killFocus", myClass ().getPersistentName ())
	if(event.eventType == FocusEvent::kSetFocus)
	{
		if(savedFocusView)
		{
			CCL_PRINTF ("... restore focus view: %s\n", savedFocusView->myClass ().getPersistentName ());
			setFocusView (savedFocusView, false);
		}
	}
	else
	{
		// don't kill focus if caused by a PopupSelector
		if(Desktop.isPopupActive () && Desktop.getTopWindow (kPopupLayer) != this)
			return true;
		
		saveFocusView (nullptr);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::onKeyDown (const KeyEvent& event)
{
	GUI.setLastKeyState (event.state);
	GUI.trackUserInput (event);

	// 1) try global handler
	if(GUI.tryGlobal (this, event))
		return true;

	// 2) try active mouse handler (if any)...
	if(mouseHandler && mouseHandler->triggerKey (event))
		return true;

	// 3) try "raw" key event in focus view...
	if(focusView && (focusView != this) && focusView->onKeyDown (event))
		return true;

	// 4) try Key Commands... (not during MouseHandler!)
	if(!mouseHandler && GUI.translateKey (event, this))
		return true;

	tryMouseMove (event);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::onKeyUp (const KeyEvent& event)
{
	GUI.setLastKeyState (event.state);
	GUI.trackUserInput (event);

	// 1) try global handler
	if(GUI.tryGlobal (this, event))
		return true;

	// 2) try active mouse handler (if any)...
	if(mouseHandler && mouseHandler->triggerKey (event))
		return true;

	// 3) try "raw" key event in focus view...
	if(focusView && focusView->onKeyUp (event))
		return true;

	tryMouseMove (event);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::tryMouseMove (const KeyEvent& event)
{
	// Service views that are dependent on mouseMove & modifiers
	if(event.vKey == VKey::kShift || event.vKey == VKey::kCommand || 
	   event.vKey == VKey::kOption || event.vKey == VKey::kControl)
	{
		Point p;
		GUI.getMousePosition (p);
#if 1
		Window* window = this;

		// if the mouse is over the current mouseView in another window (on top of us): use that window
		if(View* mouseView = GUI.getMouseView ())
			if(Window* mouseWindow = unknown_cast<Window> (Desktop.findWindow (p)))
				if(mouseView->getWindow () == mouseWindow)
					window = mouseWindow;

		window->screenToClient (p);
		GUI.onMouseMove (window, MouseEvent (MouseEvent::kMouseMove, p, event.state), true); // force (even if not moved)

#else	// old behavior: mouseMove always to active window
		if(isInsideClient (p))
			GUI.onMouseMove (this, MouseEvent (MouseEvent::kMouseMove, p, event.state), true); // force (even if not moved)
		else
		{
			// if mouse is not in this window, try in the current mouseView
			if(View* mouseView = GUI.getMouseView ())
				if(mouseView->getWindow () != this)
				{
					GUI.getMousePosition (p);
					mouseView->screenToClient (p);
					if(mouseView->isInsideClient (p))
						mouseView->onMouseMove (MouseEvent (MouseEvent::kMouseMove, p, event.state));
				}
		}
#endif
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::tryMaximize (bool state)
{
	// get monitor size
	Rect monitorSize;
	int monitor = Desktop.findMonitor (getSize ().getCenter (), true);
	Desktop.getMonitorSize (monitorSize, monitor, true);

	// check if window can fill monitor
	const SizeLimit& limits = getSizeLimits ();
	bool canH = limits.maxWidth >= monitorSize.getWidth ();
	bool canV = limits.maxHeight >= monitorSize.getHeight ();
	if(canH && canV)
		maximize (state);
	else
	{
		// try to "maximize" at least in one direction
		Rect size (getSize ());
		size.setWidth (ccl_min (limits.maxWidth, monitorSize.getWidth ()));
		size.setHeight (ccl_min (limits.maxHeight, monitorSize.getHeight ()));
		if(canH)
			size.centerH (monitorSize);
		if(canV)
			size.centerV (monitorSize);
		setSize (size);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::shouldBeTranslucent () const
{
	// criteria for "platform-level translucency" (lets underlying content from other windows shine trough in not fully opaque areas)
	// note: independent from this, kTransparent might be used just as an optimization to avoid drawing the background, 
	// when the whole window is covered with opaque views (see Window::drawBackground).
	return getStyle ().isTranslucent ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::canReceiveDrag () const
{
	// when a modal window exists (includes PopupSelector), don't accept dragging into other windows
	if(IWindow* topDialog = Desktop.getTopWindow (kDialogLayer))
		return topDialog == this;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::onDragEnter (const DragEvent& event)
{
	if(!canReceiveDrag ())
		return false;

	activate ();
	return GUI.onDragEvent (this, event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::onDragOver (const DragEvent& event)
{
	if(!canReceiveDrag ())
		return false;

	return GUI.onDragEvent (this, event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::onDragLeave (const DragEvent& event)
{
	return GUI.onDragEvent (this, event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::onDrop (const DragEvent& event)
{
	if(!canReceiveDrag ())
		return false;

	return GUI.onDragEvent (this, event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API Window::clientToWindow (Point& p) const 
{ 
	return p; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API Window::clientToScreen (Point& p) const
{
	return p;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::finishScroll (RectRef rect, PointRef delta)
{
	Rect r1, r2;
	if(delta.y < 0)
		r1 (rect.left, rect.bottom + delta.y, rect.right, rect.bottom);
	else
		r1 (rect.left, rect.top, rect.right, rect.top + delta.y);

	if(delta.x < 0)
		r2 (rect.right + delta.x, rect.top, rect.right, rect.bottom);
	else
		r2 (rect.left, rect.top, rect.left + delta.x, rect.bottom);
		
	if(!r1.isEmpty ())
		invalidate (r1);
	if(!r2.isEmpty ())
		invalidate (r2);
	
	#if DEBUG_LOG
	if(!r1.isEmpty ())
		Debugger::printf ("finishScroll: invalidate: x: %d, %d,  y: %d, %d\n", r1.left, r1.right, r1.top, r1.bottom);
	if(!r2.isEmpty ())
		Debugger::printf ("finishScroll: invalidate: x: %d, %d,  y: %d, %d\n", r2.left, r2.right, r2.top, r2.bottom);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef Window::getSettingsID (String& id) const
{
	id = CCLSTR ("WindowState");
	id.append (CCLSTR ("/"));
	if(!getName ().isEmpty ())
		id.append (getName ());
	else
		id.append (myClass ().getPersistentName ());
	return id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::shouldStoreSize () const
{
	return	style.isCustomStyle (Styles::kWindowBehaviorRestoreSize) ||
			style.isCustomStyle (Styles::kWindowBehaviorRestorePosition) ||
			style.isCustomStyle (Styles::kWindowBehaviorRestoreCenter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Window::restoreSize ()
{
	if(style.isCustomStyle (Styles::kWindowPrivateFlagRestored))
		return true;

	style.setCustomStyle (Styles::kWindowPrivateFlagRestored);

	String settingsID;
	getSettingsID (settingsID);
	Attributes& a = getWindowSettings ().getAttributes (settingsID);
	
	int left = a.getInt ("size.left");
	int top = a.getInt ("size.top");
	int width = a.getInt ("size.width");
	int height = a.getInt ("size.height");
	
	Rect r (left, top, left + width, top + height);
	if(r.isEmpty ())
		return false;

	getSizeLimits ();
	sizeLimits.makeValid (r);

	if(style.isCustomStyle (Styles::kWindowBehaviorRestoreSize))
	{
		bool maximized = a.getBool ("maximized");
		if(maximized)
		{
			setUserSize (r);
			maximize (true);
		}
		else
			setSize (r);
		
		updateClient ();
	}
	else if(style.isCustomStyle (Styles::kWindowBehaviorRestorePosition))
	{
		setPosition (r.getLeftTop ());
	}
	else if(style.isCustomStyle (Styles::kWindowBehaviorRestoreCenter))
	{
		Point p;
		p.x = left + width/2 - getWidth ()/2;
		p.y = top + height/2 - getHeight ()/2;
		setPosition (p);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::storeSize ()
{
	String settingsID;
	getSettingsID (settingsID);
	Attributes& a = getWindowSettings ().getAttributes (settingsID);

	Rect r;
	if(isMaximized ())
		getUserSize (r);
	else
		r = getSize ();

	a.set ("size.left", r.left);
	a.set ("size.top", r.top);
	a.set ("size.width", r.getWidth ());
	a.set ("size.height", r.getHeight ());

	a.set ("maximized", isMaximized ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::onVisualStyleChanged ()
{
	SuperClass::onVisualStyleChanged ();

	updateBackgroundColor ();
	safe_release (backgroundRenderer); // e.g. on mac, Window::draw is called already during constructor (setWindowSize -> RenderTarget::onSize), before a visual style is set
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Window)
	DEFINE_METHOD_NAME ("popupContextMenu")
	DEFINE_METHOD_NAME ("showPlatformInformation")
	DEFINE_METHOD_NAME ("close")	
END_METHOD_NAMES (Window)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Window::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "popupContextMenu")
	{
		Point where;

		// e.g. TriggerView passed by MethodInvoker (initialTarget)
		UnknownPtr<IView> view (msg[0].asUnknown ());
		if(view)
			view->clientToWindow (where);

		popupContextMenu (where);
		return true;
	}
	else if(msg == "showPlatformInformation")
	{
		showPlatformInformation ();
		return true;
	}
	else if(msg == "close")	
	{
		deferClose ();
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CCL::Window::setContentScaleFactor (float factor)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* Window::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW RootViewAccessibilityProvider (*this);
	return accessibilityProvider;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Default implementation for platform-specific methods
//////////////////////////////////////////////////////////////////////////////////////////////////

void Window::updateMenuBar () {}
void Window::setWindowSize (Rect& size) {}
void CCL_API Window::setWindowTitle (StringRef title) {}
void CCL_API Window::moveWindow (PointRef pos) {}
void CCL_API Window::invalidate (RectRef rect) {}
void Window::showWindow (bool state) {}
void CCL_API Window::maximize (tbool state) {}
tbool CCL_API Window::isMaximized () const { return false; }
tbool CCL_API Window::isMinimized () const { return false; }
void CCL_API Window::setUserSize (RectRef size) {}
void CCL_API Window::getUserSize (Rect& size) const { ASSERT (0) }
tbool CCL_API Window::isVisible () const { return true; }
void CCL_API Window::center () {}
void CCL_API Window::redraw () { ASSERT (0) }
void CCL_API Window::activate () {}
tbool CCL_API Window::isActive () const { return true; }
tbool CCL_API Window::close () { ASSERT (0) return false; }
void Window::updateSize () {}
void CCL_API Window::getFrameSize (Rect& size) const { getClientRect (size); }
void Window::moveWindow () { ASSERT(0) }
void Window::resizeWindow (int edge) { ASSERT(0) }
void Window::updateBackgroundColor () {}
bool Window::setOpacity (float opacity) { return false; }
void Window::beforeMouseDown (const MouseEvent& event) {}
float CCL_API Window::getContentScaleFactor () const  { ASSERT (0) return 1.f; }
tbool CCL_API Window::setFullscreen (tbool state) { return false; }
tbool CCL_API Window::isFullscreen () const { return false; }
EventResult Window::handleEvent (SystemEvent& e) { ASSERT(0) return nullptr; }
void Window::fromNativeWindow (void* nativeWindow) { ASSERT (0) }
void Window::makeNativePopupWindow (IWindow* parent) { ASSERT (0) }
void Window::makeNativeChildWindow (void* nativeParent) { ASSERT (0) }
void Window::showPlatformInformation () {}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL
