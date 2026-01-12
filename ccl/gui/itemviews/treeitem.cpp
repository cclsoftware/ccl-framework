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
// Filename    : ccl/gui/itemviews/treeitem.cpp
// Description : Tree data model
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/itemviews/treeitem.h"

#include "ccl/base/collections/objectlist.h"
#include "ccl/base/message.h"

#include "ccl/gui/graphics/imaging/image.h"

#include "ccl/public/base/irecognizer.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/storage/iattributelist.h"

#define SAVE_ONLY_VISIBLE_STATES 1 // when saving a TreeItemState, ignore the whole subtree of a collapsed item

namespace CCL {

//************************************************************************************************
// Tree Traverser
//************************************************************************************************

struct TreeFindItem: TreeVisibleTraverser
{
	int searchIndex;
	TreeItem* result;

	TreeFindItem (int searchIndex, Tree& tree)
	: TreeVisibleTraverser (tree),
	  searchIndex (searchIndex),
	  result (nullptr)
	{}

	bool visit (TreeItem* item) override
	{
		if(!checkVisible (item))
			return true;

		if(currentIndex != searchIndex)
			return true;

		result = item;
		return false;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct TreeFindPosition: TreeVisibleTraverser
{
	TreeItem* searchItem;
	int row;
	int column;

	TreeFindPosition (TreeItem* searchItem, Tree& tree)
	: TreeVisibleTraverser (tree),
	  searchItem (searchItem),
	  row (-1),
	  column (-1)
	{}

	bool visit (TreeItem* item) override
	{
		if(!checkVisible (item))
			return true;

		if(item != searchItem)
			return true;

		row = currentIndex;
		column = currentInset;
		return false;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct TreeCountVisible: TreeVisibleTraverser
{
	int numRows;
	int numColumns;

	TreeCountVisible (Tree& tree)
	: TreeVisibleTraverser (tree),
	  numRows (0),
	  numColumns (0)
	{}

	bool visit (TreeItem* item) override
	{
		if(checkVisible (item))
		{
			ccl_lower_limit (numRows, currentIndex);
			ccl_lower_limit (numColumns, currentInset);
		}
		return true;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct TreeFindByItemIndex: TreeTraverser
{
	IUnknown* object;
	TreeItem* result;

	TreeFindByItemIndex (ItemIndexRef index, int mode)
	: TreeTraverser (mode),
	  object (index.getObject ()),
	  result (nullptr)
	{}

	bool visit (TreeItem* item) override
	{
		if(isEqualUnknown (item->getData (), object))
		{
			result = item;
			return false;
		}
		return true;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct TreeFindByRecognizer: TreeTraverser
{
	const IRecognizer* recognizer;
	TreeItem* result;

	TreeFindByRecognizer (const IRecognizer* recognizer, int mode)
	: TreeTraverser (mode),
	  recognizer (recognizer),
	  result (nullptr)
	{}

	bool visit (TreeItem* item) override
	{
		if(recognizer->recognize (item->getData ()))
		{
			result = item;
			return false;
		}
		return true;
	}
};

//************************************************************************************************
// TreeTraverser
//************************************************************************************************

TreeTraverser::TreeTraverser (int mode)
: mode (mode),
  currentIndex (0),
  currentInset (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeTraverser::stepInto (TreeItem* item) const
{
	if(mode & kOnlyExpanded)
		return item->isExpanded ();
	return true;
}

//************************************************************************************************
// TreeVisibleTraverser
//************************************************************************************************

TreeVisibleTraverser::TreeVisibleTraverser (Tree& tree)
: tree (tree)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeVisibleTraverser::stepInto (TreeItem* item) const
{
	return tree.isItemVisible (item) ? TreeTraverser::stepInto (item) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeVisibleTraverser::checkVisible (TreeItem* item)
{
	if(tree.isItemVisible (item) && !(item == &tree && (mode & kHiddenRoot)))
		return true;

	currentIndex--;
	return false;
}

//************************************************************************************************
// ItemNavigator
/** Helper for navigating through tree items. */
//************************************************************************************************

template<int direction>
struct ItemNavigator
{
public:
	ItemNavigator (IItemModel* model, int flags)
	: model (model),
	  onlySelectable ((flags & ITreeItem::kOnlySelectable) != 0),
	  onlyExpanded   ((flags & ITreeItem::kOnlyExpanded) != 0),
	  ignoreRoot     ((flags & ITreeItem::kIgnoreRoot) != 0)
	{}

	TreeItem* navigate (TreeItem* startItem, int rows);

private:
	struct DirTraits;

	IItemModel* model;
	bool onlySelectable;
	bool onlyExpanded;
	bool ignoreRoot;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
struct ItemNavigator<1>::DirTraits
{
	static inline TreeItem* getNextVisible (TreeItem* item, bool onlyExpanded)	{ return item->getNextVisible (onlyExpanded); }
	static inline bool endReached (TreeItem* item, bool ignoreRoot)				{ return item == nullptr; }
};

template<>
struct ItemNavigator<-1>::DirTraits
{
	static inline TreeItem* getNextVisible (TreeItem* item, bool onlyExpanded)	{ return item->getPreviousVisible (onlyExpanded); }
	static inline bool endReached (TreeItem* item, bool ignoreRoot)				{ return item == nullptr || (ignoreRoot && !item->getParent ()); }
};

//************************************************************************************************
// ItemNavigator
//************************************************************************************************

template<int direction>
TreeItem* ItemNavigator<direction>::navigate (TreeItem* startItem, int rows)
{
	typedef DirTraits Traits;

	TreeItem* result = startItem;
	TreeItem* item   = startItem;
	for(int i = 0; i < ccl_abs (rows); i++)
	{
		if(Traits::endReached (item = Traits::getNextVisible (item, onlyExpanded), ignoreRoot))
			break;
		else if(!onlySelectable || model->canSelectItem (item->asIndex ()))
			result = item;
	}
	if(result == startItem && item)
	{
		while(!Traits::endReached (item = Traits::getNextVisible (item, onlyExpanded), ignoreRoot))
			if(!onlySelectable || model->canSelectItem (item->asIndex ()))
			{
				result = item;
				break;
			}
	}
	return result;
}

//************************************************************************************************
// TreeItem::DataIterator
//************************************************************************************************

class TreeItem::DataIterator: public ObjectListIterator
{
public:
	DataIterator (const ObjectList& items)
	: ObjectListIterator (items)
	{}

	IUnknown* CCL_API nextUnknown () override
	{
		TreeItem* item = ccl_cast<TreeItem> (next ());
		return item ? item->getData () : nullptr;
	}
};

//************************************************************************************************
// TreeItem
//************************************************************************************************

DEFINE_CLASS (TreeItem, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeItem::TreeItem (StringRef title)
: parent (nullptr),
  data (nullptr),
  state (0),
  items (nullptr),
  title (title),
  textWidth (-1),
  height (-1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeItem::~TreeItem ()
{
	if(items)
		items->release ();
	
	if(data)
		data->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeItem::absorbItem (TreeItem& item)
{
	AutoPtr<ObjectList> oldItems (items);
	AutoPtr<IUnknown>   oldData (data);

	items = item.items;
	data  = item.data;
	state = item.state;
	title = item.title;

	item.items = nullptr;
	item.data = nullptr;

	if(items)
		ListForEachObject (*items, TreeItem, child)
			child->parent = this;
		EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemIndex TreeItem::asIndex () const
{
	TreeItem* This = const_cast<TreeItem*> (this);
	return ItemIndex (This);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TreeItem::createSubItems ()
{
	if(!wasExpanded ())
	{
		ASSERT (!isExpanded ())
		expand (true);
		expand (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeItem* TreeItem::getChild (StringID name, bool createItems) const
{
	Tree* tree = getTree ();
	if(IItemModel* model = tree ? tree->getModel () : nullptr)
	{
		if(createItems)
			const_cast<TreeItem*> (this)->createSubItems ();

		if(items)
		{
			ListForEachObject (*items, TreeItem, item)
				MutableCString n;
				model->getUniqueItemName (n, item);
				if(name == n)
					return item;
			EndFor
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeItem* TreeItem::findByIndex (ItemIndexRef index, tbool onlyExpanded) const
{
	if(index.getTreeItem ())
		return unknown_cast<TreeItem> (index.getTreeItem ());

	if(!index.getObject ())
		return nullptr;

	TreeFindByItemIndex t (index, onlyExpanded ? TreeFindByItemIndex::kOnlyExpanded : 0);
	traverse (t);
	return t.result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITreeItem* CCL_API TreeItem::findChild (IUnknown* data) const
{
	if(items)
		ListForEachObject (*items, TreeItem, child)
			if(child->getData () == data)
				return child;
		EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITreeItem* CCL_API TreeItem::findItem (IUnknown* data, tbool onlyExpanded) const
{
	return findByIndex (ItemIndex (data), onlyExpanded);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITreeItem* CCL_API TreeItem::findItem (const IRecognizer* recognizer, tbool onlyExpanded) const
{
	TreeFindByRecognizer t (recognizer, onlyExpanded ? TreeFindByItemIndex::kOnlyExpanded : 0);
	traverse (t);
	return t.result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeItem::makePath (MutableCString& path, ITreeItem* relativeTo)
{
	Tree* tree = getTree ();
	if(IItemModel* model = tree ? tree->getModel () : nullptr)
	{
		makePath (path, relativeTo, *model);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeItem::makePath (MutableCString& path, ITreeItem* relativeTo, IItemModel& model)
{
	if(this != relativeTo)
	{
		if(parent && parent->makePath (path, relativeTo, model))
			path.append ("/");

		MutableCString name;
		model.getUniqueItemName (name, this);
		path.append (name);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITreeItem* CCL_API TreeItem::findItem (StringID path, tbool createItems, tbool acceptAncestor)
{
	if(path.isEmpty ())
		return this;
	else
	{
		int index = path.index ("/");
		MutableCString name (index != 0 ? path.subString (0, index) : nullptr);
	
		TreeItem* child = getChild (name, createItems != 0);
		if(child)
		{
			if(index < 0)
				return child;
			else
			{
				CString remainder (path + index + 1);
				return child->findItem (remainder, createItems, acceptAncestor);
			}
		}
		return acceptAncestor ? this : nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Tree* TreeItem::getTree () const
{
	return parent ? parent->getTree () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeItem::hasAncestor (TreeItem* ancestor)
{
	// check if ancestor is a (grand)parent of this
	TreeItem* parent = getParent ();
	while(parent)
	{
		if(parent == ancestor)
			return true;
		parent = parent->getParent ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeItem::addItem (TreeItem* item, int index)
{
	ASSERT (item != nullptr && item->parent == nullptr)
	if(!items)
	{
		items = NEW ObjectList;
		items->objectCleanup ();
	}
	item->parent = this;

	if(index >= 0)
		items->insertAt (index, item);
	else
		items->add (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TreeItem::addSubItem (IUnknown* data, int index)
{
	TreeItem* item = NEW TreeItem;
	item->setData (data);
	addItem (item, index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeItem::removeItem (TreeItem* item)
{
	ASSERT (item != nullptr && item->parent == this)
	if(items)
	{
		item->parent = nullptr;
		items->remove (item);
		item->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TreeItem::remove ()
{
	if(parent)
		parent->removeItem (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* TreeItem::newIterator () const
{
	return items ? items->newIterator () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TreeItem::countItems () const
{
	return items ? items->count () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeItem::isEmpty () const
{
	return !items || items->isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TreeItem::removeAll ()
{
	if(items)
		items->removeAll ();
	
	isExpanded (false);
	wasExpanded (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITreeItem* CCL_API TreeItem::navigate (int rows, int flags)
{
	Tree* tree = getTree ();
	IItemModel* model = tree ? tree->getModel () : nullptr;
	if(!model)
		return nullptr;

	if(rows > 0)
		return ItemNavigator<1> (model, flags).navigate (this, rows);
	else
		return ItemNavigator<-1> (model, flags).navigate (this, rows);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeItem* TreeItem::getNextVisible (bool onlyExpanded)
{
	return getNextVisible (true, onlyExpanded, *getTree ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeItem* TreeItem::getNextVisible (bool deep, bool onlyExpanded, Tree& tree)
{
	if(deep && (!onlyExpanded || isExpanded ()))
	{
		if(!onlyExpanded)
			createSubItems ();

		if(items)
			ListForEachObject (*items, TreeItem, child)
				if(tree.isItemVisible (child))
					return child;
			EndFor
	}

	if(parent && parent->items)
	{
		bool foundThis = false;
		ListForEachObject (*parent->items, TreeItem, item)
			if(foundThis)
			{
				if(tree.isItemVisible (item))
					return item;
			}
			else if(item == this)
				foundThis = true;
		EndFor

		return parent->getNextVisible (false, onlyExpanded, tree);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeItem* TreeItem::getPreviousVisible (bool onlyExpanded)
{
	if(parent)
		if(TreeItem* prev = parent->findPreviousChildDeep (this, onlyExpanded, *getTree ()))
			return prev;
	return parent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeItem* TreeItem::findPreviousChildDeep (TreeItem* startItem, bool onlyExpanded, Tree& tree)
{
	ASSERT (startItem == nullptr || startItem->getParent () == this)
	if(onlyExpanded)
	{
		if(!isExpanded ())
			return nullptr;
	}
	else
		createSubItems ();

	bool skip = startItem != nullptr; // skip all up to startItem
	if(items)
	{
		ListForEachObjectReverse (*items, TreeItem, child)
			if(!tree.isItemVisible (child))
				continue;

			if(skip)
			{
				if(child == startItem)
					skip = false;
				continue;
			}

			// find the last (deep) descendant of child
			if(TreeItem* deepChild = child->findPreviousChildDeep (nullptr, onlyExpanded, tree))
				if(tree.isItemVisible (deepChild))
					return deepChild;

			// no deep child
			return child;
		EndFor
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeItem::traverse (TreeTraverser& t) const
{
	if(!t.visit (const_cast<TreeItem*> (this)))
		return false;

	if(items && t.stepInto (const_cast<TreeItem*> (this)))
	{
		t.currentInset++;
		ListForEachObject (*items, TreeItem, item)
			t.currentIndex++;
			if(!item->traverse (t))
				return false;
		EndFor
		t.currentInset--;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeItem* TreeItem::getVisibleItem (int idx) const
{
	Tree* tree = getTree ();
	if(!tree)
		return nullptr;

	TreeFindItem t (idx, *tree);
	traverse (t);
	return t.result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeItem::getItemPosition (TreeItem* searchItem, int& row, int& column) const
{
	Tree* tree = getTree ();
	if(!tree)
		return 0;

	TreeFindPosition t (searchItem, *tree);
	traverse (t);
	row = t.row;
	column = t.column;
	return row != -1 && column != -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeItem::countVisible (int& numRows, int& numColumns) const
{
	Tree* tree = getTree ();
	if(!tree)
		return;

	TreeCountVisible t (*tree);
	traverse (t);
	numRows = t.numRows + 1;
	numColumns = t.numColumns + 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeItem* TreeItem::getParent () const
{ 
	return parent; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITreeItem* CCL_API TreeItem::getParentItem () const
{
	return parent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeItem::setData (IUnknown* _data)
{
	take_shared<IUnknown> (data, _data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API TreeItem::getData () const 
{ 
	return data; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeItem::getContent (IUnknownList& list) const
{
	if(!wasExpanded ())
		return false;

	if(items)
		ListForEachObject (*items, TreeItem, item)
			if(item->getData ())
				list.add (item->getData (), true);
		EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API TreeItem::getContent () const
{
	if(wasExpanded () && items)
		return NEW DataIterator (*items);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TreeItem::getState () const
{
	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IViewStateHandler* CCL_API TreeItem::createExpandState ()
{
	return NEW TreeItemState;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IViewStateHandler* CCL_API TreeItem::storeExpandState ()
{
	TreeItemState* state = NEW TreeItemState ();
	state->store (*this);
	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeItem::restoreExpandState (IViewStateHandler* state)
{
	TreeItemState* itemState = unknown_cast<TreeItemState> (state);
	bool result = itemState ? itemState->restore (*this) : false;

	if(Tree* tree = getTree ())
	{
		UnknownPtr<ISubject> modelSubject (tree->getModel ());
		if(modelSubject)
			modelSubject->signal (Message ("updateSize"));
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* TreeItem::getIcon ()
{
	Tree* tree = getTree ();
	return tree ? tree->getItemIcon (this) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* TreeItem::getThumbnail ()
{
	Tree* tree = getTree ();
	return tree ? tree->getItemThumbnail (this) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef TreeItem::getTitle (String& result)
{
	result.empty ();
	if(!title.isEmpty ())
		result = title;
	else
	{
		Tree* tree = getTree ();
		if(tree)
			tree->getItemTitle (result, this);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeItem::setTitle (StringRef _title) 
{ 
	title = _title; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeItem::checkIsFolder ()
{
	if(items && !items->isEmpty ())
		return true;

	Tree* tree = getTree ();
	if(tree && tree->isItemFolder (this))
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeItem::isFolder ()
{
	if(state & kIsFolder)
		return true;
	if(state & kIsLeaf)
		return false;

	bool folder = checkIsFolder ();
	if(folder)
		state |= kIsFolder;
	else
		state |= kIsLeaf;
	return folder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeItem::canExpand ()
{
	if(items && !items->isEmpty ())
		return true;

	Tree* tree = getTree ();
	return tree ? tree->canExpandItem (this) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeItem::canAutoExpand ()
{
	Tree* tree = getTree ();
	return tree ? tree->canAutoExpandItem (this) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeItem::onExpand ()
{
	bool result = false;
	Tree* tree = getTree ();
	if(tree)
		result = tree->onExpandItem (this);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeItem::expand (bool state, bool deep)
{
	isExpanded (state);

	if(state && !wasExpanded ()) // first time expanded
	{
		onExpand ();
		wasExpanded (true);
	}

	if(items && deep)
		ListForEachObject (*items, TreeItem, item)
			item->expand (state, true);
		EndFor
}

//************************************************************************************************
// Tree
//************************************************************************************************

DEFINE_CLASS (Tree, TreeItem)
DEFINE_CLASS_UID (Tree, 0xFF1CCF29, 0x0AD3, 0x4D48, 0xA9, 0x67, 0x72, 0x71, 0x8B, 0xAD, 0x31, 0xB0)

//////////////////////////////////////////////////////////////////////////////////////////////////

Tree::Tree (IItemModel* model, StringRef title)
: TreeItem (title),
  model (model)
{
	setItemFilter (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Tree::~Tree ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Tree::setTreeModel (IItemModel* model)
{
	setModel (model);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITreeItem* CCL_API Tree::getRootItem ()
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Tree* Tree::getTree () const
{
	return const_cast<Tree*> (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* Tree::getItemIcon (TreeItem* item)
{
	return model ? unknown_cast<Image> (model->getItemIcon (item->asIndex ())) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* Tree::getItemThumbnail (TreeItem* item)
{
	return model ? unknown_cast<Image> (model->getItemThumbnail (item->asIndex ())) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Tree::getItemTitle (String& title, TreeItem* item)
{
	return model ? model->getItemTitle (title, item->asIndex ()) != 0 : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Tree::isItemFolder (TreeItem* item)
{
	return model ? model->isItemFolder (item->asIndex ()) != 0 : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Tree::canExpandItem (TreeItem* item)
{
	return model ? model->canExpandItem (item->asIndex ()) != 0 : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Tree::canAutoExpandItem (TreeItem* item)
{
	return model ? model->canAutoExpandItem (item->asIndex ()) != 0 : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Tree::onExpandItem (TreeItem* parent)
{
	if(!model)
		return false;

	UnknownList items;
	if(!model->getSubItems (items, parent->asIndex ()))
		return false;

	ForEachUnknown (items, obj)
		TreeItem* item = unknown_cast<TreeItem> (obj);
		if(item)
			item->retain ();
		else
		{
			item = NEW TreeItem;
			item->setData (obj); // data is shared!
		}
		parent->addItem (item);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Tree::setRootItem (IUnknown* data)
{
	TreeItem* existingItem = findByIndex (data, true);
	if(existingItem)
	{
		absorbItem (*existingItem);
	}
	else if(data)
	{
		setData (data);
		setTitle (String::kEmpty);
		removeAll ();
		wasExpanded (false);
		isExpanded (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Tree::setItemFilter (IObjectFilter* filter)
{
	if(filter)
		itemFilter.share (filter);
	else 
		itemFilter = NEW AlwaysTrueFilter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectFilter* Tree::getItemFilter () const
{
	return itemFilter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool Tree::isItemVisible (TreeItem* item)
{
	return itemFilter->matches (item->getData ());
}

//************************************************************************************************
// TreeItemState
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TreeItemState, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeItemState::TreeItemState ()
{
	subStates.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeItemState::store (TreeItem& item)
{
	if(Tree* tree = item.getTree ())
		if(IItemModel* model = tree->getModel ())
			return store (item, *model);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeItemState::restore (TreeItem& item)
{
	if(Tree* tree = item.getTree ())
		if(IItemModel* model = tree->getModel ())
		{
			item.expand (false, true);
			return restore (item, *model);
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeItemState::store (TreeItem& item, IItemModel& model)
{
	expanded = item.isExpanded ();
	bool succeeded = model.getUniqueItemName (name, &item);
	ASSERT (succeeded && !name.isEmpty ())
	CCL_PRINTF ("%sTreeItemState::store: %s %s\n", CCL_INDENT, name.str (), expanded ? "(expanded)" : "")
	CCL_ADD_INDENT (2)

	ForEach (item, TreeItem, subItem)
		if(subItem->wasExpanded ())
		{
			TreeItemState* state = NEW TreeItemState;
			if(state->store (*subItem, model))
				subStates.add (state);
			else
				state->release (); // can be ignored, all items in the branch are collapsed
		}
	EndFor
	return expanded || !subStates.isEmpty (); // tell if at least one item in this branch is expanded
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TreeItemState::restore (TreeItem& item, IItemModel& model)
{
	MutableCString itemName;
	bool succeeded = model.getUniqueItemName (itemName, &item);

	if(succeeded && itemName == name)
	{
		CCL_PRINTF ("%sTreeItemState::restore: %s %s\n", CCL_INDENT, name.str (), expanded ? "(expanded)" : "")
		CCL_ADD_INDENT (2)

		if(subStates.isEmpty ())
			item.expand (expanded);
		else
		{
			item.expand (true);

			ForEach (subStates, TreeItemState, state)
				ForEach (item, TreeItem, subItem)
					if(state->restore (*subItem, model))
						break;
				EndFor
			EndFor

			if(!expanded)
				item.expand (false);
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeItemState::saveViewState (StringID viewID, StringID viewName, IAttributeList& attributes, const IViewState*) const
{
	if(expanded)
		attributes.remove ("collapsed");
	else
	{
		#if SAVE_ONLY_VISIBLE_STATES
		return true; // ignore the whole subtree of a collapsed item (it's not visible anyway until the user expands the top item)
		#endif

		attributes.setAttribute ("collapsed", true);
	}
    
    AttributeAccessor acc (attributes);
	acc.set ("name", name, Text::kUTF8);

	if(!subStates.isEmpty ())
	{
		ForEach (subStates, TreeItemState, state)
			IAttributeList* subAttribs = acc.newAttributes ();
			state->saveViewState (viewID, viewName, *subAttribs, nullptr);
			attributes.queueAttribute ("childs", subAttribs, IAttributeList::kOwns);
		EndFor
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeItemState::loadViewState (StringID viewID, StringID viewName, const IAttributeList& attributes, IViewState*)
{
	AttributeAccessor acc (const_cast<IAttributeList&> (attributes));
	acc.getCString (name, "name", Text::kUTF8);
	expanded = !acc.getBool ("collapsed");

	UnknownPtr<IAttributeList> subAttribs;
	while((subAttribs = acc.unqueueUnknown ("childs")))
	{
		TreeItemState* state = NEW TreeItemState;
		state->loadViewState (viewID, viewName, *subAttribs, nullptr);
		subStates.add (state);
		subAttribs->release ();
	}
	return true;
}

} // namespace CCL
