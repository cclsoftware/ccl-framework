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
// Filename    : ccl/platform/cocoa/gui/systemshell.cocoa.mm
// Description : Mac OS system shell
//
//************************************************************************************************

#include "ccl/gui/system/systemshell.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/gui/framework/iwindow.h"

#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

#include "ccl/platform/cocoa/macutils.h"

#include "ccl/public/base/ccldefpush.h"
#include <AuthenticationServices/AuthenticationServices.h>

#define HIDE_AUTOSTART_WINDOW 0

//************************************************************************************************
// AuthenticationContextProvider
//************************************************************************************************

@interface CCL_ISOLATED (AuthenticationContextProvider): NSObject<ASWebAuthenticationPresentationContextProviding>
{
	ASPresentationAnchor window;
}

- (id)init:(ASPresentationAnchor)window;
- (ASPresentationAnchor)presentationAnchorForWebAuthenticationSession:(ASWebAuthenticationSession*)session;

@end

namespace CCL {

//************************************************************************************************
// CocoaSystemShell
//************************************************************************************************

class CocoaSystemShell: public SystemShell
{
public:
	// SystemShell
	tresult openApplicationSettings () override;
	tresult openNativeUrl (UrlRef url, int flags) override;
	tresult showNativeFile (UrlRef url) override;
	tresult CCL_API setRunAtStartupEnabled (tbool state) override;
	tbool CCL_API isRunAtStartupEnabled () override;
	tbool CCL_API isRunAtStartupHidden (ArgsRef args) override;
	IAsyncOperation* CCL_API startBrowserAuthentication (UrlRef url, StringRef scheme, IWindow* window = nullptr) override;
};

//************************************************************************************************
// AuthenticationSessionOperation
//************************************************************************************************

class AuthenticationSessionOperation: public AsyncOperation
{
public:
	void setSession (ASWebAuthenticationSession* session);
	void setDelegate (NSObject* delegate);

	// AsyncOperation
	void setState (AsyncOperation::State state) override;

protected:
	NSObj<ASWebAuthenticationSession> session;
	NSObj<NSObject> delegate;
};


//************************************************************************************************
// RunAtStartupHelper
//************************************************************************************************

#ifndef CCL_PLATFORM_IOS
struct RunAtStartupHelper
{
	NSString* appPath;
	CFURLRef appUrl;
	
	RunAtStartupHelper ()
	{
		appPath = [[NSBundle mainBundle] bundlePath];
		appUrl = (CFURLRef)[NSURL fileURLWithPath:appPath]; 
	}
	
	// kLSSharedFileListSessionLoginItems etc. are deprecated, can maybe replace with SMAppService
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	LSSharedFileListItemRef copyOurItem () const
	{
		LSSharedFileListItemRef ourItem = NULL;
		LSSharedFileListRef loginItems = LSSharedFileListCreate (NULL, kLSSharedFileListSessionLoginItems, NULL);
		ASSERT (loginItems)
		if(loginItems) 
		{
			UInt32 seedValue;
			NSArray* loginItemsArray = (NSArray*)LSSharedFileListCopySnapshot (loginItems, &seedValue);
			for(int i = 0 ; i < [loginItemsArray count] ; i++)
			{
				LSSharedFileListItemRef currentItemRef = (LSSharedFileListItemRef)[loginItemsArray objectAtIndex:i];
				if(CFURLRef url = LSSharedFileListItemCopyResolvedURL (currentItemRef, 0, NULL))
				{
					NSString* urlPath = [(NSURL*)url path];
					if([urlPath compare:appPath] == NSOrderedSame)
					{
						ourItem = currentItemRef;
						CFRetain (ourItem);
						break;
					}
					CFRelease (url);
				}
			}
			[loginItemsArray release];
		}
		CFRelease(loginItems);
		return ourItem;
	}
	
