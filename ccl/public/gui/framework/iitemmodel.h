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
// Filename    : ccl/public/gui/framework/iitemmodel.h
// Description : Item Model Interface
//
//************************************************************************************************

#ifndef _ccl_iitemmodel_h
#define _ccl_iitemmodel_h

#include "ccl/public/gui/framework/ilistview.h"
#include "ccl/public/gui/framework/itreeview.h"
#include "ccl/public/gui/framework/viewbox.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/collections/linkedlist.h"

namespace CCL {

struct CommandMsg;
interface IContextMenu;
interface IItemView;
interface IMouseHandler;
interface ITouchHandler;
interface IDragSession;
interface IDragHandler;
interface IItemDragVerifier;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (ColumnHeaderList, 0xE0C5B54B, 0xBAA3, 0x4DAA, 0xBE, 0x2D, 0xE1, 0x4C, 0xB5, 0x0D, 0x56, 0x13);
	DEFINE_CID (ItemListSelection, 0x7764797A, 0xB532, 0x48E3, 0x98, 0x1A, 0x22, 0x74, 0x91, 0x70, 0x0C, 0x61);
};

//************************************************************************************************
// IColumnHeaderList
/** Interface for column header list. 
	\ingroup gui_item */
//************************************************************************************************

interface IColumnHeaderList: IUnknown
{
	enum ColumnFlags
	{
		kSizable  = 1<<0,
		kMoveable = 1<<1,
		kFill	  = 1<<2,
		kHideable = 1<<3,
		kHidden	  = 1<<4,
		kSortable = 1<<5,
		kCanFit   = 1<<6,
		kEditMode = 1<<7,	///< column only appears in edit mode
		kCanEditMultiple = 1<<8, ///< leaves the current selection unchanged
		kCentered = 1 << 9  ///< draw title centered
	};

	enum { kAutoWidth = -1 }; ///< specify as width in addColumn to calculate required width

	/** Add column. */
	virtual void CCL_API addColumn (int width, StringRef title = nullptr, StringID id = nullptr, int minWidth = 0, int flags = 0) = 0;

	/** Copy columns from other list. */
	virtual void CCL_API copyFrom (const IColumnHeaderList& other) = 0;

	/** Get identifier of column with given index. */
	virtual StringID CCL_API getColumnID (int columnIndex) const = 0;
	
	/** Get number of columns. */
	virtual int CCL_API getColumnCount () const = 0;

	/** Remove all columns. */
	virtual void CCL_API removeAll () = 0;

	/** Resize column. */
	virtual void CCL_API setColumnWidth (StringID column, int width) = 0;
	
	/** Hide column. */
	virtual void CCL_API hideColumn (StringID column, tbool state) = 0;

	/** Move column. */
	virtual void CCL_API moveColumn (StringID column, int newVisiblePosition) = 0;

	/** Set user data for column. */
	virtual tbool CCL_API setColumnDataAt (int columnIndex, VariantRef data) = 0;

	/** Get user data for column. */
	virtual tbool CCL_API getColumnDataAt (Variant& data, int columnIndex) const = 0;
	
	/** Can be send by IItemView or IItemModel when sort column changed. */
	DECLARE_STRINGID_MEMBER (kSortColumnChanged)

	/** Send by IItemView when column coordinates on screen changed. */
	DECLARE_STRINGID_MEMBER (kColumnRectsChanged)

	DECLARE_IID (IColumnHeaderList)
};

DEFINE_IID (IColumnHeaderList, 0xdc299b1a, 0x2b1c, 0x497a, 0xa4, 0x68, 0x75, 0xdd, 0xe2, 0x26, 0x2c, 0xad)
DEFINE_STRINGID_MEMBER (IColumnHeaderList, kSortColumnChanged, "sortColumnChanged")
DEFINE_STRINGID_MEMBER (IColumnHeaderList, kColumnRectsChanged, "columnRectsChanged")

//************************************************************************************************
// ItemIndex
/** Item identifier (object or integer). 
	\ingroup gui_item */
//************************************************************************************************

class ItemIndex
{
public:
	ItemIndex (IUnknown* object = nullptr);
	ItemIndex (ITreeItem* treeItem);
	ItemIndex (int index);
	ItemIndex (VariantRef variant);
	
