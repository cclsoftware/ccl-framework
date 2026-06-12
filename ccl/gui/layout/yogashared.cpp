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
// Filename    : ccl/gui/layout/yogashared.cpp
// Description : Flexbox layout implementation with Facebook's Yoga library
//
//************************************************************************************************

#include "ccl/gui/layout/yogashared.h"

#include "ccl/public/collections/map.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Enum Conversions
//////////////////////////////////////////////////////////////////////////////////////////////////

static KeyValue<FlexDirection, YGFlexDirection> flexDirectionMapData[] =
{
	{FlexDirection::kRow, 			YGFlexDirectionRow},
	{FlexDirection::kColumn, 		YGFlexDirectionColumn},
	{FlexDirection::kRowReverse, 	YGFlexDirectionRowReverse},
	{FlexDirection::kColumnReverse, YGFlexDirectionColumnReverse}
};

static KeyValue<FlexWrap, YGWrap> flexWrapMapData[] =
{
	{FlexWrap::kNoWrap, 			YGWrapNoWrap},
	{FlexWrap::kWrap, 				YGWrapWrap},
	{FlexWrap::kWrapReverse, 		YGWrapWrapReverse}
};

static KeyValue<FlexJustify, YGJustify> flexJustifyMapData[] =
{
	{FlexJustify::kFlexStart, 		YGJustifyFlexStart},
	{FlexJustify::kFlexEnd, 		YGJustifyFlexEnd},
	{FlexJustify::kCenter, 			YGJustifyCenter},
	{FlexJustify::kSpaceBetween, 	YGJustifySpaceBetween},
	{FlexJustify::kSpaceAround, 	YGJustifySpaceAround},
	{FlexJustify::kSpaceEvenly, 	YGJustifySpaceEvenly}
};

static KeyValue<FlexAlign, YGAlign> flexAlignMapData[] =
{
	{FlexAlign::kFlexStart, 		YGAlignFlexStart},
	{FlexAlign::kFlexEnd, 			YGAlignFlexEnd},
	{FlexAlign::kCenter, 			YGAlignCenter},
	{FlexAlign::kStretch, 			YGAlignStretch}
};

static KeyValue<FlexAlignSelf, YGAlign> flexAlignSelfMapData[] =
{
	{FlexAlignSelf::kAuto, 			YGAlignAuto},
	{FlexAlignSelf::kFlexStart, 	YGAlignFlexStart},
	{FlexAlignSelf::kFlexEnd, 		YGAlignFlexEnd},
	{FlexAlignSelf::kCenter, 		YGAlignCenter},
	{FlexAlignSelf::kStretch, 		YGAlignStretch}
};

static KeyValue<FlexPositionType, YGPositionType> flexPositionTypeMapData[] =
{
	{FlexPositionType::kRelative,	YGPositionTypeRelative},
	{FlexPositionType::kAbsolute,	YGPositionTypeAbsolute}
};

static ConstMap<FlexDirection, YGFlexDirection> flexDirectionMap (flexDirectionMapData, ARRAY_COUNT (flexDirectionMapData));
static ConstMap<FlexWrap, YGWrap> flexWrapMap (flexWrapMapData, ARRAY_COUNT (flexWrapMapData));
static ConstMap<FlexJustify, YGJustify> flexJustifyMap (flexJustifyMapData, ARRAY_COUNT (flexJustifyMapData));
static ConstMap<FlexAlign, YGAlign> flexAlignMap (flexAlignMapData, ARRAY_COUNT (flexAlignMapData));
static ConstMap<FlexAlignSelf, YGAlign> flexAlignSelfMap (flexAlignSelfMapData, ARRAY_COUNT (flexAlignSelfMapData));
static ConstMap<FlexPositionType, YGPositionType> flexPositionTypeMap (flexPositionTypeMapData, ARRAY_COUNT (flexPositionTypeMapData));

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// FlexNodeFactory
//************************************************************************************************

FlexNode* FlexNodeFactory::createNode ()
{
	return NEW YogaNode;
}

