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
// Filename    : ccl/app/navigation/webnavigator.cpp
// Description : Web Navigator
//
//************************************************************************************************

#include "ccl/app/navigation/webnavigator.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/controlproperties.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum WebNavigatorTags
	{
		kLocation = 100,
		kTitle
	};
}

//************************************************************************************************
// WebNavigator
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebNavigator, NavigatorBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebNavigator::WebNavigator (StringRef name)
: NavigatorBase (name.isEmpty () ? CCLSTR ("WebNavigator") : name),
  webBrowser (nullptr)
{
	paramList.addString (CSTR ("location"), Tag::kLocation);
	paramList.addString (CSTR ("title"), Tag::kTitle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebNavigator::~WebNavigator ()
{
	// cleanup if view still exists (seems to happen in ccldemo only)
	signalSlots.unadviseAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WebNavigator::isAvailabe () const
{
	ASSERT (contentFrame)

	Variant available;
	return ViewBox (contentFrame).getAttribute (available, kWebBrowserViewIsAvailable) ? available.asBool () : true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebNavigator::navigate (UrlRef url)
{
	return webBrowser ? webBrowser->navigate (url) : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebNavigator::refresh ()
{
	return webBrowser ? webBrowser->refresh () : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebNavigator::goBack ()
{
	return webBrowser ? webBrowser->goBack () : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebNavigator::goForward ()
{
	return webBrowser ? webBrowser->goForward () : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebNavigator::canGoBack () const
{
	return webBrowser && webBrowser->canGoBack ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebNavigator::canGoForward () const
{
	return webBrowser && webBrowser->canGoForward ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebNavigator::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kLocation :
		{
			String urlString;
			param->toString (urlString);
			
			Url url (urlString);
			if(url.getProtocol ().isEmpty ())
				url.setProtocol (CCLSTR ("http"));

			navigate (url);
		}
		return true;
	}
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API WebNavigator::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "contentFrame")
	{
		contentFrame = ViewBox (ClassID::WebBrowserView, bounds);
		signalSlots.advise (UnknownPtr<ISubject> (contentFrame), kPropertyChanged, this, &WebNavigator::onWebViewChanged);
		// Note: advise to web view remains unbalanced, signal slot will detach itself on destroy and remain in orphaned state.
		return contentFrame;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebNavigator::onWebViewChanged (MessageRef msg)
{
	ASSERT (msg == kPropertyChanged)

	INavigator* newBrowser = nullptr;
	if(contentFrame)
	{
		Variant v;
		ViewBox (contentFrame).getAttribute (v, kWebBrowserViewNavigator);
		newBrowser = UnknownPtr<INavigator> (v.asUnknown ());
	}

	if(webBrowser != newBrowser)
	{
		if(webBrowser)
			signalSlots.unadvise (UnknownPtr<ISubject> (webBrowser));

		take_shared (webBrowser, newBrowser);

		if(webBrowser)
			signalSlots.advise (UnknownPtr<ISubject> (webBrowser), kChanged, this, &WebNavigator::onBrowserChanged);

		// initial state
		if(webBrowser && !homeUrl.isEmpty ())
			webBrowser->navigate (homeUrl);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebNavigator::onBrowserChanged (MessageRef msg)
{
	ASSERT (webBrowser)

	currentUrl = webBrowser->getCurrentUrl ();
	currentTitle = webBrowser->getCurrentTitle ();

	String urlString;
	if(!currentUrl.isEmpty ())
		urlString = UrlFullString (currentUrl, true);

	paramList.byTag (Tag::kLocation)->fromString (urlString);
	paramList.byTag (Tag::kTitle)->fromString (currentTitle);

	paramList.checkCommandStates ();
}
