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
// Filename    : core/platform/shared/jni/corejnienvironment.h
// Description : JNI environment helpers
//
//************************************************************************************************

#ifndef _corejnienvironment_h
#define _corejnienvironment_h

#include "core/platform/shared/jni/corejniclass.h"

#include "core/public/corethreading.h"

namespace Core {
namespace Java {

//************************************************************************************************
/** Jni.
	Static functions that provide access to a JNIEnv instance per thread. */
//************************************************************************************************

class Jni
{
public:
	Jni ();

	static void init (JNIEnv* jni, JavaVM* vm);
	static void share (const Jni& jni);
	static void exit ();

	static JNIEnv* getEnvironment (); ///< return JNIEnv instance for current thread
	static void detachCurrentThread ();

	static bool checkException ();
	static bool checkException (JNIEnv* jni);

	static Jni& getInstance ();

private:
	static Jni data;

	JNIEnv* jni;
	JavaVM* vm;
	Threads::TLSRef jniSlot;
};

//************************************************************************************************
// JniThreadScope
//************************************************************************************************

struct JniThreadScope
{
	JniThreadScope () { Jni::getEnvironment (); }
    ~JniThreadScope ()  { Jni::detachCurrentThread (); }
};

//************************************************************************************************
// JniAccessor
/** Helper that locally stores a JNIEnv pointer to avoid multiple successive calls to Jni::getEnvironment (). */
//************************************************************************************************

class JniAccessor
{
public:
	JniAccessor ()
	: jni (Jni::getEnvironment ())
	{}

	JniAccessor (JNIEnv* jni)
	: jni (jni)
	{}

	bool isValid () const			{ return jni != 0; }
	JNIEnv* operator -> () const	{ return jni; }
	operator JNIEnv* () const		{ return jni; }
	operator JNIEnv*& ()			{ return jni; }
	JNIEnv* getJni ()				{ return jni; }

	bool checkException ()			{ return Jni::checkException (jni); }

	// object constructors
	jobject newObject (const JniClass& jniClass) const;										///< default constructor
	jobject newObject (const JniClass& jniClass, const JniMethod& constructor, ...) const;	///< constructor with arguments

	// static (class) methods / fields
	jobject getStaticField (const JniClass& jniClass, CStringPtr fieldName, CStringPtr fieldType) const;

	// Object methods / fields
	template<class T> T getField (jobject object, const JniField<T>& field) const;
	template<class T> void setField (jobject object, const JniField<T>& field, T value) const;

private:
	JNIEnv* jni;
};

//************************************************************************************************
// JniAccessor inline
//************************************************************************************************

inline jobject JniAccessor::newObject (const JniClass& c) const
{
	ASSERT (c.defaultConstructor.isValid ())
	return c.defaultConstructor.isValid () ? jni->NewObject (c.clazz, c.defaultConstructor.methodID) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline jobject JniAccessor::newObject (const JniClass& c, const JniMethod& constructor, ...) const
{
	va_list args;
	va_start (args, constructor);
	jobject object = jni->NewObjectV (c.clazz, constructor.methodID, args);
	va_end (args);
	return object;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline jobject JniAccessor::getStaticField (const JniClass& c, CStringPtr fieldName, CStringPtr fieldType) const
{
	// todo: JniStaticMethod
	jfieldID fieldID = jni->GetStaticFieldID (c.clazz, fieldName, fieldType);
	return fieldID ? jni->GetStaticObjectField (c.clazz, fieldID) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
T JniAccessor::getField (jobject object, const JniField<T>& field) const
{
	return field.getValue (jni, object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
void JniAccessor::setField (jobject object, const JniField<T>& field, T value) const
{
	field.setValue (jni, object, value);
}

} // namespace Java
} // namespace Core

#endif // _corejnienvironment_h
