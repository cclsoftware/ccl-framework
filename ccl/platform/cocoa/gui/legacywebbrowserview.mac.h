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
// Filename    : ccl/platform/cocoa/gui/legacywebbrowserview.mac.h
// Description : Cocoa Web Browser View
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/system/webbrowserview.h"
#include "ccl/gui/windows/window.h"
#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/quartz/nshelper.h"
#include "ccl/base/message.h"

#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/base/ccldefpush.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

// WebView has been deprecated and replaced by WkWebView
// As of 2023, printing with WkWebView is broken (iOS) or not supported (macOS < v11)

using namespace CCL;

//************************************************************************************************
// LegacyWebKitControl
//************************************************************************************************

@class CCL_ISOLATED (NewWindowHandler);
@class WebFrame;
@class WebView;

class LegacyWebKitControl : public NativeWebControl
{
public:
	LegacyWebKitControl (WebBrowserView& owner);
	~LegacyWebKitControl ();
	
	void attachView ();		///< attach to owner view
	void detachView ();		///< detach from owner view
	void updateSize ();		///< owner has been resized/moved

	void setCurrentURL (UrlRef url);
	void setCurrentTitle (StringRef title);
	WebFrame* getMainFrame () const;
	
	// INavigator
	tresult CCL_API navigate (UrlRef url);
	tresult CCL_API refresh ();
	tresult CCL_API goBack ();
	tresult CCL_API goForward ();	
	CCL_ISOLATED (NewWindowHandler)* getNewWindowHandler () const {return newWindowHandler;}
	
protected:
	WebView* webView;
	id delegate;
	id newWindowHandler;
};

#pragma clang diagnostic push

#include "ccl/public/base/ccldefpop.h"
