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
// Filename    : core/platform/shared/jni/corejniobject.impl.h
// Description : JNI object (jobject) smart pointers for local and global references
//
//************************************************************************************************

#ifndef _corejniobject_impl_h
#define _corejniobject_impl_h

#include "core/platform/shared/jni/corejniobject.h"

#include <jni.h>

namespace Core {
namespace Java {

class JniAccessor;
class JniClass;

//************************************************************************************************
// LocalRef inline
//************************************************************************************************

inline LocalRef::LocalRef (JNIEnv* jni, jobject object)
: JniObjectBase (object), jni (jni)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline LocalRef::LocalRef (const LocalRef& localRef)
{
	if(localRef.jni && localRef.object)
	{
		jni = localRef.jni;
		object = jni->NewLocalRef (localRef.object);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void LocalRef::assign (JNIEnv* jni, jobject object)
{
	ASSERT (this->object == 0)
	this->jni = jni;
	this->object = object;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline LocalRef::~LocalRef ()
{
	if(object)
		jni->DeleteLocalRef (object);
}

//************************************************************************************************
// JniObject inline
//************************************************************************************************

inline JniObject::JniObject (JNIEnv* jni, jobject object)
: JniObjectBase (jni->NewGlobalRef (object))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline JniObject::JniObject ()
: JniObjectBase (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline JniObject::~JniObject ()
{
	if(object)
		Jni::getEnvironment ()->DeleteGlobalRef (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline JniObject& JniObject::newObject (const JniAccessor& jni, JniClass& jniClass)
{
	LocalRef localRef (jni, jni.newObject (jniClass));
	assign (jni, localRef);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline JniObject& JniObject::newObject (JniClass& jniClass)
{
	return newObject (JniAccessor (), jniClass);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline JniObject& JniObject::operator = (jobject object)
{
	assign (Jni::getEnvironment (), object);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void JniObject::assign (JNIEnv* jni, jobject _object)
{
	ASSERT (jni)
	if(!jni)
		return;

	if(object)
		jni->DeleteGlobalRef (object);

	object = _object;

	if(object)
		object = jni->NewGlobalRef (object);
}

} // namespace Java
} // namespace Core

#endif // _corejniobject_impl_h
