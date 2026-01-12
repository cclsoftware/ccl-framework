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
// Filename    : core/extras/games/testgame.cpp
// Description : Test Game
//
//************************************************************************************************

#include "core/extras/games/testgame.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// TestGame
//************************************************************************************************

TestGame::TestGame ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode TestGame::startup (IGameEnvironment* environment)
{
	BitmapGameCore::startup (environment);

	spriteRect (0, 0, 16, 16);
	spriteRect.center (screenRect);

	return Errors::kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestGame::shutdown ()
{
	BitmapGameCore::shutdown ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TestGame::run ()
{
	bool dirty = false;

	if(environment->isJoypadButtonPressed (kJoypadLeft))
	{
		if(spriteRect.left > 0)
		{
			spriteRect.offset (-1);
			dirty = true;
		}
	}
	else if(environment->isJoypadButtonPressed (kJoypadRight))
	{
		if(spriteRect.right < screenRect.right)
		{
			spriteRect.offset (+1);
			dirty = true;
		}
	}

	return dirty ? kFrameDirty : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TestGame::renderFrame (Graphics& g, RectRef updateRect)
{
	g.fillRect (spriteRect, Colors::kGreen);
}
