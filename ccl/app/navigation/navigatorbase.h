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
// Filename    : ccl/app/navigation/navigatorbase.h
// Description : Navigator base class
//
//************************************************************************************************

#ifndef _ccl_navigatorbase_h
#define _ccl_navigatorbase_h

#include "ccl/app/component.h"

#include "ccl/public/gui/inavigator.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/framework/iview.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/collections/objectstack.h"

namespace CCL {

class NavigationHistory;
class NavigationHistoryEntry;

//************************************************************************************************
// NavigatorBase
//************************************************************************************************

class NavigatorBase: public Component,
					 public INavigator,
					 public CommandDispatcher<NavigatorBase>
{
public:
	DECLARE_CLASS_ABSTRACT (NavigatorBase, Component)

	NavigatorBase (StringRef name = nullptr, StringRef title = nullptr);

	void setHomeUrl (UrlRef url);
	void setCurrentUrl (UrlRef url);

	virtual bool isOpen () const;

	// INavigator
	tresult CCL_API navigateDeferred (UrlRef url) override;
	UrlRef CCL_API getCurrentUrl () const override;
	StringRef CCL_API getCurrentTitle () const override;
	tresult CCL_API goHome () override;
	UrlRef CCL_API getHomeUrl () const override;

	// Command Methods
	DECLARE_COMMANDS (NavigatorBase)
	DECLARE_COMMAND_CATEGORY ("Navigation", Component)
	virtual bool onBackCmd (CmdArgs);
	virtual bool onForwardCmd (CmdArgs);
	virtual bool onHomeCmd (CmdArgs);
	virtual bool onRefreshCmd (CmdArgs);

	CLASS_INTERFACE (INavigator, Component)

protected:
	Url currentUrl; 
	Url homeUrl;
	String currentTitle;
	ViewPtr contentFrame;
};

//************************************************************************************************
// NavigatorBase2
//************************************************************************************************

class NavigatorBase2: public NavigatorBase,
					  public INavigator2
{
public:
	DECLARE_CLASS_ABSTRACT (NavigatorBase2, NavigatorBase)

	NavigatorBase2 (StringRef name = nullptr, StringRef title = nullptr);
	~NavigatorBase2 ();

	PROPERTY_FLAG (navigatorFlags, 1<<0, historyDisabled)

	// INavigator
	tresult CCL_API navigate (UrlRef url) override;
	tresult CCL_API goBack () override;
	tresult CCL_API goForward () override;
	tbool CCL_API canGoBack () const override;
	tbool CCL_API canGoForward () const override;

	// INavigator2
	INavigationHistory& CCL_API getBackwardHistory () const override;
	INavigationHistory& CCL_API getForwardHistory () const override;

	CLASS_INTERFACE (INavigator2, NavigatorBase)

protected:
	int navigatorFlags;
	mutable NavigationHistory* backwardHistory;
	mutable NavigationHistory* forwardHistory;

	void pushCurrent (NavigationHistory* history);

	// Navigation hook
	virtual void onNavigated ();
};

//************************************************************************************************
// NavigationHistory
//************************************************************************************************

class NavigationHistory: public Object,
						 public INavigationHistory
{
public:
	DECLARE_CLASS (NavigationHistory, Object)

	NavigationHistory ();

	void push (NavigationHistoryEntry* entry, bool check = true);
	NavigationHistoryEntry* peek () const;
	NavigationHistoryEntry* pop ();
	NavigationHistoryEntry* at (int index) const;
	void checkDuplicates ();
		
	void dump (CStringPtr name = nullptr);

	// INavigationHistory
	int CCL_API countEntries () const override;
	const INavigationHistoryEntry* CCL_API getEntry (int index) const override;
	const INavigationHistoryEntry* CCL_API peekEntry () const override;

	CLASS_INTERFACE (INavigationHistory, Object)

protected:
	ObjectStack entries;
};

//************************************************************************************************
// NavigationHistoryEntry
//************************************************************************************************

class NavigationHistoryEntry: public Object,
							  public INavigationHistoryEntry
{
public:
	DECLARE_CLASS (NavigationHistoryEntry, Object)

	NavigationHistoryEntry (UrlRef url = Url::kEmpty, StringRef title = nullptr);

	// INavigationHistoryEntry
	UrlRef CCL_API getUrl () const override;
	StringRef CCL_API getTitle () const override;

	CLASS_INTERFACE (INavigationHistoryEntry, Object)

protected:
	Url url;
	String title;
};

} // namespace CCL

#endif // _ccl_navigatorbase_h
