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
// Filename    : ccl/platform/android/graphics/androidgraphics.h
// Description : JNI Wrapper for android.graphics classes
//
//************************************************************************************************

#ifndef _ccl_androidgraphics_h
#define _ccl_androidgraphics_h

#include "ccl/platform/android/cclandroidjni.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// android.graphics.Point
//************************************************************************************************

DECLARE_JNI_CLASS (AndroidPoint, "android/graphics/Point")
	DECLARE_JNI_CONSTRUCTOR (construct)
	DECLARE_JNI_FIELD (int, x)
	DECLARE_JNI_FIELD (int, y)
END_DECLARE_JNI_CLASS (AndroidPoint)

//************************************************************************************************
// android.graphics.PointF
//************************************************************************************************

DECLARE_JNI_CLASS (AndroidPointF, "android/graphics/PointF")
	DECLARE_JNI_CONSTRUCTOR (construct)
	DECLARE_JNI_FIELD (int, x)
	DECLARE_JNI_FIELD (int, y)
END_DECLARE_JNI_CLASS (AndroidPointF)

//************************************************************************************************
// android.graphics.Rect
//************************************************************************************************

DECLARE_JNI_CLASS (AndroidRect, "android/graphics/Rect")
	DECLARE_JNI_CONSTRUCTOR (construct)
	DECLARE_JNI_FIELD (int, left)
	DECLARE_JNI_FIELD (int, top)
	DECLARE_JNI_FIELD (int, right)
	DECLARE_JNI_FIELD (int, bottom)
END_DECLARE_JNI_CLASS (AndroidRect)

//************************************************************************************************
// android.graphics.RectF
//************************************************************************************************

DECLARE_JNI_CLASS (AndroidRectF, "android/graphics/RectF")
	DECLARE_JNI_CONSTRUCTOR (construct)
	DECLARE_JNI_FIELD (float, left)
	DECLARE_JNI_FIELD (float, top)
	DECLARE_JNI_FIELD (float, right)
	DECLARE_JNI_FIELD (float, bottom)
END_DECLARE_JNI_CLASS (AndroidRectF)

//************************************************************************************************
// 	android.graphics.Bitmap
//************************************************************************************************

DECLARE_JNI_CLASS (Bitmap, "android/graphics/Bitmap")
	DECLARE_JNI_METHOD (void, recycle)
END_DECLARE_JNI_CLASS (Bitmap)

//************************************************************************************************
// 	android.graphics.Typeface
//************************************************************************************************

DECLARE_JNI_CLASS (Typeface, "android/graphics/Typeface")
	enum Style
	{
		NORMAL	= 0x0,
		BOLD	= 0x1,
		ITALIC	= 0x2,
	};

	DECLARE_JNI_STATIC_METHOD (jobject, create, jstring, int)
	DECLARE_JNI_STATIC_METHOD (jobject, createWithTypeface, jobject, int)
	DECLARE_JNI_STATIC_METHOD (jobject, createFromFile, jstring)
END_DECLARE_JNI_CLASS (Typeface)

} // namespace Android
} // namespace CCL

#endif // _ccl_androidgraphics_h
