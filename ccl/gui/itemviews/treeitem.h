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
// Filename    : ccl/gui/itemviews/treeitem.h
// Description : Tree data model
//
//************************************************************************************************

#ifndef _ccl_treeitem_h
#define _ccl_treeitem_h

#include "ccl/base/collections/objectlist.h"

#include "ccl/public/base/irecognizer.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/gui/framework/iitemmodel.h"
#include "ccl/public/gui/graphics/itextlayout.h"
#include "ccl/public/gui/iviewstate.h"

namespace CCL {

class Container;
class Iterator;
class Tree;
class TreeItem;
class Image;

//************************************************************************************************
// TreeTraverser
//************************************************************************************************

class TreeTraverser
{
public:
	enum Mode 
	{ 
		kOnlyExpanded	= 1<<0,
		kHiddenRoot		= 1<<1,
		kNoIcons		= 1<<2
	};

	TreeTraverser (int mode = kOnlyExpanded);
	virtual ~TreeTraverser () {}

	virtual bool stepInto (TreeItem* item) const;
	virtual bool visit (TreeItem* item) = 0;

protected:
	friend class TreeItem;
	int mode;
	int currentIndex;
	int currentInset;
};

//************************************************************************************************
// TreeVisibleTraverser
/** Base class for traversers of only visible items. */
//************************************************************************************************

struct TreeVisibleTraverser: TreeTraverser
{
	TreeVisibleTraverser (Tree& tree);

	bool stepInto (TreeItem* item) const override;

protected:
	bool checkVisible (TreeItem* item);

