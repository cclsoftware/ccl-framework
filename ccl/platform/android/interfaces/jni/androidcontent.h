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
// Filename    : ccl/platform/android/interfaces/jni/androidcontent.h
// Description : JNI Wrapper for android.content classes
//
//************************************************************************************************

#ifndef _ccl_androidcontent_h
#define _ccl_androidcontent_h

#include "ccl/platform/android/cclandroidjni.h"

#include "ccl/public/base/unknown.h"
#include "ccl/public/text/cstring.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// android.content.Context
//************************************************************************************************

DECLARE_JNI_CLASS (Context, "android/content/Context")
	enum FileCreationMode
	{
		MODE_PRIVATE = 0
	};

	DECLARE_JNI_METHOD (jobject, getSystemService, jstring)
	DECLARE_JNI_METHOD (jstring, getPackageResourcePath)
	DECLARE_JNI_METHOD (jobject, getCacheDir)
	DECLARE_JNI_METHOD (jobject, getNoBackupFilesDir)
	DECLARE_JNI_METHOD (jobject, getDir, jstring, int)
	DECLARE_JNI_METHOD (jobject, getFilesDir)
	DECLARE_JNI_METHOD (jobject, getExternalFilesDir, jstring)
	DECLARE_JNI_METHOD (jobject, getSharedPreferences, jstring, FileCreationMode)

	// type constants, e.g. for getExternalFilesDir
	DECLARE_STRINGID_MEMBER (DIRECTORY_DOCUMENTS)
	DECLARE_STRINGID_MEMBER (DIRECTORY_DOWNLOADS)
	DECLARE_STRINGID_MEMBER (DIRECTORY_MUSIC)
	DECLARE_STRINGID_MEMBER (DIRECTORY_RECORDINGS)
END_DECLARE_JNI_CLASS (Context)

///////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_STRINGID_MEMBER (ContextClass, DIRECTORY_MUSIC, "Music")
DEFINE_STRINGID_MEMBER (ContextClass, DIRECTORY_DOWNLOADS, "Download")
DEFINE_STRINGID_MEMBER (ContextClass, DIRECTORY_DOCUMENTS, "Documents")
DEFINE_STRINGID_MEMBER (ContextClass, DIRECTORY_RECORDINGS, "Recordings")

//************************************************************************************************
// android.content.Intent
//************************************************************************************************

DECLARE_JNI_CLASS (AndroidIntentClass, "android/content/Intent")
	DECLARE_JNI_METHOD (jstring, getAction)
	DECLARE_JNI_METHOD (jstring, getDataString)
END_DECLARE_JNI_CLASS (AndroidIntentClass)

//************************************************************************************************
// android.content.SharedPreferences
//************************************************************************************************

DECLARE_JNI_CLASS (SharedPreferences, "android/content/SharedPreferences")
	DECLARE_JNI_METHOD (bool, contains, jstring)
	DECLARE_JNI_METHOD (jobject, edit)
	DECLARE_JNI_METHOD (jstring, getString, jstring, jstring)
END_DECLARE_JNI_CLASS (SharedPreferences)

//************************************************************************************************
// android.content.SharedPreferences.Editor
//************************************************************************************************

DECLARE_JNI_CLASS (SharedPreferencesEditor, "android/content/SharedPreferences$Editor")
	DECLARE_JNI_METHOD (jobject, clear)
	DECLARE_JNI_METHOD (bool, commit)
	DECLARE_JNI_METHOD (jobject, putString, jstring, jstring)
	DECLARE_JNI_METHOD (jobject, remove)
END_DECLARE_JNI_CLASS (SharedPreferencesEditor)

//************************************************************************************************
// Constants
//************************************************************************************************

namespace Action 
{
	DEFINE_STRINGID (kActionMain, "android.intent.action.MAIN")

	DEFINE_STRINGID (kActionEdit, "android.intent.action.EDIT")
	DEFINE_STRINGID (kActionView, "android.intent.action.VIEW")

	DEFINE_STRINGID (kActionDefault, kActionView)
}

} // namespace Android
} // namespace CCL

#endif // _ccl_androidcontent_h
