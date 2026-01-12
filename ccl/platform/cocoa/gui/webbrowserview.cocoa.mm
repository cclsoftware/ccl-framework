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
// Filename    : ccl/platform/cocoa/gui/webbrowserview.cocoa.mm
// Description : WebKit Browser View
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/system/webbrowserview.h"
#include "ccl/gui/windows/nativewindow.h"
#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"
#if CCL_PLATFORM_MAC
#include "legacywebbrowserview.mac.h"
#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#else
#include "ccl/platform/cocoa/gui/nativeview.ios.h"
#endif
#include "ccl/base/message.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/base/ccldefpush.h"

#include <WebKit/WebKit.h>

using namespace CCL;

class LegacyWebKitControl;

@class CCL_ISOLATED (WebKitDelegate);

//************************************************************************************************
// WebKitControl
//************************************************************************************************

class WebKitControl : public NativeWebControl
{
public:
	WebKitControl (WebBrowserView& owner);
	~WebKitControl ();
	
	void onFinishNavigation ();
	void onPrint ();
	
	// NativeWebControl
	tresult CCL_API navigate (UrlRef url);
	tresult CCL_API refresh ();
	tresult CCL_API goBack ();
	tresult CCL_API goForward ();
	
	void attachView ();
	void detachView ();
	void updateSize ();
	
protected:
	NSObj<WKWebView> webView;
	NSObj<CCL_ISOLATED (WebKitDelegate)> delegate;
	
	void updateCurrentURL ();
	void updateCurrentTitle ();
};

static Configuration::BoolValue wkWebViewEnabled ("CCL.OSX.WKWebView", "Enabled", true);

//************************************************************************************************
// WebKitDelegate
//************************************************************************************************

@interface CCL_ISOLATED (WebKitDelegate) : NSObject<WKNavigationDelegate, WKScriptMessageHandler>
{
	WebKitControl* webControl;
}
- (instancetype)initWithWebControl:(WebKitControl*)control;
- (void)setWebControl:(WebKitControl*)control;

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (WebKitDelegate)