	IUnknown* getObject () const;
	ITreeItem* getTreeItem () const;
	bool getIndex (int& index) const;
	int getIndex () const;
	bool isValid () const;

	const ItemIndex& toVariant (Variant& variant) const;
	ItemIndex& fromVariant (VariantRef variant);

	bool operator == (const ItemIndex& index) const;
	bool operator != (const ItemIndex& index) const;

protected:
	enum Type { kIndex, kObject, kTreeItem };
	short type;
	union
	{
		IntPtr index;
		IUnknown* object;
		ITreeItem* treeItem;
	};
};

/** Item index reference. */
typedef const ItemIndex& ItemIndexRef;

//************************************************************************************************
// IItemSelectionIterator
/** Item Selection iteration interface. 
	\ingroup gui_item */
//************************************************************************************************

interface IItemSelectionIterator: IUnknown
{
	/** Get next ItemIndex. Returns false if iteration is finished. */
	virtual tbool CCL_API next (ItemIndex& index) = 0;

	DECLARE_IID (IItemSelectionIterator)
};

DEFINE_IID (IItemSelectionIterator, 0x890b36af, 0x9253, 0x402e, 0xbf, 0x13, 0xd9, 0xe7, 0xfc, 0x91, 0x87, 0x8d)

//////////////////////////////////////////////////////////////////////////////////////////////////
// ForEachItem : iterate trough selection
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ForEachItem(selection, indexVar) \
{ CCL::AutoPtr<CCL::IItemSelectionIterator> __iter ((selection).newIterator ()); \
  CCL::ItemIndex indexVar;\
  if(__iter) while(__iter->next (indexVar)) {

//************************************************************************************************
// IItemSelection
/** Item Selection interface. 	
	\ingroup gui_item */
//************************************************************************************************

interface IItemSelection: IUnknown
{
	/** Clone selection. */
	virtual void CCL_API clone (IItemSelection*& selection) const = 0;

	/** Check if selection is empty. */
	virtual tbool CCL_API isEmpty () const = 0;

	/** Check if multiple items are selected. */
	virtual tbool CCL_API isMultiple () const = 0;

	/** Check if item is selected. */
	virtual tbool CCL_API isSelected (ItemIndexRef index) const = 0;

	/** Create an iterator over the selected items. */
	virtual IItemSelectionIterator* CCL_API newIterator () const = 0;

	/** Add an item to the selection. */
	virtual void CCL_API select (ItemIndexRef index) = 0;

	/** Remove an item from the selection. */
	virtual tbool CCL_API unselect (ItemIndexRef index) = 0;

	/** Unselect all items. */
	virtual void CCL_API unselectAll () = 0;

	DECLARE_IID (IItemSelection)
};

DEFINE_IID (IItemSelection, 0x21e9fd2, 0xfe9a, 0x4f98, 0x98, 0x41, 0xba, 0xc, 0xce, 0x2, 0x56, 0x4a)

//************************************************************************************************
// IItemModel
/** Model interface for list and tree controls. 
	\ingroup gui_item */
//************************************************************************************************

interface IItemModel: IUnknown
{
	/** Style information. */
	struct StyleInfo
	{
		const Font& font;
		const Brush& textBrush;
		const Brush& backBrush;
		const Color& adaptiveColor;

		SolidBrush getTextBrush (bool enabled) const
		{
			SolidBrush brush2 (textBrush);
			if(enabled == false)
				brush2.blendBrushColor (backBrush, 0.5f);
			return brush2;
		}
	};

	/** Draw information. */
	struct DrawInfo
	{
		enum States
		{
			kItemSelectedState = 1<<0
		};

		IView* view;
		IGraphics& graphics;
		const Rect& rect;
		const StyleInfo& style;
		int state;		
	};

	/** Edit information. */
	struct EditInfo
	{
		IView* view;
		const Rect& rect;
		const StyleInfo& style;
		const GUIEvent& editEvent;
	};

	/** Accessibility information. */
	struct AccessibilityInfo
	{
		String name;
		int role;
		String value;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////
	/** \name Messages 
	    Message ids that can be sent by an IItemModel (in addition to the unspecific kChanged) */
	//////////////////////////////////////////////////////////////////////////////////////////////
	///@{
	DECLARE_STRINGID_MEMBER (kItemAdded)	///< arg[0] is the ItemIndex
	DECLARE_STRINGID_MEMBER (kItemRemoved)	///< arg[0] is the ItemIndex
	DECLARE_STRINGID_MEMBER (kItemModified)	///< arg[0] is the ItemIndex
	DECLARE_STRINGID_MEMBER (kNewRootItem)
	DECLARE_STRINGID_MEMBER (kUpdateColumns)
	///@}

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Backlink to view
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** View starts to use the model. */
	virtual void CCL_API viewAttached (IItemView* itemView) = 0;

	/** View ends to use the model. */
	virtual void CCL_API viewDetached (IItemView* itemView) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Item access
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get item count of (flat) list. */
	virtual int CCL_API countFlatItems () = 0;

	/** Get tree root item. */
	virtual tbool CCL_API getRootItem (ItemIndex& index) = 0;

	/** Check if item is a folder. */
	virtual tbool CCL_API isItemFolder (ItemIndexRef index) = 0;

	/** Check if item can be expanded. */
	virtual tbool CCL_API canExpandItem (ItemIndexRef index) = 0;

	/** Check if item can be expanded automatically on click (if tree view has style kTreeViewAutoExpand). */
	virtual tbool CCL_API canAutoExpandItem (ItemIndexRef index) = 0;

	/** Get children of given item. */
	virtual tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index) = 0;

	/** Get special item selection implementation provided by model. Return null to use default selection of view. */
	virtual IItemSelection* CCL_API getSelection () = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Item attributes
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get title of specified item. */
	virtual tbool CCL_API getItemTitle (String& title, ItemIndexRef index) = 0;
	
	/** Get a unique name (unique among siblings) of specified item. Must not contain a '/'. Used for storing expand states of items. */
	virtual tbool CCL_API getUniqueItemName (MutableCString& name, ItemIndexRef index) = 0;

	/** Get icon of specified item. */
	virtual IImage* CCL_API getItemIcon (ItemIndexRef index) = 0;

	/** Get thumbnail of specified item. */
	virtual IImage* CCL_API getItemThumbnail (ItemIndexRef index) = 0;

	/** Get tooltip of specified item. */
	virtual tbool CCL_API getItemTooltip (String& tooltip, ItemIndexRef index, int column) = 0;

	/** Check if item can be selected. */
	virtual tbool CCL_API canSelectItem (ItemIndexRef index) = 0;

	/** Get accessibility info. */
	virtual tbool CCL_API getItemAccessibilityInfo (AccessibilityInfo& info, ItemIndexRef index, int column) const = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Item interaction
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Item was focused. */
	virtual tbool CCL_API onItemFocused (ItemIndexRef index) = 0;

	/** Item was double-clicked, or [Enter] key was pressed. */
	virtual tbool CCL_API openItem (ItemIndexRef index, int column, const EditInfo& info) = 0;

	/** Check if item can be removed. */
	virtual tbool CCL_API canRemoveItem (ItemIndexRef index) = 0;

	/** Item should be removed (e.g. delete key pressed or dropped outside during drag session). */
	virtual tbool CCL_API removeItem (ItemIndexRef index) = 0;

	/** Check if data can be inserted (during drag session). */
	virtual tbool CCL_API canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView = nullptr) = 0;
	
	/** Insert data (during drag session). */
	virtual tbool CCL_API insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session) = 0;

	/** Edit cell. */
	virtual tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Item painting
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Draw cell in list or tree view. */
	virtual tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) = 0;
	
	/** Draw overlay on top of item icon (optional). */
	virtual tbool CCL_API drawIconOverlay (ItemIndexRef index, const DrawInfo& info) = 0;

	/** Get an optional background color id for an item. It is looked up in the visual style of the ItemView. */
	virtual StringID CCL_API getItemBackground (ItemIndexRef index) = 0;

	/** Get size of cell content. */
	virtual tbool CCL_API measureCellContent (Rect& size, ItemIndexRef index, int column, const StyleInfo& info) = 0;

	/** Draw custom item, i.e. framework draws nothing (requires kListViewDrawCustomItem style). */
	virtual tbool CCL_API drawItem (ItemIndexRef index, const DrawInfo& info) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Other
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Create column headers. */
	virtual tbool CCL_API createColumnHeaders (IColumnHeaderList& list) = 0;

	/** Get identifier of current sort column. */
	virtual tbool CCL_API getSortColumnID (MutableCString& columnID, tbool& upwards) = 0;

	/** Create object for dragging item data. */
	virtual IUnknown* CCL_API createDragSessionData (ItemIndexRef index) = 0;

	/** Append context menu for selected items. */
	virtual tbool CCL_API appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection) = 0;

	/** Interpret commands for selected items. */
	virtual tbool CCL_API interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection) = 0;

	/** Create touch handler. */
	virtual ITouchHandler* CCL_API createTouchHandler (ItemIndexRef index, int column, const EditInfo& info) = 0;

	DECLARE_IID (IItemModel)
};

