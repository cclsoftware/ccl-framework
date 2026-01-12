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
// Filename    : ccl/gui/touch/touchcollection.cpp
// Description : Touch Collection
//
//************************************************************************************************

#include "ccl/gui/touch/touchcollection.h"

using namespace CCL;

//************************************************************************************************
// TouchCollection
//************************************************************************************************

void TouchCollection::copyFrom (const ITouchCollection& other)
{
	removeAll ();
	for(int i = 0, num = other.getTouchCount (); i < num; i++)
		add (other.getTouchInfo (i));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TouchCollection::getTouchCount () const
{
	return count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const TouchInfo& CCL_API TouchCollection::getTouchInfo (int index) const
{
	return at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const TouchInfo* CCL_API TouchCollection::getTouchInfoByID (TouchID id) const 
{
	for(int i = 0, num = getTouchCount (); i < num; i++)
	{
		const TouchInfo& touch = getTouchInfo (i);
		if(touch.id == id)
			return &touch;
	}
	return nullptr;
}