//************************************************************************************************
// YogaNode
//************************************************************************************************

YogaNode::YogaNode ()
: node (YGNodeNew ())
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaNode::~YogaNode ()
{
	YGNodeFree (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

YogaNode::operator YGNodeRef () const
{
	return node;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaNode::applyContainerData (const FlexContainerData& flexData, const FlexItemData& flexItemData)
{
	YGNodeStyleSetFlexDirection (node, flexDirectionMap [flexData.direction]);
	YGNodeStyleSetFlexWrap (node, flexWrapMap [flexData.wrap]);
	YGNodeStyleSetJustifyContent (node, flexJustifyMap [flexData.justify]);
	YGNodeStyleSetAlignItems (node, flexAlignMap [flexData.align]);
	
	// Padding
	auto setYGNodeStylePadding = [this] (DesignCoordRef flexCoord, YGEdge edge)
	{
		if(flexCoord.isCoord ())
			YGNodeStyleSetPadding (node, edge, flexCoord.value);
	};
	
	setYGNodeStylePadding (flexData.padding.left, YGEdgeLeft);
	setYGNodeStylePadding (flexData.padding.top, YGEdgeTop);
	setYGNodeStylePadding (flexData.padding.right, YGEdgeRight);
	setYGNodeStylePadding (flexData.padding.bottom, YGEdgeBottom);
	
	// Gap
	auto setYGNodeStyleGap = [this] (DesignCoordRef flexCoord, YGGutter gutter)
	{
		if(flexCoord.isCoord ())
			YGNodeStyleSetGap (node, gutter, flexCoord.value);
	};
	
	setYGNodeStyleGap (flexData.gap.row, YGGutterRow);
	setYGNodeStyleGap (flexData.gap.column, YGGutterColumn);
	
	applyNodeWidth (flexItemData);
	applyNodeHeight (flexItemData);
	
	YGNodeSetHasNewLayout (node, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaNode::applyItemData (const FlexItemData& flexItemData)
{
	YGNodeStyleSetFlexGrow (node, flexItemData.grow);
	YGNodeStyleSetFlexShrink (node, flexItemData.shrink);
	
	YGNodeStyleSetAlignSelf (node, flexAlignSelfMap [flexItemData.alignSelf]);
	YGNodeStyleSetPositionType (node, flexPositionTypeMap [flexItemData.positionType]);
	
	if(flexItemData.flexBasis.isAuto ())
		YGNodeStyleSetFlexBasisAuto (node);
	else if(flexItemData.flexBasis.isPercent ())
		YGNodeStyleSetFlexBasisPercent (node, flexItemData.flexBasis.value);
	else if(flexItemData.flexBasis.isCoord ())
		YGNodeStyleSetFlexBasis (node, flexItemData.flexBasis.value);
	
	auto setYGNodeStyleMargin = [this] (DesignCoordRef flexCoord, YGEdge edge)
	{
		if(flexCoord.isAuto ())
			YGNodeStyleSetMarginAuto (node, edge);
		else if(flexCoord.isCoord ())
			YGNodeStyleSetMargin (node, edge, flexCoord.value);
	};
	
	setYGNodeStyleMargin (flexItemData.margin.top, YGEdgeTop);
	setYGNodeStyleMargin (flexItemData.margin.right, YGEdgeRight);
	setYGNodeStyleMargin (flexItemData.margin.bottom, YGEdgeBottom);
	setYGNodeStyleMargin (flexItemData.margin.left, YGEdgeLeft);
	
	auto setYGNodeStyleInset = [this] (DesignCoordRef flexCoord, YGEdge edge)
	{
		if(flexCoord.isCoord ())
			YGNodeStyleSetPosition (node, edge, flexCoord.value);
	};
	
	setYGNodeStyleInset (flexItemData.inset.top, YGEdgeTop);
	setYGNodeStyleInset (flexItemData.inset.right, YGEdgeRight);
	setYGNodeStyleInset (flexItemData.inset.bottom, YGEdgeBottom);
	setYGNodeStyleInset (flexItemData.inset.left, YGEdgeLeft);
	
	applySizeLimits (flexItemData);
	applyNodeWidth (flexItemData);
	applyNodeHeight (flexItemData);
	
	YGNodeSetHasNewLayout (node, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaNode::applySizeLimits (const FlexItemData& flexItemData)
{
	if(flexItemData.minWidth.isCoord ())
		YGNodeStyleSetMinWidth (node, flexItemData.minWidth.value);
	if(flexItemData.minHeight.isCoord ())
		YGNodeStyleSetMinHeight (node, flexItemData.minHeight.value);
	if(flexItemData.maxWidth.isCoord ())
		YGNodeStyleSetMaxWidth (node, flexItemData.maxWidth.value);
	if(flexItemData.maxWidth.isCoord ())
		YGNodeStyleSetMaxHeight (node, flexItemData.maxHeight.value);
		
	YGNodeSetHasNewLayout (node, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaNode::applyNodeWidth (const FlexItemData& flexItemData)
{
	if(flexItemData.width.isAuto ())
		YGNodeStyleSetWidthAuto (node);
	else if (flexItemData.width.isPercent ())
		YGNodeStyleSetWidthPercent (node, flexItemData.width.value);
	else
		YGNodeStyleSetWidth (node, flexItemData.width.value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaNode::applyNodeHeight (const FlexItemData& flexItemData)
{
	if(flexItemData.height.isAuto ())
		YGNodeStyleSetHeightAuto (node);
	else if (flexItemData.height.isPercent ())
		YGNodeStyleSetHeightPercent (node, flexItemData.height.value);
	else
		YGNodeStyleSetHeight (node, flexItemData.height.value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaNode::applySize (CoordF width, CoordF height)
{
	if(width <= 0.f)
		YGNodeStyleSetWidthAuto (node);
	else
		YGNodeStyleSetWidth (node, width);
	
	if(height <= 0.f)
		YGNodeStyleSetHeightAuto (node);
	else
		YGNodeStyleSetHeight (node, height);
	
	YGNodeSetHasNewLayout (node, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaNode::calculateLayout (const CoordF* availableWidth, const CoordF* availableHeight)
{
	YGNodeCalculateLayout (node,
						   availableWidth ? *availableWidth : YGUndefined, 
						   availableHeight ? *availableHeight : YGUndefined,
						   YGDirectionLTR);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool YogaNode::hasNewLayout () const
{
	return YGNodeGetHasNewLayout (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void YogaNode::setHasNewLayout (bool state)
{
	YGNodeSetHasNewLayout (node, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PointF YogaNode::getLayoutPosition () const
{
	// Yoga's YGNodeLayoutGet<Left, Right...> are distances to the corresponding parent edge
	CoordF left = YGNodeLayoutGetLeft (node);
	CoordF top = YGNodeLayoutGetTop (node);
	return {left, top};
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CoordF YogaNode::getLayoutWidth () const
{
	return YGNodeLayoutGetWidth (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CoordF YogaNode::getLayoutHeight () const
{
	return YGNodeLayoutGetHeight (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool YogaNode::insertNode (int index, FlexNode* _child)
{	
	auto* child = static_cast<YogaNode*> (_child);
	ASSERT (child != nullptr)
	if(child == nullptr)
		return false;

	size_t childCount = YGNodeGetChildCount (node);
	bool indexIsValid = index >= 0 && index <= int(childCount);
	
	ASSERT (indexIsValid)
	if(!indexIsValid)
		return false;

	YGNodeInsertChild (node, *child, index);
	
	YGNodeSetHasNewLayout (node, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool YogaNode::removeNode (FlexNode* _child)
{
	auto* child = static_cast<YogaNode*> (_child);
	ASSERT (child != nullptr)
	if(child == nullptr)
		return false;

	YGNodeRemoveChild (node, *child);
	
	YGNodeSetHasNewLayout (node, true);
	return true;
}
