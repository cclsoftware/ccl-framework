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
// Filename    : ccl/gui/help/keyglyphpainter.cpp
// Description : Key Glyph Painter
//
//************************************************************************************************

#include "ccl/gui/help/keyglyphpainter.h"

#include "ccl/gui/graphics/shapes/shapeimage.h"
#include "ccl/gui/graphics/shapes/shapebuilder.h"
#include "ccl/gui/graphics/imaging/bitmappainter.h"
#include "ccl/gui/theme/theme.h"

using namespace CCL;

//************************************************************************************************
// KeyGlyphPainter
//************************************************************************************************

KeyGlyphPainter::KeyGlyphPainter (FontRef font, BrushRef brush, int padding, int flags)
: font (font),
  brush (brush),
  padding (padding),
  flags (flags)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

KeyGlyphPainter::Glyph KeyGlyphPainter::createGlyph (VirtualKey vKey) const
{
	Glyph g;

	MutableCString imageName ("VKey:");
	imageName += VKey::getKeyName (vKey);
	Theme& theme = FrameworkTheme::instance ();
	g.image = theme.getImage (imageName);

	if(g.image == nullptr)
		g.text = VKey::getLocalizedKeyName (vKey);
	return g;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

KeyGlyphPainter::Glyph KeyGlyphPainter::createGestureGlyph (uint32 gesture) const
{
	Glyph g;

	MutableCString imageName ("Gesture:");
	switch(gesture)
	{
	case KeyState::kDrag : imageName += "Drag"; break;
	case KeyState::kDoubleClick : imageName += "DoubleClick"; break;
	case KeyState::kWheel : imageName += "Wheel"; break;
	}

	Theme& theme = FrameworkTheme::instance ();
	g.image = theme.getImage (imageName);

	if(g.image == nullptr)
		switch(gesture)
		{
		case KeyState::kDrag : g.text = CCLSTR ("drag"); break;
		case KeyState::kDoubleClick : g.text = CCLSTR ("2x"); break;
		case KeyState::kWheel : g.text = CCLSTR ("wheel"); break;
		}

	g.flags = Glyph::kStandalone;
	return g;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect KeyGlyphPainter::calcSize (const Glyph glyph) const
{
	Rect r;
	if(glyph.image)
		r (0, 0, glyph.image->getWidth (), glyph.image->getHeight ());
	else
		Font::measureString (r, glyph.text, font);

	if(padding != 0)
	{
		r.right += 2 * padding;
		r.bottom += 2 * padding;
	}
	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect KeyGlyphPainter::calcSize (const Glyph glyphs[], int count, Coord spacing) const
{
	Rect bounds;
	for(int i = 0; i < count; i++)
	{
		Rect r = calcSize (glyphs[i]);
		
		if(i > 0)
			bounds.right += spacing;
		bounds.right += r.getWidth ();
		bounds.bottom = ccl_max (bounds.bottom, r.getHeight ());
	}
	return bounds;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect KeyGlyphPainter::measureKeyString (const KeyEvent& key) const
{
	String keyString;
	key.toString (keyString, true);
	Rect size;
	Font::measureString (size, keyString, font);
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void KeyGlyphPainter::drawKeyString (IGraphics& graphics, RectRef rect, const KeyEvent& key, AlignmentRef alignment)
{
	String keyString;
	key.toString (keyString, true);
	#if (0 && DEBUG)
	SolidBrush brush (this->brush);
	brush.setColor (Colors::kBlue);
	#endif
	graphics.drawString (rect, keyString, font, brush, alignment);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void KeyGlyphPainter::makeGlyphs (Vector<Glyph>& glyphs, const KeyEvent& key) const
{
	// Modifiers
	#if (CCL_PLATFORM_MAC || CCL_PLATFORM_IOS)
	if(key.state.isSet (KeyState::kControl))
		glyphs.add (createGlyph (VKey::kControl));
	if(key.state.isSet (KeyState::kOption))
		glyphs.add (createGlyph (VKey::kOption));
	if(key.state.isSet (KeyState::kShift))
		glyphs.add (createGlyph (VKey::kShift));
	if(key.state.isSet (KeyState::kCommand))
		glyphs.add (createGlyph (VKey::kCommand));
	#else
	if(key.state.isSet (KeyState::kCommand))
		glyphs.add (createGlyph (VKey::kCommand));
	if(key.state.isSet (KeyState::kShift))
		glyphs.add (createGlyph (VKey::kShift));
	if(key.state.isSet (KeyState::kOption))
		glyphs.add (createGlyph (VKey::kOption));
	#endif

	// Virtual Key or Character
	if(key.vKey != VKey::kUnknown)
		glyphs.add (createGlyph (key.vKey));
	else if(key.character != 0)
	{
		Glyph g;
		uchar temp[2] = {key.character, 0};
		g.text.append (temp);
		glyphs.add (g);
	}

	// Gesture
	uint32 gesture = (key.state & KeyState::kGestureMask);
	if(gesture != 0)
		glyphs.add (createGestureGlyph (gesture));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect KeyGlyphPainter::measureKeyGlyphs (const KeyEvent& key) const
{
	Vector<Glyph> glyphs;
	makeGlyphs (glyphs, key);

	const Coord spacing = 2;
	
	return calcSize (glyphs, glyphs.count (), spacing);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void KeyGlyphPainter::drawKeyGlyphs (IGraphics& graphics, RectRef rect, const KeyEvent& key, AlignmentRef alignment)
{
	Vector<Glyph> glyphs;
	makeGlyphs (glyphs, key);

	const Coord spacing = 2;

	// alignment (TODO: right aligned)
	Rect bounds = calcSize (glyphs, glyphs.count (), spacing);
	bounds.offset (rect.getLeftTop ());

	if(alignment.getAlignH () == Alignment::kHCenter)
		bounds.centerH (rect);
	if(alignment.getAlignV () == Alignment::kVCenter)
		bounds.centerV (rect);

	// draw glyphs
	Rect cellRect;
	cellRect.offset (bounds.getLeftTop ());
	cellRect.setHeight (bounds.getHeight ());

	VectorForEach (glyphs, Glyph, g)
		Rect src = calcSize (g);
		
		cellRect.setWidth (src.getWidth ());
		Rect dst (src);
		dst.center (cellRect);

		Rect src2 (src);
		Rect dst2 (dst);
		if(padding != 0)
		{
			src2.right -= 2 * padding;
			src2.bottom -= 2 * padding;
			dst2.contract (padding);
		}

		if(g.image)
			BitmapPainter ().drawColorized (graphics, g.image, src2, dst2, brush.getColor ());
		else
			graphics.drawString (dst2, g.text, font, brush, Alignment::kLeftCenter);

		if(isOutline () && !g.isStandalone ())
			graphics.drawRect (dst, Pen (brush.getColor ()));

		cellRect.left = cellRect.right + spacing;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* KeyGlyphPainter::createShape (const KeyEvent& key)
{
	ShapeImage* image = NEW ShapeImage;
	ShapeBuilder g (image);
	drawKeyGlyphs (g, Rect (0, 0, kMaxCoord, kMaxCoord), key, Alignment::kLeftTop);
	return image;
}
