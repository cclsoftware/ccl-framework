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
// Filename    : ccl/app/browser/browser.h
// Description : Browser Component
//
//************************************************************************************************

#ifndef _ccl_browser_h
#define _ccl_browser_h

#include "ccl/app/component.h"

#include "ccl/app/browser/browsernode.h"
#include "ccl/app/browser/searchresultlist.h"

#include "ccl/base/collections/objectlist.h"

#include "ccl/public/base/irecognizer.h"
#include "ccl/public/app/inavigationserver.h"
#include "ccl/public/gui/idatatarget.h"
#include "ccl/public/gui/iviewstate.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/framework/styleflags.h"
#include "ccl/public/gui/framework/iitemmodel.h"

namespace CCL {

class BrowserTreeModel;
class BrowserListModel;
class BrowserState;
class BrowserModelBase;
class BrowserExtender;
class SearchComponent;
class BreadcrumbsComponent;

interface IWindow;
interface IBrowserExtension;
interface ISearchProvider;

//************************************************************************************************
// Browser
//************************************************************************************************

class Browser: public Component,
			   public INavigationServer,
			   public CommandDispatcher<Browser>
{
public:
	DECLARE_CLASS (Browser, Component)

	Browser (StringRef name = nullptr, StringRef title = nullptr);
	~Browser ();

	enum BrowserStyles
	{
		kDisplayTreeLeafs			= 1<<0,	///< display leafs in tree view
		kShowListView				= 1<<1,	///< show the list view (in additon to the tree view)
		kPersistentStates			= 1<<2,	///< save/load browser state(s) in settings
		kCanSetRoot					= 1<<3,	///< user can change the root node (set as root / up)
		kCanAddTabs					= 1<<4,	///< user can add/remove tabs
		kCanRefresh					= 1<<5,	///< user can refresh a node
		kHideColumnHeaders			= 1<<6,	///< don't show column header view when columns are used
		kHasListMode				= 1<<7,	///< browser can switch to alternative list-only mode
		kResultListHideCategories 	= 1<<8	///< search result list categories can be hidden
	};

	PROPERTY_FLAG (browserStyle, kDisplayTreeLeafs, displayTreeLeafs)
	PROPERTY_FLAG (browserStyle, kShowListView, showListView)
	PROPERTY_FLAG (browserStyle, kPersistentStates, persistentStates)
	PROPERTY_FLAG (browserStyle, kCanSetRoot, canSetRoot)
	PROPERTY_FLAG (browserStyle, kCanAddTabs, canAddTabs)
	PROPERTY_FLAG (browserStyle, kCanRefresh, canRefresh)
	PROPERTY_FLAG (browserStyle, kHideColumnHeaders, hideColumnHeaders)
	PROPERTY_FLAG (browserStyle, kHasListMode, hasListMode)
	PROPERTY_FLAG (browserStyle, kResultListHideCategories, resultListHideCategories)

	PROPERTY_OBJECT (StyleFlags, scrollStyle, ScrollStyle)
	PROPERTY_OBJECT (StyleFlags, treeStyle, TreeStyle)
	PROPERTY_OBJECT (StyleFlags, listStyle, ListStyle)
	
	PROPERTY_MUTABLE_CSTRING (formName, FormName);
	PROPERTY_BOOL (trackingEnabled, TrackingEnabled)

	/// the root of the whole node hierarchy
	virtual RootNode* getRootNode ();

	/// the topmost node that can become tree root; must be a child of root, must not be removed!
	void setTopMostNode (BrowserNode* node);
	BrowserNode* getTopMostNode ();

	/// the current root in the tree
	BrowserNode* getTreeRoot ();
	bool setTreeRoot (BrowserNode* node, bool preserveExpandState, bool updateStateName);

	void addSearch ();
	void addBreadcrumbs ();
	virtual void addBrowserNode (BrowserNode* node);
	void clearNodes ();
	void addBrowserState (BrowserNode& node);
	void setDefaultColumns (IColumnHeaderList* columns);
	void updateColumns ();
	void setNodeFilter (IObjectFilter* filter);
	void setSearchProvider (ISearchProvider* provider); ///< takes ownership
	ISearchProvider* getSearchProvider () const;
	void setSearchIcon (IImage* icon);

	BrowserNode* getFocusNode (bool includeSearchResults = false) const;
	bool setFocusNode (BrowserNode* node, bool selectExclusive = true);
	bool setTreeFocusNode (BrowserNode* node, bool selectExclusive = true);
	bool setListFocusNode (BrowserNode* node, bool selectExclusive = true);
	BrowserNode* getListParentNode () const;

	void selectAll (bool state);
	void selectNode (BrowserNode* node, bool state = true);
	bool isAnyNodeSelected () const;

	virtual void onNodeRemoved (BrowserNode* node);
	
	void getEditNodes (Container& editNodes, BrowserNode* focusNode); ///< adds all selected nodes if focusNode is selected, otherwise only focusNode
	template<class NodeClass, class Lambda>	static void visitEditNodes (BrowserNode* focusNode, const Container* nodes, const Lambda& visit);

	void expandAll (bool state = true, bool deferred = false);
	void expandNode (BrowserNode* node, bool state = true);
	bool isNodeExpanded (BrowserNode& node) const;
	bool wasExpanded (BrowserNode* node) const;
	bool isNodeVisible (BrowserNode& node) const;

	virtual bool openNode (BrowserNode* node); ///< user has double-clicked or pressed [Return]
	bool insertNode (BrowserNode* parent, BrowserNode* node, int index = -1); ///< browser owns the node
	bool removeNode (BrowserNode* node); ///< node gets released
	void redrawNode (BrowserNode* node);
	void refreshNode (BrowserNode* node, bool keepExpandState = false);
	void refreshAll (bool deferred);
	void updateThumbnail (BrowserNode* node);

	void setActivityIndicatorState (bool state); ///< can be used by nodes to indicate background activity

	Object* createSnapshot () const;
	bool restoreSnapshot (Object* snapshot);

	bool isRestoringState () const;
	bool& getRestoringState ();

	void resetScrollState ();
	void trackInteraction () const;

	// find a node using a Recognizer (iterates existing nodes)
	IUnknown* findNode (const IRecognizer* recognizer, const BrowserNode* startNode = nullptr) const;
	template<class T> T* findNode (const IRecognizer* recognizer, const BrowserNode* startNode = nullptr) const;

	/// find a node by it's path; acceptAncestor: return deepest ancestor if node not found
	BrowserNode* findNode (StringID path, bool create, bool acceptAncestor = false);
	BrowserNode* findNodeWithBreadcrumbsPath (StringID path);
	BrowserNode* findNodeInSearchResults (UrlRef path) const;

	bool makePath (MutableCString& path, BrowserNode* node, BrowserNode* startNode = nullptr) const;
	MutableCString makePath (BrowserNode* node, BrowserNode* startNode = nullptr) const;
	bool makeDisplayPath (String& displayPath, BrowserNode* node, BrowserNode* startNode = nullptr) const;

	/// find a node with given url
	virtual BrowserNode* findNodeWithUrl (UrlRef url);

	/// find a node on screen
	BrowserNode* findNode (IView* view, const Point& where);

	/// ensure that child nodes have been created
	void createChildNodes (BrowserNode& node);

	/// iterate existing child nodes of given node
	IUnknownIterator* iterateChildNodes (BrowserNode& node) const;

	/// navigate to a previous / next node
	BrowserNode* navigate (BrowserNode& startNode, int increment, IObjectFilter* filter = nullptr);

	BrowserNode* resolveNode (IItemView& itemView, ItemIndexRef index) const;

	IItemView* getTreeView () const;
	IItemView* getListView () const;
	IWindow* getWindow ();
	bool isVisible () const;
	bool isListMode () const;
	void setDefaultListMode (bool state = true);

	Styles::ListViewType getListViewType () const;
	void setListViewType (Styles::ListViewType viewType); ///< default: icons

	bool isDragSource (IDragSession& session); ///< check if browser started the drag operation

	/// Messages
	DECLARE_STRINGID_MEMBER (kNodeFocused)
	DECLARE_STRINGID_MEMBER (kNodeRemoved)

	DECLARE_STRINGID_MEMBER (kTreeRootContext) ///< context id, when context menu called for tree root node
	DECLARE_STRINGID_MEMBER (kChildrenHiddenContext) ///< context id, when children are hidden and child operations are suppressed

	// Extensions
	BrowserExtender& getExtender ();
	void addExtension (IBrowserExtension* extension);
	void addExtensionPlugIns (StringRef category);

	class ExpandState;

	// INavigationServer
	tresult CCL_API navigateTo (NavigateArgs& args) override;

	// Component
	tresult CCL_API initialize (IUnknown* context) override;
	tresult CCL_API terminate () override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	// Command Methods
	bool onAscendRoot (CmdArgs args);
	bool onResetRoot (CmdArgs args);
	bool onNewTab (CmdArgs args);
	bool onNewRootTab (CmdArgs args);
	bool onCloseTab (CmdArgs args);
	bool onRenameTab (CmdArgs args);
	bool onRefresh (CmdArgs args);
	bool onInsertSelectedItem (CmdArgs args);
	bool onNavigationNext (CmdArgs args);
	bool onNavigationEnter (CmdArgs args);
	bool onNavigationBack (CmdArgs args);
	DECLARE_COMMANDS (Browser)
	DECLARE_COMMAND_CATEGORY ("Browser", SuperClass)

	CLASS_INTERFACE (INavigationServer, Component)

private:
	DECLARE_STRINGID_MEMBER (kExpandAll)
	DECLARE_STRINGID_MEMBER (kRefreshAll)
	DECLARE_STRINGID_MEMBER (kRestoreState)

protected:
	int browserStyle;
	RootNode* rootNode;			///< absolute root of the node hierarchy
	BrowserNode* topMostNode;	///< optional, the topmost node that can become treeRootNode
	BrowserNode* treeRootNode;	///< current root node in tree
	ObjectList rootChain;		///< holds invisible nodes between rootNode (incl.) and treeRootNode (incl.)
	ITree* tree;				///< tree object for TreeView
	BrowserTreeModel* treeModel;
	BrowserListModel* listModel;
	IColumnHeaderList* defaultColumns;
	AutoPtr<IObjectFilter> nodeFilter;
	ObjectArray browserStates;
	BrowserState* currentState;
	bool settingsLoaded;
	bool restoringState;
	bool isRefreshing;
	bool showingSearchResult;
	bool listMode;
	BrowserExtender* extender;
	SearchComponent* search;
	ISearchProvider* searchProvider;
	BreadcrumbsComponent* breadcrumbs;
	
	class SearchResultList;
	class SettingsSaver;
	friend class SettingsSaver;
	SettingsSaver* saver;

protected:
	friend class BrowserModelBase;
	friend class BrowserTreeModel;
	friend class BrowserListModel;

	virtual void onInitNodes () {} ///< called when root node is created
	virtual void onStatesRestored (); ///< called after BrowserStates are restored (can add BrowserStates here)
	virtual void onViewAttached (IItemView* itemView) {} /// called when tree or list view is attached to the ItemModel
	virtual void onViewShown (IItemView* itemView) {} /// called when tree or list view is shown (View::attached)
	virtual void onViewModeChanged () {} /// called when toggled between list & tree view or when list view type changed
	virtual void onNodeFocused (BrowserNode* node, bool inList); ///< called when a node is selected in tree or list
	virtual bool onEditNode (BrowserNode& node, StringID columnID, const IItemModel::EditInfo& info); ///< called when user edits (e.g. clicks) a node
	virtual bool canSelectNode (BrowserNode* node) const;
	virtual bool drawIconOverlay (BrowserNode& node, const IItemModel::DrawInfo& info);
	virtual bool appendNodeContextMenu (BrowserNode& node, IContextMenu& contextMenu, Container* selectedNodes);
	virtual bool interpretNodeCommand (const CommandMsg& msg, ItemModel& model, BrowserNode* targetNode, const IItemSelection& selection);
	virtual tbool canInsertData (BrowserNode* node, const IUnknownList& data, IDragSession* session, IView* targetView);
	virtual tbool insertData (BrowserNode* node, const IUnknownList& data, IDragSession* session);
	virtual bool prepareRefresh (); ///< return true if the whole tree shoukd be rebrowsed; base class calls onRefresh of focus node
	virtual ISearchProvider* getSearchProvider (BrowserNode* focusNode);

	bool isActive () const;
	bool isSearchResultsVisible () const;
	bool isWindowBaseActive (IItemView* itemView) const;
	
	bool canInterpretInSearchMode (const CommandMsg& msg) const;
	const Url* getFocusSearchResult () const;
	bool showSelectedSearchResultInContext ();

	bool canExpandNode (BrowserNode& node);

	void updateBreadcrumbs (BrowserNode* node, bool force = false);
	void updateSearchResultStyle ();

	virtual bool store (Attributes& attributes) const;
	virtual bool restore (Attributes& attributes);

	Attributes& getSettings () const;
	void saveSettings () const;
	void loadSettings ();

	IView* createTreeView (const Rect& bounds);
	IView* createListView (const Rect& bounds);
	IItemView* getMainItemView () const;
	ITreeItem* getRootItem () const;
	ITreeItem* insertNodeIntoTree (BrowserNode* parent, BrowserNode* node, int index, ITreeItem* rootItem);
	ITreeItem* findTreeItem (BrowserNode& node, bool create = true);
	ListViewModelBase* getTreeModel () const;

	// BrowserStates
	void addBrowserState (BrowserState* state);
	void removeBrowserState (BrowserState* state);
	void resetBrowserStates ();
	void reorderBrowserState (int index, int newIndex);
	bool selectBrowserState (BrowserState* state);
	bool selectBrowserState (int index);
	BrowserState* getBrowserState (int index) const;
	void storeCurrentState (IItemView* itemView = nullptr);
	virtual void restoreCurrentState ();
	void renameCurrentState (StringRef name);
	void storeState (BrowserState& state, IItemView* itemView = nullptr) const;
	void restoreState (BrowserState& state);
};

//************************************************************************************************
// Browser::ExpandState
//************************************************************************************************

class Browser::ExpandState
{
public:
	bool store (Browser& browser, BrowserNode& node);
	bool restore (Browser& browser, BrowserNode& node);

private:
	AutoPtr<IViewStateHandler> treeState;
	String nodeTitle;
};

//************************************************************************************************
// Browser::SearchResultList
//************************************************************************************************

class Browser::SearchResultList: public CCL::SearchResultList
{
public:
	SearchResultList (Browser& browser);

