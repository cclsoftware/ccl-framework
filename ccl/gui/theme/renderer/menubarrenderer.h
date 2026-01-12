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
// Filename    : ccl/gui/theme/renderer/menubarrenderer.cpp
// Description : Menu Bar Renderer
//
//************************************************************************************************

#ifndef _ccl_menubarrenderer_h
#define _ccl_menubarrenderer_h

#include "ccl/gui/theme/themerenderer.h"
#include "ccl/gui/popup/menubarcontrol.h"

namespace CCL {

class MenuBarControl;

//************************************************************************************************
// MenuBarRenderer
//************************************************************************************************

class MenuBarRenderer: public ThemeRenderer
{	
public:
	MenuBarRenderer (VisualStyle* visualStyle);

	// ThemeRenderer
	void draw (View* view, const UpdateRgn& updateRgn) override;
	int hitTest (View* view, const Point& loc, Point* clickOffset) override;
	bool getPartRect (const View* view, int partCode, Rect& rect) override;

protected:
	static const int kMenuHeight;
	static const int kMenuMargin;
	static const int kMenuSpacing;
	
	SharedPtr<IImage> background;
	SharedPtr<IImage> button;

	int menuMargin;
	int menuSpacing;
	int menuHeight;
	int iconSpacing;
	Rect padding;

	struct MenuDrawArgs
	{
		MenuBarControl* menuBar;
		GraphicsDevice& port;
		FontRef font;
		SolidBrush textBrush;
		SolidBrush activeTextBrush;
		int focusMenuIndex;

		MenuDrawArgs (MenuBarControl* menuBar, GraphicsDevice& port, FontRef font, SolidBrush textBrush, SolidBrush activeTextBrush)
		: menuBar (menuBar), port (port), font (font), textBrush (textBrush), activeTextBrush (activeTextBrush), focusMenuIndex (-1)
		{}
	};

	Coord calcMenuWidth (MenuBarControl* menuBar, int index, FontRef font);
	Coord calcMenuWidth (StringRef label, IImage* icon, FontRef font);

	void drawLabel (GraphicsDevice& port, RectRef rect, StringRef text, FontRef font, BrushRef brush, IImage* icon, Coord iconSpacing);
	void drawMenu (MenuDrawArgs& args, const Rect& menuRect, int index, bool active, StringRef label, IImage* icon);
	int findMenu (MenuBarControl* menuBar, const Point& loc);
};

DECLARE_VISUALSTYLE_CLASS (MenuBarControl)

} // namespace CCL

#endif // _ccl_theme_h
