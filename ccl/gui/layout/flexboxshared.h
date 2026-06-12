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
// Filename    : ccl/gui/layout/flexboxshared.h
// Description : Flexbox shared implementation
//
//************************************************************************************************

#ifndef _ccl_flexboxshared_h
#define _ccl_flexboxshared_h

#include "ccl/public/gui/framework/designsize.h"
#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

class SkinAttributes;
interface IVisualStyleData;

//************************************************************************************************
// Flexbox Enumerations
//************************************************************************************************

enum class FlexDirection: int
{
	kRow,
	kRowReverse,
	kColumn,
	kColumnReverse
};

enum class FlexWrap: int
{
	kNoWrap,
	kWrap,
	kWrapReverse
};

enum class FlexJustify: int
{
	kFlexStart,
	kFlexEnd,
	kCenter,
	kSpaceBetween,
	kSpaceAround,
	kSpaceEvenly
};

enum class FlexAlign: int
{
	kFlexStart,
	kFlexEnd,
	kCenter,
	kStretch
};

enum class FlexAlignSelf: int
{
	kFlexStart,
	kFlexEnd,
	kCenter,
	kStretch,
	kAuto
};

enum class FlexPositionType: int
{
	kRelative,
	kAbsolute
};

enum class FlexSizeMode: int
{
	kHug,
	kHugHorizontal,
	kHugVertical,
	kFill
};

//************************************************************************************************
// FlexEdgeData
//************************************************************************************************

struct FlexEdgeData
{
	DesignCoord left = DesignCoord::kUndefined;
	DesignCoord top = DesignCoord::kUndefined;
	DesignCoord right = DesignCoord::kUndefined;
	DesignCoord bottom = DesignCoord::kUndefined;
};

//************************************************************************************************
// FlexGutterData
//************************************************************************************************

struct FlexGutterData
{
	DesignCoord row = DesignCoord::kUndefined;
	DesignCoord column = DesignCoord::kUndefined;
};

//************************************************************************************************
// FlexContainerData
//************************************************************************************************

struct FlexContainerData
{
	FlexDirection direction = FlexDirection::kRow;
	FlexWrap wrap = FlexWrap::kNoWrap;
	FlexJustify justify = FlexJustify::kFlexStart;
	FlexAlign align = FlexAlign::kStretch;

	FlexEdgeData padding;
	FlexGutterData gap;
};

//************************************************************************************************
// FlexItemData
//************************************************************************************************

struct FlexItemData
{
	DesignCoord width;
	DesignCoord height;
	
	DesignCoord minWidth = DesignCoord::kUndefined;
	DesignCoord minHeight = DesignCoord::kUndefined;
	DesignCoord maxWidth = DesignCoord::kUndefined;
	DesignCoord maxHeight = DesignCoord::kUndefined;
	
	float grow = 0.f;
	float shrink = 1.f;
	DesignCoord flexBasis; ///< size flex grow or shrink properties are applied to in relation to other items
	FlexAlignSelf alignSelf = FlexAlignSelf::kAuto;
	FlexPositionType positionType = FlexPositionType::kRelative;
	FlexSizeMode sizeMode = FlexSizeMode::kFill;
	
	FlexEdgeData margin;
	FlexEdgeData inset;
};

//************************************************************************************************
// FlexShared
//************************************************************************************************

class FlexShared
{
public:
	// Flex container styles
	DECLARE_STYLEDEF (flexDirection)
	DECLARE_STYLEDEF (flexWrap)
	DECLARE_STYLEDEF (flexJustify)
	DECLARE_STYLEDEF (flexAlign)

	// Flex item styles
	DECLARE_STYLEDEF (flexAlignSelf)
	DECLARE_STYLEDEF (flexPositionType)
	DECLARE_STYLEDEF (flexSizeMode)

	// Scan methods
	static void scan (FlexEdgeData& result, StringRef string);
	static void scan (FlexGutterData& result, StringRef string);

	// FlexContainerData methods
	static void setAttributes (FlexContainerData& flexData, const SkinAttributes& a);
	static void setAttributes (FlexContainerData& flexData, const IVisualStyleData& data, 
							   const FlexContainerData* flexDefault = nullptr);
	static void getAttributes (SkinAttributes& a, const FlexContainerData& flexData);
	static bool setProperty (FlexContainerData& flexData, StringID propertyId, VariantRef var);
	static bool getProperty (Variant& var, const FlexContainerData& flexData, StringID propertyId);

	// FlexItemData methods
	static void setAttributes (FlexItemData& flexItemData, const SkinAttributes& a);
	static void setAttributes (FlexItemData& flexItemData, const IVisualStyleData& data);
	static void getAttributes (SkinAttributes& a, const FlexItemData& flexItemData);
	static bool setProperty (FlexItemData& flexItemData, StringID propertyId, VariantRef var);
	static bool getProperty (Variant& var, const FlexItemData& flexItemData, StringID propertyId);
};

//************************************************************************************************
// FlexNode
//************************************************************************************************

class FlexNode
{
public:
	virtual ~FlexNode () {}

	virtual void applyContainerData (const FlexContainerData& flexData, const FlexItemData& flexItemData) = 0;
	virtual void applyItemData (const FlexItemData& flexItemData) = 0;
	virtual void applySizeLimits (const FlexItemData& flexItemData) = 0;
	virtual void applySize (CoordF width, CoordF height) = 0;

	virtual void calculateLayout (const CoordF* availableWidth = nullptr, const CoordF* availableHeight = nullptr) = 0;	
	virtual bool hasNewLayout () const = 0;
	virtual void setHasNewLayout (bool state) = 0;

	virtual PointF getLayoutPosition () const = 0;
	virtual CoordF getLayoutWidth () const = 0;
	virtual CoordF getLayoutHeight () const = 0;

	void getLayoutSize (RectF& itemSize) const
	{
		PointF p = getLayoutPosition ();
		CoordF w = getLayoutWidth ();
		CoordF h = getLayoutHeight ();
		itemSize (p.x, p.y, p.x + w, p.y + h);
	}
	
	virtual bool insertNode (int index, FlexNode* child) = 0;
	virtual bool removeNode (FlexNode* child) = 0;
};

//************************************************************************************************
// FlexNodeFactory
//************************************************************************************************

class FlexNodeFactory
{
public:
	static FlexNode* createNode ();
};

} // namespace CCL

#endif // _ccl_flexboxshared_h