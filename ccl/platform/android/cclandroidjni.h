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
// Filename    : ccl/platform/android/cclandroidjni.h
// Description : Android-specific JNI Helper for CCL
//
//************************************************************************************************

#ifndef _cclandroidjni_h
#define _cclandroidjni_h

#include "core/platform/shared/jni/corejnihelper.h"

#include "ccl/public/text/cclstring.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// JNI Helper Macros for CCL
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CCLGUI_CLASS_PREFIX "dev/ccl/cclgui/"
#define CCLSYSTEM_CLASS_PREFIX "dev/ccl/cclsystem/"

// Declare native methods of a Java class in package 'dev.ccl.cclgui"
#define DECLARE_JNI_CLASS_METHOD_CCLGUI(ReturnType,ClassName,MethodName,...) DECLARE_JNI_CLASS_METHOD (dev_ccl_cclgui, ReturnType, ClassName, MethodName, __VA_ARGS__)
#define DECLARE_JNI_CLASS_METHOD_CCLGUI_NO_ARGS(ReturnType,ClassName,MethodName) DECLARE_JNI_CLASS_METHOD_NO_ARGS (dev_ccl_cclgui, ReturnType, ClassName, MethodName)

// Declare native methods of a Java class in package 'dev.ccl.cclsystem"
#define DECLARE_JNI_CLASS_METHOD_CCLSYSTEM(ReturnType,ClassName,MethodName,...) DECLARE_JNI_CLASS_METHOD (dev_ccl_cclsystem, ReturnType, ClassName, MethodName, __VA_ARGS__)
#define DECLARE_JNI_CLASS_METHOD_CCLSYSTEM_NO_ARGS(ReturnType,ClassName,MethodName) DECLARE_JNI_CLASS_METHOD_NO_ARGS (dev_ccl_cclsystem, ReturnType, ClassName, MethodName)

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CCL {
namespace Android {

using Core::Java::Jni;
using Core::Java::JniAccessor;
using Core::Java::JniClass;
using Core::Java::JniMethod;
using Core::Java::JniMethodTyped;
using Core::Java::JniString;
using Core::Java::JniStringChars;
using Core::Java::JniCStringChars;
using Core::Java::LocalRef;
using Core::Java::LocalStringRef;
using Core::Java::JniObject;
using Core::Java::JniIntPtr;
using Core::Java::JniCast;
using Core::Java::JniByteArray;
using Core::Java::JniIntArray;
using Core::Java::JniFloatArray;

using Core::Java::jobject_cast;

} // namespace Android
} // namespace CCL

namespace CCL {

//************************************************************************************************
// JniCCLString
/* Create a jstring from a CCL::String. */
//************************************************************************************************

class JniCCLString: public NativeString<jstring>
{
public:
	JniCCLString (StringRef string)
	: NativeString<jstring> (string)
	{}

	jstring getString () const { return nativeString; } // for compatibility with JniString class
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// fromJavaString: assign a java string reference to a CCL String
//////////////////////////////////////////////////////////////////////////////////////////////////

inline String& fromJavaString (String& string, JNIEnv* jni, jstring javaString)
{
	if(javaString)
	{
		Android::JniStringChars chars (jni, javaString);
		jsize length = chars.length ();
		if(length > 0) // don't call assign with count == 0
			string.assign (chars, length); // java UTF-16 strings are not zero-terminated!
		else
			string.empty ();
	}
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline String& fromJavaString (String& string, const Android::LocalStringRef& javaString)
{
	return fromJavaString (string, javaString.getJni (), javaString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline String fromJavaString (const Android::LocalStringRef& javaString)
{
	String string;
	return fromJavaString (string, javaString);
}

} // namespace CCL

#endif // _cclandroidjni_h
