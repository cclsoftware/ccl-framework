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
// Filename    : ccl/gui/help/viewhighlights.cpp
// Description : View Highlights
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/help/viewhighlights.h"
#include "ccl/gui/help/tutorialviewer.h"

#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/views/sprite.h"
#include "ccl/gui/layout/directions.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/graphicspath.h"

#include "ccl/base/math/mathrange.h"
#include "ccl/base/message.h"

#include "ccl/public/cclversion.h"

using namespace CCL;

//************************************************************************************************
// ViewHighlights::Style
//************************************************************************************************

class ViewHighlights::Style
{
public:
	PROPERTY_OBJECT (SolidBrush, dimBrush, DimBrush)
	PROPERTY_OBJECT (Pen, framePen, FramePen)

	PROPERTY_VARIABLE (Coord, circleExpand, CircleExpand)
	PROPERTY_VARIABLE (Coord, rectRadius, RectRadius)
	PROPERTY_VARIABLE (Coord, rectExpand, RectExpand)

	Style ()
	: dimBrush (Color (Colors::kBlack).setAlphaF (0.6)),
      framePen (Colors::kWhite, 2),
	  circleExpand (10),
	  rectRadius (10),
	  rectExpand (3)
	{}

	void init (const IVisualStyle& vs)
	{
		setCircleExpand (vs.getMetric ("circleExpand", getCircleExpand ()));
		setRectRadius (vs.getMetric ("rectRadius", getRectRadius ()));
		setRectExpand (vs.getMetric ("rectExpand", getRectExpand ()));

		setDimBrush (SolidBrush (vs.getColor ("dimColor", getDimBrush ().getColor ())));
		setFramePen (Pen (vs.getColor ("frameColor", getFramePen ().getColor ()), vs.getMetric ("frameWidth", getFramePen ().getWidth ())));
	}
};

//************************************************************************************************
// ViewHighlights::WindowItem
//************************************************************************************************

class ViewHighlights::WindowItem: public Object,
								  public AbstractDrawable
{
public:
	WindowItem (Window* window);

	Window* getWindow () const;

	ViewItem* getViewItem (View* view) const;
	ViewItem* addViewItem (View* view, bool exclusive);
	void removeViewItem (ViewItem* viewItem);
	void removeAll ();
	bool isEmpty () const;

	void updateRects ();
	void updateSprite ();
	void hideSprite ();

	bool checkSize ();

	// IDrawable
	void CCL_API draw (const DrawArgs& args) override;
	//float CCL_API getOpacity () const override;

	CLASS_INTERFACE (IDrawable, Object)

private:
	Window* window;
	AutoPtr<Sprite> sprite;
	ObjectList viewItems;
	Point lastWindowSize;
	Style style;

	void combineDrawRects ();
	void expandDrawRect (ViewItem& viewItem, Coord expand);
};

//************************************************************************************************
// ViewHighlights::ViewItem
//************************************************************************************************

class ViewHighlights::ViewItem: public Object
{
public:
	ViewItem (View* view)
	: view (view),
	  type (kRoundRect)
	{}

	View* getView () const { return view; }
	
	bool isAttached () const;
	Rect getSizeInWindow () const;

	PROPERTY_VARIABLE (ShapeType, type, Type)

	PROPERTY_OBJECT (Rect, lastViewRect, LastViewRect)
	PROPERTY_OBJECT (Rect, drawRect, DrawRect)

private:
	ObservedPtr<View> view;
};

//************************************************************************************************
// ViewHighlights::ViewItem
//************************************************************************************************

