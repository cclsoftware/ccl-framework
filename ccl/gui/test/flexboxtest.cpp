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
// Filename    : flexboxtest.cpp
// Description : Flexbox Unit Tests
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/gui/layout/flexboxlayout.h"
#include "ccl/gui/skin/skinattributes.h"

#include "ccl/public/gui/framework/skinxmldefs.h"

using namespace CCL;

//************************************************************************************************
// FlexLayoutTest
//************************************************************************************************

class FlexLayoutTest: public Test
{
public:
	FlexLayoutTest ()
	: notified (false)
	{}

	// Test
	void setUp () override
	{
		flexLayout = NEW TestableFlexboxLayout ();
		flexLayout->addObserver (this);
	}

	void tearDown () override
	{
		flexLayout->removeObserver (this);
	}

	void CCL_API notify (ISubject* subject, MessageRef msg) override
	{
		notified = true;
	}

protected:
	struct TestableFlexboxLayout: public FlexboxLayout
	{
		LayoutContext* createContext (LayoutView* parent) override { return nullptr; }
		LayoutAlgorithm* createAlgorithm (LayoutContext* context) override { return nullptr; };
		FlexData& getFlexData () { return flexData;	};
	};

	AutoPtr<TestableFlexboxLayout> flexLayout;
	bool notified;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexLayoutTest, SkinAttributesContainFlexAttributes)
{
	MutableSkinAttributes attributes;

	flexLayout->getAttributes (attributes);

	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXDIRECTION));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXWRAP));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXJUSTIFY));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXALIGN));
	
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXPADDINGLEFT));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXPADDINGTOP));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXPADDINGRIGHT));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXPADDINGBOTTOM));
	
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXGAPROW));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXGAPCOLUMN));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexLayoutTest, SkinAttributesReflectDefaults)
{
	MutableSkinAttributes attributes;

	flexLayout->getAttributes (attributes);

	CCL_TEST_ASSERT_EQUAL ((FlexDirection)attributes.getOptions (ATTR_FLEXDIRECTION, FlexboxLayout::flexDirection), FlexDirection::kRow);
	CCL_TEST_ASSERT_EQUAL ((FlexWrap)attributes.getOptions (ATTR_FLEXWRAP, FlexboxLayout::flexWrap), FlexWrap::kNoWrap);
	CCL_TEST_ASSERT_EQUAL ((FlexJustify)attributes.getOptions (ATTR_FLEXJUSTIFY, FlexboxLayout::flexJustify), FlexJustify::kFlexStart);
	CCL_TEST_ASSERT_EQUAL ((FlexAlign)attributes.getOptions (ATTR_FLEXALIGN, FlexboxLayout::flexAlign), FlexAlign::kStretch);
	
	CCL_TEST_ASSERT_EQUAL (attributes.getString (ATTR_FLEXGAPROW), DesignCoord::kStrUndefined);
	CCL_TEST_ASSERT_EQUAL (attributes.getString (ATTR_FLEXGAPCOLUMN), DesignCoord::kStrUndefined);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexLayoutTest, SkinOptionAttributeUpdatesAreReflected)
{
	{
		MutableSkinAttributes attributes;
		
		attributes.setOptions (ATTR_FLEXDIRECTION, int(FlexDirection::kColumn), FlexboxLayout::flexDirection, true);
		attributes.setOptions (ATTR_FLEXWRAP, int(FlexWrap::kWrap), FlexboxLayout::flexWrap, true);
		attributes.setOptions (ATTR_FLEXJUSTIFY, int(FlexJustify::kFlexEnd), FlexboxLayout::flexJustify, true);
		attributes.setOptions (ATTR_FLEXALIGN, int(FlexAlign::kCenter), FlexboxLayout::flexAlign, true);
		
		flexLayout->setAttributes (attributes);
	}
	
	{
		MutableSkinAttributes attributes;
		flexLayout->getAttributes (attributes);
		
		CCL_TEST_ASSERT_EQUAL ((FlexDirection)attributes.getOptions (ATTR_FLEXDIRECTION, FlexboxLayout::flexDirection), FlexDirection::kColumn);
		CCL_TEST_ASSERT_EQUAL ((FlexWrap)attributes.getOptions (ATTR_FLEXWRAP, FlexboxLayout::flexWrap), FlexWrap::kWrap);
		CCL_TEST_ASSERT_EQUAL ((FlexJustify)attributes.getOptions (ATTR_FLEXJUSTIFY, FlexboxLayout::flexJustify), FlexJustify::kFlexEnd);
		CCL_TEST_ASSERT_EQUAL ((FlexAlign)attributes.getOptions (ATTR_FLEXALIGN, FlexboxLayout::flexAlign), FlexAlign::kCenter);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexLayoutTest, SkinPaddingAttributeUpdatesAreReflected)
{
	{
		MutableSkinAttributes attributes;
		
		attributes.setInt (ATTR_FLEXPADDINGLEFT, 10);
		attributes.setInt (ATTR_FLEXPADDINGTOP, 10);
		attributes.setInt (ATTR_FLEXPADDINGRIGHT, 10);
		attributes.setInt (ATTR_FLEXPADDINGBOTTOM, 10);
		
		flexLayout->setAttributes (attributes);
	}
	
	{
		MutableSkinAttributes attributes;
		flexLayout->getAttributes (attributes);
		
		CCL_TEST_ASSERT_EQUAL (attributes.getInt (ATTR_FLEXPADDINGLEFT), 10);
		CCL_TEST_ASSERT_EQUAL (attributes.getInt (ATTR_FLEXPADDINGTOP), 10);
		CCL_TEST_ASSERT_EQUAL (attributes.getInt (ATTR_FLEXPADDINGRIGHT), 10);
		CCL_TEST_ASSERT_EQUAL (attributes.getInt (ATTR_FLEXPADDINGBOTTOM), 10);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexLayoutTest, SkinOptionPropertyUpdatesAreReflected)
{
	flexLayout->setProperty (ATTR_FLEXDIRECTION, int(FlexDirection::kColumn));
	flexLayout->setProperty (ATTR_FLEXWRAP, int(FlexWrap::kWrap));
	flexLayout->setProperty (ATTR_FLEXJUSTIFY, int(FlexJustify::kFlexEnd));
	flexLayout->setProperty (ATTR_FLEXALIGN, int(FlexAlign::kCenter));
	
	Variant flexDirection, flexWrap, flexJustify, flexAlign;
	
	flexLayout->getProperty (flexDirection, ATTR_FLEXDIRECTION);
	flexLayout->getProperty (flexWrap, ATTR_FLEXWRAP);
	flexLayout->getProperty (flexJustify, ATTR_FLEXJUSTIFY);
	flexLayout->getProperty (flexAlign, ATTR_FLEXALIGN);
	
	CCL_TEST_ASSERT_EQUAL (flexDirection.asInt (), int(FlexDirection::kColumn));
	CCL_TEST_ASSERT_EQUAL (flexWrap.asInt (), int(FlexWrap::kWrap));
	CCL_TEST_ASSERT_EQUAL (flexJustify.asInt (), int(FlexJustify::kFlexEnd));
	CCL_TEST_ASSERT_EQUAL (flexAlign.asInt (), int(FlexAlign::kCenter));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexLayoutTest, SkinPaddingPropertyUpdatesAreReflected)
{
	Vector<CString> attributes = {ATTR_FLEXPADDINGLEFT, ATTR_FLEXPADDINGTOP, ATTR_FLEXPADDINGRIGHT, ATTR_FLEXPADDINGBOTTOM};
	
	for(auto& attribute : attributes)
	{
		flexLayout->setProperty (attribute, DesignCoord (DesignCoord::kCoord, 10).toVariant ());
		
		Variant variant;
		flexLayout->getProperty (variant, attribute);
		
		CCL_TEST_ASSERT (DesignCoord ().fromVariant (variant).isCoord ());
		CCL_TEST_ASSERT_EQUAL (DesignCoord ().fromVariant (variant).value, 10);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexLayoutTest, AttributeChangeDoesNotify)
{
	MutableSkinAttributes attributes;
	flexLayout->setAttributes (attributes);
	CCL_TEST_ASSERT (notified);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexLayoutTest, PropertyChangeDoesNotify)
{
	flexLayout->setProperty (ATTR_FLEXDIRECTION, 0);
	CCL_TEST_ASSERT (notified);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexLayoutTest, PaddingShortHandsFromPropertiesAreParsedCorrectly)
{
	FlexData& flexData = flexLayout->getFlexData ();

	flexLayout->setProperty (ATTR_FLEXPADDING, "10");
	CCL_TEST_ASSERT (flexData.padding.left == flexData.padding.top && flexData.padding.left == flexData.padding.right && flexData.padding.left == flexData.padding.bottom);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.left.value, 10);

	flexLayout->setProperty (ATTR_FLEXPADDING, "10,20");
	CCL_TEST_ASSERT (flexData.padding.left == flexData.padding.right && flexData.padding.top == flexData.padding.bottom);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.left.value, 10);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.top.value, 20);
	
	flexLayout->setProperty (ATTR_FLEXPADDING, "10,20,30");
	CCL_TEST_ASSERT (flexData.padding.top == flexData.padding.bottom);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.left.value, 10);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.top.value, 20);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.right.value, 30);
	
	flexLayout->setProperty (ATTR_FLEXPADDING, "10,20,30,40");
	CCL_TEST_ASSERT_EQUAL (flexData.padding.left.value, 10);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.top.value, 20);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.right.value, 30);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.bottom.value, 40);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexLayoutTest, PaddingShortHandsFromAttributesAreParsedCorrectly)
{
	MutableSkinAttributes attributes;
	FlexData& flexData = flexLayout->getFlexData ();
	
	attributes.setString (ATTR_FLEXPADDING, "10");
	flexLayout->setAttributes (attributes);
	
	CCL_TEST_ASSERT (flexData.padding.left == flexData.padding.top && flexData.padding.left == flexData.padding.right && flexData.padding.left == flexData.padding.bottom);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.left.value, 10);
	
	attributes.setString (ATTR_FLEXPADDING, "10,20");
	flexLayout->setAttributes (attributes);
	
	CCL_TEST_ASSERT (flexData.padding.left == flexData.padding.right && flexData.padding.top == flexData.padding.bottom);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.left.value, 10);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.bottom.value, 20);
	
	attributes.setString (ATTR_FLEXPADDING, "10,20,30");
	flexLayout->setAttributes (attributes);
	
	CCL_TEST_ASSERT (flexData.padding.top == flexData.padding.bottom);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.left.value, 10);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.top.value, 20);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.right.value, 30);
	
	attributes.setString (ATTR_FLEXPADDING, "10,20,30,40");
	flexLayout->setAttributes (attributes);
	
	CCL_TEST_ASSERT_EQUAL (flexData.padding.left.value, 10);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.top.value, 20);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.right.value, 30);
	CCL_TEST_ASSERT_EQUAL (flexData.padding.bottom.value, 40);
}

