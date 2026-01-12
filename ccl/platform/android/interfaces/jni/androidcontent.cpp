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
// Filename    : ccl/platform/android/interfaces/jni/androidcontent.cpp
// Description : JNI Wrapper for android.content classes
//
//************************************************************************************************

#include "androidcontent.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// android.content.Context
//************************************************************************************************

DEFINE_JNI_CLASS (Context)
	DEFINE_JNI_METHOD (getSystemService, "(Ljava/lang/String;)Ljava/lang/Object;")
	DEFINE_JNI_METHOD (getPackageResourcePath, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getCacheDir, "()Ljava/io/File;")
	DEFINE_JNI_METHOD (getNoBackupFilesDir, "()Ljava/io/File;")
	DEFINE_JNI_METHOD (getDir, "(Ljava/lang/String;I)Ljava/io/File;")
	DEFINE_JNI_METHOD (getFilesDir, "()Ljava/io/File;")
	DEFINE_JNI_METHOD (getExternalFilesDir, "(Ljava/lang/String;)Ljava/io/File;")
	DEFINE_JNI_METHOD (getSharedPreferences, "(Ljava/lang/String;I)Landroid/content/SharedPreferences;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// android.content.Intent
//************************************************************************************************

DEFINE_JNI_CLASS (AndroidIntentClass)
	DEFINE_JNI_METHOD (getAction, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (getDataString, "()Ljava/lang/String;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// android.content.SharedPreferences
//************************************************************************************************

DEFINE_JNI_CLASS (SharedPreferences)
	DEFINE_JNI_METHOD (contains, "(Ljava/lang/String;)Z")
	DEFINE_JNI_METHOD (edit, "()Landroid/content/SharedPreferences$Editor;")
	DEFINE_JNI_METHOD (getString, "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// android.content.SharedPreferences.Editor
//************************************************************************************************

DEFINE_JNI_CLASS (SharedPreferencesEditor)
	DEFINE_JNI_METHOD (clear, "()Landroid/content/SharedPreferences$Editor;")
	DEFINE_JNI_METHOD (commit, "()Z")
	DEFINE_JNI_METHOD (putString, "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/SharedPreferences$Editor;")
	DEFINE_JNI_METHOD (remove, "(Ljava/lang/String;)Landroid/content/SharedPreferences$Editor;")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL
