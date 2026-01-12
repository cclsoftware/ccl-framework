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
// Filename    : ccl/platform/cocoa/system/system.shared.mm
// Description : OSX/iOS Shared System Information
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/system/system.shared.h"

#include "ccl/public/systemservices.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/platform/cocoa/cclcocoa.h"
#include "ccl/platform/cocoa/macutils.h"

#include "ccl/platform/shared/posix/system/system.posix.h"

#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/base/ccldefpush.h"

#include <AudioToolbox/AudioToolbox.h>
#include <sys/sysctl.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <mach/task.h>

namespace CCL {
namespace MacUtils {

void NSDateComponentsToDateTime (NSDateComponents* components, DateTime& dateTime);
void DateTimeToNSDateComponents (const DateTime& dateTime, NSDateComponents* components);

} // namespace MacUtils	
} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

void MacUtils::NSDateComponentsToDateTime (NSDateComponents* components, DateTime& dateTime)
{
	dateTime.setTime (Time ((int)components.hour, (int)components.minute, (int)components.second, (int)(components.nanosecond / 1000 / 1000)));
	dateTime.setDate (Date ((int)components.year, (int)components.month, (int)components.day));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MacUtils::DateTimeToNSDateComponents (const DateTime& dateTime, NSDateComponents* components)
{
	components.year = dateTime.getDate ().getYear ();
	components.month = dateTime.getDate ().getMonth ();
	components.day = dateTime.getDate ().getDay ();
	components.hour = dateTime.getTime ().getHour ();
	components.minute = dateTime.getTime ().getMinute ();
	components.second = dateTime.getTime ().getSecond ();
	components.nanosecond = dateTime.getTime ().getMilliseconds () * 1000 * 1000;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT ModuleRef CCL_API System::CCL_ISOLATED (GetMainModuleRef) ()
{
	return CFBundleGetMainBundle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT tresult CCL_API System::CCL_ISOLATED (CreateUID) (UIDBytes& uid)
{
	CFObj<CFUUIDRef> uuid = CFUUIDCreate (0);
	
	CFUUIDBytes& bytes = (CFUUIDBytes&)uid;
	bytes = CFUUIDGetUUIDBytes (uuid);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (LockMemory) (tbool state, void* address, int size)
{
	if(state)
		mlock (address, size);
	else
		munlock (address, size);
}

//************************************************************************************************
// BundleImage
//************************************************************************************************

BundleImage::BundleImage (ModuleRef nativeRef, bool isLoaded)
: ExecutableImage (nativeRef, isLoaded)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

BundleImage::~BundleImage ()
{
	if(isLoaded && nativeRef)
		unload ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BundleImage::unload ()
{
	if(nativeRef)
	{
		CFBundleRef bundle = (CFBundleRef)nativeRef;
		if(CFBundleIsExecutableLoaded (bundle))
			CFBundleUnloadExecutable (bundle);
		CFRelease (bundle);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BundleImage::getPath (IUrl& path) const
{
	ASSERT (nativeRef != NULL)
	if(nativeRef == NULL)
		return false;
	
	CFObj<CFURLRef> cfUrl = CFBundleCopyBundleURL ((CFBundleRef)nativeRef);
	return cfUrl ? MacUtils::urlFromCFURL (path, cfUrl, IUrl::kFile) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API BundleImage::getFunctionPointer (CStringPtr name) const
{
	if(nativeRef == NULL)
		return nullptr;
	
	CFStringRef procName = CFStringCreateWithCString (0, name, kCFStringEncodingMacRoman);
	void* result = CFBundleGetFunctionPointerForName ((CFBundleRef)nativeRef, procName);
	
	#if DEBUG_LOG
	if(result == nullptr)
	{
		String id;
		getIdentifier (id);
		CCL_PRINTF ("Function pointer %s not found in library ", name)
		CCL_PRINTLN (id)
	}
	#endif

	CFRelease (procName);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IAttributeList* CCL_API BundleImage::getMetaInfo () const
{
	if(!metaInfo)
	{
		CFBundleRef bundle = (CFBundleRef)nativeRef;
		if(bundle != NULL)
		{
			String bundleId, bundleName, version, executable;
			bundleId.appendNativeString (CFBundleGetValueForInfoDictionaryKey (bundle, kCFBundleIdentifierKey));
			bundleName.appendNativeString (CFBundleGetValueForInfoDictionaryKey (bundle, kCFBundleNameKey));
			version.appendNativeString (CFBundleGetValueForInfoDictionaryKey (bundle, kCFBundleVersionKey));
			executable.appendNativeString (CFBundleGetValueForInfoDictionaryKey (bundle, kCFBundleExecutableKey));
			
			metaInfo = NEW Attributes;
			metaInfo->setAttribute (Meta::kPackageID, bundleId);
			metaInfo->setAttribute (Meta::kPackageName, bundleName);
			metaInfo->setAttribute (Meta::kPackageVersion, version);
			metaInfo->setAttribute (Meta::kPackageExecutable, executable);
		}
	}
	
	return metaInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BundleImage::getBinaryPath (IUrl& path) const
{
	ASSERT (nativeRef != NULL)
	if(nativeRef == NULL)
		return false;
	CFObj<CFURLRef> binaryURL = CFBundleCopyExecutableURL ((CFBundleRef)nativeRef);
	return binaryURL ? MacUtils::urlFromCFURL (path, binaryURL, IUrl::kFile) : false;
}

//************************************************************************************************
// DylibImage
//************************************************************************************************

DylibImage::DylibImage (ModuleRef nativeRef, bool isLoaded)
: ExecutableImage (nativeRef, isLoaded)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DylibImage::~DylibImage ()
{
	if(isLoaded && nativeRef)
		unload ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ModuleRef CCL_API DylibImage::getNativeReference () const
{
	ASSERT (false) // this should not be called
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DylibImage::unload ()
{
	if(nativeRef)
		::dlclose (nativeRef);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API DylibImage::getFunctionPointer (CStringPtr name) const
{
	void* result = nullptr;
	if(nativeRef)
	{
		result = ::dlsym (nativeRef, name);
		if(!result)
			CCL_PRINTF ("Function pointer not found in library: %s\n", ::dlerror ());
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DylibImage::getPath (IUrl& url) const
{
	url = path;
	return true;
}

//************************************************************************************************
// CocoaSystemInformation
//************************************************************************************************

CocoaSystemInformation::CocoaSystemInformation ()
{
	gregorianLocal = [[NSCalendar alloc] initWithCalendarIdentifier:NSCalendarIdentifierGregorian];
	gregorianUTC = [[NSCalendar alloc] initWithCalendarIdentifier:NSCalendarIdentifierGregorian];
	[gregorianUTC setTimeZone:[NSTimeZone timeZoneWithAbbreviation:@"UTC"]];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaSystemInformation::~CocoaSystemInformation ()
{
	if(gregorianLocal)
		[gregorianLocal release];
	if(gregorianUTC)
		[gregorianUTC release];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API CocoaSystemInformation::getNumberOfCPUs () const
{
	int count = 1;
	size_t size = sizeof(count);
	if(sysctlbyname ("hw.physicalcpu_max", &count, &size, nullptr, 0) == noErr)
		return count;
	else
		return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API CocoaSystemInformation::getNumberOfCores () const
{
	int count = 1;
    
	size_t size = sizeof(count);
	if(sysctlbyname ("hw.logicalcpu_max", &count, &size, nullptr, 0) == noErr)
		return count;
	else
		return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaSystemInformation::getLocalTime (DateTime& dateTime) const
{
	NSDate* now = [NSDate date];
	NSDateComponents* components = [gregorianLocal components:(NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay | NSCalendarUnitHour | NSCalendarUnitMinute | NSCalendarUnitSecond | NSCalendarUnitNanosecond) fromDate:now];
	
	MacUtils::NSDateComponentsToDateTime (components, dateTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaSystemInformation::convertLocalTimeToUTC (DateTime& utc, const DateTime& localTime) const
{
	NSDateComponents* localComponents = [[NSDateComponents alloc] init];
	MacUtils::DateTimeToNSDateComponents (localTime, localComponents);
	NSDate* date = [gregorianLocal dateFromComponents:localComponents];
	[localComponents release];
	ASSERT (date)
	if(date)
	{
		NSDateComponents* utcComponents = [gregorianUTC components:(NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay | NSCalendarUnitHour | NSCalendarUnitMinute | NSCalendarUnitSecond | NSCalendarUnitNanosecond) fromDate:date];
		
		MacUtils::NSDateComponentsToDateTime (utcComponents, utc);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaSystemInformation::convertUTCToLocalTime (DateTime& localTime, const DateTime& utc) const
{
	NSDateComponents* utcComponents = [[NSDateComponents alloc] init];
	MacUtils::DateTimeToNSDateComponents (utc, utcComponents);
	NSDate* date = [gregorianUTC dateFromComponents:utcComponents];
	[utcComponents release];
	ASSERT (date)
	if(date)
	{
		NSDateComponents* localComponents = [gregorianLocal components:(NSCalendarUnitYear | NSCalendarUnitMonth | NSCalendarUnitDay | NSCalendarUnitHour | NSCalendarUnitMinute | NSCalendarUnitSecond | NSCalendarUnitNanosecond) fromDate:date];
		
		MacUtils::NSDateComponentsToDateTime (localComponents, localTime);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaSystemInformation::convertUnixTimeToUTC (DateTime& utc, int64 unixTime) const
{
	PosixTimeConversion::convertUnixTimeToUTC (utc, unixTime);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API CocoaSystemInformation::convertUTCToUnixTime (const DateTime& utc) const
{
	return PosixTimeConversion::convertUTCToUnixTime (utc);
}
