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
// Filename    : ccl/gui/layout/flexboxlayout.cpp
// Description : Flexbox layout implementation
//
//************************************************************************************************

#include "ccl/gui/layout/flexboxlayout.h"

#include "ccl/base/message.h"

using namespace CCL;

//************************************************************************************************
// FlexboxLayout
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (FlexboxLayout, Layout)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FlexboxLayout::setAttributes (const SkinAttributes& a)
{
	FlexShared::setAttributes (flexData, a);	
	
	signal (Message (kPropertyChanged));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FlexboxLayout::getAttributes (SkinAttributes& a) const
{
	FlexShared::getAttributes (a, flexData);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutItem* FlexboxLayout::createItem (View* view)
{
	if(view != nullptr)
		return NEW FlexLayoutItem (view);
	return NEW FlexLayoutItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FlexboxLayout::setProperty (MemberID propertyId, const Variant& var)
{
	if(!FlexShared::setProperty (flexData, propertyId, var))
		return false;

	signal (Message (kPropertyChanged));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FlexboxLayout::getProperty (Variant& var, MemberID propertyId) const
{
	return FlexShared::getProperty (var, flexData, propertyId);
}

//************************************************************************************************
// FlexLayoutItem
//************************************************************************************************

DEFINE_CLASS (FlexLayoutItem, LayoutItem)

//////////////////////////////////////////////////////////////////////////////////////////////////

FlexLayoutItem::FlexLayoutItem ()
: LayoutItem ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FlexLayoutItem::FlexLayoutItem (View* view)
: LayoutItem (view)
{
	flexItemData.width.unit = initialSize.getWidth () > 0 ? DesignCoord::kCoord : DesignCoord::kAuto;
	flexItemData.height.unit = initialSize.getHeight () > 0 ? DesignCoord::kCoord : DesignCoord::kAuto;
	
	flexItemData.width.setIntValue (initialSize.getWidth ());
	flexItemData.height.setIntValue (initialSize.getHeight ());
	
	updateSizeLimits ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FlexLayoutItem::initialize (const DesignSize& designSize)
{
	bool hFit = get_flag<int> (view->getSizeMode (), IView::kHFitSize);
	bool hugHorizontal = flexItemData.sizeMode == FlexSizeMode::kHug || flexItemData.sizeMode == FlexSizeMode::kHugHorizontal;
	if(hugHorizontal || hFit)
	{
		flexItemData.width.unit = DesignCoord::kCoord;
		flexItemData.width.setIntValue (initialSize.getWidth ());
	}
	else
		flexItemData.width = designSize.width;
	
	bool vFit = get_flag<int> (view->getSizeMode (), IView::kVFitSize);
	bool hugVertical = flexItemData.sizeMode == FlexSizeMode::kHug || flexItemData.sizeMode == FlexSizeMode::kHugVertical;
	if(hugVertical || vFit)
	{
		flexItemData.height.unit = DesignCoord::kCoord;
		flexItemData.height.setIntValue (initialSize.getHeight ());
	}
	else
		flexItemData.height = designSize.height;
	
	signal (Message (kPropertyChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FlexLayoutItem::updateSizeLimits ()
{
	if(view == nullptr)
		return;
	
	const SizeLimit& limits = view->getSizeLimits ();
	
	flexItemData.minWidth.unit = DesignCoord::kCoord;
	flexItemData.minHeight.unit = DesignCoord::kCoord;
	flexItemData.maxWidth.unit = DesignCoord::kCoord;
	flexItemData.maxHeight.unit = DesignCoord::kCoord;

	flexItemData.minWidth.setIntValue (limits.minWidth);
	flexItemData.minHeight.setIntValue (limits.minHeight);
	flexItemData.maxWidth.setIntValue (limits.maxWidth);
	flexItemData.maxHeight.setIntValue (limits.maxHeight);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const FlexItemData& FlexLayoutItem::getFlexItemData () const
{
	return flexItemData;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FlexLayoutItem::setAttributes (const SkinAttributes& a)
{
	FlexShared::setAttributes (flexItemData, a);

	signal (Message (kPropertyChanged));

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FlexLayoutItem::getAttributes (SkinAttributes& a) const
{
	FlexShared::getAttributes (a, flexItemData);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FlexLayoutItem::setProperty (MemberID propertyId, const Variant& var)
{
	if(!FlexShared::setProperty (flexItemData, propertyId, var))
		return false;

	signal (Message (kPropertyChanged));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FlexLayoutItem::getProperty (Variant& var, MemberID propertyId) const
{
	return FlexShared::getProperty (var, flexItemData, propertyId);
}
