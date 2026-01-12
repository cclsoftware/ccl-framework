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
// Filename    : ccl/app/components/searchcomponent.cpp
// Description : Search Component
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/components/searchcomponent.h"
#include "ccl/app/components/searchprovider.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/storage.h"

#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/isearcher.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/guievent.h"

namespace CCL {

//************************************************************************************************
// SearchResult
//************************************************************************************************

class SearchResult: public Unknown,
					public ISearchResultSink
{
public:
	SearchResult (SearchComponent* component)
	: component (component)
	{}

	PROPERTY_SHARED_AUTO (ISearcher, searcher, Searcher)
	PROPERTY_STRING (searchTerms, SearchTerms)

	// ISearchResultSink
	tresult CCL_API addResult (IUnknown* item) override
	{
		if(component)
			(NEW Message ("Result", Variant (item, true)))->post (component);
		item->release ();
		return kResultOk;
	}

	tresult CCL_API addResults (const IUnknownList& items) override
	{
		ForEachUnknown (items, item)
			item->retain ();
			addResult (item);
		EndFor
		return kResultOk;
	}

	void CCL_API setPaginationNeeded (tbool state) override
	{
		if(state)
			(NEW Message ("PaginationNeeded"))->post (component);
	}

	CLASS_INTERFACE (ISearchResultSink, Unknown)

private:
	SearchComponent* component;
};

//************************************************************************************************
// SearchComponent::ProgressDelegate
//************************************************************************************************

class SearchComponent::ProgressDelegate: public Unknown,
										 public AbstractProgressNotify
{
public:
	ProgressDelegate (SearchComponent& component)
	: component (component)
	{}

	// IProgressNotify
	tbool CCL_API isCanceled () override { return component.shouldTerminate (); }

	CLASS_INTERFACE (IProgressNotify, Unknown)

protected:
	SearchComponent& component;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum SearchTags
	{
		kSearchTerms = 100,
		kSearchLocation,
		kHasSearchLocation,
		kSearchIcon,
		kClear,
		kCancel,
		kCanSearch,
		kIsSearching,
		kShowResult,
		kVisible,
		kSearchTermsFocused,
		kIsPaginationNeeded,
		kPaginationNext,
		kPaginationPrevious
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (ISearchProvider, 0x570b8a01, 0x7676, 0x45d2, 0x94, 0xa9, 0x80, 0xef, 0xf7, 0x59, 0x92, 0x72)
DEFINE_IID_ (ISearchResultViewer, 0x9214a2ba, 0xa4eb, 0x4365, 0x8e, 0x61, 0x63, 0x67, 0x4b, 0x27, 0xb5, 0x8b)
DEFINE_STRINGID_MEMBER_ (ISearchResultViewer, kCloseViewer, "closeViewer")

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (SearchComponent)
	DEFINE_COMMAND ("Edit",	"Search", SearchComponent::onFocusSearchField)
END_COMMANDS (SearchComponent)

//************************************************************************************************
// SearchComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SearchComponent, Component)
IMPLEMENT_COMMANDS (SearchComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

SearchComponent::SearchComponent ()
: Component (CCLSTR ("Search")),
  UserThread ("Search"),
  resultViewer (nullptr),
  searchProvider (nullptr),
  currentSearch (nullptr),
  state (kIdle),
  numResults (0),
  resultFrequency (500),
  typingTimeOutInitial (800),
  typingTimeOutAgain (300),
  numImmediateResults (50),
  scheduledStart (0),
  currentPaginationOffset (0),
  searchOptions (ISearchDescription::kIgnoreDelimiters)
{
	paramList.addString (CSTR ("searchTerms"), Tag::kSearchTerms);
	paramList.addString (CSTR ("searchLocation"), Tag::kSearchLocation);
	paramList.addImage (CSTR ("searchIcon"), Tag::kSearchIcon);
	paramList.addParam (CSTR ("clear"), Tag::kClear);
	paramList.addParam (CSTR ("cancel"), Tag::kCancel);

	paramList.addParam (CSTR ("canSearch"), Tag::kCanSearch);
	paramList.addParam (CSTR ("hasLocation"), Tag::kHasSearchLocation);
	paramList.addParam (CSTR ("isSearching"), Tag::kIsSearching);
	paramList.addParam (CSTR ("showResult"), Tag::kShowResult);
	paramList.addParam (CSTR ("visible"), Tag::kVisible);
	paramList.addParam (CSTR ("searchTermsFocused"), Tag::kSearchTermsFocused);

	paramList.addParam (CSTR ("isPaginationNeeded"), Tag::kIsPaginationNeeded);	
	paramList.addParam (CSTR ("paginationNext"), Tag::kPaginationNext);	
	paramList.addParam (CSTR ("paginationPrevious"), Tag::kPaginationPrevious);		
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SearchComponent::~SearchComponent ()
{
	safe_release (currentSearch);
	safe_release (searchProvider);
	setResultViewer (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SearchComponent::isVisible () const
{
	return paramList.byTag (Tag::kVisible)->getValue ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchComponent::setVisible (bool state)
{
	paramList.byTag (Tag::kVisible)->setValue (state, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SearchComponent::areSearchTermsFocused () const
{
	return paramList.byTag (Tag::kSearchTermsFocused)->getValue ().asBool ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SearchComponent::isShowingResult () const
{
	return paramList.byTag (Tag::kShowResult)->getValue ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchComponent::setResultViewer (ISearchResultViewer* viewer)
{
	ISubject::removeObserver (resultViewer, this);
	take_shared (resultViewer, viewer);
	ISubject::addObserver (resultViewer, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchComponent::setSearchProvider (ISearchProvider* provider)
{
	bool searchTermAvailable = (getParameterByTag (Tag::kSearchTerms)->getValue () != String::kEmpty);
	
	cancelSearch ();

	take_shared (searchProvider, provider);

	String searchLocation;
	if(provider)
		searchLocation = provider->getTitle ();
	bool canSearch = provider != nullptr;
	paramList.byTag (Tag::kCanSearch)->setValue (canSearch);
	paramList.byTag (Tag::kSearchLocation)->fromString (searchLocation);
	paramList.byTag (Tag::kHasSearchLocation)->setValue (provider && !provider->getStartPoint ().isEmpty (), true);

	IImage* icon = provider ? provider->getSearchIcon () : nullptr;
	UnknownPtr<IImageProvider> (paramList.byTag (Tag::kSearchIcon))->setImage (icon);
	
	if(searchTermAvailable)
		scheduleSearch (getTypingTimeOutAgain ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchComponent::scheduleSearch (int delay)
{
	CCL_PRINTF ("SearchComponent::scheduleSearch (delay %f seconds)\n", delay / 1000.f)
	scheduledStart = System::GetSystemTicks () + delay;
	(NEW Message ("StartSearch"))->post (this, delay);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchComponent::startSearch (StringRef searchTerms)
{
	CCL_PRINTF ("SearchComponent::startSearch \"%s\" (state %d)\n", MutableCString (searchTerms).str (), state)
	if(state == kSearching && currentSearch && searchTerms == currentSearch->getSearchTerms ())
	{
		CCL_PRINTLN ("   ignoring same searchTerms")
		return;
	}

	cancelSearch ();

	if(state == kIdle)
	{
		AutoPtr<ISearchProvider> searchProvider;
		searchProvider.share (this->searchProvider);
		if(!searchProvider)
			searchProvider = NEW MultiSearchProvider;  // support usage without search provider, creates empty MultiSearcher

		if(searchProvider && !searchTerms.isEmpty ())
		{			
			AutoPtr<SearchDescription> description (SearchDescription::create (searchProvider->getStartPoint (), searchTerms, searchOptions, searchDelimiters));
			description->setPaginationOffset (currentPaginationOffset);
			AutoPtr<ISearcher> searcher = searchProvider->createSearcher (*description);
			ASSERT (searcher != nullptr)
			if(searcher == nullptr)
				return;

			safe_release (currentSearch);
			currentSearch = NEW SearchResult (this);
			currentSearch->setSearcher (searcher);
			currentSearch->setSearchTerms (searchTerms);

			if(resultViewer)
				resultViewer->onSearchStart (*description, searchProvider);

			state = kSearching;
			numResults = 0;
			paramList.byTag (Tag::kIsSearching)->setValue (true);

			pendingResultItems.removeAll ();
			pendingSearchTerms.empty ();

			paramList.byTag (Tag::kShowResult)->setValue (true);

			startThread (Threading::kPriorityBelowNormal);
			startTimer (resultFrequency, true);
		}
		else
		{
			clearResult ();
			if(resultViewer)
				resultViewer->onSearchEnd (true);
		}
	}
	else
	{
		// must wait until thread finished
		pendingSearchTerms = searchTerms;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String SearchComponent::getSearchTerms () const
{
	if(currentSearch)
		return currentSearch->getSearchTerms ();
	else
		return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchComponent::cancelSearch ()
{
	if(state == kSearching)
	{
		ASSERT (isThreadStarted ())
		if(isThreadStarted ())
		{
			state = kCanceling;
			requestTerminate ();
		}
		else
			onSearchDone (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchComponent::onSearchDone (bool canceled)
{
	SOFT_ASSERT (!isThreadAlive (), "Search thread still alive")
	stopThread (5000);
	stopTimer ();

	if(!canceled)
		flushPendingResults ();

	paramList.byTag (Tag::kIsSearching)->setValue (false);

	if(resultViewer)
		resultViewer->onSearchEnd (canceled);

	if(canceled)
		clearResult ();

	state = kIdle;

	if(!pendingSearchTerms.isEmpty ())
		startSearch (pendingSearchTerms);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchComponent::clearResult ()
{
	safe_release (currentSearch);

	currentPaginationOffset = 0;
	paramList.byTag (Tag::kIsPaginationNeeded)->setValue (false);

	paramList.byTag (Tag::kShowResult)->setValue (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchComponent::clearSearchTerms ()
{
	cancelSearch ();

	pendingResultItems.removeAll ();
	pendingSearchTerms.empty ();
	scheduledStart = 0;

	paramList.byTag (Tag::kShowResult)->setValue (false);
	paramList.byTag (Tag::kSearchTerms)->setValue (String::kEmpty);
	
	propertyChanged ("showPlaceholderLabel");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchComponent::reset ()
{
	setSearchProvider (nullptr); // also cancels search

	clearSearchTerms ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchComponent::flushPendingResults ()
{
	if(!pendingResultItems.isEmpty ())
	{
		CCL_PRINTF ("SearchComponent: flushPendingResults (%d)\n", numResults)

		if(resultViewer)
			resultViewer->onResultItemsAdded (pendingResultItems);

		pendingResultItems.removeAll ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SearchComponent::threadEntry ()
{
	ISearcher* searcher = currentSearch ? currentSearch->getSearcher () : nullptr;
	ASSERT (searcher != nullptr)
	if(searcher)
	{
		ProgressDelegate progressDelegate (*this);
		searcher->find (*currentSearch, &progressDelegate);
	}

	(NEW Message ("ThreadDone", shouldTerminate ()))->post (this);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SearchComponent::load (const Storage& storage)
{
	// only save visibility
	paramList.restoreValue (storage.getAttributes (), paramList.byTag (Tag::kVisible));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SearchComponent::save (const Storage& storage) const
{
	paramList.storeValue (storage.getAttributes (), paramList.byTag (Tag::kVisible));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API SearchComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "SearchResult")
	{
		if(resultViewer)
			return resultViewer->createView (bounds);
	}
	return SuperClass::createView (name, data, bounds);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchComponent::focusSearchField ()
{
	UnknownPtr<ISubject> searchParam (paramList.byTag (Tag::kSearchTerms));
	if(searchParam)
		searchParam->signal (Message (IParameter::kRequestFocus));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SearchComponent::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kSearchTerms:
		scheduleSearch (isShowingResult () ? getTypingTimeOutAgain () : getTypingTimeOutInitial ());
		
		propertyChanged ("showPlaceholderLabel");
		return true;

	case Tag::kVisible:
		if(param->getValue ().asBool ())
		{
			focusSearchField ();
			break;
		}
		// else through:

	case Tag::kClear:
		clearSearchTerms ();
		scheduleSearch (0);
		focusSearchField ();
		return true;

	case Tag::kCancel:
		cancelSearch ();
		return true;

	case Tag::kPaginationNext :
		currentPaginationOffset++;
		scheduleSearch (0);
		break;

	case Tag::kPaginationPrevious :
		if(currentPaginationOffset > 0)
		{
			currentPaginationOffset--;
			scheduleSearch (0);
	}
		break;
	}
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SearchComponent::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "Result")
	{
		pendingResultItems.add (msg[0], true);

		if(++numResults < numImmediateResults)
			flushPendingResults ();
	}
	else if(msg == "ThreadDone")
	{
		onSearchDone (msg[0].asBool ());
	}
	else if(msg == "StartSearch")
	{
		int toWait = int (scheduledStart - System::GetSystemTicks ());
		if(toWait <= 0)
		{
			scheduledStart = 0;
			startSearch (paramList.byTag (Tag::kSearchTerms)->getValue ());
		}
		else
		{
			CCL_PRINTF ("SearchComponent: reschedule\n")
			(NEW Message ("StartSearch"))->post (this, toWait);
		}
	}
	else if(msg == "PaginationNeeded")
	{
		paramList.byTag (Tag::kIsPaginationNeeded)->setValue (true);
	}
	else if(msg == ISearchResultViewer::kCloseViewer && isEqualUnknown (subject, resultViewer))
	{
		paramList.byTag (Tag::kSearchTerms)->setValue (String::kEmpty, false);
		scheduleSearch (0);
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SearchComponent::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "showPlaceholderLabel")
	{
		var = (getParameterByTag (Tag::kSearchTerms)->getValue () == String::kEmpty);
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SearchComponent::terminate ()
{
	cancelSearch ();
	cancelSignals ();
	
	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SearchComponent::onIdleTimer ()
{
	flushPendingResults ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SearchComponent::onEditNavigation (const KeyEvent& event, IView* view)
{
	// hide search bar when Escape pressed in search terms edit
	if(event.vKey == VKey::kEscape)
	{
		UnknownPtr<IControl> control (view);
		if(control && control->getParameter () && control->getParameter ()->getTag () == Tag::kSearchTerms)
			paramList.byTag (Tag::kVisible)->setValue (false, true);
	}

	// give resultViewer a chance
	UnknownPtr<IEditControlHost> editControlHost (resultViewer);
	if(editControlHost)
		return editControlHost->onEditNavigation (event, view);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SearchComponent::paramPreview (IParameter* param, ParamPreviewEvent& e)
{
	if(param && param->getTag () == Tag::kSearchTerms)
	{
		// track "focus" state of searchTerms 
		bool isFocused = e.type == ParamPreviewEvent::kFocus;
		if(isFocused || e.type == ParamPreviewEvent::kUnfocus)
			paramList.byTag (Tag::kSearchTermsFocused)->setValue (isFocused);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SearchComponent::onFocusSearchField (CmdArgs args)
{
	if(!args.checkOnly ())
	{
		paramList.byTag (Tag::kVisible)->setValue (true, true);
		focusSearchField ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (SearchComponent)
	DEFINE_METHOD_NAME ("focusSearchField")
END_METHOD_NAMES (SearchComponent)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SearchComponent::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "focusSearchField")
	{
		focusSearchField ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
