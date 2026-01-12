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
// Filename    : ccl/platform/android/graphics/paintcache.cpp
// Description : Cache of Java Paint Objects
//
//************************************************************************************************

#include "ccl/platform/android/graphics/frameworkgraphics.h"

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// BitmapPaintData
//************************************************************************************************

jobject BitmapPaintData::createJavaPaint (const JniAccessor& jni, int javaIndex, const BitmapPaintData& data)
{
	return FrameworkGraphicsFactoryClass.createCachedBitmapPaint (*gGraphicsFactory, javaIndex, data.alpha, data.filtered);
}

//************************************************************************************************
// FillPaintData
//************************************************************************************************

FillPaintData::FillPaintData (SolidBrushRef brush, bool antiAlias)
: color (FrameworkGraphics::toJavaColor (brush.getColor ())),
  antiAlias (antiAlias)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject FillPaintData::createJavaPaint (const JniAccessor& jni, int javaIndex, const FillPaintData& data)
{
	return FrameworkGraphicsFactoryClass.createCachedFillPaint (*gGraphicsFactory, javaIndex, data.color, data.antiAlias);
}

//************************************************************************************************
// DrawPaintData
//************************************************************************************************

DrawPaintData::DrawPaintData (PenRef pen, bool antiAlias)
: FillPaintData (FrameworkGraphics::toJavaColor (pen.getColor ()), antiAlias),
  width (pen.getWidth ()),
  penStyle (pen.getStyle ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject DrawPaintData::createJavaPaint (const JniAccessor& jni, int javaIndex, const DrawPaintData& data)
{
	return FrameworkGraphicsFactoryClass.createCachedDrawPaint (*gGraphicsFactory, javaIndex, data.color, data.width, data.penStyle, data.antiAlias);
}

//************************************************************************************************
// TextPaintData
//************************************************************************************************

TextPaintData::TextPaintData (FontRef font, SolidBrushRef brush)
: typeface (FrameworkGraphics::FontHelper::getTypeFace (font)),
  style (font.getStyle ()),
  fontSize (font.getSize ()),
  spacing (FrameworkGraphics::FontHelper::getLetterSpacing (font)),
  color (FrameworkGraphics::toJavaColor (brush.getColor ()))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextPaintData::TextPaintData (FontRef font)
: typeface (FrameworkGraphics::FontHelper::getTypeFace (font)),
  style (font.getStyle ()),
  fontSize (font.getSize ()),
  spacing (FrameworkGraphics::FontHelper::getLetterSpacing (font)),
  color (0) // todo: treat color as "don't care" for measurements
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject TextPaintData::createJavaPaint (const JniAccessor& jni, int javaIndex, const TextPaintData& data)
{
	return FrameworkGraphicsFactoryClass.createCachedTextPaint (*gGraphicsFactory, javaIndex, data.typeface, data.style, data.fontSize, data.spacing, data.color);
}
