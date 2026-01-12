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
// Filename    : ccl/platform/android/graphics/androidtextlayout.cpp
// Description : Android Text Layout
//
//************************************************************************************************

#include "androidtextlayout.h"
#include "androidgraphics.h"
#include "frameworkgraphics.h"

#include "ccl/public/gui/graphics/updatergn.h"

//************************************************************************************************
// JNI classes
//************************************************************************************************

namespace CCL {
namespace Android {

//************************************************************************************************
// FrameworkTextLayout
//************************************************************************************************

DECLARE_JNI_CLASS (FrameworkTextLayout, CCLGUI_CLASS_PREFIX "FrameworkTextLayout")
	DECLARE_JNI_CONSTRUCTOR (construct, jstring, int, int, int, int, int, jobject, int, float, float, float)
	DECLARE_JNI_METHOD (void, setFontStyle, int, int, int, bool)
	DECLARE_JNI_METHOD (void, setFontSize, int, int, int)
	DECLARE_JNI_METHOD (void, setSpacing, int, int, float)
	DECLARE_JNI_METHOD (void, setLineSpacing, int, int, float)
	DECLARE_JNI_METHOD (void, setBaselineOffset, int, int, float)
	DECLARE_JNI_METHOD (void, setRelativeSize, int, int, float)
	DECLARE_JNI_METHOD (void, setTextColor, int, int, int)
	DECLARE_JNI_METHOD (int, getLineForOffset, int)
	DECLARE_JNI_METHOD (int, getLineStart, int)
	DECLARE_JNI_METHOD (int, getLineEnd, int)
	DECLARE_JNI_METHOD (int, getOffsetToLeftOf, int)
	DECLARE_JNI_METHOD (int, getOffsetToRightOf, int)
	DECLARE_JNI_METHOD (float, getWidth)
	DECLARE_JNI_METHOD (int, getHeight)
	DECLARE_JNI_METHOD (int, getBaseline)
	DECLARE_JNI_METHOD (void, getBounds, jobject)
	DECLARE_JNI_METHOD (void, getBoundsF, jobject)
	DECLARE_JNI_METHOD (void, getImageBounds, jobject)
	DECLARE_JNI_METHOD (void, getRangeBounds, jobject, int, int)
	DECLARE_JNI_METHOD (int, getOffsetForPosition, float, float)
	DECLARE_JNI_METHOD (void, draw, jobject, float, float, int, int)
	DECLARE_JNI_METHOD (bool, resize, int, int)
END_DECLARE_JNI_CLASS (FrameworkTextLayout)

DEFINE_JNI_CLASS (FrameworkTextLayout)
	DEFINE_JNI_CONSTRUCTOR (construct, "(Ljava/lang/String;IIIIILandroid/graphics/Typeface;IFFF)V")
	DEFINE_JNI_METHOD (setFontStyle, "(IIIZ)V")
	DEFINE_JNI_METHOD (setFontSize, "(III)V")
	DEFINE_JNI_METHOD (setSpacing, "(IIF)V")
	DEFINE_JNI_METHOD (setLineSpacing, "(IIF)V")
	DEFINE_JNI_METHOD (setBaselineOffset, "(IIF)V")
	DEFINE_JNI_METHOD (setRelativeSize, "(IIF)V")
	DEFINE_JNI_METHOD (setTextColor, "(III)V")
	DEFINE_JNI_METHOD (getLineForOffset, "(I)I")
	DEFINE_JNI_METHOD (getLineStart, "(I)I")
	DEFINE_JNI_METHOD (getLineEnd, "(I)I")
	DEFINE_JNI_METHOD (getOffsetToLeftOf, "(I)I")
	DEFINE_JNI_METHOD (getOffsetToRightOf, "(I)I")
	DEFINE_JNI_METHOD (getWidth, "()F")
	DEFINE_JNI_METHOD (getHeight, "()I")
	DEFINE_JNI_METHOD (getBaseline, "()I")
	DEFINE_JNI_METHOD (getBounds, "(Landroid/graphics/Rect;)V")
	DEFINE_JNI_METHOD (getBoundsF, "(Landroid/graphics/RectF;)V")
	DEFINE_JNI_METHOD (getImageBounds, "(Landroid/graphics/RectF;)V")
	DEFINE_JNI_METHOD (getRangeBounds, "(Landroid/graphics/RectF;II)V")
	DEFINE_JNI_METHOD (getOffsetForPosition, "(FF)I")
	DEFINE_JNI_METHOD (draw, "(L" CCLGUI_CLASS_PREFIX "FrameworkGraphics;FFII)V")
	DEFINE_JNI_METHOD (resize, "(II)Z")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// AndroidTextLayout
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AndroidTextLayout, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidTextLayout::AndroidTextLayout ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidTextLayout::draw (FrameworkGraphics& device, PointF pos, Color color, int options)
{
	FrameworkTextLayout.draw (layout, device, pos.x, pos.y, FrameworkGraphics::toJavaColor (color), options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::construct (StringRef text, Coord width, Coord height, FontRef font, LineMode mode, TextFormatRef format)
{
	JniAccessor jni;
	JniCCLString jniString (text);

	FrameworkGraphics::FontHelper f (font);
	int align = format.getAlignment ().align;
	float letterSpacing = FrameworkGraphics::FontHelper::getLetterSpacing (font);

	LocalRef newLayout (jni, jni.newObject (FrameworkTextLayout, FrameworkTextLayout.construct, jniString.getString (), width, height, align, mode, format.getFlags (), f.typeface, font.getStyle (), font.getSize (), letterSpacing, font.getLineSpacing ()));
	layout.assign (jni, newLayout);

	this->text = text;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::construct (StringRef text, CoordF width, CoordF height, FontRef font, LineMode lineMode, TextFormatRef format)
{
	// (there is no float equivalent for the implementation on the java side)
	return construct (text, coordFToInt (width), coordFToInt (height), font, lineMode, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::resize (Coord width, Coord height)
{
	if(!FrameworkTextLayout.resize (layout, width, height))
		return kResultFailed;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::resize (CoordF width, CoordF height)
{
	// (there is no float equivalent for the implementation on the java side)
	return resize (coordFToInt (width), coordFToInt (height));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::setFontStyle (const Range& range, int style, tbool state)
{
	if(!layout)
		return kResultUnexpected;

	FrameworkTextLayout.setFontStyle (layout, range.start, range.start + range.length, style, state);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::setFontSize (const Range& range, float size)
{
	if(!layout)
		return kResultUnexpected;

	FrameworkTextLayout.setFontSize (layout, range.start, range.start + range.length, size);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::setSpacing (const Range& range, float spacing)
{
	if(!layout)
		return kResultUnexpected;

	FrameworkTextLayout.setSpacing (layout, range.start, range.start + range.length, spacing);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::setLineSpacing (const Range& range, float lineSpacing)
{
	if(!layout)
		return kResultUnexpected;

	FrameworkTextLayout.setLineSpacing (layout, range.start, range.start + range.length, lineSpacing);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::setBaselineOffset (const Range& range, float offset)
{
	if(!layout)
		return kResultUnexpected;

	FrameworkTextLayout.setBaselineOffset (layout, range.start, range.start + range.length, offset);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::setSuperscript (const Range& range)
{
	if(!layout)
		return kResultUnexpected;

	FrameworkTextLayout.setBaselineOffset (layout, range.start, range.start + range.length, kSuperscriptBaselineFactor);
	FrameworkTextLayout.setRelativeSize (layout, range.start, range.start + range.length, kSuperscriptSizeFactor);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::setSubscript (const Range& range)
{
	if(!layout)
		return kResultUnexpected;

	FrameworkTextLayout.setBaselineOffset (layout, range.start, range.start + range.length, -kSubscriptBaselineFactor);
	FrameworkTextLayout.setRelativeSize (layout, range.start, range.start + range.length, kSubscriptSizeFactor);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::setTextColor (const Range& range, Color color)
{
	if(!layout)
		return kResultUnexpected;

	FrameworkTextLayout.setTextColor (layout, range.start, range.start + range.length, FrameworkGraphics::toJavaColor (color));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::getBounds (Rect& bounds, int flags) const
{
	if(!layout)
		return kResultUnexpected;

	JniAccessor jni;
	JniObject androidRect;
	androidRect.newObject (jni, AndroidRect);

	FrameworkTextLayout.getBounds (layout, androidRect);

	FrameworkGraphics::toCCLRect (bounds, jni, androidRect);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::getBounds (RectF& bounds, int flags) const
{
	if(!layout)
		return kResultUnexpected;

	JniAccessor jni;
	JniObject androidRect;
	androidRect.newObject (jni, AndroidRectF);

	FrameworkTextLayout.getBoundsF (layout, androidRect);

	FrameworkGraphics::toCCLRect (bounds, jni, androidRect);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::getImageBounds (RectF& bounds) const
{
	if(!layout)
		return kResultUnexpected;

	JniAccessor jni;
	JniObject androidRect;
	androidRect.newObject (jni, AndroidRectF);

	FrameworkTextLayout.getImageBounds (layout, androidRect);

	FrameworkGraphics::toCCLRect (bounds, jni, androidRect);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::getBaselineOffset (PointF& offset) const
{
	if(!layout)
		return kResultUnexpected;

	offset (0, FrameworkTextLayout.getBaseline (layout));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::hitTest (int& textIndex, PointF& position) const
{
	if(!layout)
		return kResultUnexpected;

	textIndex = FrameworkTextLayout.getOffsetForPosition (layout, position.x, position.y);

	RectF offset;
	getCharacterBounds (offset, textIndex);
	position = offset.getLeftTop ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::getCharacterBounds (RectF& offset, int textIndex) const
{
	if(!layout)
		return kResultUnexpected;

	JniAccessor jni;
	JniObject androidRect;
	androidRect.newObject (jni, AndroidRectF);

	int nextIndex = FrameworkTextLayout.getOffsetToRightOf (layout, textIndex);
	FrameworkTextLayout.getRangeBounds (layout, androidRect, textIndex, nextIndex);

	RectF rect;
	FrameworkGraphics::toCCLRect (rect, jni, androidRect);
	offset = rect;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::getTextBounds (IMutableRegion& bounds, const Range& range) const
{
	if(!layout)
		return kResultUnexpected;

	JniAccessor jni;
	int startLine = FrameworkTextLayout.getLineForOffset (layout, range.start);
	int endLine = FrameworkTextLayout.getLineForOffset (layout, range.start + range.length);

	for(int i = startLine; i <= endLine; i++)
	{
		int startIndex = (i == startLine) ? range.start : FrameworkTextLayout.getLineStart (layout, i);
		int endIndex = (i == endLine) ? range.start + range.length : FrameworkTextLayout.getLineEnd (layout, i);

		JniObject androidRect;
		androidRect.newObject (jni, AndroidRectF);

		FrameworkTextLayout.getRangeBounds (layout, androidRect, startIndex, endIndex);

		RectF rect;
		FrameworkGraphics::toCCLRect (rect, jni, androidRect);
		bounds.addRect (rectFToInt (rect));
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidTextLayout::getLineRange (Range& range, int textIndex) const
{
	if(!layout)
		return kResultUnexpected;

	int line = FrameworkTextLayout.getLineForOffset (layout, textIndex);
	range.start = FrameworkTextLayout.getLineStart (layout, line);
	range.length = FrameworkTextLayout.getLineEnd (layout, line) - range.start;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API AndroidTextLayout::getText () const
{
	return text;
}