bool ViewHighlights::ViewItem::isAttached () const
{
	return view && view->isAttached ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect ViewHighlights::ViewItem::getSizeInWindow () const
{
	Rect viewRect;
	if(view)
	{
		Point pos;
		view->clientToWindow (pos);

		viewRect = view->getSize ();
		viewRect.moveTo (pos);
	}
	return viewRect;
}

//************************************************************************************************
// ViewHighlights::WindowItem
//************************************************************************************************

ViewHighlights::WindowItem::WindowItem (Window* window)
: window (window)
{
	viewItems.objectCleanup (true);

	style.init (FrameworkTheme::instance ().getStyle ("Standard.ViewHighlights"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* ViewHighlights::WindowItem::getWindow () const
{
	return window;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewHighlights::ViewItem* ViewHighlights::WindowItem::getViewItem (View* view) const
{
	for(auto viewItem : iterate_as<ViewItem> (viewItems))
		if(viewItem->getView () == view)
			return viewItem;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewHighlights::ViewItem* ViewHighlights::WindowItem::addViewItem (View* view, bool exclusive)
{
	if(exclusive)
		viewItems.removeAll ();

	auto viewItem = NEW ViewItem (view);
	viewItems.add (viewItem);
	return viewItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewHighlights::WindowItem::removeViewItem (ViewItem* viewItem)
{
	if(viewItems.remove (viewItem))
		viewItem->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewHighlights::WindowItem::removeAll ()
{
	viewItems.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewHighlights::WindowItem::isEmpty () const
{
	return viewItems.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewHighlights::WindowItem::updateSprite ()
{
	Rect size;
	window->getClientRect (size);

	if(sprite)
	{
		sprite->move (size);
		sprite->refresh ();
	}
	else
	{
		if(NativeGraphicsEngine::instance ().hasGraphicsLayers ())
			sprite = NEW SublayerSprite (window, this, size);
		else
			sprite = NEW FloatingSprite (window, this, size);

		sprite->show ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewHighlights::WindowItem::hideSprite ()
{
	if(sprite)
	{
		sprite->hide ();
		sprite.release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewHighlights::WindowItem::checkSize ()
{
	bool refreshNeeded = false;

	for(auto viewItem : iterate_as<ViewItem> (viewItems))
	{
		Rect viewRect (viewItem->getSizeInWindow ());
		if(viewRect != viewItem->getLastViewRect ())
			refreshNeeded = true;

		if(!viewItem->isAttached ())
			removeViewItem (viewItem);
	}

	if(sprite)
	{
		if(window&& window->getSize ().getSize () != lastWindowSize)
		{
			updateRects ();
			updateSprite ();
		}
		else if(refreshNeeded)
		{
			updateRects ();
			sprite->refresh ();
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewHighlights::WindowItem::combineDrawRects ()
{
	for(auto viewItem : iterate_as<ViewItem> (viewItems))
	{
		if(viewItem->getType () == kRoundRect)
		{
			RectRef rect (viewItem->getDrawRect ());
			if(!rect.isEmpty ())
			{
				// find another rect sharing an exact same edge, take over it's area
				for(auto neighborItem : iterate_as<ViewItem> (viewItems))
					if(neighborItem != viewItem && neighborItem->getType () == kRoundRect)
					{
						RectRef neighborRect (neighborItem->getDrawRect ());

						auto takeOverRect = [&] ()
						{
							// viewItem takes over rect of neighborItem
							viewItem->setDrawRect (Rect (rect).join (neighborRect));
							neighborItem->setDrawRect (Rect ());
						};

						if(neighborRect.top == rect.top && neighborRect.bottom == rect.bottom)
						{
							// vertical edge
							if(neighborRect.right == rect.left || neighborRect.left == rect.right)
								takeOverRect ();
						}
						else if(neighborRect.left == rect.left && neighborRect.right == rect.right)
						{
							// horizontal edge
							if(neighborRect.bottom == rect.top || neighborRect.top == rect.bottom)
								takeOverRect ();
						}
					}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewHighlights::WindowItem::expandDrawRect (ViewItem& viewItem, Coord expand)
{
	Rect rect (viewItem.getDrawRect ());
	if(!rect.isEmpty ())
	{
		rect.expand (expand);

		for(auto neighborItem : iterate_as<ViewItem> (viewItems))
			if(neighborItem != &viewItem)
			{
				RectRef neighborRect (neighborItem->getDrawRect ());
				Coord vOverlap = Math::Range<Coord> (rect.top, rect.bottom).getOverlapLength (Math::Range<Coord> (neighborRect.top, neighborRect.bottom));
				Coord hOverlap = Math::Range<Coord> (rect.left, rect.right).getOverlapLength (Math::Range<Coord> (neighborRect.left, neighborRect.right));

				// adjust horizontally if rects overlap vertically (edges touch) and vice versa, but only in the direction with the smallest overlap
				// (rect can also shrink if they overlap)
				if(vOverlap > 0 && vOverlap > hOverlap)
				{
					// left edge
					if(neighborRect.left < rect.left)
						ccl_lower_limit (rect.left, neighborRect.right); 

					// right edge
					if(neighborRect.right > rect.right)
						ccl_upper_limit (rect.right, neighborRect.left);
				}
				else if(hOverlap > 0)
				{
					// top edge
					if(neighborRect.top < rect.top)
						ccl_lower_limit (rect.top, neighborRect.bottom);

					// bottom edge
					if(neighborRect.bottom > rect.bottom)
						ccl_upper_limit (rect.bottom, neighborRect.top);
				}
			}

		viewItem.setDrawRect (rect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewHighlights::WindowItem::updateRects ()
{
	for(auto viewItem : iterate_as<ViewItem> (viewItems))
	{
		Rect viewRect (viewItem->getSizeInWindow ());
		viewItem->setLastViewRect (viewRect);

		if(!viewRect.isEmpty ())
		{
			constexpr Coord kMaxCircleSize = 50;
			ShapeType type = (viewRect.getWidth () > kMaxCircleSize || viewRect.getHeight () > kMaxCircleSize) ? kRoundRect : kCircle;
			// roundRect looks better for larger areas: could be defined via a visual style, or argument of highlightControl

			if(type == kCircle)
			{
				Coord radius = ccl_max (viewRect.getWidth (), viewRect.getHeight ());
				Rect origRect (viewRect);
				viewRect.setSize (Point (radius, radius)).center (origRect);
			}
			viewItem->setType (type);
		}
		viewItem->setDrawRect (viewRect);
	}

	combineDrawRects ();

	// expand draw rects, avoid overlaps with neighbor rects
	for(auto viewItem : iterate_as<ViewItem> (viewItems))
		expandDrawRect (*viewItem, viewItem->getType () == kRoundRect ? style.getRectExpand () : style.getCircleExpand ());
		// TODO: enlarge circle more exactly, so that the full view is inside
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ViewHighlights::WindowItem::draw (const DrawArgs& args)
{
	auto buildHighlitePath = [&] (GraphicsPath& path, bool isFrame = false)
	{
		for(auto viewItem : iterate_as<ViewItem> (viewItems))
		{
			Rect viewRect (viewItem->getDrawRect ());
			if(!viewRect.isEmpty ())
			{
				bool useRoundRect = viewRect.getWidth () > 100 || viewRect.getHeight () > 100;
				// roundRect looks better for larger areas: could be defined via a visual style, or argument of highlightControl

				if(viewItem->getType () == kRoundRect)
				{
					viewRect.bound (args.size);
					path.addRoundRect (viewRect, style.getRectRadius (), style.getRectRadius ());
				}
				else
				{
					path.startFigure (Point (viewRect.right, viewRect.getCenter ().y)); // start & end at 0 degrees
					path.addArc (viewRect, 0, 360);
					path.closeFigure ();
				}
			}
		}
	};

	GraphicsPath dimPath;
	dimPath.setFillMode (IGraphicsPath::kFillEvenOdd);

	// rectangle covering the window
	dimPath.startFigure (Point ());
	dimPath.addRect (args.size);
	dimPath.closeFigure ();

	// add a figure for each highlight view: the fill mode causes them to be considered "outside" of the fill area
	buildHighlitePath (dimPath);

	// frame around highlight areas
	GraphicsPath framePath;
	buildHighlitePath (framePath, true);

	args.graphics.fillPath (dimPath, style.getDimBrush ());
	args.graphics.drawPath (framePath, style.getFramePen ());

	lastWindowSize = args.size.getSize ();
}

//************************************************************************************************
// ViewHighlights
//************************************************************************************************

ViewHighlights::ViewHighlights ()
: isModifying (false)
{
	windowItems.objectCleanup (true);

	Desktop.addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewHighlights::~ViewHighlights ()
{
	Desktop.removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewHighlights::addView (View* view, bool exclusive)
{
	// support nullptr view to achieve the window dim effect only

	if(view != nullptr)
	{
		Window* window = view->getWindow ();
		WindowItem* windowItem = getWindowItem (window, true);
		ASSERT (windowItem)
		if(windowItem)
		{
			if(!windowItem->getViewItem (view))
				windowItem->addViewItem (view, exclusive);

			windowItem->updateRects ();
		}
	}

	// add window items for all remaining windows (no highlight, dim only)
	for(int i = 0, count = Desktop.countWindows (); i < count; i++)
		if(auto window = unknown_cast<Window> (Desktop.getWindow (i)))
			if(handlesWindow (*window))
				getWindowItem (window, true);

	if(!isModifying)
		updateSprites (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewHighlights::removeView (View* view)
{
	Window* window = view->getWindow ();
	if(WindowItem* windowItem = getWindowItem (window, false))
		if(ViewItem* viewItem = windowItem->getViewItem (view))
		{
			windowItem->removeViewItem (viewItem);

			if(windowItem->isEmpty ())
			{
				removeWindowItem (windowItem);

				// un-dim all windows if the last highlight was removed
				if(!hasAnyHighlights ())
					for(auto windowItem : iterate_as<WindowItem> (windowItems))
						removeWindowItem (windowItem);
			}
			else
			{
				windowItem->updateRects ();
			}
		}

	if(!isModifying)
		updateSprites (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewHighlights::removeAll ()
{
	if(isModifying)
	{
		for(auto windowItem : iterate_as<WindowItem> (windowItems))
			windowItem->removeAll (); // remove view items, but keep window dimmed
	}
	else
	{
		ForEach (windowItems, WindowItem, windowItem)
			removeWindowItem (windowItem);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewHighlights::modifyHighlights (bool begin)
{
	if(begin != isModifying)
	{
		isModifying = begin;

		if(!isModifying)
			updateSprites (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewHighlights::hasAnyHighlights () const
{
	for(auto windowItem : iterate_as<WindowItem> (windowItems))
		if(!windowItem->isEmpty ())
			return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewHighlights::handlesWindow (Window& window)
{
	// skip tutorial window and CCL Spy
	return window.getTitle () != CCL_SPY_NAME
		&& unknown_cast<TutorialViewer> (window.getController ()) == nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewHighlights::WindowItem* ViewHighlights::getWindowItem (Window* window, bool create)
{
	ASSERT (window)
	if(window)
		for(auto windowItem : iterate_as<WindowItem> (windowItems))
			if(windowItem->getWindow () == window)
				return windowItem;

	if(create)
	{
		auto windowItem = NEW WindowItem (window);
		window->addHandler (this);
		windowItems.add (windowItem);

		startTimer (kRefreshRate);
		return windowItem;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewHighlights::removeWindowItem (WindowItem* windowItem)
{
	ASSERT (windowItem)
	if(windowItem)
	{
		windowItem->getWindow ()->removeHandler (this);
		windowItem->hideSprite ();

		if(windowItems.remove (windowItem))
			windowItem->release ();

		if(windowItems.isEmpty ())
			stopTimer ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewHighlights::onWindowEvent (WindowEvent& event)
{
	if(event.eventType == WindowEvent::kClose)
		removeWindowItem (getWindowItem (unknown_cast<Window> (&event.window), false));

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewHighlights::updateSprites (bool deferred)
{
	if(deferred)
		(NEW Message ("updateSprites"))->post (this, -1);
	else
	{
		for(auto windowItem : iterate_as<WindowItem> (windowItems))
			windowItem->updateSprite ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewHighlights::onIdleTimer ()
{
	// check for changed window / view sizes and removed views, remove window item if no more views
	for(auto windowItem : iterate_as<WindowItem> (windowItems))
		if(!windowItem->checkSize ())
			removeWindowItem (windowItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewHighlights::onWindowAdded (Window* window)
{
	if(hasAnyHighlights () && window && window->isAttached ())
	{
		// add window item (no highlight): dim immediately after window was opened (avoid flicker)
		if(handlesWindow (*window))
			getWindowItem (window, true)->updateSprite ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ViewHighlights::notify (ISubject* subject, MessageRef msg)
{
	if(msg == DesktopManager::kWindowAdded)
	{
		auto window = unknown_cast<Window> (msg[0]);
		onWindowAdded (window);
	}
	else if(msg == "updateSprites")
		updateSprites (false);
}
