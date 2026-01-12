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
// Filename    : core/platform/android/corefilesystem.android.cpp
// Description : File System Android implementation
//
//************************************************************************************************

#include "core/platform/android/corefilesystem.android.h"

#include "core/platform/shared/jni/corejnihelper.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// android.content.Context
//************************************************************************************************

DECLARE_JNI_CLASS (Context, "android/content/Context")
	DECLARE_JNI_METHOD (jobject, getCacheDir)
	DECLARE_JNI_METHOD (jobject, getFilesDir)
END_DECLARE_JNI_CLASS (Context)

DEFINE_JNI_CLASS (Context)
	DEFINE_JNI_METHOD (getCacheDir, "()Ljava/io/File;")
	DEFINE_JNI_METHOD (getFilesDir, "()Ljava/io/File;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// dev.ccl.core.CurrentContext
//************************************************************************************************

DECLARE_JNI_CLASS (CurrentContext, "dev/ccl/core/CurrentContext")
	DECLARE_JNI_STATIC_METHOD (jobject, get)
END_DECLARE_JNI_CLASS (CurrentContext)

DEFINE_JNI_CLASS (CurrentContext)
	DEFINE_JNI_STATIC_METHOD (get, "()Landroid/content/Context;")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace Core

using namespace Core;
using namespace Java;
using namespace Platform;

//************************************************************************************************
// FileSystem
//************************************************************************************************

IFileSystem& FileSystem::instance ()
{
	static AndroidFileSystem theFileSystem;
	return theFileSystem;
}

//************************************************************************************************
// AndroidFileSystem
//************************************************************************************************

void AndroidFileSystem::getDirectory (FileName& dirname, DirType type)
{
	if(!Jni::getEnvironment ())
		return;

	JniAccessor jni;
	LocalRef file;
	LocalRef currentContext (jni, CurrentContext.get ());

	switch(type)
	{
	case kTempDir :
		if(currentContext)
			file.assign (jni, Context.getCacheDir (currentContext));
		break;
	case kDataDir :
	case kAppDir :
	case kAppSupportDir :
		if(currentContext)
			file.assign (jni, Context.getFilesDir (currentContext));
		break;
	case kHomeDir :
	case kSharedDataDir :
	case kSharedAppDir :
	case kSharedAppSupportDir :
		ASSERT (false)
		break;
	case kWorkingDir :
		::getcwd (dirname.getBuffer (), dirname.getSize ());
		break;
	}

    if(file != nullptr)
	{ 
		LocalStringRef localString (jni, File.getAbsolutePath (file));
		JniCStringChars pathString (jni, localString);
		if(pathString)
			dirname = (CStringPtr)pathString;
	}
}
