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
// Filename    : ccl/platform/android/gui/androidintent.cpp
// Description : Android Intent
//
//************************************************************************************************

#include "androidintent.h"

#include "ccl/platform/android/interfaces/jni/androidcontent.h"

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// AndroidIntent
//************************************************************************************************

AndroidIntent::AndroidIntent (JNIEnv* jni, jobject object)
: JniObject (jni, object)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidIntent::~AndroidIntent ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CString AndroidIntent::getAction () const
{
	JniAccessor jni;
	LocalStringRef string (jni, AndroidIntentClass.getAction (*this));
	return MutableCString (fromJavaString (string));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String AndroidIntent::getDataString () const
{
	JniAccessor jni;
	LocalStringRef string (jni, AndroidIntentClass.getDataString (*this));
	return fromJavaString (string);
}
