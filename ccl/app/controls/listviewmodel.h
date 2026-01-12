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
// Filename    : ccl/app/controls/listviewmodel.h
// Description : List View Model
//
//************************************************************************************************

#ifndef _ccl_listviewmodel_h
#define _ccl_listviewmodel_h

#include "ccl/app/controls/itemviewmodel.h"
#include "ccl/app/controls/listviewitem.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/gui/iparameter.h"

namespace CCL {

//************************************************************************************************
// ListViewModelBase
//************************************************************************************************

class ListViewModelBase: public ItemModel
{
public:
	DECLARE_CLASS_ABSTRACT (ListViewModelBase, ItemModel)

	ListViewModelBase ();

	// Column identifiers
	DECLARE_STRINGID_MEMBER (kIconID)
	DECLARE_STRINGID_MEMBER (kTitleID)
	DECLARE_STRINGID_MEMBER (kSubtitleID)
	DECLARE_STRINGID_MEMBER (kCheckBoxID)
	DECLARE_STRINGID_MEMBER (kEditSelectID)

	// Signals
	DECLARE_STRINGID_MEMBER (kEditItemCell) ///< args[0]: ListViewItem, args[1]: column identifier, args[2]: edit info (boxed)

	// Model options
	PROPERTY_BOOL (subtitlesEnabled, SubtitlesEnabled) ///< draw list item titles with subtitles (detail with kSubtitleID)
	PROPERTY_BOOL (keepSelectedItem, KeepSelectedItem) ///< remember first selected item when view is detached, select it again when attached
	PROPERTY_SHARED_AUTO (IParameter, editModeParam, EditModeParam)

	// Columns
	IColumnHeaderList& getColumns ();
	void setColumnAlignment (int columnIndex, AlignmentRef alignment);
	Alignment getColumnAlignment (int columnIndex) const;

	void setListViewType (Styles::ListViewType viewType);
	Styles::ListViewType getListViewType () const;

	// Helper methods related to attached ItemView:
	ListViewItem* getFocusItem () const;
	ListViewItem* getFirstSelectedItem () const;
	void getSelectedItems (Container& selected) const;
	bool isAnyItemChecked ();

	template<typename Lambda> bool visitItems (Lambda visitItem);
	template<typename Lambda> bool visitSelectedItems (Lambda visitItem);
	virtual bool getIndex (ItemIndex& index, const ListViewItem* item) const;

	// ItemModel
	tbool CCL_API getItemTitle (String& title, ItemIndexRef index) override;
	IImage* CCL_API getItemIcon (ItemIndexRef index) override;
	IImage* CCL_API getItemThumbnail (ItemIndexRef index) override;
	tbool CCL_API getItemTooltip (String& tooltip, ItemIndexRef index, int column) override;
	StringID CCL_API getItemBackground (ItemIndexRef index) override;
	tbool CCL_API canSelectItem (ItemIndexRef index) override;
	tbool CCL_API createColumnHeaders (IColumnHeaderList& list) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override;
	IUnknown* CCL_API createDragSessionData (ItemIndexRef index) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	AutoPtr<IColumnHeaderList> columns;
	Styles::ListViewType listViewType = Styles::kListViewList;
	SharedPtr<ListViewItem> savedSelectedItem;

	class ViewItemVisitor;
	template<typename Lambda> class LambdaViewItemVisitor;

	class ColumnInfo: public Object
	{
	public:
		DECLARE_CLASS (ColumnInfo, Object)
		PROPERTY_VARIABLE (Alignment, alignment, Alignment)
	};

	ColumnInfo* getColumnInfo (int column, bool create = false) const;

	StringID getColumnID (int column) const;
	int getColumnIndex (StringID id) const;

	enum ColumnType { kIconColumn, kTitleColumn, kCheckBoxColumn, kEditSelectColumn, kDetailColumn, kEmptyColumn };
	ColumnType getColumnType (CString& id, int column) const;

	virtual	ListViewItem* resolve (ItemIndexRef index) const = 0;
	virtual bool visitItemsInternal (ViewItemVisitor& itemVisitor);
	bool visitSelectedItemsInternal (ViewItemVisitor& itemVisitor);

	virtual bool editCheckBoxColumn (ItemIndexRef index, const EditInfo& info);

	// Notifications
	virtual void onSelectionChanged () {}
	virtual void onVisibleChanged (bool state) {}
	virtual void onColumnRectsChanged () {}	
};

//************************************************************************************************
// ListViewSorter
//************************************************************************************************

class ListViewSorter: public Object
{
public:
	DECLARE_CLASS (ListViewSorter, Object)

	ListViewSorter (StringID id = nullptr, StringRef title = nullptr, 
					ObjectArray::CompareFunction* sortFunction = nullptr);

	PROPERTY_MUTABLE_CSTRING (id, ID)
	PROPERTY_STRING (title, Title)
	PROPERTY_POINTER (ObjectArray::CompareFunction, sortFunction, SortFunction)
	PROPERTY_BOOL (reversed, Reversed)

	virtual void sort (ObjectArray& items);

