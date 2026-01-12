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
// Filename    : core/extras/games/staticgameenv.cpp
// Description : Game environment for static views
//
//************************************************************************************************

#include "core/extras/games/staticgameenv.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// StaticGameEnvironment
//************************************************************************************************

StaticGameEnvironment::StaticGameEnvironment (BitmapPixelFormat screenFormat)
: gameView (nullptr),
  screenFormat (screenFormat),
  joypadState (0),
  game (nullptr),
  renderer (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticGameEnvironment::setProperty (const Property& value)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticGameEnvironment::getProperty (Property& value)
{
	if(ImplementGetInterface<StaticGameEnvironment, IGameEnvironment> (this, value))
		return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticGameEnvironment::release ()
{
	ASSERT (0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticGameEnvironment::setGame (IGameCore* _game)
{
	game = _game;
	renderer = GetInterface<IGameBitmapRenderer> (game);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticGameEnvironment::startGame (bool state)
{
	if(game)
	{
		if(state)
		{
			game->startup (this);

			if(gameView)
				gameView->setPainter (this);
		}
		else
		{
			game->shutdown ();

			if(gameView)
				gameView->setPainter (nullptr);
		}

		if(gameView)
			gameView->invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticGameEnvironment::runGame ()
{
	if(game)
	{
		int result = game->run ();
		if(result & IGameCore::kFrameDirty)
		{
			if(gameView)
				gameView->invalidate ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int StaticGameEnvironment::getScreenWidth () const
{
	return gameView ? gameView->getSize ().getWidth () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int StaticGameEnvironment::getScreenHeight () const
{
	return gameView ? gameView->getSize ().getHeight () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int StaticGameEnvironment::getScreenFormat () const
{
	return screenFormat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool StaticGameEnvironment::isJoypadButtonPressed (JoypadButton button) const
{
	return get_bit (joypadState, button);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticGameEnvironment::setJoypadButtonPressed (JoypadButton button, bool state)
{
	set_bit (joypadState, button, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticGameEnvironment::resetJoypadState ()
{
	joypadState = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int StaticGameEnvironment::getPointerValue (PointerValue which) const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StaticGameEnvironment::drawView (const StaticView& view, const DrawEvent& e)
{
	ASSERT (renderer)
	if(!renderer)
		return;

	// TODO:
	// - limit to visible client of view
	// - don't clear background twice (see root view)
	if(auto g = core_cast<BitmapGraphicsRenderer> (&e.graphics))
	{
		BitmapData& data = g->getBitmap ().accessForWrite ();
		renderer->renderFrame (data, 0, 0);
	}
}
