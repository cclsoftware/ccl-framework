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
// Filename    : ccl/gui/layout/workspaceframes.cpp
// Description : Workspace FrameItem classes
//
//************************************************************************************************

#define DEBUG_LOG 0
#define DEBUG_DRAW_ID (0 && DEBUG)
#define DEBUG_DRAW_ACTIVE (0 && DEBUG)
#define DEBUG_DRAW_HELPID (0 && DEBUG)
#define DEBUG_DRAW (DEBUG_DRAW_ID || DEBUG_DRAW_ACTIVE || DEBUG_DRAW_HELPID)
#define DEBUG_MULTI_DETACHED 0
#define DEBUG_PROFILE 0

#include "ccl/gui/layout/workspaceframes.h"
#include "ccl/gui/layout/workspace.h"
#include "ccl/gui/layout/divider.h"
#include "ccl/gui/layout/boxlayout.h"
#include "ccl/gui/layout/layoutprimitives.h"

#include "ccl/gui/windows/window.h"
#include "ccl/gui/windows/windowmanager.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/views/viewanimation.h"
#include "ccl/gui/controls/pluginview.h"
#include "ccl/gui/skin/skinexpression.h"
#include "ccl/gui/skin/form.h"
#include "ccl/gui/gui.h"

#include "ccl/app/paramalias.h"

#include "ccl/base/boxedtypes.h"
#include "ccl/base/trigger.h"
#include "ccl/base/storage/storage.h"

