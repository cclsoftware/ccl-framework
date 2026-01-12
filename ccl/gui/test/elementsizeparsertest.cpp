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
// Filename    : elementsizeparsertest.cpp
// Description : Element Size Parser Unit Tests
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/gui/skin/skinmodel.h"
#include "ccl/gui/skin/skinattributes.h"

#include "ccl/public/gui/framework/skinxmldefs.h"

using namespace CCL;

//************************************************************************************************
// ElementSizeParserTest
//************************************************************************************************

class ElementSizeParserTest: public Test,
							 public SkinElements::ElementSizeParser
{
protected:
	MutableSkinAttributes attributes;
};
 
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (ElementSizeParserTest, UnitShouldBeAutoForEmptyAttributes)
{
	trySizeAttributes (attributes);
	
	CCL_TEST_ASSERT_EQUAL (designSize.width.unit, DesignCoord::kAuto);
	CCL_TEST_ASSERT_EQUAL (designSize.height.unit, DesignCoord::kAuto);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (ElementSizeParserTest, UnitShouldBeCoordForCoordinateSize)
{
	attributes.setString (ATTR_SIZE, "0,0,80,80");
	trySizeAttributes (attributes);
	
	CCL_TEST_ASSERT_EQUAL (designSize.width.unit, DesignCoord::kCoord);
	CCL_TEST_ASSERT_EQUAL (designSize.height.unit, DesignCoord::kCoord);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (ElementSizeParserTest, UnitShouldBeCoordForCoordinateRect)
{
	attributes.setString (ATTR_RECT, "0,0,80,80");
	trySizeAttributes (attributes);
	
	CCL_TEST_ASSERT_EQUAL (designSize.width.unit, DesignCoord::kCoord);
	CCL_TEST_ASSERT_EQUAL (designSize.height.unit, DesignCoord::kCoord);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (ElementSizeParserTest, WidthUnitShouldBeCoordForCoordinateWidth)
{
	attributes.setString (ATTR_WIDTH, "80");
	trySizeAttributes (attributes);
	
	CCL_TEST_ASSERT_EQUAL (designSize.width.unit, DesignCoord::kCoord);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (ElementSizeParserTest, HeightUnitShouldBeCoordForCoordinateHeight)
{
	attributes.setString (ATTR_HEIGHT, "80");
	trySizeAttributes (attributes);
	
	CCL_TEST_ASSERT_EQUAL (designSize.height.unit, DesignCoord::kCoord);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (ElementSizeParserTest, UnitShouldBePercentForRelativeSize)
{
	attributes.setString (ATTR_SIZE, "0%,0%,80%,80%");
	trySizeAttributes (attributes);
	
	CCL_TEST_ASSERT_EQUAL (designSize.width.unit, DesignCoord::kPercent);
	CCL_TEST_ASSERT_EQUAL (designSize.height.unit, DesignCoord::kPercent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (ElementSizeParserTest, UnitShouldRepresentMixedSize)
{
	attributes.setString (ATTR_SIZE, "0,0,40,80%");
	trySizeAttributes (attributes);
	
	CCL_TEST_ASSERT_EQUAL (designSize.width.unit, DesignCoord::kCoord);
	CCL_TEST_ASSERT_EQUAL (designSize.height.unit, DesignCoord::kPercent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (ElementSizeParserTest, UnitShouldRepresentMixedRect)
{
	attributes.setString (ATTR_RECT, "0%,0,80%,40");
	trySizeAttributes (attributes);
	
	CCL_TEST_ASSERT_EQUAL (designSize.width.unit, DesignCoord::kPercent);
	CCL_TEST_ASSERT_EQUAL (designSize.height.unit, DesignCoord::kCoord);
	CCL_TEST_ASSERT_EQUAL (designSize.height.value, 40);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (ElementSizeParserTest, PlainSizeShouldBeParsedFromMixedRect)
{
	attributes.setString (ATTR_SIZE, "0%,0,80%,40");
	trySizeAttributes (attributes);
	
	CCL_TEST_ASSERT_EQUAL (size.getHeight (), 40);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (ElementSizeParserTest, WidthUnitShouldBePercentForRelativeWidth)
{
	attributes.setString (ATTR_WIDTH, "80%");
	trySizeAttributes (attributes);
	
	CCL_TEST_ASSERT_EQUAL (designSize.width.unit, DesignCoord::kPercent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (ElementSizeParserTest, HeightUnitShouldBePercentForRelativeHeight)
{
	attributes.setString (ATTR_HEIGHT, "80%");
	trySizeAttributes (attributes);
	
	CCL_TEST_ASSERT_EQUAL (designSize.height.unit, DesignCoord::kPercent);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (ElementSizeParserTest, PartialSizeShouldBeParsedCorrectly)
{
	attributes.setString (ATTR_SIZE, "0,20");
	trySizeAttributes (attributes);

	CCL_TEST_ASSERT_EQUAL (designSize.top.value, 20);
	CCL_TEST_ASSERT_EQUAL (designSize.height.value, 0);

	CCL_TEST_ASSERT_EQUAL (size.top, 20);
	CCL_TEST_ASSERT_EQUAL (size.getHeight (), 0);
}
