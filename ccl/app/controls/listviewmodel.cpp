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
// Filename    : ccl/app/controls/listviewmodel.cpp
// Description : List View Model
//
//************************************************************************************************

#include "ccl/app/controls/listviewmodel.h"
#include "ccl/app/utilities/boxedguitypes.h"

#include "ccl/base/message.h"

#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/themeelements.h"
#include "ccl/public/gui/framework/usercontrolbase.h"
#include "ccl/public/gui/framework/iaccessibility.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/plugservices.h"

namespace CCL {

static DEFINE_ARRAY_COMPARE (SortByTitle, ListViewItem, lhs, rhs)
	return lhs->getTitle ().compareWithOptions (rhs->getTitle (), Text::kIgnoreCase|Text::kCompareNumerically);
}

//************************************************************************************************
// DetailListViewSorter
//************************************************************************************************

class DetailListViewSorter: public ListViewSorter
{
public:
	DECLARE_CLASS_ABSTRACT (DetailListViewSorter, ListViewSorter)
	
	DetailListViewSorter (StringID columnId, StringRef title, StringID detailId);
	
	static MutableCString activeDetailId;
	PROPERTY_MUTABLE_CSTRING (detailId, DetailId)
	
	// ListViewSorter
	void sort (ObjectArray& items) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static DEFINE_ARRAY_COMPARE (SortByDetail, ListViewItem, lhs, rhs)
	MutableCString detailId = DetailListViewSorter::activeDetailId;
	if(!detailId.isEmpty ())
	{
		Variant left, right;
		if(lhs->getDetail (left, detailId) && rhs->getDetail (right, detailId))
			return left.compare (right);
	}
	return 0;
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// ListViewModelBase
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ListViewModelBase, ItemModel)

DEFINE_STRINGID_MEMBER_ (ListViewModelBase, kIconID, "icon")
DEFINE_STRINGID_MEMBER_ (ListViewModelBase, kTitleID, "title")
DEFINE_STRINGID_MEMBER_ (ListViewModelBase, kSubtitleID, "subtitle")
DEFINE_STRINGID_MEMBER_ (ListViewModelBase, kCheckBoxID, "check")
DEFINE_STRINGID_MEMBER_ (ListViewModelBase, kEditSelectID, "edit")

