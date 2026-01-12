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
// Filename    : core/platform/shared/jni/corejniclass.h
// Description : JNI class information management
//
//************************************************************************************************

#ifndef _corejniclass_h
#define _corejniclass_h

#include "core/public/corevector.h"
#include "core/public/coretypes.h"

#include <jni.h>

namespace Core {
namespace Java {

class JniClass;

//************************************************************************************************
// JniFieldBase
//************************************************************************************************

class JniFieldBase
{
public:
	bool isValid () const;
	void initialize (JNIEnv* jni, JniClass& c, CStringPtr methodName, CStringPtr signature);

protected:
	jfieldID fieldID;

	JniFieldBase ();
};

//************************************************************************************************
// JniField
//************************************************************************************************

template<typename T>
class JniField: public JniFieldBase
{
public:
	T getValue (JNIEnv* jni, jobject object) const;
	void setValue (JNIEnv* jni, jobject object, T value) const;
};

//************************************************************************************************
// JniMethodBase
//************************************************************************************************

class JniMethodBase
{
public:
	bool isValid () const;

protected:
	friend class JniClass;
	friend class JniAccessor;

	static va_list va_empty;
	jmethodID methodID;

	JniMethodBase ();
};

//************************************************************************************************
// JniMethod
//************************************************************************************************

class JniMethod: public JniMethodBase
{
public:
	void initialize (JNIEnv* jni, JniClass& c, CStringPtr methodName, CStringPtr signature);

protected:
	template<class Ret> Ret call (jobject object, ...) const;

private:
	template<class Ret> Ret callInternal (jobject object, va_list args = va_empty) const;
};

//************************************************************************************************
// JniMethodTyped
//************************************************************************************************

template<class Ret, class... Args>
class JniMethodTyped: public JniMethod
{
public:
	Ret operator () (jobject object, Args... args) const
	{
		return JniMethod::call<Ret> (object, args...);
	}
};

//************************************************************************************************
// JniStaticMethod
//************************************************************************************************

class JniStaticMethod: public JniMethodBase
{
public:
	void initialize (JNIEnv* jni, JniClass& c, CStringPtr methodName, CStringPtr signature);

protected:
	template<class Ret> Ret call (...) const;

private:
	jclass clazz;

	template<class Ret> Ret callInternal (va_list args = va_empty) const;
};

//************************************************************************************************
// JniStaticMethodTyped
//************************************************************************************************

template<class Ret, class... Args>
class JniStaticMethodTyped: public JniStaticMethod
{
public:
	Ret operator () (Args... args) const
	{
		return JniStaticMethod::call<Ret> (args...);
	}
};

//************************************************************************************************
// JniClass
//************************************************************************************************

class JniClass
{
public:
	JniClass (CStringPtr className);
	JniClass (jclass clazz);
	JniClass (JNIEnv* jni, jobject object);
	virtual ~JniClass ();

	virtual void initialize (JNIEnv* jni);
	void terminate (JNIEnv* jni);

