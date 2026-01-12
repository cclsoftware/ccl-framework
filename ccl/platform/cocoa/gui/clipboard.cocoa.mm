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
// Filename    : ccl/platform/cocoa/gui/clipboard.cocoa.mm
// Description : Mac OS Clipboard
//
//************************************************************************************************

#include "ccl/gui/system/clipboard.h"

#include "ccl/public/text/cclstring.h"

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"


namespace CCL {

//************************************************************************************************
// CocoaClipboard
//************************************************************************************************

class CocoaClipboard: public Clipboard
{
public:
	CocoaClipboard ();

	// Clipboard
	bool setNativeText (StringRef text);
	bool getNativeText (String& text) const;
	bool hasNativeContentChanged ();

protected:
	NSInteger changeCount;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// CocoaClipboard
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (Clipboard, CocoaClipboard)

CocoaClipboard::CocoaClipboard ()
: changeCount (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaClipboard::setNativeText (StringRef text)
{
	NSString* nsString = text.createNativeString<NSString*> ();

	NSPasteboard *pasteBoard = [NSPasteboard generalPasteboard];
	[pasteBoard declareTypes:[NSArray arrayWithObjects:NSPasteboardTypeString, nil] owner:nil];
	[pasteBoard setString:nsString forType:NSPasteboardTypeString];

	[nsString release];
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaClipboard::getNativeText (String& text) const
{
	NSString* nsString = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
	text.appendNativeString (nsString);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaClipboard::hasNativeContentChanged ()
{
	NSInteger c = [[NSPasteboard generalPasteboard] changeCount];
	if(c != changeCount)
	{
		changeCount = c;
		return true;
	}
	return false;
}
