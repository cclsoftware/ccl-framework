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
// Filename    : ccl/gui/theme/renderer/tabviewrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/tabviewrenderer.h"

#include "ccl/gui/views/graphicsport.h"
#include "ccl/gui/graphics/graphicspath.h"

#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/gui/iparameter.h"

using namespace CCL;

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////

enum
{
	kTabSlope	= 20,
	kTabControl	= 10,
	kMenuTabWidth = 16,
	kContentMargin = 3,
	kTabMargin = 0,
	kTabSpacing = 0
};

//************************************************************************************************
// TabViewGeometry
/** Encapsulates the differences between horizontal & vertical tabviews. */
//************************************************************************************************

template<int orientation> struct TabViewGeometry;

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> struct TabViewGeometry<Styles::kHorizontal>
{
	static inline void clientRectToContent (Rect& rect, int tabHeight)	{ rect.top += tabHeight; }
	static inline void clientRectToHeader (Rect& rect, int tabHeight)	{ rect.bottom = rect.top + tabHeight; }
	static inline void headerToFirstTab (Rect& rect, Coord inset, Coord overlap) { rect.top += inset; rect.left += inset + overlap; }

	static inline void setTabWidth (Rect& rect, Coord width)			{ rect.right = rect.left + width; }
	static inline void setTabHeight (Rect& rect, Coord height)			{ rect.bottom = rect.top + height; }
	static inline void toNextTab (Rect& rect, Coord offset = 0)			{ rect.left = rect.right + offset; }
	static inline void offset (Rect& rect, Coord offset)				{ rect.left += offset; rect.right += offset; }
	static inline void offset (Point& p, Coord offset)					{ p.x += offset; }
	static inline bool isInside (PointRef p, RectRef rect)				{ return p.x >= rect.left && p.x <= rect.right; }
	static inline Coord& getStartCoord (Rect& rect)						{ return rect.left; }
	static inline Coord& getEndCoord (Rect& rect)						{ return rect.right; }

	static inline void makePath (GraphicsPath& path, RectRef r);
	static inline void drawLabel (GraphicsDevice& port, RectRef rect, StringRef text, FontRef font, BrushRef brush, IImage* icon, Coord iconSpacing, bool center);
	static inline void drawButton (GraphicsDevice& port, RectRef rect, IImage* button);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> struct TabViewGeometry<Styles::kVertical>
{
	static inline void clientRectToContent (Rect& rect, int tabHeight)				{ rect.right -= tabHeight; }
	static inline void clientRectToHeader (Rect& rect, int tabHeight)				{ rect.left = rect.right - tabHeight; }
	static inline void headerToFirstTab (Rect& rect, Coord inset, Coord overlap)	{ rect.right -= inset; rect.top += inset + overlap; }

	static inline void setTabWidth (Rect& rect, Coord width)			{ rect.bottom = rect.top + width; }
	static inline void setTabHeight (Rect& rect, Coord height)			{ rect.right = rect.left + height; }
	static inline void toNextTab (Rect& rect, Coord offset = 0)			{ rect.top = rect.bottom + offset; }
	static inline void offset (Rect& rect, Coord offset)				{ rect.top += offset; rect.bottom += offset; }
	static inline void offset (Point& p, Coord offset)					{ p.y += offset; }
	static inline bool isInside (PointRef p, RectRef rect)				{ return p.y >= rect.top && p.y <= rect.bottom; }
	static inline Coord& getStartCoord (Rect& rect)						{ return rect.top; }
	static inline Coord& getEndCoord (Rect& rect)						{ return rect.bottom; }

	static inline void makePath (GraphicsPath& path, RectRef r);
	static inline void drawLabel (GraphicsDevice& port, RectRef rect, StringRef text, FontRef font, BrushRef brush, IImage* icon, Coord iconSpacing, bool center);
	static inline void drawButton (GraphicsDevice& port, RectRef rect, IImage* button);
};

} // namespace CCL 


//////////////////////////////////////////////////////////////////////////////////////////////////

