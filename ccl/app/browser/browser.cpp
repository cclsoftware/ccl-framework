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
// Filename    : ccl/app/browser/browser.cpp
// Description : Browser Component
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/app/browser/browser.h"
#include "ccl/app/browser/browsernode.h"
#include "ccl/app/browser/browserextender.h"
#include "ccl/app/browser/filesystemnodes.h"

#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/controls/draghandler.h"
#include "ccl/app/controls/listviewmodel.h"
#include "ccl/app/components/searchcomponent.h"
#include "ccl/app/components/searchprovider.h"
#include "ccl/app/components/breadcrumbscomponent.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/storage/storage.h"

#include "ccl/public/app/signals.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/system/cclanalytics.h"
#include "ccl/public/plugservices.h"

#include "ccl/public/gui/framework/iitemmodel.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/skinxmldefs.h"
#include "ccl/public/gui/framework/idragndrop.h"
#include "ccl/public/gui/framework/iscrollview.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/controlsignals.h"
#include "ccl/public/gui/framework/usercontrolbase.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/graphics/graphicsfactory.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/idatatarget.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/appanalytics.h"
#include "ccl/public/guiservices.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Browser")
	XSTRING (Refresh, "Refresh")
	XSTRING (Up, "Up")
	XSTRING (ResetRoot, "Reset Root")
	XSTRING (SetAsRoot, "Set as Root")
	XSTRING (NewTab, "New Tab")
	XSTRING (NewTabFromHere, "New Tab From Here")
	XSTRING (NewRootTab, "New Root Tab")
	XSTRING (CloseTab, "Close Tab")
	XSTRING (RenameTab, "Rename Tab")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CCL {

//************************************************************************************************
// BrowserModelBase
//************************************************************************************************

class BrowserModelBase: public ListViewModelBase
{
public:
	DECLARE_CLASS_ABSTRACT (BrowserModelBase, ListViewModelBase)

	BrowserModelBase (Browser& browser);

	virtual BrowserNode* resolveNode (ItemIndexRef index) const = 0;

	Browser& getBrowser () { return browser; }
	void getSelectedNodes (Container& selectedNodes, const IItemSelection* selection, BrowserNode* alternativeNode);
	tbool appendNodeContextMenu (IContextMenu& menu, BrowserNode* contextNode, const IItemSelection* selection);

	void setColumns (IColumnHeaderList* columns) { this->columns.share (columns); updateColumns (); }

	// ListViewModelBase
	tbool CCL_API getUniqueItemName (MutableCString& name, ItemIndexRef index) override;
	tbool CCL_API drawIconOverlay (ItemIndexRef index, const DrawInfo& info) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API openItem (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection) override;
	tbool CCL_API interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection) override;
	tbool CCL_API canRemoveItem (ItemIndexRef index) override;
	tbool CCL_API canSelectItem (ItemIndexRef index) override;
	tbool CCL_API isItemFolder (ItemIndexRef index) override;
	tbool CCL_API canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView = nullptr) override;
	tbool CCL_API insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session) override;
	void CCL_API viewAttached (IItemView* itemView) override;

	void CCL_API notify (CCL::ISubject* subject, CCL::MessageRef msg) override;

protected:
	Browser& browser;
	
	// ListViewModelBase
	ListViewItem* resolve (ItemIndexRef index) const override { return resolveNode (index); }
};

//************************************************************************************************
// BrowserListModel
//************************************************************************************************

class BrowserListModel: public BrowserModelBase
{
public:
	BrowserListModel (Browser& browser);
	~BrowserListModel ();

	bool getNodeIndex (ItemIndex& index, const BrowserNode* node);
	BrowserNode* findNode (const IRecognizer* recognizer) const;
	BrowserNode* findNodeInstance (BrowserNode* node) const;

	BrowserNode* getParent () const;
	void setParent (BrowserNode* node, Container* oldNodes = nullptr);
	
	bool extractChildNodesForReuse (ObjectList& childNodes, BrowserNode* node, ITreeItem* treeItem);

	void collectNodes (Container& result, BrowserNode::NodeFlags flags);
	void addNode (BrowserNode* node, int index = -1);
	bool removeNode (BrowserNode* node);

	void selectNode (BrowserNode* node, bool exclusive);
	void checkAutoSelect ();

	void invalidateNode (BrowserNode* node);
	void updateParentIcon ();

	BrowserNode* getFocusNode (bool onlyIfSelected = false) const;

	// BrowserModelBase
	BrowserNode* resolveNode (ItemIndexRef index) const override;
	int CCL_API countFlatItems () override;
	tbool CCL_API onItemFocused (ItemIndexRef index) override;
	tbool CCL_API appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection) override;
	tbool CCL_API interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection) override;

protected:
	BrowserNode* parent;
	ObjectList parentChain;
	ObjectArray nodes;
	IImageProvider* parentIcon;
	IImageProvider* parentOverlay;
	MutableCString previousParentPath;

	typedef BrowserModelBase SuperClass;
};

//************************************************************************************************
// BrowserTreeModel
//************************************************************************************************

class BrowserTreeModel: public BrowserModelBase
{
public:
	BrowserTreeModel (Browser& browser, BrowserListModel& listModel);

	// BrowserModelBase
	BrowserNode* resolveNode (ItemIndexRef index) const override;
	tbool CCL_API getRootItem (ItemIndex& index) override;
	tbool CCL_API canExpandItem (ItemIndexRef index) override;
	tbool CCL_API canAutoExpandItem (ItemIndexRef index) override;
	tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index) override;
	tbool CCL_API onItemFocused (ItemIndexRef index) override;

protected:
	BrowserListModel& listModel;
};

//************************************************************************************************
// BrowserState
//************************************************************************************************

class BrowserState: public Object
{
public:
	DECLARE_CLASS (BrowserState, Object)

	BrowserState ();
	BrowserState (const BrowserState& state);

	PROPERTY_STRING (name, Name)
	PROPERTY_MUTABLE_CSTRING (rootPath, RootPath)
	PROPERTY_MUTABLE_CSTRING (focusPath, FocusPath)
	PROPERTY_BOOL (listView, ListView)
	PROPERTY_BOOL (focusInList, FocusInList)
	PROPERTY_VARIABLE (int, hScroll, HScroll)
	PROPERTY_VARIABLE (int, vScroll, VScroll)

	IViewStateHandler* getExpandState (ITreeItem& rootItem);
	void setExpandState (IViewStateHandler* state);

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
	bool toString (String& string, int flags = 0) const override { string = name; return true; }

private:
	mutable AutoPtr<Attributes> expandStateAttribs;
	AutoPtr<IViewStateHandler> expandState;
};

//************************************************************************************************
// Browser::SettingsSaver
//************************************************************************************************

class Browser::SettingsSaver: public CCL::SettingsSaver
{
public:
	SettingsSaver (Browser& browser): browser (browser) {}

