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
// Filename    : ccl/platform/win/text/unicodestring.win.h
// Description : Windows-specific Unicode String Implementation
//
//************************************************************************************************

#ifndef _unicodestring_win_h
#define _unicodestring_win_h

#include "ccl/text/strings/unicodestringbuffer.h"

namespace CCL {

//************************************************************************************************
// WindowsUnicodeString
//************************************************************************************************

class WindowsUnicodeString: public UnicodeStringBuffer
{
public:
	// UnicodeStringBuffer
	IString* CCL_API cloneString () const override;
	void* CCL_API createNativeString () const override;
	void CCL_API releaseNativeString (void* nativeString) const override;
	tresult CCL_API appendNativeString (const void* nativeString) override;
	tbool CCL_API isNormalized (NormalizationForm form) const override;
	tresult CCL_API normalize (NormalizationForm form) override;
};

//************************************************************************************************
// WindowsUnicodeUtilities
//************************************************************************************************

class WindowsUnicodeUtilities: public UnicodeUtilities
{
public:
	// UnicodeUtilities
	tbool CCL_API isAlpha (uchar c) const override;
	tbool CCL_API isAlphaNumeric (uchar c) const override;
	tbool CCL_API isLowercase (uchar c) const override;
	tbool CCL_API isUppercase (uchar c) const override;
	uchar CCL_API toLowercase (uchar c) const override;
	uchar CCL_API toUppercase (uchar c) const override;
};

} // namespace CCL

#endif // _unicodestring_win_h
