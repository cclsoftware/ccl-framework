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
// Filename    : ccl/gui/popup/menucontrol.cpp
// Description : Menu Control
//
//************************************************************************************************

#define DEBUG_LOG 0
#define USE_KEY_GLYPHS (CCL_PLATFORM_MAC || CCL_PLATFORM_IOS)

#include "ccl/gui/popup/menucontrol.h"
#include "ccl/gui/popup/menubarcontrol.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/popup/extendedmenu.h"
#include "ccl/gui/popup/parametermenubuilder.h"
#include "ccl/gui/views/viewanimation.h"
#include "ccl/gui/views/viewaccessibility.h"
#include "ccl/gui/controls/editbox.h"
#include "ccl/gui/graphics/imaging/multiimage.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/help/keyglyphpainter.h"
#include "ccl/gui/graphics/imaging/image.h"
#include "ccl/gui/graphics/imaging/imagecache.h"

#include "ccl/base/message.h"

#include "ccl/public/systemservices.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// MenuControlConstants
//////////////////////////////////////////////////////////////////////////////////////////////////

enum MenuControlConstants
{
	kSubMenuOverlap = 2,
	kMaxMenuItemDescriptionWidth = 400,

	kSubMenuOpenTimeOut = 400,
	kSubMenuCloseTimeOut = 200
};

//************************************************************************************************
// MenuClientAccessibilityProvider
//************************************************************************************************

class MenuClientAccessibilityProvider: public ViewAccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (MenuClientAccessibilityProvider, ViewAccessibilityProvider)

	MenuClientAccessibilityProvider (MenuControl::ClientView& owner);
	
	// ViewAccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;
	void CCL_API getElementName (String& name) const override;

protected:
	MenuControl::ClientView& getClientView () const;
};

//************************************************************************************************
// MenuItemButtonAccessibilityProvider
//************************************************************************************************

