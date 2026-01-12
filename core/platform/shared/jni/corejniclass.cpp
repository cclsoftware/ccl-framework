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
// Filename    : core/platform/shared/jni/corejniclass.cpp
// Description : JNI class information management
//
//************************************************************************************************

#include "core/platform/shared/jni/corejniclass.h"
#include "core/platform/shared/jni/corejniobject.h"
#include "core/platform/shared/jni/corejnienvironment.h"

using namespace Core;
using namespace Java;

//************************************************************************************************
// JniClassRegistry
//************************************************************************************************

JniClassRegistry& JniClassRegistry::instance ()
{
	static JniClassRegistry theInstance;
	return theInstance;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void JniClassRegistry::initializeClasses (JNIEnv* jni)
{
	VectorForEachFast (classes, JniClass*, c)
		c->initialize (jni);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void JniClassRegistry::terminateClasses (JNIEnv* jni)
{
	VectorForEachFast (classes, JniClass*, c)
		c->terminate (jni);
	EndFor
}

//************************************************************************************************
// JniMethodBase
//************************************************************************************************

va_list JniMethodBase::va_empty;

//************************************************************************************************
// JniMethod
//************************************************************************************************

template<> jobject JniMethod::callInternal<jobject> (jobject object, va_list args) const
{
	return Jni::getEnvironment ()->CallObjectMethodV (object, methodID, args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> jobjectArray JniMethod::callInternal<jobjectArray> (jobject object, va_list args) const
{
	return jobject_cast<jobjectArray> (callInternal<jobject> (object, args));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> jstring JniMethod::callInternal<jstring> (jobject object, va_list args) const
{
	return jobject_cast<jstring> (callInternal<jobject> (object, args));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> int JniMethod::callInternal<int> (jobject object, va_list args) const
{
	return Jni::getEnvironment ()->CallIntMethodV (object, methodID, args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> int64 JniMethod::callInternal<int64> (jobject object, va_list args) const
{
	return Jni::getEnvironment ()->CallLongMethodV (object, methodID, args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> float JniMethod::callInternal<float> (jobject object, va_list args) const
{
	return Jni::getEnvironment ()->CallFloatMethodV (object, methodID, args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> bool JniMethod::callInternal<bool> (jobject object, va_list args) const
{
	return Jni::getEnvironment ()->CallBooleanMethodV (object, methodID, args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> void JniMethod::callInternal<void> (jobject object, va_list args) const
{
	return Jni::getEnvironment ()->CallVoidMethodV (object, methodID, args);
}

//************************************************************************************************
// JniStaticMethod
//************************************************************************************************

template<> jobject JniStaticMethod::callInternal<jobject> (va_list args) const
{
	return Jni::getEnvironment ()->CallStaticObjectMethodV (clazz, methodID, args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> jobjectArray JniStaticMethod::callInternal<jobjectArray> (va_list args) const
{
	return jobject_cast<jobjectArray> (callInternal<jobject> (args));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> jstring JniStaticMethod::callInternal<jstring> (va_list args) const
{
	return jobject_cast<jstring> (callInternal<jobject> (args));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> int JniStaticMethod::callInternal<int> (va_list args) const
{
	return Jni::getEnvironment ()->CallStaticIntMethodV (clazz, methodID, args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> int64 JniStaticMethod::callInternal<int64> (va_list args) const
{
	return Jni::getEnvironment ()->CallStaticLongMethodV (clazz, methodID, args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> float JniStaticMethod::callInternal<float> (va_list args) const
{
	return Jni::getEnvironment ()->CallStaticFloatMethodV (clazz, methodID, args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> bool JniStaticMethod::callInternal<bool> (va_list args) const
{
	return Jni::getEnvironment ()->CallStaticBooleanMethodV (clazz, methodID, args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> void JniStaticMethod::callInternal<void> (va_list args) const
{
	return Jni::getEnvironment ()->CallStaticVoidMethodV (clazz, methodID, args);
}
