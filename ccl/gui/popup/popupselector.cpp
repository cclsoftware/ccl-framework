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
// Filename    : ccl/gui/popup/popupselector.cpp
// Description : Popup Selector
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/popup/menupopupselector.h"
#include "ccl/gui/popup/parametermenubuilder.h"
#include "ccl/gui/popup/palettepopup.h"
#include "ccl/gui/popup/extendedmenu.h"
#include "ccl/gui/popup/menucontrol.h"
#include "ccl/gui/popup/popupslider.h"

#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/views/focusnavigator.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/views/viewdecorator.h"
#include "ccl/gui/skin/form.h"
#include "ccl/gui/keyevent.h"

#include "ccl/gui/theme/visualstyle.h"
#include "ccl/gui/theme/thememanager.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/gui.h"

#include "ccl/base/storage/configuration.h"
#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (PopupSelector::popupStyles)
	{"left",	PopupSizeInfo::kLeft},
	{"right",	PopupSizeInfo::kRight},
	{"hcenter",	PopupSizeInfo::kHCenter},
	{"hmouse",	PopupSizeInfo::kHMouse},
	{"top",		PopupSizeInfo::kTop},
	{"bottom",	PopupSizeInfo::kBottom},
	{"vcenter",	PopupSizeInfo::kVCenter},
	{"vmouse",	PopupSizeInfo::kVMouse},
	{"offset",	PopupSizeInfo::kHasOffset},
	{"hfillwindow",	PopupSizeInfo::kHFillWindow},
	{"vfillwindow",	PopupSizeInfo::kVFillWindow},
END_STYLEDEF
	
//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IPopupSelectorWindow, 0x82c59979, 0x2a25, 0x4ea1, 0x90, 0xb6, 0x45, 0xd9, 0xc1, 0xea, 0xb4, 0x71)
DEFINE_IID_ (INativePopupSelectorWindow, 0xc19afe6c, 0x35b9, 0x11ed, 0xa3, 0x16, 0xc8, 0xff, 0x28, 0x15, 0x7a, 0x9d)

//************************************************************************************************
// PopupArranger
//* Helper for sizing a popup, decides if it should popup upwards or downwards from the start point. */
//************************************************************************************************

class PopupArranger
{
public:
	PopupArranger (const PopupSizeInfo& sizeInfo);

	SizeLimit& getLimits (SizeLimit& limits);	///< advice before creating a view
	Point getWindowLocation (View& view);		///< decision for existing view

	Window* getParentWindow () const;
	int getMonitor () const { return monitor; }

private:
	const PopupSizeInfo& sizeInfo;
	int hAlign;
	int vAlign;
	View* parent;
	bool wantRightwards;	///< extend rightwards from calculated position if enough space
	bool wantDownwards;		///< extend downwards from calculated position if enough space

