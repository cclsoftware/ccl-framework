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
// Filename    : ccl/gui/layout/sizevariantlayout.cpp
// Description : Clipper Layout
//
//************************************************************************************************

#include "ccl/gui/layout/sizevariantlayout.h"
#include "ccl/gui/layout/layoutprimitives.h"
#include "ccl/gui/layout/directions.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"

#include "ccl/base/message.h"

#define FITSIZE_EXPERIMENT 1

namespace CCL {

//************************************************************************************************
// SizeVariantLayoutAlgorithm
//************************************************************************************************

template<typename Direction>
class SizeVariantLayoutAlgorithm: public AnchorLayoutAlgorithm
{
public:
	SizeVariantLayoutAlgorithm (AnchorLayoutContext* context, AnchorLayoutData& layoutData);
	~SizeVariantLayoutAlgorithm ();

	// LayoutAlgorithm
	void doLayout () override;
	void onSize (const Point& delta) override;
	void calcSizeLimits (SizeLimit& limits) override;
	AnchorLayoutAlgorithm* onViewAdded (int index, AnchorLayoutItem* item) override;
	AnchorLayoutAlgorithm* onViewRemoved (AnchorLayoutItem* item) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	void flushLayout () override;

private:
	void onViewsChanged ();

	Coord getMinSize (AnchorLayoutItem* item) { return item->priority; } // // abusing the priority member for minSize (todo: cleanup)

	AnchorLayoutItem* getItemForSize ();
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// SizeVariantLayout
//************************************************************************************************

DEFINE_CLASS (SizeVariantLayout, AnchorLayout)

//////////////////////////////////////////////////////////////////////////////////////////////////

SizeVariantLayout::SizeVariantLayout ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutAlgorithm* SizeVariantLayout::createAlgorithm (LayoutContext* context)
{
	if(auto* anchorLayoutContext = ccl_cast<AnchorLayoutContext> (context))
	{
		bool isVertical = anchorLayoutContext->getStyle ().isCommonStyle (Styles::kVertical);

		if(isVertical)
			return NEW SizeVariantLayoutAlgorithm<VerticalDirection> (anchorLayoutContext, layoutData);
		else
			return NEW SizeVariantLayoutAlgorithm<HorizontalDirection> (anchorLayoutContext, layoutData);
	}

	return nullptr;
}

//************************************************************************************************
// SizeVariantLayoutAlgorithm
//************************************************************************************************

template <typename Direction>
SizeVariantLayoutAlgorithm<Direction>::SizeVariantLayoutAlgorithm (AnchorLayoutContext* context, AnchorLayoutData& layoutData)
: AnchorLayoutAlgorithm (context, layoutData)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
SizeVariantLayoutAlgorithm<Direction>::~SizeVariantLayoutAlgorithm ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
AnchorLayoutAlgorithm* SizeVariantLayoutAlgorithm<Direction>::onViewAdded (int index, AnchorLayoutItem* item)
{
	onViewsChanged ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
AnchorLayoutAlgorithm* SizeVariantLayoutAlgorithm<Direction>::onViewRemoved (AnchorLayoutItem* item)
{
	onViewsChanged ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void SizeVariantLayoutAlgorithm<Direction>::onViewsChanged ()
{
	if(context->getSizeMode () & Direction::OtherDirection::kFitSize)
		return;

	// calculate preferred size
	preferredSize (0, 0);
	ArrayForEachFast (context->getLayoutItems (), AnchorLayoutItem, item)
		Rect size = item->getView ()->getSize ();
		ccl_lower_limit (preferredSize.x, size.right);
		ccl_lower_limit (preferredSize.y, size.bottom);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void SizeVariantLayoutAlgorithm<Direction>::onSize (const Point& delta)
{
	// also resize the hidden views
	LayoutPrimitives::resizeChildItems (context->getLayoutItems (), context->getLayoutRect (), delta, context->isSizeModeDisabled ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void SizeVariantLayoutAlgorithm<Direction>::calcSizeLimits (SizeLimit& limits)
{
	#if FITSIZE_EXPERIMENT
	if(context->getSizeMode () & Direction::OtherDirection::kFitSize)
	{
		Direction::OtherDirection::getMin (limits) = Direction::OtherDirection::getMax (limits) = Direction::OtherDirection::getCoord (preferredSize);
		CCL_PRINTF ("SizeVariant limits: %d\n", Direction::OtherDirection::getCoord (preferredSize));
	}
	#endif
	
	Direction::getMax (limits) = 0;
	Direction::getMin (limits) = kMaxCoord;

	ArrayForEachFast (context->getLayoutItems (), AnchorLayoutItem, item)
		ccl_lower_limit (Direction::getMax (limits), Direction::getMax (item->sizeLimits));
		ccl_upper_limit (Direction::getMin (limits), Direction::getMin (item->sizeLimits));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void SizeVariantLayoutAlgorithm<Direction>::doLayout ()
{
	AnchorLayoutItem* selectedItem = getItemForSize ();
	ASSERT (selectedItem)
	if(selectedItem == nullptr)
		return;

	// first hide old item, then show the new one
	ArrayForEachFast (context->getLayoutItems (), AnchorLayoutItem, item)
		if(selectedItem != item)
			context->hideItem (item);
	EndFor
	context->showItem (selectedItem);

	#if FITSIZE_EXPERIMENT
	// fitsize in other direction
	if(context->getSizeMode () & Direction::OtherDirection::kFitSize)
	{
		// set item size as our prefered size
		Direction::OtherDirection::getCoord (preferredSize) = Direction::OtherDirection::getLength (selectedItem->getView ());

		// defer autoSizing:
		// if we are called during the resizing of parent views, trying to autoSize here would fail in some parent's onChildSized (checks for !isResizing)
		if(Direction::OtherDirection::getLength (preferredSize) != Direction::OtherDirection::getLength (context->getLayoutRect ()))
			(NEW Message ("autoSize"))->post (this, -1);
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
AnchorLayoutItem* SizeVariantLayoutAlgorithm<Direction>::getItemForSize ()
{
	Coord available = ccl_max (0, Direction::getLength (context->getLayoutRect ()));

	// todo: order item by minSize would give faster access (for many items)
	AnchorLayoutItem* bestItem = nullptr;

	ArrayForEachFast (context->getLayoutItems (), AnchorLayoutItem, item)
		if(getMinSize (item) <= available)
			if(!bestItem || getMinSize (item) > getMinSize (bestItem))
				bestItem = item;
	EndFor
	return bestItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void CCL_API SizeVariantLayoutAlgorithm<Direction>::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "autoSize")
	{
		if(context->getSizeMode () & Direction::OtherDirection::kFitSize)
		{
			context->requestResetSizeLimits ();

			tbool fitH = Direction::isVertical ();
			context->requestAutoSize (fitH, !fitH);
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Direction>
void SizeVariantLayoutAlgorithm<Direction>::flushLayout ()
{
	System::GetSignalHandler ().flush (this);
}
