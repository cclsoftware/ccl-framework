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
// Filename    : ccl/app/editing/editcursor.cpp
// Description : Edit Cursor
//
//************************************************************************************************

#include "ccl/app/editing/editcursor.h"

#include "ccl/app/controls/usercontrol.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/gui/framework/isprite.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// EditCursor
//************************************************************************************************

class EditCursorShape: public SolidDrawable
{
public:
	EditCursorShape (UserControl& editView)
	: SolidDrawable (editView.getVisualStyle ().getColor ("cursorcolor", editView.getTheme ().getThemeColor (ThemeElements::kAlphaCursorColor)))
	{}
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// AbstractEditCursor
//************************************************************************************************

UIDRef AbstractEditCursor::getSpriteClass ()
{
	if(GraphicsFactory::hasGraphicsLayers ())
		return ClassID::SublayerSprite;
	else
		return ClassID::FloatingSprite;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (AbstractEditCursor, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AbstractEditCursor::AbstractEditCursor (UserControl* editView)
: editView (editView)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserControl* AbstractEditCursor::getEditView () const
{
	return editView;
}

//************************************************************************************************
// BasicEditCursor
//************************************************************************************************

DEFINE_CLASS_HIDDEN (BasicEditCursor, AbstractEditCursor)

//////////////////////////////////////////////////////////////////////////////////////////////////

BasicEditCursor::BasicEditCursor (UserControl* editView)
: AbstractEditCursor (editView),
  sprite (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

BasicEditCursor::~BasicEditCursor ()
{
	ASSERT (sprite == nullptr)
	if(sprite)
		sprite->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BasicEditCursor::isVisible () const
{
	return sprite && sprite->isVisible ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BasicEditCursor::setVisible (bool state)
{
	if(state && !sprite)
	{
		attached ();
		ASSERT (sprite != nullptr)
	}

	if(sprite && sprite->isVisible () != (tbool)state)
	{
		if(state)
			sprite->show ();
		else
			sprite->hide ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BasicEditCursor::attached ()
{
	ASSERT (sprite == nullptr && editView != nullptr)
	if(!sprite)
	{
		AutoPtr<IDrawable> drawable = createDrawable ();
		
		UID cid (getSpriteClass ());
		if(cid == ClassID::FloatingSprite)
			drawable->takeOpacity ();

		sprite = ccl_new<ISprite> (cid);
		ASSERT (sprite != nullptr)

		Rect rect;
		getSpriteRect (rect);
		sprite->construct (*editView, rect, drawable);
		sprite->show ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BasicEditCursor::removed ()
{
	if(sprite)
		sprite->hide (),
		sprite->release (),
		sprite = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BasicEditCursor::updateSize ()
{
	if(sprite)
	{
		Rect rect;
		getSpriteRect (rect);
		sprite->move (rect);
	}
}

//************************************************************************************************
// EditCursor
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditCursor, BasicEditCursor)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditCursor::EditCursor (UserControl* editView, int width)
: BasicEditCursor (editView),
  position (0),
  width (width)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDrawable* EditCursor::createDrawable ()
{
	return NEW EditCursorShape (*editView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord EditCursor::getPosition () const
{
	return position;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditCursor::move (PointRef newPosition)
{
	position = newPosition.x;
	if(sprite)
		sprite->moveTo (Point (newPosition.x, sprite->getSize ().top));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditCursor::scrolled (Coord delta)
{
	position += delta;
	if(sprite)
		sprite->scrolled (Point (delta, 0));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect& EditCursor::getSpriteRect (Rect& rect) const
{
	ASSERT (editView != nullptr)
	rect (position, 0, position + width, editView->getHeight ());
	return rect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditCursor::setColor (ColorRef color)
{
	if(sprite)
		if(SolidDrawable* drawable = static_cast<SolidDrawable*> (sprite->getDrawable ()))
		{
			if(drawable->getBrush ().getColor () != color)
			{			
				tbool visible = sprite->isVisible ();
				if(visible)
					sprite->hide ();

				drawable->setBrush (SolidBrush (color));
				sprite->refresh ();

				if(visible)
					sprite->show ();
			}
		}
}

//************************************************************************************************
// CrossHairCursor
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CrossHairCursor, AbstractEditCursor)

//////////////////////////////////////////////////////////////////////////////////////////////////

CrossHairCursor::CrossHairCursor (UserControl* editView, int width)
: AbstractEditCursor (editView),
  width (width),
  hSprite (nullptr),
  vSprite (nullptr),
  visible (false)
{
	ASSERT (editView != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CrossHairCursor::~CrossHairCursor ()
{
	ASSERT (hSprite == nullptr)
	safe_release (hSprite);
	ASSERT (vSprite == nullptr)
	safe_release (vSprite);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CrossHairCursor::isVisible () const
{
	return visible;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CrossHairCursor::setVisible (bool state)
{
	if(state != visible)
	{
		visible = state;
		if(editView->isAttached ())
		{
			if(visible)
				showSprite ();
			else
				hideSprite ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CrossHairCursor::move (PointRef p)
{
	position = p;
	updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CrossHairCursor::attached ()
{
	if(visible)
		showSprite ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CrossHairCursor::removed  ()
{
	hideSprite ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CrossHairCursor::updateSize ()
{
	Rect clientRect;
	editView->getClientRect (clientRect);

	if(hSprite)
	{
		Rect r (0, position.y, clientRect.getWidth (), position.y + width);
		hSprite->move (r);
	}

	if(vSprite)
	{
		Rect r (position.x, 0, position.x + width, clientRect.getHeight ());
		vSprite->move (r);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CrossHairCursor::showSprite ()
{
	ASSERT (hSprite == nullptr && vSprite == nullptr)
	if(hSprite == nullptr && vSprite == nullptr)
	{
		AutoPtr<EditCursorShape> shape = NEW EditCursorShape (*editView);
		
		UID cid (getSpriteClass ());
		if(cid == ClassID::FloatingSprite)
			shape->takeOpacity ();
		
		hSprite = ccl_new<ISprite> (cid);
		ASSERT (hSprite != nullptr)
		hSprite->construct (*editView, Rect (), static_cast<SolidDrawable*> (shape));
		hSprite->show ();

		vSprite = ccl_new<ISprite> (cid);
		ASSERT (vSprite != nullptr)
		vSprite->construct (*editView, Rect (), static_cast<SolidDrawable*> (shape));
		vSprite->show ();

		updateSize (); // init size
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CrossHairCursor::hideSprite ()
{
	if(hSprite)
		hSprite->hide (),
		hSprite->release (),
		hSprite = nullptr;

	if(vSprite)
		vSprite->hide (),
		vSprite->release (),
		vSprite = nullptr;
}