	Tree& tree;	// for calling Tree::isItemVisible
};

//************************************************************************************************
// TreeItem
//************************************************************************************************

class TreeItem: public Object,
				public ITreeItem
{
public:
	DECLARE_CLASS (TreeItem, Object)

	TreeItem (StringRef title = nullptr);
	~TreeItem ();

	PROPERTY_VARIABLE (Coord, textWidth, TextWidth)
	PROPERTY_VARIABLE (Coord, height, Height)
	PROPERTY_AUTO_POINTER (ITextLayout, textLayout, TextLayout)
	PROPERTY_STRING (textLayoutString, TextLayoutString)

	ItemIndex asIndex () const;
	TreeItem* findByIndex (ItemIndexRef index, tbool onlyExpanded = true) const;

	TreeItem* getParent () const;
	TreeItem* getChild (StringID name, bool createItems) const;///< find a direct child by unique name
	bool hasAncestor (TreeItem* ancestor);
	virtual Tree* getTree () const;

	void addItem (TreeItem* item, int index = -1);
	void removeItem (TreeItem* item); ///< item gets released
	Iterator* newIterator () const;
	int countItems () const;

	bool traverse (TreeTraverser& t) const;

	TreeItem* getVisibleItem (int idx) const;
	bool getItemPosition (TreeItem* item, int& row, int& column) const;
	void countVisible (int& numRows, int& numColumns) const; 

	TreeItem* getNextVisible (bool onlyExpanded);
	TreeItem* getPreviousVisible (bool onlyExpanded);

	// ITreeItem
	IUnknown* CCL_API getData () const override;
	tbool CCL_API getContent (IUnknownList& list) const override;
	IUnknownIterator* CCL_API getContent () const override;
	void CCL_API createSubItems () override;
	int CCL_API getState () const override;
	ITreeItem* CCL_API getParentItem () const override;
	tbool CCL_API isEmpty () const override;
	ITreeItem* CCL_API findChild (IUnknown* data) const override;
	ITreeItem* CCL_API findItem (IUnknown* data, tbool onlyExpanded = true) const override;
	ITreeItem* CCL_API findItem (const IRecognizer* recognizer, tbool onlyExpanded = true) const override;
	ITreeItem* CCL_API findItem (StringID path, tbool createItems, tbool acceptAncestor = false) override;
	ITreeItem* CCL_API navigate (int rows, int flags) override;
	tbool CCL_API makePath (MutableCString& path, ITreeItem* relativeTo = nullptr) override;
	void CCL_API addSubItem (IUnknown* data, int index = -1) override;
	void CCL_API removeAll () override;
	void CCL_API remove () override;
	IViewStateHandler* CCL_API createExpandState () override;
	IViewStateHandler* CCL_API storeExpandState () override;
	tbool CCL_API restoreExpandState (IViewStateHandler* state) override;

	void setData (IUnknown* data);

	Image* getIcon ();
	Image* getThumbnail ();
	void setTitle (StringRef title);
	StringRef getTitle (String& title);
	
	virtual bool canExpand ();
	virtual bool onExpand ();
	bool canAutoExpand ();
	bool isFolder ();

	PROPERTY_FLAG (state, kIsExpanded, isExpanded)
	PROPERTY_FLAG (state, kIsSelected, isSelected)
	PROPERTY_FLAG (state, kWasExpanded, wasExpanded)

	void expand (bool state = true, bool deep = false);

	CLASS_INTERFACE (ITreeItem, Object)

	void absorbItem (TreeItem& item); ///< takes childs, data, state, title	

protected:
	TreeItem* parent;
	ObjectList* items;
	IUnknown* data;
	int state;
	String title;

	class DataIterator;

	bool checkIsFolder ();
	TreeItem* getNextVisible (bool deep, bool onlyExpanded, Tree& tree);
	TreeItem* findPreviousChildDeep (TreeItem* startItem, bool onlyExpanded, Tree& tree);
	bool makePath (MutableCString& path, ITreeItem* relativeTo, IItemModel& model);
};

//************************************************************************************************
// Tree
//************************************************************************************************

class Tree: public TreeItem,
			public ITree
{
public:
	DECLARE_CLASS (Tree, TreeItem)

	Tree (IItemModel* model = nullptr, StringRef title = nullptr);
	~Tree ();

	PROPERTY_POINTER (IItemModel, model, Model)

	void setItemFilter (IObjectFilter* filter);
	IObjectFilter* getItemFilter () const;

	Image* getItemIcon (TreeItem* item);
	Image* getItemThumbnail (TreeItem* item);
	bool getItemTitle (String& title, TreeItem* item);
	bool isItemFolder (TreeItem* item);
	bool canExpandItem (TreeItem* item);
	bool canAutoExpandItem (TreeItem* item);
	bool onExpandItem (TreeItem* item);
	tbool isItemVisible (TreeItem* item);

	// ITree
	void CCL_API setTreeModel (IItemModel* model) override;
	void CCL_API setRootItem (IUnknown* data) override;
	virtual ITreeItem* CCL_API getRootItem () override;

	// TreeItem
	Tree* getTree () const override;

	CLASS_INTERFACE (ITree, TreeItem)

private:
	AutoPtr<IObjectFilter> itemFilter;
};

//************************************************************************************************
// TreeItemState
//************************************************************************************************

class TreeItemState: public Object,
					 public IViewStateHandler
{
public:
	DECLARE_CLASS (TreeItemState, Object)

	TreeItemState ();

	bool store   (TreeItem& item);
	bool restore (TreeItem& item);

	PROPERTY_MUTABLE_CSTRING (name, Name)
	PROPERTY_BOOL (expanded, Expanded)

	// IViewStateHandler
	tbool CCL_API saveViewState (StringID viewID, StringID viewName, IAttributeList& attributes, const IViewState* state) const override;
	tbool CCL_API loadViewState (StringID viewID, StringID viewName, const IAttributeList& attributes, IViewState* state) override;

	CLASS_INTERFACE (IViewStateHandler, Object)

private:
	bool store   (TreeItem& item, IItemModel& model);
	bool restore (TreeItem& item, IItemModel& model);

	ObjectList subStates;
};

} // namespace CCL

#endif // _ccl_treeitem_h
