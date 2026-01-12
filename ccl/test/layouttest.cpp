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
// Description : GUI Unit Tests
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/gui/framework/skinxmldefs.h"
#include "ccl/public/gui/framework/viewbox.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef int SizeMode;

//************************************************************************************************
// LayoutTestBase
//************************************************************************************************

class LayoutTestBase: public ParameterizedTest<SizeMode>
{
public:
	ViewBox addView (RectRef size, SizeLimit sizeLimit = {})
	{
		ViewBox view (ClassID::View, size);

		SizeMode sizeMode = getTestValue ();
		view.setSizeMode (sizeMode);

		if(sizeLimit.isValid ())
			view.setSizeLimits (sizeLimit);

		layoutView->getChildren ().add (view);
		return view;
	}

	// Test
	void setUp () override
	{
		layoutView = ViewBox (ClassID::AnchorLayoutView, bounds, Styles::kHorizontal);
		layoutViewObject = layoutView;

		layoutView.setAttribute (ATTR_SPACING, 0);
		layoutView.setAttribute (ATTR_MARGIN, 0);
	}

protected:
	ViewBox layoutView;
	Rect bounds;

	LayoutTestBase ()
	: bounds (0, 0, 100, 100)
	{}

private:
	AutoPtr<IView> layoutViewObject;
};

//************************************************************************************************
// BoxLayoutRigidTest
//************************************************************************************************

