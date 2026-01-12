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
// Filename    : ccl/app/navigation/webnavigator.h
// Description : Web Navigator
//
//************************************************************************************************

#ifndef _ccl_webnavigator_h
#define _ccl_webnavigator_h

#include "ccl/app/navigation/navigatorbase.h"

namespace CCL {

//************************************************************************************************
// WebNavigator
//************************************************************************************************

class WebNavigator: public NavigatorBase
{
public:
	DECLARE_CLASS (WebNavigator, NavigatorBase)

	WebNavigator (StringRef name = nullptr);
	~WebNavigator ();

	bool isAvailabe () const; // must be called after WebBrowserView has been created

	// INavigator
	tresult CCL_API navigate (UrlRef url) override;
	tresult CCL_API refresh () override;
	tresult CCL_API goBack () override;
	tresult CCL_API goForward () override;
	tbool CCL_API canGoBack () const override;
	tbool CCL_API canGoForward () const override;

	// Component
	tbool CCL_API paramChanged (IParameter* param) override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

protected:
	INavigator* webBrowser;

	void onWebViewChanged (MessageRef msg);
	void onBrowserChanged (MessageRef msg);
};

} // namespace CCL

#endif // _ccl_webnavigator_h
