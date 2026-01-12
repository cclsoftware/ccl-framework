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
// Filename    : ccl/platform/android/gui/androidview.h
// Description : JNI Wrapper for android.view classes
//
//************************************************************************************************

#ifndef _ccl_androidview_h
#define _ccl_androidview_h

#include "ccl/platform/android/cclandroidjni.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// android.view.View
//************************************************************************************************

DECLARE_JNI_CLASS (AndroidView, "android/view/View")
	DECLARE_JNI_METHOD (int, getLeft)
	DECLARE_JNI_METHOD (int, getTop)
	DECLARE_JNI_METHOD (int, getWidth)
	DECLARE_JNI_METHOD (int, getHeight)
	DECLARE_JNI_METHOD (float, getX)
	DECLARE_JNI_METHOD (float, getY)
	DECLARE_JNI_METHOD (void, setX, float)
	DECLARE_JNI_METHOD (void, setY, float)
	DECLARE_JNI_METHOD (float, getAlpha)
	DECLARE_JNI_METHOD (void, setAlpha, float)
	DECLARE_JNI_METHOD (void, invalidate)
	DECLARE_JNI_METHOD (void, setVisibility, int)
	DECLARE_JNI_METHOD (int, getVisibility)
END_DECLARE_JNI_CLASS (AndroidView)

//************************************************************************************************
// android.view.ViewGroup
//************************************************************************************************

DECLARE_JNI_CLASS (ViewGroup, "android/view/ViewGroup")
	DECLARE_JNI_METHOD (void, addView, jobject)
	DECLARE_JNI_METHOD (void, removeView, jobject)
	DECLARE_JNI_METHOD (void, setClipChildren, bool)
END_DECLARE_JNI_CLASS (ViewGroup)

} // namespace Android
} // namespace CCL

#endif // _ccl_androidview_h
