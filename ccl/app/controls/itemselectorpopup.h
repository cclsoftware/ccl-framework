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
// Filename    : ccl/app/controls/itemselectorpopup.h
// Description : Item Selector Popup
//
//************************************************************************************************

#ifndef _ccl_itemselectorpopup_h
#define _ccl_itemselectorpopup_h

#include "ccl/app/component.h"

#include "ccl/base/collections/stringlist.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/public/gui/framework/ipopupselector.h"
#include "ccl/public/gui/framework/popupselectorclient.h"
#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/gui/framework/guievent.h"

namespace CCL {

interface IRegularExpression;

//************************************************************************************************
// IItemsProvider
//************************************************************************************************

interface IItemsProvider: IUnknown
{
	virtual IUnknownIterator* newUnknownIterator () const = 0;
	
	virtual KeyEvent getUnknownShortcut (IUnknown* item) const = 0;
	
	virtual String getUnknownTitle (IUnknown* item) const = 0;

	DECLARE_IID (IItemsProvider)
};

//************************************************************************************************
// ObjectItemsProvider
//************************************************************************************************

class ObjectItemsProvider: public Object,
						   public IItemsProvider
							 
{
public:
	virtual Iterator* newItemsIterator () const { return nullptr; }
	virtual KeyEvent getItemShortcut (Object* item) const { return KeyEvent (); }
	virtual String getItemTitle (Object* item) const { return String::kEmpty; }

	// IItemsProvider
	IUnknownIterator* newUnknownIterator () const override { return newItemsIterator (); }
	KeyEvent getUnknownShortcut (IUnknown* item) const override { return getItemShortcut (unknown_cast<Object> (item)); }
	String getUnknownTitle (IUnknown* item) const override { return getItemTitle (unknown_cast<Object> (item)); }

	CLASS_INTERFACE (IItemsProvider, Object)
};

//************************************************************************************************
// ItemSelectorPopup
/** Opens a popup where the user can select one of the given items by typing its number (1-based index)
	or name (Object::toString). */
//************************************************************************************************

class ItemSelectorPopup: public Component,
						 public PopupSelectorClient,
						 public IdleClient
{
public:
	ItemSelectorPopup (IItemsProvider* itemsProvider, String startString = String::kEmpty);
	~ItemSelectorPopup ();

	PROPERTY_MUTABLE_CSTRING (formName, FormName)
	PROPERTY_MUTABLE_CSTRING (persistenceID, PersistenceID)
	PROPERTY_BOOL (wantsEnter, WantsEnter)
	PROPERTY_BOOL (matchResultNumber, MatchResultNumber)
	PROPERTY_STRING (itemTitleSeparator, ItemTitleSeparator)

	/** Run popup using the given form. The selected item is returned as result value of the IAsyncOperation when it's completed. */
	IAsyncOperation* run ();

	CLASS_INTERFACE (IPopupSelectorClient, Component)

private:
	AutoPtr<IPopupSelector> popupSelector;
	AutoPtr<AsyncOperation> asyncOperation;
	IParameter* typedStringParam;
	IParameter* selectedItemParam;
	SharedPtr<Container> availableItems;
	SharedPtr<IItemsProvider> itemsProvider;
	Vector<IUnknown*> candidates;
	IUnknown* selectedItem;
	IRegularExpression* regExp;
	String typedString;
	String startString;
	StringList recentChoices;
	bool wasTimeOutSelection;

	enum { kTimeOutMs = 1000, kMaxRecentChoices = 30 };

	Attributes* getSettings () const;
	void storeSettings () const;
	bool restoreSettings ();
	void setRecentItem (IUnknown& item);

	void prepareSearch ();
	bool matchesItemTitle (IUnknown& item, bool matchStart = true) const;
	IUnknown* findShortcutItem (const KeyEvent& shortcut);
	IUnknown* findCandidate (StringRef title) const;
	void collectCandidates ();
	void selectCandidate (IUnknown* item);
	void setTypedStringTemporary (bool temporary);
	Result checkTypedString (bool acceptFirstMatch = true);

	// PopupSelectorClient
	IView* CCL_API createPopupView (SizeLimit& limits) override;
	bool hasPopupResult () override;
	Result CCL_API onKeyDown (const KeyEvent& event) override;
	void CCL_API onPopupClosed (Result result) override;

	// IdleClient
	void onIdleTimer () override;
};

} // namespace CCL

#endif // _ccl_itemselectorpopup_h
