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
// Filename    : ccl/gui/system/webbrowserview.cpp
// Description : Web Browser View
//
//************************************************************************************************

#include "ccl/gui/system/webbrowserview.h"

#include "ccl/base/message.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/framework/controlproperties.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("WebView")
	XSTRING (GoBack, "Go Back")
	XSTRING (GoForward, "Go Forward")
	XSTRING (ReloadPage, "Reload Page")
	XSTRING (CopyText, "Copy Text to Clipboard")
END_XSTRINGS

//************************************************************************************************
// WebBrowserView
//************************************************************************************************

BEGIN_STYLEDEF (WebBrowserView::customStyles)
	{"localonly", Styles::kWebBrowserViewBehaviorRestrictToLocal},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (WebBrowserView, View)
DEFINE_CLASS_UID (WebBrowserView, 0xacdebcd3, 0xaba3, 0x4baf, 0x9f, 0x9d, 0xed, 0xbe, 0xdb, 0xf8, 0xed, 0x65)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebBrowserView::WebBrowserView (IUnknown* controller, const Rect& size, StyleRef style, StringRef title)
: View (size, style, title),
  controller (controller),
  nativeControl (nullptr)
{
	wantsFocus (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebBrowserView::~WebBrowserView ()
{
	ASSERT (nativeControl == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API WebBrowserView::getController () const
{
	return controller;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebBrowserView::setController (IUnknown* _controller)
{
	this->controller = _controller;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INavigator* WebBrowserView::getNavigator () const
{
	return nativeControl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebBrowserView::attached (View* parent)
{
	SuperClass::attached (parent);

	ASSERT (nativeControl == nullptr)
	nativeControl = NativeWebControl::createInstance (*this);
	if(nativeControl)
		nativeControl->attachView ();

	signal (Message (kPropertyChanged));

	// notify controller explicitly
	if(UnknownPtr<IObserver> observer = static_cast<IUnknown*> (controller))
		observer->notify (this, Message (kPropertyChanged));

	// EXPERIMENTAL:
	if(nativeControl)
		(NEW Message ("takeFocus"))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebBrowserView::removed (View* parent)
{
	if(nativeControl)
	{
		nativeControl->detachView ();
		safe_release (nativeControl);
		signal (Message (kPropertyChanged));

		// notify controller explicitly
		if(UnknownPtr<IObserver> observer = static_cast<IUnknown*> (controller))
			observer->notify (this, Message (kPropertyChanged));
	}

	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebBrowserView::onFocus (const FocusEvent& event)
{
	if(event.eventType == FocusEvent::kSetFocus)
		if(nativeControl)
			nativeControl->takeFocus ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebBrowserView::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "takeFocus")
		takeFocus ();
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebBrowserView::onSize (const Point& delta)
{
	SuperClass::onSize (delta);

	if(nativeControl)
		nativeControl->updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebBrowserView::onMove (const Point& delta)
{
	SuperClass::onMove (delta);

	if(nativeControl)
		nativeControl->updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebBrowserView::onDisplayPropertiesChanged (const DisplayChangedEvent& event)
{
	SuperClass::onDisplayPropertiesChanged (event);

	if(nativeControl)
		nativeControl->updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebBrowserView::onContextMenu (const ContextMenuEvent& event)
{
	if(nativeControl)
	{
		if(nativeControl->isTextSelected ())
			event.contextMenu.addCommandItem (XSTR (CopyText), "Navigation", "Copy", nativeControl); // don't use Edit/Copy because shortcut key doesn't work!
		else
		{
			event.contextMenu.addCommandItem (XSTR (GoBack), "Navigation", "Back", nativeControl);
			event.contextMenu.addCommandItem (XSTR (GoForward), "Navigation", "Forward", nativeControl);
			event.contextMenu.addSeparatorItem ();
			event.contextMenu.addCommandItem (XSTR (ReloadPage), "Navigation", "Refresh", nativeControl);
		}
		return true;
	}
	else
		return SuperClass::onContextMenu (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebBrowserView::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == kWebBrowserViewNavigator)
	{
		var.takeShared (getNavigator ());
		return nativeControl != nullptr;
	}
	else if(propertyId == kWebBrowserViewIsAvailable)
	{
		var = NativeWebControl::isAvailable ();
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// NativeWebControl
//************************************************************************************************

NativeWebControl::NativeWebControl (WebBrowserView& owner)
: owner (owner),
  textSelected (false),
  commandState (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeWebControl::~NativeWebControl ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StyleRef NativeWebControl::getOptions () const
{
	return owner.getStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect NativeWebControl::getSizeInWindow () const
{
	Rect size (owner.getSize ());
	Point offset;
	owner.clientToWindow (offset);
	size.moveTo (offset);
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeWebControl::navigate (UrlRef url)
{
	CCL_NOT_IMPL ("Can't navigate!\n")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeWebControl::navigateDeferred (UrlRef url)
{
	return navigate (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeWebControl::refresh ()
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API NativeWebControl::getCurrentUrl () const
{
	return currentUrl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API NativeWebControl::getCurrentTitle () const
{
	return currentTitle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeWebControl::goBack ()
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeWebControl::goForward ()
{
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeWebControl::canGoBack () const
{
	return flagCanBack ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeWebControl::canGoForward () const
{
	return flagCanForward ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NativeWebControl::goHome ()
{
	CCL_NOT_IMPL ("Should not be called!\n")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API NativeWebControl::getHomeUrl () const
{
	return Url::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeWebControl::checkCommandCategory (CStringRef category) const
{
	return category == "Navigation";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NativeWebControl::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == "Navigation")
	{
		if(msg.name == "Copy")
		{
			if(msg.checkOnly ())
				return isTextSelected ();
			else
			{
				copyText ();
				return true;
			}
		}
		else if(msg.name == "Back")
		{
			if(msg.checkOnly ())
				return canGoBack ();
			else
			{
				goBack ();
				return true;
			}
		}
		else if(msg.name == "Forward")
		{
			if(msg.checkOnly ())
				return canGoForward ();
			else
			{
				goForward ();
				return true;
			}
		}
		else if(msg.name == "Refresh")
		{
			if(!msg.checkOnly ())
				refresh ();
			return true;
		}
	}
	return false;
}
