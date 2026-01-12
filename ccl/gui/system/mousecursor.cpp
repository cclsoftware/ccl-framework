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
// Filename    : ccl/gui/system/mousecursor.cpp
// Description : Mouse Cursor
//
//************************************************************************************************

#include "ccl/gui/system/mousecursor.h"

using namespace CCL;

//************************************************************************************************
// MouseCursor
//************************************************************************************************

MouseCursorFactory* MouseCursor::factory = nullptr;
void MouseCursor::setFactory (MouseCursorFactory* _factory)
{
	factory = _factory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseCursor* MouseCursor::createCursor (int themeCursorId)
{
	return factory ? factory->createCursor (themeCursorId) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseCursor* MouseCursor::createCursor (Image& image, PointRef hotspot)
{
	MouseCursor* cursor = factory ? factory->createCursor (image, hotspot) : nullptr;
	if(cursor)
		cursor->setOriginalImage (&image);
	return cursor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (MouseCursor, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseCursor::MouseCursor (bool ownCursor)
: ownCursor (ownCursor)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MouseCursor::makeCurrent ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API MouseCursor::createImage () const
{
	if(originalImage)
		return return_shared<IImage> (originalImage);
	else
		return nullptr; // TODO: create image from native cursor?
}
