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
// Filename    : ccl/gui/help/keyglyphpainter.h
// Description : Key Glyph Painter
//
//************************************************************************************************

#ifndef _ccl_keyglyphpainter_h
#define _ccl_keyglyphpainter_h

#include "ccl/gui/keyevent.h"

#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/collections/vector.h"

namespace CCL {

class Image;

//************************************************************************************************
// KeyGlyphPainter
//************************************************************************************************

class KeyGlyphPainter
{
public:
	KeyGlyphPainter (FontRef font, BrushRef brush, int padding = 0, int flags = 0);

	PROPERTY_OBJECT (Font, font, Font)
	PROPERTY_OBJECT (SolidBrush, brush, Brush)

	enum Flags
	{
		kOutline = 1<<0
	};

	PROPERTY_VARIABLE (int, flags, Flags)
	PROPERTY_VARIABLE (int, padding, Padding)
	PROPERTY_FLAG (flags, kOutline, isOutline)

	Rect measureKeyString (const KeyEvent& key) const;
	void drawKeyString (IGraphics& graphics, RectRef rect, const KeyEvent& key, AlignmentRef alignment);

	Rect measureKeyGlyphs (const KeyEvent& key) const;
	void drawKeyGlyphs (IGraphics& graphics, RectRef rect, const KeyEvent& key, AlignmentRef alignment);

	Image* createShape (const KeyEvent& key);

protected:
	struct Glyph
	{
		SharedPtr<IImage> image;
		String text;
		int flags;

		Glyph ()
		: flags (0)
		{}

		enum Flags { kStandalone = 1<<0 };
		PROPERTY_FLAG (flags, kStandalone, isStandalone)
	};

	Glyph createGlyph (VirtualKey vKey) const;
	Glyph createGestureGlyph (uint32 gesture) const;
	void makeGlyphs (Vector<Glyph>& glyphs, const KeyEvent& key) const;

	Rect calcSize (const Glyph glyph) const;
	Rect calcSize (const Glyph glyphs[], int count, Coord spacing) const;
};

} // namespace CCL

#endif // _ccl_keyglyphpainter_h
