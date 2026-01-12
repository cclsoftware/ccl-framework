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
// Filename    : ccl/app/controls/treeviewmodel.cpp
// Description : Tree View Model
//
//************************************************************************************************

#include "ccl/app/controls/treeviewmodel.h"

#include "ccl/base/collections/objectlist.h"

using namespace CCL;

//************************************************************************************************
// TreeViewModel::ListViewAdapter
/** Translates contents of TreeViewModel root folder for presenting in a ListView. */
//************************************************************************************************

class TreeViewModel::ListViewAdapter: public Object,
									  public IItemModel
{
public:
	ListViewAdapter (TreeViewModel& treeModel)
	: treeModel (treeModel)
	{
		rootFolder.share (treeModel.getRootFolderNode ());
		if(!rootFolder)
			rootFolder = NEW TreeViewFolderNode; // empty folder as fallback to avoid null pointer checks
	}

	const ObjectArray& getFlatItems () { return rootFolder->getContent (); }

	ItemIndex makeTreeIndex (ItemIndexRef index)
	{
		Object* item = getFlatItems ().at (index.getIndex ());
		return ItemIndex (ccl_as_unknown (item));
	}

	ItemIndex makeListIndex (const ListViewItem* item)
	{
		return ItemIndex (getFlatItems ().index (item));
	}

	// IItemModel
	void CCL_API viewAttached (IItemView* itemView) override { treeModel.viewAttached (itemView); }
	void CCL_API viewDetached (IItemView* itemView) override { treeModel.viewDetached (itemView);}
	int CCL_API countFlatItems () override { return getFlatItems ().count (); }
	tbool CCL_API getRootItem (ItemIndex& index) override { ASSERT (0) return treeModel.getRootItem (index);	}
	tbool CCL_API isItemFolder (ItemIndexRef index) override { ASSERT (0) return treeModel.isItemFolder (makeTreeIndex (index));	}
	tbool CCL_API canExpandItem (ItemIndexRef index) override { ASSERT (0) return treeModel.canExpandItem (makeTreeIndex (index)); }
	tbool CCL_API canAutoExpandItem (ItemIndexRef index) override { ASSERT (0) return treeModel.canAutoExpandItem (makeTreeIndex (index)); }
	tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index) override { ASSERT (0) return treeModel.getSubItems (items, makeTreeIndex (index)); }
	IItemSelection* CCL_API getSelection () override { return treeModel.getSelection (); }
	tbool CCL_API getItemTitle (String& title, ItemIndexRef index) override { return treeModel.getItemTitle (title, makeTreeIndex (index)); }
	tbool CCL_API getUniqueItemName (MutableCString& name, ItemIndexRef index) override { return treeModel.getUniqueItemName (name, makeTreeIndex (index)); }
	IImage* CCL_API getItemIcon (ItemIndexRef index) override { return treeModel.getItemIcon (makeTreeIndex (index)); }
	IImage* CCL_API getItemThumbnail (ItemIndexRef index) override {	return treeModel.getItemThumbnail (makeTreeIndex (index)); }
	tbool CCL_API getItemTooltip (String& tooltip, ItemIndexRef index, int column) override { return treeModel.getItemTooltip (tooltip, makeTreeIndex (index), column); }
	tbool CCL_API canSelectItem (ItemIndexRef index) override { return treeModel.canSelectItem (makeTreeIndex (index)); }
	tbool CCL_API onItemFocused (ItemIndexRef index) override { return treeModel.onItemFocused (makeTreeIndex (index)); }
	tbool CCL_API openItem (ItemIndexRef index, int column, const EditInfo& info) override { return treeModel.openItem (makeTreeIndex (index), column, info);}
	tbool CCL_API canRemoveItem (ItemIndexRef index) override { return treeModel.canRemoveItem (makeTreeIndex (index)); }
	tbool CCL_API removeItem (ItemIndexRef index) override {	return treeModel.removeItem (makeTreeIndex (index)); }
	tbool CCL_API canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView = nullptr) override { return treeModel.canInsertData (makeTreeIndex (index), column, data, session, targetView); }
	tbool CCL_API insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session) override { return treeModel.insertData (makeTreeIndex (index), column, data, session); }
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override { return treeModel.editCell (makeTreeIndex (index), column, info); }
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override { return treeModel.drawCell (makeTreeIndex (index), column, info); }
	tbool CCL_API drawItem (ItemIndexRef index, const DrawInfo& info) override { return treeModel.drawItem (makeTreeIndex (index), info); }
	tbool CCL_API drawIconOverlay (ItemIndexRef index, const DrawInfo& info) override { return treeModel.drawIconOverlay (makeTreeIndex (index), info); }
	StringID CCL_API getItemBackground (ItemIndexRef index) override { return treeModel.getItemBackground (makeTreeIndex (index)); }
	tbool CCL_API measureCellContent (Rect& size, ItemIndexRef index, int column, const StyleInfo& info) override { return treeModel.measureCellContent (size, makeTreeIndex (index), column, info);	}
	tbool CCL_API createColumnHeaders (IColumnHeaderList& list) override { return treeModel.createColumnHeaders (list); }
	tbool CCL_API getSortColumnID (MutableCString& columnID, tbool& upwards) override { return treeModel.getSortColumnID (columnID, upwards); }
	IUnknown* CCL_API createDragSessionData (ItemIndexRef index) override { return treeModel.createDragSessionData (makeTreeIndex (index)); }
	tbool CCL_API appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection) override { return treeModel.appendItemMenu (menu, makeTreeIndex (item), selection); }
	tbool CCL_API interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection) override { return treeModel.interpretCommand (msg, makeTreeIndex (item), selection); }
	ITouchHandler* CCL_API createTouchHandler (ItemIndexRef index, int column, const EditInfo& info) override { return treeModel.createTouchHandler (makeTreeIndex (index), column, info); }
	tbool CCL_API getItemAccessibilityInfo (AccessibilityInfo& info, ItemIndexRef index, int column) const override { return treeModel.getItemAccessibilityInfo (info, index, column); }

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override { treeModel.notify (subject, msg); }

	CLASS_INTERFACE (IItemModel, Object)