	bool setEnabled (bool newState)
	{
		bool result = false;
		LSSharedFileListRef loginItems = LSSharedFileListCreate (NULL, kLSSharedFileListSessionLoginItems, NULL);
		if(!loginItems)
			return result;

		LSSharedFileListItemRef itemRef = copyOurItem ();
		if(newState && itemRef == 0)
		{
			CFMutableDictionaryRef propertiesToSet = CFDictionaryCreateMutable (NULL, 1, NULL, NULL);
			#if HIDE_AUTOSTART_WINDOW		
			CFDictionaryAddValue (propertiesToSet, kLSSharedFileListLoginItemHidden, kCFBooleanTrue);
			#else
			CFDictionaryAddValue (propertiesToSet, kLSSharedFileListLoginItemHidden, kCFBooleanFalse);
			#endif
			itemRef = LSSharedFileListInsertItemURL (loginItems, kLSSharedFileListItemLast, NULL, NULL, appUrl, propertiesToSet, NULL);
			result = itemRef != NULL;
			CFRelease (propertiesToSet);
		}
		else if(!newState && itemRef != 0)
		{
			result = LSSharedFileListItemRemove (loginItems,itemRef) == noErr;
		}
		
		if(itemRef)
			CFRelease (itemRef);

		return result;
	}
	#pragma clang diagnostic push

	bool isEnabled () const
	{
		LSSharedFileListItemRef itemRef = copyOurItem ();
		if(itemRef)
		{
			CFRelease (itemRef);
			return true;
		}
		else
			return false;
	}
	