class MenuItemButtonAccessibilityProvider: public ViewAccessibilityProvider,
										   public IAccessibilityActionProvider
{
public:
	DECLARE_CLASS_ABSTRACT (MenuItemButtonAccessibilityProvider, ViewAccessibilityProvider)

	MenuItemButtonAccessibilityProvider (MenuControl::ItemButton& owner);
	
	// ViewAccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;
	void CCL_API getElementName (String& name) const override;

	// IAccessibilityActionProvider
	tresult CCL_API performAction () override;

	CLASS_INTERFACE (IAccessibilityActionProvider, ViewAccessibilityProvider)

protected:
	MenuControl::ItemButton& getItemButton () const;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ForEachMenuItem(menu, item) \
{ for(int __index = 0; __index < (menu).countItems (); __index++) \
  { MenuItem* item = (menu).at (__index);

//////////////////////////////////////////////////////////////////////////////////////////////////
/** A MenuControl is a custom implementation of a menu (e\.g\. a context menu). */

BEGIN_VISUALSTYLE_CLASS (MenuControl, VisualStyle, "MenuControlStyle")
	ADD_VISUALSTYLE_FONT ("smallfont")				///< used for menu item descriptions
	ADD_VISUALSTYLE_COLOR ("bordercolor")			///< used to draw a border around the whole menu
	ADD_VISUALSTYLE_COLOR ("separatorcolor")		///< used to draw separator lines
	ADD_VISUALSTYLE_COLOR ("headercolor")			///< used to fill the header area
	ADD_VISUALSTYLE_COLOR ("selectionbackcolor")	///< used to hihlite selected items, if no "selectionbarimage" is available
	ADD_VISUALSTYLE_COLOR ("selectionframecolor")	///< used to draw a frame around selected items
	ADD_VISUALSTYLE_COLOR ("disabledtextcolor")		///< used instead of "textcolor" for disabled items
	ADD_VISUALSTYLE_COLOR ("headertextcolor")		///< used instead of "textcolor" in the header area
	ADD_VISUALSTYLE_COLOR ("selectedtextcolor")		///< used instead of "textcolor" for the currently selected item
	ADD_VISUALSTYLE_IMAGE ("checkmarkicon")			///< a checkmark icon drawn after a checked item
	ADD_VISUALSTYLE_COLOR  ("iconcolor")			///< color to colorize icons of unselected items
	ADD_VISUALSTYLE_COLOR  ("selectediconcolor")	///< used instead of "iconcolor" to colorize icons of selected items
	ADD_VISUALSTYLE_IMAGE ("selectionbarimage")		///< background image for a selected item
	ADD_VISUALSTYLE_METRIC ("normaliconsize")		///< icon size (in points) for a normal menu
	ADD_VISUALSTYLE_METRIC ("largeiconsize")		///< icon size (in points) for a large menu
	ADD_VISUALSTYLE_METRIC ("spacing")				///< spacing (in points) between items
	ADD_VISUALSTYLE_METRIC ("separatorSpacing")		///< optional spacing definition for spacing (in points) between separated items
	ADD_VISUALSTYLE_METRIC ("segmentspacing")		///< spacing (in points) between segments of an item (icon, title, key, arrow)
	ADD_VISUALSTYLE_METRIC ("segmentmargin")		///< margin (in points) between left border and first segment of an item (icon, title, key, arrow)
	ADD_VISUALSTYLE_METRIC ("explicitrowheight")	///< row height (in points) is not determined by the iconsize and spacing anymore
	ADD_VISUALSTYLE_METRIC ("menuarrowwidth")		///< width (in points) of the arrow that indicates a sub menu
	ADD_VISUALSTYLE_METRIC ("checkmarkwidth")		///< width (in points) used to draw the checkmark icon
END_VISUALSTYLE_CLASS (MenuControl)

DEFINE_IID_ (IMenuControl, 0xd1785e53, 0xee72, 0x4b73, 0x8f, 0x90, 0xf9, 0x5f, 0x88, 0xf0, 0x96, 0xf8)

//************************************************************************************************
// MenuItemPainter
//************************************************************************************************

MenuItemPainter::MenuItemPainter ()
: backColor (Colors::kWhite),
  secondaryBackColor (Colors::kWhite),
  borderColor (Colors::kBlack),
  separatorColor (Colors::kBlack),
  headerColor (Color (Colors::kLtGray).addBrightness (.1f)),
  selectionBackColor (Color (Colors::kYellow).addBrightness (.5f)),
  selectionFrameColor (Colors::kGray),
  textColor (Colors::kBlack),
  disabledTextColor (Colors::kGray),
  selectedTextColor (Colors::kBlack),
  headerTextColor (Colors::kBlack),
  normalIconSize (20),
  largeIconSize (32),
  iconColor (Color (0,0,0,0)),
  selectedIconColor (iconColor),
  spacing (2),
  segmentMargin (2),
  explicitRowHeight (0),
  separatorSpacing (3),
  segmentSpacing (6),
  headerSpacing (2),
  checkMarkWidth (12),
  menuArrowWidth (20),
  closeIconWidth (20),
  maxViewWidth (0),
  maxTitleWidth (0),
  maxKeyWidth (0),
  fixedSubMenuWidth (0),
  checkMarkNeeded (false),
  markupPainter (nullptr)
{
	Theme& theme = FrameworkTheme::instance ();
	font = theme.getThemeFont (ThemeElements::kMenuFont);

	smallFont = font;
	smallFont.setSize (font.getSize () - 1.f);
	//smallFont.isItalic (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItemPainter::~MenuItemPainter ()
{
	safe_release (markupPainter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItemPainter::updateStyle (const IVisualStyle& visualStyle)
{
	font = visualStyle.getFont (StyleID::kTextFont, font);
	smallFont = visualStyle.getFont ("smallfont", smallFont);

	backColor = visualStyle.getBackColor ();
	secondaryBackColor = visualStyle.getColor ("secondarybackcolor", backColor);
	borderColor = visualStyle.getColor ("bordercolor", borderColor);
	separatorColor = visualStyle.getColor ("separatorcolor", textColor);
	headerColor = visualStyle.getColor ("headercolor", headerColor);
	selectionBackColor = visualStyle.getColor ("selectionbackcolor", selectionBackColor);
	selectionFrameColor = visualStyle.getColor ("selectionframecolor", selectionFrameColor);
	textColor = visualStyle.getTextColor ();
	disabledTextColor = visualStyle.getColor ("disabledtextcolor", disabledTextColor);
	headerTextColor = visualStyle.getColor ("headertextcolor", headerTextColor);
	selectedTextColor = visualStyle.getColor ("selectedtextcolor", selectedTextColor);

	setCheckMarkIcon (visualStyle.getImage ("checkmarkicon"));
	if(checkMarkIcon)
		checkMarkWidth = checkMarkIcon->getWidth ();
	setSelectionBarImage (visualStyle.getImage ("selectionbarimage"));

	normalIconSize = visualStyle.getMetric<Coord> ("normaliconsize", normalIconSize);
	largeIconSize = visualStyle.getMetric<Coord> ("largeiconsize", largeIconSize);
	iconColor = visualStyle.getColor ("iconcolor", Color (0,0,0,0)),
	selectedIconColor = visualStyle.getColor ("selectediconcolor", iconColor),
	spacing = visualStyle.getMetric<Coord> ("spacing", spacing);
	separatorSpacing = visualStyle.getMetric<Coord> ("separatorspacing", spacing + 1);
	segmentSpacing = visualStyle.getMetric<Coord> ("segmentspacing", segmentSpacing);
	segmentMargin = visualStyle.getMetric<Coord> ("segmentmargin", segmentMargin);
	headerSpacing = visualStyle.getMetric<Coord> ("headerspacing", headerSpacing);
	explicitRowHeight = visualStyle.getMetric<Coord> ("explicitrowheight", explicitRowHeight);
	
	setMenuArrowIcon (visualStyle.getImage ("menuarrow"));
	if(menuArrowIcon)
		menuArrowWidth = menuArrowIcon->getWidth ();

	setCloseIcon (visualStyle.getImage ("closeicon"));
	if(closeIcon)
		closeIconWidth = closeIcon->getWidth ();

	menuArrowWidth = visualStyle.getMetric<Coord> ("menuarrowwidth", menuArrowWidth);
	closeIconWidth = visualStyle.getMetric<Coord> ("closeiconwidth", closeIconWidth);
	checkMarkWidth = visualStyle.getMetric<Coord> ("checkmarkwidth", checkMarkWidth);
	fixedSubMenuWidth = visualStyle.getMetric<Coord> ("fixedsubmenuwidth", fixedSubMenuWidth);

	if(visualStyle.getMetric<int> ("markup", 0) > 0)
	{
		if(markupPainter == nullptr)
			markupPainter = NEW MarkupPainter ();
	}
	else
		safe_release (markupPainter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItemPainter::ItemType MenuItemPainter::getItemType (const MenuItem& item) const
{
	if(item.isSeparator ())
		return kSeparator;
	if(item.isHeader ())
		return kHeader;
	if(item.getSubMenu () || ccl_cast<ExtendedMenu::ParameterItem> (&item))
		return kSubMenu;
	if(item.getSplitMenu ())
		return kSplitMenu;
	if(ccl_cast<ExtendedMenu::ViewItem> (&item))
		return kViewItem;
	return kRegular;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String MenuItemPainter::getDisplayTitle (const MenuItem& item) const
{
	String title;
	if(Menu* subMenu = item.getSubMenu ())
		title = subMenu->getTitle ();
	else if(!item.isSeparator ())
		title = item.getTitle ();
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItemPainter::MenuVariant MenuItemPainter::getMenuVariant (const MenuItem& item) const
{
	if(Menu* menu = item.getParent ())
		if(menu->getVariant () == Menu::strLargeVariant)
			return kLarge;
	return kNormal;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord MenuItemPainter::getIconSize (MenuVariant variant) const
{
	return variant == kLarge ? largeIconSize : normalIconSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord MenuItemPainter::getItemHeight (MenuVariant variant, ItemType type, const MenuItem& item) const
{
	if(type == kSeparator)
		return separatorSpacing;
	else if(type == kViewItem)
		return ((const ExtendedMenu::ViewItem&)item).getHeight ();
	else
	{
		Coord height = getExplicitRowHeight ();
		if(height > 0)
			return height;
		
		height = getIconSize (variant) + spacing;
		if(item.getIcon ())
			height += spacing;

		if(type == kHeader)
			height += headerSpacing;

		if(!item.getDescription ().isEmpty ())
		{
			Coord descHeight = height;
			descHeight *= 2;
			descHeight /= 3;
			height += descHeight;
		}

		return height;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItemPainter::recalc (const Menu& menu)
{
	maxTitleWidth = 0;
	maxKeyWidth = 0;
	maxViewWidth = 0;
	checkMarkNeeded = false;
	iconSpaceNeeded = false;

	ForEachMenuItem (menu, item)
		if(ExtendedMenu::ViewItem* viewItem = ccl_cast<ExtendedMenu::ViewItem> (item))
		{
			Coord width = viewItem->getWidth ();
			if(width > maxViewWidth)
				maxViewWidth = width;

			continue;
		}

		if(item->isChecked ())
			checkMarkNeeded = true;
		if(item->getIcon ())
			iconSpaceNeeded = true;

		String title = getDisplayTitle (*item);
		Coord titleWidth = Font::getStringWidth (title, font);

		if(titleWidth > maxTitleWidth)
			maxTitleWidth = titleWidth;

		if(!item->getDescription ().isEmpty ())
		{
			Coord descriptionWidth = Font::getStringWidth (item->getDescription (), smallFont);
			if(descriptionWidth > kMaxMenuItemDescriptionWidth)
				descriptionWidth = kMaxMenuItemDescriptionWidth;
			if(descriptionWidth > maxTitleWidth)
				maxTitleWidth = descriptionWidth;
		}

		Coord keyWidth = 0;
		if(const KeyEvent* key = item->getAssignedKey ())
		{
			KeyGlyphPainter painter (font, SolidBrush (textColor));
			#if USE_KEY_GLYPHS
			Rect keyRect = painter.measureKeyGlyphs (*key);
			#else
			Rect keyRect = painter.measureKeyString (*key);
			#endif
			keyWidth = keyRect.getWidth ();
		}

		if(keyWidth > maxKeyWidth)
			maxKeyWidth = keyWidth;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItemPainter::getItemMetrics (ItemMetrics& metrics, const MenuItem& item) const
{
	ItemType type = getItemType (item);
	ASSERT (type != kViewItem)
	metrics.type = type;

	MenuVariant variant = getMenuVariant (item);

	Coord left = segmentMargin;
	metrics.checkPos = left;
	if(checkMarkNeeded)
		left += checkMarkWidth + spacing;

	metrics.iconPos = left;

	Coord iconWidth = 0;
	if(iconSpaceNeeded)
		iconWidth = getIconSize (variant);

	metrics.iconWidth = iconWidth;
	left += iconWidth;
	left += segmentSpacing;
	if(!checkMarkNeeded && !iconSpaceNeeded)
		left += segmentSpacing;

	metrics.titlePos = left;

	left += maxTitleWidth;
	left += segmentSpacing;

	Coord keySpacing = maxKeyWidth + segmentSpacing;
	Coord arrowSpacing = menuArrowWidth + spacing;

	Coord viewWidth = maxViewWidth;
	if(item.getParent () && item.getParent ()->getParent () && fixedSubMenuWidth > 0)
		viewWidth = fixedSubMenuWidth;

	metrics.keyPos = ccl_max (viewWidth - keySpacing - arrowSpacing, left);
	left += keySpacing;

	metrics.arrowPos = ccl_max (viewWidth - arrowSpacing, left);
	left += arrowSpacing;

	metrics.width = ccl_max (viewWidth, left);
	metrics.height = getItemHeight (variant, type, item);

	metrics.rowCount = !item.getDescription ().isEmpty () ? 2 : 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect MenuItemPainter::getItemSize (const MenuItem& item) const
{
	ItemMetrics metrics;
	getItemMetrics (metrics, item);
	return Rect (0, 0, metrics.width, metrics.height);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Color MenuItemPainter::getItemTextColor (const MenuItem& item, int state) const
{
	if(!item.isEnabled ())
		return disabledTextColor;
	if(state == View::kMouseOver)
		return selectedTextColor;
	return textColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItemPainter::drawItemBackground (IGraphics& graphics, RectRef itemSize, const MenuItem& item, int state, bool parentOfCurrentSubMenu) const
{
	bool selected = state == View::kMouseOver && item.isEnabled ();
	if(selected && selectionBarImage)
	{
		Rect src (0, 0, selectionBarImage->getWidth (), selectionBarImage->getHeight ());
		graphics.drawImage (selectionBarImage, src, itemSize);
	}
	else
	{
		bool isInSubMenu = false;
		Menu* parent = item.getParent ();
		if(parent && parent->getParent ())
			isInSubMenu = true;

		Color drawColor = backColor;
		if(selected)
			drawColor = selectionBackColor;
		else if(parentOfCurrentSubMenu || isInSubMenu)
			drawColor = secondaryBackColor;

		graphics.fillRect (itemSize, SolidBrush (drawColor));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItemPainter::drawItem (IGraphics& graphics, RectRef itemSize, const MenuItem& item, int state, bool parentOfCurrentSubMenu) const
{
	ItemMetrics metrics;
	getItemMetrics (metrics, item);

	// Header
	if(metrics.type == kHeader)
	{
		Rect headerRect (itemSize);
		headerRect.top += spacing;
		headerRect.bottom -= spacing;
		graphics.fillRect (headerRect, SolidBrush (headerColor));

		Rect titleRect (itemSize);
		titleRect.left += metrics.titlePos;

		Font font (getFont ());
		font.isBold (true);

		// TODO: item description!

		if(markupPainter)
			markupPainter->drawMarkupString (graphics, titleRect, item.getTitle (), font, SolidBrush (headerTextColor), Alignment::kLeftCenter);
		else
			graphics.drawString (titleRect, item.getTitle (), font, SolidBrush (headerTextColor), Alignment::kLeftCenter);

		return;
	}

	// Background
	bool selected = state == View::kMouseOver && item.isEnabled ();
	drawItemBackground (graphics, itemSize, item, state, parentOfCurrentSubMenu);

	if(metrics.type == kSeparator)
	{
		Point p1 (itemSize.left, itemSize.getCenter ().y);
		Point p2 (itemSize.right, p1.y);
		graphics.drawLine (p1, p2, Pen (separatorColor));
	}
	else
	{
		Color color = getItemTextColor (item, state);

		auto drawMenuIcon = [&](IImage* icon, RectRef srcRect, RectRef dstRect, bool selected, bool isTemplate)
		{
			IImage* modifiedIcon = nullptr;
			
			if(!item.isEnabled ())
				modifiedIcon = ModifiedImageCache::instance ().lookup (icon, getDisabledTextColor ());
			else if(isTemplate && selected && getSelectedIconColor ().getAlphaF () != 0)
				modifiedIcon = ModifiedImageCache::instance ().lookup (icon, getSelectedIconColor ());
			else if(isTemplate && getIconColor ().getAlphaF () != 0)
				modifiedIcon = ModifiedImageCache::instance ().lookup (icon, getIconColor ());
			
			graphics.drawImage (modifiedIcon ? modifiedIcon : icon, srcRect, dstRect);
		};
		
		// Checkmark
		if(item.isChecked ())
		{
			Rect checkRect (itemSize);
			checkRect.left += metrics.checkPos;
			checkRect.right = checkRect.left + checkMarkWidth;

			if(checkMarkIcon)
			{
				Rect src (0, 0, checkMarkIcon->getWidth (), checkMarkIcon->getHeight ());
				Rect dst (src);
				dst.center (checkRect);
				
				bool isTemplate = false;
				if(Image* icon = unknown_cast<Image> (checkMarkIcon))
					isTemplate = icon->getIsTemplate ();
				
				drawMenuIcon (checkMarkIcon, src, dst, selected, isTemplate);
			}
			else
			{
				Rect r (0, 0, 6, 6);
				r.center (checkRect);
				Pen pen (color, 2.f);
				graphics.drawLine (r.getLeftTop (), r.getRightBottom (), pen);
				graphics.drawLine (r.getRightTop (), r.getLeftBottom (), pen);
			}
		}

		// Icon
		if(Image* icon = item.getIcon ())
		{
			bool isTemplate = icon->getIsTemplate (); // remember template state in case icon changes below

			// Icon rules:
			// 1) For images with multiple sizes choose best matching size for menu first.
			// 2) For images with multiple states always draw first frame to get a consistent result.
			icon = ImageResolutionSelector::selectImage (icon, Point (metrics.iconWidth, itemSize.getHeight ()));
			icon->setCurrentFrame (0);
			
			if(icon->getIsTemplate ())
				isTemplate = true;

			Rect iconRect (itemSize);
			iconRect.left += metrics.iconPos;
			iconRect.right = iconRect.left + metrics.iconWidth;

			Rect srcRect;
			icon->getSize (srcRect);
			Rect dstRect (srcRect);
			dstRect.center (iconRect);
			
			drawMenuIcon (icon, srcRect, dstRect, selected, isTemplate);
		}

		// Title
		String title = getDisplayTitle (item);
		if(!title.isEmpty ())
		{
			Rect titleRect (itemSize);
			titleRect.left += metrics.titlePos;
			titleRect.right = titleRect.right + metrics.width;
			if(metrics.rowCount > 1)
				titleRect.setHeight (titleRect.getHeight () / metrics.rowCount);

			Font font (getFont ());
			if(item.isItalic ())
				font.isItalic (true);
			if(metrics.rowCount > 1)
			{
				//font.isUnderline (true);
				font.isBold (true);
			}

			if(markupPainter)
				markupPainter->drawMarkupString (graphics, titleRect, title, font, SolidBrush (color), Alignment::kLeftCenter);
			else
				graphics.drawString (titleRect, title, font, SolidBrush (color), Alignment::kLeftCenter);

			if(metrics.rowCount > 1)
			{
				titleRect.offset (0, titleRect.getHeight ());
				if(markupPainter)
					markupPainter->drawMarkupString (graphics, titleRect, item.getDescription (), getSmallFont (), SolidBrush (color), Alignment::kLeftCenter);
				else
					graphics.drawString (titleRect, item.getDescription (), getSmallFont (), SolidBrush (color), Alignment::kLeftCenter);
			}
		}

		// Key
		if(const KeyEvent* key = item.getAssignedKey ())
		{
			Rect keyRect (itemSize);
			keyRect.left += metrics.keyPos;
			keyRect.right = keyRect.left + maxKeyWidth;

			KeyGlyphPainter painter (font, SolidBrush (color));
			#if USE_KEY_GLYPHS
			painter.drawKeyGlyphs (graphics, keyRect, *key, Alignment::kLeftCenter);
			#else
			painter.drawKeyString (graphics, keyRect, *key, Alignment::kLeftCenter);
			#endif
		}

		// Menu arrow
		if(metrics.type == kSubMenu || metrics.type == kSplitMenu)
			drawMenuArrow (graphics, itemSize, item.isEnabled (), state);
	}

	// Selection
	if(selected)
	{
		graphics.drawRect (itemSize, Pen (selectionFrameColor));
		
		if(metrics.type == kSplitMenu)
		{
			Coord left = metrics.arrowPos - spacing / 2;
			graphics.drawLine (Point (left, itemSize.top + 1), Point (left, itemSize.bottom - 1), Pen (selectionFrameColor));
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect MenuItemPainter::getBackButtonSize (RectRef itemSize) const
{
	Rect buttonRect;
	if(getMenuArrowWidth () > 0)
	{
		buttonRect = itemSize;
		buttonRect.right = buttonRect.left + getMenuArrowWidth ();
		if(!menuArrowIcon) // add spacing only when there is no arrow-image 
			buttonRect.right += getSpacing ();
	}
	return buttonRect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItemPainter::drawBackButton (IGraphics& graphics, RectRef itemSize, const MenuItem& item, int state) const
{
	Rect internalItemSize (itemSize);
	internalItemSize.left += getMenuArrowWidth ();
	drawItem (graphics, internalItemSize, item, 0, false);// true  /*parentOfCurrentSubMenu*/

	Rect arrowRect (getBackButtonSize (itemSize));
	if(arrowRect.getWidth () > 0)
	{
		drawItemBackground (graphics, arrowRect, item, state, false);
		drawMenuArrow (graphics, itemSize, item.isEnabled (), state, true /*drawBackArrow*/);
	}

	drawSeparatorBottom (graphics, itemSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItemPainter::drawCloseButton (IGraphics& graphics, RectRef itemSize, const MenuItem& item, int state) const
{
	drawItem (graphics, itemSize, item, 0, false);

	if(closeIcon)
	{
		Rect iconRect (0, 0, getCloseIconWidth (), getCloseIconWidth ());
		iconRect.align (itemSize, Alignment::kRightCenter);
		iconRect.offset (- (getMenuArrowWidth () - iconRect.getWidth ()) / 2 - getSegmentMargin ()); // align with menu arrows below

		Rect srcRect (0, 0, closeIcon->getWidth (), closeIcon->getHeight ());
		graphics.drawImage (closeIcon, srcRect, iconRect);
	}

	drawSeparatorBottom (graphics, itemSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItemPainter::drawSeparatorBottom (IGraphics& graphics, RectRef itemSize) const
{
	Point p1 (itemSize.getLeftBottom ());
	Point p2 (itemSize.getRightBottom ());
	p1.offset (0, -1);
	p2.offset (0, -1);
	graphics.drawLine (p1, p2, Pen (getSeparatorColor ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuItemPainter::drawMenuArrow (IGraphics& graphics, RectRef itemSize, bool enabled, int state, bool drawBackArrow) const
{
	if(menuArrowWidth <= 0)
		return;
		
	bool selected = state == View::kMouseOver && enabled;
	
	if(menuArrowIcon)
	{
		Rect arrowRect;
		if(drawBackArrow)
		{
			IImage::Selector (menuArrowIcon, ThemeNames::kNormalOn);
			arrowRect = getBackButtonSize (itemSize);
		}
		else
		{
			IImage::Selector (menuArrowIcon, ThemeNames::kNormal);
			arrowRect = itemSize;
			arrowRect.left = itemSize.right - menuArrowIcon->getWidth ();
		}

		arrowRect.setHeight (menuArrowIcon->getHeight ());
		arrowRect.centerV (itemSize);
		
		Rect srcRect (0, 0, menuArrowIcon->getWidth (), menuArrowIcon->getHeight ());
		
		IImage* modifiedIcon = nullptr;
		if(!enabled)
			modifiedIcon = ModifiedImageCache::instance ().lookup (getMenuArrowIcon (), getDisabledTextColor ());
		else if(selected && getSelectedIconColor ().getAlphaF () != 0)
			modifiedIcon = ModifiedImageCache::instance ().lookup (getMenuArrowIcon (), getSelectedIconColor ());
		else
			modifiedIcon = ModifiedImageCache::instance ().lookup (getMenuArrowIcon (), selected ? selectedTextColor : textColor);
			
		graphics.drawImage (modifiedIcon ? modifiedIcon : getMenuArrowIcon (), srcRect, arrowRect);
	}
	else
	{
		  
		Point p[3];
		
		if(drawBackArrow)
		{
			Rect arrowRect (getBackButtonSize (itemSize));
			
			Rect r (0, 0, 4, 8);
		  	r.center (arrowRect);
		  	p[0] = r.getRightTop ();
		  	p[1] = Point (r.left, r.getCenter ().y);
		  	p[2] = r.getRightBottom ();
		}
		else
		{
		  	Rect arrowRect (itemSize);
		  	arrowRect.left = arrowRect.right - menuArrowWidth - spacing;

			Rect r (0, 0, 4, 8);
			r.center (arrowRect);
		 	p[0] = r.getLeftTop ();
		 	p[1] = Point (r.right, r.getCenter ().y);
		 	p[2] = r.getLeftBottom ();
		}
		
		graphics.fillTriangle (p, SolidBrush (enabled ? (selected ? selectedTextColor : textColor) : disabledTextColor));
	}
}

//************************************************************************************************
// MenuControl::ItemButton
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MenuControl::ItemButton, View)

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::ItemButton::ItemButton (MenuItemPainter* painter, MenuItem* item)
: painter (painter),
  item (item),
  subMenuControl (nullptr),
  subPopupSelector (nullptr)
{
	ASSERT (painter != nullptr && item != nullptr)
	
	setTooltip (item->getTooltip ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem* MenuControl::ItemButton::getItem () const
{
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl* MenuControl::ItemButton::getSubMenuControl () const
{
	return subMenuControl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::ItemButton::setSubMenuControl (MenuControl* control)
{
	subMenuControl = control;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::ItemButton::calcAutoSize (Rect& r)
{
	r = painter->getItemSize (*item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::ItemButton::draw (const UpdateRgn& updateRgn)
{
	GraphicsPort graphics (this);
	Rect r;
	getClientRect (r);
	int state = getMouseState ();
	bool parentOfCurrentSubMenu = false;
	if(MenuControl* control = (getParent<MenuControl> ()))
	{
		ItemButton* openButton = control->getClient ()->getOpenSubMenuItem ();
		parentOfCurrentSubMenu = openButton == this;
	}

	painter->drawItem (graphics, r, *item, state, parentOfCurrentSubMenu);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuControl::ItemButton::isClickable () const
{
	MenuItemPainter::ItemType type = painter->getItemType (*item);
	if(type == MenuItemPainter::kRegular || type == MenuItemPainter::kSplitMenu)
		return item->isEnabled ();
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuControl::ItemButton::canOpenSubMenu () const
{
	MenuItemPainter::ItemType type = painter->getItemType (*item);
	if(type == MenuItemPainter::kSubMenu || type == MenuItemPainter::kSplitMenu)
		return item->isEnabled ();
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuControl::ItemButton::onMouseDown (const MouseEvent& event)
{
	// for touch input, prefer subMenu (of a a split menu) in the right half
	bool preferSubMenu = event.wasTouchEvent () && canOpenSubMenu () && event.where.x > getWidth () / 2;

	if(isClickable () && !preferSubMenu)
	{
		select ();
	}
	else if(canOpenSubMenu ())
	{
		ClientView* client = getParent<ClientView> ();
		if(client)
		{
			ItemButton* openSibling = client->getOpenSubMenuItem ();
			if(this == openSibling) // already open
				return true;

			if(!client->getRootMenuControl ()->isTimerEnabled ()) // e.g. on touch input (windows doesn't always send touch input in nested popups)
			{
				if(openSibling)
					openSibling->closeSubMenu ();

				// open subMenu deferred, after sibling menu has closed (avoid deeply nested dialog call stacks of old subMenus)
				client->setCurrentItem (this, false);
				(NEW Message ("popupSubMenu", false))->post (getParent<MenuControl> ());
				return true;
			}
		}
		popupSubMenu ();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuControl::ItemButton::popupSubMenu (bool keyNavigation)
{
	Point pos (getSize ().getWidth () - kSubMenuOverlap, 0);

	Menu* subMenu = item->getSubMenu ();
	if(subMenu == nullptr)
		subMenu = item->getSplitMenu ();
	if(subMenu)
	{
		MenuControl* parentControl = getParent<MenuControl> ();
		AutoPtr<MenuControl> control = NEW MenuControl (subMenu, parentControl ? parentControl->getMenuStyle () : nullptr);
		control->setParentControl (parentControl);

		// select first item on key navigation
		if(keyNavigation)
			control->getClient ()->selectNextItem (true);

		ScopedVar<MenuControl*> scope (subMenuControl, control);
		control->popup (pos, this);

		if(item->getSplitMenu ())
		{
			// after split menu closed: close parent menu if split item was selected (clicked) 
			if(parentControl && parentControl->getResultItem () == item)
				parentControl->closeAll (true);
		}
	}
	// try parameter popup
	else if(ExtendedMenu::ParameterItem* paramItem = ccl_cast<ExtendedMenu::ParameterItem> (item))
	{
		PopupSelector selector;
		selector.setTheme (getTheme ());
		selector.setVisualStyle (getTheme ().getStandardStyle (ThemePainter::kMenuControlStyle));
		selector.setMenuMode (true);

		PopupSizeInfo sizeInfo (pos, this);
		sizeInfo.canFlipParentEdge (true);

		ScopedVar<PopupSelector*> scope (subPopupSelector, &selector);
		selector.popup (paramItem->getParameter (), sizeInfo, MenuPresentation::kExtended);

		// kOk or kCancel on parameter popup: close all
		if(selector.getPopupResult () != PopupClient::kIgnore) // (we set kIgnore when closing in closeSubMenu)
			if(MenuControl* control = (getParent<MenuControl> ()))
				control->closeAll ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::ItemButton::select ()
{
	ClientView* client = getParent<ClientView> ();
	if(client)
		client->setClickedItem (item);

	// when clicked on split menu item with open submenu: set as result item before closing
	if(item->getSplitMenu () && client && client->getOpenSubMenuItem ())
		if(MenuControl* parentControl = getParent<MenuControl> ())
			parentControl->setResultItem (item);

	SharedPtr<Object> holder (this);
	closeSubMenu (); // close split menu

	// if the popup stays open on click, select the clicked item
	if(MenuControl* control = (getParent<MenuControl> ()))
		if(control->getPopupClient ()->isIgnoringMouseClick ())
			item->select ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::ItemButton::closeSubMenu ()
{
	// first close child subMenus (deep)
	if(subMenuControl)
		if(ItemButton* openChild = subMenuControl->getClient ()->getOpenSubMenuItem ())
			openChild->closeSubMenu ();

	closeSubMenuInternal ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::ItemButton::closeSubMenuInternal ()
{
	UnknownPtr<IPopupSelectorWindow> window;
	if(subMenuControl)
		window = ccl_as_unknown (subMenuControl->getWindow ());
	else if(subPopupSelector)
		window = subPopupSelector->getCurrentWindow ();

	if(window)
	{
		window->setPopupResult (IPopupSelectorClient::kIgnore); // don't close parent menus in onPopupClosed
		window->closePopup ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuControl::ItemButton::isSubMenuOpen () const
{
	return subMenuControl != nullptr || subPopupSelector != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuControl::ItemButton::onNavigate (const KeyEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* MenuControl::ItemButton::getAccessibilityProvider () 
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW MenuItemButtonAccessibilityProvider (*this);
	return accessibilityProvider;
}

//************************************************************************************************
// MenuItemButtonAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MenuItemButtonAccessibilityProvider, ViewAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItemButtonAccessibilityProvider::MenuItemButtonAccessibilityProvider (MenuControl::ItemButton& owner)
: ViewAccessibilityProvider (owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::ItemButton& MenuItemButtonAccessibilityProvider::getItemButton () const
{
	return static_cast<MenuControl::ItemButton&> (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API MenuItemButtonAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kMenuItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MenuItemButtonAccessibilityProvider::getElementName (String& name) const
{
	MenuControl::ItemButton& button = getItemButton ();
	MenuItem* item = button.getItem ();
	if(item)
	{
		name = item->getTitle ();
		if(name.isEmpty ())
			name = item->getTooltip ();
	}
	if(name.isEmpty ())
		SuperClass::getElementName (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MenuItemButtonAccessibilityProvider::performAction ()
{
	MenuControl::ItemButton& button = getItemButton ();
	SharedPtr<Unknown> lifeGuard (&button);
	if(!button.isClickable ())
		return kResultFailed;
	button.select ();
	return kResultOk;
}

//************************************************************************************************
// MenuControl::ClientView
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MenuControl::ClientView, View)

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::ClientView::ClientView (Menu* menu, VisualStyle* menuStyle)
: menu (menu),
  painter (NEW MenuItemPainter),
  clickedItem (nullptr),
  wasKeyNavigation (false),
  pageBreakIndex (-1),
  nextSubMenuCheck (NumericLimits::kMaxInt64),
  margin (2)
{
	if(menuStyle == nullptr)
		menuStyle = ThemePainter::getStandardStyle (ThemePainter::kMenuControlStyle);
	if(menuStyle)
	{
		painter->updateStyle (*menuStyle);
		margin = menuStyle->getMetric<Coord> ("margin", margin);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::ClientView::construct ()
{
	bool wasSeparator = false;

	ASSERT (menu != nullptr)
	ForEachMenuItem (*menu, item)
		if(ExtendedMenu::ViewItem* viewItem = ccl_cast<ExtendedMenu::ViewItem> (item))
		{
			View* view = unknown_cast<View> (viewItem->getView ());
			ASSERT (view)
			addView (return_shared (view));
		}
		else
		{
			if(item->getSubMenu () && item->getSubMenu ()->isEmpty ())
				continue; // don't add buttons for empty sub menus

			if(wasSeparator && item->isSeparator ())
				continue; // avoid successive separators

			ItemButton* button = createItemButton (item);
			addView (button);
		}
		wasSeparator = item->isSeparator ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::ItemButton* MenuControl::ClientView::createItemButton (MenuItem* item)
{
	return NEW ItemButton (painter, item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Menu* MenuControl::ClientView::getMenu () const
{
	return menu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::ItemButton* MenuControl::ClientView::getCurrentItem () const
{
	return currentItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::ItemButton* MenuControl::ClientView::getOpenSubMenuItem ()
{
	ForEachViewFast (*this, view)
		if(ItemButton* button = ccl_cast<ItemButton> (view))
			if(button->isSubMenuOpen ())
				return button;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::ItemButton* MenuControl::ClientView::findSubMenuItem (StringRef name) const
{
	ForEachViewFast (*this, view)
		if(auto button = ccl_cast<ItemButton> (view))
			if(auto item = button->getItem ())
				if(auto subMenu = item->getSubMenu ())
					if(subMenu->getName () == name)
						return button;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl* MenuControl::ClientView::getRootMenuControl ()
{
	MenuControl* control = getParent<MenuControl> ();
	if(control)
		while(MenuControl* parent = control->getParentControl ())
			control = parent;
	return control;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::ClientView::setCurrentItem (ItemButton* button, bool keyNavigation)
{
	if(button != currentItem)
	{
		CCL_PRINTF ("MenuControl::setCurrentItem: %s\n", button ? MutableCString (painter->getDisplayTitle (*button->getItem ())).str () : "0")
		int timeout = 0;

		if(currentItem)
		{
			currentItem->setMouseState (kMouseNone);

			// check if leaving an open subMenu button
			if(currentItem->isSubMenuOpen ())
				timeout = kSubMenuCloseTimeOut;
		}

		currentItem = button;

		if(currentItem)
		{
			currentItem->setMouseState (kMouseOver);
			MenuControl* control = getParent<MenuControl> ();
			Coord y = currentItem->getPosition ().y - control->getVScrollParam ()->getValue ().asInt ();
			if(y < 0)
				control->scrollByV (y);
			else if(y + currentItem->getHeight () > control->getHeight ())
				control->scrollByV (y + currentItem->getHeight () - control->getHeight ());

			// also update result item, if there is already one
			if(getClickedItem () && currentItem->isClickable ())
				setClickedItem (currentItem->getItem ());

			if(timeout == 0 && currentItem->canOpenSubMenu ())
				timeout = kSubMenuOpenTimeOut;
		}

		if(Window* window = getWindow ())
			window->redraw ();

		if(timeout)
			nextSubMenuCheck = System::GetSystemTicks () + timeout;
	}
	wasKeyNavigation = keyNavigation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::ClientView::setClickedItem (MenuItem* item)
{
	clickedItem = item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::ItemButton* MenuControl::ClientView::getNextSelectableItem (Direction direction)
{
	int inc = 0;
	
	View* view = getCurrentItem ();
	int currentIndex = view ? index (view) : -1;
	int targetIndex = currentIndex;
	
	switch (direction)
	{
		case kMoveLeft:
		{
			if(pageBreakIndex == -1 || targetIndex < pageBreakIndex)
				return nullptr;
			
			inc = -1;
			if(targetIndex >= (2 * pageBreakIndex))
				targetIndex = (2 * pageBreakIndex - 1);
			
			targetIndex -= pageBreakIndex;
			
			view = getFirst ();
			int firstIndex = view ? index (view) : -1;
			if(targetIndex < firstIndex)
				targetIndex = firstIndex;
			
			break;
		}
		case kMoveUp:
		{
			inc = -1;
			targetIndex += inc;
			break;
		}
		case kMoveRight:
		{
			if(pageBreakIndex == -1 || targetIndex >= pageBreakIndex)
				return nullptr;
			
			inc = 1;
			targetIndex += pageBreakIndex;
			
			view = getLast ();
			int lastIndex = view ? index (view) : -1;
			if(targetIndex > lastIndex)
				targetIndex = lastIndex;

			break;
		}
		case kMoveDown:
		{
			inc = 1;
			targetIndex += inc;
			break;
		}
	}
	
	do
	{
		view = getChild (targetIndex);
		if(!view)
		{
			view = (inc > 0) ? getFirst () : getLast ();
			targetIndex = view ? index (view) : -1;
		}
		if(targetIndex == currentIndex)
			break;
		
		ItemButton* button = ccl_cast<ItemButton> (view);
		if(button && (button->isClickable () || button->canOpenSubMenu ()))
			return button;
		
		targetIndex += inc;
		if(currentIndex < 0 && (targetIndex  < 0 || targetIndex >= views.count ()))
			break;

	} while (true);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuControl::ClientView::selectNextItem (bool keyNavigation)
{
	if(ItemButton* firstButton = getNextSelectableItem (ClientView::kMoveDown))
	{
		setCurrentItem (firstButton, keyNavigation);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::ClientView::checkSubMenus ()
{
	ItemButton* currentButton = getCurrentItem ();
	ItemButton* openButton = getOpenSubMenuItem ();

	if(nextSubMenuCheck != NumericLimits::kMaxInt64)
	{
		int64 now = System::GetSystemTicks ();
		if(now >= nextSubMenuCheck)
		{
			if(openButton && openButton != currentButton)
			{
				CCL_PRINTF ("MenuControl: close subMenu (%s)\n", openButton ? MutableCString (painter->getDisplayTitle (*openButton->getItem ())).str () : "0")
				openButton->closeSubMenu ();
				return; // might open another one in next check
			}
			else if(currentButton && currentButton->canOpenSubMenu () && !currentButton->isSubMenuOpen () && !wasKeyNavigation)
			{
				CCL_PRINTF ("MenuControl: open subMenu (%s)\n", currentButton ? MutableCString (painter->getDisplayTitle (*currentButton->getItem ())).str () : "0")
				currentButton->popupSubMenu ();
				return;
			}
			nextSubMenuCheck = NumericLimits::kMaxInt64;
		}
	}

	if(openButton)
		if(MenuControl* subControl = openButton->getSubMenuControl ())
			if(ClientView* subClient = subControl->getClient ())
				subClient->checkSubMenus ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::ClientView::updateSize ()
{
	painter->recalc (*menu);

	static const int kReasonableMenuHeight = 650;

	// recalc items
	const Coord spacing = painter->getSpacing ();
	Coord top = margin;
	Coord topMargin = margin;
	Coord totalWidth = 0;
	int itemIndex = 0;
	int numberOfHeaderItems = 0;
	pageBreakIndex = -1;
	
	ForEachViewFast (*this, view)
		if(ItemButton* button = ccl_cast<ItemButton> (view))
		{
			if(button->getItem ()->isHeader ())
			{
				if(itemIndex == 0)
					topMargin -= spacing;
				numberOfHeaderItems += 1;
			}
			
			button->autoSize ();
		}

		if(view->getWidth () > totalWidth)
			totalWidth = view->getWidth ();
	
		top += view->getHeight ();
		itemIndex++;
	EndFor


	if(top > kReasonableMenuHeight && numberOfHeaderItems > 1)
	{
		int wrapItemCandidate = -1;
		int lastWrapOffset = NumericLimits::kMaxInt;
		int idealWrapIndex = itemIndex / 2;
		itemIndex = 0;
		
		ForEachViewFast (*this, view)
			if(ItemButton* button = ccl_cast<ItemButton> (view))
			{
				if(button->getItem ()->isHeader ())
				{
					if(itemIndex > 0) // at least one item before page break
					{
						wrapItemCandidate = itemIndex;
		
						int wrapOffset = ccl_abs (idealWrapIndex - wrapItemCandidate);
						if(wrapOffset < lastWrapOffset)
						{
							lastWrapOffset = wrapOffset;
							pageBreakIndex = wrapItemCandidate;
						}
					}
				}
			}
			itemIndex++;
		EndFor
	}

	Rect totalSize;

	// layout items
	top = topMargin;
	Coord left = margin;
	bool isWrapItem = false;
	itemIndex = 0;
	ForEachViewFast (*this, view)
		Rect r (view->getSize ());
		r.setWidth (totalWidth);
	
		if(pageBreakIndex == itemIndex)
		{
			left += totalWidth + spacing;
			top = topMargin;
		}
	
		r.moveTo (Point (left, top));
		view->setSize (r);

		totalSize.join (r);
		top += r.getHeight ();
		itemIndex++;
	EndFor

	// resize client
	totalSize.right += margin;
	totalSize.bottom += margin;

	// disable size mode while resizing to fit childs (as in autoSize)
	bool wasDisabled = isSizeModeDisabled ();
	disableSizeMode (true);
	setSize (totalSize);
	disableSizeMode (wasDisabled);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::ClientView::draw (const UpdateRgn& updateRgn)
{
	GraphicsPort graphics (this);
	Rect r;
	getClientRect (r);
	Color color = painter->getBackColor ();
	if(getParent ())
		color = painter->getSecondaryBackColor ();

	graphics.fillRect (r, SolidBrush (color));

	SuperClass::draw (updateRgn);

	graphics.drawRect (r, Pen (painter->getBorderColor ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::ClientView::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	// update painter when visual style changes
	if(MenuControl* control = getParent<MenuControl> ())
	{
		VisualStyle* menuStyle = control->getMenuStyle ();
		if(!menuStyle)
			menuStyle = ThemePainter::getStandardStyle (ThemePainter::kMenuControlStyle);
		
		if(menuStyle)
			if(menuStyle->hasReferences (event.scheme))
				painter->updateStyle (*menuStyle);		
	}

	SuperClass::onColorSchemeChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* MenuControl::ClientView::getAccessibilityProvider () 
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW MenuClientAccessibilityProvider (*this);
	return accessibilityProvider;
}

//************************************************************************************************
// MenuClientAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MenuClientAccessibilityProvider, ViewAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuClientAccessibilityProvider::MenuClientAccessibilityProvider (MenuControl::ClientView& owner)
: ViewAccessibilityProvider (owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::ClientView& MenuClientAccessibilityProvider::getClientView () const
{
	return static_cast<MenuControl::ClientView&> (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API MenuClientAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kMenu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MenuClientAccessibilityProvider::getElementName (String& name) const
{
	MenuControl::ClientView& clientView = getClientView ();
	Menu* menu = clientView.getMenu ();
	if(menu)
		name = menu->getTitle ();
	if(name.isEmpty ())
		SuperClass::getElementName (name);
}

//************************************************************************************************
// MenuControl
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MenuControl, ScrollView)

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::MenuControl (Menu* menu, VisualStyle* menuStyle, View* target, StyleRef scrollStyle)
: ScrollView (Rect (), target, scrollStyle),
  menuStyle (menuStyle),
  parentControl (nullptr),
  popupClient (nullptr)
{
	if(target == nullptr)
	{
		target = NEW ClientView (menu, menuStyle);
		this->target = target;
		static_cast<ClientView*> (target)->construct ();
		savedTargetSize = target->getSize ().getSize ();
		ScrollView::construct ();
		ScrollView::checkAutoHide ();
	}

	popupClient = NEW PopupClient (*this);
	updateSize ();

	GUI.getMousePosition (initialMousePos);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::~MenuControl ()
{
	cancelSignals ();

	popupClient->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle* MenuControl::getMenuStyle () const
{
	return menuStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::ClientView* MenuControl::getClient () const
{
	return (ClientView*)getTarget ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient* MenuControl::getPopupClient () const
{
	return popupClient;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuControl::isTopLevel () const
{
	return parentControl == nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::updateSize ()
{
	ClientView* client = getClient ();
	client->updateSize ();

	// limit to monitor height
	Point mousePos;
	GUI.getMousePosition (mousePos);
	int monitor = Desktop.findMonitor (mousePos, true);
	Rect monitorSize;
	Desktop.getMonitorSize (monitorSize, monitor, true);

	Rect size (client->getSize ());
	Coord h = size.getHeight ();
	if(View* header = getHeader ())
		h += header->getHeight ();

	ccl_upper_limit (h, monitorSize.getHeight ());
	ccl_upper_limit (h, client->getMaxControlHeight ()); // client can constrain further
	size.setHeight (h);
	setSize (size.moveTo (getSize ().getLeftTop ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord MenuControl::ClientView::getMaxControlHeight () const
{
	return kMaxCoord;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuControl::popup (const Point& where, View* view)
{
	ASSERT (!isTopLevel ()) // only called for a subMenu!

	ClientView* clientView = getClient ();
	Menu* menu = clientView->getMenu ();
	if(menu->isEmpty ())
		return 0;

	updateSize (); // menu dimension might have changed

	PopupSelector selector;
	selector.setTheme (getTheme ());
	selector.setVisualStyle (getTheme ().getStandardStyle (ThemePainter::kMenuControlStyle));
	selector.setMenuMode (true);

	PopupSizeInfo sizeInfo (where, view);
	sizeInfo.canFlipParentEdge (true);
	return selector.popup (return_shared (this), popupClient, sizeInfo) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::closeAll (bool deferred)
{
	if(deferred)
	{
		KeyState keyState;
		GUI.getKeyState (keyState);
		(NEW Message ("closeAll", keyState.keys))->post (this);
	}
	else
	{
		KeyState keyState;
		GUI.getKeyState (keyState);
		closeAllInternal (keyState);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::closeAllInternal (KeyState keyState)
{
	CCL_PRINTF ("closeAllInternal: mouse pressed: %d\n", keyState.isSet (KeyState::kMouseMask))

	// if mouse is pressed, find mouse window
	IWindow* clickedWindow = nullptr;
	Point mousePos;
	if(keyState.isSet (KeyState::kMouseMask))
	{
		GUI.getMousePosition (mousePos);
		clickedWindow = Desktop.findWindow (mousePos);
	}
	MenuControl* control = this;
	while(control)
	{
		Window* window = control->getWindow ();
		if(!window)
			break;

		if(window == clickedWindow) // don't close the clicked one
		{
		 	window->screenToClient (mousePos);
			MouseEvent event (MouseEvent::kMouseDown, mousePos, keyState);
			window->onMouseDown (event);
			break;
		}
				
		control = control->getParentControl ();
		window->close ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::ClientView* MenuControl::getActiveClientView ()
{
	return getClient ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuControl::navigate (const KeyEvent& event)
{
	ClientView* client = getActiveClientView ();
	MenuControl* control = client->getParent<MenuControl> ();
	#if DEBUG
	bool subMenuOpen = client->getOpenSubMenuItem () != nullptr;
	bool subViewHasFocus = client != getClient ();
	ASSERT (!subMenuOpen || !subViewHasFocus) // keys should go to the deepest modal popup
	#endif

	suspendMouseTracking ();

	switch(event.vKey)
	{
	case VKey::kUp:
	case VKey::kDown:
		if(ItemButton* button = client->getNextSelectableItem ((event.vKey == VKey::kDown) ? ClientView::kMoveDown : ClientView::kMoveUp))
		{
			client->setCurrentItem (button, true);
			return true;
		}
		else
			return false;

	case VKey::kRight: // open subMenu
		if(ItemButton* currentButton = client->getCurrentItem ())
		{
			if(currentButton->canOpenSubMenu ())
			{
				if(currentButton->isSubMenuOpen ())
					(NEW Message ("focusSubMenu"))->post (control);
				else
					(NEW Message ("popupSubMenu"))->post (control);
				return true;
			}
			else if(ItemButton* button = client->getNextSelectableItem (ClientView::kMoveRight))
			{
				client->setCurrentItem (button, true);
				return true;
			}
		}

		// finally delegate to MenuBarControl
		if(auto* menuBarControl = MenuBarControl::getActiveControl ())
			menuBarControl->onKeyDown (event);
		return true;

	case VKey::kLeft: // close subMenu
		if(ItemButton* button = client->getNextSelectableItem (ClientView::kMoveLeft))
			client->setCurrentItem (button, true);
		else if(MenuControl* parentControl = control->getParentControl ())
		{
			if(parentControl->getClient ()->getOpenSubMenuItem ())
				(NEW Message ("closeSubMenu"))->post (parentControl);
		}
		else if(getClient ()->getOpenSubMenuItem ())
		{
			(NEW Message ("closeSubMenu"))->post (control);
		}

		// finally delegate to MenuBarControl
		else if(auto* menuBarControl = MenuBarControl::getActiveControl ())
			menuBarControl->onKeyDown (event);
		return true;

	case VKey::kEnter:
	case VKey::kReturn:
		// a) close on enter/return on a focus view with a NativeTextControl (EditBox, ComboBox)
		//    that has the kEditBoxDialogBehavior option (in a ViewItem)
		if(Window* window = getWindow ())
		{
			if(View* focusView = window->getFocusView ())
			{
				UnknownPtr<ITextParamProvider> textParamProvider (static_cast<IObject*> (focusView));
				if(textParamProvider && focusView->getStyle ().isCustomStyle (Styles::kEditBoxBehaviorDialogEdit))
				{
					closeAll ();
					return true;
				}
			}
		}

		// b) open submenu
		if(ItemButton* button = client->getCurrentItem ())
		{
			if(button->canOpenSubMenu ())
			{
				(NEW Message ("popupSubMenu"))->post (control);
				return true;
			}
			else if(button->onNavigate (event))
				return true;
		}
		break;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MenuControl::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "popupSubMenu")
	{
		if(ItemButton* currentButton = getClient ()->getCurrentItem ())
		{
			if(currentButton->canOpenSubMenu () && !currentButton->isSubMenuOpen ())
			{
				bool keyNavigation = true;
				if(msg.getArgCount () > 0 && msg.getArg (0).asBool () == false)
					keyNavigation = false;

				currentButton->popupSubMenu (keyNavigation);
			}
		}
	}
	else if(msg == "closeSubMenu")
	{
		if(ItemButton* openButton = getClient ()->getOpenSubMenuItem ())
		{
			getClient ()->setCurrentItem (openButton, true); // prevent reopen in idle
			openButton->closeSubMenu ();
		}
	}
	else if(msg == "closeAll")
	{
		KeyState keyState (msg[0].asInt ());
		closeAllInternal (keyState);
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* MenuControl::findActiveMouseView (Window* mouseWindow, PointRef mousePos)
{
	return mouseWindow->findView (mousePos, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::trackItem (Window* mouseWindow, PointRef mousePos)
{
	if(mouseWindow && !mouseWindow->getMouseHandler () && mousePos != initialMousePos)
	{
		// to prevent closing of mouseWindow via timeout (e.g. on fast diagonal move into subMenu), set parent ItemButton as current
		View* mouseView = findActiveMouseView (mouseWindow, mousePos);
		auto mouseControl = mouseView ? mouseView->getParent<MenuControl> () : nullptr;
		if(mouseControl)
		{
			if(MenuControl* parentControl = mouseControl->getParentControl ())
				if(ItemButton* openButton = parentControl->getClient ()->getOpenSubMenuItem ())
					parentControl->getClient ()->setCurrentItem (openButton, false);
		
			// set current ItemButton under mouse (null for a custom view)
			auto mouseButton = ccl_cast<ItemButton> (mouseView);
			if(ClientView* clientView = mouseControl->getClient ())
				clientView->setCurrentItem (mouseButton, false);
		}
	}

	if(ClientView* client = getClient ())
		client->checkSubMenus ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::suspendMouseTracking ()
{
	if(MenuControl* parentControl = getParentControl ())
		parentControl->suspendMouseTracking ();
	else
		GUI.getMousePosition (initialMousePos);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* MenuControl::findActiveMouseWindow ()
{
	if(Window* topModal = Desktop.getTopWindow (kPopupLayer))
	{
		// ignore if topmodal window is not a MenuControl (e.g. popped up from a SelectBox in a ViewItem)
		UnknownPtr<IPopupSelectorClient> topClient (topModal->asUnknown ());
		if(unknown_cast<MenuControl::PopupClient> (topClient) == nullptr)
			return nullptr;
	}

	// find window under mouse
	return unknown_cast<Window> (Desktop.findWindowUnderCursor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::onIdleTimer ()
{
	ASSERT (isTopLevel ())
	Point mousePos;
	GUI.getMousePosition (mousePos);
	if(mousePos != initialMousePos) // prevent conflict between mouse move & key navigation
	{
		if(Window* mouseWindow = findActiveMouseWindow ())
		{
			Point clientPos (mousePos);
			mouseWindow->screenToClient (clientPos);
			if(UnknownPtr<IPopupSelectorWindow> (mouseWindow->asUnknown ()).isValid ())
			{
				trackItem (mouseWindow, clientPos);
			}
			else if(auto* menuBarControl = ccl_cast<MenuBarControl> (mouseWindow->findView (clientPos, true)))
			{
				// mouse is in menu bar control: switch to other menu under mouse
				menuBarControl->windowToClient (clientPos);
				int mouseMenuIndex = menuBarControl->findMenu (clientPos);
				int activeMenuIndex = menuBarControl->getActiveIndex ();

				if(mouseMenuIndex != activeMenuIndex && mouseMenuIndex >= 0)
				{
					closeAll ();
					menuBarControl->activateMenu (mouseMenuIndex);
				}
			}
		}
	}
}

//************************************************************************************************
// MenuControl::PopupClient
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (MenuControl::PopupClient, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::PopupClient::PopupClient (MenuControl& control)
: control (control),
  cancelOnMouseUp (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuControl::PopupClient::hasPopupResult ()
{
	if(ClientView* client = control.getActiveClientView ())
		if(ItemButton* button = client->getCurrentItem ())
			return button->isClickable ();

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuControl::PopupClient::closeAll (bool deferred)
{
	control.closeAll (deferred);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MenuControl::PopupClient::attached (IWindow& popupWindow)
{
	PopupSelectorClient::attached (popupWindow);

	if(control.isTopLevel ())
	{
		// toplevel control has idle timer
		control.startTimer ();

		// select first item if opened from a MenuBarControl via key navigation
		auto* menuBarControl = MenuBarControl::getActiveControl ();
		if(menuBarControl && menuBarControl->isInKeyEvent ())
			control.getClient ()->selectNextItem (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MenuControl::PopupClient::onPopupClosed (Result result)
{
	CCL_PRINTF ("MenuControl::PopupClient::onPopupClosed (%d) \t%d x %d\n", result, control.getWidth (), control.getHeight ())
	control.stopTimer ();

	if(result == kOkay || result == kCancel)
	{
		ClientView* client = control.getActiveClientView ();

		if(result == kOkay) // no clickedItem is set yet when selected via mouseUp
			if(client->getClickedItem () == nullptr && client->getCurrentItem () && client->getCurrentItem ()->isClickable ())
				client->setClickedItem (client->getCurrentItem ()->getItem ());

		// also close parent windows
		if(MenuControl* parentControl = control.getParentControl ())
			parentControl->closeAll ();

		if(result == kOkay && client && client->getClickedItem ())
			client->getRootMenuControl ()->setResultItem (client->getClickedItem ());
			//client->getClickedItem ()->select (); do not select while still in modal dialog loop!
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPopupSelectorClient::Result CCL_API MenuControl::PopupClient::onKeyDown (const KeyEvent& event)
{
	Result result = PopupSelectorClient::onKeyDown (event);
	if(result == kIgnore)
	{
		result = kSwallow;

		if(!control.navigate (event))
		{
			Menu* menu = control.getClient ()->getMenu ();
			if(MenuItem* item = menu->findItemWithKey (event))
			{
				control.getClient ()->setClickedItem (item);
				result = kOkay;
			}
			else
			{
				if(Window* window = control.getWindow ())
					if(View* focusView = window->getFocusView ())
						if(!ccl_cast<ItemButton> (focusView))
							return kIgnore; // will pass event to view tree
			}
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPopupSelectorClient::Result CCL_API MenuControl::PopupClient::onKeyUp (const KeyEvent& event)
{
	return kSwallow;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API MenuControl::PopupClient::onMouseDown (const MouseEvent& event, IWindow& window)
{
	if(event.wasTouchEvent () || event.wasPenEvent ())
	{
		if(control.isTopLevel ())
			control.stopTimer (); // turn off timer based opening/closing of submenus for touch input

		// for touch input: ensure currentItem is updated before onMouseUp
		// (GUI mousePos has just been updated for the first time since popup opened)
		auto popupWindow = unknown_cast<Window> (&window);
		if(UnknownPtr<IPopupSelectorWindow> (ccl_as_unknown (popupWindow)).isValid ())
			control.trackItem (popupWindow, event.where);
	}
	
	return PopupSelectorClient::onMouseDown (event, window);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API MenuControl::PopupClient::onMouseUp (const MouseEvent& event, IWindow& window)
{
	if(isCancelOnMouseUp ())
		return kCancel;

	if(event.wasTouchEvent () && control.getClient ()->getOpenSubMenuItem () && !ccl_cast<CompactMenuControl> (&control))
		return kIgnore;

	// ignore if clicked outside client view (e.g. scrollbar / scrollbutton)
	auto popupWindow = unknown_cast<Window> (&window);
	if(!popupWindow->findView (event.where, AutoPtr<IRecognizer> (Recognizer::create ([] (IUnknown* unk) { return unknown_cast<MenuControl::ClientView> (unk) != nullptr; }))))
		return kIgnore;

	return PopupSelectorClient::onMouseUp (event, window);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MenuControl::PopupClient::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IParameter::kUpdateMenu)
	{
		if(control.isTopLevel ())
		{
			Menu* menu = control.getClient ()->getMenu ();
			ParameterMenuBuilder* builder = menu ? ParameterMenuBuilder::extractBuilder (*menu) : nullptr;
			if(builder)
			{
				// note: this is a very incomplete implementation (compared to MenuPopupSelector):
				// instead of rebuilding the whole menu (would be diffcult when submenus are open),
				// we just take the "checked" states from a new built temporary menu
				auto findItembyName = [] (const Menu& menu, StringRef name) -> MenuItem*
				{
					for(int i = 0, numItems = menu.countItems (); i < numItems; i++)
						if(MenuItem* item = menu.at (i))
							if(item->getName () == name)
								return item;

					return nullptr;
				};

				AutoPtr<Menu> newMenu (builder->buildMenu ());
				if(newMenu)
					for(int i = 0, numItems = newMenu->countItems (); i < numItems; i++)
						if(MenuItem* newItem = newMenu->at (i))
							if(MenuItem* existingItem = findItembyName (*menu, newItem->getName ()))
								existingItem->check (newItem->isChecked ());

				control.getClient ()->invalidate ();
			}
		}
	}
	SuperClass::notify (subject, msg);
}

//************************************************************************************************
// CompactMenuControl
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CompactMenuControl, MenuControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

CompactMenuControl::CompactMenuControl (Menu* menu, VisualStyle* menuStyle)
: MenuControl (menu, menuStyle, NEW ClientView (menu, menuStyle), StyleFlags (Styles::kSmall|Styles::kTransparent, Styles::kScrollViewBehaviorExtendTarget|Styles::kScrollViewBehaviorAutoHideVBar))
{
	ClientView* client = getCompactClient ();

	getClient ()->construct ();
	updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::ClientView* CompactMenuControl::getActiveClientView ()
{
	ClientView* client = getCompactClient ();
	ClientView* activeClient = client ? client->getActiveClientView () : nullptr;
	CCL_PRINTF ("getActiveClientView: %d\n", activeClient ? activeClient->getDepth () : -666);
	return activeClient;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Window* CompactMenuControl::findActiveMouseWindow ()
{
	Window* result = SuperClass::findActiveMouseWindow ();
	if(result == nullptr)
	{
		ClientView* client = getCompactClient ();
		client->setCurrentItem (nullptr, false);

		if(ClientView* subClient = client->getSubClient ())
			subClient->setCurrentItem (nullptr, false);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CompactMenuControl::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "focusSubMenu")
	{
		ClientView* client = getCompactClient ();
		if(ClientView* subClient = client->getSubClient ())
			subClient->selectNextItem (true);
	}
	else
		SuperClass::notify (subject, msg);
}

//************************************************************************************************
// CompactMenuControl::ClientView
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CompactMenuControl::ClientView, MenuControl::ClientView)

//////////////////////////////////////////////////////////////////////////////////////////////////

CompactMenuControl::ClientView::ClientView (Menu* menu, VisualStyle* menuStyle)
: MenuControl::ClientView (menu, menuStyle),
  minSize (kMinCoord, kMinCoord),
  maxWidth (kMaxCoord),
  minColumnHeight (0),
  depth (0),
  inUpdateSize (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::ClientView::initWithParent (const ClientView& parentView)
{
	painter->setFixedSubMenuWidth (parentView.getPainter ()->getFixedSubMenuWidth ());

	if(painter->getFixedSubMenuWidth () > 0)
	{
		ForEachMenuItem (*menu, item)
			if(auto viewItem = ccl_cast<ExtendedMenu::ViewItem> (item))
			{
				View* view = unknown_cast<View> (viewItem->getView ());
				if(view && (view->getSizeMode () & (kAttachLeft|kAttachRight)) == (kAttachLeft|kAttachRight))
				{
					Rect size (view->getSize ());
					size.setWidth (painter->getFixedSubMenuWidth ());
					view->setSize (size);
				}
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuControl::ItemButton* CompactMenuControl::ClientView::createItemButton (MenuItem* item)
{
	return NEW ItemButton (painter, item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CompactMenuControl::ClientView* CompactMenuControl::ClientView::getParentClient () const
{
	CompactMenuContainer* container = getContainer ();
	CompactMenuControl* prevColumn = container ? container->getControl (getDepth () - 1) : nullptr;
	return prevColumn ? prevColumn->getCompactClient () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CompactMenuControl::ClientView* CompactMenuControl::ClientView::getSubClient () const
{
	CompactMenuContainer* container = getContainer ();
	CompactMenuControl* nextColumn = container ? container->getControl (getDepth () + 1) : nullptr;
	return nextColumn ? nextColumn->getCompactClient () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CompactMenuControl::ClientView* CompactMenuControl::ClientView::getActiveClientView () const
{
	ClientView* subClient = getSubClient ();
	if(subClient && subClient->getCurrentItem ())
		return subClient->getActiveClientView ();

	return ccl_const_cast (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::ClientView::updateHeader (HeaderType type)
{
	CompactMenuControl* control = getCompactControl ();

	// determine current header type
	HeaderType currentType = kHeaderNone;
	if(View* header = control->getHeader ())
	{
		if(ccl_cast<CloseButton> (header))
			currentType = kHeaderCloseButton;
		else if(auto backButton = ccl_cast<CloseButton> (header))
			currentType = backButton->isEmpty () ? kHeaderBackButton : kHeaderBackAndCloseButton;
	}

	if(type != currentType)
	{
		View* newHeader = nullptr;
		if(type == kHeaderBackButton || type == kHeaderBackAndCloseButton)
		{
			newHeader = NEW BackButton (painter, getMenu ());
			newHeader->autoSize ();

			if(type == kHeaderBackAndCloseButton)
			{
				auto closeButton = NEW CloseButton (painter);

				Rect closeRect (newHeader->getSize ());
				closeRect.setWidth (closeRect.getHeight ());
				closeRect.offset (newHeader->getWidth () - closeRect.getWidth ());

				closeButton->setSize (closeRect);
				closeButton->setSizeMode (kAttachRight|kVCenter);
				newHeader->addView (closeButton);
			}
		}
		else if(type == kHeaderCloseButton)
		{
			newHeader = NEW CloseButton (painter);
			newHeader->autoSize ();
		}
		control->setHeader (newHeader);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::ClientView::checkSubMenus ()
{
	CompactMenuContainer* container = getContainer ();
	if(container && getDepth () == container->getMaxColumns () - 1) // don't auto expand from last column
		return;

	MenuControl::ItemButton* currentButton = getCurrentItem ();
	if(nextSubMenuCheck != NumericLimits::kMaxInt64)
	{
		int64 now = System::GetSystemTicks ();
		if(now >= nextSubMenuCheck)
		{
			if(currentButton && currentButton->canOpenSubMenu () && !currentButton->isSubMenuOpen () && !wasKeyNavigation)
			{
				CCL_PRINTF ("MenuControl: open subMenu (%s)\n", currentButton ? MutableCString (painter->getDisplayTitle (*currentButton->getItem ())).str () : "0")
				currentButton->popupSubMenu ();
				return;
			}
			nextSubMenuCheck = NumericLimits::kMaxInt64;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::ClientView::setMinWidth (Coord minWidth)
{
	ccl_lower_limit (minSize.x, minWidth);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::ClientView::updateSize ()
{
	ScopedVar<bool> scope (inUpdateSize, true);

	painter->recalc (*menu);

	// recalc items
	const Coord spacing = painter->getSpacing ();
	Coord top = margin;
	Coord topMargin = margin;
	Coord totalWidth = 0; // of items (margin not included)

	Coord maxItemWidth = getMaxWidth () - 2 * margin;
	Coord minItemWidth = ccl_min (getMinSize ().x - 2 * margin, maxItemWidth);
	ccl_lower_limit (totalWidth, minItemWidth);

	bool isFirstView = true;
	ForEachViewFast (*this, view)
		if(ItemButton* button = ccl_cast<ItemButton> (view))
		{
			if(button->getItem ()->isHeader ())
			{
				if(isFirstView)
					topMargin -= spacing;
			}
			
			button->autoSize ();
		}

		Rect size (view->getSize ());
		if(size.getWidth () > maxItemWidth)
		{
			size.setWidth (maxItemWidth);
			view->setSize (size);
		}

		if(view->getWidth () > totalWidth)
			totalWidth = view->getWidth ();
	
		top += view->getHeight ();
		isFirstView = false;
	EndFor

	Rect totalSize;

	// layout items
	top = topMargin;
	ForEachViewFast (*this, view)
		Rect r (view->getSize ());
		r.setWidth (totalWidth);
		r.moveTo (Point (margin, top));
		view->setSize (r);

		totalSize.join (r);
		top += r.getHeight ();
	EndFor

	// resize client
	totalSize.right += margin;
	totalSize.bottom += margin;

	CompactMenuControl* control = getCompactControl ();
	View* header = control ? control->getHeader () : nullptr;
	Coord headerH = header ? header->getHeight () : 0;

	Coord minH = getMinColumnHeight (); // explicit min. height (e.g. from parent control)
	if(minH > 0)
		minH -= headerH;
	ccl_upper_limit (minH, getMaxControlHeight ());
	ccl_lower_limit (totalSize.bottom, minH);

	// disable size mode while resizing to fit childs (as in autoSize)
	bool wasDisabled = isSizeModeDisabled ();
	disableSizeMode (true);
	setSize (totalSize);
	disableSizeMode (wasDisabled);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::ClientView::closeDeepestMenu ()
{
	if(CompactMenuContainer* container = getContainer ())
		container->removeColumn (-1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::ClientView::setCurrentItem (MenuControl::ItemButton* button, bool keyNavigation)
{
	if(button != currentItem && !ccl_cast<BackButton> (button))
	{
		MenuControl::ClientView::setCurrentItem (button, keyNavigation);
		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::ClientView::setClickedItem (MenuItem* item)
{
	SuperClass::setClickedItem (item);

	// also in top level control
	if(MenuControl* rootControl = getRootMenuControl ())
		if(rootControl->getClient () && rootControl->getClient () != this)
			rootControl->getClient ()->setClickedItem (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::ClientView::onChildSized (View* child, const Point& delta)
{
	if(!inUpdateSize && !ccl_cast<ItemButton> (child))
	{
		// custom view has resized: update layout
		updateSize ();
		CCL_PRINTF ("CompactMenuControl::ClientView::updateClient %d x %d\n", getWidth (), getHeight ())
	}
	SuperClass::onChildSized (child, delta);
}

//************************************************************************************************
// CompactMenuControl::ItemButton
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CompactMenuControl::ItemButton, MenuControl::ItemButton)

//////////////////////////////////////////////////////////////////////////////////////////////////

CompactMenuControl::ItemButton::ItemButton (MenuItemPainter* painter, MenuItem* item)
: MenuControl::ItemButton (painter, item)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CompactMenuControl::ItemButton::popupSubMenu (bool keyNavigation)
{
	ClientView* parentView = getParent<ClientView> ();
	if(parentView && item->getSubMenu () && parentView->getOpenSubMenuItem () != this)
	{
		CompactMenuControl* parentControl = parentView->getCompactControl ();
		CompactMenuContainer* container = parentControl->getContainer ();
		ASSERT (container)
		if(!container)
			return false;

		CompactMenuControl* menuControl = container->createMenuControl (item->getSubMenu (), parentControl);
		container->addColumn (menuControl);

		subMenuControl = menuControl;

		MenuControl* rootControl = parentView->getRootMenuControl ();
		if(rootControl)
			rootControl->getPopupClient ()->acceptOnMouseUp (false); // prevent closing automatically when the new column has appeared under mouse, will be reset on next mousedown

		if(keyNavigation)
			menuControl->getClient ()->selectNextItem (keyNavigation);

		parentView->invalidate ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::ItemButton::closeSubMenu ()
{
	if(!isClickable () && subMenuControl)
	{
		if(auto client = ccl_cast<CompactMenuControl::ClientView> (subMenuControl->getClient ()))
		{
			client->setCurrentItem (nullptr, false);

			if(CompactMenuContainer* container = client->getContainer ())
				if(container->countColumns () > container->getMaxColumns ())
					container->removeColumn (client->getDepth ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CompactMenuControl::ItemButton::onMouseDown (const MouseEvent& event)
{
	// re-enable acceptOnMouseUp behavior (disabled when new column appears)
	ClientView* client = getParent<ClientView> ();
	if(MenuControl* rootControl = client ? client->getRootMenuControl () : nullptr)
		rootControl->getPopupClient ()->acceptOnMouseUp (true);

	return SuperClass::onMouseDown (event);
}

//************************************************************************************************
// CompactMenuControl::HeaderButton
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (CompactMenuControl::HeaderButton, CompactMenuControl::ItemButton)

//////////////////////////////////////////////////////////////////////////////////////////////////

CompactMenuControl::HeaderButton::HeaderButton (MenuItemPainter* painter)
: ItemButton (painter, NEW MenuItem),
  isActive (false)
{
	//item->isHeader (true);
	item->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CompactMenuControl::HeaderButton::isClickable () const
{
	return false; // prevent close on mouseUp after scrolling
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::HeaderButton::checkActiveArea (PointRef position)
{
	Rect r;
	getClientRect (r);
	bool active = getActiveArea (r).pointInside (position);

	if(active != isActive)
	{
		isActive = active;
		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect CompactMenuControl::HeaderButton::getActiveArea (RectRef itemSize) const
{
	return itemSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::HeaderButton::draw (const UpdateRgn& updateRgn)
{
	GraphicsPort graphics (this);
	Rect r;
	getClientRect (r);

	int state = isActive ? getMouseState () : kMouseNone;
	drawButton (graphics, r, state);

	View::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CompactMenuControl::HeaderButton::onMouseEnter (const MouseEvent& event)
{
	checkActiveArea (event.where);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CompactMenuControl::HeaderButton::onMouseMove (const MouseEvent& event)
{
	checkActiveArea (event.where);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CompactMenuControl::HeaderButton::onMouseLeave (const MouseEvent& event)
{
	checkActiveArea (Point (-1, -1));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CompactMenuControl::HeaderButton::onMouseDown (const MouseEvent& event)
{
	if(View::onMouseDown (event))
		return true;

	checkActiveArea (event.where);
	if(isActive)
	{
		push ();
		return true;
	}
	return false;
}

//************************************************************************************************
// CompactMenuControl::BackButton
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (CompactMenuControl::BackButton, CompactMenuControl::HeaderButton)

//////////////////////////////////////////////////////////////////////////////////////////////////

CompactMenuControl::BackButton::BackButton (MenuItemPainter* painter, Menu* menu)
: HeaderButton (painter)
{
	if(menu)
		item->setTitle (menu->getTitle ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect CompactMenuControl::BackButton::getActiveArea (RectRef itemSize) const
{
	return painter->getBackButtonSize (itemSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::BackButton::drawButton (IGraphics& graphics, RectRef rect, int state)
{
	painter->drawBackButton (graphics, rect, *item, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::BackButton::push ()
{
	// close deepest menu -> back to parent menu 
	if(auto control = getParent<CompactMenuControl> ())
		control->getCompactClient ()->closeDeepestMenu ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CompactMenuControl::BackButton::onNavigate (const KeyEvent& event)
{
	if(event.vKey == VKey::kEnter || event.vKey == VKey::kReturn)
	{
		if(auto client = getParent<CompactMenuControl::ClientView> ())
		{
			if(ClientView* parentClient = client->getParentClient ())
				if(MenuControl::ItemButton* openButton = parentClient->getOpenSubMenuItem ())
					parentClient->setCurrentItem (openButton, true);

			client->closeDeepestMenu ();
		}
		return true;
	}
	return false;
}

//************************************************************************************************
// CloseButton
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (CompactMenuControl::CloseButton, CompactMenuControl::HeaderButton)

//////////////////////////////////////////////////////////////////////////////////////////////////

CompactMenuControl::CloseButton::CloseButton (MenuItemPainter* painter)
: HeaderButton (painter)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::CloseButton::drawButton (IGraphics& graphics, RectRef rect, int state)
{
	painter->drawCloseButton (graphics, rect, *item, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuControl::CloseButton::push ()
{
	if(auto containerView = getParent<CompactMenuContainer> ())
		if(auto popupClient = unknown_cast<MenuControl::PopupClient> (containerView->getPopupClient ()))
			popupClient->setCancelOnMouseUp (true);
}

//************************************************************************************************
// CompactMenuContainer::ColumnSizeHelper
//************************************************************************************************

class CompactMenuContainer::ColumnSizeHelper
{
public:
	ColumnSizeHelper (MenuItemPainter& painter, Coord margin);

	Rect calcColumnSizeDeep (const Menu& menu) const;
	int getMaxPossibleColumns (const Menu& menu, Coord maxTotalWidth, int currentColumns = 0, Coord currentWidth = 0) const;

	Rect calcMenuSize (const Menu& menu) const;

private:
	MenuItemPainter& painter;
	Coord margin;
};

//************************************************************************************************
// CompactMenuContainer::ColumnSizeHelper
//************************************************************************************************

CompactMenuContainer::ColumnSizeHelper::ColumnSizeHelper (MenuItemPainter& painter, Coord margin)
: painter (painter),
	margin (margin)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect CompactMenuContainer::ColumnSizeHelper::calcMenuSize (const Menu& menu) const
{
	Rect totalSize;
	Coord top = margin;
	Coord heightForSubMenuHeader = 0;
	painter.recalc (menu);

	ForEachMenuItem (menu, item)
		Rect itemSize;
		if(auto viewItem = ccl_cast<ExtendedMenu::ViewItem> (item))
			itemSize = viewItem->getView ()->getSize ();
		else
		{
			itemSize = painter.getItemSize (*item);
			heightForSubMenuHeader = itemSize.getHeight ();
		}
		
		itemSize.moveTo (Point (0, top));
		totalSize.join (itemSize);

		top += itemSize.getHeight ();
	EndFor
	
	totalSize.bottom += heightForSubMenuHeader;

	totalSize.right += margin;
	totalSize.bottom += margin;
	return totalSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect CompactMenuContainer::ColumnSizeHelper::calcColumnSizeDeep (const Menu& menu) const
{
	// determine size of this menu
	Rect totalSize (calcMenuSize (menu));

	// recursion: include sizes of all possible submenus, the goal is one size that fits all
	ForEachMenuItem (menu, item)
		if(item->getSubMenu ())
			totalSize.join (calcColumnSizeDeep (*item->getSubMenu ()));
	EndFor

	return totalSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CompactMenuContainer::ColumnSizeHelper::getMaxPossibleColumns (const Menu& menu, Coord maxTotalWidth, int currentColumns, Coord currentWidth) const
{
	currentWidth += calcMenuSize (menu).getWidth ();

	if(currentWidth > maxTotalWidth)
		return currentColumns; // this menu's column is too wide
	else
	{
		// this one still fits (no limit so far), continue with sub menus
		int result = NumericLimits::kMaxInt;

		ForEachMenuItem (menu, item)
			if(item->getSubMenu ())
			{
				int maxColumnsDeep = getMaxPossibleColumns (*item->getSubMenu (), maxTotalWidth, currentColumns + 1, currentWidth);
				ccl_upper_limit (result, maxColumnsDeep);
			}
		EndFor

		return result;
	}
}

//************************************************************************************************
// CompactMenuContainer
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (CompactMenuContainer, View)

//////////////////////////////////////////////////////////////////////////////////////////////////

CompactMenuContainer::CompactMenuContainer (Menu* menu, VisualStyle* menuStyle, int maxColumns)
: menuStyle (menuStyle),
  maxColumns (maxColumns),
  availableScreenSize (getAvailableScreenSize ()),
  unifyColumnWidth (false),
  needsCloseButton (false),
  wasAttached (false)
{
	SizeLimit limits;
	limits.setUnlimited ();
	setSizeLimits (limits);

	// build first column (root menu)
	CompactMenuControl* rootControl = createMenuControl (menu, nullptr);
	CompactMenuControl::ClientView* client = rootControl->getCompactClient ();
	MenuItemPainter* painter = client->getPainter ();

	ColumnSizeHelper columnSizeHelper (*painter, client->getMargin ());
	Rect columnSize (columnSizeHelper.calcColumnSizeDeep (*menu).getSize ());

	if(painter->getFixedSubMenuWidth () > 0)
		if(columnSize.getWidth () > painter->getFixedSubMenuWidth ())
		{
			CCL_PRINTF ("fixedSubMenuWidth: %d -> columnSize %d\n", painter->getFixedSubMenuWidth (), columnSize.getWidth ())
			painter->setFixedSubMenuWidth (columnSize.getWidth ());
		}

	int totalMenuColumns = getTotalColumnsInMenu (*menu);
	if(totalMenuColumns > 1)
	{
		// limit number of columns so that they fit on the monitor / application window
		int possibleColumns = ccl_max (1, columnSizeHelper.getMaxPossibleColumns (*menu, availableScreenSize.getWidth ()));
		if(possibleColumns < getMaxColumns ())
			setMaxColumns (possibleColumns);
	}

	// optionally add a close button to the header of a single column menu
	if(menuStyle && menuStyle->getMetric<bool> ("singleColumnCloseButton", false))
		needsCloseButton = getMaxColumns () == 1;

	if(columnSize.getWidth () > availableScreenSize.getWidth ())
		columnSize.setWidth (availableScreenSize.getWidth ());

	if(totalMenuColumns  > getMaxColumns ())
	{
		// animated scrolling through columns is required: we need the same size for all columns for a nicer "push" animation
		unifyColumnWidth = true; // (could also be forced via a visualstyle option)
		minColumnSize = columnSize.getSize ();

		client->setMinWidth (minColumnSize.x);
	}

	addColumn (rootControl);

	String initialSubMenuPath (menu->getInitialSubMenuPath ());
	if(!initialSubMenuPath.isEmpty ())
	{
		// optionally open subMenu specified by path of menu names
		MenuControl* parentControl = rootControl;
		ForEachStringToken (initialSubMenuPath, "/", subMenuName)
			MenuControl::ItemButton* button = parentControl->getClient ()->findSubMenuItem (subMenuName);
			if(button && button->canOpenSubMenu ())
			{
				parentControl->getClient ()->setCurrentItem (button, false);
				button->popupSubMenu (false);

				parentControl = button->getSubMenuControl ();
				if(parentControl)
					continue;
			}
			break;
		EndFor
	}
	else if(getMaxColumns () > 1)
	{
		// automatically open second column if possible
		auto button = ccl_cast<MenuControl::ItemButton> (rootControl->getClient ()->getFirst ());
		if(button && button->canOpenSubMenu ())
		{
			rootControl->getClient ()->setCurrentItem (button, false);
			button->popupSubMenu (false);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect CompactMenuContainer::getAvailableScreenSize ()
{
	Rect availableSize;
	
	#if !CCL_PLATFORM_DESKTOP
	// mobile platform: application window size
	if(auto window = unknown_cast<Window> (Desktop.getApplicationWindow  ()))
	{
		window->getClientRect (availableSize);
		return availableSize;
	}
	#endif

	// desktop platform (or fallback): size of monitor of parent window 
	int monitor = 0;
	if(auto window = unknown_cast<Window> (Desktop.getDialogParentWindow ()))
		monitor = Desktop.findNearestMonitor (window->getSize ());
	else
	{
		Point mousePos;
		GUI.getMousePosition (mousePos);
		monitor = Desktop.findMonitor (mousePos, true);
	}
	Desktop.getMonitorSize (availableSize, monitor, true);
	return availableSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CompactMenuContainer::getTotalColumnsInMenu (const Menu& menu)
{
	int columns = 0;
	ForEachMenuItem (menu, item)
		if(item->getSubMenu ())
			ccl_lower_limit (columns, getTotalColumnsInMenu (*item->getSubMenu ()));
	EndFor

	return columns + 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CompactMenuContainer::countColumns () const
{
	return views.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient* CompactMenuContainer::getPopupClient () const
{
	CompactMenuControl* rootControl = getControl (0);
	ASSERT (rootControl)
	return rootControl->getPopupClient ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem* CompactMenuContainer::getResultItem () const
{
	CompactMenuControl* rootControl = getControl (0);
	ASSERT (rootControl)
	return rootControl->getResultItem ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CompactMenuControl* CompactMenuContainer::getControl (int columnIndex) const
{
	return ccl_cast<CompactMenuControl> (ccl_const_cast (this)->getChild (columnIndex));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CompactMenuContainer::getFirstVisibleColumn () const
{
	return ccl_max (0, countColumns () - getMaxColumns ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CompactMenuContainer::getBackButtonColumn () const
{
	int firstVisibleColumn = getFirstVisibleColumn ();
	return firstVisibleColumn > 0 ? firstVisibleColumn : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuContainer::updateBackButton ()
{
	int backButtonColumn = getBackButtonColumn ();

	for(int i = 0; i < countColumns (); i++)
		if(auto control = ccl_cast<CompactMenuControl> (getChild (i)))
		{
			CompactMenuControl::HeaderType type = CompactMenuControl::kHeaderNone;
			if(i == backButtonColumn)
				type = needsCloseButton ? CompactMenuControl::kHeaderBackAndCloseButton : CompactMenuControl::kHeaderBackButton;
			else if(needsCloseButton)
				type = CompactMenuControl::kHeaderCloseButton;

			control->getCompactClient ()->updateHeader (type);
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CompactMenuControl* CompactMenuContainer::createMenuControl (Menu* menu, CompactMenuControl* parentControl)
{
	CompactMenuControl* menuControl = NEW CompactMenuControl (menu, menuStyle);

	if(parentControl)
	{
		menuControl->setParentControl (parentControl);

		Point minSize (parentControl->getSize ().getSize ());
		//if(parentView->getOpenSubMenuItem ())
		//	minSize.x = parentView->getOpenSubMenuItem ()->getWidth ();

		CompactMenuControl::ClientView* subClient = menuControl->getCompactClient ();
		subClient->setDepth (parentControl->getCompactClient ()->getDepth () + 1);
		subClient->initWithParent (*parentControl->getCompactClient ());
	}
	return menuControl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuContainer::addColumn (CompactMenuControl* control)
{
	int oldFirstColumn = getFirstVisibleColumn ();
	int oldDepth = countColumns () - 1;
	int newDepth = control->getCompactClient ()->getDepth ();

	// remove old conflicting columns
	for(int removeIndex = oldDepth; removeIndex >= newDepth; removeIndex--)
	{
		CompactMenuControl* c = getControl (removeIndex);
		ASSERT (c)
		if(!c)
			break;

		removeColumnViewInternal (c);
	}

	control->setSize (Rect (control->getSize ()).moveTo (Point (kMinCoord, 0))); // move out of view until final layout (for animation snapshout)
	addView (control);

	int newFirstColumn = getFirstVisibleColumn ();

	AutoPtr<ViewAnimator> animator;
	if(newFirstColumn > oldFirstColumn) // animate only if columns are shifted
	{
		animator = ViewAnimator::create (this, Styles::kTransitionPushLeft);
		if(animator)
			animator->snipFromView (this);
	}

	// adjust whole container (in layoutColumns) if new column exceeds current height (e.g. when not scrolling columns)
	if(control->getHeight () > getHeight ())
		ccl_lower_limit (minColumnSize.y, control->getHeight ());

	layoutColumns ();

	if(animator)
	{
		animator->snipToView (this);
		animator->makeTransition ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuContainer::removeColumn (int index)
{
	if(index < 0)
		index = countColumns () - 1;

	if(index <= 0)
		return;

	if(CompactMenuControl* control = getControl (index))
	{
		int oldFirstColumn = getFirstVisibleColumn ();

		AutoPtr<ViewAnimator> animator;
		if(oldFirstColumn > 0) // animate only if columns are shifted
		{
			animator = ViewAnimator::create (this, Styles::kTransitionPush);
			if(animator)
				animator->snipFromView (this);
		}

		// remove given and deeper columns
		int lastColumn = countColumns () - 1;
		while(lastColumn >= index)
		{
			CompactMenuControl* c = getControl (lastColumn);
			ASSERT (c)
			removeColumnViewInternal (c);
			lastColumn--;
		}

		layoutColumns ();

		if(animator)
		{
			animator->snipToView (this);
			animator->makeTransition ();
		}
	}
	
	index -= 1;
	if(index >= 0)
		if(CompactMenuControl* control = getControl (index))
			control->getCompactClient ()->setCurrentItem (nullptr, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuContainer::removeColumnViewInternal (CompactMenuControl* control)
{
	// reset pointer in "parent" button
	MenuControl::ClientView* parentClient = control->getCompactClient ()->getParentClient ();
	MenuControl::ItemButton* openButton = parentClient ? parentClient->getOpenSubMenuItem () : nullptr;
	if(openButton && openButton->getSubMenuControl () == control)
		openButton->setSubMenuControl (nullptr);

	removeView (control);
	control->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuContainer::layoutColumns ()
{
	updateBackButton ();
	
	Coord containerHeight = getHeight ();
	
	Coord height = ccl_max (minColumnSize.y, containerHeight);
	if(!requestedSize.isNull ())
		ccl_upper_limit (height, requestedSize.y);

	if(isResizing ()) // can't change height while resizing
		height = containerHeight;

	Rect columnRect (0, 0, 0, height);

	int firstColumn = getFirstVisibleColumn ();
	int column = 0;

	for(auto child : views)
	{
		auto control = ccl_cast<CompactMenuControl> (child);
		ASSERT (control)
		if(control)
		{
			Point oldScrollPos = control->getTargetView ()->getSize ().getLeftTop ();

			CompactMenuControl::ClientView* client = control->getCompactClient ();

			client->setMinColumnHeight (containerHeight);			

			if(unifyColumnWidth)
				client->setMinWidth (getMinColumnWidth ());

			if(requestedSize.x > 0)
				client->setMaxWidth (requestedSize.x);
			else
				client->setMaxWidth (availableScreenSize.getWidth ());

			control->updateSize ();

			Rect controlRect (columnRect);
			controlRect.setWidth (control->getWidth ());
			if(unifyColumnWidth)
			{
				Rect rect (control->getSize ());
				if(getMinColumnWidth () > rect.getWidth ())
				{
					rect.setWidth (getMinColumnWidth ());
					control->setSize (rect);

					Rect clipRect;
					control->getClipViewRect (clipRect);
					Rect clientRect (client->getSize ());
					if(clientRect.getWidth () < clipRect.getWidth ())
						client->setSize (clientRect.setWidth (clipRect.getWidth ()));
				}
			}

			if(column++ >= firstColumn)
			{
				columnRect.setWidth (control->getWidth ());
				control->setSize (columnRect);
				columnRect.offset (columnRect.getWidth ());
			}
			else
				control->setSize (Rect (control->getSize ()).moveTo (Point (kMinCoord, 0))); // move out of view

			// restore scroll position, might have been reset during resizing (e.g. in control->updateSize ())
			control->scrollTo (oldScrollPos);
		}
	}

	// resize container to visible columns
	Rect totalSize;
	int firstVisible = getFirstVisibleColumn ();
	for(int i = firstVisible; i < firstVisible + getMaxColumns (); i++)
		if(CompactMenuControl* c = getControl (i))
			totalSize.join (c->getSize ());
		else
			break;

	totalSize.moveTo (getSize ().getLeftTop ());

	ScopedFlag<kAttachDisabled> scope (sizeMode);
	setSize (totalSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord CompactMenuContainer::getMinColumnWidth () const
{
	Coord w = minColumnSize.x;
	if(requestedSize.x > 0)
		ccl_upper_limit (w, requestedSize.x);
	return w;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuContainer::attached (View* parent)
{
	// suppress fitsize: we place invisible child menu controls "outside"
	setSizeMode (getSizeMode () & ~kFitSize);

	SuperClass::attached (parent);

	wasAttached = true;

	if(requestedSize.y == 0)
	{
		// take current height as requested if window has fixed vertical limits (e.g. set by PopupSelector)
		Window* window = getWindow ();
		View* firstView = window ? window->getFirst () : nullptr;
		
		if(firstView && firstView->hasExplicitSizeLimits ()
			&& firstView->getSizeLimits ().minHeight == firstView->getSizeLimits ().maxHeight)
				requestedSize.y = getHeight ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CompactMenuContainer::onSize (const Point& delta)
{
	CCL_PRINTF ("CompactMenuContainer::onsize %d x %d (delta %d x %d)%s\n", size.getWidth (), size.getHeight (), delta.x, delta.y, isAttached () ? "" : " not attached")

	if(wasAttached)
	{
		// we are resized via the PopupSelector window (and some parent views between), likely triggered by the OS: remember the new size
		Window* window = getWindow ();
		if(window && window->isAttached () && window->isResizing ())
			requestedSize = getSize ().getSize ();
	}
	layoutColumns ();

	SuperClass::onSize (delta);
}