inline void TabViewGeometry<Styles::kHorizontal>::makePath (GraphicsPath& path, RectRef r)
{
	Point p1 (r.left + kTabSlope, r.top);
	Point p2 (r.right - kTabSlope, r.top);
	
	path.addBezier (Point (r.left, r.bottom), Point (r.left + kTabControl, r.bottom),
		Point (r.left + kTabSlope - kTabControl, r.top), p1);
	path.lineTo (p2);
	path.lineTo (Point (r.right - kTabSlope, r.top));
	path.addBezier (Point (r.right - kTabSlope, r.top), Point (r.right - kTabSlope + kTabControl, r.top),
		Point (r.right - kTabControl, r.bottom), Point (r.right, r.bottom));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void TabViewGeometry<Styles::kHorizontal>::drawLabel (GraphicsDevice& port, RectRef rect, StringRef text, FontRef font, BrushRef brush, IImage* icon, Coord iconSpacing, bool center)
{
	Rect labelRect (rect);
	if(icon)
	{
		Rect iconSize (0, 0, icon->getWidth (), icon->getHeight ());
		Rect iconRect (iconSize);
		if(text.isEmpty ())
			iconRect.center (rect);
		else
		{
			iconRect.centerV (rect);
			iconRect.offset (iconSpacing);
		}

		port.drawImage (icon, iconSize, iconRect);

		labelRect.left = iconRect.right;
	}

	if(!text.isEmpty ())
		port.drawString (labelRect, text, font, brush, center ? Alignment::kCenter : Alignment::kLeftCenter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void TabViewGeometry<Styles::kHorizontal>::drawButton (GraphicsDevice& port, RectRef rect, IImage* button)
{
	port.drawImage (button, Rect (0, 0, button->getWidth (), button->getHeight ()), rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void TabViewGeometry<Styles::kVertical>::makePath (GraphicsPath& path, RectRef r)
{
	Point p1 (r.right, r.top + kTabSlope);
	Point p2 (r.right, r.bottom - kTabSlope);
	path.addBezier (Point (r.left, r.top), Point (r.left, r.top + kTabControl),
		Point (r.right, r.top + kTabSlope - kTabControl), p1);
	path.lineTo (p2);
	path.addBezier (p2, Point (r.right, r.bottom - kTabSlope + kTabControl),
		Point (r.right, r.bottom - kTabControl), Point (r.left, r.bottom));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void TabViewGeometry<Styles::kVertical>::drawLabel (GraphicsDevice& port, RectRef rect, StringRef text, FontRef font, BrushRef brush, IImage* icon, Coord iconSpacing, bool center)
{
	ASSERT (icon == nullptr) // not implemented!

	Transform t;
	t.translate ((float)rect.right, (float)rect.top);
	t.rotate (Math::degreesToRad (90.f));
	Rect r (0, 0, rect.getHeight (), rect.getWidth ());
	port.saveState ();
	port.addTransform (t);
	port.drawString (r, text, font, brush, center ? Alignment::kCenter : Alignment::kLeftCenter);
	port.restoreState ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void TabViewGeometry<Styles::kVertical>::drawButton (GraphicsDevice& port, RectRef rect, IImage* button)
{
	port.drawImage (button, Rect (0, 0, button->getWidth (), button->getHeight ()), rect);
	/*
	Transform t;
	t.translate ((float)rect.right, (float)rect.top);
	t.rotate (Math::degreesToRad (90.f));
	Rect r (0, 0, rect.getHeight (), rect.getWidth ());
	port.saveState ();
	port.addTransform (t);
	port.drawImage (button, Rect (0, 0, button->getWidth (), button->getHeight ()), rect);
	port.restoreState ();
	*/
}

//************************************************************************************************
// TabViewRenderer
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (TabView, VisualStyle, "TabViewStyle")
	ADD_VISUALSTYLE_IMAGE  ("background")		///< background for content rect
	ADD_VISUALSTYLE_IMAGE  ("headerBackground")	///< background for header rect
	ADD_VISUALSTYLE_IMAGE  ("first")			///< background for the first of multiple buttons
	ADD_VISUALSTYLE_IMAGE  ("middle")			///< background for the middle buttons
	ADD_VISUALSTYLE_IMAGE  ("last")				///< background for the last of multiple buttons
	ADD_VISUALSTYLE_IMAGE  ("single")			///< background for a single button
	ADD_VISUALSTYLE_IMAGE  ("menuIcon")			///< icon shown on menu button
	ADD_VISUALSTYLE_IMAGE  ("menuBackground")	///< background for menu button (shown when not all tabs fit in the view)
	ADD_VISUALSTYLE_METRIC ("tabHeight")		///< height of tab buttons
	ADD_VISUALSTYLE_COLOR  ("borderColor")		///< color of border frame, drawn when no"background" image is available
	ADD_VISUALSTYLE_COLOR  ("activecolor")		///< used to draw instead of "backcolor" for the active tab (when no button image available)
	ADD_VISUALSTYLE_COLOR  ("activetextcolor")	///< used instead of "textcolor" for the active tab
	ADD_VISUALSTYLE_METRIC ("prefericon")		///< in case of an icon and a title - only show the icon
END_VISUALSTYLE_CLASS (TabView)

//////////////////////////////////////////////////////////////////////////////////////////////////

TabViewRenderer::TabViewRenderer (VisualStyle* visualStyle)
: ThemeRenderer (visualStyle),
  menuTabWidth (kMenuTabWidth)
{
	background = visualStyle->getImage ("background");
	headerBackground = visualStyle->getImage ("headerBackground");
	firstButton = visualStyle->getImage ("first");
	middleButton = visualStyle->getImage ("middle");
	lastButton = visualStyle->getImage ("last");
	singleButton = visualStyle->getImage ("single");
	menuIcon = visualStyle->getImage ("menuIcon");
	menuBackground = visualStyle->getImage ("menuBackground");
	tabMenuIcon = visualStyle->getImage ("tabMenuIcon");
	tabMargin = visualStyle->getMetric<int> ("tabmargin", kTabMargin);
	tabSpacing = visualStyle->getMetric<int> ("tabSpacing", kTabSpacing);
	visualStyle->getPadding (padding);
	
	if(menuBackground)
		menuTabWidth = menuBackground->getWidth ();

	tabSlope = (middleButton == nullptr) ? kTabSlope : 0;
	tabHeight = visualStyle->getMetric<int> ("tabHeight", kTabSlope);
	inset = (middleButton == nullptr) ? 2 : 0;
	iconSpacing = 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord TabViewRenderer::calcTabWidth (TabView* tabView, int index, FontRef font)
{
	String title;
	tabView->getTabTitle (title, index);
	return calcTabWidth (tabView, index, title, tabView->getTabIcon (index), font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord TabViewRenderer::calcTabWidth (TabView* tabView, int index, StringRef label, IImage* icon, FontRef font)
{
	Coord width = 0;
	if(!label.isEmpty ())
		width = Font::getStringWidth (label, font);
	if(icon)
	{
		width += icon->getWidth ();// + 2 * iconSpacing;
		if(!label.isEmpty ())
			width += iconSpacing;
	}

	if(tabView->getStyle ().isCustomStyle (Styles::kTabViewBehaviorTabMenu) && tabMenuIcon && index == tabView->getActiveIndex ())
		width += tabMenuIcon->getWidth () + iconSpacing;

	Coord fillWidth = tabView->getFillWidth ();
	if(fillWidth > 0)
	{
		int numTabs = tabView->countTabs ();
		Coord w = fillWidth / numTabs;
		Coord remainder = fillWidth - w * numTabs;
		width += w;
		if(index < remainder)
			width++;
	}
	
	return width + tabMargin;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::drawTab (View* view, GraphicsDevice& port, RectRef r, int tabIndex)
{
	TabView* tabView = (TabView*)view;
	Font textFont (visualStyle->getTextFont ());
	SolidBrush textBrush = visualStyle->getTextBrush ();

	TabDrawArgs args (tabView, port, textFont, textBrush, visualStyle->getColor ("activetextcolor", textBrush.getColor ()));

	String label;
	tabView->getTabTitle (label, tabIndex);
	Font::collapseString (label, r.getWidth (), args.font, Font::kTrimModeRight);

	IImage* icon = tabView->getTabIcon (tabIndex);
	bool active = tabIndex == tabView->getActiveIndex ();

	if(view->getStyle ().isVertical ())
		drawTab<TabViewGeometry<Styles::kVertical> > (args, r, kSingleTab, tabIndex, active, label, icon, true);
	else
		drawTab<TabViewGeometry<Styles::kHorizontal> > (args, r, kSingleTab, tabIndex, active, label, icon, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	if(view->getStyle ().isVertical ())
		draw<TabViewGeometry<Styles::kVertical> > (view, updateRgn);
	else
		draw<TabViewGeometry<Styles::kHorizontal> > (view, updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TabViewRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	if(view->getStyle ().isVertical ())
		return hitTest<TabViewGeometry<Styles::kVertical> > (view, loc, clickOffset);
	else
		return hitTest<TabViewGeometry<Styles::kHorizontal> > (view, loc, clickOffset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabViewRenderer::getPartRect (const View* view, int partCode, Rect& rect)
{
	if(view->getStyle ().isVertical ())
		return getPartRect<TabViewGeometry<Styles::kVertical> > (view, partCode, rect);
	else
		return getPartRect<TabViewGeometry<Styles::kHorizontal> > (view, partCode, rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Geometry> 
void TabViewRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	TabView* tabView = (TabView*)view;
	GraphicsPort port (view);
	Font textFont (visualStyle->getTextFont ());
	SolidBrush textBrush = visualStyle->getTextBrush ();

	TabDrawArgs args (tabView, port, textFont, textBrush, visualStyle->getColor ("activetextcolor", textBrush.getColor ()));

	Rect clientRect;
	view->getClientRect (clientRect);

	if(headerBackground)
	{
		
		Rect headerRect (clientRect);
		Geometry::clientRectToHeader (headerRect, tabHeight);
		if(headerRect.getWidth () > 5 && headerRect.getHeight () > 5)
		{
			Rect bgSize (0, 0, headerBackground->getWidth (), headerBackground->getHeight ());
			port.drawImage (headerBackground, bgSize, headerRect);
		}
	}
	
	// background
	Rect content (clientRect);
	Geometry::clientRectToContent (content, tabHeight);
	if(content.getWidth () > 5 && content.getHeight () > 5)
	{
		if(background)
		{
			Rect bgSize (0, 0, background->getWidth (), background->getHeight ());
			port.drawImage (background, bgSize, content);
		}
		else
			port.drawRect (content, Pen (visualStyle->getColor ("borderColor")));
	}

	int activeIndex = tabView->getActiveIndex ();
	int numTabs = tabView->countTabs ();
	if(numTabs <= 0)
		return;

	Rect header (clientRect);
	Geometry::clientRectToHeader (header, tabHeight);
	Coord tabsEndCoord = Geometry::getEndCoord (header);

	// running tab
	String label;
	TabKind tabKind = kStartTab;
	Rect tabRect = header;
	Geometry::headerToFirstTab (tabRect, inset, 0);

	// active tab
	String activeTabLabel;
	IImage* activeTabIcon = nullptr;
	TabKind activeTabKind = kStartTab;
	Rect activeTabRect;

	// menu tab
	Rect menuRect;
	Coord menuStart = kMaxCoord;

	// reset menu & scroll offset for calculation
	tabView->setMenu (false);
	tabView->setScrollOffset (0);
	tabView->setFillWidth (0);

	// check if menu required
	Rect requiredSize;
	calcViewSize<Geometry> (tabView, requiredSize);
	Coord remaingWidth = header.getWidth () - requiredSize.getWidth ();//todo: Geometry
	bool isOverflow = remaingWidth < 0;
	bool canHaveMenu = !tabView->getStyle ().isCustomStyle (Styles::kTabViewBehaviorNoMenu);
	bool hasMenu = canHaveMenu && isOverflow;
	tabView->setMenu (hasMenu);

	// extend tabs to fill header
	if(remaingWidth > 0)
	{
		if(tabView->getStyle ().isCustomStyle (Styles::kTabViewBehaviorExtendTabs))
			tabView->setFillWidth (remaingWidth);
		else if(tabView->getStyle ().isCustomStyle (Styles::kTabViewAppearanceCentered))
			tabView->setCenterOffset (remaingWidth / 2);
	}

	// use scroll offset to make the active tab completely visible
	Coord scrollOffset = 0;
	getPartRect (view, TabView::kPartFirstTab + activeIndex, activeTabRect);
	if(activeIndex != numTabs - 1)
		activeTabRect.right -= tabSpacing;
	
	if(isOverflow)
	{
		if(hasMenu)
		{
			// calc menu rect
			getPartRect (view, TabView::kPartMenuTab, menuRect);
			menuStart = Geometry::getStartCoord (menuRect);
			tabsEndCoord = menuStart;
		}

		Coord overlap = Geometry::getEndCoord (activeTabRect) - tabsEndCoord;
		if(overlap > 0 && numTabs > 1)
		{
			scrollOffset = overlap;
			tabView->setScrollOffset (scrollOffset);

			// recalc active rect
			getPartRect (view, TabView::kPartFirstTab + activeIndex, activeTabRect);
		}
	}

	Geometry::offset (tabRect, -scrollOffset);
	Geometry::offset (tabRect, tabView->getCenterOffset ());

	if(numTabs == 1 && !hasMenu)
		tabKind = kSingleTab;

	for(int i = 0; i < numTabs; i++)
	{
		if(Geometry::getStartCoord (tabRect) >= tabsEndCoord)
			break;

		Coord currentTabSpacing = (i == numTabs - 1) ? 0 : tabSpacing;
		
		tabView->getTabTitle (label, i);
		IImage* icon = tabView->getTabIcon (i);
		Coord width = calcTabWidth (tabView, i, label, icon, args.font);
		width += (middleButton ? kTabControl : 2 * kTabSlope);
		Geometry::setTabWidth (tabRect, width - currentTabSpacing);

		// skip tab if completly beyond left edge
		bool beyondLeft = Geometry::getEndCoord (tabRect) < 0;
		bool collapsed = false;
		if(!beyondLeft)
		{
			if(Geometry::getStartCoord (tabRect) < 0)
			{
				// overflow on left edge...
				Geometry::getStartCoord (tabRect) = 0;
				width = Geometry::getEndCoord (tabRect) - Geometry::getStartCoord (header);
				Font::collapseString (label, width, args.font, Font::kTrimModeRight);
				collapsed = true;		
			}
			else
			{
				Coord overflow = Geometry::getEndCoord (tabRect) - tabsEndCoord;
				if(overflow > 0)
				{
					// overflow on right edge
					width -= overflow;
					Geometry::setTabWidth (tabRect, width);
					Font::collapseString (label, width, args.font, Font::kTrimModeRight);
					collapsed = true;
				}
			}

			if(i == activeIndex)
			{
				activeTabKind = tabKind;
				activeTabLabel = label;
				activeTabIcon = icon;
			}
			else
				drawTab<Geometry> (args, tabRect, tabKind, i, false, label, icon, !collapsed);
		}		

		Geometry::toNextTab (tabRect, - tabSlope + currentTabSpacing);

		if(!beyondLeft)
			tabKind = (hasMenu || i < numTabs - 2) ? kMidtab : kEndTab;
	}


	if(activeIndex >= 0)
		drawTab<Geometry> (args, activeTabRect, activeTabKind, activeIndex, true, activeTabLabel, activeTabIcon, true);

	if(hasMenu)
	{
		if(menuBackground)
			port.drawImage (menuBackground, Rect (0, 0, menuBackground->getWidth (), menuBackground->getHeight ()), menuRect);
		else
			drawTab<Geometry> (args, menuRect, kEndTab, TabView::kPartMenuTab - TabView::kPartFirstTab, false, nullptr, nullptr, true);

		if(menuIcon)
		{
			Rect iconSize (0, 0, menuIcon->getWidth (), menuIcon->getHeight ());
			Rect iconRect (iconSize);
			iconRect.center (menuRect);
			port.drawImage (menuIcon, iconSize, iconRect);
		}
	}

	view->View::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Geometry>
void TabViewRenderer::drawTab (TabDrawArgs& args, const Rect& tabRect, TabKind kind, int index, bool active, StringRef label, IImage* icon, bool center)
{
	if(middleButton)
	{
		IImage* image = middleButton;
		switch(kind)
		{
		case kStartTab:
			if(firstButton)
				image = firstButton;
			break;

		case kEndTab:
			if(lastButton)
				image = lastButton;
			break;

		case kSingleTab:
			if(singleButton)
				image = singleButton;
             break;
         case kMidtab:
            image = middleButton;
            break;
		}

		int frameIndex = -1;
		if(active)
		{
			if(args.tabView->getMouseOverTab () == index)
				frameIndex = image->getFrameIndex (ThemeNames::kMouseOverOn);

			if(args.tabView->getMouseDownTab () == index)
				frameIndex = image->getFrameIndex (ThemeNames::kPressedOn);

			if(frameIndex < 0)
				frameIndex = image->getFrameIndex (ThemeNames::kNormalOn);
			if(frameIndex < 0)
				frameIndex = image->getFrameIndex (ThemeNames::kPressedOn);
			if(frameIndex < 0)
				frameIndex = image->getFrameIndex (ThemeNames::kPressed);
		}
		else
		{
			if(args.tabView->getMouseOverTab () == index)
				frameIndex = image->getFrameIndex (ThemeNames::kMouseOver);

			if(args.tabView->getMouseDownTab () == index)
				frameIndex = image->getFrameIndex (ThemeNames::kPressed);

			if(frameIndex < 0)
				frameIndex = image->getFrameIndex (ThemeNames::kNormal);
		}
		image->setCurrentFrame (frameIndex);

		Geometry::drawButton (args.port, tabRect, image);
	}
	else
	{
		Pen tabPen (visualStyle->getForeColor ());
		SolidBrush tabBrush (visualStyle->getBackColor ());
		SolidBrush activeTabBrush (visualStyle->getColor ("activecolor", Color(0xFF, 0xFF, 0xFF)));

		GraphicsPath path;
		Geometry::makePath (path, tabRect);
		path.closeFigure ();

		args.port.fillPath (path, active ? activeTabBrush : tabBrush);
		args.port.drawPath (path, tabPen);
	}

	Rect labelRect = tabRect;
	
	labelRect.left	 += padding.left;
	labelRect.right	 -= padding.right;
	labelRect.top	 += padding.top;
	labelRect.bottom  -= padding.bottom;
	
	if(active && tabMenuIcon && args.tabView->getStyle ().isCustomStyle (Styles::kTabViewBehaviorTabMenu))
	{
		// draw menu icon
		Rect iconSize (0, 0, tabMenuIcon->getWidth (), tabMenuIcon->getHeight ());
		Rect iconRect (iconSize);
		iconRect.offset (labelRect.right - iconRect.right);
		iconRect.centerV (labelRect);
		args.port.drawImage (tabMenuIcon, iconSize, iconRect);

		labelRect.right = iconRect.left - iconSpacing;
	}

	IImage::Selector (icon, active || args.tabView->getMouseDownTab () == index ? ThemeNames::kPressed : ThemeNames::kNormal);
	Geometry::drawLabel (args.port, labelRect, label, args.font, active ? args.activeTextBrush : args.textBrush, icon, iconSpacing, center);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Geometry>
int TabViewRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	TabView* tabView = (TabView*)view;
	int tab = findTab<Geometry> (tabView, loc);
	if(tab >= 0)
	{
		tab += TabView::kPartFirstTab;
		if(tab <= TabView::kPartLastTab)
			return tab;
	}
	
	Rect client;
	tabView->getClientRect (client);

	Rect rect (client);
	rect.contract (inset);
	Geometry::clientRectToContent (rect, tabHeight);
	if(rect.pointInside (loc))
		return TabView::kPartContent;

	rect = client;
	Geometry::clientRectToHeader (rect, tabHeight);
	if(rect.pointInside (loc))
		return TabView::kPartHeader;

	return TabView::kPartNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Geometry>
bool TabViewRenderer::getPartRect (const View* view, int partCode, CCL::Rect& rect)
{
	TabView* tabView = (TabView*)view;
	if(partCode == TabView::kPartContent)
	{
		view->getClientRect (rect);
		rect.contract (visualStyle->getMetric<int> ("contentmargin", kContentMargin));
		Geometry::clientRectToContent (rect, tabHeight);
		return true;
	}

	if(partCode == TabView::kPartViewSize)
	{
		calcViewSize<Geometry> ((TabView*)view, rect);
		return true;
	}

	if(partCode == TabView::kPartHeader)
	{
		view->getClientRect (rect);
		Geometry::clientRectToHeader (rect, tabHeight);
		return true;
	}

	if(partCode == TabView::kPartMenuTab)
	{
		if(tabView->isMenu ())
		{
			view->getClientRect (rect);
			Geometry::clientRectToHeader (rect, tabHeight);
			Geometry::headerToFirstTab (rect, inset, 0);
			Geometry::toNextTab (rect, -menuTabWidth);
			return true;
		}
		return false;
	}

	if(partCode >= TabView::kPartFirstTab && partCode <= TabView::kPartLastTab)
	{
		Rect r;
		view->getClientRect (r);
		Geometry::clientRectToHeader (r, tabHeight);
		Geometry::headerToFirstTab (r, inset, 0);
		Geometry::offset (r, - tabView->getScrollOffset ());

		Font tabFont (visualStyle->getTextFont ());	

		int numTabs = tabView->countTabs ();
		for(int i = 0; i < numTabs; i++)
		{
			Coord width = calcTabWidth (tabView, i, tabFont);
			Geometry::setTabWidth (r, width + (middleButton ? kTabControl : 2 * kTabSlope));
			if(i == partCode - TabView::kPartFirstTab)
			{
				Geometry::offset (r, tabView->getCenterOffset ());
				rect = r;
				return true;
			}
			Geometry::toNextTab (r, -tabSlope);
		}
	}

	if(partCode >= TabView::kPartTabMenu && tabMenuIcon)
	{
		int tab = partCode - TabView::kPartTabMenu;
		getPartRect (view, TabView::kPartFirstTab + tab, rect);
		rect.left = rect.right - tabMenuIcon->getWidth ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Geometry>
int TabViewRenderer::findTab (TabView* tabView, const Point& loc)
{
	Rect rect;
	tabView->getClientRect (rect);
//	rect.contract (inset);

	Geometry::clientRectToHeader (rect, tabHeight);
	if(!rect.pointInside (loc))
		return -1;

	if(tabView->isMenu () && loc.x > Geometry::getEndCoord (rect) - menuTabWidth)
		return TabView::kPartMenuTab - TabView::kPartFirstTab;

	Point p (loc);
	Geometry::offset (p, tabView->getScrollOffset ());
	Geometry::offset (rect, tabView->getCenterOffset ());

	if(middleButton == nullptr)
	{
		rect.contract (1);
		Geometry::headerToFirstTab (rect, inset, tabSlope / 2); // half width of the overlapping curves
	}

	Font tabFont (visualStyle->getTextFont ());
	int numTabs = tabView->countTabs ();
	for(int i = 0; i < numTabs; i++)
	{
		Coord width = calcTabWidth (tabView, i, tabFont);
		Geometry::setTabWidth (rect, width + (middleButton ? kTabControl : 2 * kTabSlope));

		if(Geometry::isInside (p, rect))
			return i;
		
		Geometry::toNextTab (rect);
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Geometry> 
void TabViewRenderer::calcViewSize (TabView* tabView, Rect& r)
{
	Rect rect;
	Geometry::setTabWidth (rect, 0);
	Geometry::setTabHeight (rect, tabHeight);
	
	Font tabFont (visualStyle->getTextFont ());	

	int numTabs = tabView->countTabs ();
	for(int i = 0; i < numTabs; i++)
	{
		Coord width = calcTabWidth (tabView, i, tabFont);
		Geometry::setTabWidth (rect, width + (middleButton ? kTabControl : 2 * kTabSlope));
		if(i < (numTabs - 1))
			Geometry::toNextTab (rect, -tabSlope);
	}

	r = rect;
	r.left = 0;
	r.top = 0;
}
