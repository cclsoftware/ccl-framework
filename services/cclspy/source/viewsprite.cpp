//************************************************************************************************
//
// CCL Spy
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
// Filename    : viewsprite.cpp
// Description : View Highlite Sprite
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "viewsprite.h"

#include "ccl/public/guiservices.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

using namespace Spy;
using namespace CCL;

//************************************************************************************************
// ViewSpriteDrawable
//************************************************************************************************

class ViewSpriteDrawable: public Unknown,
						  public AbstractDrawable
{
public:
	ViewSpriteDrawable (ViewSprite& sprite, bool floating)
	: sprite (sprite),
	  floating (floating)
	{}

	PROPERTY_BOOL (floating, Floating)

	// IDrawable
	void CCL_API draw (const DrawArgs& args) override
	{
		Rect rect (args.size);

		if(floating == true)
		{
			Color c (sprite.getBackColor ());
			args.graphics.fillRect (rect, SolidBrush (c));

			c (sprite.getFrameColor ());
			args.graphics.drawRect (rect, Pen (c, 5));
		}
		else
		{
			if(sprite.getBackColor ().alpha != 0)
			{
				Color c (sprite.getBackColor ());
				c.setAlphaF (.15f);
				args.graphics.fillRect (rect, SolidBrush (c));
			}

			Color c (sprite.getFrameColor ());
			c.setAlphaF (.2f);
			args.graphics.drawRect (rect, Pen (c, 5));
			c.setAlphaF (.7f);
			args.graphics.drawRect (rect, Pen (c));

			if(sprite.isShowInfo ())
			{
				Coord w = rect.getWidth ();
				Coord h = rect.getHeight ();
				if(w >= 40 && h >= 10)
				{
					String s;
					s << w << " x " << h;

					Rect textRect;
					Font font ("Arial", 10);
					Font::measureString (textRect, s, font);
					textRect.moveTo (rect.getLeftTop ());

					Color back (sprite.getBackColor ());
					ColorHSV hsv (back);
					hsv.s = 0.6f;
					hsv.v = 1.f;
					hsv.a = 0.8f;
					hsv.toColor (back);
					args.graphics.fillRect (textRect, SolidBrush (back));

					hsv.v = 0.2f;
					hsv.s = 0.2f;
					hsv.a = 1.f;
					hsv.toColor (c);
					args.graphics.drawString (textRect, s, font, SolidBrush (c), Alignment::kLeftTop);
				}
			}
		}
	}

	float CCL_API getOpacity () const override
	{
		return floating ? 0.35f : 1.f;
	}

	CLASS_INTERFACE (IDrawable, Unknown)

private:
	ViewSprite& sprite;
};

//************************************************************************************************
// ViewSprite
//************************************************************************************************

ViewSprite::ViewSprite ()
: view (nullptr),
  viewSubject (nullptr),
  window (nullptr),
  showUntil (-1),
  nextUpdate (0),
  backColor (Colors::kRed),
  frameColor (Colors::kRed),
  showUntilMouseUp (false),
  showInfo (false)
{
	backColor.alpha = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewSprite::~ViewSprite ()
{
	hide ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewSprite::calcSize (Rect& rect)
{
	if(view)
	{
		rect = view->getSize ();
		ccl_lower_limit (rect.bottom, rect.top + 2);
		ccl_lower_limit (rect.right, rect.left + 2);

		// use window as reference view for sprite, so we can show the full size
		Point pos;
		if(window)
			view->clientToWindow (pos);
		rect.moveTo (pos);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewSprite::show (IView* view, int64 duration)
{
	hide ();

	if(view && ViewBox (view).isAttached ())
	{
		this->view = view;
		viewSubject = UnknownPtr<ISubject> (view);
		if(viewSubject)
			viewSubject->addObserver (this);

		window = view->getIWindow ();
		ASSERT (window)

		Rect rect;
		calcSize (rect);

		if(window)
		{
			view = UnknownPtr<IView> (window); // use window as reference view for sprite
			window->addHandler (this);
		}

		sprite = ccl_new<ISprite> (GraphicsFactory::hasGraphicsLayers () ? CCL::ClassID::SublayerSprite : CCL::ClassID::FloatingSprite);
		if(sprite)
		{
			AutoPtr<ViewSpriteDrawable> drawable (NEW ViewSpriteDrawable (*this, false));
			sprite->construct (view, rect, drawable);
			sprite->show ();

			showUntil = -1;
			if(duration >= 0)
				showUntil = System::GetSystemTicks () + duration;

			nextUpdate = System::GetSystemTicks () + kUpdateFreq;

			System::GetGUI ().addIdleTask (this);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ViewSprite::hide ()
{
	if(sprite)
	{
		System::GetGUI ().removeIdleTask (this);
		showUntil = -1;

		if(viewSubject)
		{
			viewSubject->removeObserver (this);
			viewSubject = nullptr;
			view = nullptr;
		}

		if(window)
		{
			window->removeHandler (this);
			window = nullptr;
		}
		sprite->hide ();
		sprite.release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ViewSprite::isVisible () const
{
	return sprite.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ViewSprite::onWindowEvent (WindowEvent& windowEvent)
{
	if(windowEvent.eventType == WindowEvent::kClose)
		hide ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ViewSprite::onTimer (ITimer* timer)
{
	if(isVisible ())
	{
		int64 now = System::GetSystemTicks ();

		if(showUntilMouseUp)
		{
			KeyState keys;
			System::GetGUI ().getKeyState (keys);
			if(keys.isSet (KeyState::kMouseMask|KeyState::kCommand))
			{
				if(now >= showUntil)
					showUntil = now + 300;
			}
			else
				showUntilMouseUp = false;
		}

		if(showUntil >= 0 && now >= showUntil)
			hide ();
		else if(now >= nextUpdate)
		{
			Rect rect;
			calcSize (rect);
			sprite->move (rect);

			nextUpdate = now + kUpdateFreq;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ViewSprite::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kDestroyed)
		hide ();
}
