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
// Filename    : core/platform/shared/jni/corejnionload.cpp
// Description : JNI module entry
//
//************************************************************************************************

#include "core/platform/shared/jni/corejnihelper.h"

using namespace Core;
using namespace Java;

//////////////////////////////////////////////////////////////////////////////////////////////////
// JNI module entry
// - must be linked in each module that requires JNI access
// - the module must be explicitly loaded from the Java side, see FrameworkActivity.loadNativeLibraries
// - note: the counterpart JNI_OnUnload is never called on Android
//////////////////////////////////////////////////////////////////////////////////////////////////

JNIEXPORT jint JNI_OnLoad (JavaVM* vm, void* reserved)
{
	if(vm)
	{
		JNIEnv* jni = 0;
		if(vm->GetEnv (reinterpret_cast<void**> (&jni), JNI_VERSION_1_6) != JNI_OK)
			return -1;

		Jni::init (jni, vm);
	}
	return JNI_VERSION_1_6;
}
