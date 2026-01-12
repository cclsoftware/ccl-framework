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
// Filename    : core/extras/games/gamecore.cpp
// Description : Game core
//
//************************************************************************************************

#include "core/extras/games/gamecore.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// BitmapGameCore
//************************************************************************************************

BitmapGameCore::BitmapGameCore ()
: environment (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

BitmapGameCore::~BitmapGameCore ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapGameCore::setProperty (const Property& value)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapGameCore::getProperty (Property& value)
{
	if(ImplementGetInterface<BitmapGameCore, IGameCore> (this, value))
		return;
	if(ImplementGetInterface<BitmapGameCore, IGameBitmapRenderer> (this, value))
		return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapGameCore::release ()
{
	delete this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode BitmapGameCore::startup (IGameEnvironment* _environment)
{
	ASSERT (_environment)
	if(!_environment)
		return Errors::kError_InvalidArgument;

	environment = _environment;
	screenRect (0, 0, environment->getScreenWidth (), environment->getScreenHeight ());

	return Errors::kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapGameCore::shutdown ()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int BitmapGameCore::run ()
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RendererClass>
void BitmapGameCore::renderInternal (BitmapData& data, int offsetX, int offsetY)
{
	Bitmap bitmap (data);
	RendererClass graphics (bitmap);
	graphics.setOrigin (Point (-offsetX, -offsetY));
	Rect updateRect (offsetX, offsetY, offsetX + data.width, offsetY + data.height);
	renderFrame (graphics, updateRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ErrorCode BitmapGameCore::renderFrame (BitmapData& data, int offsetX, int offsetY)
{
	switch(data.format)
	{
	case kBitmapRGBAlpha :
		renderInternal<ColorBitmapRenderer> (data, offsetX, offsetY);
		break;

	case kBitmapRGB565 :
		renderInternal<RGB565BitmapRenderer> (data, offsetX, offsetY);
		break;

	case kBitmapMonochrome :
		renderInternal<MonoBitmapRenderer> (data, offsetX, offsetY);
		break;
		
	default :
		ASSERT (0) // implement if needed
		return Errors::kError_InvalidArgument;
	}

	return Errors::kError_NoError;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BitmapGameCore::renderFrame (Graphics& g, RectRef updateRect)
{
}
