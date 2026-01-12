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
// Filename    : ccl/gui/help/helpinfobuilder.cpp
// Description : Help Info Builder
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/help/helpinfobuilder.h"
#include "ccl/gui/help/keyglyphpainter.h"

#include "ccl/gui/graphics/shapes/shapeimage.h"
#include "ccl/gui/graphics/shapes/shapebuilder.h"
#include "ccl/gui/graphics/imaging/bitmappainter.h"
#include "ccl/gui/graphics/imaging/bitmapfilter.h"

#include "ccl/gui/views/imageview.h"
#include "ccl/gui/controls/label.h"
#include "ccl/gui/theme/thememanager.h"

using namespace CCL;

//************************************************************************************************
// HelpInfoBuilder::OptionItem
//************************************************************************************************

HelpInfoBuilder::OptionItem::OptionItem ()
: modifiers (0),
  flags (0)
{}

//************************************************************************************************
// HelpInfoBuilder
//************************************************************************************************

DEFINE_CLASS (HelpInfoBuilder, Object)
DEFINE_CLASS_UID (HelpInfoBuilder, 0x5196abae, 0xbcf5, 0x403f, 0xa1, 0x9a, 0xdb, 0xbc, 0xc8, 0xf9, 0xe6, 0xb2)

//////////////////////////////////////////////////////////////////////////////////////////////////

