//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/gui/layout/flexboxshared.cpp
// Description : Flexbox shared implementation
//
//************************************************************************************************

#include "ccl/gui/layout/flexboxshared.h"

#include "ccl/gui/skin/skinattributes.h"
#include "ccl/gui/theme/visualstyle.h"

#include "ccl/public/gui/framework/skinxmldefs.h"

using namespace CCL;

//************************************************************************************************
// FlexShared
//************************************************************************************************

BEGIN_STYLEDEF (FlexShared::flexDirection)
	{"row",				int(FlexDirection::kRow)},
	{"column",			int(FlexDirection::kColumn)},
	{"rowreverse",		int(FlexDirection::kRowReverse)},
	{"columnreverse",	int(FlexDirection::kColumnReverse)},
END_STYLEDEF

BEGIN_STYLEDEF (FlexShared::flexWrap)
	{"nowrap",			int(FlexWrap::kNoWrap)},
	{"wrap", 			int(FlexWrap::kWrap)},
	{"wrapreverse", 	int(FlexWrap::kWrapReverse)},
END_STYLEDEF

BEGIN_STYLEDEF (FlexShared::flexJustify)
	{"flexstart", 		int(FlexJustify::kFlexStart)},
	{"flexend", 		int(FlexJustify::kFlexEnd)},
	{"center", 			int(FlexJustify::kCenter)},
	{"spacebetween", 	int(FlexJustify::kSpaceBetween)},
	{"spacearound", 	int(FlexJustify::kSpaceAround)},
	{"spaceevenly", 	int(FlexJustify::kSpaceEvenly)},
END_STYLEDEF

BEGIN_STYLEDEF (FlexShared::flexAlign)
	{"flexstart", 		int(FlexAlign::kFlexStart)},
	{"flexend", 		int(FlexAlign::kFlexEnd)},
	{"center", 			int(FlexAlign::kCenter)},
	{"stretch", 		int(FlexAlign::kStretch)},
END_STYLEDEF

BEGIN_STYLEDEF (FlexShared::flexAlignSelf)
	{"flexstart", 		int(FlexAlignSelf::kFlexStart)},
	{"flexend", 		int(FlexAlignSelf::kFlexEnd)},
	{"center", 			int(FlexAlignSelf::kCenter)},
	{"stretch", 		int(FlexAlignSelf::kStretch)},
	{"auto", 			int(FlexAlignSelf::kAuto)},
END_STYLEDEF

BEGIN_STYLEDEF (FlexShared::flexPositionType)
	{"relative", 		int(FlexPositionType::kRelative)},
	{"absolute", 		int(FlexPositionType::kAbsolute)},
END_STYLEDEF

BEGIN_STYLEDEF (FlexShared::flexSizeMode)
	{"hug", 			int(FlexSizeMode::kHug)},
	{"hughorizontal", 	int(FlexSizeMode::kHugHorizontal)},
	{"hugvertical", 	int(FlexSizeMode::kHugVertical)},
	{"fill", 			int(FlexSizeMode::kFill)},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

