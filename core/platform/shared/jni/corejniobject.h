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
// Filename    : core/platform/shared/jni/corejniobject.h
// Description : JNI object (jobject) smart pointers for local and global references
//
//************************************************************************************************

#ifndef _corejniobject_h
#define _corejniobject_h

#include <jni.h>

namespace Core {
namespace Java {

class JniAccessor;
class JniClass;

//************************************************************************************************
// JniObjectBase
//************************************************************************************************

class JniObjectBase
{
public:
	JniObjectBase (jobject object)
	: object (object)
	{}

	JniObjectBase ()
	: object (0)
	{}

	bool isValid () const			{ return object != 0; }
	jobject operator -> () const	{ return object; }
	operator jobject () const		{ return object; }
	operator jobject& ()			{ return object; }
	jobject getJObject () const		{ return object; }

protected:
	jobject object;
};

//************************************************************************************************
// LocalRef
/** Local reference to a Java object, deleted in destructor.
	Both constructor and assign wrap an existing local reference and don't create a new one. */
//************************************************************************************************

class LocalRef: public JniObjectBase
{
public:
	LocalRef (JNIEnv* jni = 0, jobject object = 0); ///< wraps existing local reference, does not create a new one
	LocalRef (const LocalRef& localRef);			///< create a new local reference from an existing one
	~LocalRef ();

	void assign (JNIEnv* jni, jobject object);		///< wraps existing local reference, does not create a new one

	JNIEnv* getJni () const { return jni; }

private:
	JNIEnv* jni;
};

//************************************************************************************************
// LocalStringRef
/** Local reference to a Java string object. */
//************************************************************************************************

class LocalStringRef: public LocalRef
{
public:
	LocalStringRef (JNIEnv* jni, jstring string)
	: LocalRef (jni, string)
	{}

	operator jstring () const		{ return (jstring)object; }
};

//************************************************************************************************
// JniObject
/** Global reference to a java object. The global reference is added in constructor, assign, newObject and deleted in destructor. */
//************************************************************************************************

class JniObject: public JniObjectBase
{
public:
	JniObject (JNIEnv* jni, jobject object);
	JniObject ();
	~JniObject ();

	// create new object (default constructor)
	JniObject& newObject (const JniAccessor& jni, JniClass& jniClass);
	JniObject& newObject (JniClass& jniClass);

	JniObject& operator = (jobject object);

	void assign (JNIEnv* jni, jobject object);
};

//************************************************************************************************
// JniIntPtr + JniCast (pass native objects to Java)
//************************************************************************************************

typedef jlong JniIntPtr;

template <typename T>
struct JniCast
{
	JniIntPtr asIntPtr () { return reinterpret_cast<JniIntPtr> (static_cast<T*> (this)); }

	static T* fromIntPtr (JniIntPtr value) { return reinterpret_cast<T*> (value); }
	static JniIntPtr toIntPtr (T* value) { return reinterpret_cast<JniIntPtr> (value); }
};

//************************************************************************************************
// jobject_cast (cast jobject to derived types)
//************************************************************************************************

template <typename T> T jobject_cast (jobject object) { return static_cast<T> (object); }

} // namespace Java
} // namespace Core

#endif // _corejniobject_h
