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
// Filename    : core/platform/shared/jni/corejnistring.h
// Description : JNI String helpers for transferring strings to and from java
//
//************************************************************************************************

#ifndef _corejnistring_h
#define _corejnistring_h

#include "core/public/coretypes.h"

#include <jni.h>

namespace Core {
namespace Java {

//************************************************************************************************
// JniString
//************************************************************************************************

class JniString
{
public:
	JniString (JNIEnv* jni, CStringPtr text);
	JniString (JNIEnv* jni, const uchar* text, int length);
	~JniString ();

	jstring getString () const { return string; }

	operator jstring () const { return string; }

protected:
	JNIEnv* jni;
	jstring string;
};

//************************************************************************************************
// JniStringChars, JniCStringChars
/** Helpers for accessing charcters of a java string. */
//************************************************************************************************

template<class Char>
class JniStringCharsBase
{
public:
	JniStringCharsBase (JNIEnv* jni, jstring string);

	operator const Char* () const	{ return chars; }

protected:
	JNIEnv* jni;
	jstring string;
	const Char* chars;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

class JniStringChars: public JniStringCharsBase<jchar>
{
public:
	JniStringChars (JNIEnv* jni, jstring string);
	~JniStringChars ();

	jsize length () const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

class JniCStringChars: public JniStringCharsBase<char>
{
public:
	JniCStringChars (JNIEnv* jni, jstring string);
	~JniCStringChars ();

	jsize length () const;
};

//************************************************************************************************
// JniString inline
//************************************************************************************************

inline JniString::JniString (JNIEnv* jni, CStringPtr text)
: jni (jni),
  string (jni->NewStringUTF (text))
{
	ASSERT (string)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline JniString::JniString (JNIEnv* jni, const uchar* text, int length)
: jni (jni),
  string (jni->NewString (text, length))
{
	ASSERT(string)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline JniString::~JniString ()
{
	jni->DeleteLocalRef (string);
}

//************************************************************************************************
// JniStringCharsBase inline
//************************************************************************************************

template<class Char>
inline JniStringCharsBase<Char>::JniStringCharsBase (JNIEnv* jni, jstring string)
: jni (jni),
  string (string),
  chars (0)
{}

//************************************************************************************************
// JniStringChars inline
//************************************************************************************************

inline JniStringChars::JniStringChars (JNIEnv* jni, jstring string)
: JniStringCharsBase (jni, string)
{
	if(string)
		chars = jni->GetStringChars (string, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline JniStringChars::~JniStringChars ()
{
	if(chars)
		jni->ReleaseStringChars (string, chars);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline jsize JniStringChars::length () const
{
	return jni->GetStringLength (string);
}

//************************************************************************************************
// JniCStringChars inline
//************************************************************************************************

inline JniCStringChars::JniCStringChars (JNIEnv* jni, jstring string)
: JniStringCharsBase (jni, string)
{
	if(string)
		chars = jni->GetStringUTFChars (string, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline JniCStringChars::~JniCStringChars ()
{
	if(chars)
		jni->ReleaseStringUTFChars (string, chars);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline jsize JniCStringChars::length () const
{
	return jni->GetStringUTFLength (string);
}

} // namespace Java
} // namespace Core

#endif // _corejnistring_h