DEFINE_IID (IItemModel, 0x5a8661f7, 0xcc02, 0x4432, 0xbb, 0xcd, 0x98, 0xf9, 0x92, 0x9d, 0x9c, 0xfa)
DEFINE_STRINGID_MEMBER (IItemModel, kItemAdded, "itemAdded")
DEFINE_STRINGID_MEMBER (IItemModel, kItemRemoved, "itemRemoved")
DEFINE_STRINGID_MEMBER (IItemModel, kItemModified, "itemModified")
DEFINE_STRINGID_MEMBER (IItemModel, kNewRootItem, "newRootItem")
DEFINE_STRINGID_MEMBER (IItemModel, kUpdateColumns, "updateColumns")

//************************************************************************************************
// IItemDragTarget
/** 
	\ingroup gui_item */
//************************************************************************************************

interface IItemDragTarget: CCL::IUnknown
{
	/** Flags for drag handler. */
	enum ItemDragFlags
	{
		kCanDragBetweenItems = 1<<0,	///< drag between items enabled (implies insert indicator)
		kCanDragOnItem       = 1<<1,	///< drag on item enabled (implies insert indicator)
		kDropInsertsData     = 1<<2,	///< call IItemModel::insertData() on drop (needs an IItemView)
		kDragWithItemIcon    = 1<<3,	///< show item icon during drag (needs IItemModel::getItemIcon())
		kCanDragPrePostItems = 1<<4 	///< drag before or last item permitted
	};

