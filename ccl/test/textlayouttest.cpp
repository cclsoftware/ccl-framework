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
// Filename    : textlayouttest.cpp
// Description : GUI Unit Tests
//
//************************************************************************************************

#include "ccl/base/unittest.h"
#include "ccl/base/development.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/textfile.h"

#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/gui/graphics/itextlayout.h"
#include "ccl/public/gui/graphics/updatergn.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// TextBoundsRegion
/** Keeps the rectangles as they were added. */
//************************************************************************************************

class TextBoundsRegion: public Unknown,
						public IMutableRegion
{
public:
	const Vector<Rect>& getRects () const { return rects; }

	// IMutableRegion
	void CCL_API addRect (RectRef rect) override { rects.add (rect); }
	void CCL_API setEmpty () override { rects.removeAll (); }

	tbool CCL_API rectVisible (RectRef rect) const override
	{
		for(RectRef r : rects)
		{
			if(r.intersect (rect))
				return true;
		}

		return false;
	}

	Rect CCL_API getBoundingBox () const override
	{
		Rect bounds;
		bounds.setReallyEmpty ();
		for(RectRef r : rects)
			bounds.join (r);

		return bounds;
	}

	CLASS_INTERFACE (IMutableRegion, Unknown)

protected:
	Vector<Rect> rects;
};

//************************************************************************************************
// TextLayoutTest
//************************************************************************************************