	operator jclass () const;
	jclass getClass () const;

protected:
	friend class JniMethod;
	friend class JniStaticMethod;
	friend class JniFieldBase;
	friend class JniAccessor;
	CStringPtr className;
	jclass clazz;
	JniMethod defaultConstructor;
};

//************************************************************************************************
// Class path prefix for Java classes in package "dev.ccl.core"
//************************************************************************************************

#define CORE_CLASS_PREFIX "dev/ccl/core/"

//************************************************************************************************
// Macros for declaring and implementing a JNI meta class with it's methods and fields
//************************************************************************************************

#define DECLARE_JNI_CLASS(ClassName, classPath) \
struct ClassName##Class: public Core::Java::JniClass \
{ \
	ClassName##Class (): JniClass (classPath) {} \
	void initialize (JNIEnv* jni) override;
#define DECLARE_JNI_CONSTRUCTOR(methodName, ...) Core::Java::JniMethodTyped<void, ##__VA_ARGS__> methodName;
#define DECLARE_JNI_METHOD(returnType, methodName, ...) Core::Java::JniMethodTyped<returnType, ##__VA_ARGS__> methodName;
#define DECLARE_JNI_STATIC_METHOD(returnType, methodName, ...) Core::Java::JniStaticMethodTyped<returnType, ##__VA_ARGS__> methodName;
#define DECLARE_JNI_FIELD(type, fieldName) Core::Java::JniField<type> fieldName;
#define END_DECLARE_JNI_CLASS(ClassName) }; extern ClassName##Class ClassName;

#define DEFINE_JNI_CLASS(ClassName) \
ClassName##Class ClassName;\
void ClassName##Class::initialize (JNIEnv* jni) { JniClass::initialize (jni);
#define DEFINE_JNI_CONSTRUCTOR(methodName, signature) methodName.initialize (jni, *this, "<init>", signature);
#define DEFINE_JNI_DEFAULT_CONSTRUCTOR DEFINE_JNI_CONSTRUCTOR (defaultConstructor, "()V")
#define DEFINE_JNI_METHOD(methodName, signature) methodName.initialize (jni, *this, #methodName, signature);
#define DEFINE_JNI_METHOD_NAME(methodName, javaName, signature) methodName.initialize (jni, *this, #javaName, signature);
#define DEFINE_JNI_STATIC_METHOD(methodName, signature) DEFINE_JNI_METHOD (methodName, signature)
#define DEFINE_JNI_STATIC_METHOD_NAME(methodName, javaName, signature) DEFINE_JNI_METHOD_NAME (methodName, javaName, signature)
#define DEFINE_JNI_FIELD(fieldName, signature) fieldName.initialize (jni, *this, #fieldName, signature);
#define END_DEFINE_JNI_CLASS }

//************************************************************************************************
// Macros for declaring native methods of a Java class as C functions
//************************************************************************************************

#define DECLARE_JNI_CLASS_METHOD(package,ReturnType,ClassName,MethodName,...) \
extern "C" JNIEXPORT ReturnType Java_##package##_##ClassName##_##MethodName (JNIEnv* env, jobject This, __VA_ARGS__)

#define DECLARE_JNI_CLASS_METHOD_NO_ARGS(package,ReturnType,ClassName,MethodName) \
extern "C" JNIEXPORT ReturnType Java_##package##_##ClassName##_##MethodName (JNIEnv* env, jobject This)

// for classes in package 'core":
#define DECLARE_JNI_CLASS_METHOD_CORE(ReturnType,ClassName,MethodName,...) DECLARE_JNI_CLASS_METHOD (dev_ccl_core, ReturnType, ClassName, MethodName, __VA_ARGS__)
#define DECLARE_JNI_CLASS_METHOD_CORE_NO_ARGS(ReturnType,ClassName,MethodName) DECLARE_JNI_CLASS_METHOD_NO_ARGS (dev_ccl_core, ReturnType, ClassName, MethodName)

//************************************************************************************************
// JniClassRegistry
//************************************************************************************************

class JniClassRegistry
{
public:
	static JniClassRegistry& instance ();