	/** Creates a default drag handler with optional insert indicator and drop handling. */
	virtual IDragHandler* CCL_API createDragHandler (int flags = kCanDragBetweenItems, IItemDragVerifier* verifier = nullptr) = 0;

	DECLARE_IID (IItemDragTarget)
};

DEFINE_IID (IItemDragTarget, 0x88a2326a, 0x6dc6, 0x4fd8, 0xa6, 0x6b, 0xfe, 0x85, 0xd2, 0x54, 0xfd, 0xf2)

//************************************************************************************************
// IItemView
/** View interface for list and tree controls. 
	\ingroup gui_item */
//************************************************************************************************

interface IItemView: IItemDragTarget
{
	/** Associate model with view (model is shared). */
	virtual void CCL_API setModel (IItemModel* model) = 0;

	/** Get associated model. */
	virtual IItemModel* CCL_API getModel () const = 0;

	/** Get selection. */
	virtual const IItemSelection& CCL_API getSelection () const = 0;

	/** Select or unselect an item. */
	virtual tbool CCL_API selectItem (ItemIndexRef index, tbool state) = 0;

	/** Select or unselect all items. */
	virtual tbool CCL_API selectAll (tbool state) = 0;

	/** Remove an item. */
	virtual tbool CCL_API removeItem (ItemIndexRef index) = 0;

	/** Find all items in a rectangle. */
	virtual tbool CCL_API findItems (const Rect& rect, IItemSelection& items) const = 0;

	/** Find an item at a position. */
	virtual tbool CCL_API findItem (ItemIndex& index, const Point& where) const = 0;