	// SearchResultList
	bool showResultInContext (UrlRef url, bool checkOnly) override;
	bool appendResultContextMenu (IContextMenu& menu, UrlRef url) override;
	tbool interpretResultCommand (const CommandMsg& msg, UrlRef url) override;

	// ListViewModel
	tbool CCL_API onItemFocused (ItemIndexRef index) override;
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API openItem (ItemIndexRef index, int column, const EditInfo& info) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	Browser& browser;

	typedef CCL::SearchResultList SuperClass;
};

//************************************************************************************************
// NodeRemover
//************************************************************************************************

class NodeRemover: public Object,
				   public ICommandHandler
{
public:
	NodeRemover (Browser& browser, const ObjectList& nodes);

	PROPERTY_BOOL (checkOnly, CheckOnly)
	PROPERTY_BOOL (removeDeferred, RemoveDeferred)
	PROPERTY_MUTABLE_CSTRING (contextID, ContextID)

	Iterator* newIterator () const;			///< iterate remaining candidate nodes

	void removeNode (BrowserNode* node);	///< allow removing this node
	void keepNode (BrowserNode* node);		///< deny removing this node

	CLASS_INTERFACE (ICommandHandler, Object)

	bool perform (bool checkOnly);

protected:
	friend class Browser;
	NodeRemover (BrowserModelBase& browserModel, const IItemSelection& selection);

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

private:
	Browser& browser;
	ObjectList candidates;
	ObjectList remaining;
	ObjectList removed;

	~NodeRemover () {cancelSignals ();} // can't live on the stack
	void removeNodes ();
};

//************************************************************************************************
// NewTabTarget
/** Data target base class for adding a new tab via dragging. */
//************************************************************************************************

class NewTabTarget: public Component,
					public IDataTarget
{
public:
	// IDataTarget
	tbool CCL_API canInsertData (const IUnknownList& data, IDragSession* session = nullptr, IView* targetView = nullptr, int insertIndex = -1) override;
	tbool CCL_API insertData (const IUnknownList& data, IDragSession* session = nullptr, int insertIndex = -1) override;

	CLASS_INTERFACE (IDataTarget, Component)

protected:
	Browser* browser;

	NewTabTarget (Browser* browser);
	virtual bool canCreateTab (Browser& browser, const IUnknownList& data)				{ return false; }
	virtual BrowserNode* findNewTabRoot (Browser& browser, const IUnknownList& data)	{ return nullptr; }
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// NodeRemover inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Iterator* NodeRemover::newIterator () const
{ return remaining.newIterator (); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Browser inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
inline T* Browser::findNode (const IRecognizer* recognizer, const BrowserNode* startNode) const
{
	return unknown_cast<T> (findNode (recognizer, startNode));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline BrowserState* Browser::getBrowserState (int index) const
{
	return (BrowserState*)browserStates.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class NodeClass, class Lambda>
void Browser::visitEditNodes (BrowserNode* focusNode, const Container* nodes, const Lambda& visit)
{
	ObjectList editNodes;
	if(!nodes && focusNode)
		if(Browser* browser = focusNode->getBrowser ())
		{
			browser->getEditNodes (editNodes, focusNode);
			nodes = &editNodes;
		}

	if(nodes)
		for(auto n : *nodes)
			if(auto node = ccl_cast<NodeClass> (n))
				visit (*node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline ITreeItem* Browser::getRootItem () const
{ return tree ? tree->getRootItem () : nullptr; }

inline bool Browser::isRestoringState () const
{ return restoringState; }

inline bool& Browser::getRestoringState ()
{ return restoringState; }

inline BrowserExtender& Browser::getExtender ()
{ ASSERT (extender != nullptr) return *extender; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_browser_h
