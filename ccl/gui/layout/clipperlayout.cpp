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
// Filename    : ccl/gui/layout/clipperlayout.cpp
// Description : Clipper Layout
//
//************************************************************************************************

#include "ccl/gui/layout/clipperlayout.h"
#include "ccl/gui/layout/layoutprimitives.h"

namespace CCL {

//************************************************************************************************
// ClipperLayoutAlgorithm
//************************************************************************************************

class ClipperLayoutAlgorithm: public AnchorLayoutAlgorithm
{
public:
	ClipperLayoutAlgorithm (AnchorLayoutContext* context, AnchorLayoutData& layoutData);

	// LayoutAlgorithm
	void doLayout () override;
	void onSize (const Point& delta) override;
	AnchorLayoutAlgorithm* onViewAdded (int index, AnchorLayoutItem* item) override;
	AnchorLayoutAlgorithm* onViewRemoved (AnchorLayoutItem* item) override;

private:
	void onViewsChanged ();
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// ClipperLayout
//************************************************************************************************

DEFINE_CLASS (ClipperLayout, AnchorLayout)

//////////////////////////////////////////////////////////////////////////////////////////////////

ClipperLayout::ClipperLayout ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LayoutAlgorithm* ClipperLayout::createAlgorithm (LayoutContext* context)
{
	if(auto* anchorLayoutContext = ccl_cast<AnchorLayoutContext> (context))
		return NEW ClipperLayoutAlgorithm (anchorLayoutContext, layoutData);

	return nullptr;
}

//************************************************************************************************
// ClipperLayoutAlgorithm
//************************************************************************************************

ClipperLayoutAlgorithm::ClipperLayoutAlgorithm (AnchorLayoutContext* context, AnchorLayoutData& layoutData)
: AnchorLayoutAlgorithm (context, layoutData)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnchorLayoutAlgorithm* ClipperLayoutAlgorithm::onViewAdded (int index, AnchorLayoutItem* item)
{
	onViewsChanged ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AnchorLayoutAlgorithm* ClipperLayoutAlgorithm::onViewRemoved (AnchorLayoutItem* item)
{
	onViewsChanged ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClipperLayoutAlgorithm::onViewsChanged ()
{
	// calculate preferred size
	preferredSize (0, 0);
	ForEach (context->getLayoutItems (), AnchorLayoutItem, item)
		Rect size = item->getView ()->getSize ();
		ccl_lower_limit (preferredSize.x, size.right);
		ccl_lower_limit (preferredSize.y, size.bottom);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClipperLayoutAlgorithm::onSize (const Point& delta)
{
	// also resize the hidden views
	LayoutPrimitives::resizeChildItems (context->getLayoutItems (), context->getLayoutRect (), delta, context->isSizeModeDisabled ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClipperLayoutAlgorithm::doLayout ()
{
	Rect containerRect (0, 0, context->getLayoutRect ().getSize ());

	containerRect.right++; // include end coords in rectInside
	containerRect.bottom++;

	ForEach (context->getLayoutItems (), AnchorLayoutItem, item)
		if(containerRect.rectInside (item->getView ()->getSize ()))
			context->showItem (item);
		else
			context->hideItem (item);
	EndFor
}