- (instancetype)initWithWebControl:(WebKitControl*)control
{
	ASSERT(control)
	if(self = [super init])
		webControl = control;
	
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setWebControl:(WebKitControl*)control
{
	webControl = control;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)webView:(WKWebView*)webView decidePolicyForNavigationAction:(WKNavigationAction*)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
{
	if(webControl)
	{
		bool openInNewWindow = ([navigationAction targetFrame] == nil);
		NSString* scheme = [[[navigationAction request] URL] scheme];		
		if(openInNewWindow || [@[@"http", @"https", @"file", @"about"] indexOfObject:scheme] == NSNotFound) // pass e.g. "mailto://" and "{appname}://" to the OS
		{
			decisionHandler (WKNavigationActionPolicyCancel);
			#if CCL_PLATFORM_MAC
			[[NSWorkspace sharedWorkspace] openURL:[[navigationAction request] URL]];
			#else
			[[UIApplication sharedApplication] openURL:[[navigationAction request] URL] options:@{} completionHandler:nil];
			#endif
			return;
		}
		
		if(webControl->getOptions ().isCustomStyle (Styles::kWebBrowserViewBehaviorRestrictToLocal))
		{
			Url url;
			if(CCL::MacUtils::urlFromNSUrl (url, [[navigationAction request] URL]))
			{
				bool accepted = url.getProtocol ().isEmpty () || url.getProtocol ().compare (CCLSTR ("file"), false) == 0;
				if(accepted == false)
				{
					decisionHandler (WKNavigationActionPolicyCancel);
					System::GetSystemShell ().openUrl (url);
					return;
				}
			}
		}
	}
	decisionHandler (WKNavigationActionPolicyAllow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)webView:(WKWebView*)webView didFinishNavigation:(WKNavigation*)navigation;
{
	if(webControl)
		webControl->onFinishNavigation ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)userContentController:(WKUserContentController*)userContentController didReceiveScriptMessage:(WKScriptMessage*)message
{
	if(webControl)
		if([message.name isEqualToString:@"print"])
		{
			webControl->onPrint ();
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG_LOG

- (void)webView:(WKWebView*)webView didFailProvisionalNavigation:(WKNavigation*)navigation withError:(NSError*)error
{
	NSLog (@"didFailProvisionalNavigation: %@", error);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)webView:(WKWebView*)webView didFailNavigation:(WKNavigation*)navigation withError:(NSError*)error
{
	NSLog (@"didFailNavigation: %@", error);
}

#endif

@end

//************************************************************************************************
// NativeWebControl
//************************************************************************************************

bool NativeWebControl::isAvailable ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NativeWebControl* NativeWebControl::createInstance (WebBrowserView& owner)
{
	#if CCL_PLATFORM_IOS
	return NEW WebKitControl (owner);
	#else
	if(wkWebViewEnabled)
		return NEW WebKitControl (owner);
	else
		return NEW LegacyWebKitControl (owner);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebKitControl::WebKitControl (WebBrowserView& owner)
: NativeWebControl (owner)
{
	CGRect frame = {0};
	frame.size.width = owner.getWidth ();
	frame.size.height = owner.getHeight ();
	
	delegate = [[CCL_ISOLATED (WebKitDelegate) alloc] initWithWebControl:this];
	
	WKWebViewConfiguration* config = [[[WKWebViewConfiguration alloc] init] autorelease];
	WKUserScript* script = [[[WKUserScript alloc] initWithSource:@"window.print = function() { window.webkit.messageHandlers.print.postMessage('print') }" injectionTime:WKUserScriptInjectionTimeAtDocumentEnd forMainFrameOnly:NO] autorelease];
	[config.userContentController addUserScript:script];
	[config.userContentController addScriptMessageHandler:delegate name:@"print"];
	webView = [[WKWebView alloc] initWithFrame:frame configuration:config];
	[webView setNavigationDelegate:delegate];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebKitControl::~WebKitControl ()
{
	[delegate setWebControl:nil];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebKitControl::attachView ()
{
	CGRect frame = {0};
	MacOS::toCGRect (frame, getSizeInWindow ());
	[webView setFrame:frame];
	#if CCL_PLATFORM_MAC
	if(OSXWindow* window = OSXWindow::cast (owner.getWindow ()))
		if(NSView* parentView = [[window->getNativeView ()->getView () window] contentView])
			[parentView addSubview:webView];
	#else
	if(IOSWindow* window = IOSWindow::cast (owner.getWindow ()))
		if(UIView* parentView = window->getNativeView ()->getView ())
			[parentView addSubview:webView];
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebKitControl::detachView ()
{
	[webView removeFromSuperview];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebKitControl::updateSize ()
{
	CGRect frame = {0};
	frame.size.width = owner.getWidth ();
	frame.size.height = owner.getHeight ();
	[webView setFrame:frame];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebKitControl::onFinishNavigation ()
{
	updateCurrentURL ();
	updateCurrentTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebKitControl::onPrint ()
{
	// printing is broken with WKWebView, see e.g. https://developer.apple.com/forums/thread/78354
	ASSERT (0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebKitControl::updateCurrentURL ()
{
	MacUtils::urlFromNSUrl (currentUrl, [webView URL], IUrl::kFile);
	flagCanBack ([webView canGoBack]);
	flagCanForward ([webView canGoForward]);
	signal (Message (kChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebKitControl::updateCurrentTitle ()
{
	String title;
	title.appendNativeString ([webView title]);
	currentTitle = title;
	signal (Message (kChanged));
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebKitControl::navigate (UrlRef url)
{
	StringRef path = url.getPath ();
	#if DEBUG_LOG
	MutableCString cString (path);
	#endif
	CCL_PRINTF("Request URL: %s\n", cString.str ())
	NSURL* nsUrl = [NSURL alloc];
	
	const String separator = CCLSTR ("#");
	if(path.contains (separator))
	{
		AutoPtr<IStringTokenizer> t = String (path).tokenize (separator);
		if(t)
		{
			uchar unused = 0;
			String prefix = t->nextToken (unused);
			String postfix = separator;
			postfix.append (t->nextToken (unused));
			Url baseUrl (url);
			baseUrl.setPath (prefix);
			MacUtils::urlToNSUrl (baseUrl, nsUrl);
			NSString* postfixNSString = (NSString*) postfix.createNativeString<CFStringRef> ();
			nsUrl = [NSURL URLWithString:postfixNSString relativeToURL:nsUrl];
			[postfixNSString release];
		}
	}
	else
		MacUtils::urlToNSUrl (url, nsUrl);
	
	if([[nsUrl path] length] > 0)
	{
		[webView loadRequest:[NSURLRequest requestWithURL:nsUrl]];
		#if CCL_PLATFORM_MAC
		[[webView window] orderFront:nil];
		#endif
		return kResultOk;
	}
	else
		return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebKitControl::refresh ()
{
	[webView reload];
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebKitControl::goBack ()
{
	if([webView goBack])
		return kResultOk;
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
tresult CCL_API WebKitControl::goForward ()
{
	if([webView goForward])
		return kResultOk;
	return kResultFailed;
}
