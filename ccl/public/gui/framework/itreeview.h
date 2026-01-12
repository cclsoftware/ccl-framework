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
// Filename    : ccl/public/gui/framework/itreeview.h
// Description : Tree View Interface
//
//************************************************************************************************

#ifndef _ccl_itreeview_h
#define _ccl_itreeview_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

class MutableCString;
interface IUnknownList;
interface IRecognizer;
interface IObjectFilter;
interface IViewStateHandler;
interface IItemModel;
interface IUnknownIterator;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** Tree object for TreeView (supports ITreeItem). */
	DEFINE_CID (Tree, 0xFF1CCF29, 0x0AD3, 0x4D48, 0xA9, 0x67, 0x72, 0x71, 0x8B, 0xAD, 0x31, 0xB0)
}

//************************************************************************************************
// ITreeItem
/** Tree item interface. */
//************************************************************************************************

interface ITreeItem: IUnknown
{
	enum States // state flags
	{
		kIsExpanded		= 1<<0,
		kIsSelected		= 1<<1,
		kWasExpanded	= 1<<2,
		kIsFolder		= 1<<3,
		kIsLeaf			= 1<<4
	};

	/** Get associated data. */
	virtual IUnknown* CCL_API getData () const = 0;

	/** Get data of existing subitems. */
	virtual tbool CCL_API getContent (IUnknownList& list) const = 0;

	/** Get data of existing subitems. */
	virtual IUnknownIterator* CCL_API getContent () const = 0;

	/** Ensure that subItems were created. */
	virtual void CCL_API createSubItems () = 0;

	/** Get state flags. */
	virtual int CCL_API getState () const = 0;

	/** Get parent item. */
	virtual ITreeItem* CCL_API getParentItem () const = 0;

	/** Check if the item is empty (has no no childs). */
	virtual tbool CCL_API isEmpty () const = 0;

	/** Find a direct child of this with the given data. */
	virtual ITreeItem* CCL_API findChild (IUnknown* data) const = 0;

	/** Find a subitem (including this) with the given data. */
	virtual ITreeItem* CCL_API findItem (IUnknown* data, tbool onlyExpanded = true) const = 0;

	/** Find a subitem (including this) using a recognizer. */
	virtual ITreeItem* CCL_API findItem (const IRecognizer* recognizer, tbool onlyExpanded = true) const = 0;

	/** Find a subitem by path. */
	virtual ITreeItem* CCL_API findItem (StringID path, tbool createItems, tbool acceptAncestor = false) = 0;

	/** Create a path to the item, optionally starting from item relativeTo. */
	virtual tbool CCL_API makePath (MutableCString& path, ITreeItem* relativeTo = nullptr) = 0;

	enum NavigateFlags { kOnlySelectable = 1<<0, kOnlyExpanded = 1<<1, kIgnoreRoot = 1<<2 };

	/** Get a next / previous item specicified by rows. */
	virtual ITreeItem* CCL_API navigate (int rows, int flags) = 0;

	/** Add a new subitem. */
	virtual void CCL_API addSubItem (IUnknown* data, int index = -1) = 0;

	/** Remove all subitems and collapse this. */
	virtual void CCL_API removeAll () = 0;

	/** Remove the item from it's parent. */
	virtual void CCL_API remove () = 0;

	/** Create an empty expansion state. */
	virtual IViewStateHandler* CCL_API createExpandState () = 0;

	/** Store expansion states starting with this item. */
	virtual IViewStateHandler* CCL_API storeExpandState () = 0;

	/** Restore expansion states starting with this item. */
	virtual tbool CCL_API restoreExpandState (IViewStateHandler* state) = 0;

	DECLARE_IID (ITreeItem)
};

DEFINE_IID (ITreeItem, 0x18b2842e, 0x11de, 0x4e3b, 0xb9, 0x59, 0x8e, 0xef, 0xd5, 0x48, 0x54, 0x80)

//************************************************************************************************
// ITree
//************************************************************************************************

interface ITree: IUnknown
{
	/** Set tree item model. */
	virtual void CCL_API setTreeModel (IItemModel* model) = 0;

	/** Set data object of root item. */
	virtual void CCL_API setRootItem (IUnknown* data) = 0;

	/** Get root tree item. */
	virtual ITreeItem* CCL_API getRootItem () = 0;

	DECLARE_IID (ITree)
};

DEFINE_IID (ITree, 0xE966B291, 0xE37C, 0x424F, 0xAD, 0x56, 0xB2, 0x81, 0x3B, 0x0B, 0xB9, 0x45)

//************************************************************************************************
// ITreeView
//************************************************************************************************

interface ITreeView: IUnknown
{
	enum ExpandModes // expand modes
	{
		kExpandParents		= 1<<0,	///< also expand all parent items recursively
		kExpandChilds		= 1<<1,	///< also expand all child items recursively
		kCheckCanAutoExpand	= 1<<2	///< don't expand items that can't autoExpand
	};

	/** Get the root item. */
	virtual ITreeItem* CCL_API getRootItem () = 0;

	/** Expand. */
	virtual void CCL_API expandItem (ITreeItem* item, tbool state = true, int expandMode = 0) = 0;

	/** Refresh (drop subItems). */
	virtual void CCL_API refreshItem (ITreeItem* item) = 0;

	/** Set a filter that tells if an item is visible. */
	virtual void CCL_API setItemFilter (IObjectFilter* filter) = 0;

	/** Set a new Tree object (shared, a Tree can be created via ccl_new, see ClassID below). */
	virtual void CCL_API setTree (ITree* tree) = 0;

	/** Thumbnails have changed. */
	virtual void CCL_API updateThumbnails () = 0;

	/** Get text distance to left side of item rect */
	virtual int CCL_API getItemTextInset (ITreeItem* item) = 0;

	DECLARE_STRINGID_MEMBER (kItemExpanded) ///< arg[0]: ITreeItem; arg[1]: state (bool)

	DECLARE_IID (ITreeView)
};

DEFINE_IID (ITreeView, 0x2A2FBC77, 0xCC72, 0x4DD8, 0xBE, 0x8A, 0x27, 0x6C, 0x1F, 0x54, 0x86, 0x35)
DEFINE_STRINGID_MEMBER (ITreeView, kItemExpanded, "itemExpanded")

} // namespace CCL

#endif // _ccl_itreeview_h