	void initializeClasses (JNIEnv* jni);
	void terminateClasses (JNIEnv* jni);

protected:
	friend class JniClass;
	void addClass (JniClass* jniClass);

private:
	Core::Vector<JniClass*> classes;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

// JniMethod::callInternal specialization for void return type
template<> void JniMethod::callInternal<void> (jobject object, va_list) const;

// JniStaticMethod::callInternal specialization for void return type
template<> void JniStaticMethod::callInternal<void> (va_list) const;

// JniField::get specializations
template<> inline float JniField<float>::getValue (JNIEnv* jni, jobject object) const { return jni->GetFloatField (object, fieldID); }
template<> inline int JniField<int>::getValue (JNIEnv* jni, jobject object) const { return jni->GetIntField (object, fieldID); }
template<> inline int64 JniField<int64>::getValue (JNIEnv* jni, jobject object) const { return jni->GetLongField (object, fieldID); }
template<> inline bool JniField<bool>::getValue (JNIEnv* jni, jobject object) const { return jni->GetBooleanField (object, fieldID); }

// JniField::set specializations
template<> inline void JniField<float>::setValue (JNIEnv* jni, jobject object, float value) const { jni->SetFloatField (object, fieldID, value); }
template<> inline void JniField<int>::setValue (JNIEnv* jni, jobject object, int value) const { jni->SetIntField (object, fieldID, value); }
template<> inline void JniField<int64>::setValue (JNIEnv* jni, jobject object, int64 value) const { jni->SetLongField (object, fieldID, value); }
template<> inline void JniField<bool>::setValue (JNIEnv* jni, jobject object, bool value) const { jni->SetBooleanField (object, fieldID, value); }

//************************************************************************************************
// JniClassRegistry inline
//************************************************************************************************

inline void JniClassRegistry::addClass (JniClass* jniClass)
{
	classes.add (jniClass);
}

//************************************************************************************************
// JniMethodBase inline
//************************************************************************************************

inline JniMethodBase::JniMethodBase ()
: methodID (0) {}

inline bool JniMethodBase::isValid () const
{ return methodID != 0; }

//************************************************************************************************
// JniMethod inline
//************************************************************************************************

inline void JniMethod::initialize (JNIEnv* jni, JniClass& c, CStringPtr methodName, CStringPtr signature)
{
	ASSERT (c.clazz)
	if(!c.clazz)
		return;

	methodID = jni->GetMethodID (c.clazz, methodName, signature);
	ASSERT (methodID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Ret>
Ret JniMethod::call (jobject object, ...) const
{
	va_list args;
	va_start (args, object);
	const Ret& result = callInternal<Ret> (object, args);
	va_end (args);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
inline void JniMethod::call (jobject object, ...) const
{
	va_list args;
	va_start (args, object);
	callInternal<void> (object, args);
	va_end (args);
}

//************************************************************************************************
// JniStaticMethod inline
//************************************************************************************************

inline void JniStaticMethod::initialize (JNIEnv* jni, JniClass& c, CStringPtr methodName, CStringPtr signature)
{
	ASSERT (c.clazz)
	if(!c.clazz)
		return;

	clazz = c.clazz;

	methodID = jni->GetStaticMethodID (clazz, methodName, signature);
	ASSERT (methodID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Ret>
Ret JniStaticMethod::call (...) const
{
	va_list args;
	va_start (args, this);
	const Ret& result = callInternal<Ret> (args);
	va_end (args);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
inline void JniStaticMethod::call (...) const
{
	va_list args;
	va_start (args, this);
	callInternal<void> (args);
	va_end (args);
}

//************************************************************************************************
// JniFieldBase inline
//************************************************************************************************

inline JniFieldBase::JniFieldBase ()
: fieldID (0) {}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool JniFieldBase::isValid () const
{ return fieldID != 0; }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void JniFieldBase::initialize (JNIEnv* jni, JniClass& c, CStringPtr fieldName, CStringPtr signature)
{
	if(c.clazz)
	{
		fieldID = jni->GetFieldID (c.clazz, fieldName, signature);
		ASSERT (fieldID);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// JniClass inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline JniClass::JniClass (CStringPtr className)
: className (className),
  clazz (0)
{
	JniClassRegistry::instance ().addClass (this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline JniClass::JniClass (jclass clazz)
: className (0), clazz (clazz)
{
	ASSERT (clazz);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline JniClass::JniClass (JNIEnv* jni, jobject object)
: className (0),
  clazz (jni->GetObjectClass (object))
{
	ASSERT (clazz);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline JniClass::~JniClass ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void JniClass::initialize (JNIEnv* jni)
{
	jclass localClass = jni->FindClass (className);
	clazz = localClass ? reinterpret_cast<jclass> (jni->NewGlobalRef (localClass)) : 0;
	ASSERT (clazz)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void JniClass::terminate (JNIEnv* jni)
{
	if(clazz)
		jni->DeleteGlobalRef (clazz);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline JniClass::operator jclass () const
{ return clazz; }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline jclass JniClass::getClass () const
{ return clazz; }

} // namespace Java
} // namespace Core

#endif // _corejniclass_h
