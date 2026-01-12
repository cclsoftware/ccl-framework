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
// Filename    : core/extras/games/testgame.h
// Description : Test Game
//
//************************************************************************************************

#ifndef _testgame_h
#define _testgame_h

#include "core/extras/games/gamecore.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// TestGame
//************************************************************************************************

class TestGame: public BitmapGameCore
{
public:
	TestGame ();

	// BitmapGameCore
	ErrorCode startup (IGameEnvironment* environment);
	void shutdown ();
	int run ();

protected:
	Rect spriteRect;

	// BitmapGameCore
	void renderFrame (Graphics& g, RectRef updateRect);
};

} // namespace Portable
} // namespace Core

#endif // _testgame_h