//************************************************************************************************
// FlexItemTest
//************************************************************************************************

class FlexItemTest: public Test
{
public:
	FlexItemTest ()
	: notified (false)
	{}

	// Test
	void setUp () override
	{
		flexItem.addObserver (this);
	}

	void tearDown () override
	{
		flexItem.removeObserver (this);
	}

	void CCL_API notify (ISubject* subject, MessageRef msg) override
	{
		notified = true;
	}

protected:
	FlexItem flexItem;
	bool notified;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexItemTest, SkinAttributesContainFlexAttributes)
{
	MutableSkinAttributes attributes;

	flexItem.getAttributes (attributes);

	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXGROW));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXSHRINK));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXBASIS));

	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXALIGNSELF));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXPOSITIONTYPE));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXSIZEMODE));
	
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXMARGINTOP));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXMARGINRIGHT));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXMARGINBOTTOM));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXMARGINLEFT));
	
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXINSETTOP));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXINSETRIGHT));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXINSETBOTTOM));
	CCL_TEST_ASSERT (attributes.exists (ATTR_FLEXINSETLEFT));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexItemTest, SkinAttributesReflectDefaults)
{
	MutableSkinAttributes attributes;
	
	flexItem.getAttributes (attributes);
	
	CCL_TEST_ASSERT_EQUAL (attributes.getFloat (ATTR_FLEXGROW), 0.f);
	CCL_TEST_ASSERT_EQUAL (attributes.getFloat (ATTR_FLEXSHRINK), 1.f);
	CCL_TEST_ASSERT_EQUAL (attributes.getString (ATTR_FLEXBASIS), DesignCoord::kStrAuto);
	
	CCL_TEST_ASSERT_EQUAL (attributes.getOptions (ATTR_FLEXALIGNSELF, flexItem.flexAlignSelf), (int)FlexAlignSelf::kAuto);
	CCL_TEST_ASSERT_EQUAL (attributes.getOptions (ATTR_FLEXPOSITIONTYPE, flexItem.flexPositionType), (int)FlexPositionType::kRelative);
	CCL_TEST_ASSERT_EQUAL (attributes.getOptions (ATTR_FLEXSIZEMODE, flexItem.flexSizeMode), (int)FlexSizeMode::kFill);
	
	CCL_TEST_ASSERT_EQUAL (attributes.getString (ATTR_FLEXMARGINTOP), DesignCoord::kStrUndefined);
	CCL_TEST_ASSERT_EQUAL (attributes.getString (ATTR_FLEXMARGINRIGHT), DesignCoord::kStrUndefined);
	CCL_TEST_ASSERT_EQUAL (attributes.getString (ATTR_FLEXMARGINBOTTOM), DesignCoord::kStrUndefined);
	CCL_TEST_ASSERT_EQUAL (attributes.getString (ATTR_FLEXMARGINLEFT), DesignCoord::kStrUndefined);
	
	CCL_TEST_ASSERT_EQUAL (attributes.getString (ATTR_FLEXINSETTOP), DesignCoord::kStrUndefined);
	CCL_TEST_ASSERT_EQUAL (attributes.getString (ATTR_FLEXINSETRIGHT), DesignCoord::kStrUndefined);
	CCL_TEST_ASSERT_EQUAL (attributes.getString (ATTR_FLEXINSETBOTTOM), DesignCoord::kStrUndefined);
	CCL_TEST_ASSERT_EQUAL (attributes.getString (ATTR_FLEXINSETLEFT), DesignCoord::kStrUndefined);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexItemTest, AttributesUpdateItemData)
{
	MutableSkinAttributes attributes;
	attributes.setFloat (ATTR_FLEXGROW, .5f);
	attributes.setFloat (ATTR_FLEXSHRINK, .5f);
	
	attributes.setOptions (ATTR_FLEXALIGNSELF, int(FlexAlignSelf::kFlexEnd), FlexItem::flexAlignSelf, true);
	attributes.setOptions (ATTR_FLEXPOSITIONTYPE, int(FlexPositionType::kAbsolute), FlexItem::flexPositionType, true);
	attributes.setOptions (ATTR_FLEXSIZEMODE, int(FlexSizeMode::kHugVertical), FlexItem::flexSizeMode, true);
	
	attributes.setInt (ATTR_FLEXBASIS, 10);
	
	flexItem.setAttributes (attributes);

	const FlexItemData& flexData = flexItem.getFlexItemData ();
	
	CCL_TEST_ASSERT_EQUAL (flexData.grow, .5f);
	CCL_TEST_ASSERT_EQUAL (flexData.shrink, .5f);
	CCL_TEST_ASSERT_EQUAL (flexData.alignSelf, FlexAlignSelf::kFlexEnd);
	CCL_TEST_ASSERT_EQUAL (flexData.positionType, FlexPositionType::kAbsolute);
	CCL_TEST_ASSERT_EQUAL (flexData.sizeMode, FlexSizeMode::kHugVertical);
	CCL_TEST_ASSERT_EQUAL (flexData.flexBasis.value, 10);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexItemTest, AttributeDoesNotChangeOtherAttribute)
{
	MutableSkinAttributes attributes;
	attributes.setFloat (ATTR_FLEXGROW, .5f);
	attributes.setFloat (ATTR_FLEXSHRINK, 2.f);

	flexItem.setAttributes (attributes);

	CCL_TEST_ASSERT_EQUAL (flexItem.getFlexItemData ().grow, .5f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexItemTest, AttributeChangeDoesNotify)
{
	MutableSkinAttributes attributes;
	attributes.setFloat (ATTR_FLEXGROW, .5f);

	flexItem.setAttributes (attributes);

	CCL_TEST_ASSERT (notified);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexItemTest, MarginCoordShorthandsAreReflected)
{
	const FlexItemData& flexItemData = flexItem.getFlexItemData ();

	flexItem.setProperty (ATTR_FLEXMARGIN, "10");
	CCL_TEST_ASSERT (flexItemData.margin.left == flexItemData.margin.top && flexItemData.margin.left == flexItemData.margin.right && flexItemData.margin.left == flexItemData.margin.bottom);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.left.value, 10);

	flexItem.setProperty (ATTR_FLEXMARGIN, "10,20");
	CCL_TEST_ASSERT (flexItemData.margin.left == flexItemData.margin.right && flexItemData.margin.top == flexItemData.margin.bottom);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.left.value, 10);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.top.value, 20);
	
	flexItem.setProperty (ATTR_FLEXMARGIN, "10,20,30");
	CCL_TEST_ASSERT (flexItemData.margin.top == flexItemData.margin.bottom);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.left.value, 10);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.top.value, 20);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.right.value, 30);
	
	flexItem.setProperty (ATTR_FLEXMARGIN, "10,20,30,40");
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.left.value, 10);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.top.value, 20);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.right.value, 30);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.bottom.value, 40);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexItemTest, MarginAutoShorthandsAreReflected)
{
	const FlexItemData& flexItemData = flexItem.getFlexItemData ();

	flexItem.setProperty (ATTR_FLEXMARGIN, "auto");
	CCL_TEST_ASSERT (flexItemData.margin.left == flexItemData.margin.top && flexItemData.margin.left == flexItemData.margin.right && flexItemData.margin.left == flexItemData.margin.bottom);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.left.unit, DesignCoord::kAuto);

	flexItem.setProperty (ATTR_FLEXMARGIN, "auto,auto");
	CCL_TEST_ASSERT (flexItemData.margin.left == flexItemData.margin.right && flexItemData.margin.top == flexItemData.margin.bottom);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.left.unit, DesignCoord::kAuto);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.top.unit, DesignCoord::kAuto);
	
	flexItem.setProperty (ATTR_FLEXMARGIN, "auto,auto,auto");
	CCL_TEST_ASSERT (flexItemData.margin.top == flexItemData.margin.bottom);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.left.unit, DesignCoord::kAuto);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.top.unit, DesignCoord::kAuto);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.right.unit, DesignCoord::kAuto);
	
	flexItem.setProperty (ATTR_FLEXMARGIN, "auto,auto,auto,auto");
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.left.unit, DesignCoord::kAuto);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.top.unit, DesignCoord::kAuto);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.right.unit, DesignCoord::kAuto);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.bottom.unit, DesignCoord::kAuto);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexItemTest, MarginMixedShorthandsAreReflected)
{
	const FlexItemData& flexItemData = flexItem.getFlexItemData ();

	flexItem.setProperty (ATTR_FLEXMARGIN, "auto");
	CCL_TEST_ASSERT (flexItemData.margin.left == flexItemData.margin.top && flexItemData.margin.left == flexItemData.margin.right && flexItemData.margin.left == flexItemData.margin.bottom);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.left.unit, DesignCoord::kAuto);

	flexItem.setProperty (ATTR_FLEXMARGIN, "auto,10");
	CCL_TEST_ASSERT (flexItemData.margin.left == flexItemData.margin.right && flexItemData.margin.top == flexItemData.margin.bottom);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.left.unit, DesignCoord::kAuto);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.top.value, 10);
	
	flexItem.setProperty (ATTR_FLEXMARGIN, "auto,10,auto");
	CCL_TEST_ASSERT (flexItemData.margin.top == flexItemData.margin.bottom);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.left.unit, DesignCoord::kAuto);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.top.value, 10);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.right.unit, DesignCoord::kAuto);
	
	flexItem.setProperty (ATTR_FLEXMARGIN, "auto,10,auto,20");
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.left.unit, DesignCoord::kAuto);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.top.value, 10);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.right.unit, DesignCoord::kAuto);
	CCL_TEST_ASSERT_EQUAL (flexItemData.margin.bottom.value, 20);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexItemTest, InsetShorthandsAreReflected)
{
	const FlexItemData& flexItemData = flexItem.getFlexItemData ();

	flexItem.setProperty (ATTR_FLEXINSET, "10");
	CCL_TEST_ASSERT (flexItemData.inset.left == flexItemData.inset.top && flexItemData.inset.left == flexItemData.inset.right && flexItemData.inset.left == flexItemData.inset.bottom);
	CCL_TEST_ASSERT_EQUAL (flexItemData.inset.left.value, 10);

	flexItem.setProperty (ATTR_FLEXINSET, "10,20");
	CCL_TEST_ASSERT (flexItemData.inset.left == flexItemData.inset.right && flexItemData.inset.top == flexItemData.inset.bottom);
	CCL_TEST_ASSERT_EQUAL (flexItemData.inset.left.value, 10);
	CCL_TEST_ASSERT_EQUAL (flexItemData.inset.top.value, 20);
	
	flexItem.setProperty (ATTR_FLEXINSET, "10,20,30");
	CCL_TEST_ASSERT (flexItemData.inset.top == flexItemData.inset.bottom);
	CCL_TEST_ASSERT_EQUAL (flexItemData.inset.left.value, 10);
	CCL_TEST_ASSERT_EQUAL (flexItemData.inset.top.value, 20);
	CCL_TEST_ASSERT_EQUAL (flexItemData.inset.right.value, 30);
	
	flexItem.setProperty (ATTR_FLEXINSET, "10,20,30,40");
	CCL_TEST_ASSERT_EQUAL (flexItemData.inset.left.value, 10);
	CCL_TEST_ASSERT_EQUAL (flexItemData.inset.top.value, 20);
	CCL_TEST_ASSERT_EQUAL (flexItemData.inset.right.value, 30);
	CCL_TEST_ASSERT_EQUAL (flexItemData.inset.bottom.value, 40);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexItemTest, IntPropertiesAreReflected)
{
	flexItem.setProperty (ATTR_FLEXBASIS, DesignCoord (DesignCoord::kCoord, 10).toVariant ());

	Variant basis, margin, inset;
	flexItem.getProperty (basis, ATTR_FLEXBASIS);

	CCL_TEST_ASSERT (DesignCoord ().fromVariant (basis).isCoord ());
	CCL_TEST_ASSERT_EQUAL (DesignCoord ().fromVariant (basis).value, 10);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexItemTest, FlexFloatPropertiesAreReflected)
{
	flexItem.setProperty (ATTR_FLEXGROW, Variant (.5f));
	flexItem.setProperty (ATTR_FLEXSHRINK, Variant (.5f));

	Variant grow, shrink;
	flexItem.getProperty (grow, ATTR_FLEXGROW);
	flexItem.getProperty (shrink, ATTR_FLEXGROW);

	CCL_TEST_ASSERT_EQUAL (flexItem.getFlexItemData ().grow, .5f);
	CCL_TEST_ASSERT_EQUAL (flexItem.getFlexItemData ().shrink, .5f);
	CCL_TEST_ASSERT_EQUAL (grow.asFloat (), .5f);
	CCL_TEST_ASSERT_EQUAL (shrink.asFloat (), .5f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexItemTest, FlexEnumPropertiesAreReflected)
{
	flexItem.setProperty (ATTR_FLEXALIGNSELF, (int)FlexAlignSelf::kCenter);
	flexItem.setProperty (ATTR_FLEXPOSITIONTYPE, (int)FlexPositionType::kAbsolute);
	flexItem.setProperty (ATTR_FLEXSIZEMODE, (int)FlexSizeMode::kHug);
	
	Variant alignSelf, positionType, sizeMode;
	flexItem.getProperty (alignSelf, ATTR_FLEXALIGNSELF);
	flexItem.getProperty (positionType, ATTR_FLEXPOSITIONTYPE);
	flexItem.getProperty (sizeMode, ATTR_FLEXSIZEMODE);

	CCL_TEST_ASSERT_EQUAL (flexItem.getFlexItemData ().alignSelf, FlexAlignSelf::kCenter);
	CCL_TEST_ASSERT_EQUAL (flexItem.getFlexItemData ().positionType, FlexPositionType::kAbsolute);
	CCL_TEST_ASSERT_EQUAL (flexItem.getFlexItemData ().sizeMode, FlexSizeMode::kHug);
	
	CCL_TEST_ASSERT_EQUAL ((FlexAlignSelf)alignSelf.asInt (), FlexAlignSelf::kCenter);
	CCL_TEST_ASSERT_EQUAL ((FlexPositionType)positionType.asInt (), FlexPositionType::kAbsolute);
	CCL_TEST_ASSERT_EQUAL ((FlexSizeMode)sizeMode.asInt (), FlexSizeMode::kHug);
}

//************************************************************************************************
// FlexAlgorithmTest
//************************************************************************************************

class FlexAlgorithmTest: public Test
{
public:
	// Test
	void setUp () override
	{
		flexLayout = LayoutFactory::instance ().createLayout (LAYOUTCLASS_FLEXBOX);
		flexLayout->setProperty (ATTR_FLEXDIRECTION, (int)FlexDirection::kRow);

		layoutView = NEW LayoutView (Rect (), 0);
		context = flexLayout->createContext (layoutView);

		flexAlgorithm = flexLayout->createAlgorithm (context);
	}

protected:
	AutoPtr<Layout> flexLayout;
	AutoPtr<LayoutView> layoutView;
	AutoPtr<LayoutAlgorithm> flexAlgorithm;
	AutoPtr<LayoutContext> context;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, PreferredSizeIsZeroInitially)
{
	const Point& preferredSize = flexAlgorithm->getPreferredSize ();

	CCL_TEST_ASSERT_EQUAL (preferredSize.x, 0.f);
	CCL_TEST_ASSERT_EQUAL (preferredSize.y, 0.f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, PreferredSizeIsNotInfluencedByParentSize)
{
	layoutView->setSize (Rect (100.f, 100.f));
	const Point& preferredSize = flexAlgorithm->getPreferredSize ();

	CCL_TEST_ASSERT_EQUAL (preferredSize.x, 0);
	CCL_TEST_ASSERT_EQUAL (preferredSize.y, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, PreferredSizeFitsChildrenIfAutoSized)
{
	View view ({0, 0, 20, 20});
	AutoPtr<LayoutItem> item = flexLayout->createItem (&view);
	flexAlgorithm->onItemAdded (item);

	const Point& preferredSize = flexAlgorithm->getPreferredSize ();
	CCL_TEST_ASSERT_EQUAL (preferredSize.x, 20);
	CCL_TEST_ASSERT_EQUAL (preferredSize.y, 20);

	flexAlgorithm->onItemRemoved (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, PreferredSizeIncreasedByPaddingIfAutoSized)
{
	View view ({0, 0, 20, 20});
	AutoPtr<LayoutItem> item = flexLayout->createItem (&view);

	flexLayout->setProperty (ATTR_FLEXPADDING, "10");
	flexAlgorithm->onItemAdded (item);

	const Point& preferredSize = flexAlgorithm->getPreferredSize ();
	CCL_TEST_ASSERT_EQUAL (preferredSize.x, 40);
	CCL_TEST_ASSERT_EQUAL (preferredSize.y, 40);

	flexAlgorithm->onItemRemoved (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, PreferredSizeIncreasedByIndividualPaddingIfAutoSized)
{
	View view ({0, 0, 20, 20});
	AutoPtr<LayoutItem> item = flexLayout->createItem (&view);

	flexLayout->setProperty (ATTR_FLEXPADDINGTOP, DesignCoord (DesignCoord::kCoord, 10).toVariant ());
	flexLayout->setProperty (ATTR_FLEXPADDINGRIGHT, DesignCoord (DesignCoord::kCoord, 10).toVariant ());
	flexAlgorithm->onItemAdded (item);

	const Point& preferredSize = flexAlgorithm->getPreferredSize ();
	CCL_TEST_ASSERT_EQUAL (preferredSize.x, 30);
	CCL_TEST_ASSERT_EQUAL (preferredSize.y, 30);

	flexAlgorithm->onItemRemoved (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, PreferredSizeCorrespondsToLayoutWidthAndHeightIfSet)
{
	layoutView->setSize (Rect (0, 0, 100, 100));
	flexAlgorithm->onSize ({100, 100});

	View view ({0, 0, 20, 20});
	AutoPtr<LayoutItem> item = flexLayout->createItem (&view);

	flexAlgorithm->onItemAdded (item);

	const Point& preferredSize = flexAlgorithm->getPreferredSize ();
	CCL_TEST_ASSERT_EQUAL (preferredSize.x, 100);
	CCL_TEST_ASSERT_EQUAL (preferredSize.y, 100);

	flexAlgorithm->onItemRemoved (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, PreferredSizeCorrespondsToWrappedItems)
{
	layoutView->setSize ({0, 0, 20, 0});
	
	flexAlgorithm = flexLayout->createAlgorithm (context);
	flexLayout->setProperty (ATTR_FLEXWRAP, (int)FlexWrap::kWrap);

	View view0;
	view0.setSize ({0, 0, 20, 20});
	AutoPtr<LayoutItem> item0 = flexLayout->createItem (&view0);

	View view1;
	view1.setSize ({0, 0, 20, 20});
	AutoPtr<LayoutItem> item1 = flexLayout->createItem (&view1);

	flexAlgorithm->onItemAdded (item0);
	flexAlgorithm->onItemAdded (item1);

	const Point& preferredSize = flexAlgorithm->getPreferredSize ();
	CCL_TEST_ASSERT_EQUAL (preferredSize.x, 20);
	CCL_TEST_ASSERT_EQUAL (preferredSize.y, 40);

	flexAlgorithm->onItemRemoved (item0);
	flexAlgorithm->onItemRemoved (item1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, SingleChildWithPositiveGrowFillsTheContainer)
{
	layoutView->setSize (Rect (0, 0, 100, 100));
	flexAlgorithm->onSize ({100, 100});

	View view;
	AutoPtr<LayoutItem> item = flexLayout->createItem (&view);
	item->setProperty (ATTR_FLEXGROW, 1.f);
	flexAlgorithm->onItemAdded (item);

	flexAlgorithm->doLayout ();

	CCL_TEST_ASSERT_EQUAL (view.getWidth (), 100);

	flexAlgorithm->onItemRemoved (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, ChildrenWithPositiveGrowFillTheContainer)
{
	layoutView->setSize (Rect (0, 0, 100, 100));
	flexAlgorithm->onSize ({100, 100});

	View view1;
	AutoPtr<LayoutItem> item1 = flexLayout->createItem (&view1);
	item1->setProperty (ATTR_FLEXGROW, 1.f);
	flexAlgorithm->onItemAdded (item1);

	View view2;
	AutoPtr<LayoutItem> item2 = flexLayout->createItem (&view2);
	item2->setProperty (ATTR_FLEXGROW, 1.f);
	flexAlgorithm->onItemAdded (item2);

	flexAlgorithm->doLayout ();

	CCL_TEST_ASSERT_EQUAL (view1.getPosition ().x, 0);
	CCL_TEST_ASSERT_EQUAL (view1.getWidth (), 50);
	CCL_TEST_ASSERT_EQUAL (view2.getPosition ().x, 50);
	CCL_TEST_ASSERT_EQUAL (view2.getWidth (), 50);

	flexAlgorithm->onItemRemoved (item1);
	flexAlgorithm->onItemRemoved (item2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, InsertedChildIsAtExpectedPosition)
{
	layoutView->setSize (Rect (0, 0, 100, 100));
	flexAlgorithm->onSize ({100, 100});

	// Add first child
	View view1;
	AutoPtr<LayoutItem> item1 = flexLayout->createItem (&view1);
	item1->setProperty (ATTR_FLEXGROW, 1.f);
	flexAlgorithm->onItemAdded (item1);

	// Add second child
	View view2;
	AutoPtr<LayoutItem> item2 = flexLayout->createItem (&view2);
	item2->setProperty (ATTR_FLEXGROW, 1.f);
	flexAlgorithm->onItemInserted (0, item2);

	flexAlgorithm->doLayout ();

	CCL_TEST_ASSERT_EQUAL (view1.getPosition ().x, 50);
	CCL_TEST_ASSERT_EQUAL (view1.getWidth (), 50);
	CCL_TEST_ASSERT_EQUAL (view2.getPosition ().x, 0);
	CCL_TEST_ASSERT_EQUAL (view2.getWidth (), 50);

	flexAlgorithm->onItemRemoved (item1);
	flexAlgorithm->onItemRemoved (item2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, RemovedChildMakesRoomForOtherItems)
{
	layoutView->setSize (Rect (0, 0, 100, 100));
	flexAlgorithm->onSize ({100, 100});

	View view1;
	AutoPtr<LayoutItem> item1 = flexLayout->createItem (&view1);
	item1->setProperty (ATTR_FLEXGROW, 1.f);
	flexAlgorithm->onItemAdded (item1);

	View view2;
	AutoPtr<LayoutItem> item2 = flexLayout->createItem (&view2);
	item2->setProperty (ATTR_FLEXGROW, 1.f);
	flexAlgorithm->onItemInserted (0, item2);

	flexAlgorithm->onItemRemoved (item2);

	flexAlgorithm->doLayout ();

	CCL_TEST_ASSERT_EQUAL (view1.getPosition ().x, 0);
	CCL_TEST_ASSERT_EQUAL (view1.getWidth (), 100);

	flexAlgorithm->onItemRemoved (item1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, ChildSizeLimitsAreRespected)
{
	layoutView->setSize (Rect (0, 0, 100, 100));
	flexAlgorithm->onSize ({100, 100});

	View view1;
	view1.setSizeLimits ({0, 0, 30, 30});
	AutoPtr<LayoutItem> item1 = flexLayout->createItem (&view1);
	item1->setProperty (ATTR_FLEXGROW, 1.f);
	flexAlgorithm->onItemAdded (item1);

	View view2;
	AutoPtr<LayoutItem> item2 = flexLayout->createItem (&view2);
	item2->setProperty (ATTR_FLEXGROW, 1.f);
	flexAlgorithm->onItemAdded (item2);

	flexAlgorithm->doLayout ();

	CCL_TEST_ASSERT_EQUAL (view1.getPosition ().x, 0);
	CCL_TEST_ASSERT_EQUAL (view1.getWidth (), 30);
	CCL_TEST_ASSERT_EQUAL (view1.getHeight (), 30);

	CCL_TEST_ASSERT_EQUAL (view2.getPosition ().x, 30);
	CCL_TEST_ASSERT_EQUAL (view2.getWidth (), 70);
	CCL_TEST_ASSERT_EQUAL (view2.getHeight (), 100);

	flexAlgorithm->onItemRemoved (item1);
	flexAlgorithm->onItemRemoved (item2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, ChildMarginsContributeToTheParentsPreferredSize)
{
	View view ({0, 0, 20, 20});
	AutoPtr<LayoutItem> item = flexLayout->createItem (&view);
	
	item->setProperty (ATTR_FLEXMARGIN, "10");
	flexAlgorithm->onItemAdded (item);

	const Point& preferredSize = flexAlgorithm->getPreferredSize ();
	CCL_TEST_ASSERT_EQUAL (preferredSize.x, 40);
	CCL_TEST_ASSERT_EQUAL (preferredSize.y, 40);

	flexAlgorithm->onItemRemoved (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, IndividualMarginsContributeToTheParentsPreferredSize)
{
	View view ({0, 0, 20, 20});
	AutoPtr<LayoutItem> item = flexLayout->createItem (&view);

	item->setProperty (ATTR_FLEXMARGINLEFT, DesignCoord (DesignCoord::kCoord, 10).toVariant ());
	flexAlgorithm->onItemAdded (item);

	const Point& preferredSize = flexAlgorithm->getPreferredSize ();
	CCL_TEST_ASSERT_EQUAL (preferredSize.x, 30);
	CCL_TEST_ASSERT_EQUAL (preferredSize.y, 20);

	flexAlgorithm->onItemRemoved (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, ChildrenArePositionedAccordingToMargin)
{
	View view ({0, 0, 20, 20});
	AutoPtr<LayoutItem> item = flexLayout->createItem (&view);

	item->setProperty (ATTR_FLEXMARGIN, "10");
	flexAlgorithm->onItemAdded (item);
	flexAlgorithm->doLayout ();
	
	CCL_TEST_ASSERT_EQUAL (view.getPosition ().x, 10);
	CCL_TEST_ASSERT_EQUAL (view.getPosition ().y, 10);

	flexAlgorithm->onItemRemoved (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, AbsolutelyPositionedChildrenAreDetachedFromLayoutFlow)
{
	View view1 ({0, 0, 20, 20});
	AutoPtr<LayoutItem> item1 = flexLayout->createItem (&view1);
	item1->setProperty (ATTR_FLEXPOSITIONTYPE, int(FlexPositionType::kAbsolute));
	
	View view2 ({0, 0, 20, 20});
	AutoPtr<LayoutItem> item2 = flexLayout->createItem (&view2);

	flexAlgorithm->onItemAdded (item1);
	flexAlgorithm->onItemAdded (item2);
	flexAlgorithm->doLayout ();
	
	CCL_TEST_ASSERT_EQUAL (view2.getPosition ().x, 0);

	flexAlgorithm->onItemRemoved (item1);
	flexAlgorithm->onItemRemoved (item2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, AbsolutelyPositionedChildrenUseTheirInitialSize)
{
	View view ({0, 0, 20, 20});
	AutoPtr<LayoutItem> item = flexLayout->createItem (&view);
	
	item->setProperty (ATTR_FLEXPOSITIONTYPE, int(FlexPositionType::kAbsolute));
	
	flexAlgorithm->onItemAdded (item);
	flexAlgorithm->doLayout ();
	
	CCL_TEST_ASSERT_EQUAL (view.getWidth (), 20);
	CCL_TEST_ASSERT_EQUAL (view.getHeight (), 20);

	flexAlgorithm->onItemRemoved (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, ChildIsInsetIndividuallyFromParentIfAbsolutePositionType)
{
	layoutView->setSize (Rect (0, 0, 100, 100));
	flexAlgorithm->onSize ({100, 100});
	
	View view;
	AutoPtr<LayoutItem> item = flexLayout->createItem (&view);
	
	item->setProperty (ATTR_FLEXPOSITIONTYPE, int(FlexPositionType::kAbsolute));
	item->setProperty (ATTR_FLEXINSETTOP, DesignCoord (DesignCoord::kCoord, 10).toVariant ());
	item->setProperty (ATTR_FLEXINSETRIGHT, DesignCoord (DesignCoord::kCoord, 10).toVariant ());
	item->setProperty (ATTR_FLEXINSETBOTTOM, DesignCoord (DesignCoord::kCoord, 10).toVariant ());
	item->setProperty (ATTR_FLEXINSETLEFT, DesignCoord (DesignCoord::kCoord, 10).toVariant ());
	
	flexAlgorithm->onItemAdded (item);
	flexAlgorithm->doLayout ();
	
	CCL_TEST_ASSERT_EQUAL (view.getPosition ().x, 10);
	CCL_TEST_ASSERT_EQUAL (view.getPosition ().y, 10);
	CCL_TEST_ASSERT_EQUAL (view.getWidth (), 80);
	CCL_TEST_ASSERT_EQUAL (view.getHeight (), 80);

	flexAlgorithm->onItemRemoved (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, RelativelySizedChildrenWithFullSizeFillParentAccordingly)
{
	layoutView->setSize (Rect (0, 0, 200, 200));
	flexAlgorithm->onSize ({200, 200});
	
	View view;
	AutoPtr<FlexItem> item = ccl_cast<FlexItem> (flexLayout->createItem (&view));
	
	DesignCoord zeroPercent (DesignCoord::kPercent, 0);
	DesignCoord hundredPercent (DesignCoord::kPercent, 100);
	item->initialize (DesignSize (zeroPercent, zeroPercent, hundredPercent, hundredPercent));
	
	flexAlgorithm->onItemAdded (item);
	flexAlgorithm->doLayout ();
	
	CCL_TEST_ASSERT_EQUAL (view.getWidth (), 200);
	CCL_TEST_ASSERT_EQUAL (view.getHeight (), 200);

	flexAlgorithm->onItemRemoved (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, RelativelySizedChildrenWithPartialSizeFillParentAccordingly)
{
	layoutView->setSize (Rect (0, 0, 200, 200));
	flexAlgorithm->onSize ({200, 200});
	
	View view;
	AutoPtr<FlexItem> item = ccl_cast<FlexItem> (flexLayout->createItem (&view));
	
	DesignCoord zeroPercent (DesignCoord::kPercent, 0);
	DesignCoord sixtyPercent (DesignCoord::kPercent, 60);
	item->initialize (DesignSize (zeroPercent, zeroPercent, sixtyPercent, sixtyPercent));
	
	flexAlgorithm->onItemAdded (item);
	flexAlgorithm->doLayout ();
	
	CCL_TEST_ASSERT_EQUAL (view.getWidth (), 120);
	CCL_TEST_ASSERT_EQUAL (view.getHeight (), 120);

	flexAlgorithm->onItemRemoved (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, MixedSizedChildrenWithPartialSizeFillParentAccordingly)
{
	layoutView->setSize (Rect (0, 0, 200, 200));
	flexAlgorithm->onSize ({200, 200});
	
	View view;
	AutoPtr<FlexItem> item = ccl_cast<FlexItem> (flexLayout->createItem (&view));
	
	DesignCoord zeroPercent (DesignCoord::kPercent, 0);
	DesignCoord sixtyPercent (DesignCoord::kPercent, 60);
	item->initialize (DesignSize ({DesignCoord::kCoord, 0}, zeroPercent, {DesignCoord::kCoord, 40}, sixtyPercent));
	
	flexAlgorithm->onItemAdded (item);
	flexAlgorithm->doLayout ();
	
	CCL_TEST_ASSERT_EQUAL (view.getWidth (), 40);
	CCL_TEST_ASSERT_EQUAL (view.getHeight (), 120);

	flexAlgorithm->onItemRemoved (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexAlgorithmTest, RelativeFlexBasisIsConsideredAccordingly)
{
	layoutView->setSize (Rect (0, 0, 200, 200));
	flexAlgorithm->onSize ({200, 200});
	
	View view;
	AutoPtr<FlexItem> item = ccl_cast<FlexItem> (flexLayout->createItem (&view));
	item->setProperty (ATTR_FLEXBASIS, DesignCoord (DesignCoord::kPercent, 80).toVariant ());
	
	flexAlgorithm->onItemAdded (item);
	flexAlgorithm->doLayout ();
	
	CCL_TEST_ASSERT_EQUAL (view.getWidth (), 160);

	flexAlgorithm->onItemRemoved (item);
}

//************************************************************************************************
// FlexCascadesTest
//************************************************************************************************

class FlexCascadesTest: public Test
{
public:
	// Test
	void setUp () override
	{
		flexLayout = LayoutFactory::instance ().createLayout (LAYOUTCLASS_FLEXBOX);
		flexLayout->setProperty (ATTR_FLEXDIRECTION, (int)FlexDirection::kRow);
		
		layoutView = NEW LayoutView (Rect (), 0);
		layoutView->setLayout (flexLayout);
	}

protected:
	AutoPtr<Layout> flexLayout;
	AutoPtr<LayoutView> layoutView;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexCascadesTest, LayoutViewFitsChildrenByDefault)
{
	layoutView->addView (NEW View ({0, 0, 20, 20}));
	layoutView->autoSize (); // Performed by skin view element

	CCL_TEST_ASSERT_EQUAL (layoutView->getWidth (), 20);
	CCL_TEST_ASSERT_EQUAL (layoutView->getHeight (), 20);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexCascadesTest, CascadedLayoutViewFitsChildrenByDefault)
{
	AutoPtr<LayoutView> parentLayoutView = NEW LayoutView (Rect (), 0);
	parentLayoutView->setLayout (flexLayout);

	layoutView->addView (NEW View ({0, 0, 20, 20}));
	layoutView->autoSize (); // Performed by skin view element
	
	parentLayoutView->addView (layoutView.detach ());
	parentLayoutView->autoSize ();
	
	CCL_TEST_ASSERT_EQUAL (parentLayoutView->getWidth (), 20);
	CCL_TEST_ASSERT_EQUAL (parentLayoutView->getHeight (), 20);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexCascadesTest, LayoutChildrenFollowParentIfSizeNotAuto)
{
	layoutView->setSize (Rect (0, 0, 100, 100));
	layoutView->addView (NEW View ({0, 0, 20, 20}));
	
	layoutView->autoSize (); // Performed by skin view element
	
	CCL_TEST_ASSERT_EQUAL (layoutView->getWidth (), 100);
	CCL_TEST_ASSERT_EQUAL (layoutView->getHeight (), 100);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexCascadesTest, CascadedLayoutChildrenFollowParentIfSizeNotAuto)
{
	AutoPtr<LayoutView> parentLayoutView = NEW LayoutView (Rect (0, 0, 100, 100), 0);
	parentLayoutView->setLayout (flexLayout);
	
	layoutView->addView (NEW View ({0, 0, 20, 20}));
	layoutView->autoSize (); // Performed by skin view element
	
	LayoutView* layoutViewPtr = layoutView.detach ();
	
	parentLayoutView->addView (layoutViewPtr);
	parentLayoutView->autoSize ();
	
	// The container must stretch along the cross axis. Since parentLayout is a row, that means the height should fill the parent.
	CCL_TEST_ASSERT_EQUAL (layoutViewPtr->getWidth (), 20);
	CCL_TEST_ASSERT_EQUAL (layoutViewPtr->getHeight (), 100);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexCascadesTest, CascadedLayoutViewsWithFixedSizeKeepFixedSize)
{
	AutoPtr<LayoutView> parentLayoutView = NEW LayoutView (Rect (0, 0, 100, 100), 0);
	parentLayoutView->setLayout (flexLayout);
	
	View* viewPtr = NEW View ({0, 0, 20, 20});
	layoutView->addView (viewPtr);
	layoutView->autoSize (); // Performed by skin view element

	parentLayoutView->addView (layoutView.detach ());
	parentLayoutView->autoSize ();
	
	CCL_TEST_ASSERT_EQUAL (viewPtr->getWidth (), 20);
	CCL_TEST_ASSERT_EQUAL (viewPtr->getHeight (), 20);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexCascadesTest, CascadedLayoutChildrenFollowParentIfSizeIsChanged)
{
	AutoPtr<LayoutView> parentLayoutView = NEW LayoutView (Rect (), 0);
	parentLayoutView->setLayout (flexLayout);
	
	layoutView->addView (NEW View ({0, 0, 20, 20}));
	layoutView->autoSize (); // Performed by skin view element

	LayoutView* layoutViewPtr = layoutView.detach ();
	parentLayoutView->addView (layoutViewPtr);
	
	parentLayoutView->autoSize ();
	parentLayoutView->setSize (Rect (0, 0, 100, 100));
	
	// The container must stretch along the cross axis. Since parentLayout is a row, that means the height should fill the parent.
	CCL_TEST_ASSERT_EQUAL (layoutViewPtr->getWidth (), 20);
	CCL_TEST_ASSERT_EQUAL (layoutViewPtr->getHeight (), 100);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexCascadesTest, LayoutChildWithZeroInsetFillsContainerIfAbsolutePositionType)
{
	layoutView->setSize (Rect (0, 0, 100, 100));
	
	LayoutView* childLayoutViewPtr = NEW LayoutView (Rect (), 0);
	childLayoutViewPtr->setLayout (flexLayout);
	
	layoutView->addView (childLayoutViewPtr);
	FlexItem* item = ccl_cast<FlexItem> (layoutView->findLayoutItem (childLayoutViewPtr));
	item->setProperty (ATTR_FLEXPOSITIONTYPE, int(FlexPositionType::kAbsolute));
	item->setProperty (ATTR_FLEXINSET, "0");
	
	layoutView->autoSize ();
	
	CCL_TEST_ASSERT_EQUAL (childLayoutViewPtr->getPosition ().x, 0);
	CCL_TEST_ASSERT_EQUAL (childLayoutViewPtr->getPosition ().y, 0);
	CCL_TEST_ASSERT_EQUAL (childLayoutViewPtr->getWidth (), 100);
	CCL_TEST_ASSERT_EQUAL (childLayoutViewPtr->getHeight (), 100);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexCascadesTest, CascadedLayoutChildWithZeroInsetFillsContainerIfAbsolutePositionType)
{
	layoutView->setSize (Rect (0, 0, 100, 100));
	
	LayoutView* childLayoutViewPtr = NEW LayoutView (Rect (), 0);
	childLayoutViewPtr->setLayout (flexLayout);
	
	childLayoutViewPtr->addView (NEW View (Rect (0, 0, 20, 20)));
	childLayoutViewPtr->autoSize ();
	
	layoutView->addView (childLayoutViewPtr);
	
	FlexItem* item = ccl_cast<FlexItem> (layoutView->findLayoutItem (childLayoutViewPtr));
	item->setProperty (ATTR_FLEXPOSITIONTYPE, int(FlexPositionType::kAbsolute));
	item->setProperty (ATTR_FLEXINSET, DesignCoord (DesignCoord::kCoord, 0).toVariant ());
	
	layoutView->autoSize ();
	
	CCL_TEST_ASSERT_EQUAL (childLayoutViewPtr->getPosition ().x, 0);
	CCL_TEST_ASSERT_EQUAL (childLayoutViewPtr->getPosition ().y, 0);
	CCL_TEST_ASSERT_EQUAL (childLayoutViewPtr->getWidth (), 100);
	CCL_TEST_ASSERT_EQUAL (childLayoutViewPtr->getHeight (), 100);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexCascadesTest, ChildContainerWithZeroInsetIsResizedWithParent)
{
	layoutView->setSize (Rect (0, 0, 100, 100));
	
	LayoutView* childLayoutViewPtr = NEW LayoutView (Rect (), 0);
	childLayoutViewPtr->setLayout (flexLayout);
	
	childLayoutViewPtr->addView (NEW View (Rect (0, 0, 20, 20)));
	childLayoutViewPtr->autoSize ();
	
	layoutView->addView (childLayoutViewPtr);
	
	FlexItem* item = ccl_cast<FlexItem> (layoutView->findLayoutItem (childLayoutViewPtr));
	item->setProperty (ATTR_FLEXPOSITIONTYPE, int(FlexPositionType::kAbsolute));
	item->setProperty (ATTR_FLEXINSET, "0");
	
	layoutView->setSize (Rect (0, 0, 200, 200));
	layoutView->autoSize ();
	
	CCL_TEST_ASSERT_EQUAL (childLayoutViewPtr->getWidth (), 200);
	CCL_TEST_ASSERT_EQUAL (childLayoutViewPtr->getHeight (), 200);
}

//************************************************************************************************
// FlexDynamicUpdatesTest
//************************************************************************************************

class FlexDynamicUpdatesTest: public Test
{
public:
	// Test
	void setUp () override
	{
		flexLayout = LayoutFactory::instance ().createLayout (LAYOUTCLASS_FLEXBOX);
		flexLayout->setProperty (ATTR_FLEXDIRECTION, (int)FlexDirection::kRow);
		
		layoutView = NEW LayoutView (Rect (), 0);
		layoutView->setLayout (flexLayout);
	}

protected:
	AutoPtr<Layout> flexLayout;
	AutoPtr<LayoutView> layoutView;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexDynamicUpdatesTest, LayoutViewSizesToChildrenDynamically)
{
	View* view = NEW View ({0, 0, 20, 20});
	layoutView->addView (view);
	
	view->setSize ({0, 0, 40, 40});
	
	CCL_TEST_ASSERT_EQUAL (layoutView->getWidth (), 40);
	CCL_TEST_ASSERT_EQUAL (layoutView->getHeight (), 40);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexDynamicUpdatesTest, LayoutViewSizesToChildrenDynamicallyAndMultipleTimes)
{
	View* view = NEW View ({0, 0, 20, 20});
	layoutView->addView (view);
	
	view->setSize ({0, 0, 40, 40});
	view->setSize ({0, 0, 60, 60});
	
	CCL_TEST_ASSERT_EQUAL (layoutView->getWidth (), 60);
	CCL_TEST_ASSERT_EQUAL (layoutView->getHeight (), 60);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (FlexDynamicUpdatesTest, LayoutViewSizesToChildLimitsDynamically)
{
	View* view = NEW View ({0, 0, 20, 20});
	layoutView->addView (view);
	
	// Simulate a view which passes it's size limit changes to it's parent
	view->setSizeLimits (SizeLimit (40, 40, 80, 80));
	layoutView->onChildLimitsChanged (view);
	
	CCL_TEST_ASSERT_EQUAL (layoutView->getWidth (), 40);
	CCL_TEST_ASSERT_EQUAL (layoutView->getHeight (), 40);
}