HelpInfoBuilder::HelpInfoBuilder ()
: ignoreModifiers (0)
{
	options.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HelpInfoBuilder::~HelpInfoBuilder ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline IImage* resolveIcon (StringID iconName)
{
	if(ITheme* appTheme = ThemeManager::instance ().getApplicationTheme ())
		return appTheme->getImage (iconName);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API HelpInfoBuilder::setAttribute (AttrID id, VariantRef value)
{
	switch(id)
	{
	case kIcon : 
		if(value.isObject ())
			setIcon (UnknownPtr<IImage> (value.asUnknown ()));
		else
			setIcon (resolveIcon (MutableCString (value.asString ())));
		break;
		
	case kTitle : 
		setTitle (value.asString ()); 
		break;

	case kDescription : 
		setDescription (value.asString ());
		break;

	case kIgnoreModifiers :
		setIgnoreModifiers (value.asInt ());
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API HelpInfoBuilder::addOption (uint32 modifiers, IImage* icon, StringRef text)
{
	OptionItem* item = NEW OptionItem;
	item->setModifiers (modifiers);
	item->setIcon (icon);
	item->setText (text);
	options.add (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API HelpInfoBuilder::addOption (uint32 modifiers, StringID iconName, StringRef text)
{
	addOption (modifiers, resolveIcon (iconName), text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API HelpInfoBuilder::setActiveOption (uint32 modifiers)
{
	modifiers &= ~ignoreModifiers;

	ForEach (options, OptionItem, item)
		if(modifiers == 0)
			item->isActive (false);
		else
		{
			uint32 itemModifiers = item->getModifiers () & KeyState::kModifierMask; // ignore gestures
			item->isActive (modifiers == itemModifiers);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static IBitmapFilter* createBitmapFilter (bool tint, bool colorize, bool invert, Color color)
{
	if(!tint && !invert)
		return nullptr;

	BitmapFilterList* filterList = NEW BitmapFilterList;
	filterList->addFilter (NEW BitmapFilters::RevertPremultipliedAlpha);
	
	if(invert)
		filterList->addFilter (NEW BitmapFilters::Inverter);
	
	// either tint OR colorize
	if(tint)
	{
		BitmapFilters::Tinter* tinter = NEW BitmapFilters::Tinter;
		tinter->setColor (color);
		filterList->addFilter (tinter);
	}
	else if(colorize)
	{
		BitmapFilters::Colorizer* colorizer = NEW BitmapFilters::Colorizer;
		colorizer->setColor (color);
		filterList->addFilter (colorizer);
	}

	filterList->addFilter (NEW BitmapFilters::PremultipliedAlpha);
	return *filterList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API HelpInfoBuilder::createImage (const Point& size, const IVisualStyle& style)
{
	#if DEBUG_LOG
	static int debugCounter = 0;
	CCL_PRINTF ("HelpInfo createImage %d\n", debugCounter++)
	#endif

	Font font (style.getTextFont ());
	Font titleFont (font);
	titleFont.isBold (true);

	Color normalColor (style.getTextColor ());
	SolidBrush textBrush (normalColor);
	Color activeColor (style.getColor ("hilite", Colors::kYellow));

	bool vertical = style.getMetric<bool> ("vertical", true);
	Coord rowHeight = style.getMetric<Coord> ("rowHeight", 24);
	Coord spacing = style.getMetric<Coord> ("spacing", 2);
	IImage* divider = style.getImage ("divider");

	BitmapPainter imagePainter;
	imagePainter.setBackColor (style.getBackColor ());

	bool tint = style.getMetric<bool> ("tint", false);
	bool colorize = style.getMetric<bool> ("colorize", false);
	bool invert = style.getMetric<bool> ("invert", false);
	AutoPtr<IBitmapFilter> normalFilter = createBitmapFilter (tint, colorize, invert, normalColor);
	AutoPtr<IBitmapFilter> activeFilter = createBitmapFilter (tint, colorize, invert, activeColor);
	imagePainter.setFilter (normalFilter, true);

	ShapeImage* image = NEW ShapeImage;
	ShapeBuilder g (image);

	if(vertical)
	{
		Coord modifierWidth = style.getMetric<Coord> ("modifierWidth", 52);
		Coord iconWidth = style.getMetric<Coord> ("iconWidth", 32);

		// determine max. text width
		Coord maxTextWidth = 0;
		ForEach (options, OptionItem, item)
			Coord textWidth = Font::getStringWidth (item->getText (), font);
			if(textWidth > maxTextWidth)
				maxTextWidth = textWidth;
		EndFor

		Coord top = 0;

		// Icon
		if(Image* icon = unknown_cast<Image> (this->icon))
		{
			Rect src (0, 0, icon->getWidth (), icon->getHeight ());
			Rect dst (src);
			Rect r (0, 0, src.getWidth (), rowHeight);
			dst.centerV (r);

			imagePainter.drawImage (g, icon, src, dst);
		}

		// Title
		if(!title.isEmpty ())
		{
			Rect r (0, 0, Font::getStringWidth (title, titleFont) + spacing, rowHeight);
			if(icon) // offset by icon
				r.offset (icon->getWidth () + spacing);
			g.drawString (r, title, titleFont, textBrush, Alignment::kLeftCenter);
			top += rowHeight;
		}

		ForEach (options, OptionItem, item)
			if(item->isActive ())
			{
				textBrush.setColor (activeColor);
				imagePainter.setFilter (activeFilter, true);
			}
			else
			{
				textBrush.setColor (normalColor);
				imagePainter.setFilter (normalFilter, true);
			}

			// Modifiers
			Rect rect (0, 0, modifierWidth, rowHeight);
			rect.offset (0, top);
			if(item->getModifiers ())
			{
				KeyEvent keyEvent;
				keyEvent.state.keys = item->getModifiers ();
	
				AutoPtr<Image> keyGlyph = KeyGlyphPainter (font, textBrush, 2, KeyGlyphPainter::kOutline).createShape (keyEvent);
				Rect src (0, 0, keyGlyph->getWidth (), keyGlyph->getHeight ());
				Rect dst (src);
				dst.centerV (rect);

				g.drawImage (keyGlyph, src, dst);
			}

			// Text
			rect.offset (rect.getWidth () + spacing);
			rect.setWidth (maxTextWidth);
			g.drawString (rect, item->getText (), font, textBrush, Alignment::kLeftCenter);

			// Icon
			rect.offset (rect.getWidth () + spacing);
			rect.setWidth (iconWidth);
			if(Image* icon = unknown_cast<Image> (item->getIcon ()))
			{
				Rect src (0, 0, icon->getWidth (), icon->getHeight ());
				Rect dst (src);
				dst.center (rect);

				imagePainter.drawImage (g, icon, src, dst);
			}

			top += rowHeight;
		EndFor
	}
	else // horizontal
	{
		Rect rect (spacing, 0, kMaxCoord, rowHeight);
		Image* headerIcon = unknown_cast<Image> (this->icon);
		Coord titleWidth = title.isEmpty () ? 0 : Font::getStringWidth (title, titleFont) + spacing;

		// Icon
		if(headerIcon)
		{
			Rect src (0, 0, headerIcon->getWidth (), headerIcon->getHeight ());
			Rect dst (src);
			dst.centerV (rect);
			
			imagePainter.drawImage (g, headerIcon, src, dst);

			rect.left = dst.right + spacing;
		}

		// Title
		if(titleWidth > 0)
		{
			rect.setWidth (titleWidth);
			g.drawString (rect, title, titleFont, textBrush, Alignment::kLeftCenter);
			rect.left = rect.right + spacing;
		}

		if(headerIcon || titleWidth > 0)
		{
			if(divider)
			{
				rect.left += spacing; //TODO: spaces -> need to crop images
				g.drawImage (divider, rect.getLeftTop ());
				rect.left += divider->getWidth () + 2 * spacing;
			}
			else
				rect.left += 3 * spacing;
		}

		ForEach (options, OptionItem, item)
			if(item->isActive ())
			{
				textBrush.setColor (activeColor);
				imagePainter.setFilter (activeFilter, true);
			}
			else
			{
				textBrush.setColor (normalColor);
				imagePainter.setFilter (normalFilter, true);
			}

			// Modifiers
			if(item->getModifiers ())
			{
				KeyEvent keyEvent;
				keyEvent.state.keys = item->getModifiers ();

				AutoPtr<Image> keyGlyph = KeyGlyphPainter (font, textBrush, 2, KeyGlyphPainter::kOutline).createShape (keyEvent);
				if(keyGlyph->getWidth () > 0)
				{
					Rect src (0, 0, keyGlyph->getWidth (), keyGlyph->getHeight ());
					rect.setWidth (src.getWidth ());

					Rect dst (src);
					dst.offset (rect.left);
					dst.centerV (rect);

					g.drawImage (keyGlyph, src, dst);

					rect.left = rect.right + spacing;
				}
			}
			
			// Text
			if(!item->getText ().isEmpty ())
			{
				Coord textWidth = Font::getStringWidth (item->getText (), font);
				rect.setWidth (textWidth);

				g.drawString (rect, item->getText (), font, textBrush, Alignment::kLeftCenter);

				rect.left = rect.right + spacing;
			}

			// Icon
			if(Image* icon = unknown_cast<Image> (item->getIcon ()))
			{
				Rect src (0, 0, icon->getWidth (), icon->getHeight ());
				rect.setWidth (src.getWidth ());

				Rect dst (src);
				dst.offset (rect.left);
				dst.centerV (rect);

				imagePainter.drawImage (g, icon, src, dst);

				rect.left = rect.right + spacing;
			}

			// Divider
			if(divider)
			{
				rect.left += spacing; //TODO: spaces -> need to crop images
				g.drawImage (divider, rect.getLeftTop ());
				rect.left += divider->getWidth () + 2 * spacing;
			}
			else
				rect.left += 3 * spacing;

		EndFor	
	}

	return image;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API HelpInfoBuilder::createView (const Rect& size, const IVisualStyle& style)
{
	Rect rect (size);

	#if 0 // TEXT
	//rect.setHeight (20);
	return NEW Label (rect, StyleFlags (0, Styles::kMultiLineLabel), createText ());
	
	#else // IMAGE
	AutoPtr<IImage> image = createImage (Point (size.getWidth (), size.getHeight ()), style);
	rect (0, 0, image->getWidth (), image->getHeight ());
	return NEW ImageView (image, rect, StyleFlags (0, Styles::kImageViewAppearanceFitImage));
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String CCL_API HelpInfoBuilder::createText ()
{
	String text;
	ForEach (options, OptionItem, item)
		String modifierString;
		KeyState (item->getModifiers ()).toString (modifierString, true);
		text << modifierString << " - " << item->getText () << "\n";
	EndFor
	return text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HelpInfoBuilder::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "icon")
		{ setAttribute (kIcon, var); return true; }
	
	if(propertyId == "title")
		{ setAttribute (kTitle, var); return true; }
	
	if(propertyId == "description")
		{ setAttribute (kDescription, var); return true; }
	
	if(propertyId == "ignoreModifiers")
		{ setAttribute (kIgnoreModifiers, var); return true; }

	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HelpInfoBuilder::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "icon")
		{ var.takeShared (getIcon ()); return true; }

	if(propertyId == "title")
		{ var = getTitle (); var.share (); return true; }

	if(propertyId == "description")
		{ var = getDescription (); var.share (); return true; }

	if(propertyId == "ignoreModifiers")
		{ var = (int64)getIgnoreModifiers (); return true; }

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (HelpInfoBuilder)
	DEFINE_METHOD_NAME ("addOption")
END_METHOD_NAMES (HelpInfoBuilder)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HelpInfoBuilder::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "addOption")
	{
		uint32 modifiers = msg[0].asInt ();
		String text (msg[2].asString ());
		
		if(msg[1].isObject ())
		{
			IImage* icon = UnknownPtr<IImage> (msg[1].asUnknown ());
			addOption (modifiers, icon, text);
		}
		else
		{
			MutableCString iconName (msg[1].asString ());
			addOption (modifiers, iconName, text);
		}
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// HelpInfoCollection
//************************************************************************************************

DEFINE_CLASS (HelpInfoCollection, Object)
DEFINE_CLASS_UID (HelpInfoCollection, 0xee576883, 0x638d, 0x4a0b, 0x8a, 0x2e, 0x2, 0x7e, 0x77, 0x4c, 0x9a, 0xc5)

//////////////////////////////////////////////////////////////////////////////////////////////////

HelpInfoCollection::HelpInfoCollection ()
{
	items.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IHelpInfoBuilder* CCL_API HelpInfoCollection::getInfo (StringID id) const
{
	ArrayForEach (items, Item, item)
		if(item->getID () == id)
			return item->getHelpInfo ();
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API HelpInfoCollection::addInfo (StringID id, IHelpInfoBuilder* _helpInfo)
{
	HelpInfoBuilder* helpInfo = unknown_cast<HelpInfoBuilder> (_helpInfo);
	ASSERT (helpInfo)
	if(!helpInfo)
		return kResultInvalidArgument;

	Item* item = NEW Item;
	item->setID (id);
	item->setHelpInfo (helpInfo);
	items.add (item);
	return kResultOk;
}
