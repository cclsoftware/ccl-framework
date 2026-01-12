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
// Filename    : ccl/app/controls/itemselectorpopup.cpp
// Description : Item Selector Popup
//
//************************************************************************************************

#include "ccl/app/controls/itemselectorpopup.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/storage.h"

#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/text/iregexp.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// IItemsProvider
//************************************************************************************************

DEFINE_IID_ (IItemsProvider, 0xf370e312, 0x379b, 0x48e7, 0x84, 0x88, 0x1f, 0x5e, 0x71, 0xab, 0x88, 0x73)

//************************************************************************************************
// ItemSelectorPopup
//************************************************************************************************

ItemSelectorPopup::ItemSelectorPopup (IItemsProvider* itemsProvider, String startString)
: itemsProvider (itemsProvider),
  selectedItem (nullptr),
  regExp (nullptr),
  wasTimeOutSelection (false),
  wantsEnter (false),
  matchResultNumber (false),
  startString (startString)
{
	ASSERT (itemsProvider)
	selectedItemParam = paramList.addString ("selectedItem");
	typedStringParam = paramList.addString ("typedString");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemSelectorPopup::~ItemSelectorPopup ()
{
	safe_release (regExp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes* ItemSelectorPopup::getSettings () const
{
	if(!getPersistenceID ().isEmpty ())
	{
		String path ("ItemSelector/");
		path << getPersistenceID ();
		return &Settings::instance ().getAttributes (path);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemSelectorPopup::storeSettings () const
{
	if(Attributes* a = getSettings ())
	{
		a->set ("typed", typedString);
		a->set ("recent", recentChoices, true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemSelectorPopup::restoreSettings ()
{
	if(Attributes* a = getSettings ())
	{
		a->get (typedString, "typed");
		a->get (recentChoices, "recent");
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemSelectorPopup::setRecentItem (IUnknown& item)
{
	String title (itemsProvider->getUnknownTitle (&item));
	if(!title.isEmpty ())
	{
		if(!recentChoices.moveToHead (title))
			recentChoices.prepend (title);

		// remove oldest
		while(recentChoices.count () > kMaxRecentChoices)
			recentChoices.removeLast ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* ItemSelectorPopup::run ()
{
	ASSERT (asyncOperation == nullptr)
	asyncOperation = NEW AsyncOperation;
	asyncOperation->retain ();
	asyncOperation->setState (AsyncOperation::kStarted);

	if(!startString.isEmpty ())
	{
		// optional search string provided from outside: select immediately (if match found)
		typedString = startString;
		checkTypedString ();
		if(selectedItem)
		{
			asyncOperation->setResult (selectedItem);
			asyncOperation->setState (AsyncOperation::kCompleted);
			return asyncOperation;
		}
	}
	else if(restoreSettings () && !typedString.isEmpty ())
	{
		// restore last typedString and recently chosen titles
		setTypedStringTemporary (true); // typed string will be shown disabled, but overwritten on first key input
		checkTypedString (false);

		StringRef lastChoice = recentChoices.at (0);
		if(!lastChoice.isEmpty ())
			if(IUnknown* lastCandidate = findCandidate (lastChoice))
				selectCandidate (lastCandidate);
	}

	popupSelector = ccl_new<IPopupSelector> (CCL::ClassID::PopupSelector);
	if(popupSelector)
	{
		PopupSizeInfo sizeInfo (nullptr, PopupSizeInfo::kHCenter|PopupSizeInfo::kVCenter);
		popupSelector->setTheme (getTheme ());
		Promise (popupSelector->popupAsync (this, sizeInfo)).then ([&] (IAsyncOperation& popupOperation)
		{
			if(selectedItem)
			{
				setRecentItem (*selectedItem);
				storeSettings ();

				asyncOperation->setResult (selectedItem);
			}
			asyncOperation->setState (AsyncOperation::kCompleted);
		});
	}

	return asyncOperation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API ItemSelectorPopup::createPopupView (SizeLimit& limits)
{
	ITheme* theme = getTheme ();
	ASSERT (theme != nullptr)
	return theme ? theme->createView (getFormName (), this->asUnknown ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemSelectorPopup::matchesItemTitle (IUnknown& item, bool matchStart) const
{
	String title = itemsProvider->getUnknownTitle (&item);

	if(matchStart == false)
		return title.contains (typedString, false);

	if(!getItemTitleSeparator ().isEmpty ())
	{
		ASSERT (regExp)
		if(regExp)
			return regExp->isFullMatch (title);
	}

	// simple check at beginning of title when no titleSeparator given (also a fallback in case regExp construction failed)
	if(title.startsWith (typedString, false))
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ItemSelectorPopup::findShortcutItem (const KeyEvent& shortcut)
{
	KeyEvent key (shortcut);
	if(key.isVKeyValid ())
		key.character = 0;
	else
		key.character = Unicode::toUppercase (key.character);

	IterForEachUnknown (itemsProvider->newUnknownIterator (), item)
		if(key == itemsProvider->getUnknownShortcut (item))
			return item;
	EndFor

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemSelectorPopup::prepareSearch ()
{
	safe_release (regExp);

	if(!typedString.isEmpty ())
	{
		// build regular expression matching each word of the typed string at the beginning of a word in a title string
		String expression (".*");
		ForEachStringToken (typedString, getItemTitleSeparator (), typedToken)
			expression << "\\b" << typedToken << ".*"; // \b for word boundary
		EndFor

		regExp = System::CreateRegularExpression ();
		if(regExp->construct (expression, IRegularExpression::kCaseInsensitive) != kResultOk)
			safe_release (regExp);

		CCL_PRINTF ("ItemSelectorPopup: regexp: %s\n", MutableCString (expression).str ())
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemSelectorPopup::collectCandidates ()
{
	prepareSearch ();

	auto addMatches = [this](bool onStart = true)
	{
		if(itemsProvider)
		{
			IterForEachUnknown (itemsProvider->newUnknownIterator (), item)
				if(matchesItemTitle (*item, onStart))
					candidates.addOnce (item);
			EndFor
		}
	};

	candidates.removeAll ();
	if(itemsProvider && !typedString.isEmpty ())
	{
		if(isMatchResultNumber ())
		{
			int64 number;
			if(typedString.getIntValue (number) && number > 0)
			{
				// check item numbers
				int i = 1;
				IterForEachUnknown (itemsProvider->newUnknownIterator (), item)
					String s;
					s << i++;
					if(s.startsWith (typedString))
						candidates.add (item);
				EndFor
			}
		}

		// add matches on start/token start first
		addMatches (true);

		// extend list with matches in complete string
		addMatches (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ItemSelectorPopup::findCandidate (StringRef title) const
{
	for(auto item : candidates)
	{
		String itemTitle (itemsProvider->getUnknownTitle (item));
		if(itemTitle == title)
			return item;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemSelectorPopup::selectCandidate (IUnknown* item)
{
	selectedItem = item;

	String title;
	if(selectedItem)
		title = itemsProvider->getUnknownTitle (selectedItem);

	selectedItemParam->setValue (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API ItemSelectorPopup::onKeyDown (const KeyEvent& event)
{
	stopTimer ();

	// first key: check shortcuts
	if(typedString.isEmpty ())
		if(IUnknown* item = findShortcutItem (event))
		{
			selectCandidate (item);
			if(wantsEnter)
				return IPopupSelectorClient::kSwallow;
			else
				return IPopupSelectorClient::kOkay;
		}

	if(event.isCharValid () && Unicode::isPrintable (event.character))
	{
		setTypedStringTemporary (false);

		// character typed
		uchar character[2] = { event.character, 0 };
		typedString.append (character);
		return checkTypedString ();
	}
	else if(event.isVKeyValid ())
	{
		switch(event.vKey)
		{
		case VKey::kBackspace:
			setTypedStringTemporary (false);
			typedString.truncate (typedString.length () - 1);
			return checkTypedString ();

		case VKey::kUp:
		case VKey::kDown:
			if(selectedItem)
			{
				// arrow keys select between multiple matches
				int index = candidates.index (selectedItem);
				index += (event.vKey == VKey::kUp) ? - 1 : 1;
				index = ccl_bound (index, 0, candidates.count () - 1);

				selectCandidate (candidates.at (index));
				return IPopupSelectorClient::kSwallow;
			}

		case VKey::kReturn:
		case VKey::kEnter:
			return hasPopupResult () ? kOkay : kCancel; // different from PopupSelectorClient default behavior: also close when no result
			break;
		}
	}
	return PopupSelectorClient::onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result ItemSelectorPopup::checkTypedString (bool acceptFirstMatch)
{
	typedStringParam->fromString (typedString);

	collectCandidates ();
	selectedItem = !candidates.isEmpty () ? candidates[0] : nullptr;

	if(!getPersistenceID ().isEmpty ())
	{
		// prefer recently chosen items (titles in recentChoices)
		selectedItem = nullptr;
		int selectedRecentIndex = NumericLimits::kMaxInt;

		for(auto item : candidates)
		{
			String title (itemsProvider->getUnknownTitle (item));
			int recentIndex = recentChoices.index (title);
			if(recentIndex < 0)
				recentIndex = NumericLimits::kMaxInt;

			if(!selectedItem || recentIndex < selectedRecentIndex)
			{
				selectedItem = item;
				selectedRecentIndex = recentIndex;
				if(recentIndex == 0)
					break;
			}
		}
	}

	selectCandidate (selectedItem);

	if(selectedItem && !wantsEnter && acceptFirstMatch)
	{
		if(candidates.count () > 1)
			startTimer (kTimeOutMs, false);
		else
			return IPopupSelectorClient::kOkay;
	}
	return IPopupSelectorClient::kSwallow;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemSelectorPopup::setTypedStringTemporary (bool temporary)
{
	bool wasTemporary = !typedStringParam->isEnabled ();
	typedStringParam->enable (!temporary);

	if(wasTemporary)
	{
		typedString.empty ();
		typedStringParam->fromString (typedString);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemSelectorPopup::hasPopupResult ()
{
	return selectedItem != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemSelectorPopup::onIdleTimer ()
{
	if(hasPopupResult ())
	{
		// timeout (multiple matches): close popup, will use currently selected item
		wasTimeOutSelection = true;

		if(popupSelector)
			popupSelector->close ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemSelectorPopup::onPopupClosed (Result result)
{
	if(result != IPopupSelectorClient::kOkay && !wasTimeOutSelection)
		selectedItem = nullptr;
}

