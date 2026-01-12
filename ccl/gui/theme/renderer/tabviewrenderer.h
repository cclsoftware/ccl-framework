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

#ifndef _ccl_tabviewrenderer_h
#define _ccl_tabviewrenderer_h

#include "ccl/gui/theme/themerenderer.h"
#include "ccl/gui/controls/tabview.h"

namespace CCL {

class TabView;

//************************************************************************************************
// TabViewRenderer
//************************************************************************************************

class TabViewRenderer: public ThemeRenderer,
					   public ITabViewRenderer
{	
public:
	TabViewRenderer (VisualStyle* visualStyle);

	void draw (View* view, const UpdateRgn& updateRgn) override;
	int hitTest (View* view, const Point& loc, Point* clickOffset) override;
	bool getPartRect (const View* view, int partCode, Rect& rect) override;

	// ITabViewRenderer
	void drawTab (View* view, GraphicsDevice& port, RectRef r, int tabIndex) override;

	CLASS_INTERFACE (ITabViewRenderer, ThemeRenderer)

protected:
	SharedPtr<IImage> background;
	SharedPtr<IImage> headerBackground;	
	SharedPtr<IImage> firstButton;
	SharedPtr<IImage> middleButton;
	SharedPtr<IImage> lastButton;
	SharedPtr<IImage> singleButton;
	SharedPtr<IImage> menuIcon;
	SharedPtr<IImage> menuBackground;
	SharedPtr<IImage> tabMenuIcon;

	int tabMargin;
	int tabSpacing;
	int tabSlope;
	int tabHeight;
	int inset;
	int iconSpacing;
	int menuTabWidth;
	Rect padding;

	enum TabKind { kStartTab, kMidtab, kEndTab, kSingleTab };

	struct TabDrawArgs
	{
		TabView* tabView;
		GraphicsDevice& port;
		FontRef font;
		SolidBrush textBrush;
		SolidBrush activeTextBrush;

		TabDrawArgs (TabView* tabView, GraphicsDevice& port, FontRef font, SolidBrush textBrush, SolidBrush activeTextBrush)
		: tabView (tabView), port (port), font (font), textBrush (textBrush), activeTextBrush(activeTextBrush)
		{}
	};

	Coord calcTabWidth (TabView* tabView, int index, FontRef font);
	Coord calcTabWidth (TabView* tabView, int index, StringRef label, IImage* icon, FontRef font);

	template<class Geometry> void draw (View* view, const UpdateRgn& updateRgn);
	template<class Geometry> void drawTab (TabDrawArgs& args, const Rect& tabRect, TabKind kind, int index, bool active, StringRef label, IImage* icon, bool center);
	template<class Geometry> int hitTest (View* view, const Point& loc, Point* clickOffset);
	template<class Geometry> bool getPartRect (const View* view, int partCode, Rect& rect);
	template<class Geometry> int findTab (TabView* tabView, const Point& loc);
	template<class Geometry> void calcViewSize (TabView* tabView, Rect& rect);
};

DECLARE_VISUALSTYLE_CLASS (TabView)

} // namespace CCL

#endif // _ccl_theme_h
