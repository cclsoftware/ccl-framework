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
// Filename    : ccl/app/components/searchcomponent.h
// Description : Search Component
//
//************************************************************************************************

#ifndef _ccl_searchcomponent_h
#define _ccl_searchcomponent_h

#include "ccl/app/component.h"

#include "ccl/public/system/userthread.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/commanddispatch.h"

namespace CCL {

interface ISearchProvider;
interface ISearchResultViewer;

class SearchResult;

//************************************************************************************************
// SearchComponent
//************************************************************************************************

class SearchComponent: public Component,
					   public CommandDispatcher<SearchComponent>,
					   private Threading::UserThread,
					   private IEditControlHost,
					   private IParamPreviewHandler,
					   private IdleClient
{
public:
	DECLARE_CLASS (SearchComponent, Component)
	DECLARE_METHOD_NAMES (SearchComponent)
	
	SearchComponent ();
	~SearchComponent ();

	ISearchResultViewer* getResultViewer ();
	void setResultViewer (ISearchResultViewer* resultViewer);
	void setSearchProvider (ISearchProvider* searchProvider);

	void reset ();
	void clearSearchTerms ();

	void startSearch (StringRef searchTerms);
	String getSearchTerms () const;
	void cancelSearch ();
	bool areSearchTermsFocused () const;	
	bool isShowingResult () const;

	bool isVisible () const;
	void setVisible (bool state);

	PROPERTY_VARIABLE (int, typingTimeOutInitial, TypingTimeOutInitial)			///< ms after typing search terms, before the initial search starts
	PROPERTY_VARIABLE (int, typingTimeOutAgain, TypingTimeOutAgain)				///< ms after typing search terms, before another search starts (result already shown)
	PROPERTY_VARIABLE (int, resultFrequency, ResultFrequency)					///< ms between feeding searchProvider
	PROPERTY_VARIABLE (int, numImmediateResults, NumImmediateResults)			///< number of results fed immediately
	PROPERTY_VARIABLE (int, searchOptions, SearchOptions)						///< ISearchDescription::Options used for search
	PROPERTY_STRING (searchDelimiters, SearchDelimiters)						///< Delimiter character(s) used when searching

	// Component
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	CCL::tbool CCL_API getProperty (CCL::Variant& var, MemberID propertyId) const override;
	tresult CCL_API terminate () override;
	
	// Command Methods
	DECLARE_COMMANDS (SearchComponent)
	DECLARE_COMMAND_CATEGORY ("Edit", SuperClass)
	bool onFocusSearchField (CmdArgs args);

	CLASS_INTERFACE3 (ITimerTask, IEditControlHost, IParamPreviewHandler, Component)
			
private:
	class ProgressDelegate;

	ISearchResultViewer* resultViewer;
	ISearchProvider* searchProvider;
	SearchResult* currentSearch;
	UnknownList pendingResultItems;
	String pendingSearchTerms;	///< next search to be started after thread is done
	int64 scheduledStart;		///< time when search should be started
	int currentPaginationOffset;
	int numResults;
	int state;

	enum State { kIdle, kSearching, kCanceling };

	void scheduleSearch (int delay);
	void onSearchDone (bool canceled);
	void clearResult ();
	void flushPendingResults ();
	void focusSearchField ();

	// Component
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	
	// UserThread
	int threadEntry () override;

	// IdleClient
	void onIdleTimer () override;

	// IEditControlHost
	tbool CCL_API onEditNavigation (const KeyEvent& event, IView* control) override;
	void CCL_API onEditControlLostFocus (IView* control) override {}

	// IParamPreviewHandler
	void CCL_API paramPreview (IParameter* param, ParamPreviewEvent& e) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline ISearchResultViewer* SearchComponent::getResultViewer ()
{ return resultViewer; }

} // namespace CCL

#endif // _ccl_searchcomponent_h