	// SettingsSaver
	void restore (Settings&) override { browser.loadSettings (); }
	void flush (Settings&) override { browser.saveSettings (); }

private:
	Browser& browser;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// BrowserState
//************************************************************************************************

DEFINE_CLASS (BrowserState, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserState::BrowserState ()
: listView (false),
  focusInList (false),
  hScroll (0),
  vScroll (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserState::BrowserState (const BrowserState& state)
: listView (state.listView),
  focusInList (state.focusInList),
  hScroll (state.hScroll),
  vScroll (state.vScroll)
{
	Attributes attributes;
	Storage storage (attributes);
	state.save (storage);
	load (storage);

	name = state.getName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IViewStateHandler* BrowserState::getExpandState (ITreeItem& rootItem)
{
	if(!expandState && expandStateAttribs)
	{
		expandState = rootItem.createExpandState ();
		ASSERT (expandState)
		expandState->loadViewState (nullptr, nullptr, *expandStateAttribs, nullptr);
	}
	return expandState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserState::setExpandState (IViewStateHandler* state)
{
	expandState = state;
	expandStateAttribs = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserState::load (const Storage& storage)
{
	Attributes& attribs (storage.getAttributes ());
	attribs.get (name, "name");
	attribs.get (rootPath, "root", Text::kUTF8);
	attribs.get (focusPath, "focus", Text::kUTF8);
	attribs.getBool (listView, "listView");
	attribs.getBool (focusInList, "focusInList");

	expandStateAttribs.share (attribs.getAttributes ("state"));
	expandState = nullptr;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserState::save (const Storage& storage) const
{
	Attributes& attribs (storage.getAttributes ());
	attribs.set ("name", name);    
	attribs.set ("root", rootPath, Text::kUTF8);
	attribs.set ("focus", focusPath, Text::kUTF8);
	attribs.set ("listView", listView);
	attribs.set ("focusInList", isFocusInList ());
	
	if(expandStateAttribs || expandState)
	{
		if(!expandStateAttribs)
		{
			expandStateAttribs = NEW Attributes;
			expandState->saveViewState (nullptr, nullptr, *expandStateAttribs, nullptr);
		}
		attribs.set ("state", expandStateAttribs, Attributes::kShare);
	}
	return true;
}

//************************************************************************************************
// Browser::ExpandState
//************************************************************************************************

bool Browser::ExpandState::store (Browser& browser, BrowserNode& node)
{
	ITreeItem* treeItem = browser.findTreeItem (node, false);
	treeState = treeItem ? treeItem->storeExpandState () : nullptr;
	nodeTitle = node.getTitle ();
	return treeState.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::ExpandState::restore (Browser& browser, BrowserNode& node)
{
	if(treeState)
		if(ITreeItem* treeItem = browser.findTreeItem (node, false))
		{
			// restoreExpandState checks if the stored title matches the item: temporarily apply the old title in case the node was renamed
			String newTitle (node.getTitle ());
			node.setTitle (nodeTitle);

			treeItem->restoreExpandState (treeState);

			node.setTitle (newTitle);
			return true;
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum BrowserTags
	{
		kBrowserStates = 100,
		kItemZoom,
		kShowThumbnails,
		kListViewType,
		kListMode,
		kListParentIcon,
		kListParentOverlay,
		kActivityIndicator,
		kFocusNode,
		kFocusNodeExpandable,
		kFocusNodeParent
	};
}

//************************************************************************************************
// Browser
//************************************************************************************************

BEGIN_COMMANDS (Browser)
	DEFINE_COMMAND ("Browser",	"Ascend Root",	Browser::onAscendRoot)
	DEFINE_COMMAND ("Browser",	"Reset Root",	Browser::onResetRoot)
	DEFINE_COMMAND ("Browser",	"New Tab",		Browser::onNewTab)
	DEFINE_COMMAND ("Browser",	"New Root Tab",	Browser::onNewRootTab)
	DEFINE_COMMAND ("Browser",	"Close Tab",	Browser::onCloseTab)
	DEFINE_COMMAND ("Browser",	"Rename Tab",	Browser::onRenameTab)
	DEFINE_COMMAND ("Browser",	"Refresh",		Browser::onRefresh)
	DEFINE_COMMAND_ARGS ("Browser",	"Insert Selected Item", Browser::onInsertSelectedItem, 0, "Replace")
	DEFINE_COMMAND ("Navigation", "Next",		Browser::onNavigationNext)
	DEFINE_COMMAND ("Navigation", "Previous",	Browser::onNavigationNext)
	DEFINE_COMMAND ("Navigation", "Enter",		Browser::onNavigationEnter)
	DEFINE_COMMAND ("Navigation", "Back",		Browser::onNavigationBack)
END_COMMANDS (Browser)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (Browser, Component)

DEFINE_STRINGID_MEMBER_ (Browser, kNodeFocused, "nodeFocused")
DEFINE_STRINGID_MEMBER_ (Browser, kNodeRemoved, "nodeRemoved")
DEFINE_STRINGID_MEMBER_ (Browser, kExpandAll,	"expandAll")
DEFINE_STRINGID_MEMBER_ (Browser, kRefreshAll,	"refreshAll")
DEFINE_STRINGID_MEMBER_ (Browser, kRestoreState, "restoreState")
DEFINE_STRINGID_MEMBER_ (Browser, kTreeRootContext, "TreeRoot")
DEFINE_STRINGID_MEMBER_ (Browser, kChildrenHiddenContext, "childrenHiddenContext")

//////////////////////////////////////////////////////////////////////////////////////////////////

Browser::Browser (StringRef name, StringRef title)
: Component (name, title),
  browserStyle (kShowListView),
  scrollStyle (0, Styles::kScrollViewBehaviorAutoHideBoth|
				  Styles::kScrollViewBehaviorVScrollSpace),
  treeStyle (0, Styles::kTreeViewBehaviorAutoExpand),
  listStyle (0, Styles::kItemViewBehaviorSelection|
				Styles::kListViewAppearanceExtendLastColumn|
				Styles::kListViewAppearanceAutoCenterIcons|
				Styles::kListViewBehaviorNavigateFlat),
  tree (ccl_new<ITree> (ClassID::Tree)),
  rootNode (nullptr),
  topMostNode (nullptr),
  treeRootNode (nullptr),
  defaultColumns (nullptr),
  currentState (nullptr),
  saver (nullptr),
  settingsLoaded (false),
  restoringState (false),
  isRefreshing (false),
  showingSearchResult (false),
  listMode (false),
  trackingEnabled (false),
  extender (NEW BrowserExtender),
  search (nullptr),
  searchProvider (nullptr),
  breadcrumbs (nullptr)
{
	addComponent (extender);

	rootChain.objectCleanup ();
	browserStates.objectCleanup ();

	listModel = NEW BrowserListModel (*this);
	treeModel = NEW BrowserTreeModel (*this, *listModel);

	if(tree)
		tree->setTreeModel (treeModel);

	paramList.addList (CSTR ("browserStates"), Tag::kBrowserStates);
	IParameter* itemZoom = paramList.addFloat (1.f, 2.f, CSTR ("itemZoom"), Tag::kItemZoom);
	itemZoom->setPrecision (10);

	paramList.addParam (CSTR ("showThumbnails"), Tag::kShowThumbnails);
	paramList.addInteger (0, Styles::kNumListViewTypes - 1, CSTR ("listViewType"), Tag::kListViewType)->setValue (Styles::kListViewIcons); // see Styles::ListViewType
	paramList.addParam (CSTR ("listMode"), Tag::kListMode);
	paramList.addImage (CSTR ("listParentIcon"), Tag::kListParentIcon);
	paramList.addImage (CSTR ("listParentOverlay"), Tag::kListParentOverlay);
	paramList.addInteger (0, NumericLimits::kMaxInt16, CSTR ("activityIndicator"), Tag::kActivityIndicator)->setReadOnly (true);

	paramList.addString (CSTR ("focusNode"), Tag::kFocusNode);
	paramList.addParam (CSTR ("focusNodeExpandable"), Tag::kFocusNodeExpandable);
	paramList.addString (CSTR ("focusNodeParent"), Tag::kFocusNodeParent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Browser::~Browser ()
{
	cancelSignals ();

	if(breadcrumbs)
		breadcrumbs->removeObserver (this);

	if(tree)
		tree->release ();
	if(treeModel)
		treeModel->release ();
	if(listModel)
		listModel->release ();
	if(rootNode)
		rootNode->release ();
	if(defaultColumns)
		defaultColumns->release ();
	if(searchProvider)
		searchProvider->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::addExtension (IBrowserExtension* extension)
{
	getExtender ().addExtension (extension);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::addExtensionPlugIns (StringRef category)
{
	getExtender ().addExtensionPlugIns (category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::addSearch ()
{
	ASSERT (search == nullptr)
	addComponent (search = NEW SearchComponent);

	AutoPtr<SearchResultList> resultList (NEW SearchResultList (*this));
	search->setResultViewer (resultList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::addBreadcrumbs ()
{
	ASSERT (breadcrumbs == nullptr)
	addComponent (breadcrumbs = NEW BreadcrumbsComponent);
	breadcrumbs->addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Browser::initialize (IUnknown* context)
{
	ASSERT (!saver)
	Settings::instance ().addSaver (saver = NEW SettingsSaver (*this));
	
	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Browser::terminate ()
{
	if(saver)
		Settings::instance ().removeSaver (saver);
	safe_release (saver);

	safe_release (tree);
	safe_release (treeModel);
	safe_release (listModel);
	safe_release (rootNode);
	rootChain.removeAll ();

	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::trackInteraction () const
{
	if(trackingEnabled)
	{
		Attributes analyticsData;
		analyticsData.set (AnalyticsID::kBrowserName, getName ());
		ccl_analytics_event (AnalyticsID::kBrowserInteraction, &analyticsData);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::clearNodes ()
{
	listModel->setParent (nullptr);
	
	safe_release (rootNode);
	treeRootNode = nullptr;
	rootChain.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::store (Attributes& attributes) const
{
	CCL_PRINTF ("Browser \"%s\" saveState\n", MutableCString (getName ()).str ())
	if(persistentStates ())
	{
		attributes.remove ("states");
		ForEach (browserStates, BrowserState, state)
			attributes.queue ("states", state, Attributes::kShare);
		EndFor

		attributes.set ("stateIndex", paramList.byTag (Tag::kBrowserStates)->getValue ().asInt ());
	}

	attributes.set ("showThumbnails", paramList.byTag (Tag::kShowThumbnails)->getValue ().asBool ());
	attributes.set ("listViewType", paramList.byTag (Tag::kListViewType)->getValue ().asInt ());
	if(hasListMode ())
		attributes.set ("listMode", paramList.byTag (Tag::kListMode)->getValue ().asBool ());

	// store children
	AutoPtr<Attributes> childAttribs (NEW Attributes);
	bool result = saveChildren (Storage (*childAttribs));
	ASSERT (result)
	if(result && !childAttribs->isEmpty ())
		attributes.set (CSTR ("childs"), childAttribs, Attributes::kShare);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::restore (Attributes& attributes)
{
	CCL_PRINTF ("Browser \"%s\" loadState\n", MutableCString (getName ()).str ())

	// restore children
	if(Attributes* childAttribs = attributes.getAttributes ("childs"))
	{
		bool result = loadChildren (Storage (*childAttribs));
		ASSERT (result)
	}

	bool showThumbnails = (treeStyle.custom & Styles::kItemViewAppearanceThumbnails) != 0;
	attributes.getBool (showThumbnails, "showThumbnails");
	treeStyle.setCustomStyle (Styles::kItemViewAppearanceThumbnails, showThumbnails);
	paramList.byTag (Tag::kShowThumbnails)->setValue (showThumbnails);

	int listViewType = getListViewType ();
	attributes.getInt (listViewType, "listViewType");
	paramList.byTag (Tag::kListViewType)->setValue (listViewType);

	if(hasListMode ())
		attributes.getBool (listMode, "listMode");
	paramList.byTag (Tag::kListMode)->setValue (listMode);

	int stateIndex = 0;
	if(persistentStates ())
	{
		ASSERT (browserStates.isEmpty ())
		while(BrowserState* state = attributes.unqueueObject<BrowserState> ("states"))
			addBrowserState (state);

		stateIndex = attributes.getInt ("stateIndex");
	}

	bool mustAddState = browserStates.isEmpty ();
	if(mustAddState)
		addBrowserState (NEW BrowserState);

	ccl_upper_limit (stateIndex, browserStates.count () - 1);
	paramList.byTag (Tag::kBrowserStates)->setValue (stateIndex);
	selectBrowserState (stateIndex);

	if(mustAddState)
		if(BrowserNode* treeRoot = getTreeRoot ())
			renameCurrentState (treeRoot->getTitle ());

	onStatesRestored ();
	updateSearchResultStyle ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::onStatesRestored ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::resetScrollState ()
{
	if(currentState)
	{
		currentState->setHScroll (0);
		currentState->setVScroll (0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::save (const Storage& storage) const
{
	// take a snapshot first
	const_cast<Browser*> (this)->storeCurrentState ();
	return store (storage.getAttributes ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::load (const Storage& storage)
{
	return restore (storage.getAttributes ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& Browser::getSettings () const
{
	String path ("Browser.");
	path.append (getName ());
	return Settings::instance ().getAttributes (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::saveSettings () const
{
	if(settingsLoaded)
	{
		CCL_PRINTF ("Browser \"%s\" saveSettings\n", MutableCString (getName ()).str ())
		store (getSettings ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::loadSettings ()
{
	CCL_PRINTF ("Browser \"%s\" loadSettings\n", MutableCString (getName ()).str ())
	restore (getSettings ());
	settingsLoaded = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RootNode* Browser::getRootNode ()
{
	if(!rootNode)
	{
		rootNode = NEW RootNode (this, getTitle ());
		treeRootNode = rootNode;
		if(tree)
			tree->setRootItem (ccl_as_unknown (treeRootNode));

		ASSERT (rootChain.isEmpty ())
		rootChain.prepend (rootNode);
		rootNode->retain ();

		onInitNodes ();
	}
	return rootNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* Browser::getTreeRoot ()
{
	if(!treeRootNode)
		treeRootNode = getTopMostNode ();
	return treeRootNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::setTreeRoot (BrowserNode* newRoot, bool preserveExpandState, bool updateStateName)
{
	if(!newRoot)
		newRoot = getTopMostNode ();

	if(newRoot != treeRootNode)
	{
		// check if we are going upwards (new root is parent of old root)
		if(preserveExpandState)
			preserveExpandState = treeRootNode && treeRootNode->hasAncestor (newRoot);

		ITreeItem* rootItem = tree->getRootItem ();

		// store expansion states if going upwards
		AutoPtr<IViewStateHandler> oldRootItemState;
		MutableCString oldRootPath;
		if(preserveExpandState && rootItem)
		{
			oldRootItemState = rootItem->storeExpandState ();
			makePath (oldRootPath, treeRootNode);
		}

		MutableCString oldFocusPath;
		if(BrowserNode* focusNode = getFocusNode ())
			makePath (oldFocusPath, focusNode);

		// clear rootChain, but keep it's nodes until we're done
		ObjectList holder;
		holder.objectCleanup ();
		holder.add (rootChain, Container::kShare);
		rootChain.removeAll ();

		// save parent nodes of new root in rootChain
		BrowserNode* parent = newRoot;
		while(parent)
		{
			parent->retain ();
			rootChain.prepend (parent);
			parent = parent->getParent ();
		}

		treeRootNode = newRoot;

		treeModel->signal (Message (IItemModel::kNewRootItem));
		if(!getTreeView () && tree)
			tree->setRootItem (ccl_as_unknown (treeRootNode));

		if(oldRootItemState)
		{
			BrowserNode* oldTreeRoot = findNode (oldRootPath, true); // the new incarnation of this node!
			if(oldTreeRoot)
			{
				ITreeItem* oldTreeRootItem = rootItem->findItem (oldTreeRoot->asUnknown (), false);
				if(oldTreeRootItem)
					oldTreeRootItem->restoreExpandState (oldRootItemState);
			}
		}

		BrowserNode* focusNode = findNode (oldFocusPath, true);
		setTreeFocusNode (focusNode);
		if(updateStateName)
			renameCurrentState (newRoot->getTitle ());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::renameCurrentState (StringRef name)
{
	if(currentState)
	{
		currentState->setName (name);
		UnknownPtr<ISubject> statesParam (paramList.byTag (Tag::kBrowserStates));
		if(statesParam)
			statesParam->signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::storeCurrentState (IItemView* itemView)
{
	if(currentState)
		storeState (*currentState, itemView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::restoreCurrentState ()
{
	if(currentState && getMainItemView ())
		restoreState (*currentState);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* Browser::createSnapshot () const
{
	BrowserState* snapshot = NEW BrowserState;
	storeState (*snapshot);
	return snapshot;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::restoreSnapshot (Object* snapshot)
{
	BrowserState* state = ccl_cast<BrowserState> (snapshot);
	if(!state)
		return false;

	restoreState (*state);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::storeState (BrowserState& state, IItemView* itemView) const
{
	CCL_PRINTF ("Browser \"%s\" storeState %s\n", MutableCString (getName ()).str (), MutableCString (state.getName ()).str ())

	if(!itemView)
		 itemView = getMainItemView ();
	
	UnknownPtr<ITreeView> treeView (itemView);

	// save tree root & focus node
	MutableCString rootPath;
	if(treeRootNode && treeRootNode != rootNode)
		makePath (rootPath, treeRootNode);

	MutableCString focusPath;
	BrowserNode* focusNode = nullptr;

	bool isList = false;
	bool focusInList = false;
	if(isListMode () && listModel->getItemView ())
	{
		isList = true;
		if(focusNode = listModel->getFocusNode (true))
			focusInList = true;
	}
	else if(treeView)
	{
		ItemIndex treeIndex;
		if(treeModel->getItemView ()->getFocusItem (treeIndex))
		{
			focusNode = treeModel->resolveNode (treeIndex);
		
			// avoid descending one level deeper each time when switching between tree and list:
			// use tree focus node as list parent if expanded, othwerwise select the node as child
			ITreeItem* treeItem = treeIndex.getTreeItem ();
			focusInList = treeItem && (treeItem->getState () & ITreeItem::kIsExpanded) == 0;
		}
	}
	if(!focusNode)
		focusNode = getFocusNode (); // (in tree)

	if(focusNode)
		makePath (focusPath, focusNode);

	state.setRootPath (rootPath);
	state.setFocusPath (focusPath);
	state.setFocusInList (focusInList);
	state.setListView (isList);

	CCL_PRINTF ("   root:  %s\n   focus: %s (%s)\n", rootPath.str (), focusPath.str (), state.isFocusInList () ? "list" : "tree")

	if(treeView)
	{
		ITreeItem* rootItem = treeView->getRootItem ();
		if(rootItem)
			state.setExpandState (rootItem->storeExpandState ());
	}

	if(itemView)
	{
		// store scroll params
		if(IScrollView* scrollView = GetViewInterfaceUpwards<IScrollView> (ViewBox (itemView)))
		{
			if(IParameter* param = scrollView->getVScrollParam ())
				state.setVScroll (param->getValue ());
			if(IParameter* param = scrollView->getHScrollParam ())
				state.setHScroll (param->getValue ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::restoreState (BrowserState& state)
{
	CCL_PRINTF ("Browser \"%s\" restoreState %s\n", MutableCString (getName ()).str (), MutableCString (state.getName ()).str ())
	CCL_PRINTF ("   root:  %s\n   focus: %s (%s)\n", state.getRootPath ().str (), state.getFocusPath ().str (), state.isFocusInList () ? "list" : "tree")

	ScopedVar<bool> guard (restoringState, true);

	// restore tree root
	if(canSetRoot ())
	{
		BrowserNode* treeRoot = findNode (state.getRootPath (), true);
		if(!treeRoot)
		{
			// new root is not reachable under current treeRoot, try again from absolute root node
			setTreeFocusNode (nullptr); // setTreeRoot would browse to the old focus node (useless here, we will set another one below)
			setTreeRoot (nullptr, false, false);
			treeRoot = findNode (state.getRootPath (), true);
		}
		if(treeRoot && !treeRoot->isFolder ())
			treeRoot = treeRoot->getParent ();

		setTreeRoot (treeRoot, false, false);
	}

	IItemView* itemView = getMainItemView ();

	UnknownPtr<ITreeView> treeView (itemView);
	ITreeItem* rootItem = treeView ? treeView->getRootItem () : nullptr;
	if(rootItem)
	{
		IViewStateHandler* expandState = state.getExpandState (*rootItem);
		if(expandState)
		{
			rootItem->restoreExpandState (expandState);
			if(treeStyle.isCustomStyle (Styles::kTreeViewAppearanceNoRoot))
				treeView->expandItem (rootItem, true);

			UnknownPtr<IObserver> treeeViewObj (treeView);
			if(treeeViewObj)
				treeeViewObj->notify (nullptr, Message ("updateSize"));
		}
		else
		{
			// expand root, but collapse childs
			treeView->expandItem (rootItem, false, ITreeView::kExpandChilds);
			treeView->expandItem (rootItem, true);
		}
	}

	BrowserNode* focusNode = findNode (state.getFocusPath (), true, true);
	if(!focusNode)
		focusNode = getTreeRoot (); // fallback to treeRoot as focusNode

	// set focus in tree or list view
	// - fallback to tree if we only found an ancestor of the specified node
	// - fallback to tree when listView had no selected child node (focus is list parent then)
	bool foundExactNode = makePath (focusNode) == state.getFocusPath ();
	if(treeView || !state.isFocusInList () || !foundExactNode)
		setTreeFocusNode (focusNode, true);
	else
		setListFocusNode (focusNode, true);

	updateBreadcrumbs (listModel->getParent ());

	if(itemView)
	{
		// restore scroll params (if stored from the same view type)
		if(isListMode () == state.isListView ())
		{
			if(IScrollView* scrollView = GetViewInterfaceUpwards<IScrollView> (ViewBox (itemView)))
			{
				if(IParameter* param = scrollView->getVScrollParam ())
					param->setValue (state.getVScroll ());
				if(IParameter* param = scrollView->getHScrollParam ())
					param->setValue (state.getHScroll ());
			}
		}
		else if(focusNode)
			itemView->makeItemVisible (ItemIndex (focusNode->asUnknown ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::addBrowserState (BrowserNode& node)
{
	MutableCString nodePath;
	makePath (nodePath, &node);

	BrowserState* newState = NEW BrowserState;
	newState->setRootPath (nodePath);
	newState->setName (node.getTitle ());
	addBrowserState (newState);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::addBrowserState (BrowserState* state)
{
	if(state->getName ().isEmpty ())
	{
		String name;
		name.appendIntValue (browserStates.count () + 1);
		state->setName (name);
	}
	browserStates.add (state);

	UnknownPtr<IListParameter> statesParam (paramList.byTag (Tag::kBrowserStates));
	statesParam->appendValue (Variant (ccl_as_unknown (state)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::removeBrowserState (BrowserState* state)
{
	int index = browserStates.index (state);
	if(index >= 0)
	{
		bool wasCurrent = (state == currentState);

		browserStates.remove (state);
		IParameter* statesParam = paramList.byTag (Tag::kBrowserStates);
		UnknownPtr<IListParameter> statesListParam (statesParam);
		statesListParam->removeAt (index);
		state->release ();

		if(wasCurrent)
		{
			currentState = nullptr;
			ccl_upper_limit (index, browserStates.count () - 1);
			selectBrowserState (index);
		}
		else
		{
			// shift index if removed before current
			int currentIndex = browserStates.index (currentState);
			if(currentIndex > index)
				statesParam->setValue (currentIndex - 1, false);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::resetBrowserStates ()
{
	browserStates.removeAll ();
	currentState = nullptr;

	IParameter* statesParam = paramList.byTag (Tag::kBrowserStates);
	UnknownPtr<IListParameter> statesListParam (statesParam);
	statesListParam->removeAll ();

	addBrowserState (NEW BrowserState);
	setTreeRoot (getTopMostNode (), false, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::reorderBrowserState (int index, int newIndex)
{
	BrowserState* state = getBrowserState (index);
	if(state && newIndex >= 0 && newIndex < browserStates.count ())
	{
		bool result = browserStates.remove (state);
		ASSERT (result)
		if(result)
		{
			result = browserStates.insertAt (newIndex, state);
			ASSERT (result)

			// rebuild list param
			IParameter* statesParam = paramList.byTag (Tag::kBrowserStates);
			UnknownPtr<IListParameter> statesListParam (statesParam);
			statesListParam->removeAll ();

			ForEach (browserStates, BrowserState, state)
				statesListParam->appendValue (Variant (state->asUnknown ()));
			EndFor

			// determine new index of current state
			int currentIndex = browserStates.index (currentState);
			statesParam->setValue (currentIndex, true);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::selectBrowserState (BrowserState* state)
{
	if(state)
	{
		if(state != currentState)
		{
			storeCurrentState ();
			currentState = state;
			restoreCurrentState ();

			int index = browserStates.index (currentState);
			ASSERT (index >= 0)
			paramList.byTag (Tag::kBrowserStates)->setValue (index, false);

			if(search)
			{
				search->reset ();
				search->setSearchProvider (getSearchProvider (getFocusNode ()));
			}
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::selectBrowserState (int index)
{
	return selectBrowserState (getBrowserState (index));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Browser::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kBrowserStates :
		selectBrowserState (param->getValue ().asInt ());
		return true;

	case Tag::kItemZoom :
		if(treeModel->getItemView ())
			ViewBox (treeModel->getItemView ())->setZoomFactor (param->getValue ());
		if(listModel->getItemView ())
			ViewBox (listModel->getItemView ())->setZoomFactor (param->getValue ());
		break;

	case Tag::kShowThumbnails:
		treeStyle.setCustomStyle (Styles::kItemViewAppearanceThumbnails, param->getValue ());
		if(treeModel->getItemView ())
		{
			ViewBox vb (treeModel->getItemView ());
			ViewBox::StyleModifier (vb).setCustomStyle (Styles::kItemViewAppearanceThumbnails, param->getValue ());
		}
		updateSearchResultStyle ();
		break;

	case Tag::kListViewType:
		if(UnknownPtr<IListView> listView = getListView ())
		{
			Styles::ListViewType viewType = param->getValue ().asInt ();
			listView->setViewType (viewType);
		}
		updateSearchResultStyle ();
		onViewModeChanged ();
		break;

	case Tag::kListMode:
		storeCurrentState (); // e.g. save focus node from treeview to be restored in icon listview
		listMode = param->getValue ().asBool ();
		updateSearchResultStyle ();
		onViewModeChanged ();
		break;
	}
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::addBrowserNode (BrowserNode* node)
{
	if(getRootNode ())
		getRootNode ()->add (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::setTopMostNode (BrowserNode* node)
{
	ASSERT (rootNode)

	bool isAllowed = !canSetRoot () || (getRootNode () && rootNode->getContent ().contains (node));
	ASSERT (isAllowed)
	if(isAllowed)
		topMostNode = node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* Browser::getTopMostNode ()
{
	if(topMostNode)
		return topMostNode;

	return getRootNode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::canExpandNode (BrowserNode& node)
{
#if 1
	// strictly: only if it has subnodes
	return node.hasSubNodes ();
#else
	// less strictly: even empty folders
	return node.isFolder ();
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::setNodeFilter (IObjectFilter* filter)
{
	nodeFilter.share (filter);

	UnknownPtr<ITreeView> treeView (getTreeView ());
	if(treeView)
		treeView->setItemFilter (filter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::setDefaultColumns (IColumnHeaderList* columns)
{
	take_shared (defaultColumns, columns);

	if(treeModel)
		treeModel->setColumns (columns);
	if(listModel)
		listModel->setColumns (columns);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::updateColumns ()
{
	if(getTreeView ())
		treeModel->updateColumns ();
	if(getListView ())
		listModel->updateColumns ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::selectAll (bool state)
{
	if(IItemView* treeView = treeModel->getItemView ())
		treeView->selectAll (state);

	// todo: list view
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::selectNode (BrowserNode* node, bool state)
{
	IItemView* itemView = treeModel->getItemView ();
	if(node && itemView)
	{
		ItemIndex nodeIndex (node->asUnknown ());
		itemView->selectItem (nodeIndex, state);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::isAnyNodeSelected () const
{
	IItemView* itemView = treeModel->getItemView ();
	return itemView && !itemView->getSelection ().isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::onNodeRemoved (BrowserNode* node)
{
	signal (Message (kNodeRemoved, Variant (ccl_as_unknown (node), true)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::getEditNodes (Container& editNodes, BrowserNode* focusNode)
{
	if(IItemView* itemView = getTreeView ())
		treeModel->getSelectedNodes (editNodes, &itemView->getSelection (), nullptr);

	if(focusNode && !editNodes.contains (focusNode))
	{
		editNodes.objectCleanup (true);
		editNodes.removeAll ();
		editNodes.add (return_shared (focusNode));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::expandAll (bool state, bool deferred)
{
	if(deferred)
		(NEW Message (kExpandAll, state))->post (this);
	else
	{
		UnknownPtr<ITreeView> treeView (treeModel->getItemView ());
		ITreeItem* rootItem = treeView ? treeView->getRootItem () : nullptr;
		if(rootItem)
			treeView->expandItem (rootItem, state, ITreeView::kExpandChilds|ITreeView::kCheckCanAutoExpand);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITreeItem* Browser::findTreeItem (BrowserNode& node, bool create)
{
	if(ITreeItem* rootItem = getRootItem ())
	{
		if(ITreeItem* item = rootItem->findItem (node.asUnknown (), false))
			return item;

		if(create)
		{
			// node is not in tree (may exist e.g. in a folder node)
			// force creation of all ancestors, then try again
			MutableCString path;
			makePath (path, &node);
			findNode (path, true, false);
			return rootItem->findItem (node.asUnknown (), false);
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewModelBase* Browser::getTreeModel () const
{
	return treeModel;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::expandNode (BrowserNode* node, bool state)
{
	UnknownPtr<ITreeView> treeView (treeModel->getItemView ());
	if(treeView && node)
		if(ITreeItem* item = findTreeItem (*node, true))
		{
			int expandMode = state ? ITreeView::kExpandParents : 0;
			treeView->expandItem (item, state, expandMode);
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::isNodeExpanded (BrowserNode& node) const
{
	if(ITreeItem* rootItem = getRootItem ())
		if(ITreeItem* item = rootItem->findItem (node.asUnknown (), false))
			return item->getState () & ITreeItem::kIsExpanded;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::isNodeVisible (BrowserNode& node) const
{
	IItemView* treeView = treeModel ? treeModel->getItemView () : nullptr;
	if(treeView)
	{
		Rect nodeRect;
		treeView->getItemRect (nodeRect, ItemIndex (node.asUnknown ()));
		
		Rect visible;
		ViewBox (treeView).getVisibleClient (visible);
		return visible.intersect (nodeRect);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* Browser::getFocusNode (bool includeSearchResults) const
{
	if(includeSearchResults && isSearchResultsVisible ())
	{
		if(const Url* searchResult = getFocusSearchResult ())
			return ccl_const_cast (this)->findNodeWithUrl (*searchResult);
	}
	else
	{
		if(IItemView* mainItemView = getMainItemView ())
		{
			ItemIndex index;
			if(mainItemView->getFocusItem (index))
				if(BrowserNode* node = resolveNode (*mainItemView, index))
					return node;
		}
		if(listModel)
			return listModel->getParent ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::setTreeFocusNode (BrowserNode* node, bool selectExclusive)
{
	if(node && !node->isFolder () && !displayTreeLeafs ())
		node = nullptr;

	IItemView* treeItemView = treeModel->getItemView ();
	if(treeItemView)
	{
		if(node)
			findTreeItem (*node, true); // force creation of items

		ItemIndex nodeIndex (node ? node->asUnknown () : nullptr);
		treeItemView->setFocusItem (nodeIndex, selectExclusive);
		if(!node)
			treeItemView->selectAll (false);

		if(search)
			search->setSearchProvider (getSearchProvider (getFocusNode ()));
	}
	else if(node)
		onNodeFocused (node, false);
	return node != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::setListFocusNode (BrowserNode* node, bool selectExclusive)
{
	BrowserNode* listParent = node;
	if(listParent)
		listParent = listParent->getParent ();

	ObjectList childNodes;
	ITreeItem* treeItem = listParent ? findTreeItem (*listParent) : nullptr;
	bool canReuseNodes = listModel->extractChildNodesForReuse (childNodes, listParent, treeItem);
	listModel->setParent (listParent, canReuseNodes ? &childNodes : nullptr);

	BrowserNode* listNode = node ? listModel->findNodeInstance (node) : nullptr;
	listModel->selectNode (listNode, true);
	return listNode != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* Browser::getListParentNode () const
{
	BrowserNode* listParent = nullptr;
	if(listModel && isListMode ())
		listParent = listModel->getParent ();
	return listParent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::setFocusNode (BrowserNode* node, bool selectExclusive)
{
	if(isListMode ())
		return setListFocusNode (node, selectExclusive);
	else
		return setTreeFocusNode (node, selectExclusive);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::openNode (BrowserNode* node)
{
	if(node && node->onOpen ())
		return true;

	BrowserNode* listParent = listModel ? listModel->getParent () : nullptr;

	if(node == listParent)
		return false;
	else
	{
		bool result = setTreeFocusNode (node);

		if(isListMode ())
			if(node && node != listModel->getParent ())
				listModel->setParent (node);

		return result;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITreeItem* Browser::insertNodeIntoTree (BrowserNode* parent, BrowserNode* node, int index, ITreeItem* rootItem)
{
	ITreeItem* parentItem = rootItem->findItem (parent->asUnknown (), false);
	if(!parentItem)
		if(BrowserNode* grandParent = parent->getParent ())
		{
			// recursively insert parent
			FolderNode* grandFolder = ccl_cast<FolderNode> (grandParent);
			int parentIndex = grandFolder ? grandFolder->getNodeIndex (parent) : -1;

			parentItem = insertNodeIntoTree (grandParent, parent, parentIndex, rootItem);
			// note: parentItem can be 0 here if grandParent's item exists but wasn't expanded yet (nothing todo then)
		}

	if(parentItem)
	{
		if(parentItem->getState () & ITreeItem::kWasExpanded)
		{
			// parent has already created subItems, add the new one
			parentItem->addSubItem (node->asUnknown (), index);

			Variant itemVariant;
			ItemIndex (node->asUnknown ()).toVariant (itemVariant);
			treeModel->signal (Message (IItemModel::kItemAdded, itemVariant));
		}
		else
		{
			// parent has not created subItems yet, refresh the node and treeItem
			UnknownPtr<ITreeView> treeView (treeModel->getItemView ());
			if(treeView)
			{
				// node might get released! (replaced by a new instance)
				parent->onRefresh ();
				treeView->refreshItem (parentItem);
			}
		}
		return parentItem->findChild (node->asUnknown ());
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::insertNode (BrowserNode* parent, BrowserNode* node, int index)
{
	bool added = false;

	// insert into list
	if(listModel->getParent () == parent)
	{
		listModel->addNode (node, index);
		listModel->signal (Message (kChanged));
		added = true;
	}

	// insert into tree
	if(node->isFolder () || displayTreeLeafs ())
		if(ITreeItem* rootItem = getRootItem ())
			if(insertNodeIntoTree (parent, node, index, rootItem))
				added = true;

	if(added)
		node->setParent (parent);
	node->release ();
	return added;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::removeNode (BrowserNode* node)
{
	SharedPtr<BrowserNode> holder (node);

	BrowserNode* parent = node->getParent ();

	bool removed = false;

	// remove from tree
	if(ITreeItem* rootItem = getRootItem ())
	{
		ITreeItem* item = rootItem->findItem (node->asUnknown (), false);
		if(item)
		{
			if(IItemView* treeItemView = treeModel->getItemView ())
			{
				treeItemView->removeItem (ItemIndex (item));
				ASSERT (!treeItemView->getSelection ().isSelected (item))
			}
			else
				item->remove ();
			removed = true;
		}
	}

	// remove from list
	if(BrowserNode* listParent = listModel ? listModel->getParent () : nullptr)
	{
		if((listParent == node) || listParent->hasAncestor (node))
		{
			// node or an ancestor of node is parent of list
			listModel->setParent (nullptr);
			removed = true;	
		}
		else if(listParent == parent || parent == nullptr) // also check when node has no parent (already removed from parent, e.g. in SortedNode::removeNode)
		{
			// node is in the list
			if(listModel->removeNode (node))
				removed = true;
		}
	}

	if(parent)
	{
		parent->onNodeRemoved (node);
		node->setParent (nullptr);
	}

	if(removed)
		onNodeRemoved (node);

	return removed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::updateThumbnail (BrowserNode* node)
{
	if(UnknownPtr<ITreeView> treeView = getTreeView ())
		treeView->updateThumbnails ();

	if(IItemView* listView = getListView ())
	{
		// the list model might contain a different instance for that node: update its thumbnail as well
		if(BrowserNode* listNode = listModel->findNodeInstance (node))
		{
			listNode->setThumbnail (node->getThumbnail ());
			listModel->invalidateNode (listNode);
		}
	}

	if(search && search->isShowingResult ())
		if(CCL::SearchResultList* resultList = unknown_cast<CCL::SearchResultList> (search->getResultViewer ()))
			resultList->signal (Message (kChanged)); // trigger ItemView::updateSize (invalidate is not enough when item height changes due to thumbnail)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::setActivityIndicatorState (bool state)
{
	IParameter* p = paramList.byTag (Tag::kActivityIndicator);
	if(state)
		p->increment ();
	else
		p->decrement ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::redrawNode (BrowserNode* node)
{
	// check tree view
	IItemView* treeItemView (treeModel->getItemView ());
	UnknownPtr<ITreeView> treeView (treeItemView);
	ITreeItem* rootItem = treeView ? treeView->getRootItem () : nullptr;
	if(rootItem)
	{
		ITreeItem* item = rootItem->findItem (node->asUnknown ());
		if(item)
			treeItemView->invalidateItem (ItemIndex (item));
	}

	// check list view
	IItemView* listItemView (listModel->getItemView ());
	if(listItemView)
		listModel->invalidateNode (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::refreshNode (BrowserNode* node, bool keepExpandState)
{
	if(node->onRefresh () && treeModel != nullptr)
	{
		BrowserNode* listParent = listModel->getParent ();

		// can't refresh invisible child of a flat folder: refresh parent instead
		if(FlatFolderNode* flatFolder = ccl_cast<FlatFolderNode> (node->getParent ()))
			node = flatFolder;

		MutableCString oldTreeFocusPath;
		MutableCString oldListFocusPath;
		MutableCString oldListParentPath;
		bool wasFocusNodeVisible = false;
		if(BrowserNode* focusNode = getFocusNode ())
		{
			makePath (oldTreeFocusPath, focusNode);
			wasFocusNodeVisible = isNodeVisible (*focusNode);
		}

		if(BrowserNode* listFocusNode = listModel->getFocusNode ())
			makePath (oldListFocusPath, listFocusNode);

		IItemView* treeItemView = treeModel->getItemView ();
		IItemView* listItemView = listModel->getItemView ();
		UnknownPtr<ITreeView> treeView (treeItemView);
		ITreeItem* rootItem = treeView ? UnknownPtr<ITreeItem> (treeView->getRootItem ()) : UnknownPtr<ITreeItem> (tree);
		ITreeItem* treeItem = rootItem ? rootItem->findItem (node->asUnknown (), false) : nullptr;
		if(!treeItem && node == getRootNode ())
			treeItem = rootItem;

		Point treeScrollPos;
		Point listScrollPos;
		IScrollView* treeScrollView = nullptr;
		IScrollView* listScrollView = listItemView ? GetViewInterfaceUpwards<IScrollView> (ViewBox (listItemView)) : nullptr;
		if(listScrollView)
			listScrollView->getPosition (listScrollPos);

		if(treeItem)
		{
			AutoPtr<IViewStateHandler> expandState (keepExpandState ? treeItem->storeExpandState () : nullptr);
			
			bool mustUpdateBreadcrumbs = breadcrumbs && node == listParent;
			bool isListAffected = false;

			if(listParent)
			{
				makePath (oldListParentPath, listParent);

				isListAffected = (listParent == node) || listParent->hasAncestor (node);
				if(isListAffected)
					listModel->setParent (nullptr);
			}

			if(treeView)
			{
				if(treeScrollView = GetViewInterfaceUpwards<IScrollView> (ViewBox (treeView)))
					treeScrollView->getPosition (treeScrollPos);

				treeView->refreshItem (treeItem);
			}
			else
				treeItem->removeAll (); // treeview not visible: directly reset child items

			if(expandState)
				treeItem->restoreExpandState (expandState);

			ScopedVar<bool> scope (isRefreshing, true);

			// try to restore focus node (also makes it visible)
			BrowserNode* focusNode = findNode (oldTreeFocusPath, true);
			if(focusNode)
			{
				bool selectExclusive = treeItemView && !treeItemView->getSelection ().isSelected (treeItem);
				setTreeFocusNode (focusNode, selectExclusive);
			}
			if(!oldListParentPath.isEmpty ())
			{
				listParent = findNode (oldListParentPath, true);
				if(listParent)
				{
					ITreeItem* listParentItem = findTreeItem (*listParent, true);

					ObjectList childNodes;
					bool canReuseNodes = listModel->extractChildNodesForReuse (childNodes, listParent, listParentItem);
					listModel->setParent (listParent, canReuseNodes ? &childNodes : nullptr);
				}
			}

			BrowserNode* listFocusNode = findNode (oldListFocusPath, true);
			if(listFocusNode && isListMode ())
				listModel->selectNode (listFocusNode, true);
			else
				listModel->checkAutoSelect ();

			// restore scroll position
			if(treeScrollView)
				treeScrollView->scrollTo (treeScrollPos);

			// make focus node visible (again); more important than absolute scroll position, since tree structure might have changed
			if(wasFocusNodeVisible && treeItemView && focusNode)
				treeItemView->makeItemVisible (ItemIndex (focusNode->asUnknown ()));

			// might have to update breadcrumbs (last segment might now have subFolders)
			if(mustUpdateBreadcrumbs)
				updateBreadcrumbs (listModel->getParent (), true);
		}
		else if(node == listParent)
		{
			SharedPtr<BrowserNode> holder (node);
			listModel->setParent (nullptr);
			listModel->setParent (node);
		}

		// restore scroll position
		if(listScrollView)
			listScrollView->scrollTo (listScrollPos);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::wasExpanded (BrowserNode* node) const
{
	// check if node has revealed it's childs in listModel
	if(listModel && listModel->getParent () == node)
		return true;

	// check if node was expanded in tree
	UnknownPtr<ITreeView> treeView (treeModel->getItemView ());
	ITreeItem* rootItem = treeView ? treeView->getRootItem () : nullptr;
	ITreeItem* treeItem = rootItem ? rootItem->findItem (node->asUnknown (), false) : nullptr;
	return treeItem && (treeItem->getState () & ITreeItem::kWasExpanded);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::canSelectNode (BrowserNode* node) const
{
	if(ccl_cast<SeparatorNode> (node))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* Browser::resolveNode (IItemView& itemView, ItemIndexRef index) const
{
	BrowserModelBase* model = nullptr;
	if(itemView.getModel () == treeModel)
		model = treeModel;
	else if(itemView.getModel () == listModel)
		model = listModel;

	return model ? model->resolveNode (index) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* Browser::findNode (const IRecognizer* recognizer, const BrowserNode* startNode) const
{
	// try tree
	if(ITreeItem* startItem = getRootItem ())
	{
		// find item of startNode
		if(startNode)
			startItem = startItem->findItem (ccl_const_cast (startNode)->asUnknown (), false);
		if(startItem)
		{
			ITreeItem* foundItem = startItem->findItem (recognizer, false);
			return foundItem ? foundItem->getData () : nullptr;
		}
	}

	// try list view
	if(listModel->getParent () == startNode)
	{
		BrowserNode* result = listModel->findNode (recognizer);
		if(result)
			return result->asUnknown ();
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::makePath (MutableCString& path, BrowserNode* node, BrowserNode* startNode) const
{
	ASSERT (node)
	if(node == nullptr)
		return false;

	BrowserNode* parent = node->getParent ();
	if(parent && node != startNode)
	{
		makePath (path, parent, startNode);

		if(ccl_cast<FlatFolderNode> (parent)) // exclude "hidden" childs of a flat folder from path
			return true;

		path.append ("/");
	}
	MutableCString name;
	node->getUniqueName (name);
	path.append (name);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Browser::makePath (BrowserNode* node, BrowserNode* startNode) const
{
	MutableCString path;
	makePath (path, node, startNode);
	return path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::makeDisplayPath (String& path, BrowserNode* node, BrowserNode* startNode) const
{
	ASSERT (node)
	if(node == nullptr)
		return false;

	BrowserNode* parent = node->getParent ();
	if(parent && node != startNode)
	{
		makeDisplayPath (path, parent, startNode);

		if(ccl_cast<FlatFolderNode> (parent)) // exclude "hidden" childs of a flat folder from path
			return true;

		path.append ("/");
	}
	path.append (node->getTitle ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* Browser::findNode (StringID path, bool create, bool acceptAncestor)
{
	if(path.isEmpty () || !getRootNode ())
		return nullptr;

	BrowserNode* foundNode = nullptr;

	MutableCString remainder (path);
	ForEach (rootChain, BrowserNode, node)
		if(remainder.isEmpty ())
			return foundNode;

		int index = remainder.index ("/");
		MutableCString name (remainder.subString (0, index));
		ASSERT (!name.isEmpty ())

		MutableCString nodeName;
		if(node->getUniqueName (nodeName) && name == nodeName)
		{
			foundNode = node;
			if(index < 0 || index >= remainder.length() - 1)
				return foundNode;
			remainder = MutableCString (remainder + index + 1);
		}
		else
			return nullptr;
	EndFor

	if(!remainder.isEmpty ())
	{
		foundNode = nullptr;

		if(ITreeItem* rootItem = getRootItem ())
			if(ITreeItem* item = rootItem->findItem (remainder, create, acceptAncestor))
			{
				foundNode = unknown_cast<BrowserNode> (item->getData ());
				if(isListMode ())
				{
					// if the node we're looking for doesn't exist in the tree, we might have found its parent, that could contain the node as a child in the list view
					if(foundNode && acceptAncestor && foundNode == listModel->getParent () && !(item->getState () & ITreeItem::kWasExpanded))
					{
						MutableCString foundPath (makePath (foundNode));
						if(foundPath != path && path.startsWith (foundPath))
						{
							MutableCString childPath (path.subString (foundPath.length () + 1));
							BrowserNode* childNode = listModel->findNode (AutoPtr<IRecognizer> (Recognizer::create ([&] (IUnknown* obj)
								{
									BrowserNode* node = unknown_cast<BrowserNode> (obj);
									MutableCString name;
									return node && node->getUniqueName (name) && name == childPath;
								})));
							if(childNode)
								foundNode = childNode;
						}
					}
				}
			}
	}
	return foundNode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* Browser::findNodeWithBreadcrumbsPath (StringID path)
{
	BrowserNode* topMostNodeParent = topMostNode ? topMostNode->getParent () : nullptr;
	if(topMostNodeParent)
	{
		// expand to full path
		MutableCString fullPath;
		makePath (fullPath, topMostNodeParent);
		fullPath.append ("/").append (path);

		return findNode (fullPath, true);
	}
	return findNode (path, true);
	}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* Browser::findNodeInSearchResults (UrlRef path) const
{
	if(search && search->isShowingResult ())
		if(CCL::SearchResultList* resultList = unknown_cast<CCL::SearchResultList> (search->getResultViewer ()))
			return resultList->findResultNode (path);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* Browser::findNodeWithUrl (UrlRef url)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* Browser::findNode (IView* view, PointRef where)
{
	UnknownPtr<IItemView> itemView (view);
	if(itemView)
	{
		// check if it's our tree or list view
		BrowserModelBase* model = nullptr;
		if(itemView->getModel () == treeModel)
			model = treeModel;
		else if(itemView->getModel () == listModel)
			model = listModel;

		ItemIndex item;
		if(model && itemView->findItem (item, where))
			return model->resolveNode (item);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::createChildNodes (BrowserNode& node)
{
	if(ITreeItem* rootItem = getRootItem ())
		if(ITreeItem* item = rootItem->findItem (node.asUnknown (), false))
		{
			ScopedVar<bool> guard (restoringState, true);
			item->createSubItems ();
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* Browser::iterateChildNodes (BrowserNode& node) const
{
	if(ITreeItem* rootItem = getRootItem ())
		if(ITreeItem* item = rootItem->findItem (node.asUnknown (), false))
			return item->getContent ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* Browser::navigate (BrowserNode& startNode, int increment, IObjectFilter* filter)
{
	enum { kNavigateFlags = ITreeItem::kOnlySelectable|ITreeItem::kIgnoreRoot };

	// find treeItem of startNode
	ITreeItem* startItem = findTreeItem (startNode, true);
	if(!startItem)
		return nullptr;

	// navigate in tree
	if(ITreeItem* nextItem = startItem->navigate (increment, kNavigateFlags))
	{
		BrowserNode* nextNode = unknown_cast<BrowserNode> (nextItem->getData ());

		if(filter)
		{
			// skip nodes until matched by filter
			increment = ccl_bound (increment, -1, +1);

			while(nextNode && !filter->matches (nextNode->asUnknown ()))
			{
				ITreeItem* next = nextItem->navigate (increment, kNavigateFlags);
				if(next == nextItem)
					break;
				
				nextItem = next;
				nextNode = unknown_cast<BrowserNode> (nextItem->getData ());
			}
		}
		return nextNode;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemView* Browser::getTreeView () const
{
	return treeModel ? treeModel->getItemView () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemView* Browser::getListView () const
{
	return listModel ? listModel->getItemView () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemView* Browser::getMainItemView () const
{
	return isListMode () ? getListView () : getTreeView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* Browser::getWindow ()
{
	UnknownPtr<IView> treeView (treeModel->getItemView ());
	if(treeView)
		return treeView->getIWindow ();

	UnknownPtr<IView> listView (listModel->getItemView ());
	if(listView)
		return listView->getIWindow ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::isVisible () const
{
	return getTreeView () || getListView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::isListMode () const
{
	return listMode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::setDefaultListMode (bool state)
{
	listMode = state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Styles::ListViewType Browser::getListViewType () const
{
	return paramList.byTag (Tag::kListViewType)->getValue ();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::setListViewType (Styles::ListViewType viewType)
{
	return paramList.byTag (Tag::kListViewType)->setValue (viewType, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::updateSearchResultStyle ()
{
	if(CCL::SearchResultList* resultList = search ? unknown_cast<CCL::SearchResultList> (search->getResultViewer ()) : nullptr)
	{
		// result list follows view type of our ListView
		Styles::ListViewType viewType = isListMode () ? getListViewType () : Styles::kListViewList;

		resultList->setShowCategories (viewType == Styles::kListViewList && !resultListHideCategories ());
		resultList->setListViewType (viewType);

		StyleFlags style (resultList->getListStyle ());
		style.setCustomStyle (Styles::kItemViewAppearanceThumbnails, !isListMode () && treeStyle.isCustomStyle (Styles::kItemViewAppearanceThumbnails));
		resultList->setListStyle (style);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Browser::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "hasSearch")
	{
		var = search != nullptr;
		return true;
	}
	if(propertyId == "hasListMode")
	{
		var = hasListMode ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Browser::getObject (StringID name, UIDRef classID)
{
	if(name == "tabsDropTarget")
		if(Component* target = getComponent ("TabsDropTarget"))
			return target->asUnknown ();

	return SuperClass::getObject (name, classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API Browser::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "Browser")
	{
		if(!getFormName ().isEmpty ())
		{
			IView* view = getTheme ()->createView (getFormName (), this->asUnknown ());
			if(view)
			{
				view->setSize (bounds);
				return view;
			}
		}
		return createView ("BrowserView", data, bounds);
	}
	else if(name == "BrowserView")
	{
		Coord treeWidth = bounds.getWidth ();
		if(showListView ())
			treeWidth = Coord (treeWidth * .3);

		// Tree Control
		Rect r (0, 0, treeWidth, bounds.getHeight ());
		IView* treeControl = createTreeView (r);

		if(showListView ())
		{
			// layout container
			ViewBox frame (ClassID::AnchorLayoutView, bounds, Styles::kHorizontal);
			frame.setSizeMode (IView::kAttachAll);
			frame.setAttribute (ATTR_SPACING, 0);
			frame.getChildren ().add (treeControl);

			Coord dividerWidth = frame.getTheme ().getThemeMetric (ThemeElements::kDividerSize);
			Coord listWidth = bounds.getWidth () - treeWidth - dividerWidth;

			// Divider
			r (0, 0, dividerWidth, bounds.getHeight ());
			ViewBox divider (ClassID::Divider, r, Styles::kHorizontal);
			divider.setSizeMode (IView::kAttachTop|IView::kAttachBottom);
			divider.setSizeLimits (SizeLimit (dividerWidth, 0, dividerWidth, kMaxCoord));
			frame.getChildren ().add (divider);

			// List Control
			r (0, 0, listWidth, bounds.getHeight ());
			frame.getChildren ().add (createListView (r));
			return frame;
		}
		else
			return treeControl;
	}
	else if(name == "TreeView")
	{
		return createTreeView (bounds);
	}
	else if(name == "ListView")
	{
		listModel->setParent (getTreeRoot ());
		return createListView (bounds);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* Browser::createTreeView (const Rect& bounds)
{
	// Tree Control
	ViewBox treeControl (ClassID::TreeControl, bounds, scrollStyle);
	treeControl.setSizeMode (IView::kAttachAll);

	UnknownPtr<IItemView> treeItemView (treeControl);
	UnknownPtr<ITreeView> treeView (treeItemView);
	treeView->setTree (tree);
	treeView->setItemFilter (nodeFilter);

	StyleFlags treeStyle = this->treeStyle;
	treeStyle.custom |= Styles::kItemViewBehaviorSelection|Styles::kItemViewBehaviorSwallowAlphaChars;
	if(defaultColumns && defaultColumns->getColumnCount () > 1 && !hideColumnHeaders ())
		treeStyle.custom |= Styles::kItemViewAppearanceHeader;
	ViewBox treeViewBox (treeItemView);
	treeViewBox.setStyle (treeStyle);

	treeItemView->setModel (treeModel);

	float itemZoom = paramList.byTag (Tag::kItemZoom)->getValue ();
	treeViewBox.setZoomFactor (itemZoom);
	return treeControl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* Browser::createListView (const Rect& bounds)
{
	ViewBox listControl (ClassID::ListControl, bounds, scrollStyle);
	listControl.setSizeMode (IView::kAttachAll);

	IItemView* listView = listControl.as<IItemView> ();

	StyleFlags listStyle = this->listStyle;
	
	if(defaultColumns && defaultColumns->getColumnCount () > 1 && !hideColumnHeaders ())
		listStyle.custom |= Styles::kItemViewAppearanceHeader|Styles::kItemViewBehaviorSwallowAlphaChars;
	ViewBox listViewBox (listView);
	listViewBox.setStyle (listStyle);

	listView->setModel (listModel);

	float itemZoom = paramList.byTag (Tag::kItemZoom)->getValue ();
	listViewBox.setZoomFactor (itemZoom);

	listControl.as<IListView> ()->setViewType (getListViewType ());
	listControl.as<IListView> ()->setTextTrimMode (Font::kTrimModeRight);
	
	return listControl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::isDragSource (IDragSession& session)
{
	if(IUnknown* source = session.getSource ())
		return isEqualUnknown (source, getTreeView ())
			|| isEqualUnknown (source, getListView ());

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Browser::navigateTo (NavigateArgs& args)
{
	Rect bounds;
	ViewBox contentFrame = &args.contentFrame;
	contentFrame.getClientRect (bounds);

	IView* view = createView ("Browser", Variant (), bounds);
	ASSERT (view != nullptr)

	FormBox form (view->getSize ());
	form.setSizeMode (IView::kAttachAll);
	form.getChildren ().add (view);
	form.setController (this->asUnknown ());
	form.setSize (bounds);

	contentFrame.getChildren ().removeAll ();
	contentFrame.setTitle (getTitle ());
	contentFrame.getChildren ().add (form);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::setSearchProvider (ISearchProvider* provider)
{
	safe_release (searchProvider);
	searchProvider = provider;
	
	if(search)
		search->setSearchProvider (getSearchProvider (getFocusNode ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearchProvider* Browser::getSearchProvider () const
{
	return searchProvider;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::setSearchIcon (IImage* icon)
{
	if(SearchProvider* sp = unknown_cast<SearchProvider> (searchProvider))
	{
		sp->setSearchIcon (icon);

		// update in search component 
		if(search)
			search->setSearchProvider (getSearchProvider (getFocusNode ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearchProvider* Browser::getSearchProvider (BrowserNode* focusNode)
{
	if(searchProvider)
		return searchProvider;

	if(focusNode)
		if(ISearchProvider* provider = focusNode->getSearchProvider ())
			return provider;

	return getTreeRoot ()->getSearchProvider ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::drawIconOverlay (BrowserNode& node, const IItemModel::DrawInfo& info)
{
	return node.drawIconOverlay (info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::onNodeFocused (BrowserNode* node, bool inList)
{
	paramList.checkCommandStates ();

	if(search && node && node->getBrowser ()) // don't use search result node as provider
		search->setSearchProvider (getSearchProvider (node));

	if(breadcrumbs && !inList && !isRefreshing)
		updateBreadcrumbs (node);

	if(!inList)
		listModel->checkAutoSelect ();

	if(inList == isListMode ()) // ignore focus change in "other" item model
	{
		paramList.byTag (Tag::kFocusNode)->setValue (node ? node->getTitle () : String::kEmpty);
		if(node)
		{
			paramList.byTag (Tag::kFocusNodeExpandable)->setValue (canExpandNode (*node));

			BrowserNode* parentNode = node->getParent ();
			paramList.byTag (Tag::kFocusNodeParent)->setValue (parentNode ? parentNode->getTitle () : String::kEmpty);
		}
	}

	IUnknown* nodeUnknown = node ? node->asUnknown () : nullptr;
	signal (Message (kNodeFocused, nodeUnknown));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::updateBreadcrumbs (BrowserNode* node, bool force)
{
	if(breadcrumbs)
	{
		MutableCString pathString;
		String displayPath;

		if(node)
			if(BrowserNode* folder = node->isFolder () ? node : node->getParent ())
			{
				// start the paths at the highest folder "reachable" for the user
				makePath (pathString, folder, topMostNode);
				makeDisplayPath (displayPath, folder, topMostNode);
			}
		breadcrumbs->setPath (String (Text::kUTF8, pathString), displayPath, force);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::onEditNode (BrowserNode& node, StringID columnID, const IItemModel::EditInfo& info)
{
	trackInteraction (); // e.g. click on node (most nodes do not handle onEdit)

	return node.onEdit (columnID, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool Browser::canInsertData (BrowserNode* node, const IUnknownList& data, IDragSession* session, IView* targetView)
{
	if(node)
	{
		UnknownPtr<IDataTarget> nodeTarget (node->asUnknown ());
		if(nodeTarget && nodeTarget->canInsertData (data, session, targetView))
			return true;

		// try to turn the tables: offer node to a dragged data target
		UnknownPtr<IDataTarget> sourceTarget (session ? session->getItems ().getFirst () : nullptr);
		if(sourceTarget)
		{
			UnknownList data2;
			data2.add (node->asUnknown (), true);
			return sourceTarget->canInsertData (data2, session, targetView);
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool Browser::insertData (BrowserNode* node, const IUnknownList& data, IDragSession* session)
{
	if(node)
	{
		UnknownPtr<IDataTarget> nodeTarget (node->asUnknown ());
		if(nodeTarget && nodeTarget->insertData (data, session))
			return true;

		UnknownPtr<IDataTarget> sourceTarget (session ? session->getItems ().getFirst () : nullptr);
		if(sourceTarget)
		{
			UnknownList data2;
			data2.add (node->asUnknown (), true);
			return sourceTarget->insertData (data2, session);
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Browser::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IItemView::kSelectionChanged)
	{
		paramList.checkCommandStates ();

		if(search)
		{
			// reset search provider when nothing is selected
			IItemView* treeView = getTreeView ();
			if(treeView && treeView->getSelection ().isEmpty ())
				search->setSearchProvider (getSearchProvider (nullptr));
		}
		return;
	}
	else if(msg == IItemView::kViewAttached)
	{
		IItemView* mainItemView = getMainItemView ();
		if(isEqualUnknown (subject, mainItemView))
		{
			// defer restoring state (but not if coming back from search result viewer)
			if(!showingSearchResult)
				(NEW Message (kRestoreState))->post (this);

			showingSearchResult = false;
		}

		if(isListMode ())
			if(UnknownPtr<IListView> listView = subject)
				listView->setViewType (getListViewType ());

		onViewShown (mainItemView);
	}
	else if(msg == IItemView::kViewRemoved)
	{
		IItemView* mainItemView = getMainItemView ();
		if(isEqualUnknown (subject, mainItemView))
		{
			showingSearchResult = search && search->isShowingResult ();

			storeCurrentState (mainItemView);
			#if 0 // shouldn't be necessary anymore, this came from a time when the tree didn't exist without a treeview
			if(listModel)
				listModel->setParent (0); // release our focus node, his parent may die soon!
			#endif
		}
	}
	else if(msg == IItemView::kDragSessionDone)
	{
		UnknownPtr<IDragSession> session (msg[0]);
		if(session && session->getResult () != IDragSession::kDropNone)
			trackInteraction ();
	}
	else if(msg == kExpandAll)
	{
		expandAll (msg[0].asBool ());
		return;
	}
	else if(msg == kRefreshAll)
	{
		refreshAll (false);
		return;
	}
	else if(msg == kRestoreState)
	{
		WaitCursor waitCursor (System::GetGUI ()); // this could take a while
		if(!settingsLoaded)
			loadSettings ();
		else
			restoreCurrentState (); // (is also called during loadSettings)
	}
	else if(msg == Signals::kTabViewReorder)
	{
		if(msg.getArgCount () >= 3 && msg[0] == "browserStates")
			reorderBrowserState (msg[1], msg[2]);
	}
	else if(msg == CCL::Signals::kTabViewGetDataTarget)
	{
		if(msg.getArgCount () >= 2 && msg[0] == "browserStates")
			if(IUnknown* dataTarget = getObject ("tabsDropTarget", ccl_iid<IDataTarget> ()))
			{
				UnknownPtr<IVariant> result (msg[1]);
				if(result)
					result->assign (dataTarget);
			}
	}
	else if(subject == breadcrumbs)
	{
		if(msg == BreadcrumbsComponent::kPathSelected)
		{
			MutableCString path (msg[0].asString (), Text::kUTF8);
			
			if(BrowserNode* node = findNodeWithBreadcrumbsPath (path))
				setTreeFocusNode (node);
		}
		else if(msg == BreadcrumbsComponent::kQuerySubFolders)
		{
			UnknownPtr<BreadcrumbsComponent::ISubFolderQuery> subFolderQuery (msg[0]);
			if(subFolderQuery)
			{
				BrowserNode* node = findNodeWithBreadcrumbsPath (MutableCString (subFolderQuery->getParentPath (), Text::kUTF8));
				if(node)
				{
					// find sub nodes
					ObjectList subNodes;
					subNodes.objectCleanup (true);
					if(node == listModel->getParent ())
						listModel->collectNodes (subNodes, BrowserNode::NodeFlags::kFolders);
					else
						node->getSubNodes (subNodes, BrowserNode::NodeFlags::kFolders);

					// provide node names, titles, icons
					for(auto child : iterate_as<BrowserNode> (subNodes))
					{
						MutableCString name;
						child->getUniqueName (name);

						IImage* icon = child->getIcon ();
						if(!icon)
							icon = FileIcons::instance ().getDefaultFolderIcon ();
						subFolderQuery->addSubFolder (String (name), child->getTitle (), icon);
					}
				}
			}
		}
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::isWindowBaseActive (IItemView* itemView) const
{
	if(itemView)
		if(IWindowBase* windowBase = GetViewInterfaceUpwards<IWindowBase> (ViewBox (itemView)))
			return windowBase->isActive () != 0;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::isActive () const
{
	return isWindowBaseActive (getTreeView ()) || isWindowBaseActive (getListView ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::isSearchResultsVisible () const
{
	if(search && search->isShowingResult ())
	{
		ISearchResultViewer* viewer = search->getResultViewer ();
		if(viewer && viewer->isViewVisible ())
			if(CCL::SearchResultList* resultList = unknown_cast<CCL::SearchResultList> (search->getResultViewer ()))
				return resultList->getItemView () != nullptr;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Url* Browser::getFocusSearchResult () const
{
	if(search && search->isShowingResult ())
		if(CCL::SearchResultList* resultList = unknown_cast<CCL::SearchResultList> (search->getResultViewer ()))
			if(auto resultNode = ccl_cast<Browsable::FileNode> (resultList->getFocusItem ()))
				return resultNode->getPath ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::showSelectedSearchResultInContext ()
{
	auto* resultList = search ? unknown_cast<CCL::SearchResultList> (search->getResultViewer ()) : nullptr;
	return resultList ? resultList->showSelectedResultInContext () : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::canInterpretInSearchMode (const CommandMsg& msg) const
{
	return msg.name.startsWith ("Insert Selected ");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Browser::interpretCommand (const CommandMsg& msg)
{
	bool searchVisible = isSearchResultsVisible ();
	if(!isActive ())
	{
		// delegate to search child component, if visible
		if(searchVisible)
			if(search->interpretCommand (msg))
				return true;

		// open browser before search component can perform "Search" command
		if(search && msg.category == "Edit" && msg.name == "Search")
		{
			Attributes args;
			args.set ("workspaceID", RootComponent::instance ().getApplicationID ());
			args.set ("State", true);
			if(System::GetCommandTable ().performCommand (CommandMsg (CSTR ("View"), CSTR ("Browser"), args.asUnknown (), msg.flags)) && msg.checkOnly ())
				return true;
		}
	}

	// built-in commands
	if(!searchVisible || canInterpretInSearchMode (msg))
		if(CommandDispatcher<Browser>::dispatchCommand (msg))
		{
			if(!msg.checkOnly ())
				trackInteraction ();
			return true;
		}

	if(!isVisible ())
		return false;

	// delegate to tree & list view (they will delegate to the focus/selected nodes)
	ICommandHandler* handler1 = nullptr;
	ICommandHandler* handler2 = nullptr;
	if(treeModel && listModel)
	{
		UnknownPtr<IView> treeView (treeModel->getItemView ());
		UnknownPtr<IView> listView (listModel->getItemView ());

		if(treeView && ViewBox (treeView).isAttached ())
			handler1 = UnknownPtr<ICommandHandler> (treeView->getController ());

		if(listView && ViewBox (listView).isAttached ())
		{
			IUnknown* listController = listView->getController ();
			handler2 = UnknownPtr<ICommandHandler> (listController);
		
			// try list view first if it's focused
			if(IWindow* window = ViewBox (listView).getWindow ())
				if(IView* focusView = window->getFocusIView ())
					if(focusView->getController () == listController)
						ccl_swap (handler1, handler2);
		}
	}

	bool result = false;

	if(handler1 && handler1->interpretCommand (msg))
		result = true;
	else if(handler2 && handler2->interpretCommand (msg))
		result = true;
	else
		result = SuperClass::interpretCommand (msg); // child components

	if(result && !msg.checkOnly ())
		trackInteraction ();

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::interpretNodeCommand (const CommandMsg& msg, ItemModel& model, BrowserNode* targetNode, const IItemSelection& selection)
{
	auto* browserModel = ccl_cast<BrowserModelBase> (&model);
	if(!browserModel)
		return false;

	if(msg.category == "Edit" && msg.name == "Delete")
	{
		AutoPtr<NodeRemover> remover (NEW NodeRemover (*browserModel, selection));
		return remover->perform (msg.checkOnly ());
	}
	else if(msg.category == "Browser" && msg.name == "Set as Root")
	{
		if(canSetRoot ())
		{
			ForEachItem (selection, index)
				BrowserNode* node = browserModel->resolveNode (index);
				if(node && node->hasSubNodes ())
				{
					if(!msg.checkOnly ())
					{
						setTreeRoot (node, true, true);
						trackInteraction ();
					}
					return true;
				}
				break;
			EndFor
		}
		return false;
	}

	// check if called from a context menu for the tree root node
	bool isTreeRoot = false;
	UnknownPtr<IMenuItem> menuItem (msg.invoker);
	if(menuItem)
	{
		IMenu* menu = menuItem->getParentMenu ();
		Variant value;
		if(menu && menu->getMenuAttribute (value, IMenu::kMenuData))
		{
			UnknownPtr<IContextMenu> contextMenu (value);
			if(contextMenu && contextMenu->getContextID () == Browser::kTreeRootContext)
				isTreeRoot = true;
		}
	}

	ObjectList selectedNodes;
	browserModel->getSelectedNodes (selectedNodes, &selection, isTreeRoot ? getTreeRoot () : nullptr);

	// todo: maybe ask focus node first
	ForEach (selectedNodes, BrowserNode, node)
		if(node->interpretCommand (msg, &selectedNodes))
		{
			trackInteraction ();
			return true;
		}
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::appendNodeContextMenu (BrowserNode& node, IContextMenu& contextMenu, Container* selectedNodes)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Browser::appendContextMenu (IContextMenu& contextMenu)
{
	if(contextMenu.getContextID ().startsWith ("TabView:browserStates"))
	{
		int64 tabIndex = 0;
		if(contextMenu.getContextID ().subString (22).getIntValue (tabIndex))
			selectBrowserState ((int)tabIndex);

		if(treeModel)
		{
			treeModel->appendNodeContextMenu (contextMenu, nullptr, nullptr); // for tree root
			contextMenu.addSeparatorItem ();
	}
	}

	if(canSetRoot ())
	{
		contextMenu.addCommandItem (XSTR (Up), CSTR ("Browser"), CSTR ("Ascend Root"), nullptr);
		contextMenu.addCommandItem (XSTR (ResetRoot), CSTR ("Browser"), CSTR ("Reset Root"), nullptr);
	}

	if(canAddTabs ())
	{
		contextMenu.addSeparatorItem ();
		if(canSetRoot ())
			contextMenu.addCommandItem (XSTR (NewTabFromHere), CSTR ("Browser"), CSTR ("New Tab"), nullptr);
		contextMenu.addCommandItem (XSTR (NewRootTab), CSTR ("Browser"), CSTR ("New Root Tab"), nullptr);
		contextMenu.addCommandItem (XSTR (CloseTab), CSTR ("Browser"), CSTR ("Close Tab"), nullptr);
		contextMenu.addCommandItem (XSTR (RenameTab), CSTR ("Browser"), CSTR ("Rename Tab"), nullptr);
	}

	if(!showingSearchResult)
		contextMenu.addCommandItem (XSTR (Refresh), CSTR ("Browser"), CSTR ("Refresh"), nullptr);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::onAscendRoot (CmdArgs args)
{
	if(!isActive ())
		return false;

	BrowserNode* treeRoot = getTreeRoot ();
	BrowserNode* parent = treeRoot ? treeRoot->getParent () : nullptr;
	if(!canSetRoot () || !parent || treeRoot == getTopMostNode ())
		return false;

	if(!args.checkOnly ())
		setTreeRoot (parent, true, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::onResetRoot (CmdArgs args)
{
	if(!isActive ())
		return false;

	BrowserNode* top = getTopMostNode ();
	if(!canSetRoot () || top == getTreeRoot ())
		return false; // already at topmost node

	if(!args.checkOnly ())
		setTreeRoot (top, true, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::onNewTab (CmdArgs args)
{
	if(!isActive ())
		return false;

	BrowserNode* focusNode = getFocusNode ();
	if(!focusNode)
		return false;
	if(!focusNode->isFolder ())
		return false;

	if(!args.checkOnly () && currentState)
	{
		storeCurrentState ();

		BrowserState* newState = NEW BrowserState (*currentState);
		if(!newState->getFocusPath ().isEmpty ())
			newState->setRootPath (newState->getFocusPath ());

		addBrowserState (newState);
		selectBrowserState (newState);
		if(BrowserNode* rootNode = getTreeRoot ())
			renameCurrentState (rootNode->getTitle ());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::onNewRootTab (CmdArgs args)
{
	if(!isActive ())
		return false;

	if(!args.checkOnly () && currentState)
	{
		storeCurrentState ();

		BrowserState* newState = NEW BrowserState;

		addBrowserState (newState);
		selectBrowserState (newState);
		if(BrowserNode* rootNode = getTreeRoot ())
			renameCurrentState (rootNode->getTitle ());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::onCloseTab (CmdArgs args)
{
	if(!isActive ())
		return false;

	if(!currentState || browserStates.count () < 2)
		return false;

	if(!args.checkOnly ())
		removeBrowserState (currentState);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::onRenameTab (CmdArgs args)
{
	if(!isActive ())
		return false;

	if(!currentState)
		return false;

	if(!args.checkOnly ())
	{
		ParamContainer params;
		IParameter* param = params.addString (CSTR ("Name"));// todo: translate?
		param->fromString (currentState->getName ());
		if(DialogBox ()->runWithParameters (CCLSTR ("RenameBrowserTabDialog"), params, XSTR (RenameTab)) == DialogResult::kOkay)
		{
			String name = param->getValue ().asString ();
			if(name != currentState->getName ())
				renameCurrentState (name);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::onRefresh (CmdArgs args)
{
	if(!isActive ())
		return false;

#if 1
	if(!args.checkOnly ())
		if(prepareRefresh ())
			refreshAll (false);
#else
	// old behavior: only refresh focus node
	BrowserNode* focusNode = getFocusNode ();
	if(!focusNode)
		return false;

	if(!args.checkOnly ())
		refreshNode (focusNode);
#endif
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::onInsertSelectedItem (CmdArgs args)
{
	if(!isVisible ())
		return false;

	AutoPtr<IUnknown> item;

	if(isSearchResultsVisible ())
	{
		if(const Url* searchResult = getFocusSearchResult ())
			item = ccl_as_unknown (NEW Url (*searchResult));
	}
	else
	{
		BrowserNode* focusNode = getFocusNode ();
		if(!focusNode)
			return false;

		item = focusNode->createDragObject ();
	}

	if(!item)
		return false;

	if(!args.checkOnly ())
	{
		Message msg (Signals::kInsertData, Variant (item, true), CommandAutomator::Arguments (args).getBool ("Replace"));
		SignalSource (Signals::kEditing).signal (msg);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::onNavigationNext (CmdArgs args)
{
	if(!isActive ())
		return false;

	if(!args.checkOnly ())
	{
		bool isNext = args.name.contains ("Next");

		if(isListMode ())
			interpretCommand (CommandMsg ("Navigation", isNext ? "Right" : "Left"));
		else
			interpretCommand (CommandMsg ("Navigation", isNext ? "Down" : "Up"));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::onNavigationEnter (CmdArgs args)
{
	if(!isActive ())
		return false;

	if(!args.checkOnly ())
	{
		if(isListMode ())
		{
			// list: enter selected folder
			if(BrowserNode* listFocusNode = getFocusNode ())
				openNode (listFocusNode);
		}
		else
			interpretCommand (CommandMsg ("Navigation", "Right")); // tree: expand folder or move down
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::onNavigationBack (CmdArgs args)
{
	if(!isActive ())
		return false;

	if(!args.checkOnly ())
	{
		if(isListMode ())
		{
			// list: up to parent folder
			if(BrowserNode* listParent = listModel->getParent ())
				if(BrowserNode* parent = listParent->getParent ())
					openNode (parent);
		}
		else
			interpretCommand (CommandMsg ("Navigation", "Left")); // tree: collapse folder or move up
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Browser::refreshAll (bool deferred)
{
	if(deferred)
		(NEW Message (kRefreshAll))->post (this, -1);
	else
	{
		// refresh tree root node and restore current state
		if(BrowserNode* topNode = getTreeRoot ())
		{
			AutoPtr<BrowserState> bs;
			BrowserState* state = currentState;
			if(!state)
				bs = state = NEW BrowserState;

			storeState (*state);

			ScopedVar<bool> guard (restoringState, true); // causes presets to be fetched synchronously (see PresetNodesBuilder::shouldForcePresets, fixes issue with disappearing presets of focus node)

			refreshNode (topNode);
			restoreState (*state);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::prepareRefresh ()
{
	// focus node might need a refresh notification, too
	BrowserNode* focusNode = nullptr;
	if(isListMode () && listModel)
	{
		focusNode = listModel->getFocusNode (true);
		if(focusNode == nullptr)
			focusNode = listModel->getParent ();
	}
	else
		focusNode = getFocusNode ();

	if(focusNode && focusNode != getTreeRoot ())
		focusNode->onRefresh ();

	return true;
}

//************************************************************************************************
// NodeRemover
//************************************************************************************************

NodeRemover::NodeRemover (Browser& browser, const ObjectList& nodes)
: browser (browser),
  checkOnly (false),
  removeDeferred (false)
{
	candidates.objectCleanup ();
	candidates.add (nodes, Container::kShare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NodeRemover::NodeRemover (BrowserModelBase& browserModel, const IItemSelection& selection)
: browser (browserModel.getBrowser ()),
  checkOnly (false),
  removeDeferred (false)
{
	candidates.objectCleanup ();

	// collect nodes from selection
	ForEachItem (selection, index)
		BrowserNode* node = browserModel.resolveNode (index);
		if(node)
		{
			node->retain ();
			candidates.add (node);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NodeRemover::removeNode (BrowserNode* node)
{
	removed.add (node);
	remaining.remove (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NodeRemover::keepNode (BrowserNode* node)
{
	remaining.remove (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NodeRemover::perform (bool checkOnly)
{
	if(!checkOnly)
	{
		// close popup first
		CommandMsg cmd ("", "perform");
		if(System::GetDesktop ().closePopupAndDeferCommand (this, cmd))
		{
			retain ();
			return true;
		}
	}

	setCheckOnly (checkOnly);
	remaining.add (candidates);

	if(checkOnly)
	{
		ListForEachObject (candidates, BrowserNode, node)
			if(node->performRemoval (*this))
				return true;
		EndFor
		return false;
	}
	else
	{
		// offer all remaining nodes to each node
		ListForEachObject (candidates, BrowserNode, node)
			if(remaining.contains (node))
			{
				node->performRemoval (*this);
				if(remaining.isEmpty ())
					break;
			}
		EndFor
	
		browser.trackInteraction ();

		if(removeDeferred)
		{
			retain ();
			(NEW Message (CSTR ("removeNodes")))->post (this);
		}
		else
			removeNodes ();
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void NodeRemover::removeNodes ()
{
	IWindow::UpdateCollector uc (browser.getWindow ());

	ListForEachObject (removed, BrowserNode, node)
		browser.removeNode (node);
	EndFor

	browser.selectAll (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NodeRemover::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "removeNodes")
	{
		removeNodes ();
		release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NodeRemover::checkCommandCategory (CStringRef category) const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NodeRemover::interpretCommand (const CommandMsg& msg)
{
	if(msg.name == "perform")
	{
		perform (false);
		release ();
		return true;
	}
	return false;
}

//************************************************************************************************
// NewTabTarget
//************************************************************************************************

NewTabTarget::NewTabTarget (Browser* browser)
: Component (CCLSTR ("TabsDropTarget")),
  browser (browser)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NewTabTarget::canInsertData (const IUnknownList& data, IDragSession* session, IView* targetView, int insertIndex)
{
	if(browser && canCreateTab (*browser, data))
	{
		if(session)
		{
			if(targetView)
			{
				AutoPtr<DragHandler> feedBack (NEW DragHandlerDelegate<DragHandler> (targetView, this));
				feedBack->getSpriteBuilder ().addHeader (XSTR (NewTab));
				feedBack->buildSprite ();
				session->setDragHandler (feedBack);
			}
			session->setResult (IDragSession::kDropCopyReal);
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NewTabTarget::insertData (const IUnknownList& data, IDragSession* session, int insertIndex)
{
	if(browser && canCreateTab (*browser, data))
	{
		// create new root tab
		browser->onNewRootTab (CommandMsg ());

		// find (create) the new tab root node and set as root
		if(BrowserNode* tabRoot = findNewTabRoot (*browser, data))
			browser->setTreeRoot (tabRoot, false, true);

		return true;
	}
	return false;
}

//************************************************************************************************
// BrowserModelBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT (BrowserModelBase, ListViewModelBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserModelBase::BrowserModelBase (Browser& browser)
: browser (browser)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserModelBase::getUniqueItemName (MutableCString& name, ItemIndexRef index)
{
	BrowserNode* node = resolveNode (index);
	return node ? node->getUniqueName (name) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserModelBase::drawIconOverlay (ItemIndexRef index, const DrawInfo& info)
{
	if(BrowserNode* node = resolveNode (index))
		return browser.drawIconOverlay (*node, info);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserModelBase::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	if(BrowserNode* node = resolveNode (index))
	{
		if(columns)
			return node->drawDetail (info, columns->getColumnID (column), Alignment ());
		else if(column == 0)
			return node->drawDetail (info, nullptr, Alignment ()); // give node a chance to draw the title on it's own
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserModelBase::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	if(BrowserNode* node = resolveNode (index))
	{
		#if 0 // dosn't seem to be required (done by listview after selection) but introduces unwanted latency
		// in list mode, first try to open folder on double click
		if(browser.isListMode () && node->isFolder () && UnknownPtr<IListView> (info.view).isValid ())
			if(auto mouseEvent = info.editEvent.as<MouseEvent> ())
			{
				if(info.view->detectDoubleClick (*mouseEvent))
					if(browser.openNode (node))
						return true;
			}
		#endif

		StringID columnID = columns ? columns->getColumnID (column) : CString::kEmpty;
		return browser.onEditNode (*node, columnID, info);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserModelBase::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	BrowserNode* node = resolveNode (index);
	Browser* browser = node ? node->getBrowser () : nullptr;
	if(browser)
		return browser->openNode (node);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool BrowserModelBase::appendNodeContextMenu (IContextMenu& menu, BrowserNode* contextNode, const IItemSelection* selection)
{
	// use tree root if menu was not requested for a node (e.g. clicked in empty space)
	BrowserNode* treeRoot = nullptr;
	BrowserNode* node = contextNode;
	if(!contextNode || contextNode == browser.getTreeRoot ())
	{
		node = browser.getTreeRoot ();
		menu.setContextID (Browser::kTreeRootContext);
	}

	if(node)
	{
		if(contextNode && browser.canSetRoot ())
			menu.addCommandItem (XSTR (SetAsRoot), CSTR ("Browser"), CSTR ("Set as Root"), nullptr);

		AutoPtr<ObjectList> selectedNodes (NEW ObjectList);
		getSelectedNodes (*selectedNodes, selection, treeRoot);

		browser.appendNodeContextMenu (*node, menu, selectedNodes);

		// give extensions a chance
		browser.getExtender ().extendBrowserNodeMenu (node, menu, selectedNodes);

		if(node->appendContextMenu (menu, selectedNodes) == kResultOk) // todo: also pass selection...
			return true; // break
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserModelBase::appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection)
{
	BrowserNode* contextNode = resolveNode (item);
	return appendNodeContextMenu (menu, contextNode, &selection);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserModelBase::getSelectedNodes (Container& selectedNodes, const IItemSelection* selection, BrowserNode* alternativeNode)
{
	selectedNodes.objectCleanup (true);

	if(alternativeNode)
	{
		alternativeNode->retain ();
		selectedNodes.add (alternativeNode);
	}
	else if(selection)
	{
		ForEachItem (*selection, index)
			BrowserNode* node = resolveNode (index);
			if(node)
			{
				node->retain ();
				selectedNodes.add (node);
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserModelBase::interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection)
{
	BrowserNode* node = resolveNode (item);
	return browser.interpretNodeCommand (msg, *this, node, selection);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserModelBase::canRemoveItem (ItemIndexRef index)
{
	return false; // we handle the delete command directly
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserModelBase::canSelectItem (ItemIndexRef index)
{
	return browser.canSelectNode (resolveNode (index));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserModelBase::isItemFolder (ItemIndexRef index)
{
	BrowserNode* node = resolveNode (index);
	return node && node->isFolder ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserModelBase::canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView)
{
	return browser.canInsertData (resolveNode (index), data, session, targetView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserModelBase::insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session)
{
	browser.trackInteraction ();

	return browser.insertData (resolveNode (index), data, session);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BrowserModelBase::viewAttached (IItemView* itemView)
{
	ItemViewObserver<AbstractItemModel>::viewAttached (itemView);
	browser.onViewAttached (itemView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API BrowserModelBase::notify (ISubject* subject, MessageRef msg)
{
	browser.notify (subject, msg);
}

//************************************************************************************************
// BrowserTreeModel
//************************************************************************************************

BrowserTreeModel::BrowserTreeModel (Browser& browser, BrowserListModel& listModel)
: BrowserModelBase (browser),
  listModel (listModel)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* BrowserTreeModel::resolveNode (ItemIndexRef index) const
{
	return unknown_cast<BrowserNode> (index.getObject ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserTreeModel::getRootItem (ItemIndex& index)
{
	index = ItemIndex (ccl_as_unknown (browser.getTreeRoot ()));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserTreeModel::canExpandItem (ItemIndexRef index)
{
	BrowserNode* node = resolveNode (index);
	return node && browser.canExpandNode (*node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserTreeModel::canAutoExpandItem (ItemIndexRef index) 
{ 
	BrowserNode* node = resolveNode (index);
	return node && node->canAutoExpand ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserTreeModel::getSubItems (IUnknownList& items, ItemIndexRef index)
{
	BrowserNode* node = resolveNode (index);
	if(!node)
		return false;

	BrowserNode::NodeFlags nodeFlags;
	nodeFlags.wantFolders (true);
	if(browser.displayTreeLeafs ())
		nodeFlags.wantLeafs (true);

	ObjectList list;
	if(node == listModel.getParent ())
		listModel.collectNodes (list, nodeFlags);
	else
		node->getSubNodes (list, nodeFlags);

	ForEach (list, Object, object)
		items.add (object->asUnknown ());
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserTreeModel::onItemFocused (ItemIndexRef index)
{
	BrowserNode* node = resolveNode (index);

	if(!browser.isRefreshing)
	{
		if(node)
		{
			ObjectList childNodes;
			if(listModel.extractChildNodesForReuse (childNodes, node, index.getTreeItem ()))
			{
				listModel.setParent (node, &childNodes);
				browser.onNodeFocused (node, false);
				return true;
			}
		}

		BrowserNode* listParent = node;

		if(node && !node->isFolder ())
		{
			listParent = node->getParent ();

			if(listModel.getItemView ())
				if(BrowserNode* listNode = listModel.findNodeInstance (node))
					listModel.selectNode (listNode, true);
		}

		listModel.setParent (listParent);
	}

	browser.onNodeFocused (node, false);
	return true;
}

//************************************************************************************************
// BrowserListModel
//************************************************************************************************

BrowserListModel::BrowserListModel (Browser& browser)
: BrowserModelBase (browser),
  parent (nullptr),
  parentIcon (nullptr),
  parentOverlay (nullptr)
{
	parentChain.objectCleanup ();
	nodes.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserListModel::~BrowserListModel ()
{
	if(parent)
		parent->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserListModel::getNodeIndex (ItemIndex& index, const BrowserNode* node)
{
	int i = 0;
	ForEach (nodes, BrowserNode, n)
		if(n == node)
		{
			index = i;
			return true;
		}
		i++;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* BrowserListModel::findNode (const IRecognizer* recognizer) const
{
	ForEach (nodes, BrowserNode, n)
		if(recognizer->recognize (n->asUnknown ()))
			return n;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* BrowserListModel::findNodeInstance (BrowserNode* node) const
{
	ASSERT (node)
	if(node == nullptr)
		return nullptr;

	if(node->getParent () == parent)
	{
		if(nodes.contains (node))
			return node;

		// the list model might contain a different instance for that node than the tree, find it by name
		MutableCString nodeName;
		node->getUniqueName (nodeName);
		return findNode (AutoPtr<IRecognizer> (Recognizer::create ([&] (IUnknown* obj)
		{
			BrowserNode* node = unknown_cast<BrowserNode> (obj);
			MutableCString name;
			return node && node->getUniqueName (name) && name == nodeName;
		})));
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* BrowserListModel::getFocusNode (bool onlyIfSelected) const
{
	if(getItemView ())
	{
		ItemIndex listIndex;
		if(getItemView ()->getFocusItem (listIndex))
			if(!onlyIfSelected || getItemView ()->getSelection ().isSelected (listIndex))
				return resolveNode (listIndex);
 	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserListModel::selectNode (BrowserNode* node, bool exclusive)
{
	if(getItemView ())
	{
		if(exclusive)
			getItemView ()->selectAll (false);

		ItemIndex listIndex;
		if(getNodeIndex (listIndex, node))
		{
			getItemView ()->selectItem (listIndex, true);
			getItemView ()->setFocusItem (listIndex, false);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserListModel::checkAutoSelect ()
{
	// in list mode, ensure that a node in the list is selected
	if(browser.isListMode ())
		if(getItemView () && getItemView ()->getSelection ().isEmpty ())
		{
			if(parent && !previousParentPath.isEmpty ())
			{
				// when navigating upwards (to an ancestor node of the previous parent), select the subfolder we came from
				MutableCString parentPath;
				browser.makePath (parentPath, parent);
				parentPath += "/";
				if(previousParentPath.startsWith (parentPath))
				{
					MutableCString subFolderName (previousParentPath.subString (parentPath.length (), -1));
					int separatorIndex = subFolderName.index ('/');
					if(separatorIndex > 0)
						subFolderName.truncate (separatorIndex);

					parentPath += subFolderName;
					if(BrowserNode* subFolderNode = browser.findNode (parentPath, false))
					{
						selectNode (subFolderNode, true);
						return;
					}
				}
			}

			// fallback: select first node
			getItemView ()->setFocusItem (ItemIndex (0), true);
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserListModel::invalidateNode (BrowserNode* node)
{
	if(node == parent)
		updateParentIcon ();

	if(getItemView ())
	{
		ItemIndex listIndex;
		if(getNodeIndex (listIndex, node))
			getItemView ()->invalidateItem (listIndex);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* BrowserListModel::getParent () const
{
	return parent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserListModel::extractChildNodesForReuse (ObjectList& childNodes, BrowserNode* node, ITreeItem* treeItem)
{
	if(treeItem)
	{
		// if item was expanded already, we can reuse nodes from child tree items in list model
		UnknownList tempList;
		if(treeItem->getContent (tempList))
		{
			ObjectList missing;
			missing.objectCleanup (); // will be released when leaving scope

			ForEachUnknown (tempList, unk)
				childNodes.add (unknown_cast<Object> (unk));
			EndFor
			
			if(!browser.displayTreeLeafs ()) // must get missing leafs from node
			{
				node->getSubNodes (missing, BrowserNode::NodeFlags::kLeafs);
				childNodes.add (missing);
			}
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserListModel::setParent (BrowserNode* node, Container* oldNodes)
{
	if(parent != node)
	{
		CCL_PRINTF ("BrowserListModel::setParent %s (was %s)\n", node ? MutableCString (node->getTitle ()).str () : "", parent ? MutableCString (parent->getTitle ()).str () : "")

		previousParentPath.empty ();
		if(parent)
			browser.makePath (previousParentPath, parent);

		take_shared<BrowserNode> (parent, node);

		ObjectList oldParentChain;
		oldParentChain.objectCleanup (true);
		oldParentChain.add (parentChain, Container::kShare);

		parentChain.removeAll ();

		if(parent)
		{
			// share all ancestor nodes of the new parent node (prevent a dangling parent pointer when our parent's parent is destroyed)
			BrowserNode* p = parent->getParent ();
			while(p)
			{
				parentChain.prepend (return_shared (p));
				p = p->getParent ();
			}
		}

		nodes.removeAll ();
		if(oldNodes)
			nodes.add (*oldNodes, Container::kShare);
		else if(node)
			node->getSubNodes (nodes);

		updateParentIcon ();
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserListModel::updateParentIcon ()
{
	if(!parentIcon)
	{
		parentIcon = UnknownPtr<IImageProvider> (browser.paramList.byTag (Tag::kListParentIcon));
		parentOverlay = UnknownPtr<IImageProvider> (browser.paramList.byTag (Tag::kListParentOverlay));
	}

	IImage* icon = nullptr;
	AutoPtr<IImage> overlay;
	if(parent)
	{
		icon = parent->getIcon ();
		if(!icon)
			icon = FileIcons::instance ().getDefaultFolderIcon ();
		
		// draw icon overlay into bitmap
		UnknownPtr<IView> view (getItemView ());
		if(view)
		{
			const IVisualStyle& vs = view->getVisualStyle ();

			Coord iconW = vs.getMetric<Coord> ("listparent.icon.width", 32);
			Coord iconH = vs.getMetric<Coord> ("listparent.icon.height", 32);
			Rect iconSize (0, 0, iconW, iconH);

			IWindow* window = view->getIWindow ();
			float scaleFactor = window ? window->getContentScaleFactor () : 1.f;

			overlay = GraphicsFactory::createBitmap (iconW, iconH, IBitmap::kRGBAlpha, scaleFactor);
			{
				AutoPtr<IGraphics> graphics = GraphicsFactory::createBitmapGraphics (overlay);
				//graphics->clearRect (iconSize);

				// mimic what ItemViews do
				Font font (vs.getTextFont ());
				Brush textBrush (vs.getTextBrush ());
				Brush backBrush (vs.getBackBrush ());
				Color iconColor (vs.getColor ("iconcolor"));

				IItemModel::StyleInfo styleInfo = { font, textBrush, backBrush, iconColor };
				IItemModel::DrawInfo drawInfo = { view, *graphics, iconSize, styleInfo, 0 };
				parent->drawIconOverlay (drawInfo);
			}
		}
	}
	parentIcon->setImage (icon);
	parentOverlay->setImage (overlay);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserListModel::collectNodes (Container& result, BrowserNode::NodeFlags flags)
{
	if(flags.wantAll ())
		result.add (nodes, Container::kShare);
	else
	{
		ForEach (nodes, BrowserNode, node)
			if(flags.shouldAdd (node->isFolder ()))
			{
				result.add (node);
				node->retain ();
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void BrowserListModel::addNode (BrowserNode* node, int index)
{
	node->retain ();
	if(index >= 0)
		nodes.insertAt (index, node);
	else
		nodes.add (node);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool BrowserListModel::removeNode (BrowserNode* node)
{
	if(getItemView ())
		getItemView ()->selectItem (ItemIndex (node->asUnknown ()), false);

	if(nodes.remove (node))
	{
		node->release ();
		signal (Message (kChanged));
		return true;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BrowserNode* BrowserListModel::resolveNode (ItemIndexRef index) const
{
	return (BrowserNode*)nodes.at (index.getIndex ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API BrowserListModel::countFlatItems ()
{
	return nodes.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserListModel::onItemFocused (ItemIndexRef index)
{
	BrowserNode* node = resolveNode (index);
	Browser* browser = node ? node->getBrowser () : nullptr;
	if(browser)
	{
		browser->onNodeFocused (node, true);
		return true;
	}
	else
		return BrowserModelBase::onItemFocused (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserListModel::appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection)
{
	BrowserNode* contextNode = resolveNode (item);
	if(contextNode == nullptr)
	{
		contextNode = getParent ();
		menu.setContextID (Browser::kTreeRootContext);
	}
	else
		menu.setContextID (Browser::kChildrenHiddenContext);

	return appendNodeContextMenu (menu, contextNode, &selection);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API BrowserListModel::interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection)
{
	if(selection.isEmpty () && item.isValid () == false)
		if(BrowserNode* parent = getParent ())
			if(parent->interpretCommand (msg))
				return true;
	
	return SuperClass::interpretCommand (msg, item, selection);
}

//************************************************************************************************
// Browser::SearchResultList
//************************************************************************************************

Browser::SearchResultList::SearchResultList (Browser& browser)
: browser (browser)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::SearchResultList::showResultInContext (UrlRef url, bool checkOnly)
{
	if(BrowserNode* node = browser.findNodeWithUrl (url))
	{
		if(!checkOnly)
		{
			browser.expandNode (node);
			browser.setFocusNode (node, true);
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Browser::SearchResultList::appendResultContextMenu (IContextMenu& menu, UrlRef url)
{
	if(BrowserNode* node = browser.findNodeWithUrl (url))
	{
		ObjectArray selectedNodes;
		selectedNodes.add (node);
		return node->appendContextMenu (menu, &selectedNodes) == kResultTrue;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool Browser::SearchResultList::interpretResultCommand (const CommandMsg& msg, UrlRef url)
{
	if(BrowserNode* node = browser.findNodeWithUrl (url))
	{
		ObjectArray selectedNodes;
		selectedNodes.add (node);
		return node->interpretCommand (msg, &selectedNodes);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Browser::SearchResultList::onItemFocused (ItemIndexRef index)
{
	if(BrowserNode* node = ccl_cast<BrowserNode> (resolve (index)))
	{
		browser.onNodeFocused (node, true);
		return true;
	}
	return SuperClass::onItemFocused (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Browser::SearchResultList::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	if(BrowserNode* node = ccl_cast<BrowserNode> (resolve (index)))
	{
		StringID columnID = columns ? columns->getColumnID (column) : CString::kEmpty;
		return browser.onEditNode (*node, columnID, info);
	}
	return SuperClass::editCell (index, column, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Browser::SearchResultList::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	if(BrowserNode* node = ccl_cast<BrowserNode> (resolve (index)))
	{
		// try to open files via systemshell (not deferred: need the real result)
		if(node->onOpen (false))
			return true;
		
		// show in context as fallback (e.g. for folders)
		if(onShowResultInContext (index, false))
		{
			// in list mode: navigate inside found node if possible
			if(browser.isListMode ())
				if(BrowserNode* focusNode = browser.getFocusNode ())
					browser.openNode (focusNode);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Browser::SearchResultList::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IItemView::kDragSessionDone)
		browser.notify (subject, msg);
	else
		return SuperClass::notify (subject, msg);
}

