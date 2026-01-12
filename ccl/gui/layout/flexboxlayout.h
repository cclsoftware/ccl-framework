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
// Filename    : ccl/gui/layout/flexboxlayout.h
// Description : Flexbox layout implementation
//
//************************************************************************************************

#ifndef _ccl_flexboxlayout_h
#define _ccl_flexboxlayout_h

#include "ccl/gui/layout/layoutview.h"

#include "ccl/public/gui/framework/designsize.h"

namespace CCL {

//************************************************************************************************
// Flexbox Data Types
//************************************************************************************************

enum class FlexDirection
{
	kRow,
	kRowReverse,
	kColumn,
	kColumnReverse
};

enum class FlexWrap
{
	kNoWrap,
	kWrap,
	kWrapReverse
};

enum class FlexJustify
{
	kFlexStart,
	kFlexEnd,
	kCenter,
	kSpaceBetween,
	kSpaceAround,
	kSpaceEvenly
};

enum class FlexAlign
{
	kFlexStart,
	kFlexEnd,
	kCenter,
	kStretch
};

enum class FlexAlignSelf
{
	kFlexStart,
	kFlexEnd,
	kCenter,
	kStretch,
	kAuto
};

enum class FlexPositionType
{
	kRelative,
	kAbsolute
};

enum class FlexSizeMode
{
	kHug,
	kHugHorizontal,
	kHugVertical,
	kFill
};

//************************************************************************************************
// EdgeData
//************************************************************************************************

struct EdgeData
{
	DesignCoord left = DesignCoord::kUndefined;
	DesignCoord top = DesignCoord::kUndefined;
	DesignCoord right = DesignCoord::kUndefined;
	DesignCoord bottom = DesignCoord::kUndefined;
	
	EdgeData& fromString (StringRef string);
};

//************************************************************************************************
// GutterData
//************************************************************************************************

struct GutterData
{
	DesignCoord row = DesignCoord::kUndefined;
	DesignCoord column = DesignCoord::kUndefined;
	
	GutterData& fromString (StringRef string);
};

//************************************************************************************************
// FlexData
//************************************************************************************************

struct FlexData
{
	FlexDirection direction = FlexDirection::kRow;
	FlexWrap wrap = FlexWrap::kNoWrap;
	FlexJustify justify = FlexJustify::kFlexStart;
	FlexAlign align = FlexAlign::kStretch;
	
	EdgeData padding;
	GutterData gap;
};

//************************************************************************************************
// FlexboxLayout
//************************************************************************************************

class FlexboxLayout: public Layout
{
public:
	DECLARE_CLASS_ABSTRACT (FlexboxLayout, Layout)

	DECLARE_STYLEDEF (flexDirection)
	DECLARE_STYLEDEF (flexWrap)
	DECLARE_STYLEDEF (flexJustify)
	DECLARE_STYLEDEF (flexAlign)

	// Layout
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	LayoutItem* createItem (View* view = nullptr) override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

protected:
	FlexData flexData;
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
	DesignCoord flexBasis; ///< The size flex grow or shrink properties are applied to in relation to other items.
	FlexAlignSelf alignSelf = FlexAlignSelf::kAuto;
	FlexPositionType positionType = FlexPositionType::kRelative;
	FlexSizeMode sizeMode = FlexSizeMode::kFill;
	
	EdgeData margin;
	EdgeData inset;
};

//************************************************************************************************
// FlexItem
//************************************************************************************************

class FlexItem: public LayoutItem
{
public:
	DECLARE_CLASS (FlexItem, LayoutItem)

	DECLARE_STYLEDEF (flexAlignSelf)
	DECLARE_STYLEDEF (flexPositionType)
	DECLARE_STYLEDEF (flexSizeMode)
	
	FlexItem ();
	FlexItem (View* view);

	void initialize (const DesignSize& designSize);
	void updateSizeLimits ();
	const FlexItemData& getFlexItemData () const;

	// LayoutItem
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

protected:
	FlexItemData flexItemData;
};

} // namespace CCL

#endif // _ccl_flexboxlayout_h