	bool isHidden () const
	{
		bool result = false;
		LSSharedFileListItemRef itemRef = copyOurItem ();
		if(itemRef)
		{
			CFTypeRef property = LSSharedFileListItemCopyProperty (itemRef, kLSSharedFileListLoginItemHidden);
			if(property)
			{
				if(CFGetTypeID (property) == CFBooleanGetTypeID ())
					if(CFBooleanGetValue ((CFBooleanRef)property))
						result = true;
				CFRelease (property);				
			}
			CFRelease (itemRef);
		}
		return result;
	}
};
#endif // !CCL_PLATFORM_IOS

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// AuthenticationContextProvider
//************************************************************************************************

@implementation CCL_ISOLATED (AuthenticationContextProvider)

- (id)init:(ASPresentationAnchor)_window
{
	if(self = [super init])
		window = _window;

	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (ASPresentationAnchor)presentationAnchorForWebAuthenticationSession:(ASWebAuthenticationSession*)session
{
	return window;
}

@end

//************************************************************************************************
// CocoaSystemShell
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (SystemShell, CocoaSystemShell)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CocoaSystemShell::openApplicationSettings ()
{
	#ifndef CCL_PLATFORM_IOS
		return kResultNotImplemented;
	#else
		[[UIApplication sharedApplication] openURL:[NSURL URLWithString:UIApplicationOpenSettingsURLString] options:@{} completionHandler:nil];
		return kResultOk;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CocoaSystemShell::openNativeUrl (UrlRef url, int flags)
{
	NSURL* nsUrl = [NSURL alloc];
	
	if(MacUtils::urlToNSUrl (url, nsUrl))
	#ifndef CCL_PLATFORM_IOS
		[[NSWorkspace sharedWorkspace] openURL:nsUrl];
	#else
		[[UIApplication sharedApplication] openURL:nsUrl options:@{} completionHandler:nil];
	#endif
	else
	 	return kResultFailed;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CocoaSystemShell::showNativeFile (UrlRef url)
{
	#ifndef CCL_PLATFORM_IOS
	bool isBundle = false;
	NSURL* nsUrl = [NSURL alloc];
	if(MacUtils::urlToNSUrl (url, nsUrl))
		isBundle = MacUtils::isBundle (nsUrl);
	if(url.isFolder () && !isBundle)
		return openNativeUrl (url, 0);
	
	String path;
	url.toDisplayString (path);
	NSString* filePath = path.createNativeString<NSString*> ();

 	BOOL result = [[NSWorkspace sharedWorkspace] selectFile:filePath inFileViewerRootedAtPath:@""];

	[filePath release];
	return result ? kResultOk : kResultFailed;
	#else
	return openNativeUrl (url, 0);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaSystemShell::setRunAtStartupEnabled (tbool state)
{
	#ifndef CCL_PLATFORM_IOS
	return RunAtStartupHelper ().setEnabled (state != 0) ? kResultOk : kResultFailed;
	#else	 
	return kResultNotImplemented;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaSystemShell::isRunAtStartupEnabled ()
{
	#ifndef CCL_PLATFORM_IOS
	return RunAtStartupHelper ().isEnabled ();
	#else	 
	return false;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CocoaSystemShell::isRunAtStartupHidden (ArgsRef args)
{
	#ifndef CCL_PLATFORM_IOS
	return RunAtStartupHelper ().isHidden ();
	#else	 
	return false;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API CocoaSystemShell::startBrowserAuthentication (UrlRef url, StringRef scheme, IWindow* window)
{
	bool success = false;
	auto* operation = NEW AuthenticationSessionOperation ();
	addOperation (return_shared (operation)); // keep an internal ref count

	NSURL* authUrl = [NSURL alloc];
	MacUtils::urlToNSUrl (url, authUrl);
	NSString* authScheme = [scheme.createNativeString<NSString*> () autorelease];

	ASPresentationAnchor authAnchor = nil;
	#if CCL_PLATFORM_MAC
	if(window)
	{
		if(NSView* view = static_cast<NSView*>(window->getSystemWindow ()))
			authAnchor = [view window];
	}
	else
		authAnchor = [NSApp mainWindow];
	#elif CCL_PLATFORM_IOS
	if(window)
	{
		if(UIViewController* viewController = static_cast<UIViewController*>(window->getSystemWindow ()))
			authAnchor = [[viewController view] window];
	}
	else
		authAnchor = [[[UIApplication sharedApplication] windows] firstObject];
	#endif

	if(authUrl && authScheme && authAnchor)
	{
		ASWebAuthenticationSession* session = [[ASWebAuthenticationSession alloc] initWithURL:authUrl callbackURLScheme:authScheme completionHandler:^(NSURL* url, NSError* error)
		{
			if(error)
				operation->setState (IAsyncInfo::kFailed);
			else
			{
				void(^work) (void) = ^
				{
					AutoPtr<Url> result = NEW Url ();
					MacUtils::urlFromNSUrl (*result, url);
					operation->setResult (Variant ().takeShared (result->asUnknown ()));
					operation->setState (IAsyncInfo::kCompleted);
				};
				// call on main thread for signal handler
				if([NSThread isMainThread] == YES)
					work ();
				else
					dispatch_sync (dispatch_get_main_queue (), work);
			}
	    }];
		if(session)
		{
			NSObject<ASWebAuthenticationPresentationContextProviding>* delegate = [[CCL_ISOLATED (AuthenticationContextProvider) alloc] init:authAnchor];
			[session setPresentationContextProvider:delegate];
			operation->setSession (session);
			operation->setDelegate (delegate);
			success = [session start];
		}
	}
	operation->setState (success ? IAsyncInfo::kStarted : IAsyncInfo::kFailed);
	return operation;
}

//************************************************************************************************
// AuthenticationSessionOperation
//************************************************************************************************

void AuthenticationSessionOperation::setSession (ASWebAuthenticationSession* _session)
{
	session = _session;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AuthenticationSessionOperation::setDelegate (NSObject* _delegate)
{
	delegate = _delegate;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AuthenticationSessionOperation::setState (State newState)
{
	if(newState != getState ())
	{
		if(newState == kCanceled)
			[session cancel];
		if(newState >= kCompleted)
		{
			SystemShell::instance ().removeOperation (this);
			deferDestruction (this); // this will release the internal ref count
		}

		AsyncOperation::setState (newState);
	}
}