protected:
	TreeViewModel& treeModel;
	AutoPtr<TreeViewFolderNode> rootFolder;
};

//************************************************************************************************
// TreeViewModel
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TreeViewModel, ListViewModelBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeViewModel::TreeViewModel ()
: listViewAdapter (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeViewModel::~TreeViewModel ()
{
	safe_release (listViewAdapter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITreeView* TreeViewModel::getTreeView () const
{
	return UnknownPtr<ITreeView> (getItemView ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeViewFolderNode* TreeViewModel::getRootFolderNode () const
{
	return ccl_cast<TreeViewFolderNode> (getRootNode ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemModel* TreeViewModel::getListViewAdapter ()
{
	if(!listViewAdapter)
		listViewAdapter = NEW ListViewAdapter (*this);
	return listViewAdapter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TreeViewModel::signal (MessageRef msg)
{
	SuperClass::signal (msg);
	if(listViewAdapter)
		listViewAdapter->signal (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeViewNode* TreeViewModel::resolveNode (ItemIndexRef index) const
{
	auto node = unknown_cast<TreeViewNode> (index.getObject ());

	if(!node && listViewAdapter && UnknownPtr<IListView> (getItemView ()).isValid ())
		node = ccl_cast<TreeViewNode> (listViewAdapter->getFlatItems ().at (index.getIndex ()));

	return node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeViewModel::getIndex (ItemIndex& index, const ListViewItem* item) const
{
	if(ITreeView* treeView = getTreeView ())
	{
		if(ITreeItem* rootItem = treeView->getRootItem ())
			if(ITreeItem* treeItem = rootItem->findItem (ccl_as_unknown (item), false))
			{
				index = ItemIndex (treeItem);
				return true;
			}
	}
	else if(UnknownPtr<IListView> listView = getItemView ())
	{
		if(listViewAdapter)
		{
			index = listViewAdapter->makeListIndex (item);
			return index.isValid ();
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeViewModel::visitItemsInternal (ViewItemVisitor& itemVisitor)
{
	if(UnknownPtr<ITreeView> treeView = getItemView ())
	{
		if(ITreeItem* rootItem = treeView->getRootItem ())
		{
			AutoPtr<IRecognizer> r (Recognizer::create ([&] (IUnknown* data)
			{
				auto node = unknown_cast<TreeViewNode> (data);
				return !node || !itemVisitor.visit (*node);
			}));

			return rootItem->findItem (r, false);
		}
	}
	else if(UnknownPtr<IListView> listView = getItemView ())
	{
		if(listViewAdapter)
		{
			for(auto n : listViewAdapter->getFlatItems ())
				if(auto node = ccl_cast<ListViewItem> (n))
					if(!itemVisitor.visit (*node))
						return false;

			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeViewModel::redrawNode (TreeViewNode* node)
{
	ASSERT (node != nullptr)
	if(node == nullptr)
		return;

	if(UnknownPtr<ITreeView> treeView = getItemView ())
		if(ITreeItem* rootItem = treeView->getRootItem ())
			if(ITreeItem* treeItem = rootItem->findItem (node->asUnknown (), false))
				getItemView ()->invalidateItem (ItemIndex (treeItem));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeViewModel::refreshNode (TreeViewNode* node)
{
	ASSERT (node != nullptr)
	if(node == nullptr)
		return;

	if(UnknownPtr<ITreeView> treeView = getItemView ())
		if(ITreeItem* rootItem = treeView->getRootItem ())
			if(ITreeItem* treeItem = rootItem->findItem (node->asUnknown (), false))
				treeView->refreshItem (treeItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeViewModel::expandNode (TreeViewNode* node)
{
	ASSERT (node != nullptr)
	if(node == nullptr)
		return;

	if(UnknownPtr<ITreeView> treeView = getItemView ())
		if(ITreeItem* rootItem = treeView->getRootItem ())
			if(ITreeItem* treeItem = rootItem->findItem (node->asUnknown (), false))
				treeView->expandItem (treeItem, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeViewModel::makeNodeVisible (TreeViewNode* node)
{
	ASSERT (node != nullptr)
	if(node == nullptr)
		return;

	if(UnknownPtr<ITreeView> treeView = getItemView ())
		if(ITreeItem* rootItem = treeView->getRootItem ())
			if(ITreeItem* treeItem = rootItem->findItem (node->asUnknown (), false))
				UnknownPtr<IItemView> (treeView)->makeItemVisible (treeItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeViewModel::setFocusNode (TreeViewNode* node)
{
	ASSERT (node != nullptr)
	if(node == nullptr)
		return;

	if(UnknownPtr<ITreeView> treeView = getItemView ())
		if(ITreeItem* rootItem = treeView->getRootItem ())
			if(ITreeItem* treeItem = rootItem->findItem (node->asUnknown (), false))
				UnknownPtr<IItemView> (treeView)->setFocusItem (treeItem, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeViewNode* TreeViewModel::getFocusNode () const
{
	if(auto itemView = const_cast<TreeViewModel*> (this)->getItemView ())
	{
		ItemIndex focusItem;
		if(itemView->getFocusItem (focusItem))
			if(ITreeItem* treeItem = focusItem.getTreeItem ())
			{
				TreeViewNode* node = unknown_cast<TreeViewNode> (treeItem->getData ());
				return node;
			}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeViewModel::getRootItem (ItemIndex& index)
{
	ASSERT (rootNode != nullptr)
	if(rootNode == nullptr)
		return false;

	index = ItemIndex (ccl_as_unknown (rootNode));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TreeViewModel::countFlatItems ()
{
	TreeViewFolderNode* rootFolder = getRootFolderNode ();
	return rootFolder ? rootFolder->getContent ().count () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeViewModel::isItemFolder (ItemIndexRef index)
{
	TreeViewNode* node = resolveNode (index);
	return node && node->isFolder ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeViewModel::canExpandItem (ItemIndexRef index)
{
	TreeViewNode* node = resolveNode (index);
	return node && node->hasSubNodes ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeViewModel::canAutoExpandItem (ItemIndexRef index)
{
	TreeViewNode* node = resolveNode (index);
	return node && node->canAutoExpand ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeViewModel::getSubItems (IUnknownList& items, ItemIndexRef index)
{
	TreeViewNode* node = resolveNode (index);
	if(!node)
		return false;

	ObjectList list;
	node->getSubNodes (list, TreeViewNode::NodeFlags::kAll);
	ForEach (list, Object, object)
		items.add (object->asUnknown ());
	EndFor
	return true;
}
