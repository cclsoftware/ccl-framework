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
// Filename    : ccl/app/controls/statsview.cpp
// Description : Statistics Graphing View
//
//************************************************************************************************

#include "ccl/app/controls/statsview.h"

#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/gui/graphics/igraphics.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/system/istatistics.h"

using namespace CCL;

//************************************************************************************************
// StatsGraphView
//************************************************************************************************

DEFINE_CLASS (StatsGraphView, UserControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

StatsGraphView::StatsGraphView (IStatisticsProvider* provider, RectRef size, StringRef category)
: provider (provider),
  category (category)
{
	if(provider)
		ISubject::addObserver (provider, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StatsGraphView::~StatsGraphView ()
{
	if(provider)
		ISubject::removeObserver (provider, this);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void StatsGraphView::setCategory (StringRef _category)
{
	category = _category;
	invalidate ();
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void StatsGraphView::attached (IView* parent)
{
	SuperClass::attached (parent);

	const IVisualStyle& vs = getVisualStyle ();
	backColor = vs.getColor ("backcolor", backColor);
	lineColor = vs.getColor ("linecolor", lineColor);
	brushColor = vs.getColor ("brushcolor", brushColor);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void StatsGraphView::draw (const DrawEvent& event)
{
	IGraphics& g = event.graphics;
	g.fillRect (event.updateRgn.bounds, SolidBrush (backColor));
	
	if(provider)
	{
		Pen pathPen (lineColor, (Pen::Size) 1.5f);
		SolidBrush pathBrush (brushColor);
		AutoPtr<IGraphicsPath> brushPath = GraphicsFactory::createPath ();
		
		PointF p;
		int width = getWidth ();
		CoordF height = static_cast<CoordF> (getHeight ());
		float xPos = 0;
		if(IStatisticsCollection* stats = provider->getData (MutableCString (category)))
		{
			brushPath->startFigure (p (0, height));
			int count = stats->countValues ();
			int offset = ccl_max (0, count-width);	
			int numItems = count - offset; 
			for(int i = 0; i < numItems; i++)
			{
				IStatisticsCollection::Value value = stats->getValue (i + offset);
				CoordF y = height - (value.normalized * height);
				ASSERT (y >= 0 && y <= height)
				xPos = i;
				brushPath->lineTo (p ((CoordF)xPos, y));
			}
		}

		brushPath->lineTo (p ((CoordF)xPos, height));
		brushPath->closeFigure ();
		g.fillPath (brushPath, pathBrush);		
		g.drawPath (brushPath, pathPen);
	}
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API StatsGraphView::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IStatisticsProvider::kStatsUpdated)
		invalidate ();
}
