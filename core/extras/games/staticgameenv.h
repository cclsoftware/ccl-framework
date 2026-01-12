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
// Filename    : core/extras/games/staticgameenv.h
// Description : Game environment for static views
//
//************************************************************************************************

#ifndef _staticgameenv_h
#define _staticgameenv_h

#include "core/public/gui/coregameinterface.h"

#include "core/portable/gui/corestaticview.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// StaticGameEnvironment
//************************************************************************************************

class StaticGameEnvironment: public IGameEnvironment,
							 public StaticViewPainter
{
public:
	StaticGameEnvironment (BitmapPixelFormat screenFormat);

	PROPERTY_POINTER (StaticCustomView, gameView, GameView)

	void setGame (IGameCore* game);
	void startGame (bool state);
	void runGame ();

	void setJoypadButtonPressed (JoypadButton button, bool state);
	void resetJoypadState ();

	// IPropertyHandler
	void setProperty (const Property& value);
	void getProperty (Property& value);
	void release ();

	// IGameEnvironment
	int getScreenWidth () const;
	int getScreenHeight () const;
	int getScreenFormat () const;
	bool isJoypadButtonPressed (JoypadButton button) const;
	int getPointerValue (PointerValue which) const;

	// IStaticViewPainter
	void drawView (const StaticView& view, const DrawEvent& e);

protected:
	BitmapPixelFormat screenFormat;
	int joypadState;
	IGameCore* game;
	IGameBitmapRenderer* renderer;
};

} // namespace Portable
} // namespace Core

#endif // _staticgameenv_h