	// Object
	bool toString (String& string, int flags = 0) const override;
};

//************************************************************************************************
// ListViewModel
//************************************************************************************************

class ListViewModel: public ListViewModelBase
{
public:
	DECLARE_CLASS (ListViewModel, ListViewModelBase)
	DECLARE_METHOD_NAMES (ListViewModel)
	DECLARE_PROPERTY_NAMES (ListViewModel)

	ListViewModel ();
	~ListViewModel ();

	// Signals
	DECLARE_STRINGID_MEMBER (kItemChecked) ///< args[0]: ListViewItem (can be null)
	DECLARE_STRINGID_MEMBER (kItemOpened) ///< args[0]: ListViewItem, args[1]:column (int)
	DECLARE_STRINGID_MEMBER (kItemFocused) ///< args[0]: ListViewItem

	// Model options
	PROPERTY_BOOL (itemRemovalEnabled, ItemRemovalEnabled) ///< can items be removed? (default is off)
	PROPERTY_BOOL (simpleItemCheck, SimpleItemCheck) ///< no swipe, modifiers to check multiple items

	// List items
	bool isEmpty () const;
	ListViewItem* getItem (int index) const;
	Iterator* newIterator () const;
	void removeAll ();
	void addItem (ListViewItem* item);
	void insertItem (int index, ListViewItem* item);
	void addSorted (ListViewItem* item);
	bool removeItem (ListViewItem* item);

	void enableAll (bool state);
	void checkAll (bool state, bool onlyEnabled = true);
	void checkAlone (int index, bool state, bool onlyEnabled = true);

	// Sorting
	void addSorter (ListViewSorter* sorter);
	void addTitleSorter (StringRef sorterTitle = nullptr);
	void addDetailSorter (StringID columnId, StringID detailId, StringRef sorterTitle = nullptr);
	ListViewSorter* getActiveSorter () const;
	ListViewSorter* getSorterWithID (StringID id) const;
	void sortBy (ListViewSorter* sorter);
	void resort ();
	
	// Helper methods related to attached ItemView
	int getFirstSelectedIndex () const;
	bool canRemoveSelectedItems () const;
	void removeSelectedItems ();

	virtual bool removeItems (ItemIndexRef index, const IItemSelection& selection); ///< remove multiple items (not in IItemModel)

	// ListViewModelBase
	bool getIndex (ItemIndex& index, const ListViewItem* item) const override;
	int CCL_API countFlatItems () override;
	tbool CCL_API canRemoveItem (ItemIndexRef index) override;
	tbool CCL_API removeItem (ItemIndexRef index) override;
	tbool CCL_API openItem (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API getSortColumnID (MutableCString& columnID, tbool& upwards) override;
	tbool CCL_API measureCellContent (Rect& size, ItemIndexRef index, int column, const StyleInfo& info) override;
	tbool CCL_API onItemFocused (ItemIndexRef index) override;
	tbool CCL_API interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API getItemAccessibilityInfo (AccessibilityInfo& info, ItemIndexRef index, int column) const override;

protected:
	class CheckBoxMouseHandler;

	ObjectArray items;
	ObjectArray sorters;
	SharedPtr<ListViewSorter> activeSorter;
	ItemIndex anchorIndex;

	bool getMultiSelectionRange (ItemIndex& fromIndex, ItemIndex& toIndex, bool& toggle, const MouseEvent& mouseEvent);
	void setAnchorItem (ListViewItem* item);

	/** Helper to handle editCell() for item checked state. */
	bool editCheckBoxColumn (ItemIndexRef index, const EditInfo& info) override;

	// Notifications
	virtual void onSortColumnChanged (StringID columnID, bool upwards);
	virtual void onItemChecked (ListViewItem* item); // 0: multiple items might have changed

	// ListViewModelBase
	ListViewItem* resolve (ItemIndexRef index) const override;
	bool visitItemsInternal (ViewItemVisitor& itemVisitor) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// ListViewModelBase::ViewItemVisitor
//************************************************************************************************

class ListViewModelBase::ViewItemVisitor
{
public:
	virtual bool visit (ListViewItem& item) const = 0; ///< return false to stop traversal
};

//************************************************************************************************
// ListViewModelBase::LambdaItemVisitor
//************************************************************************************************

template<typename Lambda>
class ListViewModelBase::LambdaViewItemVisitor: public ViewItemVisitor
{
public:
	LambdaViewItemVisitor (const Lambda& visitItem)
	: visitItem (visitItem)
	{}

	// ItemVisitor
	bool visit (ListViewItem& item) const override { return visitItem (item); }

private:
	Lambda visitItem;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<typename Lambda>
inline bool ListViewModelBase::visitItems (Lambda visitItem)
{ LambdaViewItemVisitor<Lambda> v (visitItem); return visitItemsInternal (v); }

template<typename Lambda>
inline bool ListViewModelBase::visitSelectedItems (Lambda visitItem)
{ LambdaViewItemVisitor<Lambda> v (visitItem); return visitSelectedItemsInternal (v); }

} // namespace CCL

#endif // _ccl_listviewmodel_h
