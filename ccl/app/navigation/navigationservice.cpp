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
// Filename    : ccl/app/navigation/navigationservice.cpp
// Description : Navigation Service
//
//************************************************************************************************

#include "ccl/app/navigation/navigationservice.h"
#include "ccl/public/app/inavigationserver.h"

#include "ccl/base/storage/attributes.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/inavigator.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/guiservices.h"

namespace CCL {

//************************************************************************************************
// ThemeNavigationServer
//************************************************************************************************

class ThemeNavigationServer: public Object,
							 public INavigationServer
{
public:
	// INavigationServer
	tresult CCL_API navigateTo (NavigateArgs& args) override;

	CLASS_INTERFACE (INavigationServer, Object)
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// NavigationService
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (NavigationService, Object)
DEFINE_SINGLETON (NavigationService)
const String NavigationService::kObjectProtocol = CCLSTR ("object");

//////////////////////////////////////////////////////////////////////////////////////////////////

NavigationService::NavigationService ()
: themeServer (NEW ThemeNavigationServer)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

NavigationService::~NavigationService ()
{
	if(themeServer)
		themeServer->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NavigationService::isValidProtocol (StringRef protocol) const
{
	return protocol == kObjectProtocol || protocol == IThemeManager::kThemeProtocol;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INavigationServer* NavigationService::lookupServer (UrlRef url) const
{
	if(url.getProtocol () == kObjectProtocol)
	{
		UnknownPtr<INavigationServer> server = System::GetObjectTable ().getObjectByUrl (url);
		return server;
	}
	else if(url.getProtocol () == IThemeManager::kThemeProtocol)
		return themeServer;

	ASSERT (0)
	return nullptr;
}

//************************************************************************************************
// ThemeNavigationServer
//************************************************************************************************

tresult CCL_API ThemeNavigationServer::navigateTo (NavigateArgs& args)
{
	IView* pageView = nullptr;
	ITheme* theme = System::GetThemeManager ().getTheme (MutableCString (args.url.getHostName ()));
	ASSERT (theme != nullptr)
	if(theme)
	{
		MutableCString formName ("/");
		formName.append (args.url.getPath ());
		Attributes attributes;
		attributes.set ("navigator", static_cast<IUnknown*> (&args.navigator));
		pageView = theme->createView (formName, args.contentComponent, &attributes);
	}

	if(pageView)
	{
		// remove old content
		args.contentFrame.getChildren ().removeAll (); 

		// move to origin
		ViewBox v (pageView);
		Rect size = args.contentFrame.getSize ();
		size.moveTo (Point ());
		v.setSize (size);

		// add view
		args.contentFrame.getChildren ().add (pageView);
		
		// set title
		ViewBox (&args.contentFrame).setTitle (ViewBox (pageView).getTitle ());
	}

	return pageView ? kResultOk : kResultFalse;
}
