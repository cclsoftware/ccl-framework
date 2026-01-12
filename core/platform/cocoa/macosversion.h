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
// Filename    : core/platform/cocoa/macosversion.h
// Description : Check macOS version
//
//************************************************************************************************

#ifndef _macosversion_h
#define _macosversion_h

#import <Cocoa/Cocoa.h>

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool GetMacOSVersion (int& major, int& minor, int& patch)
{
	bool result = false;
	
	NSDictionary * systemVersion = [NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"];
	NSString *versionString = [systemVersion objectForKey:@"ProductVersion"];
	NSArray* numbers = [versionString componentsSeparatedByString:@"."];

	if([numbers count] > 0)
		major = [[numbers objectAtIndex:0] intValue];
	if([numbers count] > 1)
    {
		minor = [[numbers objectAtIndex:1] intValue];
        result = true;
    }
	if([numbers count] > 2)
		patch = [[numbers objectAtIndex:2] intValue];
    else
        patch = 0;
    
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool IsAtLeastMacOSVersion (int wantMajor, int wantMinor, int wantPatch = 0)
{
	int major = 0;
	int minor = 0;
	int patch = 0;
	
	if(GetMacOSVersion (major, minor, patch))
		return major >= wantMajor && minor >= wantMinor && patch >= wantPatch;
	else
		return false;
}

#endif // _macosversion_h
