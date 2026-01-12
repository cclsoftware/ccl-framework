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
// Filename    : ccl/gui/itemviews/itemviewaccessibility.cpp
// Description : ItemView Accessibility
//
//************************************************************************************************

#include "ccl/gui/itemviews/itemviewaccessibility.h"
#include "ccl/gui/itemviews/listview.h"
#include "ccl/gui/itemviews/treeview.h"
#include "ccl/gui/itemviews/treeitem.h"
#include "ccl/gui/itemviews/headerview.h"

namespace CCL {

//************************************************************************************************
// ItemAccessibilityProvider
//************************************************************************************************

class ItemAccessibilityProvider: public AccessibilityProvider,
								 public IAccessibilityActionProvider,
								 public IAccessibilityValueProvider
{
public:
	DECLARE_CLASS_ABSTRACT (ItemAccessibilityProvider, AccessibilityProvider)
	
	ItemAccessibilityProvider (const ItemViewAccessibilityProvider& parent, const ItemIndex& index, int column);

	PROPERTY_VARIABLE (int, column, Column)

	// AccessibilityProvider
	AccessibilityProvider* findElementProvider (AccessibilityDirection direction) const override;
	void CCL_API getElementName (String& name) const override;
	int CCL_API getElementState () const override;
	tresult CCL_API getElementBounds (Rect& bounds, AccessibilityCoordSpace space) const override;
	AccessibilityElementRole CCL_API getElementRole () const override;
	View* getView () const override;
	tresult CCL_API makeVisible (tbool relaxed = false) override;
	
	// IAccessibilityActionProvider
	tresult CCL_API performAction () override;

	// IAccessibilityValueProvider
	tbool CCL_API isReadOnly () const override;
	tresult CCL_API getValue (String& value) const override;
	tresult CCL_API setValue (StringRef value) const override;
	tbool CCL_API canIncrement () const override { return false; }
	tresult CCL_API increment () const override { return kResultNotImplemented; }
	tresult CCL_API decrement () const override { return kResultNotImplemented; }

	CLASS_INTERFACE2 (IAccessibilityActionProvider, IAccessibilityValueProvider, AccessibilityProvider)

protected:
	const ItemViewAccessibilityProvider& parent;
	ItemIndex index;
};

//************************************************************************************************
// TreeItemAccessibilityProvider
//************************************************************************************************

class TreeItemAccessibilityProvider: public ItemAccessibilityProvider,
									 public IAccessibilityExpandCollapseProvider
{
public:
	using ItemAccessibilityProvider::ItemAccessibilityProvider;

	// IAccessibilityExpandCollapseProvider
	tresult CCL_API expand (tbool state = true) override;
	tbool CCL_API isExpanded () override;

	CLASS_INTERFACE (IAccessibilityExpandCollapseProvider, ItemAccessibilityProvider)

protected:
	TreeItem* getTreeItem () const;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// ItemAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ItemAccessibilityProvider, AccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemAccessibilityProvider::ItemAccessibilityProvider (const ItemViewAccessibilityProvider& _parent, const ItemIndex& _index, int _column)
: parent (_parent),
  index (_index),
  column (_column)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* ItemAccessibilityProvider::findElementProvider (AccessibilityDirection direction) const
{
	AccessibilityProvider* result = nullptr;
	switch(direction)
	{
	case AccessibilityDirection::kParent :
		result = parentProvider;
		break;

	case AccessibilityDirection::kNextSibling :
	case AccessibilityDirection::kPreviousSibling :
		if(parentProvider)
		{
			const ObjectArray& siblings = parentProvider->getChildren ();
			int index = siblings.index (this);
			ASSERT (index != -1)

			if(direction == AccessibilityDirection::kNextSibling)
				index++;
			else
				index--;

			if(siblings.isValidIndex (index))
				result = static_cast<AccessibilityProvider*> (siblings.at (index));
		}
		break;

	case AccessibilityDirection::kFirstChild :
		if(!children.isEmpty ())
			result = static_cast<AccessibilityProvider*> (children.first ());
		break;

	case AccessibilityDirection::kLastChild :
		if(!children.isEmpty ())
			result = static_cast<AccessibilityProvider*> (children.last ());
		break;
	}
	if(result)
		return result;
	return SuperClass::findElementProvider (direction);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemAccessibilityProvider::getElementName (String& name) const
{
	parent.getElementName (name, index, column);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ItemAccessibilityProvider::getElementState () const
{
	int state = AccessibilityElementState::kEnabled | AccessibilityElementState::kCanFocus;
	
	ItemIndex focusItem = 0;
	if(parent.getItemView ().getFocusItem (focusItem) && focusItem.getIndex () == index.getIndex ())
		state |= AccessibilityElementState::kHasFocus;

	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ItemAccessibilityProvider::getElementBounds (Rect& b, AccessibilityCoordSpace space) const
{
	return parent.getElementBounds (b, space, index, column);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API ItemAccessibilityProvider::getElementRole () const
{
	return parent.getElementRole (index, column);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ItemAccessibilityProvider::getView () const
{
	return parent.getView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ItemAccessibilityProvider::makeVisible (tbool relaxed)
{
	parent.getItemView ().makeItemVisible (index);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ItemAccessibilityProvider::performAction ()
{
	if(parent.getItemView ().selectItem (index, true))
		return kResultOk;
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemAccessibilityProvider::isReadOnly () const
{
	// TODO
	return true;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ItemAccessibilityProvider::getValue (String& value) const
{
	return parent.getElementValue (value, index, column) ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ItemAccessibilityProvider::setValue (StringRef value) const
{
	// TODO
	return kResultNotImplemented;
}

//************************************************************************************************
// TreeItemAccessibilityProvider
//************************************************************************************************

tresult CCL_API TreeItemAccessibilityProvider::expand (tbool state)
{
	if(TreeItem* item = getTreeItem ())
	{
		item->expand (state);
		return kResultOk;
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TreeItemAccessibilityProvider::isExpanded ()
{
	if(TreeItem* item = getTreeItem ())
		return item->isExpanded ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeItem* TreeItemAccessibilityProvider::getTreeItem () const
{
	if(TreeItem* firstItem = static_cast<TreeView&> (parent.getItemView ()).getFirstTreeItem ())
		return firstItem->findByIndex (index);
	return nullptr;
}

//************************************************************************************************
// ItemViewAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ItemViewAccessibilityProvider, ViewAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemViewAccessibilityProvider::ItemViewAccessibilityProvider (ItemView& itemView)
: ViewAccessibilityProvider (itemView)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemView& ItemViewAccessibilityProvider::getItemView () const
{
	return static_cast<ItemView&> (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ItemViewAccessibilityProvider::queryInterface (CCL::UIDRef iid, void** ptr)
{
	if(iid == ccl_iid<IAccessibilityTableProvider> () && getItemView ().getColumnHeaders () != nullptr)
		QUERY_INTERFACE (IAccessibilityTableProvider)
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewAccessibilityProvider::getElementName (String& name, ItemIndexRef index, int column) const
{
	if(IItemModel* model = getItemView ().getModel ())
	{
		IItemModel::AccessibilityInfo info = {};
		if(model->getItemAccessibilityInfo (info, index, column))
			name = info.name;
		if(name.isEmpty () && column == 0)
			model->getItemTitle (name, index);
		if(name.isEmpty ())
			model->getItemTooltip (name, index, column);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ItemViewAccessibilityProvider::getElementBounds (Rect& rect, AccessibilityCoordSpace space, ItemIndexRef index, int column) const
{
	if(column == 0 && getItemView ().getColumnHeaders () == nullptr)
		column = -1;

	getItemView ().getItemRect (rect, index, column);
	Rect clipping;
	getItemView ().getVisibleClient (clipping);
	rect.bound (clipping);
	Point screenOffset;
	getItemView ().clientToScreen (screenOffset);
	rect.offset (screenOffset);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole ItemViewAccessibilityProvider::getElementRole (ItemIndexRef index, int column) const
{
	if(IItemModel* model = getItemView ().getModel ())
	{
		IItemModel::AccessibilityInfo info = {};
		if(model->getItemAccessibilityInfo (info, index, column))
			return static_cast<AccessibilityElementRole> (info.role);
	}

	return AccessibilityElementRole::kDataItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemViewAccessibilityProvider::getElementValue (String& value, ItemIndexRef index, int column) const
{
	if(IItemModel* model = getItemView ().getModel ())
	{
		IItemModel::AccessibilityInfo info = {};
		if(model->getItemAccessibilityInfo (info, index, column))
		{
			value = info.value;
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewAccessibilityProvider::rebuildItemProviders ()
{
	ArrayForEachReverse (getChildren (), AccessibilityProvider, item)
		removeChildProvider (item);
	EndFor
	
	ASSERT (AccessibilityManager::isEnabled ())
	
	int visibleRows = 0;
	if(IItemModel* model = getItemView ().getModel ())
		visibleRows = model->countFlatItems ();

	for(int i = 0; i < visibleRows; i++)
		addColumnProviders<ItemAccessibilityProvider> (i);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class ItemProvider>
void ItemViewAccessibilityProvider::addColumnProviders (ItemIndexRef index)
{
	int visibleColumns = 1;
	ColumnHeaderList* headerList = getItemView ().getColumnHeaders ();
	if(headerList)
		visibleColumns = headerList->getCount (false);
	
	for(int j = 0; j < visibleColumns; j++)
	{
		int columnIndex = j;
		if(headerList)
		{
			ColumnHeader* header = headerList->getColumnAtPosition (j, false);
			ASSERT (header != nullptr)
			if(header)
			{
				if(header->isHidden ())
					continue;
				columnIndex = header->getIndex ();
			}
		}
		AutoPtr<ItemProvider> child = NEW ItemProvider (*this, index, columnIndex);
		addChildProvider (child);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ItemViewAccessibilityProvider::countColumns () const
{
	ColumnHeaderList* headerList = getItemView ().getColumnHeaders ();
	if(headerList)
		return headerList->getCount (true);
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAccessibilityProvider* CCL_API ItemViewAccessibilityProvider::getColumnHeaderProvider ()
{
	ScrollView* scrollView = ScrollView::getScrollView (&getItemView ());
	if(scrollView == nullptr)
		return nullptr;

	View* header = scrollView->getHeader ();
	if(header == nullptr)
		return nullptr;

	return header->getAccessibilityProvider ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAccessibilityProvider* CCL_API ItemViewAccessibilityProvider::getColumnHeaderItemProvider (IAccessibilityProvider* dataItem)
{
	AccessibilityProvider* headerProvider = unknown_cast<AccessibilityProvider> (getColumnHeaderProvider ());
	if(headerProvider == nullptr)
		return nullptr;
	
	ItemAccessibilityProvider* itemProvider = unknown_cast<ItemAccessibilityProvider> (dataItem);
	if(itemProvider == nullptr)
		return nullptr;

	const ObjectArray& children = headerProvider->getChildren ();
	return ccl_cast<AccessibilityProvider> (children.at (itemProvider->getColumn ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ItemViewAccessibilityProvider::countRows () const
{
	return children.count ();
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

IAccessibilityProvider* CCL_API ItemViewAccessibilityProvider::getRowHeaderProvider ()
{
	return nullptr;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

IAccessibilityProvider* CCL_API ItemViewAccessibilityProvider::getRowHeaderItemProvider (IAccessibilityProvider* dataItem)
{
	return nullptr; 
}

//************************************************************************************************
// ListViewAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ListViewAccessibilityProvider, ItemViewAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewAccessibilityProvider::ListViewAccessibilityProvider (ListView& listView)
: ItemViewAccessibilityProvider (listView)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListView& ListViewAccessibilityProvider::getListView () const
{
	return static_cast<ListView&> (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API ListViewAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kList;
}

//************************************************************************************************
// TreeViewAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TreeViewAccessibilityProvider, ItemViewAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeViewAccessibilityProvider::TreeViewAccessibilityProvider (TreeView& treeView)
: ItemViewAccessibilityProvider (treeView)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TreeView& TreeViewAccessibilityProvider::getTreeView () const
{
	return static_cast<TreeView&> (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API TreeViewAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kTree;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole TreeViewAccessibilityProvider::getElementRole (ItemIndexRef index, int column) const
{
	return AccessibilityElementRole::kDataItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeViewAccessibilityProvider::rebuildItemProviders ()
{
	ArrayForEachReverse (getChildren (), AccessibilityProvider, item)
		removeChildProvider (item);
	EndFor
	
	ASSERT (AccessibilityManager::isEnabled ())
	
	for(TreeItem* currentItem = getTreeView ().getFirstTreeItem (); currentItem != nullptr; currentItem = currentItem->getNextVisible (true))
		addColumnProviders<TreeItemAccessibilityProvider> (currentItem->asIndex ());
}
