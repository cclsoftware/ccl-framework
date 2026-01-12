//************************************************************************************************
//
// CCL Spy
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
// Filename    : shadowview.cpp
// Description : Shadow view representing a foreign View
//
//************************************************************************************************

#include "shadowview.h"

#include "ccl/base/trigger.h"
#include "ccl/public/gui/graphics/dpiscale.h"

#include "core/public/gui/coreuiproperties.h"

using namespace CCL;
using namespace Spy;

//************************************************************************************************
// ShadowView
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ShadowView, UserControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* ShadowView::buildViewTree (IEmbeddedViewHost& viewHost, IEmbeddedViewHost::ViewRef view, ShadowView* parentShadowView)
{
	ShadowView* shadowView = NEW ShadowView (viewHost, view, parentShadowView);

	for(int i = 0, num = viewHost.getSubViewCount (view); i < num; i++)
	{
		IEmbeddedViewHost::ViewRef child = viewHost.getSubViewAt (view, i);
		if(IView* shadowChild = buildViewTree (viewHost, child, shadowView))
			shadowView->getChildren ().add (shadowChild);
	}
	return *shadowView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ShadowView::ShadowView (IEmbeddedViewHost& viewHost, IEmbeddedViewHost::ViewRef view, ShadowView* parentShadowView)
{
	Core::ViewSizeProperty viewSize;
	viewHost.getViewProperty (viewSize, view);

	Core::ViewNameProperty viewName;
	viewHost.getViewProperty (viewName, view);

	Core::ViewClassProperty viewClass;
	viewHost.getViewProperty (viewClass, view);

	Core::ViewSourceProperty viewSource;
	viewHost.getViewProperty (viewSource, view);

	nativeSize = viewSize.size;

	IEmbeddedViewHost::ScreenScalingProperty scaling;
	viewHost.getViewProperty (scaling, view);

	// scale shadow view
	Rect r (nativeSize);
	r.left = DpiScale::coordToPixel (r.left, scaling.scaleFactor.x);
	r.right = DpiScale::coordToPixel (r.right, scaling.scaleFactor.x);
	r.top = DpiScale::coordToPixel (r.top, scaling.scaleFactor.y);
	r.bottom = DpiScale::coordToPixel (r.bottom, scaling.scaleFactor.y);

	setSize (r);
	setName (viewName.name);

	String source (viewSource.sourceFile);
	if(source.isEmpty () && parentShadowView)
		source = Property (parentShadowView, "source").get ().asString ();

	properties.set ("Class", viewClass.name);
	properties.set ("name", String (viewName.name));
	properties.set ("source", source);

	// todo: more properties (font, options, colors, ...) enum via IEmbeddedViewHost?
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ShadowView::getProperty (Variant& var, MemberID propertyId) const
{
	return properties.getAttribute (var, propertyId);
}