DEFINE_STRINGID_MEMBER_ (ListViewModelBase, kEditItemCell, "editItemCell")

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewModelBase::ListViewModelBase ()
: subtitlesEnabled (false),
  keepSelectedItem (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IColumnHeaderList& ListViewModelBase::getColumns ()
{
	if(columns == nullptr)
		columns = ccl_new<IColumnHeaderList> (ClassID::ColumnHeaderList);
	return *columns;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID ListViewModelBase::getColumnID (int column) const
{
	return columns ? columns->getColumnID (column) : CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListViewModelBase::getColumnIndex (StringID id) const
{
	if(columns)
		for(int i = 0; i < columns->getColumnCount (); i++)
			if(columns->getColumnID (i) == id)
				return i;
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewModel::ColumnType ListViewModelBase::getColumnType (CString& id, int column) const
{
	if(columns)
	{
		id = columns->getColumnID (column);
		if(id.isEmpty ())
			return kEmptyColumn;

		if(id == kIconID)
			return kIconColumn;
		if(id == kTitleID)
			return kTitleColumn;
		if(id == kEditSelectID)
			return kEditSelectColumn;
		if(id == kCheckBoxID)
			return kCheckBoxColumn;
	}
	else // no columns defined
	{
		if(column == 0)
			return kTitleColumn;
	}
	return kDetailColumn;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewModel::ColumnInfo* ListViewModelBase::getColumnInfo (int column, bool create) const
{
	Variant data;
	IColumnHeaderList* columns = create ? &const_cast<ListViewModelBase*> (this)->getColumns () :
								 static_cast<IColumnHeaderList*> (this->columns);
	if(columns)
		columns->getColumnDataAt (data, column);
	ColumnInfo* columnInfo = unknown_cast<ColumnInfo> (data.asUnknown ());
	if(!columnInfo && create)
	{
		columnInfo = NEW ColumnInfo;
		data.takeShared (columnInfo->asUnknown ());
		columnInfo->release ();
		if(!columns->setColumnDataAt (column, data))
			columnInfo = nullptr; // invalid column
	}
	return columnInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModelBase::setColumnAlignment (int columnIndex, AlignmentRef alignment)
{
	if(ColumnInfo* columnInfo = getColumnInfo (columnIndex, true))
		columnInfo->setAlignment (alignment);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Alignment ListViewModelBase::getColumnAlignment (int columnIndex) const
{
	if(ColumnInfo* columnInfo = getColumnInfo (columnIndex))
		return columnInfo->getAlignment ();
	return Alignment ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModelBase::getIndex (ItemIndex& index, const ListViewItem* item) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModelBase::setListViewType (Styles::ListViewType viewType)
{
	listViewType = viewType;

	for(auto itemView : getItemViews ())
	{
		UnknownPtr<IListView> listView (itemView);
		if(listView)
			listView->setViewType (viewType);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Styles::ListViewType ListViewModelBase::getListViewType () const
{
	return listViewType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModelBase::getItemTitle (String& title, ItemIndexRef index)
{
	if(ListViewItem* item = resolve (index))
	{
		title = item->getTitle ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ListViewModelBase::getItemIcon (ItemIndexRef index)
{
	ListViewItem* item = resolve (index);
	return item ? item->getIcon () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API ListViewModelBase::getItemThumbnail (ItemIndexRef index)
{
	ListViewItem* item = resolve (index);
	return item ? item->getThumbnail () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModelBase::getItemTooltip (String& tooltip, ItemIndexRef index, int column)
{
	if(ListViewItem* item = resolve (index))
	{
		StringID id = getColumnID (column);
		return item->getTooltip (tooltip, id);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API ListViewModelBase::getItemBackground (ItemIndexRef index)
{
	ListViewItem* item = resolve (index);
	return item ? item->getCustomBackground () : CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModelBase::canSelectItem (ItemIndexRef index)
{
	ListViewItem* item = resolve (index);
	return item && item->isEnabled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewItem* ListViewModelBase::getFocusItem () const
{
	ItemIndex index;
	if(IItemView* itemView = ccl_const_cast (this)->getItemView ())
		if(itemView->getFocusItem (index))
			return resolve (index);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModelBase::getSelectedItems (Container& selected) const
{
	if(IItemView* itemView = ccl_const_cast (this)->getItemView ())
		ForEachItem (itemView->getSelection (), index)
			if(ListViewItem* item = resolve (index))
				selected.add (item);
		EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewItem* ListViewModelBase::getFirstSelectedItem () const
{
	if(IItemView* itemView = ccl_const_cast (this)->getItemView ())
		ForEachItem (itemView->getSelection (), index)
			if(ListViewItem* item = resolve (index))
				return item;
		EndFor

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModelBase::visitSelectedItemsInternal (ViewItemVisitor& itemVisitor)
{
	if(IItemView* itemView = ccl_const_cast (this)->getItemView ())
		ForEachItem (itemView->getSelection (), index)
			if(ListViewItem* item = resolve (index))
				if(!itemVisitor.visit (*item))
					return false;
		EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModelBase::isAnyItemChecked ()
{
	bool checked = false;

	visitItems ([&] (ListViewItem& item)
	{
		checked = item.isChecked ();
		return !checked;
	});

	return checked;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModelBase::visitItemsInternal (ViewItemVisitor& itemVisitor)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModelBase::createColumnHeaders (IColumnHeaderList& list)
{
	if(columns)
	{
		list.copyFrom (*columns);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModelBase::editCheckBoxColumn (ItemIndexRef index, const EditInfo& info)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModelBase::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	ListViewItem* item = resolve (index);
	if(item == nullptr)
		return false;

	CString columnID;
	ColumnType columnType = getColumnType (columnID, column);
	switch(columnType)
	{
	case kIconColumn :
		if(item->getIcon ())
			drawIcon (info, item->getIcon (), item->isEnabled ());
		drawIconOverlay (index, info); // call manually, not called by ListView in this case
		break;

	case kTitleColumn :
		if(subtitlesEnabled)
		{
			Variant subTitle;
			item->getDetail (subTitle, kSubtitleID);
			drawTitleWithSubtitle (info, item->getTitle (), subTitle.asString (), item->isEnabled ());
		}
		else
			drawTitle (info, item->getTitle (), item->isEnabled ());
		break;

	case kCheckBoxColumn :
		drawCheckBox (info, item->isChecked (), item->isEnabled ());
		break;

	case kEditSelectColumn :
		{
			const IVisualStyle& vs = ViewBox (info.view).getVisualStyle ();
			Color color = vs.getColor ("itemSelectBackColor", Color (0, 0, 0, 0));
			if(color.alpha != 0)
				info.graphics.fillRect (info.rect, SolidBrush (color));

			if(IImage* icon = vs.getImage ("itemSelectIcon"))
			{
				IImage::Selector (icon, item->isChecked () ? ThemeNames::kNormalOn : ThemeNames::kNormal);
				Rect src (0, 0, icon->getWidth (), icon->getHeight ());
				Rect iconRect (src);
				iconRect.center (info.rect);
				info.graphics.drawImage (icon, src, iconRect);
			}
		}
		break;

	case kDetailColumn :
		{
			Alignment alignment = getColumnAlignment (column);
			item->drawDetail (info, columnID, alignment);
		}
		break;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModelBase::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	CString columnID;
	ColumnType columnType = getColumnType (columnID, column);

	switch(columnType)
	{
	case kCheckBoxColumn:
		return editCheckBoxColumn (index, info);

	case kEditSelectColumn :
		// using "checked" property to indicate edit selection state
		if(ListViewItem* item = resolve (index))
		{
			bool newState = !item->isChecked ();

			if(const MouseEvent* mouseEvent = info.editEvent.as<MouseEvent> ())
			{
				return swipeItems (info.view, *mouseEvent, [this, newState] (ItemIndexRef index)
				{
					if(ListViewItem* item = (ListViewItem*)resolve (index))
					{
						item->setChecked (newState);
						for(auto itemView : getItemViews ())
							itemView->invalidateItem (index);					
					}
					return false;
				});
			}
			else
			{
				item->setChecked (newState);
				for(auto itemView : getItemViews ())
					itemView->invalidateItem (index);
			}
		}
		return true;

	default:
		if(ListViewItem* item = resolve (index))
		{
			String columnID (getColumnID (column));
			CCL_BOX (BoxedEditInfo, boxedEditInfo, info)
			signal (Message (kEditItemCell, item->asUnknown (), columnID, boxedEditInfo->asUnknown ()));
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ListViewModelBase::createDragSessionData (ItemIndexRef index)
{
	if(ListViewItem* item = resolve (index))
		return item->createDragObject ();
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ListViewModelBase::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IItemView::kSelectionChanged)
	{
		onSelectionChanged ();
		signal (Message (IItemView::kSelectionChanged));
	}
	else if(msg == IColumnHeaderList::kColumnRectsChanged)
	{
		onColumnRectsChanged ();
	}
	else if(msg == IItemView::kViewAttached || msg == IItemView::kViewRemoved)
	{
		SuperClass::notify (subject, msg);

		bool state = msg == IItemView::kViewAttached;
		UnknownPtr<IItemView> itemView (subject);
		onVisibleChanged (state);

		if(editModeParam && itemView)
			itemView->setEditModeParam (editModeParam);

		if(keepSelectedItem)
		{
			if(state)
			{
				if(savedSelectedItem && itemView)
				{
					ItemIndex selIndex;
					if(getIndex (selIndex, savedSelectedItem))
						itemView->selectItem (selIndex, true);

					savedSelectedItem = nullptr;
				}
			}
			else
				savedSelectedItem = getFirstSelectedItem ();
		}
	}
}

//************************************************************************************************
// ListViewSorter
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ListViewSorter, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewSorter::ListViewSorter (StringID id, StringRef title, ObjectArray::CompareFunction* sortFunction)
: id (id),
  title (title),
  sortFunction (sortFunction),
  reversed (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewSorter::sort (ObjectArray& items)
{
	ASSERT (sortFunction != nullptr)
	if(sortFunction)
		items.sort (*sortFunction);
	if(reversed)
		items.reverse ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewSorter::toString (String& string, int flags) const
{
	string = title;
	return true;
}

//************************************************************************************************
// ListViewModel::CheckBoxMouseHandler
//************************************************************************************************

class ListViewModel::CheckBoxMouseHandler: public AbstractMouseHandler,
										   public Unknown
{
public:
	CheckBoxMouseHandler (IItemView* listView, ListViewModel& listModel, int startItem)
	: listViewModel (listModel),
	  listView (listView),
	  startItem (startItem),
	  state (false)
	{
		canEscape (true);
	}

	void onBegin () override
	{
		if(ListViewItem* item = listViewModel.getItem (startItem))
		{
			state = !item->isChecked ();
			onMove (0);
		}
	}

	bool onMove (int moveFlags) override
	{
		ItemIndex index;

		if(!listView || !listView->findItem (index, current.where))
			return true;

		int endItem = index.getIndex ();
		int i = endItem < startItem ? endItem : startItem;
		if(i < 0)
			i = 0;

		int end = endItem < startItem ? startItem : endItem;
		if(end > listViewModel.countFlatItems ())
			end = listViewModel.countFlatItems ()-1;

		for(; i <= end; i++)
		{
			if(ListViewItem* item = listViewModel.resolve (i))
			{
				if(state != item->isChecked ())
				{
					item->setChecked (state);
					listViewModel.onItemChecked (item);
				}
			}
		}

		return true;
	}

	CLASS_INTERFACE (IMouseHandler, Unknown)

protected:
	ListViewModel& listViewModel;
	IItemView* listView;
	int startItem;
	bool state;
};

//************************************************************************************************
// ListViewModel::ColumnInfo
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ListViewModel::ColumnInfo, Object)

//************************************************************************************************
// ListViewModel
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (ListViewModel, kItemChecked, "itemChecked")
DEFINE_STRINGID_MEMBER_ (ListViewModel, kItemOpened, "itemOpened")
DEFINE_STRINGID_MEMBER_ (ListViewModel, kItemFocused, "itemFocused")

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (ListViewModel, ListViewModelBase)
DEFINE_CLASS_UID (ListViewModel, 0xe647e233, 0xe915, 0x4a6a, 0x88, 0xd1, 0xc1, 0x1c, 0x8a, 0x2a, 0x2, 0xe7)
DEFINE_CLASS_NAMESPACE (ListViewModel, "Host")

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewModel::ListViewModel ()
: itemRemovalEnabled (false),
  simpleItemCheck (false)
{
	items.objectCleanup (true);
	sorters.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewModel::~ListViewModel ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewItem* ListViewModel::resolve (ItemIndexRef index) const
{
	return (ListViewItem*)items.at (index.getIndex ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModel::isEmpty () const
{
	return items.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewItem* ListViewModel::getItem (int index) const
{
	return (ListViewItem*)items.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* ListViewModel::newIterator () const
{
	return items.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::removeAll ()
{
	items.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::addItem (ListViewItem* item)
{
	items.add (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::insertItem (int index, ListViewItem* item)
{
	if(!items.insertAt (index, item))
		items.add (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::addSorted (ListViewItem* item)
{
	if(activeSorter)
		items.addSorted (item, activeSorter->getSortFunction (), activeSorter->isReversed ());
	else
		items.addSorted (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModel::removeItem (ListViewItem* item)
{
	return items.remove (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModel::getIndex (ItemIndex& index, const ListViewItem* item) const
{
	int i = items.index (item);
	index = ItemIndex (i);
	return i != -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::enableAll (bool state)
{
	for(int i = 0; i < countFlatItems (); i++)
		if(ListViewItem* item = resolve (i))
		{
			if(state != item->isEnabled ())
			{
				item->setEnabled (state);

				if(getItemView ())
					getItemView ()->invalidateItem (i);
			}
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::checkAll (bool state, bool onlyEnabled)
{
	bool needsSignal = false;

	for(int i = 0; i < countFlatItems (); i++)
		if(ListViewItem* item = resolve (i))
		{
			if(onlyEnabled && !item->isEnabled ())
				continue;

			if(state != item->isChecked ())
			{
				needsSignal = true;
				item->setChecked (state);

				for(auto itemView : getItemViews ())
					itemView->invalidateItem (i);
			}
		}

	if(needsSignal)
		onItemChecked (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::checkAlone (int index, bool _state, bool onlyEnabled)
{
	bool needsSignal = false;

	for(int i = 0; i < countFlatItems (); i++)
		if(ListViewItem* item = resolve (i))
		{
			if(onlyEnabled && !item->isEnabled ())
				continue;

			bool state = i == index ? _state : false;

			if(state != item->isChecked ())
			{
				needsSignal = true;
				item->setChecked (state);

				for(auto itemView : getItemViews ())
					itemView->invalidateItem (i);
			}
		}

	if(needsSignal)
		onItemChecked (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::addSorter (ListViewSorter* sorter)
{
	sorters.add (sorter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::addTitleSorter (StringRef sorterTitle)
{
	sorters.add (NEW ListViewSorter (kTitleID, sorterTitle, &SortByTitle));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::addDetailSorter (StringID columnId, StringID detailId, StringRef sorterTitle)
{
	sorters.add (NEW DetailListViewSorter (columnId, sorterTitle, detailId));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewSorter* ListViewModel::getActiveSorter () const
{
	return activeSorter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewSorter* ListViewModel::getSorterWithID (StringID id) const
{
	ArrayForEach (sorters, ListViewSorter, sorter)
		if(sorter->getID () == id)
			return sorter;
	EndFor
	CCL_DEBUGGER ("Sorter not found!\n")
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::sortBy (ListViewSorter* sorter)
{
	activeSorter = sorter;

	if(activeSorter)
		activeSorter->sort (items);
	else
		items.sort ();

	signal (Message (kChanged));
	signal (Message (IColumnHeaderList::kSortColumnChanged)); // sync sort column
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::resort ()
{
	sortBy (activeSorter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListViewModel::getFirstSelectedIndex () const
{
	if(IItemView* itemView = const_cast<ListViewModel*> (this)->getItemView ())
	{
		ForEachItem (itemView->getSelection (), index)
			return index.getIndex ();
		EndFor
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModel::canRemoveSelectedItems () const
{
	if(IItemView* itemView = const_cast<ListViewModel*> (this)->getItemView ())
	{
		const IItemSelection& selection = itemView->getSelection ();
		ForEachItem (selection, index)
			if(const_cast<ListViewModel*> (this)->canRemoveItem (index))
				return true;
		EndFor
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::removeSelectedItems ()
{
	if(IItemView* itemView = const_cast<ListViewModel*> (this)->getItemView ())
	{
		const IItemSelection& selection = itemView->getSelection ();
		ItemIndex focusIndex;
		itemView->getFocusItem (focusIndex);
		removeItems (focusIndex, selection);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModel::visitItemsInternal (ViewItemVisitor& itemVisitor)
{
	for(int i = 0; i < countFlatItems (); i++)
		if(ListViewItem* item = resolve (ItemIndex (i)))
			if(!itemVisitor.visit (*item))
				return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ListViewModel::countFlatItems ()
{
	return items.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModel::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	if(ListViewItem* item = resolve (index))
	{
		signal (Message (kItemOpened, item->asUnknown (), column));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModel::getSortColumnID (MutableCString& columnID, tbool& upwards)
{
	if(activeSorter)
	{
		columnID = activeSorter->getID ();
		upwards = activeSorter->isReversed ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModel::measureCellContent (Rect& size, ItemIndexRef index, int column, const StyleInfo& info)
{
	ListViewItem* item = resolve (index);
	if(item == nullptr)
		return false;

	CString columnID;
	ColumnType columnType = getColumnType (columnID, column);
	if(columnType == kTitleColumn)
	{
		Font::measureString (size, item->getTitle (), info.font);
		return true;
	}
	else
		return item->measureContent (size, columnID, info);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModel::interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection)
{
	// ListView sends this before removeItem() is called per item
	if(msg.category == "Edit" && msg.name == "Delete" && !msg.checkOnly ())
		return removeItems (item, selection);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModel::canRemoveItem (ItemIndexRef index)
{
	if(!isItemRemovalEnabled ())
		return false;

	ListViewItem* item = resolve (index);
	return item && item->isEnabled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModel::removeItem (ItemIndexRef index)
{
	if(!isItemRemovalEnabled ())
		return false;

	ListViewItem* item = resolve (index);
	if(item && item->isEnabled ())
	{
		items.remove (item);
		item->release ();
		signal (Message (kChanged));
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModel::removeItems (ItemIndexRef index, const IItemSelection& selection)
{
	if(!isItemRemovalEnabled ())
		return false;

	ForEachItem (selection, selectedIndex)
		ListViewItem* item = resolve (index);
		if(item && item->isEnabled ())
		{
			items.remove (item);
			item->release ();
		}
	EndFor
	signal (Message (kChanged));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModel::editCheckBoxColumn (ItemIndexRef index, const EditInfo& info)
{
	ListViewItem* item = resolve (index);
	if(item == nullptr)
		return false;
	if(!item->isEnabled ())
		return false;

	UnknownPtr<IItemView> itemView (info.view);
	if(itemView == nullptr)
	{
		ASSERT (0)
		return false;
	}

	auto toggleItemChecked = [&] ()
	{
		item->setChecked (!item->isChecked ());
		onItemChecked (item);
	};

	if(auto me = info.editEvent.as<MouseEvent> ())
	{
		if(isSimpleItemCheck ())
		{
			toggleItemChecked ();
		}
		else
		{
			if(me->keys.isSet (KeyState::kOption))
			{
				checkAll (!item->isChecked ());
				return true;
			}
			else if(me->keys.isSet (KeyState::kCommand))
			{
				checkAlone (index.getIndex (), !item->isChecked ());
				itemView->setFocusItem (index); // modifier is used by selection, must correct it here
				return true;
			}
			else
			{
				itemView->beginMouseHandler (NEW CheckBoxMouseHandler (itemView, *this, index.getIndex ()), *me);
				return true;
			}
		}
	}
	else
	{
		toggleItemChecked ();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::setAnchorItem (ListViewItem* item)
{
	getIndex (anchorIndex, item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModel::getMultiSelectionRange (ItemIndex& fromIndex, ItemIndex& toIndex, bool& toggle, const MouseEvent& mouseEvent)
{
	IItemView* itemView = getItemView ();
	if(itemView && itemView->findItem (toIndex, mouseEvent.where))
	{
		fromIndex = toIndex;
		toggle = false;

		if(mouseEvent.keys.isSet (KeyState::kShift))
		{
			if(!anchorIndex.isValid ())
			{
				anchorIndex = getFirstSelectedIndex ();
				if(!anchorIndex.isValid ())
					anchorIndex = 0;
			}

			if(anchorIndex.isValid ())
			{
				if(anchorIndex.getIndex () < toIndex.getIndex ())
					fromIndex = anchorIndex;
				else
					toIndex = anchorIndex;
			}
		}
		else
		{
			anchorIndex = toIndex;

			if(mouseEvent.keys.isSet (KeyState::kCommand))
				toggle = true;
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ListViewModel::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IColumnHeaderList::kSortColumnChanged)
	{
		MutableCString columnID (msg[0].asString ());
		bool upwards = msg[1].asBool ();
		onSortColumnChanged (columnID, upwards);
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::onSortColumnChanged (StringID columnID, bool upwards)
{
	ListViewSorter* sorter = getSorterWithID (columnID);
	if(!sorter)
		return;

	bool wasUpwards = activeSorter && activeSorter->isReversed ();
	if(sorter != activeSorter || upwards != wasUpwards)
	{
		sorter->setReversed (upwards);
		sortBy (sorter);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::onItemChecked (ListViewItem* item)
{
	signal (Message (kItemChecked, item ? item->asUnknown () : nullptr));
	if(item)
	{
		int idx = items.index (item);
		if(idx >= 0)
			signal (Message (kItemModified, idx));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModel::onItemFocused (ItemIndexRef index) 
{
	ListViewItem* item = resolve (index);
	signal (Message (kItemFocused, item ? item->asUnknown () : nullptr));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (ListViewModel)
	DEFINE_PROPERTY_NAME ("itemCount")
	DEFINE_PROPERTY_NAME ("columns")
	DEFINE_PROPERTY_NAME ("itemView")
END_PROPERTY_NAMES (ListViewModel)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModel::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "itemCount")
	{
		var = const_cast<ListViewModel*> (this)->countFlatItems ();
		return true;
	}
	else if(propertyId == "columns")
	{
		var.takeShared (&const_cast<ListViewModel*> (this)->getColumns ());
		return true;
	}
	else if(propertyId == "itemView")
	{
		var.takeShared (const_cast<ListViewModel*> (this)->getItemView ());
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModel::getItemAccessibilityInfo (AccessibilityInfo& info, ItemIndexRef index, int column) const
{
	ListViewItem* item = resolve (index);
	if(item == nullptr)
		return false;
	
	StringID columnID = getColumnID (column);

	if(!item->getTooltip (info.name, columnID) || info.name.isEmpty ())
		info.name = item->getTitle ();

	Variant value;
	if(item->getDetail (value, columnID) && value.isString ())
		info.value = value.asString ();

	info.role = int(AccessibilityElementRole::kDataItem);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ListViewModel)
	DEFINE_METHOD_NAME ("newItem")
	DEFINE_METHOD_NAME ("getItem")
	DEFINE_METHOD_NAME ("removeAll")
	DEFINE_METHOD_NAME ("addItem")
	DEFINE_METHOD_NAME ("insertItem")
	DEFINE_METHOD_NAME ("removeItem")
	DEFINE_METHOD_NAME ("getIndex")
	DEFINE_METHOD_NAME ("changed")
	DEFINE_METHOD_NAME ("getFocusItem")
	DEFINE_METHOD_NAME ("getSelectedItems")
	DEFINE_METHOD_NAME ("setColumnAlignment")
	DEFINE_METHOD_NAME ("invalidate")
	DEFINE_METHOD_NAME ("addTitleSorter")
	DEFINE_METHOD_NAME ("addDetailSorter")
END_METHOD_NAMES (ListViewModel)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewModel::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "newItem")
	{
		String title;
		if(msg.getArgCount () > 0)
			title = msg[0].asString ();
		returnValue.takeShared (AutoPtr<IObject> (NEW ListViewItem (title)));
		return true;
	}
	else if(msg == "getItem")
	{
		returnValue.takeShared (static_cast<IObject*> (getItem (msg[0].asInt ())));
		return true;
	}
	else if(msg == "removeAll")
	{
		removeAll ();
		return true;
	}
	else if(msg == "addItem")
	{
		if(ListViewItem* item = unknown_cast<ListViewItem> (msg[0]))
		{
			item->retain ();
			addItem (item);
		}
		else
			addItem (NEW ListViewItem (msg[0].asString ()));
		return true;
	}
	else if(msg == "insertItem")
	{
		int index = msg[0].asInt ();
		if(ListViewItem* item = unknown_cast<ListViewItem> (msg[1]))
		{
			item->retain ();
			insertItem (index, item);
		}
		else
			insertItem (index, NEW ListViewItem (msg[1].asString ()));
		return true;
	}
	else if(msg == "removeItem")
	{
		if(ListViewItem* item = msg[0].isInt () ? getItem (msg[0].asInt ()) : unknown_cast<ListViewItem> (msg[0].asUnknown ()))
		{
			removeItem (item);
			item->release ();
		}
		return true;
	}
	else if(msg == "getIndex")
	{
		ListViewItem* item = unknown_cast<ListViewItem> (msg[0].asUnknown ());
		ItemIndex index;
		returnValue = getIndex (index, item) ? index.getIndex () : -1;
		return true;
	}
	else if(msg == "changed")
	{
		signal (Message (kChanged));
		return true;
	}
	else if(msg == "getFocusItem")
	{
		returnValue.takeShared (ccl_as_unknown (getFocusItem ()));
		return true;
	}
	else if(msg == "getSelectedItems")
	{
		ObjectArray selectedItems;
		getSelectedItems (selectedItems);

		AutoPtr<ObjectArray> resultItems = NEW ObjectArray;
		resultItems->objectCleanup (true);
		resultItems->add (selectedItems, Container::kShare);
		returnValue.takeShared (resultItems->asUnknown ());
		return true;
	}
	else if(msg == "setColumnAlignment")
	{
		MutableCString id (msg[0].asString ());
		MutableCString align (msg[1].asString ());

		int columnIndex = getColumnIndex (id);
		Alignment alignment;
		if(align.contains ("right"))
			alignment.setAlignH (Alignment::kRight);

		setColumnAlignment (columnIndex, alignment);
		return true;
	}
	else if(msg == "invalidate")
	{
		invalidate ();
		return true;
	}
	else if(msg == "addTitleSorter")
	{
		String title (msg[0].asString ());
		addTitleSorter (title);
		return true;
	}
	else if(msg == "addDetailSorter")
	{
		MutableCString id (msg[0].asString ());
		MutableCString detailId (msg[1].asString ());
		String title;
		if(msg.getArgCount () > 2)
			title = msg[2].asString ();
		addDetailSorter (id, detailId, title);
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// DetailListViewSorter
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DetailListViewSorter, ListViewSorter)
MutableCString DetailListViewSorter::activeDetailId;

//////////////////////////////////////////////////////////////////////////////////////////////////

DetailListViewSorter::DetailListViewSorter (StringID columnId, StringRef title, StringID detailId)
: ListViewSorter (columnId, title, &SortByDetail),
  detailId (detailId)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DetailListViewSorter::sort (ObjectArray& items)
{
	ScopedVar<MutableCString> scope (activeDetailId, getDetailId ());
	SuperClass::sort (items);
}