	/** Get Rectangle of item. */
	virtual void CCL_API getItemRect (Rect& rect, ItemIndexRef index, int column = -1) const = 0;

	/** Get selected item. */
	virtual tbool CCL_API getFocusItem (ItemIndex& index) const = 0;

	/** Select specified item. */
	virtual tbool CCL_API setFocusItem (ItemIndexRef index, tbool selectExclusive = true) = 0;

	/** Invalidate specified item. */
	virtual tbool CCL_API invalidateItem (ItemIndexRef index) = 0;

	/** Try to make an item visible by scrolling. */
	virtual void CCL_API makeItemVisible (ItemIndexRef index) = 0;

	/** Set an edit control (owned by the item view). */
	virtual void CCL_API setEditControl (IView* view, tbool directed = true) = 0;

	/** Set a parameter that controls the items view's edit mode. */
	virtual void CCL_API setEditModeParam (IParameter* parameter) = 0;

	/** Begin mouse handler, can be called from IItemModel::editCell(). */
	virtual void CCL_API beginMouseHandler (IMouseHandler* handler, const MouseEvent& mouseEvent) = 0;

	/** Find an item cell at a position. */
	virtual tbool CCL_API findItemCell (ItemIndex& row, int& column, const Point& where) const = 0;

	DECLARE_STRINGID_MEMBER (kSelectionChanged)
	DECLARE_STRINGID_MEMBER (kViewAttached)
	DECLARE_STRINGID_MEMBER (kViewRemoved)
	DECLARE_STRINGID_MEMBER (kViewFocused)
	DECLARE_STRINGID_MEMBER (kDragSessionStart)	///< arg[0]: IDragSession
	DECLARE_STRINGID_MEMBER (kDragSessionDone)  ///< arg[0]: IDragSession

	DECLARE_IID (IItemView)
};

DEFINE_IID (IItemView, 0x2e0176f9, 0x47d7, 0x4046, 0x9d, 0xdb, 0xd5, 0x25, 0x6c, 0x57, 0x2f, 0xcf)
DEFINE_STRINGID_MEMBER (IItemView, kSelectionChanged, "selectionChanged")
DEFINE_STRINGID_MEMBER (IItemView, kViewAttached,     "viewAttached")
DEFINE_STRINGID_MEMBER (IItemView, kViewRemoved,      "viewRemoved")
DEFINE_STRINGID_MEMBER (IItemView, kViewFocused,      "viewFocused")
DEFINE_STRINGID_MEMBER (IItemView, kDragSessionStart, "itemDragStart")
DEFINE_STRINGID_MEMBER (IItemView, kDragSessionDone,  "itemDragDone")

//************************************************************************************************
// IItemViewDragHandler
/**
	\ingroup gui_item
	\ingroup gui_data */
//************************************************************************************************

interface IItemViewDragHandler: CCL::IUnknown
{
	/** Get target item and relation. */
	virtual tbool CCL_API getTarget (ItemIndex& item, int& relation) = 0;

	enum ItemRelation
	{
		kOnItem,
		kBeforeItem,
		kAfterItem,
		kBeforeOrAfterItem,	///< set by an IItemDragVerifier: decide using mouse position
		kFullView			///< set by an IItemDragVerifier: display sprite on full itemview
	};

	DECLARE_IID (IItemViewDragHandler)
};

DEFINE_IID (IItemViewDragHandler, 0x89328FC9, 0xAF8E, 0x4258, 0xBF, 0xD0, 0x95, 0x95, 0x7A, 0xBE, 0x25, 0xAF)

//************************************************************************************************
// IItemDragVerifier
/**
	\ingroup gui_item
	\ingroup gui_data */
//************************************************************************************************

interface IItemDragVerifier: CCL::IUnknown
{
	/** Verify if the given target is accepted. Can adjust item and relation. */
	virtual tbool CCL_API verifyTargetItem (ItemIndex& item, int& relation) = 0;

	DECLARE_IID (IItemDragVerifier)
};

DEFINE_IID (IItemDragVerifier, 0xd6711695, 0xb75d, 0x4bfc, 0x83, 0xc7, 0x8, 0xce, 0xa0, 0x5c, 0x92, 0x70)

