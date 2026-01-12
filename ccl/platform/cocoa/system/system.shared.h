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
// Filename    : ccl/platform/cocoa/system/system.shared.h
// Description : OSX/iOS Shared System Information
//
//************************************************************************************************

#ifndef _ccl_cocoa_system_shared_h
#define _ccl_cocoa_system_shared_h

#include "ccl/system/system.h"

#include "ccl/base/storage/url.h"

@class NSCalendar;

namespace CCL {

//************************************************************************************************
// BundleImage
//************************************************************************************************

class BundleImage: public ExecutableImage
{
public:
	BundleImage (ModuleRef nativeRef, bool isLoaded);
	~BundleImage ();
	
	// ExecutableImage
	tbool CCL_API getPath (IUrl& path) const override;
	void* CCL_API getFunctionPointer (CStringPtr name) const override;
	const IAttributeList* CCL_API getMetaInfo () const override;
	tbool CCL_API getBinaryPath (IUrl& path) const override;
	
private:
	void unload ();
};

//************************************************************************************************
// DylibImage
//************************************************************************************************

class DylibImage: public ExecutableImage
{
public:
	DylibImage (ModuleRef nativeRef, bool isLoaded);
	~DylibImage ();
	
	void setPath (UrlRef newPath) { path = newPath; }
	
	// ExecutableImage
	ModuleRef CCL_API getNativeReference () const override;
	void* CCL_API getFunctionPointer (CStringPtr name) const override;
	tbool CCL_API getPath (IUrl& url) const override;
	
private:
	Url path;

	void unload ();
};
	
//************************************************************************************************
// CocoaSystemInformation
//************************************************************************************************

class CocoaSystemInformation: public SystemInformation
{
public:
	CocoaSystemInformation ();
	~CocoaSystemInformation ();
	
	// SystemInformation
	void CCL_API getLocalTime (DateTime& dateTime) const override;
	void CCL_API convertLocalTimeToUTC (DateTime& utc, const DateTime& localTime) const override;
	void CCL_API convertUTCToLocalTime (DateTime& localTime, const DateTime& utc) const override;
	void CCL_API convertUnixTimeToUTC (DateTime& utc, int64 unixTime) const override;
	int64 CCL_API convertUTCToUnixTime (const DateTime& utc) const override;	
	int CCL_API getNumberOfCPUs () const override;
	int CCL_API getNumberOfCores () const override;

protected:
	NSCalendar* gregorianLocal;
	NSCalendar* gregorianUTC;
};
	
} // namespace CCL

#endif // _ccl_cocoa_system_shared_h
