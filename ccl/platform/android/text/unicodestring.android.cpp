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
// Filename    : ccl/platform/android/text/unicodestring.android.cpp
// Description : Android-specific Unicode String Implementation
//
//************************************************************************************************

#include "ccl/platform/android/text/unicodestring.android.h"
#include "ccl/text/strings/unicode-cross-platform/ucharfunctions.h"

#include "ccl/platform/android/cclandroidjni.h"

#define USE_GLOBAL_JSTRING 0

using namespace CCL;

//************************************************************************************************
// Text functions => use cross-platform Unicode functions
//************************************************************************************************

#include "ccl/text/strings/unicodestringbuffer.impl.h"

//************************************************************************************************
// UnicodeString
//************************************************************************************************

UnicodeString* UnicodeString::newString ()
{
	return NEW AndroidUnicodeString;
}

//************************************************************************************************
// AndroidUnicodeString
//************************************************************************************************

IString* CCL_API AndroidUnicodeString::cloneString () const
{
	return NEW AndroidUnicodeString (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void* CCL_API AndroidUnicodeString::createNativeString () const
{
	JNIEnv* env = Core::Java::Jni::getEnvironment ();
	const uchar* string = text ? text : kEmpty;
	int length = Text::getLength (string);		
	jstring javaString = env->NewString (string, length);
	#if USE_GLOBAL_JSTRING
	jobject globalString = env->NewGlobalRef (javaString);
	env->DeleteLocalRef (javaString);
	return globalString;
	#else
	return javaString;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidUnicodeString::releaseNativeString (void* nativeString) const
{
	jstring javaString = reinterpret_cast<jstring> (nativeString);
	if(javaString)
	{
		JNIEnv* env = Core::Java::Jni::getEnvironment ();
		#if USE_GLOBAL_JSTRING
		env->DeleteGlobalRef (javaString);
		#else
		env->DeleteLocalRef (javaString);
		#endif
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidUnicodeString::appendNativeString (const void* nativeString)
{
	jstring javaString = reinterpret_cast<jstring> (const_cast<void*> (nativeString));
	if(javaString == 0)
		return kResultInvalidArgument;

	Core::Java::JniStringChars chars (Core::Java::Jni::getEnvironment (), javaString);
	return appendChars (chars, chars.length ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AndroidUnicodeString::isNormalized (NormalizationForm form) const
{
	// implement me
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidUnicodeString::normalize (NormalizationForm form)
{
	return kResultNotImplemented;
}

//************************************************************************************************
// UnicodeUtilities
//************************************************************************************************

UnicodeUtilities& UnicodeUtilities::getInstance ()
{
	static AndroidUnicodeUtilities theInstance;
	return theInstance;
}
