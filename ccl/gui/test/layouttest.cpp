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
// Filename    : layouttest.cpp
// Description : Layout Unit Tests
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/gui/layout/layoutview.h"
#include "ccl/public/gui/framework/skinxmldefs.h"

using namespace CCL;

//************************************************************************************************
// LayoutViewTest
//************************************************************************************************

class LayoutViewTest: public Test
{
public:
	// Test
	void setUp () override
	{
		layoutView = NEW LayoutView (Rect ());
	}

protected:
	AutoPtr<LayoutView> layoutView;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (LayoutViewTest, AttributesAreEmptyWithoutLayout)
{
	Attributes attributes;
	bool success = layoutView->getLayoutAttributes (attributes);
	
	CCL_TEST_ASSERT_FALSE (success);
	CCL_TEST_ASSERT_EQUAL (attributes.countAttributes (), 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (LayoutViewTest, AttributesExistWithLayout)
{
	AutoPtr<Layout> layout = LayoutFactory::instance ().createLayout (LAYOUTCLASS_FLEXBOX);
	layoutView->setLayout (layout);
	
	Attributes attributes;
	bool success = layoutView->getLayoutAttributes (attributes);
	
	CCL_TEST_ASSERT (success);
	CCL_TEST_ASSERT (attributes.countAttributes () > 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (LayoutViewTest, ChildAttributesAreEmptyWithoutLayoutItems)
{
	View view;
	Attributes attributes;
	bool success = layoutView->getChildLayoutAttributes (attributes, &view);
	
	CCL_TEST_ASSERT_FALSE (success);
	CCL_TEST_ASSERT_EQUAL (attributes.countAttributes (), 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (LayoutViewTest, ChildAttributesAreEmptyWithoutLayout)
{
	View* view = NEW View;
	layoutView->addView (view);
	
	Attributes attributes;
	bool success = layoutView->getChildLayoutAttributes (attributes, view);
	
	CCL_TEST_ASSERT_FALSE (success);
	CCL_TEST_ASSERT_EQUAL (attributes.countAttributes (), 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (LayoutViewTest, ChildAttributesExistForAddedViewsWithLayout)
{
	AutoPtr<Layout> layout = LayoutFactory::instance ().createLayout (LAYOUTCLASS_FLEXBOX);
	layoutView->setLayout (layout);
	
	View* view = NEW View;
	layoutView->addView (view);
	
	Attributes attributes;
	bool success = layoutView->getChildLayoutAttributes (attributes, view);
	
	CCL_TEST_ASSERT (success);
	CCL_TEST_ASSERT (attributes.countAttributes () > 0);
}
