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
// Filename    : ccl/app/controls/spritebuilder.cpp
// Description : Sprite Builder
//
//************************************************************************************************

#include "ccl/app/controls/spritebuilder.h"
#include "ccl/base/object.h"

#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/ivisualstyle.h"
#include "ccl/public/gui/framework/isprite.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// StandardSpriteDrawable
/** Draws a list of items, each item can have an icon and text. **/
//************************************************************************************************

class StandardSpriteDrawable: public Object,
							  public AbstractDrawable
{
public:
	DECLARE_CLASS_ABSTRACT (StandardSpriteDrawable, Object)

	StandardSpriteDrawable (IThemePainter& themePainter);

	void addItem (IImage* icon,
		StringRef text,
		int group = 0,
		FontRef font = Font (),
		BrushRef textBrush = SolidBrush (Colors::kBlack));

	PROPERTY_OBJECT (Brush, backgroundBrush, BackgroundBrush)
	PROPERTY_OBJECT (Pen, borderPen, BorderPen)
	PROPERTY_SHARED_AUTO (IImage, backgroundImage, BackgroundImage)
	PROPERTY_VARIABLE (Coord, margin, Margin);
	PROPERTY_VARIABLE (Coord, spacing, Spacing);
	PROPERTY_VARIABLE (Coord, radius, Radius);
	PROPERTY_VARIABLE (Coord, minWidth, MinWidth);

	PROPERTY_OBJECT (Point, size, Size)
	PROPERTY_VARIABLE (int, lastGroup, LastGroup);

	bool finish (int options);
	void arrangeItems ();
	bool replaceItemText (int index, StringRef text);
	String getItemText (int index) const;
	bool hasGroup (int group) const;

	// IDrawable
	void CCL_API draw (const DrawArgs& args) override;
	float CCL_API getOpacity () const override;

	CLASS_INTERFACE (IDrawable, Object)

private:
	class Item: public Unknown
	{
	public:
		Item (IImage* icon, StringRef text, FontRef font, BrushRef textBrush);

		void calcSize ();
		Coord getIconSize () const;
		void draw (IGraphics& graphics, PointRef pos, IThemePainter& themePainter);

		PROPERTY_OBJECT (Rect, size, Size)
		PROPERTY_VARIABLE (int, group, Group)
		PROPERTY_FLAG (flags, 1<<0, horizontalFlow)
		PROPERTY_FLAG (flags, 1<<1, largeIcon)

		String text;
		Font font;
		Brush textBrush;
		AutoPtr<IImage> icon;
		int flags;

		enum { kIconMargin = 2, kIconSize = 16, kIconSizeLarge = 32 };
	};

	enum { kLargeItemGroup = -100 };
	InterfaceList<Item> items;
	IThemePainter& themePainter;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// StandardSpriteDrawable::Item
//************************************************************************************************

StandardSpriteDrawable::Item::Item (IImage* icon, StringRef text, FontRef font, BrushRef textBrush)
: text (text),
  font (font),
  textBrush (textBrush),
  flags (0)
{
	this->icon.share (icon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord StandardSpriteDrawable::Item::getIconSize () const
{
	return largeIcon () ? kIconSizeLarge : kIconSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StandardSpriteDrawable::Item::calcSize ()
{
	size (0, 0, 0, 0);
	Font::measureString (size, text, font);

	if(icon)
	{
		Coord iconSize = getIconSize ();
		size.right += (iconSize +/* (size.right > 0) ?*/ kIconMargin /*: 0*/);
		ccl_lower_limit (size.bottom, iconSize);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StandardSpriteDrawable::Item::draw (IGraphics& graphics, PointRef offset, IThemePainter& themePainter)
{
	Rect r (size);
	r.offset (offset);
	if(icon)
	{
		Coord iconW = getIconSize ();
		Rect iconRect (r.left, r.top, r.left + iconW, r.top + iconW);
		themePainter.drawBestMatchingFrame (graphics, icon, iconRect);
		r.left += iconW + kIconMargin;
	}
	graphics.drawString (r, text, font, textBrush);
}

//************************************************************************************************
// StandardSpriteDrawable
//************************************************************************************************

DEFINE_CLASS_HIDDEN (StandardSpriteDrawable, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

StandardSpriteDrawable::StandardSpriteDrawable (IThemePainter& themePainter)
: themePainter (themePainter),
  lastGroup (-1),
  backgroundBrush (SolidBrush (Colors::kBlack)),
  borderPen (Colors::kBlack, 0),
  margin (3),
  spacing (2),
  radius (0),
  minWidth (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StandardSpriteDrawable::addItem (IImage* icon, StringRef text, int group, FontRef font, BrushRef textBrush)
{
	Item* item = NEW Item (icon, text, font, textBrush);
	item->calcSize ();
	item->setGroup (group);

	bool inserted = false;
	if(group < lastGroup)
	{
		ListForEach (items, Item*, i)
			if(i->getGroup () > group)
			{
				items.insertBefore (i, item);
				inserted = true;
				break;
			}
		EndFor
	}
	else
		lastGroup = group;

	if(!inserted)
		items.append (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StandardSpriteDrawable::finish (int options)
{
	if(items.isEmpty ())
		return false;

	int count = 0;
	int countHeaders = 0;
	IImage* icon = nullptr;
	ListForEach (items, Item*, item)
		if(count++ > 2)
			break;
		if(item->font.isBold () && item->icon == nullptr)
			countHeaders++;
		else
			icon = item->icon;
	EndFor

	// use a large icon if max. 1 header and 1 normal item
	bool largeAllowed = (options & SpriteBuilder::kForceSmallIcons) == 0;
	if(icon && countHeaders < 2 && count - countHeaders < 2 && largeAllowed)
	{
		// remove other icons
		ListForEach (items, Item*, item)
			item->icon.release ();
		EndFor

		// add new item with only the big icon (if any)
		Item* item = NEW Item (icon, nullptr, Font (), SolidBrush ());
		item->setGroup (kLargeItemGroup);
		item->largeIcon (true);
		item->calcSize ();
		item->horizontalFlow (true);
		items.prepend (item);
	}

	arrangeItems ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StandardSpriteDrawable::arrangeItems ()
{
	size (0, 0);
	Point pos;

	ListForEach (items, Item*, item)
		Rect itemSize (item->getSize ());
		itemSize.moveTo (pos);
		item->setSize (itemSize);

		// accumulate total size
		ccl_lower_limit (size.x, itemSize.right);
		ccl_lower_limit (size.y, itemSize.bottom);

		if(item->horizontalFlow ())
			pos (itemSize.right + spacing, itemSize.top);
		else
			pos (itemSize.left, itemSize.bottom + spacing);
	EndFor

	if(!size.isNull ())
	{
		size += Point (margin, margin) * 2;
		ccl_lower_limit (size.x, getMinWidth ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StandardSpriteDrawable::replaceItemText (int index, StringRef text)
{
	// skip internally added items
	ListForEach (items, Item*, item)
		if(item->getGroup () <= kLargeItemGroup)
			index++;
		else
			break;
	EndFor

	if(Item* item = items.at (index))
	{
		if(item->text != text)
		{
			item->text = text;
			item->calcSize ();
			arrangeItems ();
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String StandardSpriteDrawable::getItemText (int index) const
{
	// skip internally added items
	ListForEach (items, Item*, item)
		if(item->getGroup () <= kLargeItemGroup)
			index++;
		else
			break;
	EndFor

	Item* item = items.at (index);
	return item ? item->text : String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StandardSpriteDrawable::hasGroup (int group) const
{
	ListForEach (items, Item*, item)
		if(item->getGroup () == group)
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StandardSpriteDrawable::draw (const DrawArgs& args)
{
	if(backgroundImage)
	{
		Rect src (0, 0, backgroundImage->getWidth (), backgroundImage->getHeight ());
		args.graphics.drawImage (backgroundImage, src, size);
	}
	else
	{
		if(radius != 0)
		{
			args.graphics.fillRoundRect (args.size, radius, radius, backgroundBrush);
			if(borderPen.getWidth () > 0)
				args.graphics.drawRoundRect (args.size, radius, radius, borderPen);
		}
		else
		{
			args.graphics.fillRect (args.size, backgroundBrush);
			if(borderPen.getWidth () > 0)
				args.graphics.drawRect (args.size, borderPen);
		}
	}

	Point pos (args.size.getLeftTop ());
	pos.offset (margin, margin);

	ListForEach (items, Item*, item)
		item->draw (args.graphics, pos, themePainter);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API StandardSpriteDrawable::getOpacity () const
{
	return 0.8f;
}

//************************************************************************************************
// SpriteBuilder
//************************************************************************************************

SpriteBuilder::SpriteBuilder (IView* view)
: drawable (nullptr),
  view (view),
  numItems (0),
  createSpriteSuspended (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpriteBuilder::~SpriteBuilder ()
{
	if(drawable)
		drawable->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IVisualStyle& SpriteBuilder::getVisualStyle ()
{
	ITheme& theme = view ? ViewBox (view).getTheme () : *ViewBox::getModuleTheme ();
	return theme.getStyle ("Standard.Sprite");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* SpriteBuilder::getWarningIcon ()
{
	return getVisualStyle ().getImage ("warningicon");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpriteBuilder::setBackgroundBrush (BrushRef brush)
{
	getDrawable ()->setBackgroundBrush (brush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpriteBuilder::setBorderPen (PenRef pen)
{
	getDrawable ()->setBorderPen (pen);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpriteBuilder::setBackgroundImage (IImage* image)
{
	getDrawable ()->setBackgroundImage (image);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpriteBuilder::setMargin (Coord margin)
{
	getDrawable ()->setMargin (margin);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpriteBuilder::setSpacing (Coord spacing)
{
	getDrawable ()->setSpacing (spacing);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpriteBuilder::setRadius (Coord radius)
{
	getDrawable ()->setRadius (radius);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpriteBuilder::setMinWidth (Coord width)
{
	getDrawable ()->setMinWidth (width);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StandardSpriteDrawable* SpriteBuilder::getDrawable ()
{
	if(!drawable && view)
	{
		ViewBox vb (view);
		ITheme& theme = vb.getTheme ();
		const IVisualStyle& vs = theme.getStyle ("Standard.Sprite");

		drawable = NEW StandardSpriteDrawable (theme.getPainter ());

		font = vs.getTextFont ();
		textBrush = vs.getTextBrush ();

		drawable->setBackgroundBrush (vs.getBackBrush ());
		drawable->setBackgroundImage (vs.getBackgroundImage ());
		drawable->setMargin (vs.getMetric ("margin", drawable->getMargin ()));
		drawable->setSpacing (vs.getMetric ("spacing", drawable->getSpacing ()));
		drawable->setRadius (vs.getMetric ("radius", drawable->getRadius ()));
		drawable->setMinWidth (vs.getMetric ("minwidth", drawable->getMinWidth ()));

		float borderW = vs.getMetric ("border", 0.f);
		drawable->setBorderPen (Pen (vs.getColor ("bordercolor", Colors::kBlack), borderW));
	}
	return drawable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISprite* SpriteBuilder::createSprite (int options)
{
	if(view && !createSpriteSuspended)
	{
		getDrawable ();
		if(!drawable->finish (options))
			return nullptr;

		ISprite* sprite = ccl_new<ISprite> (ClassID::FloatingSprite);
		sprite->takeOpacity (drawable);
		sprite->construct (view, drawable->getSize (), drawable, ISprite::kKeepOnTop);
		safe_release (drawable);
		return sprite;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpriteBuilder::replaceItemText (ISprite& sprite, int index, StringRef text)
{
	if(StandardSpriteDrawable* drawable = unknown_cast<StandardSpriteDrawable> (sprite.getDrawable ()))
		if(drawable->replaceItemText (index, text))
		{
			Rect rect (sprite.getSize ());
			rect.setSize (drawable->getSize ());
			if(rect != sprite.getSize ())
				sprite.move (rect);
			else
				sprite.refresh ();
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String SpriteBuilder::getItemText (ISprite& sprite, int index) const
{
	auto* drawable = unknown_cast<StandardSpriteDrawable> (sprite.getDrawable ());
	return drawable ? drawable->getItemText (index) : String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point SpriteBuilder::getDefaultOffset () const
{
	return Point (0, 30); // don't hide mouse cursor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point SpriteBuilder::getTouchOffset () const
{
	return Point (0, -60);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SpriteBuilder::addItem (IImage* icon, StringRef text, int group, Font* font, SolidBrush* textBrush)
{
	if(!font)
		font = &this->font;
	if(!textBrush)
		textBrush = &this->textBrush;

	if(numItems >= kMaxItems)
	{
		if(numItems == kMaxItems)
			if(StandardSpriteDrawable* drawable = getDrawable ())
				drawable->addItem (nullptr, CCLSTR ("..."), group, *font, *textBrush);

		numItems++;
		return false;
	}

	if(StandardSpriteDrawable* drawable = getDrawable ())
		drawable->addItem (icon, text, group, *font, *textBrush);
	numItems++;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SpriteBuilder::addItem (StringRef text, int group, Font* font, SolidBrush* textBrush)
{
	return addItem (nullptr, text, group, font, textBrush);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpriteBuilder::addHeader (IImage* icon, StringRef text, int group)
{
	Font font (getFont ());
	font.isBold (true);
	addItem (icon, text, group, &font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SpriteBuilder::addHeader (StringRef text, int group)
{
	addHeader (nullptr, text, group);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SpriteBuilder::getLastGroup ()
{
	if(StandardSpriteDrawable* drawable = getDrawable ())
		return drawable->getLastGroup ();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SpriteBuilder::hasGroup (int group) const
{
	return drawable && drawable->hasGroup (group);
}