class BoxLayoutRigidTest: public LayoutTestBase
{
public:
	BoxLayoutRigidTest ()
	{
		addTestValue (0); // RigidBoxLayout is used if sizeMode == 0
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, TheInitialSizeIsEqualToTheProvidedBounds)
{
	CCL_TEST_ASSERT_EQUAL (layoutView->getSize (), bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, AddingAViewDoesntChangeItsParent)
{
	addView ({0, 0, 20, 20});
	CCL_TEST_ASSERT_EQUAL (layoutView->getSize (), bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, AddingAViewDoesntChangeItsParentEvenIfTooBig)
{
	addView ({0, 0, 200, 20});
	CCL_TEST_ASSERT_EQUAL (layoutView->getSize (), bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, AddedViewsUseMargin)
{
	layoutView.setAttribute (ATTR_MARGIN, 10);
	ViewBox view = addView ({0, 0, 20, 20});

	CCL_TEST_ASSERT_EQUAL (view.getPosition ().x, 10);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, AddedViewsKeepTheirWidthAndHeight)
{
	ViewBox view = addView ({0, 0, 20, 20});

	CCL_TEST_ASSERT_EQUAL (view.getWidth (), 20) << String ().appendFormat ("SizeMode is: %(1)", getTestValue ());
	CCL_TEST_ASSERT_EQUAL (view.getHeight (), 20);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, InitialViewPositionsAreDisregarded)
{
	ViewBox view = addView ({20, 0, 40, 20});
	CCL_TEST_ASSERT_EQUAL (view.getPosition ().x, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, AddedViewsKeepTheirWidthAndHeightEvenIfTooBig)
{
	ViewBox view = addView ({0, 0, 200, 20});

	CCL_TEST_ASSERT_EQUAL (view.getWidth (), 200);
	CCL_TEST_ASSERT_EQUAL (view.getHeight (), 20);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, SizeLimitsReflectChildren)
{
	addView ({0, 0, 20, 20});
	addView ({0, 0, 50, 20});
	addView ({0, 0, 30, 20});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	CCL_TEST_ASSERT_EQUAL (sizeLimit.maxWidth, 100);
	CCL_TEST_ASSERT_EQUAL (sizeLimit.minWidth, 100);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, SizeLimitsReflectMargins)
{
	layoutView.setAttribute (ATTR_MARGIN, 10);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 20});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	CCL_TEST_ASSERT_EQUAL (sizeLimit.maxWidth, 60);
	CCL_TEST_ASSERT_EQUAL (sizeLimit.minWidth, 60);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, SizeLimitsReflectSpacing)
{
	layoutView.setAttribute (ATTR_SPACING, 10);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 20});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	CCL_TEST_ASSERT_EQUAL (sizeLimit.maxWidth, 50);
	CCL_TEST_ASSERT_EQUAL (sizeLimit.minWidth, 50);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, CrossAxisSizeLimitsReflectChildren)
{
	addView ({0, 0, 20, 20});
	addView ({0, 20, 20, 40});
	addView ({0, 0, 20, 30});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	CCL_TEST_ASSERT_EQUAL (sizeLimit.maxHeight, 40);
	CCL_TEST_ASSERT_EQUAL (sizeLimit.minHeight, 40);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, CrossAxisSizeLimitsReflectMargins)
{
	layoutView.setAttribute (ATTR_MARGIN, 10);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 20});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	CCL_TEST_ASSERT_EQUAL (sizeLimit.maxHeight, 40);
	CCL_TEST_ASSERT_EQUAL (sizeLimit.minHeight, 40);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, CrossAxisSpacingHasNoEffect)
{
	layoutView.setAttribute (ATTR_SPACING, 10);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 20});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	CCL_TEST_ASSERT_EQUAL (sizeLimit.maxHeight, 20);
	CCL_TEST_ASSERT_EQUAL (sizeLimit.minHeight, 20);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, AutoLayoutUsesFixedSizeLimits)
{
	layoutView.setAttribute (ATTR_SPACING, 10);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 20});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();
	layoutView.autoSize ();

	CCL_TEST_ASSERT_EQUAL (layoutView.getWidth (), sizeLimit.minWidth);
	CCL_TEST_ASSERT_EQUAL (layoutView.getWidth (), sizeLimit.maxWidth);
	CCL_TEST_ASSERT_EQUAL (layoutView.getHeight (), sizeLimit.minHeight);
	CCL_TEST_ASSERT_EQUAL (layoutView.getHeight (), sizeLimit.maxHeight);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, LayoutHugsChildrenHorizontallyInHFitSizeMode)
{
	layoutView.setSizeMode (IView::kHFitSize);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 30});

	CCL_TEST_ASSERT_EQUAL (layoutView.getWidth (), 40);
	CCL_TEST_ASSERT_EQUAL (layoutView.getHeight (), bounds.getHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, LayoutHugsChildrenInFitSizeMode)
{
	layoutView.setSizeMode (IView::kFitSize);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 30});

	CCL_TEST_ASSERT_EQUAL (layoutView.getWidth (), 40);
	CCL_TEST_ASSERT_EQUAL (layoutView.getHeight (), 30);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, ReverseLayoutAddsItemsInReverseOrder)
{
	layoutView.setStyle (StyleFlags (Styles::kHorizontal, Styles::kLayoutReverse));

	ViewBox view0 = addView ({0, 0, 20, 20});
	ViewBox view1 = addView ({0, 0, 20, 20});
	ViewBox view2 = addView ({0, 0, 20, 20});

	CCL_TEST_ASSERT_EQUAL (view0.getPosition ().x, 80);
	CCL_TEST_ASSERT_EQUAL (view1.getPosition ().x, 60);
	CCL_TEST_ASSERT_EQUAL (view2.getPosition ().x, 40);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, ReverseAddedItemsAreConsideredOnAutoSize)
{
	layoutView.setStyle (StyleFlags (Styles::kHorizontal, Styles::kLayoutReverse));

	ViewBox view0 = addView ({0, 0, 20, 20});
	ViewBox view1 = addView ({0, 0, 20, 20});
	ViewBox view2 = addView ({0, 0, 20, 20});

	layoutView.autoSize ();

	CCL_TEST_ASSERT_EQUAL (view0.getPosition ().x, 40);
	CCL_TEST_ASSERT_EQUAL (view1.getPosition ().x, 20);
	CCL_TEST_ASSERT_EQUAL (view2.getPosition ().x, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutRigidTest, ReverseLayoutHugsChildrenInFitSizeMode)
{
	layoutView.setStyle (StyleFlags (Styles::kHorizontal, Styles::kLayoutReverse));
	layoutView.setAttribute (ATTR_MARGIN, 10);
	layoutView.setSizeMode (IView::kFitSize);

	ViewBox view0 = addView ({0, 0, 20, 20});
	ViewBox view1 = addView ({0, 0, 20, 20});
	ViewBox view2 = addView ({0, 0, 20, 20});

	CCL_TEST_ASSERT_EQUAL (view0.getPosition ().x, 50);
	CCL_TEST_ASSERT_EQUAL (view1.getPosition ().x, 30);
	CCL_TEST_ASSERT_EQUAL (view2.getPosition ().x, 10);

	CCL_TEST_ASSERT_EQUAL (layoutView.getWidth (), 80);
	CCL_TEST_ASSERT_EQUAL (layoutView.getHeight (), 40);
}

//************************************************************************************************
// BoxLayoutLockedTest
// Item sizes are locked if sizeMode != kAttachAll | (kAttachLeft|kAttachRight) (Horizontal)
//************************************************************************************************

class BoxLayoutLockedTest: public LayoutTestBase
{
public:
	BoxLayoutLockedTest ()
	{
		// Item sizes are non-responsive (aka locked) for the following size modes:
		addTestValue (IView::kAttachLeft);
		addTestValue (IView::kAttachTop);
		addTestValue (IView::kAttachRight);
		addTestValue (IView::kAttachBottom);
		addTestValue (IView::kHCenter);
		addTestValue (IView::kVCenter);
		addTestValue (IView::kHFitSize);
		addTestValue (IView::kVFitSize);
		addTestValue (IView::kFitSize);
		addTestValue (IView::kPreferCurrentSize);
		addTestValue (IView::kFill);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutLockedTest, PreferredSizeReflectsChildren)
{
	addView ({0, 0, 20, 20});
	addView ({0, 0, 50, 20});
	addView ({0, 0, 30, 20});

	layoutView.autoSize ();

	CCL_TEST_ASSERT_EQUAL (layoutView.getWidth (), 100);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutLockedTest, PreferredSizeReflectsMargins)
{
	layoutView.setAttribute (ATTR_MARGIN, 10);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 20});
	layoutView.autoSize ();

	CCL_TEST_ASSERT_EQUAL (layoutView.getWidth (), 60);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutLockedTest, PreferredSizeReflectsSpacing)
{
	layoutView.setAttribute (ATTR_SPACING, 10);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 20});
	layoutView.autoSize ();

	CCL_TEST_ASSERT_EQUAL (layoutView.getWidth (), 50);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutLockedTest, PreferredSizeReflectsChildrenOnCrossAxis)
{
	ViewBox view0 = addView ({0, 0, 20, 20});
	ViewBox view1 = addView ({0, 20, 20, 40});
	ViewBox view2 = addView ({0, 0, 20, 30});
	layoutView.autoSize ();

	if(getTestValue () == IView::kVCenter) // The view has an initial vertical offset, which changes due to the view beeing centred
		CCL_TEST_ASSERT_EQUAL (layoutView.getHeight (), 30);
	else
		CCL_TEST_ASSERT_EQUAL (layoutView.getHeight (), 40);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutLockedTest, PreferredSizeReflectsMarginOnCrossAxis)
{
	layoutView.setAttribute (ATTR_MARGIN, 10);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 20});
	layoutView.autoSize ();

	CCL_TEST_ASSERT_EQUAL (layoutView.getHeight (), 40);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutLockedTest, PreferredSizeCrossAxisSpacingHasNoEffect)
{
	layoutView.setAttribute (ATTR_SPACING, 10);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 20});

	layoutView.autoSize ();
	CCL_TEST_ASSERT_EQUAL (layoutView.getHeight (), 20);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutLockedTest, UnifySizesStyleHasNoEffectForLockedItems)
{
	layoutView.setStyle (StyleFlags (Styles::kHorizontal, Styles::kLayoutUnifySizes));

	ViewBox view0 = addView ({0, 0, 20, 20});
	ViewBox view1 = addView ({0, 0, 20, 30});
	ViewBox view2 = addView ({0, 0, 20, 40});

	CCL_TEST_ASSERT_NOT_EQUAL (view0.getHeight (), view1.getHeight ());
	CCL_TEST_ASSERT_NOT_EQUAL (view0.getHeight (), view2.getHeight ());
	CCL_TEST_ASSERT_NOT_EQUAL (view1.getHeight (), view2.getHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutLockedTest, SizeLimitForFitSizeModeReflectsMainAxisPreferredSize)
{
	layoutView.setAttribute (ATTR_MARGIN, 10);
	layoutView.setAttribute (ATTR_SPACING, 10);
	layoutView.setSizeMode (IView::kFitSize);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 30});
	addView ({0, 0, 20, 40});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	// preferred size is not directly accessible -> using bare value
	CCL_TEST_ASSERT_EQUAL (sizeLimit.maxWidth, 100);
	CCL_TEST_ASSERT_EQUAL (sizeLimit.minWidth, 100);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutLockedTest, FitSizeFollowsBiggestItem)
{
	layoutView.setSizeMode (IView::kVFitSize);

	layoutView.setAttribute (ATTR_MARGIN, 10);
	layoutView.setAttribute (ATTR_SPACING, 10);

	ViewBox view0 = addView ({0, 0, 20, 20}, {0, 10, kMaxCoord, 30});
	ViewBox view1 = addView ({0, 0, 20, 30}, {0, 10, kMaxCoord, 40});
	ViewBox view2 = addView ({0, 0, 20, 40}, {0, 30, kMaxCoord, 50});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	CCL_TEST_ASSERT_EQUAL (sizeLimit.minHeight, 60);
	CCL_TEST_ASSERT_EQUAL (sizeLimit.maxHeight, 60);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutLockedTest, OffsetIsNotConsideredForFitSize)
{
	layoutView.setSizeMode (IView::kVFitSize);

	layoutView.setAttribute (ATTR_MARGIN, 10);
	layoutView.setAttribute (ATTR_SPACING, 10);

	ViewBox view0 = addView ({0, 60, 20, 20}, {0, 10, kMaxCoord, 30});
	ViewBox view1 = addView ({0, 0, 20, 30}, {0, 10, kMaxCoord, 40});
	ViewBox view2 = addView ({0, 0, 20, 40}, {0, 30, kMaxCoord, 50});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	CCL_TEST_ASSERT_EQUAL (sizeLimit.minHeight, 60);
	CCL_TEST_ASSERT_EQUAL (sizeLimit.maxHeight, 60);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutLockedTest, CrossAxisLimitIsInfiniteByDefault)
{
	layoutView.setAttribute (ATTR_MARGIN, 10);
	layoutView.setAttribute (ATTR_SPACING, 10);

	ViewBox view0 = addView ({0, 0, 20, 20}, {0, 10, kMaxCoord, 30});
	ViewBox view1 = addView ({0, 0, 20, 30}, {0, 10, kMaxCoord, 40});
	ViewBox view2 = addView ({0, 0, 20, 40}, {0, 30, kMaxCoord, 50});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();
	CCL_TEST_ASSERT_EQUAL (sizeLimit.maxHeight, kMaxCoord);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutLockedTest, CrossAxisLowerLimitIncludesItemSizeAndMargins)
{
	layoutView.setAttribute (ATTR_MARGIN, 10);
	layoutView.setAttribute (ATTR_SPACING, 10);

	ViewBox view0 = addView ({0, 0, 20, 20}, {0, 10, kMaxCoord, 30});
	ViewBox view1 = addView ({0, 0, 20, 30}, {0, 10, kMaxCoord, 40});
	ViewBox view2 = addView ({0, 0, 20, 40}, {0, 30, kMaxCoord, 50});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	// If attached to end of the cross axis, the initial distance can't be underrun
	if(getTestValue () & IView::kAttachBottom)
		CCL_TEST_ASSERT_EQUAL (sizeLimit.minHeight, 100);
	else
		CCL_TEST_ASSERT_EQUAL (sizeLimit.minHeight, 60);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutLockedTest, IfLayoutWrapTheMinimumHeightShouldBeTheLayoutsPreferredMinimum)
{
	layoutView.setStyle (StyleFlags (Styles::kHorizontal, Styles::kLayoutWrap));

	ViewBox view0 = addView ({0, 0, 20, 20}, {0, 10, kMaxCoord, 30});
	ViewBox view1 = addView ({0, 0, 20, 30}, {0, 10, kMaxCoord, 40});
	ViewBox view2 = addView ({0, 0, 20, 40}, {0, 30, kMaxCoord, 50});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	CCL_TEST_ASSERT_EQUAL (sizeLimit.minHeight, 40);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxLayoutLockedTest, WrappingItemsShouldBeConsideredForMinHeight)
{
	layoutView.setStyle (StyleFlags (Styles::kHorizontal, Styles::kLayoutWrap));

	ViewBox view0 = addView ({0, 0, 40, 20}, {0, 10, kMaxCoord, 30});
	ViewBox view1 = addView ({0, 0, 40, 30}, {0, 10, kMaxCoord, 40});
	ViewBox view2 = addView ({0, 0, 40, 40}, {0, 30, kMaxCoord, 50});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	CCL_TEST_ASSERT_EQUAL (sizeLimit.minHeight, 70);
}

