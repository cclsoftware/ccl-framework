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
// Filename    : skiatest.cpp
// Description : Skia Unit Tests
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/platform/shared/skia/skiatextlayout.h"

#include "ccl/public/text/textencoding.h"

using namespace CCL;

//************************************************************************************************
// LayoutViewTest
//************************************************************************************************

class SkiaTextLayoutTest: public Test
{
public:
	void setUp () override
	{
		textLayout = NEW SkiaTextLayout ();
	}

protected:
	AutoPtr<SkiaTextLayout> textLayout;
	Font font;
	TextFormat format;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (SkiaTextLayoutTest, TestUpdateUtf8PositionsSimple)
{
	String text = "first text with different word length and modifications.";
	textLayout->construct (text, 0, 0, font, ITextLayout::kMultiLine, format);

	ITextLayout::Range range (0, 0);
	textLayout->getWordRange (range, 0);
	CCL_TEST_ASSERT_EQUAL (range.length, 5);

	textLayout->getWordRange (range, 17);
	CCL_TEST_ASSERT_EQUAL (range.length, 9);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (SkiaTextLayoutTest, TestUpdateUtf8PositionsSurrogatePairs)
{
	String text (Text::kUTF8, "\U0001F4AB is fantastic \U0001F635");
	textLayout->construct (text, 0, 0, font, ITextLayout::kMultiLine, format);

	ITextLayout::Range range (0, 0);
	textLayout->getWordRange (range, 0);
	CCL_TEST_ASSERT_EQUAL (range.length, 2);

	Vector<int> expectedPositions {0, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 22, 22};
	const Vector<int>& actualPositions = textLayout->getUtf8Positions ();
	CCL_TEST_ASSERT_EQUAL (expectedPositions.count (), actualPositions.count ());

	for(int i = 0; i < expectedPositions.count (); i++)
		CCL_TEST_ASSERT_EQUAL (expectedPositions.at (i), actualPositions.at (i));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (SkiaTextLayoutTest, TestUpdateUtf8PositionsTabs)
{
	String text (Text::kUTF8, "a\tab\tabc");
	textLayout->construct (text, 0, 0, font, ITextLayout::kMultiLine, format);

	ITextLayout::Range range (0, 0);
	textLayout->getWordRange (range, 0);
	CCL_TEST_ASSERT_EQUAL (range.length, 1);

	textLayout->getWordRange (range, 2);
	CCL_TEST_ASSERT_EQUAL (range.length, 2);

	textLayout->getWordRange (range, 5);
	CCL_TEST_ASSERT_EQUAL (range.length, 3);

	Vector<int> expectedPositions {0, 1, 5, 6, 7, 11, 12, 13, 14};
	const Vector<int>& actualPositions = textLayout->getUtf8Positions ();
	CCL_TEST_ASSERT_EQUAL (expectedPositions.count (), actualPositions.count ());
	for(int i = 0; i < expectedPositions.count (); i++)
		CCL_TEST_ASSERT_EQUAL (expectedPositions.at (i), actualPositions.at (i));
}