CCL_TEST (TextLayoutTest, TestLineRange)
{
	AutoPtr<ITextLayout> textLayout = GraphicsFactory::createTextLayout ();
	textLayout->construct ("a\nbc\r\n", 0, 0, Font (), ITextLayout::kMultiLine, TextFormat ());
	
	ITextLayout::Range range (0, 0);
	tresult result = textLayout->getLineRange (range, 1);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (range.start, 0);
	CCL_TEST_ASSERT_EQUAL (range.length, 2);

	result = textLayout->getLineRange (range, 1, false);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (range.start, 0);
	CCL_TEST_ASSERT_EQUAL (range.length, 1);
	
	result = textLayout->getLineRange (range, 2);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (range.start, 2);
	CCL_TEST_ASSERT_EQUAL (range.length, 4);

	result = textLayout->getLineRange (range, 2, false);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (range.start, 2);
	CCL_TEST_ASSERT_EQUAL (range.length, 2);
	
	result = textLayout->getLineRange (range, 6);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (range.start, 6);
	CCL_TEST_ASSERT_EQUAL (range.length, 0);
	CCL_WARN ("range.length: %d\n", range.length)
	
	result = textLayout->getLineRange (range, 6, false);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (range.start, 6);
	CCL_TEST_ASSERT_EQUAL (range.length, 0);
	CCL_WARN ("range.length: %d\n", range.length)
	
	result = textLayout->getLineRange (range, 7);
	CCL_TEST_ASSERT_NOT_EQUAL (result, kResultOk);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (TextLayoutTest, TestLineRangeWithoutNewline)
{
	AutoPtr<ITextLayout> textLayout = GraphicsFactory::createTextLayout ();
	textLayout->construct ("abc", 0, 0, Font (), ITextLayout::kMultiLine, TextFormat ());
	
	ITextLayout::Range range (0, 0);
	tresult result = textLayout->getLineRange (range, 0);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (range.start, 0);
	CCL_TEST_ASSERT_EQUAL (range.length, 3);
	
	result = textLayout->getLineRange (range, 3);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (range.start, 0);
	CCL_TEST_ASSERT_EQUAL (range.length, 3);
	
	result = textLayout->getLineRange (range, 4);
	CCL_TEST_ASSERT_NOT_EQUAL (result, kResultOk);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (TextLayoutTest, TestLineRangeMultipleNewlines)
{
	AutoPtr<ITextLayout> textLayout = GraphicsFactory::createTextLayout ();
	textLayout->construct ("\n\r\n\n\n\r\n", 0, 0, Font (), ITextLayout::kMultiLine, TextFormat ());
	
	ITextLayout::Range range (0, 0);
	tresult result = textLayout->getLineRange (range, 2);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (range.start, 1);
	CCL_TEST_ASSERT_EQUAL (range.length, 2);
	
	result = textLayout->getLineRange (range, 3);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (range.start, 3);
	CCL_TEST_ASSERT_EQUAL (range.length, 1);
	
	result = textLayout->getLineRange (range, 7);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (range.start, 7);
	CCL_TEST_ASSERT_EQUAL (range.length, 0);
	
	result = textLayout->getLineRange (range, 8);
	CCL_TEST_ASSERT_NOT_EQUAL (result, kResultOk);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (TextLayoutTest, TestLineRangeInvalidArgument)
{
	AutoPtr<ITextLayout> textLayout = GraphicsFactory::createTextLayout ();
	textLayout->construct ("a\nbc\r\n", 0, 0, Font (), ITextLayout::kMultiLine, TextFormat ());

	// there is no line before the text or behind its end, index 6 is the last valid one
	ITextLayout::Range range (0, 0);
	tresult result = textLayout->getLineRange (range, -1);
	CCL_TEST_ASSERT_EQUAL (result, kResultInvalidArgument);

	result = textLayout->getLineRange (range, -1, false);
	CCL_TEST_ASSERT_EQUAL (result, kResultInvalidArgument);

	result = textLayout->getLineRange (range, 7);
	CCL_TEST_ASSERT_EQUAL (result, kResultInvalidArgument);

	result = textLayout->getLineRange (range, 7, false);
	CCL_TEST_ASSERT_EQUAL (result, kResultInvalidArgument);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (TextLayoutTest, TestHitTestEndOfLine)
{
	AutoPtr<ITextLayout> textLayout = GraphicsFactory::createTextLayout ();
	textLayout->construct ("abc\nd\ne", 10, 200, Font (), ITextLayout::kMultiLine, TextFormat (Alignment::kLeftTop));
	
	int resultIndex = -1;
	PointF position (20, 1);
	tresult result = textLayout->hitTest (resultIndex, position);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (resultIndex, 3); // expected index is index after rightmost visible character: 'c' (position after \n is already on next line)
	CCL_TEST_ASSERT (position.x > 0); // expected position is a point on the right edge of the glyph.
	
	position = PointF (20, 201);
	result = textLayout->hitTest (resultIndex, position);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (resultIndex, 7);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (TextLayoutTest, TestHitTestWordBreak)
{
	AutoPtr<ITextLayout> textLayout = GraphicsFactory::createTextLayout ();
	textLayout->construct ("abcdefg hijklnmnop qrstuvw xyz", 100, 200, Font (), ITextLayout::kMultiLine, TextFormat (Alignment::kLeftTop, TextFormat::kWordBreak));
	
	int resultIndex = -1;
	PointF position (200, -1);
	tresult result = textLayout->hitTest (resultIndex, position);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (resultIndex, 0); // position is above the first line, so expected index is 0

	position = PointF (200, 1);
	result = textLayout->hitTest (resultIndex, position);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (resultIndex, 7); // expected index is index after rightmost visible character: 'g' (position after space is already on next line)
	CCL_TEST_ASSERT (position.x > 0);
	
	position = PointF (200, 201);
	result = textLayout->hitTest (resultIndex, position);
	CCL_TEST_ASSERT_EQUAL (result, kResultOk);
	CCL_TEST_ASSERT_EQUAL (resultIndex, 30);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (TextLayoutTest, TestTextBoundsOneRectPerLine)
{
	// at fractional font sizes, adjacent lines can touch or overlap by a fraction of a pixel
	String text ("abc\ndefg\nhij");
	for(float fontSize = 8.f; fontSize <= 32.f; fontSize += .25f)
	{
		Font font;
		font.setSize (fontSize);

		AutoPtr<ITextLayout> textLayout = GraphicsFactory::createTextLayout ();
		textLayout->construct (text, 200, 200, font, ITextLayout::kMultiLine, TextFormat (Alignment::kLeftTop));

		TextBoundsRegion bounds;
		tresult result = textLayout->getTextBounds (bounds, {0, text.length ()});
		CCL_TEST_ASSERT_EQUAL (result, kResultOk);

		const Vector<Rect>& lineRects = bounds.getRects ();
		CCL_TEST_ASSERT_EQUAL (lineRects.count (), 3);
		for(int i = 1; i < lineRects.count (); i++)
			CCL_TEST_ASSERT (lineRects[i].top >= lineRects[i - 1].bottom);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (TextLayoutTest, TestCaretAtEndOfRightAlignedText)
{
	// the image of an italic 'f' extends beyond its advance, the caret is placed at the end of the advance
	// (the text can be offset by a fraction of a point, because it is aligned in whole points)
	static const Coord kLayoutWidth = 200;
	String text ("abc\nwxyf");
	for(float fontSize = 8.f; fontSize <= 32.f; fontSize += .25f)
	{
		Font font;
		font.setSize (fontSize);
		font.isItalic (true);

		AutoPtr<ITextLayout> textLayout = GraphicsFactory::createTextLayout ();
		textLayout->construct (text, kLayoutWidth, 200, font, ITextLayout::kMultiLine, TextFormat (Alignment::kRightTop));

		RectF caretBounds;
		tresult result = textLayout->getCharacterBounds (caretBounds, text.length ());
		CCL_TEST_ASSERT_EQUAL (result, kResultOk);
		CCL_TEST_ASSERT_NEAR (kLayoutWidth, caretBounds.left, 1.f);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (TextLayoutTest, TestProcessLicenseTextDuration)
{
	AutoPtr<ITextLayout> textLayout = GraphicsFactory::createTextLayout ();
	Url legalFolder;
	GET_DEVELOPMENT_FOLDER_LOCATION (legalFolder, CCL_FRAMEWORK_DIRECTORY "build", "identities/ccl/legal");

	String searchPattern ("*3rd*.txt");
	String text;
	ForEachFile (File::findFiles (legalFolder, searchPattern), path)
		String licenseText = TextUtils::loadString (*path);
		if(!licenseText.isEmpty ())
		{
			if(!text.isEmpty ())
			{
				String mergedText (text);
				mergedText << "\n\n";
				mergedText << licenseText;
				text = mergedText;
			}
			else
				text = licenseText;
		}
	EndFor

	textLayout->construct (text, 0, 0, Font (), ITextLayout::kMultiLine, TextFormat ());
	ITextLayout::Range range (0, 0);

	double startCount = System::GetProfileTime ();
	textLayout->getWordRange (range, 0);
	double delta = 1000 * (System::GetProfileTime () - startCount); // in ms
	CCL_TEST_ASSERT (delta < 200);
}