//************************************************************************************************
// BoxMainAxisTest
// Tests for a responsive main axis
//************************************************************************************************

class BoxMainAxisTest: public LayoutTestBase
{
public:
	BoxMainAxisTest ()
	{
		addTestValue (IView::kAttachLeft | IView::kAttachRight); // Item sizes are responsive for main axis
		addTestValue (IView::kAttachAll);						 // Item sizes are responsive for both axis
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxMainAxisTest, PreferredSizeReflectsChildren)
{
	addView ({0, 0, 20, 20});
	addView ({0, 0, 50, 20});
	addView ({0, 0, 30, 20});

	layoutView.autoSize ();

	CCL_TEST_ASSERT_EQUAL (layoutView.getWidth (), 100);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxMainAxisTest, PreferredSizeReflectsMargins)
{
	layoutView.setAttribute (ATTR_MARGIN, 10);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 20});
	layoutView.autoSize ();

	CCL_TEST_ASSERT_EQUAL (layoutView.getWidth (), 60);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxMainAxisTest, PreferredSizeReflectsSpacing)
{
	layoutView.setAttribute (ATTR_SPACING, 10);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 20});
	layoutView.autoSize ();

	CCL_TEST_ASSERT_EQUAL (layoutView.getWidth (), 50);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxMainAxisTest, PreferredSizeReflectsChildrenOnCrossAxis)
{
	ViewBox view0 = addView ({0, 0, 20, 20});
	ViewBox view1 = addView ({0, 20, 20, 40});
	ViewBox view2 = addView ({0, 0, 20, 30});
	layoutView.autoSize ();

	if(getTestValue () == IView::kVCenter) // The view has an initial vertical offset, which changes due to the view beeing centred
		CCL_TEST_ASSERT_EQUAL (layoutView.getHeight (), 30);
	else
		CCL_TEST_ASSERT_EQUAL (layoutView.getHeight (), 40);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxMainAxisTest, PreferredSizeReactsToChildSizeChange)
{
	ViewBox view0 = addView ({0, 0, 20, 20});
	ViewBox view1 = addView ({0, 20, 20, 40});

	view1.setSize ({0, 20, 20, 60});
	layoutView.autoSize ();

	if(getTestValue () == IView::kVCenter) // The view has an initial vertical offset, which changes due to the view beeing centred
		CCL_TEST_ASSERT_EQUAL (layoutView.getHeight (), 40);
	else
		CCL_TEST_ASSERT_EQUAL (layoutView.getHeight (), 60);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxMainAxisTest, PreferredSizeReflectsMarginOnCrossAxis)
{
	layoutView.setAttribute (ATTR_MARGIN, 10);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 20});
	layoutView.autoSize ();

	CCL_TEST_ASSERT_EQUAL (layoutView.getHeight (), 40);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxMainAxisTest, PreferredSizeCrossAxisSpacingHasNoEffect)
{
	layoutView.setAttribute (ATTR_SPACING, 10);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 20});

	layoutView.autoSize ();
	CCL_TEST_ASSERT_EQUAL (layoutView.getHeight (), 20);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxMainAxisTest, UnifySizesStyleUnifiesCrossAxisItems)
{
	layoutView.setStyle (StyleFlags (Styles::kHorizontal, Styles::kLayoutUnifySizes));

	ViewBox view0 = addView ({0, 0, 20, 20});
	ViewBox view1 = addView ({0, 0, 20, 30});
	ViewBox view2 = addView ({0, 0, 20, 40});

	bool attachedToCrossAxis = getTestValue () & (IView::kAttachTop | IView::kAttachBottom);

	if(attachedToCrossAxis)
	{
		CCL_TEST_ASSERT_EQUAL (view0.getHeight (), view1.getHeight ());
		CCL_TEST_ASSERT_EQUAL (view1.getHeight (), view2.getHeight ());
	}
	else
	{
		CCL_TEST_ASSERT_NOT_EQUAL (view0.getHeight (), view1.getHeight ());
		CCL_TEST_ASSERT_NOT_EQUAL (view0.getHeight (), view2.getHeight ());
		CCL_TEST_ASSERT_NOT_EQUAL (view1.getHeight (), view2.getHeight ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxMainAxisTest, UnifySizesStyleAdaptsToResizedChild)
{
	layoutView.setStyle (StyleFlags (Styles::kHorizontal, Styles::kLayoutUnifySizes));

	ViewBox view0 = addView ({0, 0, 20, 20});
	ViewBox view1 = addView ({0, 0, 20, 30});
	ViewBox view2 = addView ({0, 0, 20, 40});

	view1.setSize ({0, 0, 20, 60});

	bool attachedToCrossAxis = getTestValue () & (IView::kAttachTop | IView::kAttachBottom);
	if(attachedToCrossAxis)
	{
		CCL_TEST_ASSERT_EQUAL (view0.getHeight (), 60);
		CCL_TEST_ASSERT_EQUAL (view0.getHeight (), view1.getHeight ());
		CCL_TEST_ASSERT_EQUAL (view1.getHeight (), view2.getHeight ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxMainAxisTest, SizeLimitForFitSizeModeReflectsPreferredSize)
{
	layoutView.setAttribute (ATTR_MARGIN, 10);
	layoutView.setAttribute (ATTR_SPACING, 10);
	layoutView.setSizeMode (IView::kFitSize);

	addView ({0, 0, 20, 20});
	addView ({0, 0, 20, 30});
	addView ({0, 0, 20, 40});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	CCL_TEST_ASSERT_EQUAL (sizeLimit.maxWidth, 100);
	CCL_TEST_ASSERT_EQUAL (sizeLimit.minWidth, 100);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxMainAxisTest, ByDefaultSizeLimitsReflectChildLimitsIncludingMarginAndSpacing)
{
	layoutView.setAttribute (ATTR_MARGIN, 10);
	layoutView.setAttribute (ATTR_SPACING, 10);

	ViewBox view0 = addView ({0, 0, 20, 20}, {10, 0, 40, kMaxCoord});
	ViewBox view1 = addView ({0, 0, 20, 30}, {10, 0, 40, kMaxCoord});
	ViewBox view2 = addView ({0, 0, 20, 40}, {10, 0, 40, kMaxCoord});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	if(getTestValue () & (IView::kAttachLeft | IView::kAttachRight)) // Item sizes are locked if not attached
	{
		CCL_TEST_ASSERT_EQUAL (sizeLimit.maxWidth, 160) << String ().appendFormat ("At test value: %(1)", getTestValue ());
		CCL_TEST_ASSERT_EQUAL (sizeLimit.minWidth, 70);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxMainAxisTest, StyleLayoutHidePriorityCanHideAllItems)
{
	layoutView.setStyle (StyleFlags (Styles::kHorizontal, Styles::kLayoutHidePriority));

	int margin = 10;
	layoutView.setAttribute (ATTR_MARGIN, margin);
	layoutView.setAttribute (ATTR_SPACING, 10);

	ViewBox view0 = addView ({0, 0, 20, 20}, {10, 0, 40, kMaxCoord});
	ViewBox view1 = addView ({0, 0, 20, 30}, {10, 0, 40, kMaxCoord});
	ViewBox view2 = addView ({0, 0, 20, 40}, {10, 0, 40, kMaxCoord});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	CCL_TEST_ASSERT_EQUAL (sizeLimit.minWidth, 2 * margin);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxMainAxisTest, LayoutWrapMinSizeConsidersBiggestItem)
{
	layoutView.setStyle (StyleFlags (Styles::kHorizontal, Styles::kLayoutWrap));

	int margin = 10;
	layoutView.setAttribute (ATTR_MARGIN, margin);
	layoutView.setAttribute (ATTR_SPACING, 10);

	ViewBox view0 = addView ({0, 0, 20, 20}, {10, 0, 40, kMaxCoord});
	ViewBox view1 = addView ({0, 0, 20, 30}, {12, 0, 40, kMaxCoord});
	ViewBox view2 = addView ({0, 0, 20, 40}, {14, 0, 40, kMaxCoord});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	if(getTestValue () & (IView::kAttachLeft | IView::kAttachRight)) // Item sizes are locked if not attached
		CCL_TEST_ASSERT_EQUAL (sizeLimit.minWidth, 14 + 2 * margin);
	else
		CCL_TEST_ASSERT_EQUAL (sizeLimit.minWidth, 20 + 2 * margin);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxMainAxisTest, MinLimitIsZeroForStyleNoMinLimit)
{
	layoutView.setStyle (StyleFlags (Styles::kHorizontal, Styles::kLayoutNoMinLimit));

	layoutView.setAttribute (ATTR_MARGIN, 10);
	layoutView.setAttribute (ATTR_SPACING, 10);

	ViewBox view0 = addView ({0, 0, 20, 20}, {10, 0, 40, kMaxCoord});
	ViewBox view1 = addView ({0, 0, 20, 30}, {12, 0, 40, kMaxCoord});
	ViewBox view2 = addView ({0, 0, 20, 40}, {14, 0, 40, kMaxCoord});

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();

	CCL_TEST_ASSERT_EQUAL (sizeLimit.minWidth, 0);
}

//************************************************************************************************
// BoxCrossAxisTest
// Tests for a responsive cross axis - e.g. top and bottom attachment on a horizontal main axis
//************************************************************************************************

class BoxCrossAxisTest: public LayoutTestBase
{
public:
	BoxCrossAxisTest ()
	{
		addTestValue (IView::kAttachTop | IView::kAttachBottom); // Item sizes are responsive for main axis
		addTestValue (IView::kAttachAll);						 // Item sizes are responsive for both axis
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxCrossAxisTest, InitialTopAndBottomDistanceIsConsideredAsMargin)
{
	ViewBox view0 = addView ({0, 20, 20, 40}, {0, 10, kMaxCoord, 30});

	Coord topDistance = view0.getSize ().top - bounds.top;
	Coord bottomDistance = bounds.bottom - view0.getSize ().bottom;

	Coord viewMinHeight = view0->getSizeLimits ().minHeight;
	Coord viewMaxHeight = view0->getSizeLimits ().maxHeight;

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();
	CCL_TEST_ASSERT_EQUAL (sizeLimit.minHeight, viewMinHeight + topDistance + bottomDistance);
	CCL_TEST_ASSERT_EQUAL (sizeLimit.maxHeight, viewMaxHeight + topDistance + bottomDistance);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxCrossAxisTest, ResizingTheParentDoesntChangeItsSizeLimits)
{
	ViewBox view0 = addView ({0, 20, 20, 40}, {0, 10, kMaxCoord, 30});
	layoutView.setSize ({0, 0, 120, 120});

	bounds = layoutView->getSize ();

	Coord topDistance = view0.getSize ().top - bounds.top;
	Coord bottomDistance = bounds.bottom - view0.getSize ().bottom;

	Coord viewMinHeight = view0->getSizeLimits ().minHeight;
	Coord viewMaxHeight = view0->getSizeLimits ().maxHeight;

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();
	CCL_TEST_ASSERT_EQUAL (sizeLimit.minHeight, viewMinHeight + topDistance + bottomDistance);
	CCL_TEST_ASSERT_EQUAL (sizeLimit.maxHeight, viewMaxHeight + topDistance + bottomDistance);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxCrossAxisTest, ItemMarginsAreLimitedByParentMargins)
{
	Coord margin = 40;
	layoutView.setAttribute (ATTR_MARGIN, margin);

	ViewBox view0 = addView ({0, 20, 20, 40}, {0, 10, kMaxCoord, 30});

	Coord topDistance = view0.getSize ().top - bounds.top;
	Coord bottomDistance = bounds.bottom - view0.getSize ().bottom;

	Coord viewMinHeight = view0->getSizeLimits ().minHeight;
	Coord viewMaxHeight = view0->getSizeLimits ().maxHeight;

	const SizeLimit& sizeLimit = layoutView.getSizeLimits ();
	CCL_TEST_ASSERT_EQUAL (sizeLimit.minHeight, viewMinHeight + topDistance + margin);
	CCL_TEST_ASSERT_EQUAL (sizeLimit.maxHeight, viewMaxHeight + topDistance + margin);
}

//************************************************************************************************
// BoxWrapLayoutTest
//************************************************************************************************

class BoxWrapLayoutTest: public LayoutTestBase
{
public:
	BoxWrapLayoutTest ()
	{
		addTestValue (0); // RigidBoxLayout is used if sizeMode == 0
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxWrapLayoutTest, WrapShouldMoveItemsToNextRow)
{
	layoutView.setStyle (StyleFlags (Styles::kHorizontal, Styles::kLayoutWrap));

	ViewBox view0 = addView ({0, 0, 40, 40});
	ViewBox view1 = addView ({0, 0, 40, 40});
	ViewBox view2 = addView ({0, 0, 40, 40});

	layoutView.setSize ({0, 0, 100, 0});
	layoutView.autoSize (false, true);

	CCL_TEST_ASSERT_EQUAL (view0.getSize ().left, 0);
	CCL_TEST_ASSERT_EQUAL (view1.getSize ().left, 40);
	CCL_TEST_ASSERT_EQUAL (view2.getSize ().left, 0);

	CCL_TEST_ASSERT_EQUAL (view0.getSize ().top, 0);
	CCL_TEST_ASSERT_EQUAL (view1.getSize ().top, 0);
	CCL_TEST_ASSERT_EQUAL (view2.getSize ().top, 40);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_P (BoxWrapLayoutTest, WrapShouldAdjustParent)
{
	layoutView.setStyle (StyleFlags (Styles::kHorizontal, Styles::kLayoutWrap));

	ViewBox view0 = addView ({0, 0, 40, 40});
	ViewBox view1 = addView ({0, 0, 40, 40});
	ViewBox view2 = addView ({0, 0, 40, 40});

	layoutView.setSize ({0, 0, 100, 0});
	layoutView.autoSize (false, true);

	CCL_TEST_ASSERT_EQUAL (layoutView.getSize ().bottom, 80);
}