//************************************************************************************************
// AbstractItemModel
/** Base class for model implementation. 
	\ingroup gui_item */
//************************************************************************************************

class AbstractItemModel: public IItemModel
{
public:	
	// View backlink
	void CCL_API viewAttached (IItemView* itemView) override
	{}
	
	void CCL_API viewDetached (IItemView* itemView) override
	{}
	
	// Item access
	int CCL_API countFlatItems () override
	{ 
		return 0; 
	}

	tbool CCL_API getRootItem (ItemIndex& index) override
	{
		return false;
	}
	
	tbool CCL_API isItemFolder (ItemIndexRef index) override
	{ 
		return false; 
	}

	tbool CCL_API canExpandItem (ItemIndexRef index) override
	{ 
		return false; 
	}
	
	tbool CCL_API canAutoExpandItem (ItemIndexRef index) override
	{ 
		return true; 
	}

	tbool CCL_API getSubItems (IUnknownList& items, ItemIndexRef index) override
	{ 
		return false; 
	}
			
	IItemSelection* CCL_API getSelection () override
	{
		return nullptr;
	}

	// Item attributes
	tbool CCL_API getItemTitle (String& title, ItemIndexRef index) override
	{ 
		return false; 
	}

	tbool CCL_API getUniqueItemName (MutableCString& name, ItemIndexRef index) override
	{
		return false;
	}
	
	IImage* CCL_API getItemIcon (ItemIndexRef index) override
	{ 
		return nullptr; 
	}

	IImage* CCL_API getItemThumbnail (ItemIndexRef index) override
	{
		return nullptr;
	}

	tbool CCL_API getItemTooltip (String& tooltip, ItemIndexRef index, int column) override
	{
		return false;
	}

	tbool CCL_API canSelectItem (ItemIndexRef index) override
	{
		return true;
	}

	tbool CCL_API getItemAccessibilityInfo (AccessibilityInfo& info, ItemIndexRef index, int column) const override
	{
		return false;
	}

	// Item actions
	tbool CCL_API onItemFocused (ItemIndexRef index) override
	{
		return false;
	}

	tbool CCL_API openItem (ItemIndexRef index, int column, const EditInfo& info) override
	{
		return false;
	}
	
	tbool CCL_API canRemoveItem (ItemIndexRef index) override
	{
		return false;
	}

	tbool CCL_API removeItem (ItemIndexRef index) override
	{
		return false;
	}
	
	tbool CCL_API canInsertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session, IView* targetView = nullptr) override
	{
		return false;
	}
	
	tbool CCL_API insertData (ItemIndexRef index, int column, const IUnknownList& data, IDragSession* session) override
	{
		return false;
	}

	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override
	{ 
		return false; 
	}

	// Item painting
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override
	{ 
		return false; 
	}

	tbool CCL_API drawItem (ItemIndexRef index, const DrawInfo& info) override
	{
		return false;
	}
	
	tbool CCL_API drawIconOverlay (ItemIndexRef index, const DrawInfo& info) override
	{
		return false;
	}

	StringID CCL_API getItemBackground (ItemIndexRef index) override
	{
		return CString::kEmpty;
	}
	
	tbool CCL_API measureCellContent (Rect& size, ItemIndexRef index, int column, const StyleInfo& info) override
	{
		return false;
	}
	
	// Other
	tbool CCL_API createColumnHeaders (IColumnHeaderList& list) override
	{ 
		return false; 
	}

	tbool CCL_API getSortColumnID (MutableCString& columnID, tbool& upwards) override
	{
		return false;
	}

	IUnknown* CCL_API createDragSessionData (ItemIndexRef index) override
	{ 
		IUnknown* u = index.getObject (); 
		if(u) u->retain (); 
		return u; 
	}

	tbool CCL_API appendItemMenu (IContextMenu& menu, ItemIndexRef item, const IItemSelection& selection) override
	{ 
		return false; 
	}

	tbool CCL_API interpretCommand (const CommandMsg& msg, ItemIndexRef item, const IItemSelection& selection) override
	{
		return false;
	}

	ITouchHandler* CCL_API createTouchHandler (ItemIndexRef index, int column, const EditInfo& info) override
	{
		return nullptr;
	}
};

