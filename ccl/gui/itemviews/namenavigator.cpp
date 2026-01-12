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
// Filename    : ccl/public/gui/framework/inamenavigator.cpp
// Description : Name Navigator
//
//************************************************************************************************

#include "ccl/gui/itemviews/namenavigator.h"

#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/base/debug.h"

using namespace CCL;

//************************************************************************************************
// NameNavigator
//************************************************************************************************

NameNavigator::NameNavigator (INamedItemIterator* iterator)
: iterator (iterator),
  sameChars (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NameNavigator::init (INamedItemIterator* iter)
{
	iterator = iter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NameNavigator::onIdleTimer ()
{
	typedString.empty ();
	sameChars = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NameNavigator::getNextItem ()
{
	return iterator->getNextItem (currentItem, currentName)
		&& currentItem != startItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NameNavigator::resetItem ()
{
	currentItem.clear ();
	currentName.empty ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NameNavigator::reset ()
{
	resetItem ();

	startItem.clear ();
	typedString.empty ();
	sameChars = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NameNavigator::onKey (Variant& resultItem, const KeyEvent& event)
{
	if(!iterator)
		return false;

	if(event.isCharValid ()
		&& event.state.getModifiers () == 0
		&& Unicode::isPrintable (event.character))
	{
		if(typedString.isEmpty ())
		{
			if(iterator->getStartItem (currentItem, currentName))
				startItem = currentItem;
			else
			{
				reset ();
				return false;
			}
		}

		startTimer (kTimeOutMs, false);

		uchar character[2] = { event.character, 0 };
		typedString.append (character);

		if(event.character != typedString.at (0))
			sameChars = false;

		Variant prevItem (currentItem);
		CCL_PRINTF ("typed \"%s\"  currentName \"%s\" \n", MutableCString (typedString).str (), MutableCString (currentName).str ())

		do // search for a matching item
		{
			if(currentName.startsWith (typedString, false))
			{
				resultItem = currentItem;
				return true;
			}
		}
		while(getNextItem ());

		// no match found; special case when typed the same character again: go to next item starting with that character
		if(sameChars && prevItem.isValid () && typedString.length () > 1)
		{
			currentItem = prevItem;
			while(getNextItem ())
				 if(currentName.startsWith (character, false))
				 {
					resultItem = currentItem;
					return true;
				 }
		}
	}
	else
	{
		// invalid character
		stopTimer ();
		typedString.empty ();
		sameChars = true;
	}
	return resetItem ();
}