#include "ccl/public/gui/iapplication.h"
#include "ccl/public/gui/iviewstate.h"
#include "ccl/public/base/irecognizer.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// FrameView
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (FrameView, WindowBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

Form* FrameView::findContentForm (const View& outerView)
{
	AutoPtr<IRecognizer> recognizer (Recognizer::create ([&] (IUnknown* obj)
	{
		auto contentForm = unknown_cast<Form> (obj);
		return contentForm && contentForm->getName () == "Content";
	}));
	return ccl_cast<Form> (outerView.findView (*recognizer));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameView::FrameView (FrameItem* frameItem, const Rect& size)
: WindowBase (size),
  frameItem (frameItem),
  frameWidth (-1)
{
	setName (frameItem->getName ()); // for cclspy
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem* FrameView::getFrameItem () const
{
	return frameItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameView::setContent (View* view)
{
	ASSERT (isEmpty ())
	ASSERT (view->getSize () == Rect (0,0,getSize ().getSize ()))

	addView (view);

	int sizeMode = view->getSizeMode ();
	if(frameItem->isFill ())
		sizeMode |= IView::kFill;
	setSizeMode (sizeMode);

	style.setCommonStyle (Styles::kTransparent, view->getStyle ().isTransparent ());
	setTitle (view->getTitle ());

	String helpId;
	if(auto contentForm = findContentForm (*view))
		helpId = contentForm->getHelpIdentifier ();
	else
		helpId = findHelpIdentifierDeep (*view);
	 setHelpIdentifier (helpId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef FrameView::findHelpIdentifierDeep (View& view)
{
	StringRef helpId = view.getHelpIdentifier ();
	if(helpId.isEmpty ())
	{
		// continue deep if there is only a single child; helpful (pun intended) when the actual content view is deeper inside a "decor" frame
		View* firstChild = view.getFirst ();
		if(firstChild && firstChild == view.getLast ())
			return findHelpIdentifierDeep (*firstChild);
	}
	return helpId;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* CCL_API FrameView::getLayoutState (StringID id, tbool create)
{
	// instances of a multiframe store their states in the name of the multiframe
	FrameItem* frame = frameItem;
	if(MultiFrameItem* multiFrame = ccl_cast<MultiFrameItem> (frame->getParentItem ()))
		frame = multiFrame;

	return frame->getLayoutState (id, create != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameView::calcSizeLimits ()
{
	// limits of our only child
	if(View* view = getFirst ())
	{
		const SizeLimit& childLimits = view->getSizeLimits ();
		sizeLimits = childLimits;
	}
	else
		sizeLimits.setFixed (Point (0, 0));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameView::onChildLimitsChanged (View* child)
{
	resetSizeLimits (); // we set explicit limits in FrameItem::show () -> forceSize

	SuperClass::onChildLimitsChanged (child);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameView::canActivate () const
{
	return !frameItem->isNoActivate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameView::onMouseDown (const MouseEvent& event)
{
	if(!frameItem->getFriendID ().isEmpty ())
		if(FrameItem* friendItem = frameItem->getPerspective ()->findFrameByID (frameItem->getFriendID ()))
			if(WindowBase* friendView = ccl_cast<WindowBase> (friendItem->getView ()))
				friendView->activate ();

	return SuperClass::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameView::onActivate (bool state)
{
	SuperClass::onActivate (state);

	if(frameItem->isFocusFrame ())
	{
		updateStyle ();

		Rect r;
		getClientRect (r);

		auto invalidateEdge = [&] (Coord& coord, Coord value) 
		{
			ScopedVar<Coord> g (coord, value);
			invalidate (r);
		};

		invalidateEdge (r.right, r.left + frameWidth);
		invalidateEdge (r.left, r.right - frameWidth);
		invalidateEdge (r.bottom, r.top + frameWidth);
		invalidateEdge (r.top, r.bottom - frameWidth);
	}

	#if DEBUG_DRAW
	Rect r;
	invalidate (getClientRect (r));
	#endif

	signalOnActivate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameView::signalOnActivate ()
{
	// let content view send trigger message
	if(View* view = getFirst ())
		view->signal (Message ("onActivate", isActive ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameView::draw (const UpdateRgn& updateRgn)
{
	SuperClass::draw (updateRgn);

	if(isActive () && frameItem->isFocusFrame ())
	{
		updateStyle ();

		Rect rect;
		getClientRect (rect);

		GraphicsPort port (this);
		port.drawRect (rect, Pen (frameColor, float(frameWidth)));
	}

#if DEBUG_DRAW
	GraphicsPort port (this);
	Rect rect;
	getClientRect (rect);

	#if DEBUG_DRAW_ACTIVE
	Color c (Colors::kGreen);
	if(isActive ())
	{
		c.setAlphaF (0.1f);
		port.fillRect (rect, SolidBrush  (c));
	}
	c.setAlphaF (0.5f);
	port.drawRect (rect, Pen (c, 3.f));
	#endif

	#if DEBUG_DRAW_ID
	String id (frameItem->getWindowID ());
	port.drawString (rect, id, getVisualStyle ().getTextFont (), SolidBrush (Colors::kGreen), Alignment (Alignment::kLeftTop));
	rect.offset (port.getStringWidth (id, getVisualStyle ().getTextFont ()));
	port.drawString (rect, String (" (") << frameItem->getName () << ")", getVisualStyle ().getTextFont (), SolidBrush (Colors::kYellow), Alignment (Alignment::kLeftTop));
	#endif

	#if DEBUG_DRAW_HELPID
	if(!getHelpIdentifier ().isEmpty ())
		port.drawString (rect, getHelpIdentifier (), getVisualStyle ().getTextFont (), SolidBrush (Colors::kYellow), Alignment (Alignment::kCenter));
	#endif
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameView::updateStyle ()
{
	if(frameWidth < 0)
	{
		FrameItem* root = frameItem->getRootFrame ();
		IView* view = root ? root->getView () : nullptr;	// LayoutView
		view = view ? view->getParentView () : nullptr;	// DockPanelView
		view = view ? view->getParentView () : nullptr;	// PerspectiveContainer
		if(view)
		{
			const IVisualStyle& vs = view->getVisualStyle ();
			frameColor = vs.getColor ("framecolor", getTheme ().getThemeColor (ThemeElements::kAlphaSelectionColor));
			frameWidth = (Coord)vs.getStrokeWidth ();
		}
	}
}

//************************************************************************************************
// EmbeddedFrameView
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (EmbeddedFrameView, View)

//////////////////////////////////////////////////////////////////////////////////////////////////

EmbeddedFrameView::EmbeddedFrameView (const Rect& size)
: View (size),
  transitionType (Styles::kTransitionNone)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EmbeddedFrameView::attached (View* parent)
{
	SuperClass::attached (parent);

	// find and connect to the frame item
	ASSERT (frameItem == nullptr)
	if(Workspace* workspace = unknown_cast<Workspace> (WorkspaceSystem::instance ().getWorkspace (workspaceID)))
	{
		frameItem = ccl_cast<EmbeddedFrameItem> (workspace->getCurrentPerspective ()->findFrameByID (getFrameID ()));
		ASSERT (frameItem != nullptr)
		if(frameItem)
			frameItem->onFrameViewAttached (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EmbeddedFrameView::removed (View* parent)
{
	// notify frame item
	if(frameItem)
	{
		frameItem->onFrameViewRemoved ();
		frameItem = nullptr;
	}

	removeAll (); // avoid dangling subViews causing trouble when attached again (e.g. inside a VariantView)
	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EmbeddedFrameView::calcSizeLimits ()
{
	if(View* view = getFirst ())
	{
		const SizeLimit& childLimits = view->getSizeLimits ();
		sizeLimits = childLimits;
	}
	else
		sizeLimits.setFixed (Point (0, 0));
}

//************************************************************************************************
// FrameItem::ViewState
//************************************************************************************************

DEFINE_CLASS (FrameItem::ViewState, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem::ViewState::ViewState (StringID windowID)
: windowID (windowID),
  flags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem::ViewState::ViewState (const ViewState& other)
: windowID (other.windowID),
  size (other.size),
  pos (other.pos),
  flags (other.flags)
{
	 setViewState (other.getViewState ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameItem::ViewState::load (const Storage& storage)
{
	Attributes& attribs (storage.getAttributes ());
	attribs.get (windowID, "windowID");
	size.x = attribs.getInt ("W");
	size.y = attribs.getInt ("H");
	pos.x = attribs.getInt ("X");
	pos.y = attribs.getInt ("Y");
	flags = attribs.getInt ("flags");
	viewState.share (attribs.getObject<Attributes> ("viewState"));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameItem::ViewState::save (const Storage& storage) const
{
	Attributes& attribs (storage.getAttributes ());
	attribs.set ("windowID", windowID);
	attribs.set ("W", size.x);
	attribs.set ("H", size.y);
	attribs.set ("X", pos.x);
	attribs.set ("Y", pos.y);
	attribs.set ("flags", flags);
	if(viewState)
		attribs.set ("viewState", viewState, Attributes::kShare);
	return true;
}

//************************************************************************************************
// FrameItem
//************************************************************************************************

BEGIN_STYLEDEF (FrameItem::customStyles)
	{"dividers", FrameItem::kDividers},
	{"popup",	 FrameItem::kPopup},
	{"multiple", FrameItem::kMultiple},
	{"pinnable", FrameItem::kPinnable},
	{"detached", FrameItem::kDetached},
	{"fill",	 FrameItem::kFill},
	{"required", FrameItem::kRequired},
	{"noactivate", FrameItem::kNoActivate},
	{"othermonitor", FrameItem::kOtherMonitor},
	{"maximize",	FrameItem::kMaximizable},
	{"fullscreen",	FrameItem::kFullscreen},
	{"focusframe",  FrameItem::kFocusFrame},
	{"system",		FrameItem::kSystem},
	{"volatile",	FrameItem::kVolatile},
	{"shared",		FrameItem::kShared},
	{"horizontal",	FrameItem::kHorizontal},
	{"vertical",	FrameItem::kVertical},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

enum { 
	kPinnedTag = 200, 
	kMaximizedTag,
	kFullscreenTag,
	kCloseTag,
	kDetachedTag
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (FrameItem, DockPanelItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem* FrameItem::createItem (int style)
{
	if(get_flag<int> (style, FrameItem::kMultiple))
		return NEW MultiFrameItem (style);

	else if(get_flag (style, FrameItem::kVertical|FrameItem::kHorizontal))
		return NEW FrameGroupItem (style);

	else if(get_flag<int> (style, FrameItem::kDetached))
	{
		if(get_flag<int> (style, FrameItem::kShared))
			return NEW SharedDetachedFrameItem (style);
		else
			return NEW DetachedFrameItem (style);
	}
	else if(get_flag<int> (style, FrameItem::kPopup))
		return NEW PopupFrameItem (style);

	else if(get_flag<int> (style, FrameItem::kSystem))
		return NEW SystemFrameItem (style);

	return NEW FrameItem (style);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem::FrameItem (int style)
: style (style),
  windowClass (nullptr),
  fillFactor (0),
  closeParam (nullptr),
  titleParam (nullptr),
  restoringView (false)
{
	groupIDs.objectCleanup (true);
	viewStates.objectCleanup (true);
	isHidable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem::FrameItem (const FrameItem& item)
: DockPanelItem (item),
  style (item.style),
  windowClass (item.windowClass),
  windowID (item.windowID),
  defaultWindowID (item.defaultWindowID),
  condition (item.condition),
  decor (item.decor),
  friendID (item.friendID),
  fillFactor (item.fillFactor),
  closeParam (nullptr),
  titleParam (nullptr),
  restoringView (false)
{
	if(windowClass)
		windowClass->retain ();

	groupIDs.objectCleanup (true);
	ForEach (item.groupIDs, Object, id)
		groupIDs.add (id->clone ());
	EndFor

	viewStates.objectCleanup (true);
	ForEach (item.viewStates, ViewState, state)
		viewStates.add (NEW ViewState (*state));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem::~FrameItem ()
{
	cancelSignals ();
	safe_release (windowClass);
	safe_release (closeParam);
	safe_release (titleParam);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FrameItem::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "isWindow")
	{
		var = false;
		return true;
	}
	else if(propertyId == "canMaximize")
	{
		var = isMaximizable ();
		return true;
	}
	else if(propertyId == "canFullscreen")
	{
		var = isFullscreen ();
		return true;
	}
	else if(propertyId == "isPinnable")
	{
		var = isPinnable ();
		return true;
	}
	else if(propertyId == "isDetachedFrame")
	{
		var = isDetachedFrame ();
		return true;
	}
	else if(propertyId == "perspective")
	{
		Perspective* perspective = getPerspective ();
		var = perspective ? perspective->getID () : nullptr;
		return true;
	}
	else if(propertyId == "parent")
	{
		if(DockPanelItem* parent = getParentItem ())
			var = ccl_as_unknown (parent);
		return true;
	}
	else if(propertyId == "embeddedFrame")
	{
		const DockPanelItem* parent = this;
		while(parent = parent->getParentItem ())
			if(EmbeddedFrameItem* embeddedFrame = ccl_cast<EmbeddedFrameItem> (ccl_const_cast (parent)))
			{
				var = embeddedFrame->asUnknown ();
				return true;
			}
	}
	else if(propertyId == "url")
	{
		String frameUrl;
		WorkspaceSystem::makeFrameUrl (frameUrl, *this);
		var = Variant (frameUrl, true);
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API FrameItem::findParameter (StringID name) const
{
	if(name == "pinned")
		return const_cast<FrameItem*> (this)->getPinnedParam ();
	if(name == "close")
		return const_cast<FrameItem*> (this)->getCloseParam ();
	if(name == "title")
		return const_cast<FrameItem*> (this)->getTitleParam ();
	if(name == "detached")
	{
		StringRef groupID = windowClass ? windowClass->getGroupID () : String::kEmpty;
		AutoPtr<IRecognizer> recognizer (Recognizer::create ([&] (IUnknown* obj)
			{
				if(FrameItem* item = unknown_cast<FrameItem> (obj))
				{
					// looking for a detached frame that accepts the given group (ignore pinned frames)
					DetachedFrameItem* detachedFrame = ccl_cast<DetachedFrameItem> (item);
					if(detachedFrame && !detachedFrame->isPinned () && detachedFrame->hasGroupID (groupID))
						return true;

					MultiFrameItem* multiFrame = ccl_cast<MultiFrameItem> (item);
					if(multiFrame && multiFrame->isDetachedFrame () && multiFrame->hasGroupID (groupID))
						if(multiFrame->countChildren () == 0)
							multiFrame->newChildItem (); // ensure a detached frame exists (will be visited next as child)
				}
				return false;
			}));

		// find a detached frame that can accept the current window class
		if(Perspective* perspective = getPerspective ())
			if(FrameItem* detachedFrame = perspective->findFrameItem (*recognizer))
				return detachedFrame->findParameter ("detached");
	}

	return SuperClass::findParameter (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* FrameItem::getCloseParam ()
{
	if(!closeParam)
	{
		closeParam = NEW Parameter ("close");
		closeParam->connect (this, kCloseTag);
	}
	return closeParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* FrameItem::getTitleParam ()
{
	if(!titleParam)
	{
		titleParam = NEW StringParam;
		titleParam->setValue (getContentTitle ());
	}
	return titleParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String FrameItem::getContentTitle () const
{
	if(view && !view->getTitle ().isEmpty ())
		return view->getTitle ();

	if(windowClass)
		return windowClass->getTitle ();

	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameItem::isPinned () const
{
	Parameter* p = const_cast<FrameItem*> (this)->getPinnedParam ();
	return p && p->getValue ().asBool ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::setPinned (bool state)
{
	if(Parameter* p = const_cast<FrameItem*> (this)->getPinnedParam ())
		p->setValue (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameItem::wasPinned (StringID windowID) const
{
	const WindowClass* wc = WindowManager::instance ().getClass (windowID);
	ViewState* viewState = wc ? ccl_const_cast (this)->lookupViewState (*wc, false, false) : nullptr;
	return viewState && viewState->isPinned ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* FrameItem::getPinnedParam ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::addGroupID (StringRef id)
{
	groupIDs.add (NEW Boxed::String (id));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameItem::hasGroupID (StringRef id)
{
	ForEach (groupIDs, Boxed::String, groupID)
		if(*groupID == id)
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const WindowClass* FrameItem::getCurrentWindowClass ()
{
	return windowClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* FrameItem::getViewController () const
{
	View* v = getView ();
	if(FrameView* frameView = ccl_cast<FrameView> (v))
		v = frameView->getChild (0);

	return v ? v->getController () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RootFrameItem* FrameItem::getRootFrame () const
{
	return unknown_cast<RootFrameItem> (getRoot ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Perspective* FrameItem::getPerspective () const
{
	RootFrameItem* rootFrame = getRootFrame ();
	return rootFrame ? rootFrame->getPerspective () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Workspace* FrameItem::getWorkspace () const
{
	Perspective* perspective = getPerspective ();
	return perspective ? perspective->getWorkspace () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem* FrameItem::findChildFrame (IRecognizer& recognizer)
{
	ForEach (getChildren (), DockPanelItem, item)
		FrameItem* frameItem = ccl_cast<FrameItem> (item);
		if(frameItem)
		{
			if(recognizer.recognize (frameItem->asUnknown ()))
				return frameItem;

			frameItem = frameItem->findChildFrame (recognizer); // recursion
			if(frameItem)
				return frameItem;
		}
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::collectChildFrames (Container& container, IObjectFilter& filter)
{
	ForEach (getChildren (), DockPanelItem, item)
		if(FrameItem* frameItem = ccl_cast<FrameItem> (item))
		{
			if(filter.matches (frameItem->asUnknown ()))
				container.add (frameItem);

			frameItem->collectChildFrames (container, filter);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* FrameItem::getLayoutState (StringID id, bool create)
{
	// build path: "windowID/FrameName/id"
	String path (getWindowID ());
	path << '/' << getName ();
	path << '/' << id;

	if(Perspective* perspective = getPerspective ())
		return perspective->getLayoutState (path, create);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectList& FrameItem::getViewStates ()
{
	// instances of a multiframe store sizes in the multiframe
	if(MultiFrameItem* multiFrameParent = ccl_cast<MultiFrameItem> (getParentItem ()))
		return multiFrameParent->getViewStates ();

	return viewStates;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem::ViewState* FrameItem::lookupViewState (WindowClassRef windowClass, bool create, bool mayUseDefault)
{
	return lookupViewState (windowClass.getViewStateID (), create, mayUseDefault);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem::ViewState* FrameItem::lookupViewState (StringID viewStateID, bool create, bool mayUseDefault)
{
	ListForEachObject (getViewStates (), ViewState, state)
		if(state->getWindowID () == viewStateID)
			return state;
	EndFor

	if(mayUseDefault)
	{
		// default frame size from xml description has no windowID (any)
		ListForEachObject (getViewStates (), ViewState, state)
			if(state->getWindowID ().isEmpty ())
				return state;
		EndFor
	}

	ViewState* state = nullptr;
	if(create)
		getViewStates ().add (state = NEW ViewState (viewStateID));
	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::initViewState (StringID windowID, StringID attribID, VariantRef value)
{
	if(const WindowClass* wc = WindowManager::instance ().getClass (windowID))
	{
		ViewState* viewState = lookupViewState (*wc, true);
		if(!viewState->getViewState ()) // only "init" if no attribute was stored before (e.g. not when "cloning" a document from a template)
		{
			AutoPtr<Attributes> attributes (NEW Attributes);
			attributes->setAttribute (attribID, value);

			viewState->setViewState (attributes);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameItem::getSavedSize (Point& size)
{
	ASSERT (windowClass)
	if(!isFill () && windowClass) // don't save size of "fill" frames
		if(ViewState* state = lookupViewState (*windowClass, false, true))
		{
			size = state->getSize ();
			return !size.isNull ();
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::saveSize (PointRef size)
{
	const WindowClass* wc = windowClass ? windowClass : WindowManager::instance ().getClass (windowID);
	if(wc)
		lookupViewState (*wc, true)->setSize (size);
	else
		lookupViewState (windowID, true)->setSize (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameItem::saveViewState ()
{
	View* view = getView ();
	if(view && windowClass)
		if(ViewState* state = lookupViewState (*windowClass, true))
		{
			saveViewStateInternal (*state);
			return true;
		}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::saveViewStateInternal (ViewState& state)
{
	CCL_PRINTF ("saveViewState: %s: %s (%d, %d) pinned: %d\n", MutableCString (getName ()).str (), windowID.str (), view->getSize ().getWidth (), view->getSize ().getHeight (), isPinned ())

	// save size, position, pinned
	state.setSize (view->getSize ().getSize ());
	state.setPosition (getPosition ());
	state.isPinned (isPinned ());

	// save controller viewState
	UnknownPtr<IViewStateHandler> viewStateHandler (getViewController ());
	if(viewStateHandler)
	{
		AutoPtr<Attributes> attributes (NEW Attributes);
		if(viewStateHandler->saveViewState (windowID, MutableCString (getName ()), *attributes, &state))
		{
			state.setViewState (attributes);
			return;
		}
	}
	state.setViewState (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::restoreView ()
{
	if(!windowID.isEmpty () || !getDefaultWindowID ().isEmpty ())
	{
		const WindowClass* wc = nullptr;
		if(!windowID.isEmpty ())
			wc = WindowManager::instance ().getClass (windowID);

		// fallback to default window class if frame is "required" (e.g. when saved window class does not exist anymore, or frame was added later)
		if(!wc && !getDefaultWindowID ().isEmpty () && isRequired ())
			wc = WindowManager::instance ().getClass (getDefaultWindowID ());

		SharedPtr<Object> holder (this); // prevent crash when this is a child of a MultiFrame that gets removed in onChildHidden

		restoringView = true;

		if(wc)
			openView (*wc);
		else
			hide ();

		restoringView = false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::restoreViews (Container& popupFrames)
{
	restoreView ();

	ForEach (*this, DockPanelItem, item)
		FrameItem* frameItem = ccl_cast<FrameItem> (item);
		if(frameItem)
			frameItem->restoreViews (popupFrames);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DockPanelItem* FrameItem::getNextVisible (DockPanelItem* parent, int startIndex, int direction)
{
	int i = startIndex + direction;
	while(DockPanelItem* next = unknown_cast<DockPanelItem> (parent->getChild (i)))
	{
		if(next->isVisible ())
			return next;
		i += direction;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::checkNeighbourDivider (DockPanelItem* parent, int startIndex, int direction, bool show)
{
	// if neighbour item is an autoShow divider, show it if the next visible item is not a divider
	DockPanelItem* neighbour = nullptr;
	DividerItem* divider = nullptr;
	do
	{
	startIndex += direction;
		neighbour = unknown_cast<DockPanelItem> (parent->getChild (startIndex));
		if(!neighbour)
			return;

		divider = ccl_cast<DividerItem> (neighbour);
	} while(!divider && !neighbour->isVisible ()); // skip invisible other items until divider found

	if(divider && !divider->isVisible () && divider->isAutoShow ())
	{
		DockPanelItem* neighbour = getNextVisible (parent, startIndex, direction);
		if(neighbour && !ccl_cast<DividerItem> (neighbour))
		{
			if(show)
				divider->show ();
			else
				divider->isVisible (true);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FrameItem::getViewForClass (WindowClassRef windowClass)
{
	View* view = getView ();
	if(FrameView* frameView = ccl_cast<FrameView> (view))
		view = frameView->getChild (0);
	if(view)
	{
		// view might be destroyed during the following calls!
		AutoPtr<IUnknown> c1 = view->getController ();
		if(c1)
			c1->retain ();
		if(view)
		{
			IUnknown* c2 = windowClass.getController ();
			if(view && isEqualUnknown (c1, c2))
				return view;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameItem::isViewOpen (WindowClassRef windowClass)
{
	return getViewForClass (windowClass) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameItem::isReallyVisible ()
{
	return isEmbedded () ? view != nullptr : isVisible ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::setVisibleState (bool state)
{
	isVisible (state);
	getVisible ()->setValue (state);
	signalWindowState (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::signalWindowState (bool state)
{
	if(windowClass)
	{
		if(WorkspaceInstance* instance = ccl_cast<WorkspaceInstance> (getWorkspace ()))
			WindowManager::instance ().onWindowStateChanged (*windowClass, instance->getID (), state);
		else
			WindowManager::instance ().onWindowStateChanged (*windowClass, state);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameItem::checkCondition (StringRef groupID) const
{
	if(condition.isEmpty ())
		return true;

#if 1
	Attributes variables;
	variables.set ("$group", groupID); // a condition may depend on the group for which we search for a valid frame

	Variant result;
	SkinExpressionParser::evaluate (result, String (condition), variables);

	CCL_PRINTF ("%s: condition is %s: %s\n", MutableCString (getName ()).str (), result.asBool () ? "true" : "false", MutableCString (condition).str ());
	return result.asBool ();
#else
	// old approach: only properties
	MutableCString propertyString (condition);
	bool wantedResult = true;
	if(condition.startsWith ("not "))
	{
		propertyString = propertyString.subString (4);
		wantedResult = false;
	}

	#if DEBUG_LOG
	MutableCString s (getName ());
	Debugger::printf ("%s: condition is %s %s\n", s.str (), Property (propertyString).get ().asBool () == wantedResult ? "true" : "false", condition.str ());
	#endif

	return Property (propertyString).get ().asBool () == wantedResult;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FrameItem::openView (WindowClassRef wc)
{
	if(viewIsLocked ())
		return nullptr;
	
	viewIsLocked (true);
	
	bool shouldReuseFrameView = false;

	if(isReallyVisible ())
	{
		saveViewState ();

		// try to reuse FrameView for new content (not for popup frames, they reuse their window anyway)
		shouldReuseFrameView = !restoringView && !(isPopup () || isDetachedFrame ());

		if((isPopup () || isDetachedFrame ()) || shouldReuseFrameView)
			signalWindowState (false);
		else
			hide ();
	}

	take_shared (windowClass, const_cast<WindowClass*> (&wc));
	windowID = windowClass->getID ();

	#if DEBUG_PROFILE
	CCL_PROFILE_START (OpenView)
	#endif

	if(!shouldReuseFrameView || !tryReuseFrameView ())
		show ();

	#if DEBUG_PROFILE
	CCL_PROFILE_STOP(OpenView)
	#endif

	if(titleParam)
		titleParam->setValue (getContentTitle ());

	if(WindowManager::instance ().shouldActivateWindows ())
		if(FrameView* frameView = ccl_cast<FrameView> (getView ()))
			frameView->activate ();

	viewIsLocked (false);
	
	return getView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::restoreViewState (WindowClass& windowClass)
{
	UnknownPtr<IViewStateHandler> viewStateHandler (windowClass.getController ());
	if(viewStateHandler)
	{
		ViewState* state = lookupViewState (windowClass, false);
		Attributes* viewState = state ? state->getViewState () : nullptr;
		viewStateHandler->loadViewState (windowID, MutableCString (getName ()), viewState ? *viewState : *(AutoPtr<Attributes> (NEW Attributes ())), state);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API FrameItem::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "Content")
		if(windowClass && windowClass->getTheme ())
		{
			// enusure that "$frame" is present, in case content view is created by another theme
			String frameUrl;
			WorkspaceSystem::makeFrameUrl (frameUrl, *this);
			Attributes arguments;
			arguments.setAttribute ("frame", frameUrl);
						
			return windowClass->getTheme ()->createView (MutableCString (windowClass->getFormName ()), windowClass->getController (), &arguments);
		}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FrameItem::createViewInternal (WindowClass& windowClass)
{
	Theme* theme = windowClass.getTheme ();
	ASSERT (theme)
	if(!theme)
		return nullptr;

	String frameUrl;
	WorkspaceSystem::makeFrameUrl (frameUrl, *this);
	
	Attributes arguments;
	arguments.setAttribute ("frame", frameUrl);

	if(Workspace* w = getWorkspace ())
	{
		String currentWorkspace (w->getID ());
		int index = currentWorkspace.lastIndex (String (":"));
		if(index > 0)
			currentWorkspace.truncate (index);
		arguments.setAttribute ("workspace", currentWorkspace);
	}

	//CCL_PRINTF ("FrameItem::createView: %s\n", MutableCString (frameUrl).str ());

	View* view = nullptr;

	MutableCString decorName;
	if(!getDecor ().isEmpty ())
	{
		decorName = "Workspace.";
		decorName += getDecor ();
	}
	else
	{
		if(isDetachedFrame ())
			decorName = "Workspace.DetachedFrame";
	}

	if(!decorName.isEmpty ())
	{
		// try to create a decorating template view; the decor form should include the actual content as view "Content" from controller "$frame"
		view = unknown_cast<View> (FrameworkTheme::instance ().createView (decorName, windowClass.getController (), &arguments));

		if(Form* outerForm = ccl_cast<Form> (view))
			if(Form* contentForm = FrameView::findContentForm (*outerForm))
			{
				StyleFlags wstyle;
				wstyle.fromLargeInt (outerForm->getWindowStyle ().toLargeInt () | contentForm->getWindowStyle ().toLargeInt ());
				outerForm->setWindowStyle (wstyle);
				if(!contentForm->getTitle ().isEmpty ())
					outerForm->setTitle (contentForm->getTitle ());
				
				// apply optional decor mask
				MutableCString decorNameMask (getDecor ());
				decorNameMask.append (".Mask");
				VisualStyle* style = unknown_cast<VisualStyle> (&theme->getStyle (decorNameMask));
				if(IImage* mask = style->getImage ("background"))
				{
					Rect padding;
					style->getPadding (padding);

					Rect maskSize (outerForm->getSize ());
					maskSize.left += padding.left;
					maskSize.top += padding.top;
					maskSize.bottom -= padding.bottom;
					maskSize.right -= padding.right;
					
					auto imageView = NEW ImageView (mask, maskSize);
					imageView->setSizeMode (IView::kAttachAll);
					imageView->setStyle (StyleFlags (Styles::kNoHelpId));
					outerForm->addView (imageView);
				}
			}
	}

	// create the view
	if(!view)
	{
		CCL_PROFILE_START (CreateView)
		view = unknown_cast<View> (theme->createView (MutableCString (windowClass.getFormName ()), windowClass.getController (), &arguments));
		CCL_PROFILE_STOP (CreateView)
	}
	if(view)
	{
		Point originalViewSize (view->getSize ().getSize ());

		Rect r (view->getSize ());
		r.moveTo (Point ());

		Point size;
		if(getSavedSize (size))
		{
			#if DEBUG_LOG
			{
				Point validSize (size);
				view->getSizeLimits ().makeValid (validSize);
				if(size != validSize)
					Debugger::printf ("Workspace frame: adjust saved size (%s, %d x %d) to valid (%d x %d)\n", windowID.str (), size.x, size.y, validSize.x, validSize.y);
			}
			#endif

			// check saved size against sizelimits (saved size might be based on an outdated skin view)
			view->getSizeLimits ().makeValid (size);
			r.setSize (size);
		}
		else if(isFill ())
		{
			// there is no saved size in "fill" frames: instead of initial size of created view, use size of existing frameview instead (from previous content, to keep sibling size stable)
			if(FrameView* frameView = ccl_cast<FrameView> (this->view))
			{
				frameView->getClientRect (r);
				view->getSizeLimits ().makeValid (r);
			}
		}

		view->setSize (r);

		// Make sure that no leftover space in the workspace frame is visible.
		// Either let the view fill the workspace frame or let the workspace frame
		// have the size of the view.
		FrameItem* parent = ccl_cast<FrameItem> (getParentItem ());
		int sizeMode = view->getSizeMode ();
		if((sizeMode & View::kHFitSize) == 0 || (parent && parent->isVertical ()))
			sizeMode |= View::kAttachLeft | View::kAttachRight;
		if((sizeMode & View::kVFitSize) == 0 || (parent && parent->isHorizontal ()))
			sizeMode |= View::kAttachTop | View::kAttachBottom;

		view->setSizeMode (sizeMode);

		FrameView* frameView = ccl_cast<FrameView> (this->view);
		if(frameView) // reusing an existing FrameView
		{
			if(getFillFactor () != 0)
				frameView->setOriginalViewSize (originalViewSize); // save original size for later use in applyFillFactor
			return view;
		}

		frameView = NEW FrameView (this, r);
		frameView->setContent (view);

		if(view && view->hasVisualStyle ())
			frameView->setVisualStyle (unknown_cast<VisualStyle> (&view->getVisualStyle ()));
		
		if(getFillFactor () != 0)
			frameView->setOriginalViewSize (originalViewSize);
		return frameView;
	}
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FrameItem::createView (Theme& theme)
{
	if(windowClass)
	{
		ThemeSelector themeSelector (&theme);

		// restore viewstate before creating view
		restoreViewState (*windowClass);
		return createViewInternal (*windowClass);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FrameItem::createMissingViews (ObjectList& items)
{
	ASSERT (!view)

	FrameItem* parentItem = ccl_cast<FrameItem> (getParentItem ());
	ASSERT (parentItem)
	if(!parentItem)
		return nullptr;

	View* parentView = parentItem->getView ();
	if(!parentView)
		parentView = parentItem->createMissingViews (items);

	if(parentView)
	{
		// create view for this item
		view = createView (parentView->getTheme ());
		if(view)
		{
			items.prepend (this);
			CCL_PRINTF ("createMissingViews: \"%s\" %s (%s) ", windowID.str (), myClass ().getPersistentName (), getName ().isEmpty () ? "" : MutableCString (getName ()).str ())
			LOG_VIEW (view, 0, false)
		}
	}
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::adjustChildView (View& childView, AnchorLayoutView& parentView)
{
	// try to adjust child to whole parent size in "other" direction of layout
	Rect rect (childView.getSize ());
	if(parentView.getStyle ().isHorizontal ())
	{
		rect.top = 0;
		rect.bottom = parentView.getSize ().getHeight ();
	}
	else
	{
		rect.left = 0;
		rect.right = parentView.getSize ().getWidth ();
	}

	const SizeLimit& childLimits (childView.getSizeLimits ());
	if(childLimits.isValid ())
	{
		// respect sizeLimits only in main layout direction; in other direction, the attachment relationship is crucial, even if violating limits
		Point validSize (rect.getSize ());
		childLimits.makeValid (validSize);
		if(parentView.getStyle ().isHorizontal ())
			rect.setWidth (validSize.x);
		else
			rect.setHeight (validSize.y);
	}
	childView.setSize (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int FrameItem::getViewIndex (FrameItem* searchItem) const
{
	// determine the index of the view for this frame in its parent view;
	// similar to DockPanelItem::getIndex, but we must ignore popup/detached frame items,
	// they are not added to our parent view, and so confuse the index relation of items and views.
	// todo: more robust approach, don't store popups in the same parent item that is also a group frame

	int idx = 0;
	if(countChildren () > 0)
		ForEach (getChildren (), DockPanelItem, item)
			FrameItem* frameItem = ccl_cast<FrameItem> (item);
			if(frameItem && (frameItem->isPopup() || frameItem->isDetachedFrame ()))
				continue;

			if(item == searchItem)
				return idx;

			if(item->isVisible ())
				idx++;
		EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameItem::applyFillFactor (FrameView* frameView, AnchorLayoutView* parentLayoutView)
{
		if(getFillFactor () == 0)
		return false;

		AnchorLayoutItem* layoutItem = static_cast<AnchorLayoutItem*> (parentLayoutView->findLayoutItem (frameView));
		if(!layoutItem)
		return false;

		// the preferred size of items with fill factor must not change
		layoutItem->fillFactor = getFillFactor ();
		layoutItem->preferredSize = frameView->getOriginalViewSize ();
		layoutItem->preferredSizeLocked (true);
		return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FrameItem::tryReuseFrameView ()
{
	if(!view)
		return false;

	if(FrameView* frameView = ccl_cast<FrameView> (view))
	{
		AnchorLayoutView* parentLayoutView = ccl_cast<AnchorLayoutView> (frameView->getParent ());
		if(!parentLayoutView)
		{
			hide ();
			return false;
		}

		// remove old view before creating new one (avoid confusing controller when reopening the same view)
		frameView->removeAll ();
		frameView->resetSizeLimits ();

		if(View* newContent = createView (frameView->getTheme ()))
		{
			Window::UpdateCollector uc (parentLayoutView->getWindow ());
			Window::SizeChangeCollector sizeChangeCollector (parentLayoutView->getWindow ());

			// fit content into layout context
			adjustChildView (*newContent, *parentLayoutView);

			// give deferred layout tasks a chance to perform (SizeVariantLayout)
			parentLayoutView->flushLayout ();

			// fit frame to content
			Point contentSize (newContent->getSize ().getSize ());
			
			// temporarily set limits to new conent size
			SizeLimit tempLimits;
			tempLimits.setFixed (contentSize);
			frameView->setSizeLimits (tempLimits);

			// set frameView to new size
			frameView->setSize (Rect (frameView->getSize ()).setSize (contentSize));

			if(!applyFillFactor (frameView, parentLayoutView))
				parentLayoutView->forceSize (frameView, contentSize);
			
			frameView->setContent (newContent);
			frameView->resetSizeLimits ();
			parentLayoutView->onChildLimitsChanged (frameView);
			parentLayoutView->onViewsChanged (); // trigger layout

			setVisibleState (true);

			// let (new) content view send trigger message
			frameView->signalOnActivate ();
			return true;
		}
	}

	hide ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FrameItem::show ()
{
	if(!view)
	{
		CCL_PRINTF ("\n----- FrameItem::Show \"%s\" (%s) %s\n", getWindowID ().str (), myClass ().getPersistentName (), getName ().isEmpty () ? "" : MutableCString (getName ()).str ())

		// first create views top-down for all parents
		ObjectList items;
		createMissingViews (items);

		// now attach them bottom-up
		FrameItem* topItem = (FrameItem*)items.getLast ();
		ForEach (items, FrameItem, item)
			FrameItem* parentItem = (FrameItem*)item->getParentItem ();
			ASSERT (parentItem)
			if(parentItem)
			{
				int index = parentItem->getViewIndex (item);
				View* parentView = parentItem->getView ();
				View* childView = item->getView ();
				ASSERT (index != -1)
				ASSERT (parentView)
				ASSERT (childView)

				#if DEBUG_LOG
				CCL_PRINTF ("\"%s\" %s (%s) [%d]", item->getWindowID ().str (), item->myClass ().getPersistentName (), getName ().isEmpty () ? "" : MutableCString (item->getName ()).str (), index)
				view->log ("\n    -> ");
				#endif

				bool forceSize= false;
				AnchorLayoutView* parentLayoutView = nullptr;
				Point childSize;

				FrameView* frameView = frameView = ccl_cast<FrameView> (childView);

				if(item == topItem)
				{
					// parent already existed before
					#if DEBUG_LOG
					Rect parentSize (parentView->getSize ());
					CCL_PRINTF ("    existing parentview (%d,%d,%d,%d)\n", parentSize.left, parentSize.top, parentSize.right, parentSize.bottom);
					#endif

					parentLayoutView = ccl_cast<AnchorLayoutView> (parentView);
					//ASSERT (parentLayoutView)
					if(parentLayoutView)
					{
						adjustChildView (*childView, *parentLayoutView); // todo: only if childView is also LayoutView ?
						Point savedSize;
						if(!restoringView && !ccl_cast<AnchorLayoutView> (childView) && getSavedSize (savedSize))
						{
							forceSize = true;
							childSize = childView->getSize ().getSize ();

							if(frameView)
							{
								// temporarily fix limits to content size
								SizeLimit tempLimits;
								tempLimits.setFixed (childSize);

								LayoutPrimitives::applySizeLimitsShallow (*frameView, tempLimits);
							}
						}

						bool isFirstChild = parentView->getFirst () == nullptr;
						parentView->insertView (index, childView);

						if(item->getFillFactor () != 0)
							if(frameView)
								item->applyFillFactor (frameView, parentLayoutView);

						if(!isFirstChild)
						{
							// try to fill empty spaces in other direction for all parent layouts
							AnchorLayoutView* parentLayout = parentLayoutView;
							while(AnchorLayoutView* grandParentLayout = ccl_cast<AnchorLayoutView> (parentLayout->getParent ()))
							{
								adjustChildView (*parentLayout, *grandParentLayout);
								parentLayout = grandParentLayout;
							}
						}

						//if(parentLayoutView->isEmpty ())
						//{CCL_PRINTF ("    parentview has no childs, take parent size (%d,%d,%d,%d)\n", r.left, r.top, r.right, r.bottom);}
					}
					else if(EmbeddedFrameView* embeddedFrameView = ccl_cast<EmbeddedFrameView> (parentView))
					{
						AutoPtr<ViewAnimator> animator;
						
						if(!parentItem->viewIsAppearing ())
							animator = ViewAnimator::create (embeddedFrameView, embeddedFrameView->getTransitionType ());
						
						// fit child into embedded frame
						Rect rect;
						parentView->getClientRect (rect);

						// but use child size if frame size is empty
						if(rect.getHeight () == 0)
							rect.setHeight (childView->getHeight ());
						if(rect.getWidth () == 0)
							rect.setWidth (childView->getWidth ());

						// but try to respect size limits where appropriate (on the other hand, don't mess up parent-child relationship for correct attaching...)
						const SizeLimit& childLimits (childView->getSizeLimits ());
						if(childLimits.isValid ())
						{
							Rect limited (rect);
							childLimits.makeValid (limited);

							if(childView->getSizeMode () & IView::kHFitSize)
								rect.setWidth (limited.getWidth ());

							if(childView->getSizeMode () & IView::kVFitSize)
								rect.setHeight (limited.getHeight ());
						}
						childView->setSize (rect);

						if(animator)
							animator->snipFromView (parentView, &rect);

						parentView->insertView (index, childView);

						// give event handler a chance to modify the appearance
						if(Workspace* w = getWorkspace ())
							if(IWorkspaceEventHandler* eventHandler = w->getEventHandler ())
							{
								WorkspaceEvent e (WorkspaceEvent::kOpenView, parentView);
								e.windowClass = WindowManager::instance ().getCurrentWindowClass ();
								e.arguments = WindowManager::instance ().getCurrentArguments ();
								e.animator = animator;

								Window::UpdateCollector uc (parentView->getWindow ()); // suppress any direct updates to the window
								eventHandler->onWorkspaceEvent (e);
							}

						if(animator)
						{						
							animator->snipToView (parentView, &rect);
							animator->makeTransition ();
						}
					}
				}
				else
				{
					// parentView has just been created
					auto hasOtherChild = [] (View& parentView)
					{
						// parentView shouldn't have any child views yet, except views from a SystemFrameItem
						ForEachView (parentView, v)
							auto frameView = ccl_cast<FrameView> (v);
							if(!frameView || !ccl_cast<SystemFrameItem> (frameView->getFrameItem ()))
								return true;
						EndFor
						return false;
					};
					ASSERT (!hasOtherChild (*parentView))
					ASSERT (ccl_cast<AnchorLayoutView> (parentView))

					if(parentView->getSize ().isEmpty ())
					{
						// parent size is empty: fit parentView to childView
						Rect parentSize (childView->getSize ());
						parentSize.moveTo (parentView->getSize ().getLeftTop ());
						parentView->setSize (parentSize);

						CCL_PRINTF ("    fit parentView to child (%d,%d,%d,%d)\n", parentSize.left, parentSize.top, parentSize.right, parentSize.bottom);
					}
					// todo: else adjustChildView after insert?
					parentView->insertView (index, childView);
				}

				#if DEBUG_LOG
				Rect r (parentView->getSize ());
				CCL_PRINTF ("    parentview after insert: (%d,%d,%d,%d)\n", r.left, r.top, r.right, r.bottom);
				r = childView->getSize ();
				CCL_PRINTF ("    childView now: (%d,%d,%d,%d)\n", r.left, r.top, r.right, r.bottom);
				#endif

				item->setVisibleState (true);

				// check if dividers must be shown left or right from this view
				index = parentItem->getIndex (item, false);
				item->checkNeighbourDivider (parentItem, index, -1);
				item->checkNeighbourDivider (parentItem, index, +1);

				if(forceSize)
				{
					parentLayoutView->forceSize (childView, childSize);

					// give LayoutView a chance to recover from potential damage done by forceSize (force doLayout)
					parentLayoutView->setLayoutSuspended (true);
					parentLayoutView->setLayoutSuspended (false);

					if(frameView)
					{
						frameView->resetSizeLimits ();
						parentLayoutView->onChildLimitsChanged (frameView);
					}
				}
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FrameItem::hide ()
{
	// find embedded frame
	EmbeddedFrameView* embeddedFrameView = nullptr;
	AutoPtr<ViewAnimator> animator;
	Rect rect;

#if 1
	// find embedded frame parent or grandparent
	DockPanelItem* item = this;
	for(int i = 0; i < 2; i++)
	{
		item = item->getParentItem ();
		if(!item)
			break;

		if(EmbeddedFrameItem* embeddedFrame = ccl_cast<EmbeddedFrameItem> (item))
		{
			embeddedFrameView = ccl_cast<EmbeddedFrameView> (embeddedFrame->getView ());
			if(embeddedFrameView && embeddedFrameView->getSize ().isEmpty ())
				embeddedFrameView = nullptr;
			break;
		}
	}
#endif

	if(embeddedFrameView)
	{
		embeddedFrameView->getVisibleClient (rect);
		animator = ViewAnimator::create (embeddedFrameView, ViewAnimator::getInverseTransition (embeddedFrameView->getTransitionType ()));
		if(animator)
			animator->snipFromView (embeddedFrameView, &rect);
	}

	SuperClass::hide ();

	if(embeddedFrameView)
	{
		// give event handler a chance to modify the appearance
		if(Workspace* w = getWorkspace ())
			if(IWorkspaceEventHandler* eventHandler = w->getEventHandler ())
			{
				WorkspaceEvent e (WorkspaceEvent::kCloseView, embeddedFrameView);
				e.windowClass = WindowManager::instance ().getCurrentWindowClass ();
				e.arguments = WindowManager::instance ().getCurrentArguments ();
				e.animator = animator;

				Window::UpdateCollector uc (embeddedFrameView->getWindow ()); // suppress any direct updates to the window
				eventHandler->onWorkspaceEvent (e);
			}
	}

	if(animator)
	{
		animator->snipToView (embeddedFrameView, &rect);
		animator->makeTransition ();
	}

	if(!isVisible ())
	{
		// hide neighbour divider if it's at start or end now, or if there are 2 dividers side by side
		DockPanelItem* parentItem = getParentItem ();
		if(parentItem)
		{
			int index = parentItem->getIndex (this, false);
			DockPanelItem* left  = getNextVisible (parentItem, index, -1);
			DockPanelItem* right = getNextVisible (parentItem, index, +1);
			DividerItem* leftDivider = ccl_cast<DividerItem> (left);
			DividerItem* rightDivider = ccl_cast<DividerItem> (right);

			if(leftDivider)
			{
				if(rightDivider || !right)
					leftDivider->hide ();
			}
			else if(rightDivider && !left)
				rightDivider->hide ();
		}
	}
	onViewHidden ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::onViewHidden ()
{
	FrameItem* parentItem = ccl_cast<FrameItem> (getParentItem ());
	if(parentItem)
		parentItem->onChildHidden (*this);

	// reset windowID if not saving state
	if(savingState ())
		savingState (false);
	else
		windowID = nullptr;

	if(isVolatile ())
	{
		// this frame appears in an EmbeddedFrame whose container view is not controlled by the workspace:
		// reset content, since we cannot guarantee that we can open the same WindowClass when restoring
		ASSERT (isEmbedded ())
		resetContent ();
	}

	signalWindowState (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::onChildHidden (FrameItem& child)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FrameItem::resetContent ()
{
	windowID = getDefaultWindowID ();
	take_shared<WindowClass> (windowClass, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FrameItem::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case kCloseTag:
		(NEW Message ("close"))->post (this, 10);
		break;
	}
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FrameItem::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "close")
	{
		if(windowClass )
			WindowManager::instance ().closeWindow (windowClass);
	}
	else
		SuperClass::notify (subject, msg);
}

//************************************************************************************************
/// RootFrameItem
//************************************************************************************************

DEFINE_CLASS (RootFrameItem, FrameGroupItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

RootFrameItem::RootFrameItem ()
: perspective (nullptr),
  hidingAll (false)
{
	setName (CCLSTR ("RootFrame"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RootFrameItem::RootFrameItem (const RootFrameItem& item)
: FrameGroupItem (item),
  perspective (nullptr),
  hidingAll (false)
{
	setName (CCLSTR ("RootFrame"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootFrameItem::restoreViews ()
{
	// restore all but PopupFrames, which all only collected 
	ObjectArray popupFrames;
	SuperClass::restoreViews (popupFrames);

	if(!popupFrames.isEmpty ())
	{
		// sort them by z-order, then restore them (topmost last)
		popupFrames.sort ();
		ForEach (popupFrames, PopupFrameItem, popupFrame)
			popupFrame->restoreView ();
		EndFor
	}

	// collect contained detachedFrames
	findChildFrame (*AutoPtr<IRecognizer> (Recognizer::create ([this] (IUnknown* obj)
		{
			if(DetachedFrameItem* detachedFrame = unknown_cast<DetachedFrameItem> (obj))
				registerDetachedFrame (detachedFrame);
			return false;
		})));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootFrameItem::registerDetachedFrame (DetachedFrameItem* frame)
{
	detachedFrames.addOnce (frame);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootFrameItem::unregisterDetachedFrame (DetachedFrameItem* frame)
{
	detachedFrames.remove (frame);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DetachedFrameItem* RootFrameItem::findDetachedFrame (IRecognizer& recognizer)
{
	for(auto detachedFrame : iterate_as<DetachedFrameItem> (detachedFrames))
		if(detachedFrame->isDetached ())
			if(recognizer.recognize (detachedFrame->asUnknown ()))
				return detachedFrame;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

struct ItemStateSaver: public Recognizer
{
	tbool CCL_API recognize (IUnknown* object) const override
	{
		if(FrameItem* frameItem = unknown_cast<FrameItem> (object))
		{
			// save view state
			frameItem->savingState (true);
			frameItem->saveViewState ();
		}
		return false;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
	
void RootFrameItem::saveItemStates ()
{
	// save item sizes & determine z-index of each popup window
	ItemStateSaver saver;
	findChildFrame (saver);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RootFrameItem::hideAll ()
{
	// first save states of all items, then hide them
	saveItemStates ();
	
	ScopedVar<bool> guard (hidingAll, true); // prevent saving states again during hide
	SuperClass::hideAll ();
}

//************************************************************************************************
/// FrameGroupItem 
//************************************************************************************************

DEFINE_CLASS (FrameGroupItem, FrameItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameGroupItem::FrameGroupItem (int _style)
: FrameItem (_style)
{
	if(!isHorizontal () && !isVertical ())
		isHorizontal (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameGroupItem::FrameGroupItem (const FrameGroupItem& item)
: FrameItem (item)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* FrameGroupItem::createView (Theme& theme)
{
	ThemeSelector selector (theme);
	
	AutoPtr<BoxLayout> layout (NEW BoxLayout);
	layout->setProperty (ATTR_SPACING, 0);
	layout->setProperty (ATTR_MARGIN, 0);

	StyleFlags layoutStyle (0, /*Styles::kLayoutUnifySizes*/0);

	// Convert FrameItem style to StyleFlags.
	layoutStyle.setCommonStyle (Styles::kHorizontal, isHorizontal ());
	layoutStyle.setCommonStyle (Styles::kVertical, isVertical ());

	AnchorLayoutView* frame = NEW AnchorLayoutView (Rect (), layoutStyle, layout);

	int parentFitSize = 0;
	if(DockPanelItem* parentItem = getParentItem ())
		if(View* parentView = parentItem->getView ())
			parentFitSize = parentView->getSizeMode () & IView::kFitSize;

	int sizeMode = View::kAttachAll | parentFitSize;
	if(isFill ())
		sizeMode |= IView::kFill;

	frame->setSizeMode (sizeMode);
	#if DEBUG
	frame->setName (getName ()); // for cclspy
	#endif

	ForEach (getChildren (), DockPanelItem, item)
		if(item->isVisible ())
		{
			ASSERT (item->getView () == nullptr)
			if(ccl_cast<DetachedFrameItem> (item))
				continue; // quick fix

			View* v = item->createView (theme);
			if(v)
			{
				// limit end coord of child in other direction to container size
				Rect r (v->getSize ());
				if(layoutStyle.isHorizontal ())
					ccl_upper_limit (r.bottom, frame->getSize ().getHeight ());
				else
					ccl_upper_limit (r.right, frame->getSize ().getWidth ());
				v->setSize (r);

				item->setViewAndState (v);
				frame->addView (v);
			}
		}
	EndFor
	return frame;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FrameGroupItem::addItem (IDockPanelItem* item)
{
	if(hasDividers () &&  countChildren () > 0)
		SuperClass::addItem (NEW DividerItem);

	return SuperClass::addItem (item);
}

//************************************************************************************************
/// DividerItem
//************************************************************************************************

DEFINE_CLASS (DividerItem, DockPanelItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

DividerItem::DividerItem ()
: dividerParam (nullptr),
  autoShow (true),
  width (0),
  outreach (-1)
{
	isHidable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DividerItem::~DividerItem ()
{
	if(dividerParam)
		dividerParam->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* DividerItem::getDividerParam ()
{
	if(!dividerParam)
	{
		dividerParam = NEW IntParam (0, kMaxCoord, "divider");

		// connect to CustomParams controller of perspective, for triggering document dirty state after manipulation
		if(RootFrameItem* rootFrame = unknown_cast<RootFrameItem> (getRoot ()))
			if(Perspective* perspective = rootFrame->getPerspective ())
				if(UnknownPtr<IParamObserver> controller = &perspective->getICustomParams ())
				{
					dividerParam->connect (controller, 0);
					dividerParam->setStorable (true);
				}

		if(Divider* divider = ccl_cast<Divider> (view))
			divider->setParameter (dividerParam);
	}
	return dividerParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API DividerItem::findParameter (StringID name) const
{
	if(name == "divider")
		return const_cast<DividerItem*> (this)->getDividerParam ();

	return SuperClass::findParameter (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* DividerItem::createView (Theme& theme)
{
	Coord w = width;
	if(w == 0)
		w = theme.getThemeMetric (ThemeElements::kDividerSize);
	else if(w < 0)
		w = 0;

	Rect rect (0, 0, w, w);
	int sizeMode = 0;
	bool horizontal = true;
	if(View* parentView = getParentView ())
	{
		if(parentView->getStyle ().isHorizontal ())
			rect.setHeight (parentView->getHeight ());
		else
		{
			rect.setWidth (parentView->getWidth ());
			horizontal = false;
		}
	}
	else if(FrameItem* parentItem = ccl_cast<FrameItem> (getParentItem ()))
		if(parentItem->isVertical ())
			horizontal = false;

	if(horizontal)
	{
		style.setCommonStyle (Styles::kHorizontal);
		sizeMode = View::kAttachTop|View::kAttachBottom;
	}
	else
	{
		style.setCommonStyle (Styles::kVertical);
		sizeMode = View::kAttachLeft|View::kAttachRight;
	}

	Divider* divider = NEW Divider (rect, getDividerParam (), style);
	divider->setTheme (const_cast<Theme*> (&theme));
	divider->setSizeMode (sizeMode);
	if(outreach >= 0) 
		divider->setOutreach (outreach);
	return divider;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DividerItem::checkSyncSlaves ()
{
	if(getView () && getView ()->getStyle ().isCustomStyle (Styles::kDividerBehaviorMaster))
		if(Divider* divider = ccl_cast<Divider> (getView ()))
			divider->triggerSyncSlaves ();
}

//************************************************************************************************
// MultiFrameItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MultiFrameItem, FrameItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MultiFrameItem::reuseSuspended = false;

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiFrameItem::suspendReuse (bool state)
{
	reuseSuspended = state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MultiFrameItem::MultiFrameItem (int style)
: FrameItem (style),
  childCounter (0),
  inHideAll (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* MultiFrameItem::openView (WindowClassRef windowClass)
{
	if(isPinnable ())
	{
		bool wantReuse = !reuseSuspended;
		if(wantReuse)
		{
			// force a new child frame if the window class was (and will be) pinned
			if(ViewState* state = lookupViewState (windowClass, false))
				if(state->isPinned ())
					wantReuse = false;

			if(wantReuse)
			{
				// try to resuse a child frame that is not pinned
				ForEach (getChildren (), DockPanelItem, item)
					FrameItem* frameItem = ccl_cast<FrameItem> (item);
					if(frameItem)
					{
						bool canReuse = !frameItem->isPinned ();
						if(canReuse)
							return frameItem->openView (windowClass);
					}
				EndFor
			}
		}
	}

	// reuse existing child frame for this window class (restored, but not open)
	AutoPtr<IRecognizer> recognizer (Recognizer::create ([&] (IUnknown* object)
	{
		FrameItem* item = unknown_cast<FrameItem> (object);
		return item && item->getWindowID () == windowClass.getID () && !item->getCurrentWindowClass ();
	}));
	if(FrameItem* frameItem = findChildFrame (*recognizer))
		return frameItem->openView (windowClass);

	// no reusable child frame found: create a new one
	return newChildItem ().openView (windowClass);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiFrameItem::hideAll ()
{
	inHideAll = true;
	SuperClass::hideAll ();
	inHideAll = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FrameItem& MultiFrameItem::newChildItem ()
{
	int childStyle = style;
	set_flag<int> (childStyle, FrameItem::kMultiple, false);

	FrameItem* childItem = FrameItem::createItem (childStyle);

	String name (getName ());
	name.append (":");
	name.appendIntValue (childCounter++);
	childItem->setName (name);
	childItem->setDecor (decor);

	childItem->setPosition (pos);

	if(DetachedFrameItem* detachedFrame = ccl_cast<DetachedFrameItem> (childItem))
	{
		for(auto groupID : iterate_as<Boxed::String> (groupIDs))
			childItem->addGroupID (*groupID);

		if(RootFrameItem* root = getRootFrame ())
			root->registerDetachedFrame (detachedFrame);
	}

	addItem (childItem);

	#if DEBUG_MULTI_DETACHED
	Debugger::printf ("MultiFrame: newChildItem: %s (total %d)\n", MutableCString (name).str (), countChildren ());
	#endif
	return *childItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiFrameItem::onChildHidden (FrameItem& child)
{
	if(!inHideAll)
	{
		if(removeChild (&child))
		{
			#if DEBUG_MULTI_DETACHED
			Debugger::printf ("MultiFrame: removeChild: %s (%d left)\n", MutableCString (child.getName ()).str (), countChildren ());
			#endif

			pos = child.getPosition (); // save position of last closed child

			if(DetachedFrameItem* detachedFrame = ccl_cast<DetachedFrameItem> (&child))
			{
				if(RootFrameItem* root = getRootFrame ())
					root->unregisterDetachedFrame (detachedFrame);

				DetachedFrameItem* newChild = updateDetachedChilds ();
				if(newChild)
					newChild->setDetached (detachedFrame->isDetached ()); // keep detached state of just removed instance
			}
	
			child.release ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DetachedFrameItem* MultiFrameItem::updateDetachedChilds ()
{
	if(isDetachedFrame ())
	{
		// need at least one "unused" (unpinned) detached frame
		if(!getChildArray ().findIf<DetachedFrameItem> ([] (const DetachedFrameItem& item) { return !item.isPinned (); }))
			return ccl_cast<DetachedFrameItem> (&newChildItem ());
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MultiFrameItem::restoreDetachedChildState ()
{
	ASSERT (isDetachedFrame ())
	if(isDetachedFrame ())
	{
		updateDetachedChilds ();
		
		DetachedFrameItem* detachedFrame = getChildArray ().findIf<DetachedFrameItem> ([] (const DetachedFrameItem& item) { return item.getWindowID ().isEmpty (); });
		if(!detachedFrame)
			detachedFrame = ccl_cast<DetachedFrameItem> (&newChildItem ());

		detachedFrame->setDetached (true);
	}
}

//************************************************************************************************
// PopupFrameItem
//************************************************************************************************

PopupFrameItem* PopupFrameItem::fromWindow (Window* window)
{
	if(window)
		if(Form* form = ccl_cast<Form> (window->getFirst ()))
			if(FrameView* frameView = ccl_cast<FrameView> (form->getFirst ()))
				return ccl_cast<PopupFrameItem> (frameView->getFrameItem ());
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (PopupFrameItem, FrameItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupFrameItem::PopupFrameItem (int style)
: FrameItem (style),
  pinned (nullptr),
  maximized (nullptr),
  fullscreen (nullptr),
  titleAlias (nullptr),
  zIndex (-1)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupFrameItem::PopupFrameItem (const PopupFrameItem& item)
: FrameItem (item),
  pinned (nullptr),
  maximized (nullptr),
  fullscreen (nullptr),
  titleAlias (nullptr),
  zIndex (-1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupFrameItem::~PopupFrameItem ()
{
	if(pinned)
		pinned->release ();
	if(maximized)
		maximized->release ();
	if(fullscreen)
		fullscreen->release ();

	if(titleAlias)
	{
		setTitleParam (nullptr);

		titleAlias->removeObserver (this);
		titleAlias->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* PopupFrameItem::getWindow ()
{
	return ccl_cast<Window> (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupFrameItem::setTitleParam (IParameter* param)
{
	ASSERT (titleAlias)
	AliasParam* alias = getTitleAlias ();

	if(IParameter* original = alias->getOriginal ())
		original->release ();

	alias->setOriginal (param);

	if(param)
		param->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupFrameItem::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case kMaximizedTag:
		if(Window* window = getWindow ())
		{
			if(window->isFullscreen ())
			{
				// leave fullscreen instead, ignore and reset maximized param
				setFullscreen (false);
				param->setValue (window->isMaximized ());
			}
			else
				maximize (param->getValue ().asBool ());
		}
		break;

	case kFullscreenTag:
		if(Window* window = getWindow ())
		{
			if(window->isMaximized ())
			{
				// unmaximize instead, ignore and reset fullscreen param
				maximize (false);
				param->setValue (window->isFullscreen ());
			}
			else
				setFullscreen (param->getValue ().asBool ());
		}
		break;

	case kPinnedTag:
		if(windowClass)
		{
			// update instances of a multi-detached frame
			if(isDetachedFrame ())
				if(MultiFrameItem* multiFrame = getParentNode<MultiFrameItem> ())
					multiFrame->updateDetachedChilds ();

			// notify windowClass controller
			UnknownPtr<IWorkspaceEventHandler> eventHandler (windowClass->getController ());
			if(eventHandler)
			{
				WorkspaceEvent e (isPinned () ? WorkspaceEvent::kPinned : WorkspaceEvent::kUnpinned, getView ());
				e.windowClass = windowClass;
				eventHandler->onWorkspaceEvent (e);
			}
		}
		break;
	}
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupFrameItem::maximize (bool state)
{
	if(Window* window = getWindow ())
	{
		window->tryMaximize (state);
		getMaximizedParam ()->setValue (window->isMaximized ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupFrameItem::setFullscreen (bool state)
{
	Window* window = getWindow ();
	if(window && window->getStyle ().isCustomStyle (Styles::kWindowBehaviorFullscreen))
		window->setFullscreen (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PopupFrameItem::compare (const Object& obj) const
{
	const PopupFrameItem* p = ccl_cast<PopupFrameItem> (&obj);
	return p ? zIndex - p->getZIndex () : Object::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupFrameItem::saveViewStateInternal (ViewState& state)
{
	bool maximized = false;
	bool fullscreen = false;
	
	Window* window = getWindow ();
	if(window)
	{
		// save z-index
		setZIndex (window->getZIndex ());

		// sync. window size
//		window->updateSize ();
		pos = window->getSize ().getLeftTop ();

		maximized = window->isMaximized () != 0;
		fullscreen = window->isFullscreen () != 0;
	}

	SuperClass::saveViewStateInternal (state);

	if(maximized || fullscreen)
	{
		// overwrite size stored by SuperClass: store the normal size
		Rect userSize;
		window->getUserSize (userSize);
		state.setSize (userSize.getSize ());
		state.setPosition (userSize.getLeftTop ());
	}
	state.isMaximized (maximized);
	state.isFullscreen (fullscreen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupFrameItem::restoreViews (Container& popupFrames)
{
	// popup frames are only collected
	popupFrames.add (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupFrameItem::tryOtherMonitor (Form& form)
{
	int numMonitors = Desktop.countMonitors ();
	if(numMonitors > 1)
	{
		Workspace* workspace = getWorkspace ();
		if(Window* mainWindow = workspace ? workspace->getWorkspaceWindow () : nullptr)
		{
			int mainMonitor = Desktop.findMonitor (mainWindow->getSize ().getCenter (), true);
			int monitor = (mainMonitor + 1) % numMonitors;
			
			Rect monitorSize;
			if(Desktop.getMonitorSize (monitorSize, monitor, true))
			{
				Rect size (form.getSize ());
				size.center (monitorSize);
				form.setSize (size);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupFrameItem::show ()
{
	if(windowClass)
	{
		Theme* theme = windowClass->getTheme ();
		ASSERT (theme)
		if(!theme)
			return;

		IUnknown* controller = windowClass->getController ();
		if(!controller && !windowClass->getControllerUrl ().isEmpty ())
			return;

		restoreViewState (*windowClass); // (IViewStateHandler may manipulate stored size here)

		bool shouldMaximize = false;
		bool shouldFullscreen = false;
		Point storedSize;
		ViewState* state = lookupViewState (*windowClass, false, true);
		if(state)
		{
			storedSize = state->getSize ();
			if(!state->getWindowID ().isEmpty ())
				pos = state->getPosition ();

			setPinned (state->isPinned ());
			shouldMaximize = state->isMaximized ();
			shouldFullscreen = state->isFullscreen ();

			// check if window is (almost) outside monitor
			Rect windowRect (pos.x, pos.y, storedSize);
			Rect monitorSize;
			int monitor = Desktop.findMonitor (windowRect.getCenter (), true);
			if(Desktop.getMonitorSize (monitorSize, monitor, true))
			{
				Rect monitorSize2 (monitorSize);
				monitorSize2.contract (30);
				if(monitorSize2.intersect (windowRect) == false)
				{
					// window will be moved inside: limit size to monitor size
					ccl_upper_limit (storedSize.x, monitorSize.getWidth ());
					ccl_upper_limit (storedSize.y, monitorSize.getHeight ());
				}
			}
		}

		saveSize (Point ()); // discard stored size for now (see below) 

		bool wasPluginViewHost = false;
		PlugInView::HostingMode oldPluginHostingMode = PlugInView::kDefaultHosting;
		SharedPtr<Window> windowToReactivate;

		auto findPlugInView = [] (View& rootView)
		{
			return static_cast<PlugInView*> (rootView.findView (*AutoPtr<IRecognizer> (Recognizer::create ([&] (IUnknown* obj)
				{ return unknown_cast<PlugInView> (obj) != nullptr; }))));
		};

		Window* window = getWindow ();
		Window::UpdateCollector updateCollector (window);
		Window::SizeChangeCollector sizeChangeCollector (window);
		if(window)
		{
			// determine old plug-in hosting mode
			if(window->getStyle ().isCustomStyle (Styles::kWindowBehaviorPluginViewHost))
			{
				wasPluginViewHost = true;
				if(PlugInView* oldPlugInView = findPlugInView (*window))
					oldPluginHostingMode = oldPlugInView->getHostingMode ();
			}

			window->removeAll (); // remove old view before creating new one (avoid confusing controller when reopening the same view)
		}

		View* content = createViewInternal (*windowClass);
		if(!content)
			return;

		AutoPtr<IParameter> titleParam;
		String helpid;

		// controller can provide window title, additional style flags, etc.
		String windowTitle (content->getTitle ());
		StyleFlags windowStyle (Styles::panelWindowStyle);
		if(Workspace* workspace = getWorkspace ())
			windowStyle = workspace->getWindowStyle ();

		if(UnknownPtr<IController> iController = controller)
		{
			if(titleParam.share (iController->findParameter (PopupFramesParams::kFrameTitle)))
				titleParam->toString (windowTitle);

			if(IParameter* styleParam = iController->findParameter (PopupFramesParams::kWindowStyle))
				windowStyle.custom |= styleParam->getValue ().asInt ();

			if(IParameter* helpParam = iController->findParameter (PopupFramesParams::kHelpID))
				helpParam->toString (helpid);
		}

		Form* frameClient = ccl_cast<Form> (content->getChild (0));
		if(frameClient)
		{
			windowStyle.custom |= frameClient->getWindowStyle ().custom;
			windowStyle.common |= (frameClient->getStyle ().common & (Styles::kTransparent|Styles::kTranslucent));
		}

		if(isMaximizable ())
			windowStyle.custom |= Styles::kWindowBehaviorMaximizable;

		if(isFullscreen ())
			windowStyle.custom |= Styles::kWindowBehaviorFullscreen;
		
		bool isInflate = windowStyle.isCustomStyle (Styles::kWindowBehaviorInflate);

		// we take care of storing size & position, don't let the window interfere
		windowStyle.setCustomStyle (Styles::kWindowBehaviorRestoreSize|Styles::kWindowBehaviorRestorePosition|Styles::kWindowBehaviorInflate, false);

		// apply stored size if window is sizeable
		if(windowStyle.isCustomStyle (Styles::kWindowBehaviorSizable) && !storedSize.isNull ())
		{
			if(content->getSizeLimits ().isValid ())
				content->getSizeLimits ().makeValid (storedSize);

			content->setSize (Rect (0, 0, storedSize));

			// give deferred layout tasks a chance to perform (SizeVariantLayout)
			content->flushLayout ();
			content->setSize (Rect (0, 0, storedSize));
		}
		else
			content->flushLayout ();

		if(GUI.getApplicationType () == GUI.kMobileApplication && windowStyle.isCustomStyle (Styles::kWindowAppearanceTitleBar))
		{
			// add own title bar view (if requested) for platforms that don't provide one
			String frameUrl;
			WorkspaceSystem::makeFrameUrl (frameUrl, *this);

			Attributes arguments;
			arguments.setAttribute ("frame", frameUrl);

			StringID kTitlebarForm = "WindowTitlebar";

			if(View* titleBar = unknown_cast<View> (FrameworkTheme::instance ().createView (kTitlebarForm, controller, &arguments)))
			{			
				titleBar->setSize (Rect (titleBar->getSize ()).setWidth (content->getWidth ()));

				AutoPtr<BoxLayout> layout (NEW BoxLayout);
				layout->setProperty (ATTR_SPACING, 0);
				layout->setProperty (ATTR_MARGIN, 0);
				AnchorLayoutView* layoutView = NEW AnchorLayoutView (Rect (0, 0, content->getWidth (), 0), StyleFlags (Styles::kVertical), layout);
				layoutView->setSizeMode (content->getSizeMode ());
				layoutView->addView (titleBar);
				layoutView->addView (content);
				content = layoutView;
			}
		}

		Form* form = ccl_cast<Form> (content);
		if(!form)
		{
			Rect rect (content->getSize ());
			rect.moveTo (pos);
			form = NEW Form (nullptr, rect, 0, windowClass->getTitle ());
			form->setTheme (theme);
			form->setWindowStyle (windowStyle);
			form->setSizeMode (content->getSizeMode ());
			form->setController (controller);
			if(!windowTitle.isEmpty ())
				form->setTitle (windowTitle);
			form->addView (content);
		}

		if(helpid.isEmpty ())
			helpid = form->getHelpIdentifier ();

		if(wasPluginViewHost)
		{
			// Workaround for issue with blank UIs: don't reuse plug-in window if old or new content use system scaling mode
			PlugInView::HostingMode newHostingMode = PlugInView::kDefaultHosting;
			PlugInView* newPlugInView = findPlugInView (*content);
			if(newPlugInView)
				newHostingMode = newPlugInView->getHostingMode ();

			if((newHostingMode == PlugInView::kSystemScaledHosting) || (oldPluginHostingMode == PlugInView::kSystemScaledHosting))
			{
				CCL_PRINTF ("PopupFrameItem: discard window (hostingmode: %d -> %d)\n", oldPluginHostingMode, newHostingMode)
				ASSERT (window)

				if(!window->isActive ())
					windowToReactivate = Desktop.getActiveWindow (); // new window will be activated on open, we will restore the previous one afterwards

				// open new window at same position (fake reuse, override saved position per WindowClass)
				Rect rect (form->getSize ());
				rect.moveTo (window->getSize ().getLeftTop ());
				form->setSize (rect);

				window->removeHandler (this);
				window->close ();
				window = nullptr;
				view = nullptr;
				updateCollector.window = sizeChangeCollector.window = nullptr;
			}
		}

		if(window)
		{
			// replace view in existing window
			window->updateSize ();
			
			form->setPosition (Point ());
			Rect size (window->getSize ());
			size.setSize (form->getSize ().getSize ());
			window->removeAll ();

			SizeLimit sizeLimits;
			sizeLimits.setUnlimited ();
			window->setSizeLimits (sizeLimits);

			window->setStyle (windowStyle);

			if(shouldFullscreen || shouldMaximize)
			{
				if((shouldFullscreen && window->isFullscreen ()) || (shouldMaximize && window->isMaximized ()))
				{
					Rect formSize;
					window->getClientRect (formSize);
					form->setSize (formSize);
				}
				else
					window->setSize (size);

				window->setUserSize (size);
			}
			else
				window->setSize (size);

			window->setTitle (form->getTitle ());
			window->setController (controller);
			window->setHelpIdentifier (helpid);
			window->addView (form);
			window->resetSizeLimits ();
			window->checkSizeLimits ();
		}
		else
		{
			if(pos.isNull () && isOtherMonitor ())
				tryOtherMonitor (*form);
			
			if((form->hasVisualStyle () == false) && content->hasVisualStyle ())
				form->setVisualStyle (unknown_cast<VisualStyle> (&content->getVisualStyle ()));
			
			window = form->open ();
			window->setHelpIdentifier (helpid);
			window->addHandler (this);
			view = window;
			if(shouldMaximize || shouldFullscreen)
				window->setUserSize (window->getSize ());

			if(windowToReactivate)
				windowToReactivate->activate ();
		}

		if(isInflate && storedSize.isNull ())
			window->inflate ();

		if(shouldFullscreen)
			setFullscreen (true);
		else if(shouldMaximize)
			maximize (true);

		setVisibleState (true);

		if(!titleParam)
		{
			titleParam = NEW StringParam;
			titleParam->setValue (windowTitle);
		}
		getTitleAlias ();
		setTitleParam (titleParam);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupFrameItem::hide ()
{
	if(titleAlias)
		setTitleParam (nullptr);

	if(isVisible ())
		onViewHidden ();

	if(Window* window = getWindow ())
	{
		view = nullptr;
		window->close ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* PopupFrameItem::getPinnedParam ()
{
	if(!pinned && isPinnable ())
	{
		pinned = NEW Parameter;
		pinned->connect (this, kPinnedTag);
	}
	return pinned;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* PopupFrameItem::getMaximizedParam ()
{
	if(!maximized)
	{
		maximized = NEW Parameter;
		maximized->connect (this, kMaximizedTag);

		Window* window = getWindow ();
		if(window && window->isMaximized ())
			maximized->setValue (true);
	}
	return maximized;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* PopupFrameItem::getFullscreenParam ()
{
	if(!fullscreen)
	{
		fullscreen = NEW Parameter;
		fullscreen->connect (this, kFullscreenTag);

		Window* window = getWindow ();
		if(window && window->isFullscreen ())
			fullscreen->setValue (true);
	}
	return fullscreen;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AliasParam* PopupFrameItem::getTitleAlias ()
{
	if(!titleAlias)
	{
		titleAlias = NEW AliasParam;
		titleAlias->addObserver (this);
	}
	return titleAlias;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API PopupFrameItem::findParameter (StringID name) const
{
	if(name == "maximized")
		return const_cast<PopupFrameItem*> (this)->getMaximizedParam ();
	if(name == "fullscreen")
		return const_cast<PopupFrameItem*> (this)->getFullscreenParam ();
	if(name == "title")
		return const_cast<PopupFrameItem*> (this)->getTitleAlias ();

	return SuperClass::findParameter (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupFrameItem::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "isWindow")
	{
		var = true;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupFrameItem::notify (ISubject* subject, MessageRef msg)
{
	if(titleAlias && subject == titleAlias)
	{
		if(msg == kChanged)
		{
			if(Window* window = getWindow ())
			{
				String windowTitle;
				titleAlias->toString (windowTitle);
				window->setTitle (windowTitle);
			}
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupFrameItem::onViewHidden ()
{
	isVisible (false);
	if(visible)
		visible->setValue (0);

	if(!getRootFrame () || !getRootFrame ()->isHidingAll ())
		if(getWindow ())
			saveViewState ();

	SuperClass::onViewHidden ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupFrameItem::onWindowEvent (WindowEvent& windowEvent)
{
	switch(windowEvent.eventType)
	{
	case WindowEvent::kDestroy:
		if(&windowEvent.window == getWindow ())
		{
			windowEvent.window.removeHandler (this);
			SharedPtr<PopupFrameItem> holder (this);
			onViewHidden ();
			view = nullptr;
		}
		break;

		case WindowEvent::kMaximize:
			if(maximized)
				maximized->setValue (true);
			break;

		case WindowEvent::kUnmaximize:
			if(maximized)
				maximized->setValue (false);
			break;

		case WindowEvent::kFullscreenEnter:
		case WindowEvent::kFullscreenLeave:
			if(fullscreen)
				fullscreen->setValue (windowEvent.eventType == WindowEvent::kFullscreenEnter);
			break;
	}
	return true;
}

//************************************************************************************************
// EmbeddedFrameItem vistors
//************************************************************************************************

class EmbeddedFrameItem::MarkAsEmbedded: public IDockPanelItemVisitor
{
	void visit (DockPanelItem& item) override
	{
		if(FrameItem* frameItem = ccl_cast<FrameItem> (&item))
			frameItem->isEmbedded (true);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

class EmbeddedFrameItem::ResetViewPointers: public IDockPanelItemVisitor
{
	void visit (DockPanelItem& item) override
	{
		if(FrameItem* frameItem = ccl_cast<FrameItem> (&item))
		{
			if(frameItem->isVolatile ())
			{
				ASSERT (frameItem->isEmbedded ())
				frameItem->resetContent ();
			}

			// save item state before
			frameItem->saveViewState ();
		}

		item.setView (nullptr);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

class EmbeddedFrameItem::UpdateWindowState: public IDockPanelItemVisitor
{
	void visit (DockPanelItem& item) override
	{
		if(item.getView ())
			if(FrameItem* frameItem = ccl_cast<FrameItem> (&item))
				frameItem->signalWindowState (true);
	}
};

//************************************************************************************************
// EmbeddedFrameItem
//************************************************************************************************

DEFINE_CLASS (EmbeddedFrameItem, FrameItem)

const CString EmbeddedFrameItem::kPropertyPrefix = CCL_PROPERTY_PREFIX;

//////////////////////////////////////////////////////////////////////////////////////////////////

EmbeddedFrameItem::EmbeddedFrameItem (int style)
: FrameItem (style)
{
	isHidable (false);
	initDefaultContent (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EmbeddedFrameItem::EmbeddedFrameItem (const EmbeddedFrameItem& item)
: FrameItem (item),
  parentClassID (item.parentClassID)
{
	isHidable (false);
	initDefaultContent (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EmbeddedFrameItem::initDefaultContent (FrameItem& item)
{
	bool hasContent = !item.getWindowID ().isEmpty ();

	// recursion
	ForEach (item, FrameItem, child)
		if(FrameItem* frameItem = ccl_cast<FrameItem> (child))
			if(initDefaultContent (*frameItem))
				hasContent = true;
	EndFor

	if(hasContent)
	{
		item.isVisible (true);

		if(DockPanelItem* parentItem = item.getParentItem ())
		{
			int index = parentItem->getIndex (&item, false);
			checkNeighbourDivider (parentItem, index, -1, false);
		}
	}
	return hasContent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EmbeddedFrameItem::setParentClassID (StringID classID)
{
	parentClassID = classID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString EmbeddedFrameItem::getParentClassID () const
{
	if(parentClassID.firstChar () == kPropertyPrefix[0])
	{
		// resolve property path
		Variant resolved;
		SkinExpressionParser::evaluate (resolved, String (parentClassID), Attributes ());
		return resolved.asString ();
	}
	return parentClassID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EmbeddedFrameItem::addItem (IDockPanelItem* item)
{
	if(DockPanelItem* dpItem = unknown_cast<FrameItem> (item))
	{
		MarkAsEmbedded me;
		dpItem->traverse (me);
	}
	return SuperClass::addItem (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EmbeddedFrameItem::onFrameViewAttached (View* frameView)
{
	setView (frameView);
	
	viewIsAppearing (true);
	initDefaultContent (*this);

	ForEach (*this, DockPanelItem, child)
		if(child->isVisible ())
			child->show ();
	EndFor

	UpdateWindowState u;
	traverse (u);

	viewIsAppearing (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EmbeddedFrameItem::onFrameViewRemoved ()
{
	ResetViewPointers r;
	traverse (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EmbeddedFrameItem::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "parentClassFrame")
	{
		MutableCString classID = getParentClassID ();
		AutoPtr<IRecognizer> r = Recognizer::create ([&] (IUnknown* obj)
		{
			FrameItem* frame = unknown_cast<FrameItem> (obj);
			return frame->getWindowID () == classID;
		});

		if(FrameItem* parentClassFrame = getPerspective ()->findFrameItem (*r))
			var = parentClassFrame->asUnknown ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// DetachSourceRecognizer
//************************************************************************************************

struct DetachSourceRecognizer: public Recognizer
{
	DetachedFrameItem& detachTarget;
	DetachSourceRecognizer (DetachedFrameItem& detachTarget) : detachTarget (detachTarget) {}

	tbool CCL_API recognize (IUnknown* object) const override
	{
		// looking for an open frame that contains a WindowClass matching the groups of the detached frame
		FrameItem* frame = unknown_cast<FrameItem> (object);
		if(frame && frame->getView () && !frame->isPinned ())
			if(const WindowClass* windowClass = frame->getCurrentWindowClass ())
				if(detachTarget.hasGroupID (windowClass->getGroupID ()))
					return true;

		return false;
	}
};

//************************************************************************************************
// DetachedFrameItem
//************************************************************************************************

DEFINE_CLASS (DetachedFrameItem, PopupFrameItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

DetachedFrameItem::DetachedFrameItem (int style)
: PopupFrameItem (style),
  detachedParam (nullptr),
  detached (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DetachedFrameItem::DetachedFrameItem (const DetachedFrameItem& item)
: PopupFrameItem (item),
  detachedParam (nullptr),
  detached (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DetachedFrameItem::~DetachedFrameItem ()
{
	cancelSignals ();
	if(detachedParam)
		detachedParam->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* CCL_API DetachedFrameItem::findParameter (StringID name) const
{
	if(name == "detached")
		return const_cast<DetachedFrameItem*> (this)->getDetachedParam ();

	return SuperClass::findParameter (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Parameter* DetachedFrameItem::getDetachedParam ()
{
	if(!detachedParam)
	{
		detachedParam = NEW Parameter ("detached");
		detachedParam->connect (this, kDetachedTag);
	}
	return detachedParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DetachedFrameItem::setDetached (bool state)
{
	if(state != detached)
	{
		detached = state;
		getDetachedParam ()->setValue (state);

		if(RootFrameItem* rootFrame = getRootFrame ())
		{
			Perspective* perspective = rootFrame->getPerspective ();
			Workspace* workspace = perspective ? perspective->getWorkspace () : nullptr;
			if(workspace)
			{
				if(detached)
				{
					// find source frame
					const WindowClass* sourceClass = nullptr;

					DetachSourceRecognizer recognizer (*this);
					if(FrameItem* sourceFrame = perspective->findFrameItem (recognizer))
						sourceClass = sourceFrame->getCurrentWindowClass ();

					if(sourceClass)
					{
						// close source frame, open class in this frame 
						workspace->closeView (*sourceClass);
						workspace->openView (*sourceClass);
					}
				}
				else
				{
					const WindowClass* windowClass = getCurrentWindowClass ();
					bool wasOpen = getView () && windowClass;
					if(wasOpen)
					{
						// close this frame, open class in another frame
						workspace->closeView (*windowClass);
						workspace->openView (*windowClass);
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DetachedFrameItem::onViewHidden ()
{
	// instances of a detached multiple frames lose their "pinned" state when closing
	if(isPinned () && ccl_cast<MultiFrameItem> (getParentItem ()))
		setPinned (false);

	SuperClass::onViewHidden ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DetachedFrameItem::paramChanged (IParameter* param)
{
	if(param->getTag () == kDetachedTag)
	{
		(NEW Message ("setDetached", param->getValue ()))->post (this);
		return true;
	}
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DetachedFrameItem::notify (ISubject* subject, MessageRef msg)
{
    SharedPtr<DetachedFrameItem> keeper (this);
    if(msg == "setDetached")
		setDetached (msg[0].asBool ());

	return SuperClass::notify (subject, msg);
}

//************************************************************************************************
// SharedDetachedFrameItem
//************************************************************************************************

DEFINE_CLASS (SharedDetachedFrameItem, DetachedFrameItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

SharedDetachedFrameItem* SharedDetachedFrameItem::findOpenItem ()
{
	Boxed::String* targetName = static_cast<Boxed::String*> (groupIDs.at (0));
	if(targetName)
	{
		AutoPtr<IRecognizer> r = Recognizer::create ([&] (IUnknown* unk)
		{
			SharedDetachedFrameItem* item = unknown_cast<SharedDetachedFrameItem> (unk);
			return item && item != this && item->getWindow () && item->hasGroupID (*targetName);
		});
		return static_cast<SharedDetachedFrameItem*> (getPerspective ()->findFrameItem (*r));
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* SharedDetachedFrameItem::openView (WindowClassRef wc)
{
	CCL_PRINTF ("SharedDetachedFrameItem::openView %s, %s\n", MutableCString (getObjectID ()).str (), wc.getID ().str ());
	SharedDetachedFrameItem* otherItem = findOpenItem ();
	if(otherItem)
	{
		CCL_PRINTF ("hide: %s, %s\n", MutableCString (otherItem->getObjectID ()).str (), MutableCString (otherItem->getWindowID ()).str ());

		const WindowClass* replacedClass = otherItem->getCurrentWindowClass ();
		UnknownPtr<IObserver> controller (replacedClass ? replacedClass->getController () : nullptr);
		if(controller)
			controller->notify (this, Message (IWorkspace::kReplacingView, String (replacedClass->getID ()), String (wc.getID ())));

		otherItem->hide ();
	}
	return SuperClass::openView (wc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SharedDetachedFrameItem::onViewHidden ()
{
	CCL_PRINTF ("SharedDetachedFrameItem::onViewHidden %s, %s\n", MutableCString (getObjectID ()).str (), windowID.str ())
	SuperClass::onViewHidden ();
}

//************************************************************************************************
// SystemFrameItem
//************************************************************************************************

DEFINE_CLASS (SystemFrameItem, FrameItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

SystemFrameItem::SystemFrameItem (int style)
: FrameItem (style)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SystemFrameItem::SystemFrameItem (const SystemFrameItem& item)
: FrameItem (item)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemFrameItem::show ()
{
	if(windowClass)
	{
		Theme* theme = windowClass->getTheme ();
		ASSERT (theme)
		if(!theme)
			return;

		View* content = createViewInternal (*windowClass);
		if(!content)
			return;

		if(getName () == "StatusBar")
			WindowManager::instance ().setStatusBarView (content);
		else if(getName () == "NavigationBar")
			WindowManager::instance ().setNavigationBarView (content);
		else if(getName () == "LeftMargin")
			WindowManager::instance ().setLeftMarginView (content);
		else if(getName () == "RightMargin")
			WindowManager::instance ().setRightMarginView (content);
		else
		{
			content->release ();
			return;
		}

		setVisibleState (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SystemFrameItem::hide ()
{
	isVisible (false);
	if(visible)
		visible->setValue (0);

	onViewHidden ();

	// check if a corresponding system frame's content in the application workspace has to be restored if the hidden frame was part of another workspace
	SystemFrameItem* systemFrame = findCounterpartInAppWorkspace ();
	if(systemFrame && systemFrame->isVisible ())
	{
		systemFrame->show ();
		return;
	}

	if(getName () == "StatusBar")
		WindowManager::instance ().setStatusBarView (nullptr);
	else if(getName () == "NavigationBar")
		WindowManager::instance ().setNavigationBarView (nullptr);
	else if(getName () == "LeftMargin")
		WindowManager::instance ().setLeftMarginView (nullptr);
	else if(getName () == "RightMargin")
		WindowManager::instance ().setRightMarginView (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SystemFrameItem* SystemFrameItem::findCounterpartInAppWorkspace () const
{
	// find SystemFrameItem with same name in application workspace
	if(IApplication* application = GUI.getApplication ())
	{
		Workspace* appWorkspace = unknown_cast<Workspace> (WorkspaceSystem::instance ().getWorkspace (application->getApplicationID ()));
		if(appWorkspace && appWorkspace != getWorkspace ())
		{
			AutoPtr<IRecognizer> recognizer (Recognizer::create ([&] (IUnknown* object)
			{
				auto* systemFrame = unknown_cast<SystemFrameItem> (object);
				return systemFrame && systemFrame->getName () == getName ();
			}));
			return ccl_cast<SystemFrameItem> (appWorkspace->findFrameItem (*recognizer));
		}
	}
	return nullptr;
}
