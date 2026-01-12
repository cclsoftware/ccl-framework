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
// Filename    : ccl/gui/layout/flexboxlayout.cpp
// Description : Flexbox layout implementation
//
//************************************************************************************************

#include "ccl/gui/layout/flexboxlayout.h"

#include "ccl/base/message.h"

#include "ccl/public/gui/framework/skinxmldefs.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Style definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (FlexboxLayout::flexDirection)
	{"row", 			int(FlexDirection::kRow)},
	{"column", 			int(FlexDirection::kColumn)},
	{"rowreverse", 	int(FlexDirection::kRowReverse)},
	{"columnreverse", 	int(FlexDirection::kColumnReverse)},
END_STYLEDEF

BEGIN_STYLEDEF (FlexboxLayout::flexWrap)
	{"nowrap", 		int(FlexWrap::kNoWrap)},
	{"wrap", 			int(FlexWrap::kWrap)},
	{"wrapreverse", 	int(FlexWrap::kWrapReverse)},
END_STYLEDEF

BEGIN_STYLEDEF (FlexboxLayout::flexJustify)
	{"flexstart", 		int(FlexJustify::kFlexStart)},
	{"flexend", 		int(FlexJustify::kFlexEnd)},
	{"center", 			int(FlexJustify::kCenter)},
	{"spacebetween", 	int(FlexJustify::kSpaceBetween)},
	{"spacearound", 	int(FlexJustify::kSpaceAround)},
	{"spaceevenly", 	int(FlexJustify::kSpaceEvenly)},
END_STYLEDEF

BEGIN_STYLEDEF (FlexboxLayout::flexAlign)
	{"flexstart", 		int(FlexAlign::kFlexStart)},
	{"flexend", 		int(FlexAlign::kFlexEnd)},
	{"center", 			int(FlexAlign::kCenter)},
	{"stretch", 		int(FlexAlign::kStretch)},
END_STYLEDEF

//************************************************************************************************
// EdgeData
//************************************************************************************************

EdgeData& EdgeData::fromString (StringRef string)
{
	if(string.isEmpty ())
		left.unit = top.unit = right.unit = bottom.unit = DesignCoord::kUndefined;

	DesignCoord* edges[4] = {&left, &top, &right, &bottom};
	int count = 0;
	ForEachStringToken (string, ",", token)
		if(count > 3)
			break;
	
		token.trimWhitespace ();
		SkinAttributes::scanDesignCoord (*edges[count++], token);
	EndFor

	// Parse shorthands
	if(count == 1) // Same for all edges
		top = right = bottom = left;
	else if(count == 2) // top/bottom, left/right
	{
		right = left;
		bottom = top;
	}
	
	return *this;
}

//************************************************************************************************
// GutterData
//************************************************************************************************

GutterData& GutterData::fromString (StringRef string)
{
	if(string.isEmpty ())
		row.unit = column.unit = DesignCoord::kUndefined;

	DesignCoord* gutters[2] = {&row, &column};
	int count = 0;
	ForEachStringToken (string, ",", token)
		if(count > 1)
			break;
	
		token.trimWhitespace ();
		SkinAttributes::scanDesignCoord (*gutters[count++], token);
	EndFor

	// Parse shorthands
	if(count == 1)
		column = row;
	
	return *this;
}

