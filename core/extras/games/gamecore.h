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
// Filename    : core/extras/games/gamecore.h
// Description : Game core
//
//************************************************************************************************

#ifndef _gamecore_h
#define _gamecore_h

#include "core/public/gui/coregameinterface.h"

#include "core/portable/gui/coregraphics.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// GameCoreFactory
//************************************************************************************************

template <class T>
struct GameCoreFactory: Core::Plugins::ClassFactory<T, IGameCore> {};

//************************************************************************************************
// BitmapGameCore
/** Game core base class rendering to bitmap. */
//************************************************************************************************

class BitmapGameCore: public IGameCore,
					  public IGameBitmapRenderer
{
public:
	BitmapGameCore ();
	virtual ~BitmapGameCore ();

	// IPropertyHandler
	void setProperty (const Property& value);
	void getProperty (Property& value);
	void release ();

	// IGameCore
	ErrorCode startup (IGameEnvironment* environment);
	void shutdown ();
	int run ();

	// IGameBitmapRenderer
	ErrorCode renderFrame (BitmapData& data, int offsetX, int offsetY);

protected:
	IGameEnvironment* environment;
	Rect screenRect;

	virtual void renderFrame (Graphics& g, RectRef updateRect);
	
	template <class RendererClass>
	void renderInternal (BitmapData& data, int offsetX, int offsetY);
};

} // namespace Portable
} // namespace Core

#endif // _gamecore_h
