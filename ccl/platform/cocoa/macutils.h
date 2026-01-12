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
// Filename    : ccl/platform/cocoa/macutils.h
// Description : Mac/iOS helper functions
//
//************************************************************************************************

#define DEBUG_RELEASE 0

#ifndef _ccl_macutils_h
#define _ccl_macutils_h

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/storage/iurl.h"

namespace CCL {

//************************************************************************************************
// CFObj
/** Core Foundation object wrapper */
//************************************************************************************************

template<class T>
struct CFObj
{
	T ref;

	CFObj (T ref = 0)
	: ref (ref)
	{}

	CFObj (const CFObj& obj)
	: ref (obj.ref)
	{ retain (); }

	~CFObj ()
	{ release (); }

	void retain ()  { if(ref) ::CFRetain ((CFTypeRef)ref); }
	
	void release () { 
        if(ref)
        {
            #if DEBUG_RELEASE
            ::CFStringRef desc = CFCopyDescription ((CFTypeRef)ref);
            String log;
            log.appendNativeString (desc);
            CCL::Debugger::println (log);
            #endif
            
            ::CFRelease ((CFTypeRef)ref);   
        }
    }
    
   	CFObj& assign (T _ref)
   	{
   		release ();
		ref = _ref;
		return *this;
	}
	
	CFObj& operator = (T _ref)	{ return assign (_ref); }
	operator T () const			{ return ref; }
};

//************************************************************************************************
// NSObj
//************************************************************************************************

template<class T>
struct NSObj
{
	T* ref;
	
	NSObj (T* ref = nullptr)
	: ref (ref)
	{}
	
	NSObj (const NSObj& obj)
	: ref (obj.ref)
	{ retain (); }
	
	~NSObj ()
	{ release (); }
	
	void retain ()  { if(ref) [ref retain]; }
	
	void release () { if(ref) [ref release]; }
	
	NSObj& assign (T* _ref)
   	{
   		release ();
		ref = _ref;
		return *this;
	}
	
	NSObj& operator = (T* _ref)	{ return assign (_ref); }
	operator T* () const		{ return ref; }
	
};

//************************************************************************************************
// MacUtils
//************************************************************************************************

namespace MacUtils 
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Path conversion
	//////////////////////////////////////////////////////////////////////////////////////////////

	bool urlFromCFURL (IUrl& url, CFURLRef cfUrlRef, int type = IUrl::kFile);
	bool urlToCFURL (const IUrl& url, CFURLRef& cfUrlRef, int type = IUrl::kFile);

	bool urlFromNSUrl (IUrl& url, const NSURL* nsUrl, int type = IUrl::kFile, bool storeBookmark = false);
	bool urlToNSUrl (const IUrl& url, NSURL*& nsUrl, int type = IUrl::kFile);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Bundle tools
	//////////////////////////////////////////////////////////////////////////////////////////////

	NSBundle* bundleFromId (NSString* bundleId);
	bool isBundle (NSURL* url);
}

} // namespace CCL

#endif // _ccl_macutils_h
