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
// Filename    : ccl/gui/itemviews/itemviewbase.h
// Description : Basic Item View
//
//************************************************************************************************

#ifndef _ccl_itemviewbase_h
#define _ccl_itemviewbase_h

#include "ccl/gui/views/scrollview.h"
#include "ccl/gui/itemviews/namenavigator.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/iitemmodel.h"

namespace CCL {

//************************************************************************************************
// ItemViewBase
/** Base class for views using IItemModel. */
//************************************************************************************************

class ItemViewBase: public View,
					public IItemView,
					public INamedItemIterator
{
public:
	DECLARE_CLASS (ItemViewBase, View)
	DECLARE_METHOD_NAMES (ItemViewBase)

	ItemViewBase (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~ItemViewBase ();

	virtual void onDragOverItem (const DragEvent& event, ItemIndexRef index);
	void disableTouchHandler (bool disable);

	void setEditMode (bool state);
	
	// IItemView
	void CCL_API setModel (IItemModel* model) override;
	IItemModel* CCL_API getModel () const override;
	const IItemSelection& CCL_API getSelection () const override;
	tbool CCL_API selectItem (ItemIndexRef index, tbool state) override;
	tbool CCL_API selectAll (tbool state) override;
	tbool CCL_API removeItem (ItemIndexRef index) override;
	tbool CCL_API findItems (const Rect& rect, IItemSelection& items) const override;
	tbool CCL_API findItem (ItemIndex& index, const Point& where) const override;
	void CCL_API getItemRect (Rect& rect, ItemIndexRef index, int column = -1) const override;
	tbool CCL_API getFocusItem (ItemIndex& index) const override;
	tbool CCL_API setFocusItem (ItemIndexRef index, tbool selectExclusive = true) override;
	tbool CCL_API invalidateItem (ItemIndexRef index) override;
	void CCL_API makeItemVisible (ItemIndexRef index) override;
	void CCL_API setEditControl (IView* view, tbool directed = true) override;
	void CCL_API setEditModeParam (IParameter* parameter) override;
	void CCL_API beginMouseHandler (IMouseHandler* handler, const MouseEvent& mouseEvent) override;
	IDragHandler* CCL_API createDragHandler (int flags = kCanDragBetweenItems, IItemDragVerifier* verifier = nullptr) override;
	tbool CCL_API findItemCell (ItemIndex& row, int& column, const Point& where) const override;

	// INamedItemIterator
	tbool CCL_API getStartItem (Variant& item, String& name) override;
	tbool CCL_API getNextItem (Variant& item, String& name) override;

	// View
	void calcAutoSize (Rect& r) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	void onSize (const Point& delta) override;
	void attached (View* parent) override;
	void removed (View* parent) override;
	bool onDragEnter (const DragEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	ITouchHandler* createTouchHandler (const TouchEvent& event) override;

	CLASS_INTERFACE3 (IItemView, IItemDragTarget, INamedItemIterator, View)

protected:
	IItemModel* model;
	mutable IItemSelection* selection;
	NameNavigator nameNavigator;
	SharedPtr<IParameter> editModeParam;

	struct SizeInfo
	{
		Coord width;
		Coord height;
		Coord hSnap;
		Coord vSnap;

		SizeInfo ()
		: width (0), height (0),
		  hSnap (1), vSnap (1)
		{}
	};
	SizeInfo sizeInfo;

	class ItemDragHandler;
	class InsertDataDragHandler;

	enum PrivateFlags
	{
		kOpenItemCalled			= 1<<(kLastPrivateFlag + 1),	///< to distinguish if editCell or openItem was called last
		kSuspendSelectSignal	= 1<<(kLastPrivateFlag + 2),	///< can be set by derived class to suspend IItemView::kSelectionChanged
		kEditMode				= 1<<(kLastPrivateFlag + 3),	///< general edit mode
		kDeleteFocusItemMode	= 1<<(kLastPrivateFlag + 4),	///< special mode waiting for confirmation to delete the current focus item
		kTouchHandlerDisabled	= 1<<(kLastPrivateFlag + 5),
		kItemViewBaseLastPrivateFlag = kLastPrivateFlag + 5
	};

	/// change types for modelChanged
	enum { kModelChanged, kItemAdded, kItemRemoved, kItemModified, kModelAssigned};

	friend class ItemControlBase;
	void updateSize (bool recalc = true);
	void signalSelectionChanged ();
	IItemModel::EditInfo makeEditInfo (RectRef rect, const GUIEvent& editEvent);

	// edit mode
	void setDeleteFocusItemMode (bool state, ItemIndexRef item = ItemIndex ());
	PROPERTY_FLAG (privateFlags, kEditMode, isEditMode)
	PROPERTY_FLAG (privateFlags, kDeleteFocusItemMode, isDeleteFocusItemMode)

	/// to be implemented by subclass
	virtual Font& getFont (Font& font) const;
	virtual bool openItem (ItemIndexRef item, int column, const GUIEvent& editEvent, RectRef rect = Rect ());
	virtual tbool editCell (ItemIndexRef item, int column, RectRef rect, const GUIEvent& editEvent);
	virtual void getSizeInfo (SizeInfo& info);
	virtual void modelChanged (int changeType, ItemIndexRef item);
	virtual IItemSelection* createSelection () const; ///< override to use another selection, default is ItemListSelection
	virtual void onItemFocused (ItemIndexRef item);
	virtual bool getNextItem (ItemIndex& item, bool forNavigation = true); ///< advance to the next item; forNavigation: only selectable items, wrap around at the end, or if item is invalid
	virtual void onEditModeChanged (bool state);
	virtual IImage* getDragImageForItem (ItemIndexRef item);

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// ItemControlBase
/** Base class for scrollable item views. */
//************************************************************************************************

class ItemControlBase: public ScrollView
{
public:
	DECLARE_CLASS (ItemControlBase, ScrollView)

	ItemControlBase (const Rect& size = Rect (), 
					 ItemViewBase* itemView = nullptr,
					 StyleRef scrollViewStyle = Styles::kScrollViewAppearanceScrollBars);

	ItemViewBase* getItemView () const;

	// ScrollView
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;
	void setStyle (StyleRef newStyle) override;
	void onVisualStyleChanged () override;
	void setTheme (Theme* theme) override;
	void setName (StringRef name) override;
	void setTitle (StringRef title) override;
	void onSize (const Point& delta) override;
	void CCL_API setZoomFactor (float factor) override;
	tbool CCL_API takeFocus (tbool directed = true) override;
};

//************************************************************************************************
// ItemListSelection
/** Canonical Implementation of IItemSelection as a linked list of ItemIndex objects. */
//************************************************************************************************

class ItemListSelection: public Object,
						 public IItemSelection
{
public:
	DECLARE_CLASS (ItemListSelection, Object)
	
	ItemListSelection ();
	ItemListSelection (const ItemListSelection& selection);
	
	// IItemSelection
	void CCL_API clone (IItemSelection*& selection) const override;
	tbool CCL_API isEmpty () const override;
	tbool CCL_API isMultiple () const override;
	tbool CCL_API isSelected (ItemIndexRef index) const override;
	IItemSelectionIterator* CCL_API newIterator () const override;
	void CCL_API select (ItemIndexRef index) override;
	tbool CCL_API unselect (ItemIndexRef index) override;
	void CCL_API unselectAll () override;

	CLASS_INTERFACE (IItemSelection, Object)

private:
	LinkedList<ItemIndex> items;
};

//************************************************************************************************
// ParamItemModel
//************************************************************************************************

class ParamItemModel: public Object,
	                  public AbstractItemModel,
					  public IItemSelection
{
public:
	DECLARE_CLASS (ParamItemModel, Object)
	CLASS_INTERFACE2 (IItemModel, IItemSelection, Object)

	ParamItemModel (StringID name = nullptr, IParameter* parameter = nullptr);
	~ParamItemModel ();

	// IItemModel
	int CCL_API countFlatItems () override;
	tbool CCL_API getItemTitle (String& title, ItemIndexRef index) override;
	tbool CCL_API onItemFocused (ItemIndexRef index) override;
	tbool CCL_API openItem (ItemIndexRef index, int column, const EditInfo& info) override;
	tbool CCL_API editCell (ItemIndexRef index, int column, const EditInfo& info) override;
	IItemSelection* CCL_API getSelection () override;

	// IItemSelection
	void CCL_API clone (IItemSelection*& selection) const override;
	tbool CCL_API isEmpty () const override;
	tbool CCL_API isMultiple () const override;
	tbool CCL_API isSelected (ItemIndexRef index) const override;
	IItemSelectionIterator* CCL_API newIterator () const override;
	void CCL_API select (ItemIndexRef index) override;
	tbool CCL_API unselect (ItemIndexRef index) override;
	void CCL_API unselectAll () override;
	
	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	MutableCString name;
	SharedPtr<IParameter> source;
};

} // namespace CCL

#endif // _ccl_itemviewbase_h