	int monitor;
	Rect monitorSize;
	Point screenPos;
	Point mirroredPos;
	Coord downAvailable;	///< available screensize downwards from start point
	Coord upAvailable;		///< available screensize upwards from start point
	Coord leftAvailable;	///< available screensize left from start point
	Coord rightAvailable;	///< available screensize right from start point
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_VISUALSTYLE_CLASS (PopupSelector, VisualStyle, "PopupSelectorStyle")
	ADD_VISUALSTYLE_METRIC  ("popup.offset.x")			///< an additional horizontal offset to the popup position, applied when opened from SelectBox or PopupBox
	ADD_VISUALSTYLE_METRIC  ("popup.offset.y")			///< an additional vertical offset to the popup position, applied when opened from SelectBox or PopupBox
	ADD_VISUALSTYLE_METRIC  ("popup.minwidth")			///< the popup's minwidth sizelimit - used to override the default sizelimit (the width of the PopupBox)
END_VISUALSTYLE_CLASS (PopupSelector)

//////////////////////////////////////////////////////////////////////////////////////////////////

const Configuration::IntValue popupMarginLeft	("GUI.PopupMargin", "Left", 0);
const Configuration::IntValue popupMarginTop	("GUI.PopupMargin", "Top", 0);
const Configuration::IntValue popupMarginRight	("GUI.PopupMargin", "Right", 0);
const Configuration::IntValue popupMarginBottom	("GUI.PopupMargin", "Bottom", 0);

//************************************************************************************************
// PopupArranger
//************************************************************************************************

PopupArranger::PopupArranger (const PopupSizeInfo& sizeInfo)
: sizeInfo (sizeInfo),
  hAlign (sizeInfo.flags & PopupSizeInfo::kHMask),
  vAlign (sizeInfo.flags & PopupSizeInfo::kVMask),
  parent (unknown_cast<View> (sizeInfo.parent)),
  monitor (-1),
  wantRightwards (true),
  wantDownwards (true)
{
	bool mouseInsideParent = false;
	Point mousePos;
	GUI.getMousePosition (mousePos);

	if(!parent)
	{
		parent = unknown_cast<View> (Desktop.getApplicationWindow ());
		if(!parent)
			parent = unknown_cast<View> (Desktop.getDialogParentWindow ());
	}

	ASSERT (parent)
	if(parent)
	{
		// determine position relative to parent
		bool vEdge = false;
		bool hEdge = false;
		switch(hAlign)
		{
			case PopupSizeInfo::kLeft:
				screenPos.x = 0;
				vEdge = true;
				break;
			case PopupSizeInfo::kRight:
				screenPos.x = parent->getSize ().getWidth ();
				wantRightwards = false;
				vEdge = true;
				break;
			case PopupSizeInfo::kHCenter:
				screenPos.x = parent->getSize ().getWidth () / 2;
				break;
			default:
				screenPos.x = sizeInfo.where.x;
				break;
		}
		switch(vAlign)
		{
			case PopupSizeInfo::kTop:
				screenPos.y = 0;
				wantDownwards = false;
				hEdge = true;
				break;
			case PopupSizeInfo::kBottom:
				screenPos.y = parent->getSize ().getHeight ();
				hEdge = true;
				break;
			case PopupSizeInfo::kVCenter:
				screenPos.y = parent->getSize ().getHeight () / 2;
				break;
			default:
				screenPos.y = sizeInfo.where.y;
				break;
		}

		if(sizeInfo.flags & PopupSizeInfo::kHasOffset)
			screenPos += sizeInfo.where; // add as offset to calculated position
		else
		{
			// switch direction if only aligned on one edge
			if(vEdge && !hEdge && !(sizeInfo.flags & (PopupSizeInfo::kHFillWindow)))
				wantRightwards = !wantRightwards;
			else if(hEdge && !vEdge && !(sizeInfo.flags & (PopupSizeInfo::kVFillWindow)))
				wantDownwards = !wantDownwards;
		}

		// translate to screen and mirror on parent center
		if(sizeInfo.canFlipParentEdge ())
			mirroredPos = parent->getSize ().getSize () - screenPos;
		else
			mirroredPos = screenPos;

		parent->clientToScreen (screenPos);
		parent->clientToScreen (mirroredPos);

		// check if mouse is inside parent's client rect
		Point mouseParent (mousePos);
		parent->screenToClient (mouseParent);
		mouseParent += parent->getSize ().getLeftTop ();
		mouseInsideParent = parent->getSize ().pointInside (mouseParent);
	}

	// align at mouse position (never mirrored)
	if(sizeInfo.flags & (PopupSizeInfo::kHMouse|PopupSizeInfo::kVMouse))
	{
		if(hAlign == PopupSizeInfo::kHMouse)
		{
			screenPos.x = mirroredPos.x = mousePos.x;
			if(sizeInfo.flags & PopupSizeInfo::kHasOffset)
				screenPos.x += sizeInfo.where.x;
		}

		if(vAlign == PopupSizeInfo::kVMouse)
		{
			screenPos.y = mirroredPos.y = mousePos.y;
			if(sizeInfo.flags & PopupSizeInfo::kHasOffset)
				screenPos.y += sizeInfo.where.y;
		}
	}

	// get monitor size: find monitor
	monitor = -1;

	// if mouse is in parent view, prefer monitor under mouse (seems to to the best criteria; had problems with coord at edge of monitor, e.g. in maximized window)
	if(mouseInsideParent)
		monitor = Desktop.findMonitor (mousePos, false);
	if(monitor < 0)
	{
		auto constrainToParentWindow = [&] (Point pos)
		{
			if(sizeInfo.flags & (PopupSizeInfo::kHFillWindow|PopupSizeInfo::kVFillWindow))
				if(Window* parentWindow = getParentWindow ())
				{
					if(sizeInfo.flags & PopupSizeInfo::kHFillWindow)
						pos.x = parentWindow->getSize ().getCenter ().x;
					if(sizeInfo.flags & PopupSizeInfo::kVFillWindow)
						pos.y = parentWindow->getSize ().getCenter ().y;
				}
			return pos;
		};

		monitor = Desktop.findMonitor (constrainToParentWindow (screenPos), false);
		if(monitor < 0)
			monitor = Desktop.findMonitor (constrainToParentWindow (mirroredPos), true);
	}

	Desktop.getMonitorSize (monitorSize, monitor, true);
	monitorSize.left += popupMarginLeft;
	monitorSize.top += popupMarginTop;
	monitorSize.right -= popupMarginRight;
	monitorSize.bottom -= popupMarginBottom;
	#if !CCL_PLATFORM_DESKTOP
	if(auto appWindow = unknown_cast<Window> (Desktop.getApplicationWindow ()))
		monitorSize.bound (appWindow->getSize ());
	#endif

	ccl_lower_limit (screenPos.y, monitorSize.top);
	ccl_lower_limit (mirroredPos.y, monitorSize.top);

	Rect screenRect (monitorSize);

	if(sizeInfo.flags & (PopupSizeInfo::kHFillWindow|PopupSizeInfo::kVFillWindow))
		if(Window* parentWindow = getParentWindow ())
		{
			Point screenOffset;
			parentWindow->clientToScreen (screenOffset);
			Rect parentWindowSize (screenOffset.x, screenOffset.y, parentWindow->getSize ().getSize ());
			parentWindowSize.bound (monitorSize);

			// constrain to window instead of monitor
			if(sizeInfo.flags & PopupSizeInfo::kHFillWindow)
			{
				screenRect.left = parentWindowSize.left;
				screenRect.right = parentWindowSize.right;
			}
			if(sizeInfo.flags & PopupSizeInfo::kVFillWindow)
			{
				screenRect.top = parentWindowSize.top;
				screenRect.bottom = parentWindowSize.bottom;
			}
		}

	// determine available space from original and mirrored position
	if(hAlign == PopupSizeInfo::kHCenter && sizeInfo.parent) // if we center the view (no fix position), we have the whole parent size available
		leftAvailable = rightAvailable = sizeInfo.parent->getSize ().getWidth ();
	if(wantRightwards)
	{
		leftAvailable	= mirroredPos.x - screenRect.left;
		rightAvailable	= screenRect.right - screenPos.x;
	}
	else
	{
		leftAvailable	= screenPos.x - screenRect.left;
		rightAvailable	= screenRect.right - mirroredPos.x;
	}

	if(vAlign == PopupSizeInfo::kVCenter && sizeInfo.parent)
		upAvailable	= downAvailable = sizeInfo.parent->getSize ().getHeight ();
	else if(wantDownwards)
	{
		upAvailable		= mirroredPos.y - screenRect.top;
		downAvailable	= screenRect.bottom - screenPos.y;
	}
	else
	{
		upAvailable		= screenPos.y - screenRect.top;
		downAvailable	= screenRect.bottom - mirroredPos.y;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* PopupArranger::getParentWindow () const
{
	return parent ? parent->getWindow () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SizeLimit& PopupArranger::getLimits (SizeLimit& limits)
{
	Coord maxH = ccl_max (downAvailable, upAvailable);

	Coord defaultMaxH = 600;
	Coord defaultMaxW = 800;
	if(ThemeSelector::currentTheme)
		if(VisualStyle* style = ThemeSelector::currentTheme->getStandardStyle (ThemePainter::kPopupMenuStyle))
		{
			defaultMaxH = style->getMetric ("maxH", defaultMaxH);
			defaultMaxW = style->getMetric ("maxW", defaultMaxW);
		}

	if(sizeInfo.sizeLimits.maxHeight == kMaxCoord)
		maxH = ccl_min (maxH, defaultMaxH);

	limits = sizeInfo.sizeLimits;

	ccl_lower_limit (limits.minWidth, 40);
	ccl_upper_limit (limits.maxWidth, ccl_max (defaultMaxW, limits.minWidth)); // prevent min/max conflict: prefer sizeInfo.minWidth
	ccl_upper_limit (limits.maxHeight, maxH);
	return limits;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point PopupArranger::getWindowLocation (View& view)
{
	#if !CCL_PLATFORM_DESKTOP
	// ensure popup fits inside (constrained) monitor size
	Rect size (view.getSize ());
	if(size.getHeight () > monitorSize.getHeight ())
	{
		size.top = monitorSize.top;
		size.bottom = monitorSize.bottom;
	}
	if(size.getWidth () > monitorSize.getWidth ())
	{
		size.left = monitorSize.left;
		size.right = monitorSize.right;
	}
	view.setSize (size);
	#endif

	Point popupSize (view.getSize ().getSize ());

	Coord freeDown	= downAvailable - popupSize.y;
	Coord freeUp	= upAvailable   - popupSize.y;
	Coord freeRight	= rightAvailable - popupSize.x;
	Coord freeLeft	= leftAvailable  - popupSize.x;

	if(sizeInfo.flags & PopupSizeInfo::kHFillWindow)
		popupSize.x = 0;
	if(sizeInfo.flags & PopupSizeInfo::kVFillWindow)
		popupSize.y = 0;

	// select directions

	bool rightWards = wantRightwards;
	bool downWards  = wantDownwards;
	
	if(sizeInfo.forceFixedPosition () == false)
	{
		rightWards = wantRightwards ? (freeRight >= 0 || freeRight >= freeLeft) : (freeLeft < 0 && freeLeft < freeRight);
		downWards  = wantDownwards  ? (freeDown >= 0 || freeDown >= freeUp)     : (freeUp < 0 && freeUp < freeDown);
	}

	Point p;
	p.x = rightWards == wantRightwards ? screenPos.x : mirroredPos.x;
	p.y = downWards  == wantDownwards  ? screenPos.y : mirroredPos.y;

	if(hAlign == PopupSizeInfo::kHCenter || hAlign == PopupSizeInfo::kHCenterRel)
		p.x -=  popupSize.x / 2;
	else if(!rightWards)
		p.x -= popupSize.x;

	if(vAlign == PopupSizeInfo::kVCenter || vAlign == PopupSizeInfo::kVCenterRel)
		p.y -=  popupSize.y / 2;
	else if(!downWards)
		p.y -= popupSize.y;

	if(!sizeInfo.forceFixedPosition ())
	{
		// keep inside monitor
		ccl_lower_limit (p.x, monitorSize.left);
		ccl_lower_limit (p.y, monitorSize.top);
		ccl_upper_limit (p.x, monitorSize.right - popupSize.x);
		ccl_upper_limit (p.y, monitorSize.bottom - popupSize.y);
	}

	if(sizeInfo.flags & (PopupSizeInfo::kHFillWindow|PopupSizeInfo::kVFillWindow))
	{
		if(Window* parentWindow = getParentWindow ())
		{
			Point screenOffset;
			parentWindow->clientToScreen (screenOffset);
			Rect parentWindowSize (screenOffset.x, screenOffset.y, parentWindow->getSize ().getSize ());

			// first adjust position to stay inside parent window
			if(sizeInfo.flags & PopupSizeInfo::kHFillWindow)
			{
				ccl_lower_limit (p.x, parentWindowSize.left);
				ccl_upper_limit (p.x, parentWindowSize.right - popupSize.x);
			}
			if(sizeInfo.flags & PopupSizeInfo::kVFillWindow)
			{
				ccl_lower_limit (p.y, parentWindowSize.top);
				ccl_upper_limit (p.y, parentWindowSize.bottom - popupSize.y);
			}

			Point clientPos (p - screenOffset); // position (so far) in parent's client coords
			Rect size (view.getSize ());
			SizeLimit limits (view.getSizeLimits ()); 

			auto setViewHeight = [&] (Coord height)
			{
				ccl_upper_limit (height, limits.maxHeight);
				Coord diff = height - size.getHeight ();
				size.setHeight (height);
				limits.setFixedHeight (height);
				return diff;
			};

			auto setViewWidth = [&] (Coord width)
			{
				ccl_upper_limit (width, limits.maxWidth);
				Coord diff = width - size.getWidth ();
				size.setWidth (width);
				limits.setFixedWidth (width);
				return diff;
			};

			if(sizeInfo.flags & PopupSizeInfo::kHFillWindow)
			{
				switch(hAlign)
				{
				case PopupSizeInfo::kLeft:
					if(clientPos.x > 0)
					{
						// keep right aligned to parent left, enlarge towards window left
						Coord diff = setViewWidth (size.getWidth () + clientPos.x);
						p.x -= diff;
					}
					else
					{
						// try full window width (from window left)
						setViewWidth (parentWindowSize.getWidth ());
					}
					break;
				case PopupSizeInfo::kRight:
					// keep left aligned to parent right, enlarge towards window right
					setViewWidth (parentWindowSize.getWidth () - clientPos.x);
					break;

				case PopupSizeInfo::kHCenter:
				case PopupSizeInfo::kHCenterRel:
					ASSERT (0) // this combination doesn't make sense, center in window instead

				default: // no vertical alignment option
					// enlarge to full window width, center in window if limit is smaller
					setViewWidth (parentWindowSize.getWidth ());
					p.x = (parentWindowSize.getWidth () - size.getWidth ()) / 2 + screenOffset.x;
					break;
				}
			}

			if(sizeInfo.flags & PopupSizeInfo::kVFillWindow)
			{
				switch(vAlign)
				{
				case PopupSizeInfo::kTop:
					if(clientPos.y > 0)
					{
						// keep bottom aligned to parent top, enlarge towards window top
						Coord diff = setViewHeight (size.getHeight () + clientPos.y);
						p.y -= diff;
					}
					else
					{
						// try full window height (from window top)
						setViewHeight (parentWindowSize.getHeight ());
					}
					break;
				case PopupSizeInfo::kBottom:
					// keep top aligned to parent bottom, enlarge towards window bottom
					setViewHeight (parentWindowSize.getHeight () - clientPos.y);
					break;

				case PopupSizeInfo::kVCenter:
				case PopupSizeInfo::kVCenterRel:
					ASSERT (0) // this combination doesn't make sense, center in window instead

				default: // no vertical alignment option
					// enlarge to full window height, center in window if limit is smaller
					setViewHeight (parentWindowSize.getHeight ());
					p.y = (parentWindowSize.getHeight () - size.getHeight ()) / 2 + screenOffset.y;
					break;
				}
			}
			view.setSize (size);
			view.setSizeLimits (limits);
		}
	}

	return p;
}

//************************************************************************************************
// PopupSelector::PopupDecorator
//************************************************************************************************

class PopupSelector::PopupDecorator: public ViewDecorator
{
public:
	PopupDecorator (PopupSelector* popupSelector, View* contentView)
	: ViewDecorator (contentView, popupSelector->getDecorName (), popupSelector->getDecorController ())
	{
		getDecorArguments ().setAttribute ("PopupSelector", Variant (this->asUnknown ()));
	}
};

//************************************************************************************************
// PopupSelectorWindow
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PopupSelectorWindow, Dialog)

static bool swallowDoubleClick = false;

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorWindow::PopupSelectorWindow (Window* _parentWindow, IPopupSelectorClient* _client, const Rect& size, StyleRef style, StringRef title)
: Dialog (size, style, title),
  popupResult (IPopupSelectorClient::kCancel),
  parentAutoSeeThru (false),
  isClosing (false),
  owner (nullptr)
{
	parentWindow = _parentWindow;
	layer = kPopupLayer;
	client.share (_client);

	anchorRect.setReallyEmpty ();

	if(_parentWindow)
	{
		// on macOS the mousehandler of the parent can survive (and swallow mouse move events)
		_parentWindow->setMouseHandler (nullptr);
		
		StyleFlags style (_parentWindow->getStyle ());
		if(style.isCustomStyle (Styles::kWindowBehaviorAutoSeeThru))
		{
			// disable parent's autoSeeThru style while this is open
			style.setCustomStyle (Styles::kWindowBehaviorAutoSeeThru, false);
			_parentWindow->setStyle (style);
			parentAutoSeeThru = true;
		}
	}

#if 0 && DEBUG // for logging in DesktopManager::onActivateWindow
	UnknownPtr<IObject> obj (_client);
	if(obj)
		setName (obj->getTypeInfo ().getClassName ());
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PopupSelectorWindow::queryInterface (CCL::UIDRef iid, void** ptr)
{
	if(ccl_iid<IPopupSelectorClient> ().equals (iid) && client)
	{
		*ptr = client;
		client->retain();
		return kResultOk;
	}
	QUERY_INTERFACE (IPopupSelectorWindow)
	return SuperClass::queryInterface (iid, ptr); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupSelectorWindow::attached (View* parent)
{
	CCL_PRINTLN ("PopupSelectorWindow::attached")
	SuperClass::attached (parent);

	if(client)
		client->attached (*this);

	if(owner && (owner->getBehavior () & IPopupSelector::kAcceptsOnClickOutside))
		setPopupResult (IPopupSelectorClient::kOkay);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupSelectorWindow::onPopupDeactivated ()
{
	// popup window deactivated (eg. clicked outside): close if no other modal window exists above us
	if(Desktop.getTopWindow (kPopupLayer) == this)
	{
		// defer closing if we are inside a drag session that was started from this window
		DragSession* dragSession = DragSession::getActiveSession ();
		View* sourceView = dragSession ? unknown_cast<View> (dragSession->getSource ()) : nullptr;
		if(sourceView && sourceView->getWindow () == this)
			dragSession->addObserver (this); // defer closing after dragging is over
		else
		{
			if(!PopupMenu::isPlatformMenuActive ())
			{
				if(MenuControl::PopupClient* menuControlClient = unknown_cast<MenuControl::PopupClient> (client))
					menuControlClient->closeAll (true);
				else
					checkClientResult (getPopupResult ());

				swallowDoubleClick = true;
			}
		}
	}

	bool result = true;
	// owner might be deallocated already
	if(!isInDestroyEvent ())
		// in menu mode, the event causing deactivation should pass through
		result = !owner->isMenuMode ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupSelectorWindow::notify (ISubject* subject, MessageRef msg)
{
	DragSession* dragSession = unknown_cast<DragSession> (subject);
	if(dragSession && msg == "endDrag")
	{
		// dragging is over
		dragSession->removeObserver (this);

		if(owner->getBehavior () & IPopupSelector::kCloseAfterDrag)
			closeWindow ();
		else
			activate (); // activate this
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupSelectorWindow::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "popupResult")
	{
		setPopupResult (var.asBool () ? IPopupSelectorClient::kOkay : IPopupSelectorClient::kCancel);
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupSelectorWindow::onClose ()
{
	isClosing = true;

	if(parentAutoSeeThru)
	{
		// restore parent's autoSeeThru style
		Window* w = unknown_cast<Window> (parentWindow);
		if(w)
			View::StyleModifier (*w).setCustomStyle (Styles::kWindowBehaviorAutoSeeThru, true);
	}
	return SuperClass::onClose ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupSelectorWindow::onMouseDown (const MouseEvent& event)
{
	if(client && checkClientResult (client->onMouseDown (event, *this)))
		return true;

	bool result = SuperClass::onMouseDown (event);
	if(client && checkClientResult (client->onEventProcessed (event, *this, nullptr)))
		result = true;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupSelectorWindow::onMouseUp (const MouseEvent& event)
{
	bool wantsMouseOutside = (owner->getBehavior () & IPopupSelector::kWantsMouseUpOutside) != 0;

	if(!wantsMouseOutside)
		if(PopupSelector::didMouseHandlerEscape (this, event))
			return SuperClass::onMouseUp (event);
	
	// ignore if window is already closing (depending on the platform, a mouse up / touch end event might be delivered or not)
	bool inside = !isClosing && (isInsideClient (event.where) || wantsMouseOutside);
	bool result = false;

	if(inside && client && checkClientResult (client->onMouseUp (event, *this)))
		result = true;

	if(SuperClass::onMouseUp (event))
		result = true;

	if(result)
		return true;

	if(inside && client && checkClientResult (client->onEventProcessed (event, *this, nullptr)))
		result = true;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupSelectorWindow::onKeyDown (const KeyEvent& event)
{
	if(client && checkClientResult (client->onKeyDown (event)))
		return true;

	bool result = Window::onKeyDown (event);
	if(client && checkClientResult (client->onEventProcessed (event, *this, nullptr)))
		result = true;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupSelectorWindow::onKeyUp (const KeyEvent& event)
{
	if(client && checkClientResult (client->onKeyUp (event)))
		return true;

	bool result = Window::onKeyUp (event);
	if(client && checkClientResult (client->onEventProcessed (event, *this, nullptr)))
		result = true;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupSelectorWindow::onGestureProcessed (const GestureEvent& event, View* view)
{
	// check if client accepts
	if(client)
		checkClientResult (client->onEventProcessed (event, *this, view));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupSelectorWindow::checkClientResult (IPopupSelectorClient::Result result)
{
	if(result != IPopupSelectorClient::kIgnore)
	{
		if(result != IPopupSelectorClient::kSwallow)
		{
			popupResult = result;
			if(popupResult == IPopupSelectorClient::kOkay)
				setDialogResult (DialogResult::kOkay);
			else if(popupResult == IPopupSelectorClient::kCancel)
				setDialogResult (DialogResult::kCancel);

			closeWindow ();
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupSelectorWindow::setSize (RectRef newSize, tbool doInvalidate)
{
	Rect adjustedSize (newSize);
	if(newSize != size)
		owner->adjustWindowSize (adjustedSize);

	SuperClass::setSize (adjustedSize, doInvalidate);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupSelectorWindow::closeWindow ()
{
	if(!isClosing)
	{
		// need to activate parent before closing, otherwise a random window might get activated on macOS
		if(IWindow* parentWindow = getParentWindow ())
			parentWindow->activate ();
		isClosing = true;
		deferClose ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupSelectorWindow::closePopup ()
{
	close ();
}

//************************************************************************************************
// NonModalPopupSelectorWindow
//************************************************************************************************

DEFINE_CLASS (NonModalPopupSelectorWindow, PopupWindow)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NonModalPopupSelectorWindow::shouldSwallowClosingEvent ()
{
	// A click / tap outside closes the popup (see processForeignEvent).
	// Based on platform conventions (similar to a click in an inactive window), it should be either swallowed or processed by the underlying window
	#if CCL_PLATFORM_WINDOWS
	return false;
	#else
	return true;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NonModalPopupSelectorWindow::processForeignEvent (const GUIEvent& event, Window* window)
{
	// close non-modal popup on mouseclick or tap in another window
	auto checkEvent = [&] ()
	{
		if(event.eventClass == GUIEvent::kMouseEvent)
			return event.eventType == MouseEvent::kMouseDown;

		if(event.eventClass == GUIEvent::kTouchEvent)
			return event.eventType == TouchEvent::kBegin;

		if(auto gestureEvent = event.as<GestureEvent> ())
			return (gestureEvent->getType () == GestureEvent::kSingleTap && gestureEvent->getState () == GestureEvent::kBegin)
				|| (gestureEvent->getType () == GestureEvent::kDoubleTap && gestureEvent->getState () == GestureEvent::kPossible);

		return false;
	};

	if(checkEvent ())
	{
		// ignore click in spy (don't close popup)
		if(window && window->getTitle () == CCL_SPY_NAME)
			return false;

		int numWindows = Desktop.countWindows ();
		for(int i = numWindows - 1; i > window->getZIndex (); i--)
		{
			auto* nonModalPopup = unknown_cast<NonModalPopupSelectorWindow> (Desktop.getWindow (i));
			if(!nonModalPopup || event.eventTime <= nonModalPopup->getAttachedTime ()) // ignore if event was created before window was opened
				continue;

			nonModalPopup->close ();

			if(shouldSwallowClosingEvent ())
				return true;

			break;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NonModalPopupSelectorWindow::NonModalPopupSelectorWindow (Window* _parentWindow, IPopupSelectorClient* _client, const Rect& size, StyleRef style, StringRef title)
: PopupWindow (size, style, title),
  popupResult (IPopupSelectorClient::kCancel),
  asyncOperation (NEW AsyncOperation),
  owner (nullptr),
  attachedTime (0)
{
	parentWindow = _parentWindow;
	layer = kPopupLayer;
	client.share (_client);

	if(_parentWindow)
		_parentWindow->setMouseHandler (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NonModalPopupSelectorWindow::~NonModalPopupSelectorWindow ()
{
	asyncOperation->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NonModalPopupSelectorWindow::queryInterface (CCL::UIDRef iid, void** ptr)
{
	if(ccl_iid<IPopupSelectorClient> ().equals (iid) && client)
	{
		*ptr = client;
		client->retain();
		return kResultOk;
	}
	QUERY_INTERFACE (IPopupSelectorWindow)
	return SuperClass::queryInterface (iid, ptr); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AsyncOperation* NonModalPopupSelectorWindow::getAsyncOperation () const
{
	return asyncOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NonModalPopupSelectorWindow::attached (View* parent)
{
	CCL_PRINTLN ("NonModalPopupSelectorWindow::attached")
	SuperClass::attached (parent);

	if(client)
		client->attached (*this);

	if(owner && (owner->getBehavior () & IPopupSelector::kAcceptsOnClickOutside))
		setPopupResult (IPopupSelectorClient::kOkay);

	setAttachedTime (System::GetProfileTime ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NonModalPopupSelectorWindow::setSize (RectRef newSize, tbool doInvalidate)
{
	Rect adjustedSize (newSize);
	if(newSize != size)
		owner->adjustWindowSize (adjustedSize);

	SuperClass::setSize (adjustedSize, doInvalidate);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NonModalPopupSelectorWindow::closePopup ()
{
	Desktop.removeWindow (this); // e.g. to prevent using this as a dialog parent window

	deferClose ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NonModalPopupSelectorWindow::checkClientResult (IPopupSelectorClient::Result result)
{
	if(result != IPopupSelectorClient::kIgnore)
	{
		if(result != IPopupSelectorClient::kSwallow)
		{
			popupResult = result;
			closePopup ();
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NonModalPopupSelectorWindow::onMouseDown (const MouseEvent& event)
{
	SharedPtr<Object> holder (this);
	if(client && checkClientResult (client->onMouseDown (event, *this)))
		return true;

	bool result = SuperClass::onMouseDown (event);
	if(client && checkClientResult (client->onEventProcessed (event, *this, nullptr)))
		result = true;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NonModalPopupSelectorWindow::onMouseUp (const MouseEvent& event)
{
	SharedPtr<Object> holder (this);
	bool wantsMouseOutside = (owner->getBehavior () & IPopupSelector::kWantsMouseUpOutside) != 0;

	if(!wantsMouseOutside)
		if(PopupSelector::didMouseHandlerEscape (this, event))
			return SuperClass::onMouseUp (event);
	
	bool inside = isInsideClient (event.where) || wantsMouseOutside;
	bool result = false;

	if(inside && client && checkClientResult (client->onMouseUp (event, *this)))
		result = true;

	if(SuperClass::onMouseUp (event))
		result = true;

	if(result)
		return true;

	if(inside && client && checkClientResult (client->onEventProcessed (event, *this, nullptr)))
		result = true;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NonModalPopupSelectorWindow::onKeyDown (const KeyEvent& event)
{
	SharedPtr<Object> holder (this);
	if(client && checkClientResult (client->onKeyDown (event)))
		return true;

	bool result = SuperClass::onKeyDown (event);
	if(client && checkClientResult (client->onEventProcessed (event, *this, nullptr)))
		result = true;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NonModalPopupSelectorWindow::onKeyUp (const KeyEvent& event)
{
	SharedPtr<Object> holder (this);
	if(client && checkClientResult (client->onKeyUp (event)))
		return true;

	bool result = SuperClass::onKeyUp (event);
	if(client && checkClientResult (client->onEventProcessed (event, *this, nullptr)))
		result = true;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NonModalPopupSelectorWindow::onGestureProcessed (const GestureEvent& event, View* view)
{
	// check if client accepts
	if(client)
		checkClientResult (client->onEventProcessed (event, *this, view));
}

//************************************************************************************************
// PopupSelector
//************************************************************************************************

DEFINE_CLASS (PopupSelector, Object)
DEFINE_CLASS_UID (PopupSelector, 0xFCDB7599, 0x685E, 0x4E20, 0x9C, 0x7B, 0x4C, 0xC2, 0x1A, 0x2B, 0xDE, 0x00)
DEFINE_STRINGID_MEMBER_ (PopupSelector, kPopupClosed, "popupClosed")

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelector::PopupSelector ()
: theme (nullptr),
  visualStyle (nullptr),
  popupResult (IPopupSelectorClient::kIgnore),
  currentWindow (nullptr),
  monitor (-1),
  behavior (0),
  menuMode (false),
  nonModal (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelector::~PopupSelector ()
{
	if(theme)
		theme->release ();
	if(visualStyle)
		visualStyle->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupSelector::setTheme (Theme* _theme)
{
	take_shared (theme, _theme);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupSelector::setTheme (const Theme& theme)
{
	setTheme (const_cast<Theme*> (&theme));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupSelector::setTheme (ITheme* theme)
{
	setTheme (unknown_cast<Theme> (theme));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupSelector::setDecor (StringID decorName, IUnknown* decorController)
{
	setDecorName (decorName);
	setDecorController (decorController);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupSelector::setVisualStyle (IVisualStyle* vs)
{
	take_shared (visualStyle, unknown_cast<VisualStyle> (vs));
	setDecorNameFromStyle (visualStyle ? *visualStyle : VisualStyle::emptyStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupSelector::setBehavior (int32 _behavior)
{
	behavior = _behavior;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int32 PopupSelector::getBehavior () const
{
	return behavior;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Theme& PopupSelector::getTheme () const
{
	if(theme)
		return *theme;

	#if DEBUG
	Debugger::println ("Warning: No theme assigned to PopupSelector!");
	#endif
	return ThemeManager::instance ().getDefaultTheme ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle* PopupSelector::getVisualStyle () const
{
	return visualStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupSelector::setDecorNameFromStyle (const IVisualStyle& vs)
{
	MutableCString decorName = vs.getString ("decorform");
	
	if(vs.getMetric<bool> ("decorform", false))
		decorName = String () << vs.getName () << "Decor";
	
	if(decorName.isEmpty ())
	{ 
		if(IVisualStyle* defaultDecorStyle = ThemePainter::getStandardStyle (ThemePainter::kPopupMenuStyle))
			decorName = String () << defaultDecorStyle->getName () << "Decor";
	}
	
	setDecorName (decorName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPopupSelectorWindow* PopupSelector::getCurrentWindow () const
{
	return currentWindow;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPopupSelectorClient::Result PopupSelector::getPopupResult () const
{
	return popupResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupSelector::isOpen ()
{
	return currentWindow != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PopupSelector::close ()
{
	if(currentWindow)
		currentWindow->closePopup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupSelector::popup (IView* view, IPopupSelectorClient* client, const PopupSizeInfo& sizeInfo)
{
	Promise promise (doPopup (view, client, sizeInfo));
	return promise->getResult ().asBool ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API PopupSelector::popupAsync (IPopupSelectorClient* client, const PopupSizeInfo& sizeInfo)
{
	return doPopup (nullptr, client, sizeInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupSelector::popup (IPopupSelectorClient* client, const PopupSizeInfo& sizeInfo)
{
	Promise promise (popupAsync (client, sizeInfo));
	return promise->getResult ().asBool ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupSelector::popup (IParameter* parameter, const PopupSizeInfo& sizeInfo, StringID menuType)
{
	Promise promise (popupAsync (parameter, sizeInfo, menuType));
	return promise->getResult ().asBool ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API PopupSelector::popupAsync (IParameter* parameter, const PopupSizeInfo& sizeInfo, StringID _menuType)
{
	// 1.) Palette
	if(UnknownPtr<IPaletteProvider> (parameter).isValid ())
	{
		AutoPtr<PalettePopup> client = NEW PalettePopup (parameter);
		client->setVisualStyle (visualStyle);
		
		client->wantsMouseUpOutside (wantsMouseUpOutside ());

		if((behavior & IPopupSelector::kStayOpenOnClick) != 0)
			client->acceptOnDoubleClick (true);

		if((behavior & IPopupSelector::kAcceptsAfterSwipe) != 0)
			client->acceptAfterSwipe (true);

		return doPopup (nullptr, client, sizeInfo);
	}

	// 2.) Menu
	CString menuType = _menuType;
	if(UnknownPtr<IParameterMenuCustomize> customizer = parameter)
		menuType = customizer->getMenuType ();

	AutoPtr<Menu> menu;
	if(menuType == MenuPresentation::kNative)
		menu = PopupMenu::create ();
	else
		menu = NEW ExtendedMenu;

	// init scale factor before building menu for icons created on the fly
	float scaleFactor = 1.f;
	if(View* view = unknown_cast<View> (sizeInfo.parent))
		if(Window* window = view->getWindow ())
			scaleFactor = window->getContentScaleFactor ();
	menu->setScaleFactor (scaleFactor);
			
	AutoPtr<ParameterMenuBuilder> menuBuilder (NEW ParameterMenuBuilder (parameter));
	menuBuilder->buildMenu (menu);
	return popupAsync (menu, sizeInfo, menuType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupSelector::popupSlider (IParameter* parameter, const PopupSizeInfo& sizeInfo, tbool horizontal)
{
	StyleFlags sliderStyle (horizontal ? Styles::kHorizontal : Styles::kVertical);
	AutoPtr<PopupSlider> slider = NEW PopupSlider (parameter, sliderStyle);
	
	slider->setPopupFormName (getDecorName ());
	setDecorName (nullptr);	// custom popup sliders don't use a PopupDecorator.
	
	return popup (slider, sizeInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupSelector::popup (IMenu* menu, const PopupSizeInfo& sizeInfo, StringID menuType)
{
	return popup (unknown_cast<Menu> (menu), sizeInfo, menuType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

class PopupSelector::MenuFinalizer: public AsyncCompletionHandler
{
public:
	MenuFinalizer (PopupSelector* popupSelector, Menu* menu)
	: popupSelector (popupSelector), menu (menu)
	{}

	PROPERTY_SHARED_AUTO (IMenuControl, menuControl, MenuControl)

	void CCL_API onCompletion (IAsyncOperation& operation) override
	{
		if(menuControl)
		{
			if(!menuControl->getPopupClient ()->isIgnoringMouseClick ()) // already selected in this case
				if(MenuItem* item = menuControl->getResultItem ())
					item->select ();

			popupSelector->setMenuMode (false); 
		}
		menu->markForGC ();
	}

private:
	SharedPtr<PopupSelector> popupSelector;
	SharedPtr<Menu> menu;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

class PopupSelector::NativeMenuFinalizer: public AsyncCompletionHandler
{
public:
	NativeMenuFinalizer (PopupMenu* nativeMenu)
	: nativeMenu (nativeMenu)
	{}

	void CCL_API onCompletion (IAsyncOperation& operation) override
	{
		if(nativeMenu)
		{
			MenuItemID itemID = (MenuItemID)operation.getResult ().asInt ();
			MenuItem* item = itemID ? nativeMenu->findItem (itemID) : nullptr;
			if(item)
				item->select ();

			nativeMenu->markForGC ();
		}
	}

private:
	SharedPtr<PopupMenu> nativeMenu;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupSelector::popup (Menu* menu, const PopupSizeInfo& sizeInfo, StringID menuType)
{
	Promise promise (popupAsync (menu, sizeInfo, menuType));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API PopupSelector::popupAsync (IMenu* _menu, const PopupSizeInfo& sizeInfo, StringID menuType)
{
	Menu* menu = unknown_cast<Menu> (_menu);
	if(!menu || menu->isEmpty ())
		return AsyncOperation::createCompleted (true);

	menu->updateKeys ();
	menu->init ();

	// 1.) Native Presentation
	if(PopupMenu* nativeMenu = ccl_cast<PopupMenu> (menu))
	{
		ASSERT (menuType.isEmpty () || menuType == MenuPresentation::kNative)
		Promise promise (nativeMenu->popupAsync (sizeInfo.where, unknown_cast<View> (sizeInfo.parent)));
		AutoPtr<NativeMenuFinalizer> finalizer (NEW NativeMenuFinalizer (nativeMenu));
		return return_shared<IAsyncOperation> (promise.then (static_cast<IAsyncCompletionHandler*> (finalizer)));
	}

	AutoPtr<MenuFinalizer> menuFinalizer (NEW MenuFinalizer (this, menu));
	IView* popupView = nullptr;
	SharedPtr<PopupSelectorClient> popupClient;


	// 2.) Tree Presentation
	if(menuType == MenuPresentation::kTree)
	{
		bool selectCheckedItems = (behavior & IPopupSelector::kMenuSelectCheckedItem) != 0;
		AutoPtr<MenuPopupSelector> client = NEW MenuPopupSelector (menu, selectCheckedItems);
		client->setVisualStyle (visualStyle);
		client->wantsMouseUpOutside (wantsMouseUpOutside ());
		client->hideHScroll (hideHScroll ());
		popupClient = client;
	}
	else
	{
		// 3.) Extended Presentation
		ASSERT (menuType.isEmpty () || menuType == MenuPresentation::kExtended || menuType == MenuPresentation::kCompact || menuType == MenuPresentation::kSingleColumn)
		AutoPtr<IMenuControl> control;
		if(menuType == MenuPresentation::kCompact)
			control = NEW CompactMenuContainer (menu, visualStyle);
		else if(menuType == MenuPresentation::kSingleColumn)
			control = NEW CompactMenuContainer (menu, visualStyle, 1);
		else
			control = NEW MenuControl (menu, visualStyle);

		menuFinalizer->setMenuControl (control);
		menuMode = true;

		popupView = UnknownPtr<IView> (control).detach ();
		popupClient = control->getPopupClient ();
	}

	if((behavior & IPopupSelector::kStayOpenOnClick) != 0)
		popupClient->acceptOnDoubleClick (true);

	Promise promise (doPopup (popupView, popupClient, sizeInfo));
	return return_shared<IAsyncOperation> (promise.then (static_cast<IAsyncCompletionHandler*> (menuFinalizer)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API PopupSelector::popupAsync (IView* view, IPopupSelectorClient* client, const PopupSizeInfo& sizeInfo)
{
	return doPopup (view, client, sizeInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupSelector::adjustWindowSize (Rect& newSize)
{
	int monitor = this->monitor;
	if(monitor < 0)
		monitor = Desktop.findMonitor (newSize.getCenter (), true);

	Rect monitorSize;
	Desktop.getMonitorSize (monitorSize, monitor, true);
		
	// check if window crosses monitor edge
	Coord verticalOutside = ccl_max (0, newSize.bottom - monitorSize.bottom);
	ccl_upper_limit (verticalOutside, newSize.top - monitorSize.top); // don't move higher than top monitor edge
	Coord horizontalOutside = ccl_max (0, newSize.right - monitorSize.right);
	ccl_upper_limit (horizontalOutside, newSize.left - monitorSize.left); // don't move further than left monitor edge
		
	if(horizontalOutside > 0 || verticalOutside > 0)
	{
		// move window to show the hidden area
		newSize.offset (-horizontalOutside, -verticalOutside);

		// take the mouse along (not if window is hidden, e.g. a re-appearing context menu)
		if(currentWindow && unknown_cast<Window> (currentWindow)->isVisible ())
		{
			Point mousePos;
			GUI.getMousePosition (mousePos);
			mousePos.offset (-horizontalOutside, -verticalOutside);
			GUI.setMousePosition (mousePos);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PopupSelector::didMouseHandlerEscape (Window* window, const MouseEvent& event)
{
	if(MouseHandler* mouseHandler = window->getMouseHandler ())
		if(View* handlerView = mouseHandler->getView ())
		{
			Point p (event.where);
			if(window->isInsideClient (p)) // only if still in window
			{
				handlerView->windowToClient (p);
				return !handlerView->isInsideClient (p);
			}
		}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* PopupSelector::doPopup (IView* popupView, IPopupSelectorClient* client, const PopupSizeInfo& sizeInfo)
{
	View* parent = unknown_cast<View> (sizeInfo.parent);
	behavior |= client->getPopupBehavior ();

	ThemeSelector themeSelector (getTheme ());
	PopupArranger arranger (sizeInfo);

	if(!popupView)
	{
		// calculate available screen space
		SizeLimit limits;
		arranger.getLimits (limits);

		// let client create the view
		popupView = client->createPopupView (limits);
		if(!popupView)
			return nullptr;
	}

	View* view = unknown_cast<View> (popupView);
	if(!view || view->getSize ().isEmpty ())
	{
		popupView->release ();
		return nullptr;
	}

	if(!decorName.isEmpty ())
	{
		AutoPtr<PopupDecorator> decorator (NEW PopupDecorator (this, view));
		view = decorator->decorateView (getTheme ());
	}

	// update popup size infos with view limits
	PopupSizeInfo popupSizeInfo = sizeInfo;
	popupSizeInfo.sizeLimits.include (view->getSizeLimits ());

	monitor = arranger.getMonitor ();
	Point p (arranger.getWindowLocation (*view));

	Rect size = view->getSize ();
	size.moveTo (Point ());
	view->setSize (size);

	size.moveTo (p); // window rect

	GUI.getMousePosition (oldMousePos);

	auto prepareWindow = [&] (auto window)
	{
		window->setOwner (this);

		window->setVisualStyle (getVisualStyle ());
		window->addView (view);
		window->setSizeMode (View::kAttachAll|View::kFitSize);

		UnknownPtr<INativePopupSelectorWindow> nativePopupSelectorWindow (window->asUnknown ());
		if(nativePopupSelectorWindow.isValid ())
			nativePopupSelectorWindow->setSizeInfo (popupSizeInfo);
		
		currentWindow = window;
	};

	retain (); // stay alive while dialog is open, so that onPopupClosed can be called safely

	Window* parentWindow = arranger.getParentWindow ();

	constexpr int kCommonStyleMask = Styles::kTransparent|Styles::kTranslucent;
	StyleFlags windowStyle (view->getStyle ().common & kCommonStyleMask, Styles::kWindowBehaviorPopupSelector);
	
	// take certain window style flags from a content form's windowStyle
	constexpr int kCommonWindowStyleMask = kCommonStyleMask;
	constexpr int kCustomWindowStyleMask = Styles::kWindowAppearanceCustomFrame | Styles::kWindowBehaviorSheetStyle;
	if(auto form = ccl_cast<Form> (view))
	{
		windowStyle.common |= (form->getWindowStyle ().common & kCommonWindowStyleMask);
		windowStyle.custom |= (form->getWindowStyle ().custom & kCustomWindowStyleMask);
	}

	if(isNonModal ())
	{
		auto window = NEW NonModalPopupSelectorWindow (parentWindow, client, size, windowStyle);
		prepareWindow (window);

		window->show ();
		window->addToDesktop ();
		window->activate ();
		window->attached (window);

		window->addHandler (this);

		AsyncOperation* asyncOperation = window->getAsyncOperation ();
		asyncOperation->setState (AsyncOperation::kStarted);
		return return_shared  (asyncOperation);
	}
	else
	{
		auto dialog = NEW PopupSelectorWindow (parentWindow, client, size, windowStyle);
		dialog->setAnchorRect (sizeInfo.anchorRect);
		prepareWindow (dialog);

		dialog->setFirstFocusView (FocusNavigator::instance ().getFirst (view));

		AutoPtr<AsyncOperation> result = NEW AsyncOperation ();
		Promise (dialog->showDialog (parentWindow)).then ([this, result] (IAsyncOperation& operation)
		{
			onPopupClosed ();
			result->setResult (operation.getResult ());
			result->setState (operation.getState ());
		});

		return result.detach ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PopupSelector::onPopupClosed ()
{
	AutoPtr<Object> releaser (this); // release refCount from doPopup

	ASSERT (currentWindow)
	if(!currentWindow)
		return;

	IPopupSelectorClient* client = currentWindow->getClient ();
	ASSERT (client)
	if(client)
		client->onPopupClosed (popupResult = currentWindow->getPopupResult ());

	signal (Message (kPopupClosed));

	if(behavior & IPopupSelector::kRestoreMousePos)
		GUI.setMousePosition (oldMousePos);

	if(swallowDoubleClick)
	{
		swallowDoubleClick = false;

		if(menuMode == false) // don't swallow double click in menu mode
			GUI.tryDoubleClick ();
	}

	if(isNonModal ())
	{
		// finish AsyncOperation
		if(auto nonModalWindow = unknown_cast<NonModalPopupSelectorWindow> (currentWindow))
			if(AsyncOperation* asyncOperation = nonModalWindow->getAsyncOperation ())
				asyncOperation->setState (getPopupResult () == IPopupSelectorClient::kOkay ? AsyncOperation::kCompleted : AsyncOperation::kCanceled);

		currentWindow.detach ();
	}
	currentWindow = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PopupSelector::onWindowEvent (WindowEvent& windowEvent)
{
	ASSERT (isNonModal ())
	if(currentWindow && windowEvent.eventType == WindowEvent::kDestroy)
	{
		windowEvent.window.removeHandler (this);
		onPopupClosed ();
	}
	return true;
}
