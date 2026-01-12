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
// Filename    : ccl/platform/android/graphics/androidgraphics.cpp
// Description : JNI Wrapper for android.graphics classes
//
//************************************************************************************************

#include "androidgraphics.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// android.graphics.Point
//************************************************************************************************

DEFINE_JNI_CLASS (AndroidPoint)
	DEFINE_JNI_DEFAULT_CONSTRUCTOR
	DEFINE_JNI_CONSTRUCTOR (construct, "(II)V")
	DEFINE_JNI_FIELD (x, "I")
	DEFINE_JNI_FIELD (y, "I")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// android.graphics.PointF
//************************************************************************************************

DEFINE_JNI_CLASS (AndroidPointF)
	DEFINE_JNI_DEFAULT_CONSTRUCTOR
	DEFINE_JNI_CONSTRUCTOR (construct, "(FF)V")
	DEFINE_JNI_FIELD (x, "F")
	DEFINE_JNI_FIELD (y, "F")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// android.graphics.Rect
//************************************************************************************************

DEFINE_JNI_CLASS (AndroidRect)
	DEFINE_JNI_DEFAULT_CONSTRUCTOR
	DEFINE_JNI_CONSTRUCTOR (construct, "(IIII)V")
	DEFINE_JNI_FIELD (left, "I")
	DEFINE_JNI_FIELD (top, "I")
	DEFINE_JNI_FIELD (right, "I")
	DEFINE_JNI_FIELD (bottom, "I")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// android.graphics.RectF
//************************************************************************************************

DEFINE_JNI_CLASS (AndroidRectF)
	DEFINE_JNI_DEFAULT_CONSTRUCTOR
	DEFINE_JNI_CONSTRUCTOR (construct, "(FFFF)V")
	DEFINE_JNI_FIELD (left, "F")
	DEFINE_JNI_FIELD (top, "F")
	DEFINE_JNI_FIELD (right, "F")
	DEFINE_JNI_FIELD (bottom, "F")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// android.graphics.Bitmap
//************************************************************************************************

DEFINE_JNI_CLASS (Bitmap)
	DEFINE_JNI_METHOD (recycle, "()V")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// 	android.graphics.Typeface
//************************************************************************************************

DEFINE_JNI_CLASS (Typeface)
	DEFINE_JNI_STATIC_METHOD (create, "(Ljava/lang/String;I)Landroid/graphics/Typeface;")
	DEFINE_JNI_STATIC_METHOD_NAME (createWithTypeface, create, "(Landroid/graphics/Typeface;I)Landroid/graphics/Typeface;")
	DEFINE_JNI_STATIC_METHOD (createFromFile, "(Ljava/lang/String;)Landroid/graphics/Typeface;")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL
