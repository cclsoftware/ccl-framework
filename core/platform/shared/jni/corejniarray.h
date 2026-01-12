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
// Filename    : core/platform/shared/jni/corejniarray.h
// Description : JNI Array helper classes
//
//************************************************************************************************

#ifndef _corejniarray_h
#define _corejniarray_h

#include "core/platform/shared/jni/corejnienvironment.h"
#include "core/platform/shared/jni/corejnistring.h"

#include "core/public/corevector.h"
#include "core/public/coreprimitives.h"

#include <jni.h>

namespace Core {
namespace Java {

//************************************************************************************************
// JniArray
//************************************************************************************************

template <typename ArrayType>
class JniArray
{
public:
	~JniArray ()
	{
		if(jArray)
			Jni::getEnvironment ()->DeleteGlobalRef (jArray);
	}

	operator ArrayType () const { return jArray; }
	ArrayType getArray () const { return jArray; }

	int getLength () const		{ return jArray ? Jni::getEnvironment ()->GetArrayLength (jArray) : 0; }

protected:
	ArrayType jArray;

	JniArray ()
	: jArray (0)
	{}

	JniArray (JNIEnv* jni, ArrayType _jArray, bool deleteLocalRef = false)
	: jArray (0)
	{
		assign (jni, _jArray, deleteLocalRef);
	}

	void assign (JNIEnv* jni, ArrayType _jArray, bool deleteLocalRef = false)
	{
		if(jArray)
			jni->DeleteGlobalRef (jArray);

		jArray = _jArray ? ArrayType (jni->NewGlobalRef (_jArray)) : 0;
		if(deleteLocalRef)
			jni->DeleteLocalRef (_jArray); // release the local reference passed to us
	}
};

//************************************************************************************************
// JniByteArray
//************************************************************************************************

class JniByteArray: public JniArray<jbyteArray>
{
public:
	JniByteArray (JNIEnv* jni, const jbyte data[], int length)
	: JniArray (jni, jni->NewByteArray (length), true)
	{
		jni->SetByteArrayRegion (jArray, 0, length, data);
	}

	JniByteArray (JNIEnv* jni, int length)
	: JniArray (jni, jni->NewByteArray (length), true)
	{}

	JniByteArray (JNIEnv* jni, jbyteArray jArray)
	: JniArray (jni, jArray)
	{}

	void reallocate (int length)
	{
		JNIEnv* jni = Jni::getEnvironment ();
		assign (jni, jni->NewByteArray (length), true);
	}

	void setData (const void* buffer, int count)
	{
		Jni::getEnvironment ()->SetByteArrayRegion (jArray, 0, count, reinterpret_cast<jbyte*> (const_cast<void*> (buffer)));
	}

	void setData (const void* buffer, int start, int count)
	{
		Jni::getEnvironment ()->SetByteArrayRegion (jArray, start, count, reinterpret_cast<jbyte*> (const_cast<void*> (buffer)));
	}

	void getData (void* buffer, int count) const
	{
		ASSERT (getLength () >= count)
		if(count)
			Jni::getEnvironment ()->GetByteArrayRegion (jArray, 0, count, reinterpret_cast<jbyte*> (buffer));
	}

	void getData (void* buffer, int start, int count) const
	{
		ASSERT (start + getLength () >= count)
		if(count)
			Jni::getEnvironment ()->GetByteArrayRegion (jArray, start, count, reinterpret_cast<jbyte*> (buffer));
	}
};

//************************************************************************************************
// JniIntArray
//************************************************************************************************

class JniIntArray: public JniArray<jintArray>
{
public:
	JniIntArray (JNIEnv* jni, const ConstVector<jint>& vector)
	: JniArray (jni, jni->NewIntArray (vector.count ()), true)
	{
		jni->SetIntArrayRegion (jArray, 0, vector.count (), vector);
	}

	JniIntArray (JNIEnv* jni, jintArray jArray)
	: JniArray (jni, jArray)
	{}

	void getData (Vector<jint>& data) const
	{
		jsize length = getLength ();
		if(data.getCapacity () < length)
			data.resize (length);

		int count = get_min (data.getCapacity (), length);
		if(count)
			Jni::getEnvironment ()->GetIntArrayRegion (jArray, 0, count, data);
		data.setCount (count);
	}
};

//************************************************************************************************
// JniFloatArray
//************************************************************************************************

class JniFloatArray: public JniArray<jfloatArray>
{
public:
	JniFloatArray (JNIEnv* jni, const ConstVector<jfloat>& vector)
	: JniArray (jni, jni->NewFloatArray (vector.count ()), true)
	{
		jni->SetFloatArrayRegion (jArray, 0, vector.count (), vector);
	}

	JniFloatArray (JNIEnv* jni, jfloatArray jArray)
	: JniArray (jni, jArray)
	{}

	void getData (Vector<jfloat>& data) const
	{
		jsize length = getLength ();
		if(data.getCapacity () < length)
			data.resize (length);

		int count = get_min (data.getCapacity (), length);
		if(count)
			Jni::getEnvironment ()->GetFloatArrayRegion (jArray, 0, count, data);
		data.setCount (count);
	}
};

//************************************************************************************************
// JniObjectArray
//************************************************************************************************

class JniObjectArray: public JniArray<jobjectArray>
{
public:
	JniObjectArray (JNIEnv* jni, int length, CStringPtr jclassName, jobject defaultObject)
	: JniArray (jni, jni->NewObjectArray (length, jni->FindClass (jclassName), defaultObject), true)
	{}

	JniObjectArray (JNIEnv* jni, jobjectArray jArray)
	: JniArray (jni, jArray)
	{}

	jobject operator[] (int index) const
	{
		return Jni::getEnvironment ()->GetObjectArrayElement (jArray, index);
	}

	void setElement (int index, jobject object)
	{
		Jni::getEnvironment ()->SetObjectArrayElement (jArray, index, object);
	}
};

//************************************************************************************************
// JniStringArray
//************************************************************************************************

class JniStringArray: public JniObjectArray
{
public:
	JniStringArray (JNIEnv* jni, int length)
	: JniObjectArray (jni, length, "java/lang/String", JniString (jni, ""))
	{}

	JniStringArray (JNIEnv* jni, jobjectArray jArray)
	: JniObjectArray (jni, jArray)
	{}

	jstring operator[] (int index) const
	{
		return (jstring) JniObjectArray::operator[] (index);
	}

	using JniObjectArray::setElement;

	void setElement (int index, CStringPtr string)
	{
		setElement (index, JniString (Jni::getEnvironment (), string));
	}
};

} // namespace Java
} // namespace Core

#endif // _corejniarray_h
