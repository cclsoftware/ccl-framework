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
// Filename    : ccl/platform/cocoa/gui/legacywebbrowserview.mac.mm
// Description : Cocoa Web Browser View
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "legacywebbrowserview.mac.h"

#include "ccl/gui/system/webbrowserview.h"
#include "ccl/gui/windows/window.h"
#include "ccl/platform/cocoa/macutils.h"
#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#include "ccl/platform/cocoa/quartz/nshelper.h"
#include "ccl/base/message.h"

#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/base/ccldefpush.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

// WebView has been deprecated and replaced by WkWebView
// As of 2023, printing with WkWebView is broken (iOS) or not supported (macOS < v11)

#import <WebKit/WebKit.h>

using namespace CCL;

//************************************************************************************************
// NewWindowHandler
//************************************************************************************************

@interface CCL_ISOLATED (NewWindowHandler) : NSObject<WebUIDelegate, WebPolicyDelegate, WebResourceLoadDelegate>
{
	WebView* webView;
}
- (CCL_ISOLATED (NewWindowHandler)*)init;
- (WebView*)webView;
@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (NewWindowHandler)

- (CCL_ISOLATED (NewWindowHandler)*)init
{
	if(self = [super init])
	{
		webView = [[WebView alloc] init];
		
		[webView setUIDelegate:self];
		[webView setPolicyDelegate:self];  
		[webView setResourceLoadDelegate:self];
	}
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)dealloc
{
	[webView release];
	[super dealloc];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)webView:(WebView*)sender decidePolicyForNavigationAction:(NSDictionary*)actionInformation request:(NSURLRequest*)request frame:(WebFrame*)frame decisionListener:(id<WebPolicyDecisionListener>)listener
{
	[listener ignore];
	[[NSWorkspace sharedWorkspace] openURL:[actionInformation objectForKey:WebActionOriginalURLKey]];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (WebView*)webView
{
	return webView;
}

@end

//************************************************************************************************
// WebControlDelegate
//************************************************************************************************

@interface CCL_ISOLATED (WebControlDelegate) : NSObject
{
	LegacyWebKitControl* webControl;
}
- (void)setWebControl:(LegacyWebKitControl*) control;
- (void)webView:(WebView*)sender didReceiveTitle:(NSString*)title forFrame:(WebFrame*)frame;
- (void)webView:(WebView*)sender printFrameView:(WebFrameView*)frameView;

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

@implementation CCL_ISOLATED (WebControlDelegate)

- (void)setWebControl:(LegacyWebKitControl*) control
{
	webControl = control;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)webView:(WebView*)sender didReceiveTitle:(NSString*)nsTitle forFrame:(WebFrame*)frame
{
	if(webControl)
	{
		if(frame == webControl->getMainFrame ())
		{
			String title;
			title.appendNativeString (nsTitle);
			webControl->setCurrentTitle (title);
			NSURL* nsUrl = [[[frame dataSource] request] URL];
			Url url;
			MacUtils::urlFromNSUrl (url, nsUrl, IUrl::kFile);
			webControl->setCurrentURL (url);			
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)webView:(WebView*)sender printFrameView:(WebFrameView*)frameView
{
	if([frameView documentViewShouldHandlePrint])
		[frameView printDocumentView];
	else
	{
		NSPrintInfo* printInfo = [NSPrintInfo sharedPrintInfo];
		[printInfo setHorizontalPagination:NSFitPagination];
		[printInfo setVerticalPagination:NSAutoPagination];
		[printInfo setVerticallyCentered:NO];
		[printInfo setHorizontallyCentered:NO];

		NSPrintOperation* printOperation = [frameView printOperationWithPrintInfo:printInfo];
		[printOperation runOperation];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (WebView*)webView:(WebView*)sender createWebViewWithRequest:(NSURLRequest*)request
{
	if(webControl)
		return [webControl->getNewWindowHandler () webView];

	return nil;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)webView:(WebView*)webView decidePolicyForNavigationAction:(NSDictionary*)actionInformation request:(NSURLRequest*)request frame:(WebFrame*)frame decisionListener:(id<WebPolicyDecisionListener>)listener
{
	if(webControl)
	{
		NSString* scheme = [[request URL] scheme];
		// pass e.g. "mailto://" and "{appname}://" to the OS
		if([@[@"http", @"https", @"file", @"about"] indexOfObject:scheme] == NSNotFound)
		{
			[listener ignore];
			[[NSWorkspace sharedWorkspace] openURL:[actionInformation objectForKey:WebActionOriginalURLKey]];
			return;
		}
		
		if(webControl->getOptions ().isCustomStyle (Styles::kWebBrowserViewBehaviorRestrictToLocal))
		{
			Url url;
			if(CCL::MacUtils::urlFromNSUrl (url, [request URL]))
			{
				bool accepted = url.getProtocol ().isEmpty () || url.getProtocol ().compare (CCLSTR ("file"), false) == 0;
				if(accepted == false)
				{
					[listener ignore];
					System::GetSystemShell ().openUrl (url);
					return;
				}
			}
		}
	}
	[listener use];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)webView:(WebView*)webView decidePolicyForNewWindowAction:(NSDictionary*)actionInformation request:(NSURLRequest*)request newFrameName:(NSString*)frameName decisionListener:(id<WebPolicyDecisionListener>)listener
{
	[[NSWorkspace sharedWorkspace] openURL:[actionInformation objectForKey:WebActionOriginalURLKey]];
}
	
#if DEBUG_LOG

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)webView:(WebView *)sender didFailProvisionalLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
	NSLog (@"didFailProvisionalLoadWithError: %@", error);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)webView:(WebView *)sender didFailLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
	NSLog (@"didFailLoadWithError: %@", error);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)webView:(WebView *)sender didCancelClientRedirectForFrame:(WebFrame *)frame
{
	NSLog (@"didCancelClientRedirectForFrame: %@", frame);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)webView:(WebView *)sender willPerformClientRedirectToURL:(NSURL *)URL delay:(NSTimeInterval)seconds fireDate:(NSDate *)date forFrame:(WebFrame *)frame
{
	NSLog (@"willPerformClientRedirectToURL: %@", URL);
}

#endif

@end

//////////////////////////////////////////////////////////////////////////////////////////////////

LegacyWebKitControl::LegacyWebKitControl (WebBrowserView& owner)
: NativeWebControl (owner)
{
	NSRect frame;
	MacOS::toNSRect (frame, owner.getSize ());
	webView = [[WebView alloc] initWithFrame:frame];
	
	delegate = [[CCL_ISOLATED (WebControlDelegate) alloc] init];
	[delegate setWebControl:this];
	[webView setFrameLoadDelegate:delegate];
	[webView setUIDelegate:delegate];
	[webView setPolicyDelegate:delegate];
	
	newWindowHandler = [[CCL_ISOLATED (NewWindowHandler) alloc] init];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LegacyWebKitControl::~LegacyWebKitControl ()
{
	[delegate setWebControl:nullptr];
	[webView release];
	[delegate release];
	[newWindowHandler release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LegacyWebKitControl::attachView ()
{
	if(Window* window = owner.getWindow ())
	{
		NSRect frame;
		MacOS::toNSRect (frame, getSizeInWindow ());
		webView.frame = frame;	
		
		if(NSWindow* nsWindow = MacOS::toNSWindow (window))
		{
			[webView setHostWindow:nsWindow];
			if(NSView* parentView = [nsWindow contentView])
				[parentView addSubview:webView];
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LegacyWebKitControl::detachView ()
{
	[webView close];
	[webView setHostWindow:nil];
	[webView removeFromSuperview];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LegacyWebKitControl::updateSize ()
{
	NSRect frame;
	MacOS::toNSRect (frame, getSizeInWindow ());
	[webView setFrame:frame];
	// enforce auto-hiding scroll bars
	[[[[[webView mainFrame] frameView] documentView] enclosingScrollView] setScrollerStyle:NSScrollerStyleOverlay];

}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LegacyWebKitControl::setCurrentURL (UrlRef url)
{
	currentUrl = url;
	flagCanBack ([webView canGoBack]);
	flagCanForward ([webView canGoForward]);
	signal (Message (kChanged));	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LegacyWebKitControl::setCurrentTitle (StringRef title)
{
	currentTitle = title;
	signal (Message (kChanged));	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebFrame* LegacyWebKitControl::getMainFrame () const
{
	return [webView mainFrame];
}
		
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LegacyWebKitControl::navigate (UrlRef url)
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
		[[webView mainFrame] loadRequest:[NSURLRequest requestWithURL:nsUrl]];
		[[webView window] orderFront:nil];
		return kResultOk;
	}
	else
		return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LegacyWebKitControl::refresh ()
{
	[[webView mainFrame] reload];
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LegacyWebKitControl::goBack ()
{
	if([webView goBack])
		return kResultOk;
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
tresult CCL_API LegacyWebKitControl::goForward ()
{
	if([webView goForward])
		return kResultOk;
	return kResultFailed;
}

#pragma clang diagnostic push