//************************************************************************************************
// ItemViewObserver
/** Mixin class for model implementation, maintains pointers to multiple attached itemViews. 
	\ingroup gui_item */
//************************************************************************************************

template<class ItemModelBase>
class ItemViewObserver: public ItemModelBase
{
public:
	ItemViewObserver ()
	{}

	IItemView* getItemView () const { return views.isEmpty () ? nullptr : views.getFirst (); }
	const auto& getItemViews () const { return views; }

	IItemView* getItemViewByName (StringRef name)
	{
		ListForEach (views, IItemView*, itemView)
			if(ViewBox (itemView).getName () == name)
				return itemView;
		EndFor
		return 0;
	}

	void makeFirst (IItemView* itemView) 
	{
		if(views.getFirst () != itemView)
			if(views.remove (itemView))
				views.prepend (itemView);
	}

	void makeLast (IItemView* itemView) 
	{
		if(views.getLast () != itemView)
			if(views.remove (itemView))
				views.append (itemView);
	}

	// IItemModel
	void CCL_API viewAttached (IItemView* itemView) override { views.append (itemView); }
	void CCL_API viewDetached (IItemView* itemView) override { views.remove (itemView); }

private:
	LinkedList<IItemView*> views;
};

//************************************************************************************************
// AbstractItemSelection
//************************************************************************************************

class AbstractItemSelection: public IItemSelection
{
public:	
	// IItemSelection
	void CCL_API clone (IItemSelection*& selection) const override	{}
	tbool CCL_API isEmpty () const override							{ return true; }
	tbool CCL_API isMultiple () const override						{ return false; }
	tbool CCL_API isSelected (ItemIndexRef index) const override	{ return false; }
	IItemSelectionIterator* CCL_API newIterator () const override	{ return nullptr; }
	void CCL_API select (ItemIndexRef index) override				{}
	tbool CCL_API unselect (ItemIndexRef index) override			{ return false; }
	void CCL_API unselectAll () override							{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ItemIndex inline
//////////////////////////////////////////////////////////////////////////////////////////////////
		
inline ItemIndex::ItemIndex (IUnknown* object)
: type (kObject), object (object) {}

inline ItemIndex::ItemIndex (ITreeItem* treeItem)
: type (kTreeItem), treeItem (treeItem) {}

inline ItemIndex::ItemIndex (int index)
: type (kIndex), index (index) {}

inline ItemIndex::ItemIndex (VariantRef variant)
{ fromVariant (variant); }

inline IUnknown* ItemIndex::getObject () const
{ return type == kObject ? object : type == kTreeItem && treeItem ? treeItem->getData () : nullptr; }

inline ITreeItem* ItemIndex::getTreeItem () const
{ return type == kTreeItem ? treeItem : nullptr; }

inline int ItemIndex::getIndex () const
{ return type == kIndex ? (int)index : -1; }

inline bool ItemIndex::getIndex (int& index) const
{ if(type == kIndex) { index = (int)this->index; return true; } return false; }

inline bool ItemIndex::isValid () const
{ return type == kObject ? object != nullptr : type == kTreeItem ? treeItem != nullptr : index >= 0; }

inline const ItemIndex& ItemIndex::toVariant (Variant& variant) const
{ variant = type == kIndex ? Variant (index) : Variant (object); return *this; }

inline ItemIndex& ItemIndex::fromVariant (VariantRef variant)
{
	if(variant.getType () == Variant::kInt)
		return *this = ItemIndex (variant.asInt ());
	UnknownPtr<ITreeItem> treeItem (variant.asUnknown ());
	if(treeItem)
		return *this = ItemIndex (treeItem.as_plain ());
	return *this = ItemIndex (variant.asUnknown ());
}

inline bool ItemIndex::operator == (const ItemIndex& index) const
{
	if(type == index.type)
		return object == index.object;

	else if((type == kObject || type == kTreeItem) && (index.type == kObject || index.type == kTreeItem))
		return getObject () == index.getObject ();
	
	return false;
}

inline bool ItemIndex::operator != (const ItemIndex& index) const
{ return ! (operator == (index)); }


//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_iitemmodel_h
