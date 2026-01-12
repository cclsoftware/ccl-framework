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
// Filename    : ccl/gui/skin/zoomableview.cpp
// Description : ZoomableView class
//
//************************************************************************************************

#include "ccl/gui/skin/zoomableview.h"

using namespace CCL;

//************************************************************************************************
// ZoomableView
//************************************************************************************************

DEFINE_CLASS (ZoomableView, View)

//////////////////////////////////////////////////////////////////////////////////////////////////

ZoomableView::ZoomableView (RectRef size, StyleRef style, StringRef title)
: View (size, style, title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZoomableView::setSupportedZoomfactors (const Vector<float>& factors)
{
	supportedZoomfactors = factors;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZoomableView::init ()
{
	// initially create the view with factor 1 to determine its "original" size
	AutoPtr<View> content (createContentView (1.f));
	originalSize = content->getSize ().getSize ();
	ccl_lower_limit (originalSize.x, 1); // avoid division by 0 in calculations
	ccl_lower_limit (originalSize.y, 1);

	float newZoomFactor = determineZoomFactor ();
	if(newZoomFactor != 0.f)
	{
		if(newZoomFactor != 1.f)
			content = createContentView (newZoomFactor); // create it again with the required zoom factor

		layoutContentView (*content);
		addView (content.detach ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float ZoomableView::determineZoomFactor () const
{
	float factorX = float (getWidth ()) / originalSize.x;
	float factorY = float (getHeight ()) / originalSize.y;
	float factor = ccl_min (factorX, factorY);

	if(!supportedZoomfactors.isEmpty ())
	{
		// choose greatest supported factor that fits the content
		float matchingFactor = supportedZoomfactors.first ();
		for(float f : supportedZoomfactors)
		{
			if(f > factor)
				break;

			matchingFactor = f;
		}
		factor = matchingFactor;
	}
	return factor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ZoomableView::createContentView (float contentZoomFactor)
{
	Theme::ZoomFactorScope scope (getTheme (), contentZoomFactor);
	ThemeSelector selector (getTheme ());
	
	auto* view = unknown_cast<View> (getTheme ().createView (formName, formController, &formArguments));
	ASSERT (view)
	if(!view)
		view = NEW View; // dummy to avoid the need for further nullptr checks
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZoomableView::layoutContentView (View& content)
{
	Rect clientRect;
	getClientRect (clientRect);

	Rect rect (0, 0, content.getSize ().getSize ());

	if(content.getSizeMode () & kAttachRight)
	{
		rect.left = (content.getSizeMode () & kAttachLeft) ? clientRect.left : (clientRect.right - rect.getWidth ());
		rect.right = clientRect.right;
	}
	else if(content.getSizeMode () & kHCenter)
		rect.centerH (clientRect);

	if(content.getSizeMode () & kAttachBottom)
	{
		rect.top = (content.getSizeMode () & kAttachTop) ? clientRect.top : (clientRect.bottom - rect.getHeight ());
		rect.bottom = clientRect.bottom;
	}
	else if(content.getSizeMode () & kVCenter)
		rect.centerV (clientRect);
	
	content.setSize (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZoomableView::attached (View* parent)
{
	SuperClass::attached (parent);

	if(isEmpty ())
		init ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ZoomableView::onSize (const Point& delta)
{
	if(isEmpty ())
		init ();
	else
	{
		float newZoomFactor = determineZoomFactor ();
		View* content = getFirst ();
		if(newZoomFactor != content->getZoomFactor ())
		{
			removeAll ();

			if(newZoomFactor != 0.f)
			{
				content = createContentView (newZoomFactor);
				layoutContentView (*content);
				addView (content);
			}
		}
		else if(content)
			layoutContentView (*content);
	}

	ScopedFlag<kAttachDisabled> disableAttach (sizeMode);
	SuperClass::onSize (delta);
}
