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
// Filename    : ccl/gui/layout/layoutprimitives.h
// Description : Layout primitives
//
//************************************************************************************************

#include "ccl/gui/layout/layoutprimitives.h"
#include "ccl/gui/layout/directions.h"

namespace CCL {
namespace LayoutPrimitives {

//************************************************************************************************
// LayoutPrimitives
//************************************************************************************************

void joinSubViewLimits (RectRef parentSize, SizeLimit& parentLimits, View* subView)
{
	LayoutPrimitives::joinSubViewLimits<HorizontalDirection> (parentSize, parentLimits, subView);
	LayoutPrimitives::joinSubViewLimits<VerticalDirection> (parentSize, parentLimits, subView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool calcTitleLimits (SizeLimit& sizeLimits, View* view)
{
	return calcTitleLimits (sizeLimits, view->getTitle (), view->getVisualStyle ().getTextFont (), view->getSizeMode ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool calcTitleLimits (SizeLimit& sizeLimits, StringRef title, FontRef font, int sizeMode)
{
	if(!title.isEmpty ())
	{
		Rect r;
		Font::measureString (r, title, font);
		r.right  += 2;
		r.bottom += 2;

		sizeLimits.minWidth = r.right;
		sizeLimits.minHeight = r.bottom;

		if((sizeMode & (IView::kAttachLeft|IView::kAttachRight)) == (IView::kAttachLeft|IView::kAttachRight))
			sizeLimits.maxWidth = kMaxCoord; // they want us to grow ...
		else
			sizeLimits.maxWidth = r.right;

		if((sizeMode & (IView::kAttachTop|IView::kAttachBottom)) == (IView::kAttachTop|IView::kAttachBottom))
			sizeLimits.maxHeight = kMaxCoord;
		else
			sizeLimits.maxHeight = r.bottom;

		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool calcMultiLineLimits (SizeLimit& sizeLimits, View* view)
{
	Coord lineWidth = view->getWidth ();
	if(lineWidth <= 0)
		lineWidth = 100;

	return calcMultiLineLimits (sizeLimits, lineWidth, view->getTitle (), view->getVisualStyle ().getTextFont (), view->getSizeMode ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool calcMultiLineLimits (SizeLimit& sizeLimits, int lineWidth, StringRef title, FontRef font, int sizeMode)
{
	if(!title.isEmpty ())
	{
		Rect r;
		Font::measureText (r, lineWidth, title, font);

		// copied from calcTitleLimits():
		sizeLimits.minWidth = r.right;
		sizeLimits.minHeight = r.bottom;

		if((sizeMode & (IView::kAttachLeft|IView::kAttachRight)) == (IView::kAttachLeft|IView::kAttachRight))
			sizeLimits.maxWidth = kMaxCoord; // they want us to grow ...
		else
			sizeLimits.maxWidth = r.right;

		if((sizeMode & (IView::kAttachTop|IView::kAttachBottom)) == (IView::kAttachTop|IView::kAttachBottom))
			sizeLimits.maxHeight = kMaxCoord;
		else
			sizeLimits.maxHeight = r.bottom;

		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void resizeChild (View* view, RectRef parentSize, const Point& delta)
{
	int a = view->getSizeMode ();
	if((a & (View::kAttachAll|View::kHCenter|View::kVCenter)) != 0)
	{
		Rect r (view->getSize ());

		if((a & View::kAttachLeft) && (a & View::kAttachRight))
			r.right += delta.x;
		else if((a & View::kAttachRight) != 0)
			r.offset (delta.x);
		else if(a & View::kHCenter)
		{
			Coord w = r.getWidth ();
			r.left = (parentSize.getWidth () - w) / 2;
			r.setWidth (w);
		}

		if((a & View::kAttachTop) && (a & View::kAttachBottom))
			r.bottom += delta.y;
		else if((a & View::kAttachBottom) != 0)
			r.offset (0, delta.y);
		else if(a & View::kVCenter)
		{
			Coord h = r.getHeight ();
			r.top = (parentSize.getHeight () - h) / 2;
			r.setHeight (h);
		}
		view->setSize (r);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void checkCenter (RectRef parentSize, View& child)
{
	int sizeMode = child.getSizeMode ();
	if((sizeMode & (View::kHCenter|View::kVCenter)) != 0)
	{
		Rect r (child.getSize ());
		if(sizeMode & View::kHCenter)
		{
			Coord w = r.getWidth ();
			r.left = (parentSize.getWidth () - w) / 2;
			r.setWidth (w);
		}
		if(sizeMode & View::kVCenter)
		{
			Coord h = r.getHeight ();
			r.top = (parentSize.getHeight () - h) / 2;
			r.setHeight (h);
		}
		child.setSize (r);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void applySizeLimitsShallow (View& view, const SizeLimit& limits)
{
	int sizeMode= view.getSizeMode ();
	view.setSizeMode (0); // avoid passing these limits deeper to childs
	view.setSizeLimits (limits);
	view.setSizeMode (sizeMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace LayoutPrimitives
} // namespace CCL
