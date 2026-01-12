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
// Filename    : ccl/platform/cocoa/quartz/quartztextlayout.h
// Description : Core Text based Text Layout
//
//************************************************************************************************

#ifndef _quartztextlayout_h
#define _quartztextlayout_h

#include "ccl/base/collections/objectarray.h"

#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/public/base/ccldefpush.h"
#include <CoreGraphics/CGContext.h>
#include <CoreText/CTFramesetter.h>
#include "ccl/public/base/ccldefpop.h"

namespace CCL {
namespace MacOS {

//************************************************************************************************
// QuartzTextLayout
//************************************************************************************************

class QuartzTextLayout: public NativeTextLayout
{
public:
	DECLARE_CLASS_ABSTRACT (QuartzTextLayout, NativeTextLayout)
	
	QuartzTextLayout ();
	~QuartzTextLayout ();
		
	void* getAttributedString () {return string;}
	void draw (CGContextRef context, PointF position, Color textColor);

	// ITextLayout
	tresult CCL_API construct (StringRef text, Coord width, Coord height, FontRef font, LineMode lineMode, TextFormatRef format) override;
	tresult CCL_API construct (StringRef text, CoordF width, CoordF height, FontRef font, LineMode lineMode, TextFormatRef format) override;
	tresult CCL_API resize (Coord width, Coord height) override;
	tresult CCL_API resize (CoordF width, CoordF height) override;
	tresult CCL_API setFontStyle (const Range& range, int style, tbool state) override;
	tresult CCL_API setFontSize (const Range& range, float size) override;
	tresult CCL_API setSpacing (const Range& range, float spacing) override;
	tresult CCL_API setLineSpacing (const Range& range, float lineSpacing) override;
	tresult CCL_API setTextColor (const Range& range, Color color) override;
	tresult CCL_API setBaselineOffset (const Range& range, float offset) override;
	tresult CCL_API setSuperscript (const Range& range) override;
	tresult CCL_API setSubscript (const Range& range) override;
	tresult CCL_API getBounds (Rect& bounds, int flags = 0) const override;
	tresult CCL_API getBounds (RectF& bounds, int flags = 0) const override;
	tresult CCL_API getImageBounds (RectF& bounds) const override;
	tresult CCL_API getBaselineOffset (PointF& offset) const override;
	tresult CCL_API hitTest (int& textIndex, PointF& position) const override;
	tresult CCL_API getCharacterBounds (RectF& offset, int textIndex) const override;
	tresult CCL_API getTextBounds (IMutableRegion& bounds, const Range& range) const override;
	tresult CCL_API getLineRange (Range& range, int textIndex) const override;
	StringRef CCL_API getText () const override;
	
protected:
	String text;
	void* string;
	CGSize boundingRect;
	mutable CGFloat totalHeight;
	Alignment alignment;
	bool wordBreak;
	LineMode lineMode;
	float ascent;
	float descent;
	float leading;
	mutable CTFramesetterRef framesetter;
	mutable CTLineRef line;
	Font font;
	ObjectArray colorRegions;
	
	CGSize getActualSize (bool applyMargin) const;
	CGPoint getTextPosition (const PointF& position) const;
	CTFrameRef createFrame (const PointF& position) const;
	tresult setSuperscript (const Range& range, float sizeFactor, float baselineFactor);

	void resetFrameSetter ();
	void resetLine ();
	void initTotalHeight () const;
};

} // namespace MacOS
} // namespace CCL

#endif // _quartztextlayout_h