void FlexShared::scan (FlexEdgeData& result, StringRef string)
{
	if(string.isEmpty ())
		result.left.unit = result.top.unit = result.right.unit = result.bottom.unit = DesignCoord::kUndefined;

	DesignCoord* edges[4] = {&result.left, &result.top, &result.right, &result.bottom};
	int count = 0;
	ForEachStringToken (string, ",", token)
		if(count > 3)
			break;
	
		token.trimWhitespace ();
		SkinAttributes::scanDesignCoord (*edges[count++], token);
	EndFor

	// Parse shorthands
	if(count == 1) // Same for all edges
		result.top = result.right = result.bottom = result.left;
	else if(count == 2) // top/bottom, left/right
	{
		result.right = result.left;
		result.bottom = result.top;
	}
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void FlexShared::scan (FlexGutterData& result, StringRef string)
{
	if(string.isEmpty ())
		result.row.unit = result.column.unit = DesignCoord::kUndefined;

	DesignCoord* gutters[2] = {&result.row, &result.column};
	int count = 0;
	ForEachStringToken (string, ",", token)
		if(count > 1)
			break;
	
		token.trimWhitespace ();
		SkinAttributes::scanDesignCoord (*gutters[count++], token);
	EndFor

	// Parse shorthands
	if(count == 1)
		result.column = result.row;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FlexShared::setAttributes (FlexContainerData& flexData, const SkinAttributes& a)
{
	flexData.direction = (FlexDirection)a.getOptions (ATTR_FLEXDIRECTION, FlexShared::flexDirection, true, int(FlexDirection::kRow));
	flexData.wrap = (FlexWrap)a.getOptions (ATTR_FLEXWRAP, FlexShared::flexWrap, true, int(FlexWrap::kNoWrap));
	flexData.justify = (FlexJustify)a.getOptions (ATTR_FLEXJUSTIFY, FlexShared::flexJustify, true, int(FlexJustify::kFlexStart));
	flexData.align = (FlexAlign)a.getOptions (ATTR_FLEXALIGN, FlexShared::flexAlign, true, int(FlexAlign::kStretch));

	if(a.exists (ATTR_FLEXPADDING))
		FlexShared::scan (flexData.padding, a.getString (ATTR_FLEXPADDING));

	a.getDesignCoord (flexData.padding.left, ATTR_FLEXPADDINGLEFT);
	a.getDesignCoord (flexData.padding.top, ATTR_FLEXPADDINGTOP);
	a.getDesignCoord (flexData.padding.right, ATTR_FLEXPADDINGRIGHT);
	a.getDesignCoord (flexData.padding.bottom, ATTR_FLEXPADDINGBOTTOM);

	if(a.exists (ATTR_FLEXGAP))
		FlexShared::scan (flexData.gap, a.getString (ATTR_FLEXGAP));

	a.getDesignCoord (flexData.gap.row, ATTR_FLEXGAPROW);
	a.getDesignCoord (flexData.gap.column, ATTR_FLEXGAPCOLUMN);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FlexShared::setAttributes (FlexContainerData& flexData, const IVisualStyleData& data,
								const FlexContainerData* flexDefault)
{
	flexData.direction = (FlexDirection)data.getOptions (ATTR_FLEXDIRECTION, int(flexDefault ? flexDefault->direction : FlexDirection::kRow));
	flexData.wrap = (FlexWrap)data.getOptions (ATTR_FLEXWRAP, int(flexDefault ? flexDefault->wrap : FlexWrap::kNoWrap));
	flexData.justify = (FlexJustify)data.getOptions (ATTR_FLEXJUSTIFY, int(flexDefault ? flexDefault->justify : FlexJustify::kFlexStart));
	flexData.align = (FlexAlign)data.getOptions (ATTR_FLEXALIGN, int(flexDefault ? flexDefault->align : FlexAlign::kStretch));

	if(flexDefault)
		flexData.padding = flexDefault->padding;
		
	String paddingString (data.getString (ATTR_FLEXPADDING));
	if(!paddingString.isEmpty ())
		FlexShared::scan (flexData.padding, paddingString);

	data.getDesignMetric (flexData.padding.left, ATTR_FLEXPADDINGLEFT);
	data.getDesignMetric (flexData.padding.top, ATTR_FLEXPADDINGTOP);
	data.getDesignMetric (flexData.padding.right, ATTR_FLEXPADDINGRIGHT);
	data.getDesignMetric (flexData.padding.bottom, ATTR_FLEXPADDINGBOTTOM);

	if(flexDefault)
		flexData.gap = flexDefault->gap;

	String gapString (data.getString (ATTR_FLEXGAP));
	if(!gapString.isEmpty ())
		FlexShared::scan (flexData.gap, gapString);

	data.getDesignMetric (flexData.gap.row, ATTR_FLEXGAPROW);
	data.getDesignMetric (flexData.gap.column, ATTR_FLEXGAPCOLUMN);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FlexShared::getAttributes (SkinAttributes& a, const FlexContainerData& flexData)
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
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FlexShared::setProperty (FlexContainerData& flexData, StringID propertyId, VariantRef var)
{
	bool handled = false;
	auto parseIf = [&propertyId, &handled] (StringID expectedId, auto parseMethod)
	{
		if(handled || (propertyId != expectedId))
			return;
		
		parseMethod ();
		handled = true;
	};
	
	parseIf (ATTR_FLEXDIRECTION, 		[&] () { flexData.direction = (FlexDirection)var.asInt (); });
	parseIf (ATTR_FLEXWRAP, 			[&] () { flexData.wrap = (FlexWrap)var.asInt (); });
	parseIf (ATTR_FLEXJUSTIFY, 			[&] () { flexData.justify = (FlexJustify)var.asInt (); });
	parseIf (ATTR_FLEXALIGN, 			[&] () { flexData.align = (FlexAlign)var.asInt (); });
	
	parseIf (ATTR_FLEXPADDING, 			[&] () { scan (flexData.padding, var.toString ()); });
	parseIf (ATTR_FLEXPADDINGLEFT, 		[&] () { flexData.padding.left.fromVariant (var); });
	parseIf (ATTR_FLEXPADDINGTOP, 		[&] () { flexData.padding.top.fromVariant (var); });
	parseIf (ATTR_FLEXPADDINGRIGHT, 	[&] () { flexData.padding.right.fromVariant (var); });
	parseIf (ATTR_FLEXPADDINGBOTTOM, 	[&] () { flexData.padding.bottom.fromVariant (var); });
	
	parseIf (ATTR_FLEXGAP, 				[&] () { scan (flexData.gap, var.toString ()); });
	parseIf (ATTR_FLEXGAPROW, 			[&] () { flexData.gap.row.fromVariant (var); });
	parseIf (ATTR_FLEXGAPCOLUMN, 		[&] () { flexData.gap.column.fromVariant (var); });

	return handled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FlexShared::getProperty (Variant& var, const FlexContainerData& flexData, StringID propertyId)
{
	bool handled = false;
	auto updateVarIf = [&propertyId, &handled] (StringID expectedId, auto updateMethod)
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

//////////////////////////////////////////////////////////////////////////////////////////////////

void FlexShared::setAttributes (FlexItemData& flexItemData, const SkinAttributes& a)
{
	flexItemData.grow = a.getFloat (ATTR_FLEXGROW, 0.f);
	flexItemData.shrink = a.getFloat (ATTR_FLEXSHRINK, 1.f);
	
	flexItemData.alignSelf = (FlexAlignSelf)a.getOptions (ATTR_FLEXALIGNSELF, flexAlignSelf, true, int(FlexAlignSelf::kAuto));
	flexItemData.positionType = (FlexPositionType)a.getOptions (ATTR_FLEXPOSITIONTYPE, flexPositionType, true, int(FlexPositionType::kRelative));
	flexItemData.sizeMode = (FlexSizeMode)a.getOptions (ATTR_FLEXSIZEMODE, flexSizeMode, true, int(FlexSizeMode::kFill));
	
	a.getDesignCoord (flexItemData.flexBasis, ATTR_FLEXBASIS);
	
	if(a.exists (ATTR_FLEXMARGIN))
		 FlexShared::scan (flexItemData.margin, a.getString (ATTR_FLEXMARGIN));
	
	a.getDesignCoord (flexItemData.margin.top, ATTR_FLEXMARGINTOP);
	a.getDesignCoord (flexItemData.margin.right, ATTR_FLEXMARGINRIGHT);
	a.getDesignCoord (flexItemData.margin.bottom, ATTR_FLEXMARGINBOTTOM);
	a.getDesignCoord (flexItemData.margin.left, ATTR_FLEXMARGINLEFT);
	
	if(a.exists (ATTR_FLEXINSET))
		FlexShared::scan (flexItemData.inset, a.getString (ATTR_FLEXINSET));
	
	a.getDesignCoord (flexItemData.inset.top, ATTR_FLEXINSETTOP);
	a.getDesignCoord (flexItemData.inset.right, ATTR_FLEXINSETRIGHT);
	a.getDesignCoord (flexItemData.inset.bottom, ATTR_FLEXINSETBOTTOM);
	a.getDesignCoord (flexItemData.inset.left, ATTR_FLEXINSETLEFT);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FlexShared::setAttributes (FlexItemData& flexItemData, const IVisualStyleData& data)
{
	flexItemData.grow = data.getMetric (ATTR_FLEXGROW, 0.f);
	flexItemData.shrink = data.getMetric (ATTR_FLEXSHRINK, 1.f);

	flexItemData.alignSelf = (FlexAlignSelf)data.getOptions (ATTR_FLEXALIGNSELF, int(FlexAlignSelf::kAuto));
	flexItemData.positionType = (FlexPositionType)data.getOptions (ATTR_FLEXPOSITIONTYPE, int(FlexPositionType::kRelative));
	flexItemData.sizeMode = (FlexSizeMode)data.getOptions (ATTR_FLEXSIZEMODE, int(FlexSizeMode::kFill));

	flexItemData.flexBasis = data.getDesignMetric (ATTR_FLEXBASIS, DesignCoord::kAuto);

	String marginString (data.getString (ATTR_FLEXMARGIN));
	if(!marginString.isEmpty ())
		 FlexShared::scan (flexItemData.margin, marginString);

	data.getDesignMetric (flexItemData.margin.top, ATTR_FLEXMARGINTOP);
	data.getDesignMetric (flexItemData.margin.right, ATTR_FLEXMARGINRIGHT);
	data.getDesignMetric (flexItemData.margin.bottom, ATTR_FLEXMARGINBOTTOM);
	data.getDesignMetric (flexItemData.margin.left, ATTR_FLEXMARGINLEFT);

	String insetString (data.getString (ATTR_FLEXINSET));
	if(!insetString.isEmpty ())
		 FlexShared::scan (flexItemData.inset, insetString);

	data.getDesignMetric (flexItemData.inset.top, ATTR_FLEXINSETTOP);
	data.getDesignMetric (flexItemData.inset.right, ATTR_FLEXINSETRIGHT);
	data.getDesignMetric (flexItemData.inset.bottom, ATTR_FLEXINSETBOTTOM);
	data.getDesignMetric (flexItemData.inset.left, ATTR_FLEXINSETLEFT);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FlexShared::getAttributes (SkinAttributes& a, const FlexItemData& flexItemData)
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
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FlexShared::setProperty (FlexItemData& flexItemData, StringID propertyId, VariantRef var)
{
	bool handled = false;
	auto parseIf = [&propertyId, &handled] (StringID expectedId, auto parseMethod)
	{
		if(handled || (propertyId != expectedId))
			return;
		
		parseMethod ();
		handled = true;
	};
	
	parseIf (ATTR_FLEXGROW, 		[&] () { flexItemData.grow = var.asFloat (); });
	parseIf (ATTR_FLEXSHRINK, 		[&] () { flexItemData.shrink = var.asFloat (); });
	parseIf (ATTR_FLEXALIGNSELF, 	[&] () { flexItemData.alignSelf = (FlexAlignSelf)var.asInt (); });
	parseIf (ATTR_FLEXPOSITIONTYPE, [&] () { flexItemData.positionType = (FlexPositionType)var.asInt (); });
	parseIf (ATTR_FLEXSIZEMODE, 	[&] () { flexItemData.sizeMode = (FlexSizeMode)var.asInt (); });
	
	parseIf (ATTR_FLEXBASIS, 		[&] () { flexItemData.flexBasis.fromVariant (var); });
	
	parseIf (ATTR_FLEXMARGIN, 		[&] () { scan (flexItemData.margin, var.toString ()); });
	parseIf (ATTR_FLEXMARGINTOP, 	[&] () { flexItemData.margin.top.fromVariant (var); });
	parseIf (ATTR_FLEXMARGINRIGHT, 	[&] () { flexItemData.margin.right.fromVariant (var); });
	parseIf (ATTR_FLEXMARGINBOTTOM, [&] () { flexItemData.margin.bottom.fromVariant (var); });
	parseIf (ATTR_FLEXMARGINLEFT, 	[&] () { flexItemData.margin.left.fromVariant (var); });
	
	parseIf (ATTR_FLEXINSET, 		[&] () { scan (flexItemData.inset, var.toString ()); });
	parseIf (ATTR_FLEXINSETTOP, 	[&] () { flexItemData.inset.top.fromVariant (var); });
	parseIf (ATTR_FLEXINSETRIGHT, 	[&] () { flexItemData.inset.right.fromVariant (var); });
	parseIf (ATTR_FLEXINSETBOTTOM, 	[&] () { flexItemData.inset.bottom.fromVariant (var); });
	parseIf (ATTR_FLEXINSETLEFT, 	[&] () { flexItemData.inset.left.fromVariant (var); });
	
	return handled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FlexShared::getProperty (Variant& var, const FlexItemData& flexItemData, StringID propertyId)
{
	bool handled = false;
	auto updateVarIf = [&propertyId, &handled] (StringID expectedId, auto updateMethod)
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