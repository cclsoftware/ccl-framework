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
// Filename    : ccl/platform/cocoa/gui/clipboard.ios.mm
// Description : iOS Clipboard
//
//************************************************************************************************

#include "ccl/gui/system/clipboard.h"

#include "ccl/public/text/cclstring.h"

#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

namespace CCL {

//************************************************************************************************
// IOSClipBoard
//************************************************************************************************

class IOSClipBoard: public Clipboard
{
public:
	// IClipboard
	tbool CCL_API setText (StringRef text);
	tbool CCL_API getText (String& text) const;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// IOSClipBoard
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (Clipboard, IOSClipBoard)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API IOSClipBoard::setText (StringRef text)
{
	NSString* nsString = text.createNativeString<NSString*> ();
	[[UIPasteboard generalPasteboard] setValue:nsString forPasteboardType:@"public.text"];

	[nsString release];
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API IOSClipBoard::getText (String& text) const
{
	NSString* nsString = [[UIPasteboard generalPasteboard] valueForPasteboardType:@"public.text"];
	text.appendNativeString (nsString);
	return true;
}
