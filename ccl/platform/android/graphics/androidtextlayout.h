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
// Filename    : ccl/platform/android/graphics/androidtextlayout.h
// Description : Android Text Layout
//
//************************************************************************************************

#ifndef _ccl_androidtextlayout_h
#define _ccl_androidtextlayout_h

#include "ccl/platform/android/cclandroidjni.h"

#include "ccl/gui/graphics/nativegraphics.h"

namespace CCL {
namespace Android {

class FrameworkGraphics;

//************************************************************************************************
// AndroidTextLayout
//************************************************************************************************

class AndroidTextLayout: public NativeTextLayout
{
public:
	DECLARE_CLASS_ABSTRACT (AndroidTextLayout, NativeTextLayout)

	AndroidTextLayout ();

	void draw (FrameworkGraphics& device, PointF pos, Color color, int options);

	// ITextLayout
	tresult CCL_API construct (StringRef text, Coord width, Coord height, FontRef font, LineMode lineMode, TextFormatRef format) override;
	tresult CCL_API construct (StringRef text, CoordF width, CoordF height, FontRef font, LineMode lineMode, TextFormatRef format) override;
	tresult CCL_API resize (Coord width, Coord height) override;
	tresult CCL_API resize (CoordF width, CoordF height) override;
	tresult CCL_API setFontStyle (const Range& range, int style, tbool state) override;
	tresult CCL_API setFontSize (const Range& range, float size) override;
	tresult CCL_API setSpacing (const Range& range, float spacing) override;
	tresult CCL_API setLineSpacing (const Range& range, float lineSpacing) override;
	tresult CCL_API setBaselineOffset (const Range& range, float offset) override;
	tresult CCL_API setSuperscript (const Range& range) override;
	tresult CCL_API setSubscript (const Range& range) override;
	tresult CCL_API setTextColor (const Range& range, Color color) override;
	tresult CCL_API getBounds (Rect& bounds, int flags = 0) const override;
	tresult CCL_API getBounds (RectF& bounds, int flags = 0) const override;
	tresult CCL_API getImageBounds (RectF& bounds) const override;
	tresult CCL_API getBaselineOffset (PointF& offset) const override;
	tresult CCL_API hitTest (int& textIndex, PointF& position) const override;
	tresult CCL_API getCharacterBounds (RectF& offset, int textIndex) const override;
	tresult CCL_API getTextBounds (IMutableRegion& bounds, const Range& range) const override;
	tresult CCL_API getLineRange (Range& range, int textIndex) const override;
	StringRef CCL_API getText () const override;

private:
	String text;
	JniObject layout;
};

} // namespace Android
} // namespace CCL

#endif // _ccl_androidtextlayout_h
