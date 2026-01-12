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
// Filename    : ccl/app/navigation/navigatorbase.cpp
// Description : Navigator base class
//
//************************************************************************************************

#include "ccl/app/navigation/navigatorbase.h"

using namespace CCL;

//************************************************************************************************
// NavigationHistoryEntry
//************************************************************************************************

DEFINE_CLASS (NavigationHistoryEntry, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

NavigationHistoryEntry::NavigationHistoryEntry (UrlRef url, StringRef title)
: url (url),
  title (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API NavigationHistoryEntry::getUrl () const
{
	return url; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API NavigationHistoryEntry::getTitle () const
{
	return title; 
}

//************************************************************************************************
// NavigationHistory
//************************************************************************************************

DEFINE_CLASS (NavigationHistory, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

NavigationHistory::NavigationHistory ()
{
	entries.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NavigationHistory::push (NavigationHistoryEntry* entry, bool check)
{ 
	if(check)
	{
		// remove all duplicated entries
		NavigationHistoryEntry* e = nullptr;
		while((e = peek ()) != nullptr)
		{
			if(!e->getUrl ().isEqualUrl (entry->getUrl ()))
				break;
			pop ()->release ();
		}
	}
	
	entries.push (entry); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NavigationHistoryEntry* NavigationHistory::at (int index) const
{
	return static_cast<NavigationHistoryEntry*> (entries.at (index));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NavigationHistoryEntry* NavigationHistory::peek () const
{
	return static_cast<NavigationHistoryEntry*> (entries.peek ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NavigationHistoryEntry* NavigationHistory::pop ()
{
	return static_cast<NavigationHistoryEntry*> (entries.pop ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NavigationHistory::checkDuplicates ()
{
	NavigationHistoryEntry* first = pop ();
	if(first)
	{
		NavigationHistoryEntry* e = nullptr;
		while((e = peek ()) != nullptr)
		{
			if(!e->getUrl ().isEqualUrl (first->getUrl ()))
				break;
			pop ()->release ();
		}

		entries.push (first);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API NavigationHistory::countEntries () const
{
	return entries.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const INavigationHistoryEntry* CCL_API NavigationHistory::getEntry (int index) const
{
	return at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const INavigationHistoryEntry* CCL_API NavigationHistory::peekEntry () const
{
	return peek ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NavigationHistory::dump (CStringPtr name)
{
	if(name && name[0])
	{
		Debugger::print ("[");
		Debugger::print (name);
		Debugger::println ("]");
	}

	int i = 0;
	ForEach (entries, NavigationHistoryEntry, e)
		String url;
		e->getUrl ().getUrl (url);
		Debugger::printf ("%d: \"", ++i);
		Debugger::print (url);
		Debugger::print ("\" \"");
		Debugger::print (e->getTitle ());
		Debugger::println ("\"");
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Navigator Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (NavigatorBase)
	DEFINE_COMMAND ("Navigation", "Back",    NavigatorBase::onBackCmd)
	DEFINE_COMMAND ("Navigation", "Forward", NavigatorBase::onForwardCmd)
	DEFINE_COMMAND ("Navigation", "Home",    NavigatorBase::onHomeCmd)
	DEFINE_COMMAND ("Navigation", "Refresh", NavigatorBase::onRefreshCmd)
END_COMMANDS (NavigatorBase)

//************************************************************************************************
// NavigatorBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (NavigatorBase, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

NavigatorBase::NavigatorBase (StringRef name, StringRef title)
: Component (name, title)
{
	paramList.addCommand (CSTR ("Navigation"), CSTR ("Back"),    CSTR ("goBack"));
	paramList.addCommand (CSTR ("Navigation"), CSTR ("Forward"), CSTR ("goForward"));
	paramList.addCommand (CSTR ("Navigation"), CSTR ("Home"),    CSTR ("goHome"));
	paramList.addCommand (CSTR ("Navigation"), CSTR ("Refresh"), CSTR ("refresh"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NavigatorBase::isOpen () const
{
	return contentFrame.isValid (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NavigatorBase::setHomeUrl (UrlRef url)
{
	homeUrl.assign (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NavigatorBase::setCurrentUrl (UrlRef url)
{
	currentUrl.assign (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NavigatorBase::navigateDeferred (UrlRef url)
{
	return navigate (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API NavigatorBase::getCurrentUrl () const
{
	return currentUrl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API NavigatorBase::getCurrentTitle () const
{
	return currentTitle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NavigatorBase::goHome ()
{
	return navigate (homeUrl);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef CCL_API NavigatorBase::getHomeUrl () const
{
	return homeUrl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_COMMANDS (NavigatorBase, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NavigatorBase::onBackCmd (CmdArgs args)
{
	if(!isOpen ())
		return false;

	if(args.checkOnly ())
		return canGoBack () ? true : false;

	goBack ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NavigatorBase::onForwardCmd (CmdArgs args)
{
	if(!isOpen ())
		return false;

	if(args.checkOnly ())
		return canGoForward () ? true : false;

	goForward ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NavigatorBase::onHomeCmd (CmdArgs args)
{
	if(args.checkOnly ())
		return !getHomeUrl ().isEmpty ();

	goHome ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NavigatorBase::onRefreshCmd (CmdArgs args)
{
	if(args.checkOnly ())
		return isOpen ();

	refresh ();
	return true;
}

//************************************************************************************************
// NavigatorBase2
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (NavigatorBase2, NavigatorBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

NavigatorBase2::NavigatorBase2 (StringRef name, StringRef title)
: NavigatorBase (name, title),
  backwardHistory (NEW NavigationHistory),
  forwardHistory (NEW NavigationHistory),
  navigatorFlags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

NavigatorBase2::~NavigatorBase2 ()
{
	safe_release (backwardHistory);
	safe_release (forwardHistory);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NavigatorBase2::navigate (UrlRef url)
{
	if(currentUrl != url)
	{
		if(!historyDisabled ())
		{
			pushCurrent (backwardHistory);
		}

		currentUrl = url;
		onNavigated ();
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NavigatorBase2::onNavigated ()
{
	backwardHistory->checkDuplicates ();
	forwardHistory->checkDuplicates ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NavigatorBase2::pushCurrent (NavigationHistory* history)
{
	if(!currentUrl.isEmpty ())
		history->push (NEW NavigationHistoryEntry (currentUrl, currentTitle));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NavigatorBase2::goBack ()
{
	tresult result = kResultFalse;
	bool oldState = historyDisabled ();
	historyDisabled (true);

	AutoPtr<NavigationHistoryEntry> e = backwardHistory->pop ();
	if(e)
	{
		pushCurrent (forwardHistory);
		result = navigate (e->getUrl ());
	}

	historyDisabled (oldState);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API NavigatorBase2::goForward ()
{
	tresult result = kResultFalse;
	bool oldState = historyDisabled ();
	historyDisabled (true);

	AutoPtr<NavigationHistoryEntry> e = forwardHistory->pop ();
	if(e)
	{
		pushCurrent (backwardHistory);
		result = navigate (e->getUrl ());
	}

	historyDisabled (oldState);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NavigatorBase2::canGoBack () const
{
	return backwardHistory->peek () != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NavigatorBase2::canGoForward () const
{
	return forwardHistory->peek () != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INavigationHistory& CCL_API NavigatorBase2::getBackwardHistory () const
{
	ASSERT (backwardHistory)
	return *backwardHistory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

INavigationHistory& CCL_API NavigatorBase2::getForwardHistory () const
{
	ASSERT (forwardHistory)
	return *forwardHistory;
}
