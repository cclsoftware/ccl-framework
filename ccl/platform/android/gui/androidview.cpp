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
// Filename    : ccl/platform/android/gui/androidview.cpp
// Description : JNI Wrapper for android.view classes
//
//************************************************************************************************

#include "androidview.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// android.view.View
//************************************************************************************************

DEFINE_JNI_CLASS (AndroidView)
	DEFINE_JNI_METHOD (getLeft, "()I")
	DEFINE_JNI_METHOD (getTop, "()I")
	DEFINE_JNI_METHOD (getWidth, "()I")
	DEFINE_JNI_METHOD (getHeight, "()I")
	DEFINE_JNI_METHOD (getX, "()F")
	DEFINE_JNI_METHOD (getY, "()F")
	DEFINE_JNI_METHOD (setX, "(F)V")
	DEFINE_JNI_METHOD (setY, "(F)V")
	DEFINE_JNI_METHOD (getAlpha, "()F")
	DEFINE_JNI_METHOD (setAlpha, "(F)V")
	DEFINE_JNI_METHOD (invalidate, "()V")
	DEFINE_JNI_METHOD (setVisibility, "(I)V")
	DEFINE_JNI_METHOD (getVisibility, "()I")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// android.view.ViewGroup
//************************************************************************************************

DEFINE_JNI_CLASS (ViewGroup)
	DEFINE_JNI_METHOD (addView,  "(Landroid/view/View;)V")
    DEFINE_JNI_METHOD_NAME (addViewAt, addView, "(Landroid/view/View;I)V")
	DEFINE_JNI_METHOD (removeView, "(Landroid/view/View;)V")
	DEFINE_JNI_METHOD (setClipChildren, "(Z)V")
	DEFINE_JNI_METHOD (indexOfChild, "(Landroid/view/View;)I")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL
