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
// Filename    : ccl/platform/android/text/unicodestring.android.h
// Description : Android-specific Unicode String Implementation
//
//************************************************************************************************

#ifndef _unicodestring_android_h
#define _unicodestring_android_h

#include "ccl/text/strings/unicodestringbuffer.h"

namespace Core {
namespace Java {
class Jni; }}

namespace CCL {

//************************************************************************************************
// AndroidUnicodeString
//************************************************************************************************

class AndroidUnicodeString: public UnicodeStringBuffer
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
// AndroidUnicodeUtilities
//************************************************************************************************

class AndroidUnicodeUtilities: public UnicodeUtilities
{
public:
	// TODO!
};

} // namespace CCL

#endif // _unicodestring_android_h
