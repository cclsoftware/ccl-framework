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
// Filename    : ccl/app/browser/searchresultlist.h
// Description : Search Result List
//
//************************************************************************************************

#ifndef _ccl_searchresultlist_h
#define _ccl_searchresultlist_h

#include "ccl/app/browser/filesystemnodes.h"
#include "ccl/app/controls/listviewmodel.h"
#include "ccl/app/components/isearchprovider.h"

#include "ccl/base/collections/stringlist.h"

#include "ccl/public/system/ifileutilities.h"

namespace CCL {

namespace Browsable {
class SearchResultNode;
class ResultCategoryNode; }

//************************************************************************************************
// SearchResultList
//************************************************************************************************

class SearchResultList: public ListViewModel,
						public ISearchResultViewer,
						public IEditControlHost
{
public:
	DECLARE_CLASS_ABSTRACT (SearchResultList, ListViewModel)

	SearchResultList ();

	enum Columns
	{
		kIcon,
		kTitle,
	};

	void setListStyle (StyleRef style);
	StyleRef getListStyle () const;

	void setShowCategories (bool state);
	bool isShowingCategories () const;

	BrowserNode* findResultNode (UrlRef path) const;
	bool showSelectedResultInContext ();

	// ListViewModel
	tbool CCL_API canSelectItem (ItemIndexRef index) override;
	StringID CCL_API getItemBackground (ItemIndexRef index) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API openItem (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection) override;
	tbool CCL_API interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection) override;
	tbool CCL_API onItemFocused (ItemIndexRef index) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	/// to be implemented by derived class:
	virtual bool showResultInContext (UrlRef url, bool checkOnly);
	virtual bool appendResultContextMenu (IContextMenu& menu, UrlRef url);
	virtual tbool interpretResultCommand (const CommandMsg& msg, UrlRef url);
	virtual bool onSearchResultFocused (UrlRef url, IImage* icon, StringRef title);

	CLASS_INTERFACE2 (ISearchResultViewer, IEditControlHost, ListViewModel)

protected:
	SharedPtr<ISearchProvider> searchProvider;
	String searchTerms;
	ObjectArray categoryNodes;
	StringList collapsedCategories;
	UnknownPtr<IFileTypeClassifier> fileTypeClassifier;
	StyleFlags listStyle;
	Coord expandSize;
	bool showCategories;

	// ISearchResultViewer
	bool isViewVisible () override;
	IView* createView (const Rect& bounds) override;
	void onSearchStart (ISearchDescription& description, ISearchProvider* provider) override;
	void onSearchEnd (bool canceled) override;
	void onResultItemsAdded (const IUnknownList& items) override;

	// IEditControlHost
	tbool CCL_API onEditNavigation (const KeyEvent& event, IView* control) override;
	void CCL_API onEditControlLostFocus (IView* control) override {}

	BrowserNode* findResultNodeInternal (UrlRef path, const ObjectArray& items) const;
	bool isShowingResultList () const;
	void selectNextResult ();
	void expandCategory (Browsable::ResultCategoryNode& node, bool state);
	tbool onShowResultInContext (ItemIndexRef item, bool checkOnly);
	
	virtual Browsable::SearchResultNode* createSearchResultNode (CCL::IUrl* url);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline StyleRef SearchResultList::getListStyle () const
{ return listStyle; }

inline bool SearchResultList::isShowingCategories () const
{ return showCategories; }

namespace Browsable {

//************************************************************************************************
// SearchResultNode
//************************************************************************************************

class SearchResultNode: public FileNode
{
public:
	DECLARE_CLASS (SearchResultNode, FileNode)

	SearchResultNode (Url* path = nullptr);

	PROPERTY_SHARED_AUTO (IUnknown, dragObject, DragObject)

	StringRef getCategory () const;
	String& getCategory ();
	void setCategory (StringRef string);

	StringRef getSortString () const;
	String& getSortString ();

	// FileNode
	IUnknown* createDragObject () override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu, Container* selectedNodes) override;
	int compare (const Object& obj) const override;

private:
	String category;
	String sortString;
};

} // namespace Browsable

} // namespace CCL

#endif // _ccl_searchresultlist_h
