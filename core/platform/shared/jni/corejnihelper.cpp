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
// Filename    : core/platform/shared/jni/corejnihelper.cpp
// Description : JNI (Java Native Interface) Helper
//
//************************************************************************************************

#include "core/system/corethread.h"

#include "core/platform/shared/jni/corejnihelper.h"

using namespace Core;
using namespace Java;

//************************************************************************************************
// Core::Jni
//************************************************************************************************

Jni Jni::data;

//////////////////////////////////////////////////////////////////////////////////////////////////

Jni& Jni::getInstance ()
{
	return data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Jni::Jni ()
: jni (0), vm (0), jniSlot (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Jni::init (JNIEnv* jni, JavaVM* vm)
{
	if(!vm && jni)
		jni->GetJavaVM (&vm);

	ASSERT (vm)

	data.jni = jni;
	data.vm = vm;
	data.jniSlot = Threads::TLS::allocate ();

	JniClassRegistry::instance ().initializeClasses (getEnvironment ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Jni::share (const Jni& other)
{
	ASSERT (!data.jni && !data.vm && !data.jniSlot)
	ASSERT (other.jni && other.vm)

	data.jni = other.jni;
	data.vm = other.vm;
	data.jniSlot = other.jniSlot;

	JniClassRegistry::instance ().initializeClasses (getEnvironment ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Jni::exit ()
{
	JniClassRegistry::instance ().terminateClasses (getEnvironment ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

JNIEnv* Jni::getEnvironment ()
{
	JNIEnv* jni = (JNIEnv*)Threads::TLS::getValue (data.jniSlot);

	if(jni == nullptr) // attach new thread to jvm
		if(data.vm && data.vm->AttachCurrentThread (&jni, nullptr) == JNI_OK)
			Threads::TLS::setValue (data.jniSlot, jni);

	ASSERT (jni != nullptr) // modules using JNI must include corejnionload.cpp
	return jni;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Jni::detachCurrentThread ()
{
	if(Threads::TLS::getValue (data.jniSlot) != 0)
	{
	   data.vm->DetachCurrentThread ();
	   Threads::TLS::setValue (data.jniSlot, 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Jni::checkException ()
{
	return checkException (getEnvironment ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Jni::checkException (JNIEnv* jni)
{
	if(jni->ExceptionCheck ())
	{
		jni->ExceptionDescribe (); // writes to logcat
		jni->ExceptionClear ();
		return true;
	}
	return false;
}

