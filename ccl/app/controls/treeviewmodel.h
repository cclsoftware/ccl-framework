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
// Filename    : ccl/app/controls/treeviewmodel.h
// Description : Tree View Model
//
//************************************************************************************************

#ifndef _ccl_treeviewmodel_h
#define _ccl_treeviewmodel_h

#include "ccl/app/controls/listviewmodel.h"
#include "ccl/app/controls/treeviewnode.h"

namespace CCL {

//************************************************************************************************
// TreeViewModel
//************************************************************************************************

class TreeViewModel: public ListViewModelBase
{
public:
	DECLARE_CLASS (TreeViewModel, ListViewModelBase)

	TreeViewModel ();
	~TreeViewModel ();

	PROPERTY_SHARED_AUTO (TreeViewNode, rootNode, RootNode)

	TreeViewNode* resolveNode (ItemIndexRef index) const;
	ITreeView* getTreeView () const;

	// Helper methods related to attached TreeView:
	void redrawNode (TreeViewNode* node);
	void refreshNode (TreeViewNode* node);
	void expandNode (TreeViewNode* node);
	void makeNodeVisible (TreeViewNode* node);
	void setFocusNode (TreeViewNode* node);
	TreeViewNode* getFocusNode () const;

	// ListViewModelBase
	tbool CCL_API getRootItem (ItemIndex& index) override;
	int CCL_API countFlatItems () override;
	tbool CCL_API isItemFolder (ItemIndexRef index) override;
	tbool CCL_API canExpandItem (ItemIndexRef index) override;
	tbool CCL_API canAutoExpandItem (ItemIndexRef index) override;
	tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index) override;
	void CCL_API signal (MessageRef msg) override;
	
	IItemModel* getListViewAdapter (); ///< provides an model for presenting the contents of the root folder in a ListView

protected:
	class ListViewAdapter;
	ListViewAdapter* listViewAdapter;

	TreeViewFolderNode* getRootFolderNode () const;

	// ListViewModelBase
	ListViewItem* resolve (ItemIndexRef index) const override { return resolveNode (index); }
	bool getIndex (ItemIndex& index, const ListViewItem* item) const override;
	bool visitItemsInternal (ViewItemVisitor& itemVisitor) override;
};

} // namespace CCL

#endif // _ccl_treeviewmodel_h