//************************************************************************************************
// FlexboxLayout
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (FlexboxLayout, Layout)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FlexboxLayout::setAttributes (const SkinAttributes& a)
{
	flexData.direction = (FlexDirection)a.getOptions (ATTR_FLEXDIRECTION, flexDirection, true, int(FlexDirection::kRow));
	flexData.wrap = (FlexWrap)a.getOptions (ATTR_FLEXWRAP, flexWrap, true, int(FlexWrap::kNoWrap));
	flexData.justify = (FlexJustify)a.getOptions (ATTR_FLEXJUSTIFY, flexJustify, true, int(FlexJustify::kFlexStart));
	flexData.align = (FlexAlign)a.getOptions (ATTR_FLEXALIGN, flexAlign, true, int(FlexAlign::kStretch));

	if(a.exists (ATTR_FLEXPADDING))
		flexData.padding.fromString (a.getString (ATTR_FLEXPADDING));
	a.getDesignCoord (flexData.padding.left, ATTR_FLEXPADDINGLEFT);
	a.getDesignCoord (flexData.padding.top, ATTR_FLEXPADDINGTOP);
	a.getDesignCoord (flexData.padding.right, ATTR_FLEXPADDINGRIGHT);
	a.getDesignCoord (flexData.padding.bottom, ATTR_FLEXPADDINGBOTTOM);

	if(a.exists (ATTR_FLEXGAP))
		flexData.gap.fromString (a.getString (ATTR_FLEXGAP));
	a.getDesignCoord (flexData.gap.row, ATTR_FLEXGAPROW);
	a.getDesignCoord (flexData.gap.column, ATTR_FLEXGAPCOLUMN);
	
	signal (Message (kPropertyChanged));

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FlexboxLayout::getAttributes (SkinAttributes& a) const
{
	a.setOptions (ATTR_FLEXDIRECTION, int(flexData.direction), flexDirection, true);
	a.setOptions (ATTR_FLEXWRAP, int(flexData.wrap), flexWrap, true);
	a.setOptions (ATTR_FLEXJUSTIFY, int(flexData.justify), flexJustify, true);
	a.setOptions (ATTR_FLEXALIGN, int(flexData.align), flexAlign, true);
	
	a.setDesignCoord (ATTR_FLEXPADDINGLEFT, flexData.padding.left);
	a.setDesignCoord (ATTR_FLEXPADDINGTOP, flexData.padding.top);
	a.setDesignCoord (ATTR_FLEXPADDINGRIGHT, flexData.padding.right);
	a.setDesignCoord (ATTR_FLEXPADDINGBOTTOM, flexData.padding.bottom);
	
	a.setDesignCoord (ATTR_FLEXGAPROW, flexData.gap.row);
	a.setDesignCoord (ATTR_FLEXGAPCOLUMN, flexData.gap.column);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutItem* FlexboxLayout::createItem (View* view)
{
	if(view != nullptr)
		return NEW FlexItem (view);
	return NEW FlexItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FlexboxLayout::setProperty (MemberID propertyId, const Variant& var)
{
	bool handled = false;
	auto parseIf = [this, &propertyId, &handled] (MemberID expectedId, auto parseMethod)
	{
		if(handled || (propertyId != expectedId))
			return;
		
		parseMethod ();
		signal (Message (kPropertyChanged));
		handled = true;
	};
	
	parseIf (ATTR_FLEXDIRECTION, 		[&] () { flexData.direction = (FlexDirection)var.asInt (); });
	parseIf (ATTR_FLEXWRAP, 			[&] () { flexData.wrap = (FlexWrap)var.asInt (); });
	parseIf (ATTR_FLEXJUSTIFY, 			[&] () { flexData.justify = (FlexJustify)var.asInt (); });
	parseIf (ATTR_FLEXALIGN, 			[&] () { flexData.align = (FlexAlign)var.asInt (); });
	
	parseIf (ATTR_FLEXPADDING, 			[&] () { flexData.padding.fromString (var.toString ()); });
	parseIf (ATTR_FLEXPADDINGLEFT, 		[&] () { flexData.padding.left.fromVariant (var); });
	parseIf (ATTR_FLEXPADDINGTOP, 		[&] () { flexData.padding.top.fromVariant (var); });
	parseIf (ATTR_FLEXPADDINGRIGHT, 	[&] () { flexData.padding.right.fromVariant (var); });
	parseIf (ATTR_FLEXPADDINGBOTTOM, 	[&] () { flexData.padding.bottom.fromVariant (var); });
	
	parseIf (ATTR_FLEXGAP, 				[&] () { flexData.gap.fromString (var.toString ()); });
	parseIf (ATTR_FLEXGAPROW, 			[&] () { flexData.gap.row.fromVariant (var); });
	parseIf (ATTR_FLEXGAPCOLUMN, 		[&] () { flexData.gap.column.fromVariant (var); });

	return handled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FlexboxLayout::getProperty (Variant& var, MemberID propertyId) const
{
	bool handled = false;
	auto updateVarIf = [&propertyId, &handled] (MemberID expectedId, auto updateMethod)
	{
		if(handled || (propertyId != expectedId))
			return;

		updateMethod ();
		handled = true;
	};
	
	updateVarIf (ATTR_FLEXDIRECTION, 		[&] () { var = int(flexData.direction); });
	updateVarIf (ATTR_FLEXWRAP, 			[&] () { var = int(flexData.wrap); });
	updateVarIf (ATTR_FLEXJUSTIFY, 			[&] () { var = int(flexData.justify); });
	updateVarIf (ATTR_FLEXALIGN, 			[&] () { var = int(flexData.align); });
	
	updateVarIf (ATTR_FLEXPADDINGLEFT, 		[&] () { var = flexData.padding.left.toVariant (); });
	updateVarIf (ATTR_FLEXPADDINGTOP, 		[&] () { var = flexData.padding.top.toVariant (); });
	updateVarIf (ATTR_FLEXPADDINGRIGHT, 	[&] () { var = flexData.padding.right.toVariant (); });
	updateVarIf (ATTR_FLEXPADDINGBOTTOM, 	[&] () { var = flexData.padding.bottom.toVariant (); });

	updateVarIf (ATTR_FLEXGAPROW, 			[&] () { var = flexData.gap.row.toVariant (); });
	updateVarIf (ATTR_FLEXGAPCOLUMN, 		[&] () { var = flexData.gap.column.toVariant (); });

	return handled;
}

//************************************************************************************************
// FlexItem
//************************************************************************************************

DEFINE_CLASS (FlexItem, LayoutItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (FlexItem::flexAlignSelf)
	{"flexstart", 		int(FlexAlignSelf::kFlexStart)},
	{"flexend", 		int(FlexAlignSelf::kFlexEnd)},
	{"center", 			int(FlexAlignSelf::kCenter)},
	{"stretch", 		int(FlexAlignSelf::kStretch)},
	{"auto", 			int(FlexAlignSelf::kAuto)},
END_STYLEDEF

BEGIN_STYLEDEF (FlexItem::flexPositionType)
	{"relative", 		int(FlexPositionType::kRelative)},
	{"absolute", 		int(FlexPositionType::kAbsolute)},
END_STYLEDEF

BEGIN_STYLEDEF (FlexItem::flexSizeMode)
	{"hug", 			int(FlexSizeMode::kHug)},
	{"hughorizontal", 	int(FlexSizeMode::kHugHorizontal)},
	{"hugvertical", 	int(FlexSizeMode::kHugVertical)},
	{"fill", 			int(FlexSizeMode::kFill)},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

FlexItem::FlexItem ()
: LayoutItem ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FlexItem::FlexItem (View* view)
: LayoutItem (view)
{
	flexItemData.width.unit = initialSize.getWidth () > 0 ? DesignCoord::kCoord : DesignCoord::kAuto;
	flexItemData.height.unit = initialSize.getHeight () > 0 ? DesignCoord::kCoord : DesignCoord::kAuto;
	
	flexItemData.width.value = initialSize.getWidth ();
	flexItemData.height.value = initialSize.getHeight ();
	
	updateSizeLimits ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FlexItem::initialize (const DesignSize& designSize)
{
	bool hFit = get_flag<int> (view->getSizeMode (), IView::kHFitSize);
	bool hugHorizontal = flexItemData.sizeMode == FlexSizeMode::kHug || flexItemData.sizeMode == FlexSizeMode::kHugHorizontal;
	if(hugHorizontal || hFit)
	{
		flexItemData.width.value = initialSize.getWidth ();
		flexItemData.width.unit = DesignCoord::kCoord;
	}
	else
		flexItemData.width = designSize.width;
	
	bool vFit = get_flag<int> (view->getSizeMode (), IView::kVFitSize);
	bool hugVertical = flexItemData.sizeMode == FlexSizeMode::kHug || flexItemData.sizeMode == FlexSizeMode::kHugVertical;
	if(hugVertical || vFit)
	{
		flexItemData.height.value = initialSize.getHeight ();
		flexItemData.height.unit = DesignCoord::kCoord;
	}
	else
		flexItemData.height = designSize.height;
	
	signal (Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FlexItemData& FlexItem::getFlexItemData () const
{
	return flexItemData;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FlexItem::setAttributes (const SkinAttributes& a)
{
	flexItemData.grow = a.getFloat (ATTR_FLEXGROW, 0.f);
	flexItemData.shrink = a.getFloat (ATTR_FLEXSHRINK, 1.f);
	
	flexItemData.alignSelf = (FlexAlignSelf)a.getOptions (ATTR_FLEXALIGNSELF, flexAlignSelf, true, int(FlexAlignSelf::kAuto));
	flexItemData.positionType = (FlexPositionType)a.getOptions (ATTR_FLEXPOSITIONTYPE, flexPositionType, true, int(FlexPositionType::kRelative));
	flexItemData.sizeMode = (FlexSizeMode)a.getOptions (ATTR_FLEXSIZEMODE, flexSizeMode, true, int(FlexSizeMode::kFill));
	
	a.getDesignCoord (flexItemData.flexBasis, ATTR_FLEXBASIS);
	
	if(a.exists (ATTR_FLEXMARGIN))
		flexItemData.margin.fromString (a.getString (ATTR_FLEXMARGIN));
	
	a.getDesignCoord (flexItemData.margin.top, ATTR_FLEXMARGINTOP);
	a.getDesignCoord (flexItemData.margin.right, ATTR_FLEXMARGINRIGHT);
	a.getDesignCoord (flexItemData.margin.bottom, ATTR_FLEXMARGINBOTTOM);
	a.getDesignCoord (flexItemData.margin.left, ATTR_FLEXMARGINLEFT);
	
	if(a.exists (ATTR_FLEXINSET))
		flexItemData.inset.fromString (a.getString (ATTR_FLEXINSET));
	
	a.getDesignCoord (flexItemData.inset.top, ATTR_FLEXINSETTOP);
	a.getDesignCoord (flexItemData.inset.right, ATTR_FLEXINSETRIGHT);
	a.getDesignCoord (flexItemData.inset.bottom, ATTR_FLEXINSETBOTTOM);
	a.getDesignCoord (flexItemData.inset.left, ATTR_FLEXINSETLEFT);

	signal (Message (kPropertyChanged));

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FlexItem::getAttributes (SkinAttributes& a) const
{
	a.setFloat (ATTR_FLEXGROW, flexItemData.grow);
	a.setFloat (ATTR_FLEXSHRINK, flexItemData.shrink);
	
	a.setOptions (ATTR_FLEXALIGNSELF, int(flexItemData.alignSelf), flexAlignSelf, true);
	a.setOptions (ATTR_FLEXPOSITIONTYPE, int(flexItemData.positionType), flexPositionType, true);
	a.setOptions (ATTR_FLEXSIZEMODE, int(flexItemData.sizeMode), flexSizeMode, true);

	a.setDesignCoord (ATTR_FLEXBASIS, flexItemData.flexBasis);
	
	a.setDesignCoord (ATTR_FLEXMARGINTOP, flexItemData.margin.top);
	a.setDesignCoord (ATTR_FLEXMARGINRIGHT, flexItemData.margin.right);
	a.setDesignCoord (ATTR_FLEXMARGINBOTTOM, flexItemData.margin.bottom);
	a.setDesignCoord (ATTR_FLEXMARGINLEFT, flexItemData.margin.left);
	
	a.setDesignCoord (ATTR_FLEXINSETTOP, flexItemData.inset.top);
	a.setDesignCoord (ATTR_FLEXINSETRIGHT, flexItemData.inset.right);
	a.setDesignCoord (ATTR_FLEXINSETBOTTOM, flexItemData.inset.bottom);
	a.setDesignCoord (ATTR_FLEXINSETLEFT, flexItemData.inset.left);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FlexItem::setProperty (MemberID propertyId, const Variant& var)
{
	bool handled = false;
	auto parseIf = [this, &propertyId, &handled] (MemberID expectedId, auto parseMethod)
	{
		if(handled || (propertyId != expectedId))
			return;
		
		parseMethod ();
		signal (Message (kPropertyChanged));
		handled = true;
	};
	
	parseIf (ATTR_FLEXGROW, 		[&] () { flexItemData.grow = var.asFloat (); });
	parseIf (ATTR_FLEXSHRINK, 		[&] () { flexItemData.shrink = var.asFloat (); });
	parseIf (ATTR_FLEXALIGNSELF, 	[&] () { flexItemData.alignSelf = (FlexAlignSelf)var.asInt (); });
	parseIf (ATTR_FLEXPOSITIONTYPE, [&] () { flexItemData.positionType = (FlexPositionType)var.asInt (); });
	parseIf (ATTR_FLEXSIZEMODE, 	[&] () { flexItemData.sizeMode = (FlexSizeMode)var.asInt (); });
	
	parseIf (ATTR_FLEXBASIS, 		[&] () { flexItemData.flexBasis.fromVariant (var); });
	
	parseIf (ATTR_FLEXMARGIN, 		[&] () { flexItemData.margin.fromString (var.toString ()); });
	parseIf (ATTR_FLEXMARGINTOP, 	[&] () { flexItemData.margin.top.fromVariant (var); });
	parseIf (ATTR_FLEXMARGINRIGHT, 	[&] () { flexItemData.margin.right.fromVariant (var); });
	parseIf (ATTR_FLEXMARGINBOTTOM, [&] () { flexItemData.margin.bottom.fromVariant (var); });
	parseIf (ATTR_FLEXMARGINLEFT, 	[&] () { flexItemData.margin.left.fromVariant (var); });
	
	parseIf (ATTR_FLEXINSET, 		[&] () { flexItemData.inset.fromString (var.toString ()); });
	parseIf (ATTR_FLEXINSETTOP, 	[&] () { flexItemData.inset.top.fromVariant (var); });
	parseIf (ATTR_FLEXINSETRIGHT, 	[&] () { flexItemData.inset.right.fromVariant (var); });
	parseIf (ATTR_FLEXINSETBOTTOM, 	[&] () { flexItemData.inset.bottom.fromVariant (var); });
	parseIf (ATTR_FLEXINSETLEFT, 	[&] () { flexItemData.inset.left.fromVariant (var); });
	
	return handled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FlexItem::getProperty (Variant& var, MemberID propertyId) const
{
	bool handled = false;
	auto updateVarIf = [&propertyId, &handled] (MemberID expectedId, auto updateMethod)
	{
		if(handled || (propertyId != expectedId))
			return;

		updateMethod ();
		handled = true;
	};

	updateVarIf (ATTR_FLEXGROW, 		[&] () { var = flexItemData.grow; });
	updateVarIf (ATTR_FLEXSHRINK, 		[&] () { var = flexItemData.shrink; });
	updateVarIf (ATTR_FLEXALIGNSELF, 	[&] () { var = int(flexItemData.alignSelf); });
	updateVarIf (ATTR_FLEXPOSITIONTYPE, [&] () { var = int(flexItemData.positionType); });
	updateVarIf (ATTR_FLEXSIZEMODE, 	[&] () { var = int(flexItemData.sizeMode); });
	updateVarIf (ATTR_FLEXBASIS, 		[&] () { var = flexItemData.flexBasis.toVariant (); });
	
	updateVarIf (ATTR_FLEXMARGINTOP, 	[&] () { var = flexItemData.margin.top.toVariant (); });
	updateVarIf (ATTR_FLEXMARGINRIGHT, 	[&] () { var = flexItemData.margin.right.toVariant (); });
	updateVarIf (ATTR_FLEXMARGINBOTTOM, [&] () { var = flexItemData.margin.bottom.toVariant (); });
	updateVarIf (ATTR_FLEXMARGINLEFT, 	[&] () { var = flexItemData.margin.left.toVariant (); });
	
	updateVarIf (ATTR_FLEXINSETTOP, 	[&] () { var = flexItemData.inset.top.toVariant (); });
	updateVarIf (ATTR_FLEXINSETRIGHT, 	[&] () { var = flexItemData.inset.right.toVariant (); });
	updateVarIf (ATTR_FLEXINSETBOTTOM, 	[&] () { var = flexItemData.inset.bottom.toVariant (); });
	updateVarIf (ATTR_FLEXINSETLEFT, 	[&] () { var = flexItemData.inset.left.toVariant (); });

	return handled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FlexItem::updateSizeLimits ()
{
	if(view == nullptr)
		return;
	
	const SizeLimit& limits = view->getSizeLimits ();
	
	flexItemData.minWidth.unit = DesignCoord::kCoord;
	flexItemData.minHeight.unit = DesignCoord::kCoord;
	flexItemData.maxWidth.unit = DesignCoord::kCoord;
	flexItemData.maxHeight.unit = DesignCoord::kCoord;

	flexItemData.minWidth.value = limits.minWidth;
	flexItemData.minHeight.value = limits.minHeight;
	flexItemData.maxWidth.value = limits.maxWidth;
	flexItemData.maxHeight.value = limits.maxHeight;
}
	
