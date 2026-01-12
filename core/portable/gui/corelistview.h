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
// Filename    : core/portable/gui/corelistview.h
// Description : List View class
//
//************************************************************************************************

#ifndef _corelistview_h
#define _corelistview_h

#include "core/portable/gui/coreview.h"

namespace Core {
namespace Portable {

class ListView;

//************************************************************************************************
// ListViewItem
/** \ingroup core_gui */
//************************************************************************************************

class ListViewItem
{
public:
	ListViewItem (CStringPtr title = nullptr);

	PROPERTY_CSTRING_BUFFER (128, title, Title)
	PROPERTY_POINTER (void, userData, UserData)
	PROPERTY_BOOL (enabled, Enabled)
};

//************************************************************************************************
// ListViewModel
/** \ingroup core_gui */
//************************************************************************************************

class ListViewModel: public ListViewModelBase
{
public:
	ListViewModel ();
	~ListViewModel ();

	virtual void viewAttached (ListView* view);
	virtual void viewDetached (ListView* view);
	virtual void changed ();
	virtual void invalidate ();

	virtual void addItem (ListViewItem* item);
	virtual bool removeItem (ListViewItem* item);
	virtual ListViewItem* findItem (void* userData) const;
	virtual void removeAll ();

	virtual void selectListItem (int index, bool state, ListView* view, int trigger) {} // trigger - see ListView::SelectionTrigger	
	virtual void itemTouched (int index, const Point& whereInItem, ListView* view) {}

	// ListViewModelBase
	int getItemCount () const override;
	CStringPtr getItemTitle (int index) const override;
	bool canSelectItem (int index) const override;
	bool isItemEnabled (int index) const override;

protected:
	FixedSizeVector<ListView*, 5> views;
	Vector<ListViewItem*> items;
};

static const CStringPtr kListViewModelType = "ListModel";

//************************************************************************************************
// ListView
/** A container with a content area, background/header area and more.
	\ingroup core_gui */
//************************************************************************************************

class ListView: public ContainerView,
				public ListViewStyle
{
public:
	DECLARE_CORE_CLASS ('LstV', ListView, View)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kListView)

	ListView (RectRef size = Rect (), ListViewModel* model = nullptr);
	~ListView ();
	
	PROPERTY_FLAG (options, Skin::kListViewBehaviorWheelSelection, isWheelSelection)
	PROPERTY_FLAG (options, Skin::kListViewBehaviorDeselectAllowed, isDeselectAllowed)

	ListViewModel* getModel () const;
	void setModel (ListViewModel* model); ///< ATT: model is *not* owned!
	void modelChanged ();

	enum SelectionTrigger { kTriggerTouch, kTriggerWheel, kTriggerInternal };
	void selectItem (int index, SelectionTrigger trigger);
	void makeSelectedItemVisible ();
	void makeItemVisible (int index);
	void getItemRect (Rect& rect, int index) const;
	
	void scrollBy (int delta);
	void scrollTo (int index);
	int getScrollPosition () const;
	
	// ContainerView
	void draw (const DrawEvent& e) override;
	void setSize (Core::RectRef newSize) override;
	void setAttributes (const Attributes& a) override;
	bool onTouchInput (const TouchEvent& e) override;
	CStringPtr getConnectionType () const override;
	void connect (void* object) override;
	void onFocus (bool state) override;

protected:
	class Content;
	Content* content;
	ListViewModel* model;
};

} // namespace Portable
} // namespace Core

#endif // _corelistview_h
