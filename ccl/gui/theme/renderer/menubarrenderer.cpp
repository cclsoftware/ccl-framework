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

#include "ccl/gui/theme/renderer/menubarrenderer.h"

using namespace CCL;

//************************************************************************************************
// MenuBarRenderer
//************************************************************************************************

const int MenuBarRenderer::kMenuHeight = 20;
const int MenuBarRenderer::kMenuMargin = 10;
const int MenuBarRenderer::kMenuSpacing = 0;

BEGIN_VISUALSTYLE_CLASS (MenuBarControl, VisualStyle, "MenuBarControlStyle")
	ADD_VISUALSTYLE_IMAGE  ("menuButton")		///< background for menu buttons
	ADD_VISUALSTYLE_METRIC ("menuHeight")		///< height of menu buttons
	ADD_VISUALSTYLE_COLOR  ("activecolor")		///< used to draw instead of "backcolor" for the active menu (when no button image available)
	ADD_VISUALSTYLE_COLOR  ("activetextcolor")	///< used instead of "textcolor" for the active menu
END_VISUALSTYLE_CLASS (MenuBarControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuBarRenderer::MenuBarRenderer (VisualStyle* visualStyle)
: ThemeRenderer (visualStyle)
{
	background = visualStyle->getImage ("background");
	button = visualStyle->getImage ("menuButton");
	menuMargin = visualStyle->getMetric<int> ("menuMargin", kMenuMargin);
	menuSpacing = visualStyle->getMetric<int> ("menuSpacing", kMenuSpacing);
	visualStyle->getPadding (padding);
	
	menuHeight = visualStyle->getMetric<int> ("menuHeight", kMenuHeight);
	iconSpacing = 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord MenuBarRenderer::calcMenuWidth (MenuBarControl* menuBar, int index, FontRef font)
{
	String title;
	menuBar->getMenuTitle (title, index);
	return calcMenuWidth (title, menuBar->getMenuIcon (index), font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord MenuBarRenderer::calcMenuWidth (StringRef label, IImage* icon, FontRef font)
{
	Coord width = 0;
	if(!label.isEmpty ())
		width = Font::getStringWidth (label, font);
	if(icon)
	{
		width += icon->getWidth ();
		if(!label.isEmpty ())
			width += iconSpacing;
	}

	return width + 2 * menuMargin;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MenuBarRenderer::hitTest (View* view, const Point& loc, Point* clickOffset)
{
	MenuBarControl* menuBar = (MenuBarControl*)view;
	int index = findMenu (menuBar, loc);
	if(index >= 0)
	{
		index += MenuBarControl::kPartFirstMenu;
		if(index <= MenuBarControl::kPartLastMenu)
			return index;
	}
	
	Rect clientRect;
	menuBar->getClientRect (clientRect);
	clientRect.setHeight (menuHeight);
	if(clientRect.pointInside (loc))
		return MenuBarControl::kPartBar;

	return MenuBarControl::kPartNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuBarRenderer::getPartRect (const View* view, int partCode, Rect& rect)
{
	MenuBarControl* menuBar = (MenuBarControl*)view;

	view->getClientRect (rect);
	rect.setHeight (menuHeight);
	
	if(partCode == MenuBarControl::kPartBar)
		return true;

	if(partCode >= MenuBarControl::kPartFirstMenu && partCode <= MenuBarControl::kPartLastMenu)
	{
		Rect menuRect (rect);
		Font menuFont (visualStyle->getTextFont ());	

		int numMenus = menuBar->countMenus ();
		for(int i = 0; i < numMenus; i++)
		{
			Coord width = calcMenuWidth (menuBar, i, menuFont);
			menuRect.setWidth (width);
			if(i == partCode - MenuBarControl::kPartFirstMenu)
			{
				rect = menuRect;
				return true;
			}
			menuRect.offset (menuSpacing + width, 0);
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MenuBarRenderer::findMenu (MenuBarControl* menuBar, const Point& loc)
{
	Rect rect;
	menuBar->getClientRect (rect);
	rect.setHeight (menuHeight);

	if(!rect.pointInside (loc))
		return -1;

	Font menuFont (visualStyle->getTextFont ());
	int numMenus = menuBar->countMenus ();
	for(int i = 0; i < numMenus; i++)
	{
		Coord width = calcMenuWidth (menuBar, i, menuFont);
		rect.setWidth (width);

		if(rect.pointInside (loc))
			return i;
		
		rect.offset (menuSpacing + width, 0);
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBarRenderer::draw (View* view, const UpdateRgn& updateRgn)
{
	MenuBarControl* menuBar = (MenuBarControl*)view;
	GraphicsPort port (view);
	Font textFont (visualStyle->getTextFont ());
	SolidBrush textBrush = visualStyle->getTextBrush ();

	MenuDrawArgs args (menuBar, port, textFont, textBrush, visualStyle->getColor ("activetextcolor", textBrush.getColor ()));

	// indicate focus menu when in mouse-over or focus state (mouse or keyboard navigation)
	if(menuBar->isFocused () || (menuBar->getMouseState () & View::kMouseOver))
		args.focusMenuIndex = menuBar->getFocusMenu ();

	Rect clientRect;
	view->getClientRect (clientRect);
	clientRect.setHeight (menuHeight);

	if(background)
	{
		if(clientRect.getWidth () > 5 && clientRect.getHeight () > 5)
		{
			Rect bgSize (0, 0, background->getWidth (), background->getHeight ());
			port.drawImage (background, bgSize, clientRect);
		}
	}
	
	int activeIndex = menuBar->getActiveIndex ();
	int numMenus = menuBar->countMenus ();
	if(numMenus <= 0)
		return;

	Rect menuRect (clientRect);
	for(int i = 0; i < numMenus; i++)
	{
		String label;
		menuBar->getMenuTitle (label, i);
		IImage* icon = menuBar->getMenuIcon (i);
		Coord width = calcMenuWidth (label, icon, args.font);
		menuRect.setWidth (width);
		
		drawMenu (args, menuRect, i, i == activeIndex, label, icon);

		menuRect.offset (menuSpacing + width, 0);
	}	

	view->View::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBarRenderer::drawMenu (MenuDrawArgs& args, const Rect& menuRect, int index, bool active, StringRef label, IImage* icon)
{
	if(button)
	{
		int frameIndex = -1;
		if(active)
		{
			if(args.focusMenuIndex == index)
				frameIndex = button->getFrameIndex (ThemeNames::kMouseOverOn);

			if(args.menuBar->getMouseDownMenu () == index)
				frameIndex = button->getFrameIndex (ThemeNames::kPressedOn);

			if(frameIndex < 0)
				frameIndex = button->getFrameIndex (ThemeNames::kNormalOn);
			if(frameIndex < 0)
				frameIndex = button->getFrameIndex (ThemeNames::kPressedOn);
			if(frameIndex < 0)
				frameIndex = button->getFrameIndex (ThemeNames::kPressed);
		}
		else
		{
			if(args.focusMenuIndex == index)
				frameIndex = button->getFrameIndex (ThemeNames::kMouseOver);

			if(args.menuBar->getMouseDownMenu () == index)
				frameIndex = button->getFrameIndex (ThemeNames::kPressed);

			if(frameIndex < 0)
				frameIndex = button->getFrameIndex (ThemeNames::kNormal);
		}
		button->setCurrentFrame (frameIndex);

		args.port.drawImage (button, Rect (0, 0, button->getWidth (), button->getHeight ()), menuRect);
	}
	else
	{
		Pen menuPen (visualStyle->getForeColor ());
		SolidBrush menuBrush (visualStyle->getBackColor ());
		SolidBrush activeMenuBrush (visualStyle->getColor ("activecolor", Color(0xFF, 0xFF, 0xFF)));

		args.port.fillRect (menuRect, active ? activeMenuBrush : menuBrush);
	}

	Rect labelRect = menuRect;
	
	labelRect.left += padding.left;
	labelRect.right -= padding.right;
	labelRect.top += padding.top;
	labelRect.bottom -= padding.bottom;
	
	IImage::Selector (icon, active || args.menuBar->getMouseDownMenu () == index ? ThemeNames::kPressed : ThemeNames::kNormal);
	drawLabel (args.port, labelRect, label, args.font, active ? args.activeTextBrush : args.textBrush, icon, iconSpacing);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuBarRenderer::drawLabel (GraphicsDevice& port, RectRef rect, StringRef text, FontRef font, BrushRef brush, IImage* icon, Coord iconSpacing)
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
		port.drawString (labelRect, text, font, brush, Alignment::kCenter);
}
