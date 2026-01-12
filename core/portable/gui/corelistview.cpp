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
// Filename    : core/portable/gui/corelistview.cpp
// Description : List View class
//
//************************************************************************************************

#include "corelistview.h"
#include "corecontrols.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// Content
// The main content/scrolling area of a ListView
//************************************************************************************************

class ListView::Content: public View,
						 public ListViewPainter
{
public:
	Content (RectRef size, ListView& listView);
	
	void selectBy (int delta, ListView::SelectionTrigger trigger);
	void selectItem (int index, ListView::SelectionTrigger trigger);
	void modelChanged ();
	
	// View
	void setSize (RectRef newSize) override;
	void draw (const DrawEvent& e) override;
	void getHandledGestures (GestureVector& gestures, PointRef where) override;
	bool onGestureInput (const GestureEvent& e) override;
	bool onTouchInput (const TouchEvent& e) override;
	bool onWheelInput (const WheelEvent& e) override;
	void onFocus (bool state) override;
	
protected:
	ListView& listView;
};

//************************************************************************************************
// Content
//************************************************************************************************

ListView::Content::Content (RectRef size, ListView& listView)
: View (size),
  ListViewPainter (listView),
  listView (listView)
{
	wantsFocus (true);
	wantsTouch (true);
	enable (true);

	ListViewPainter::setClientWidth (size.getWidth ());
	ListViewPainter::setClientHeight (size.getHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::Content::onTouchInput (const TouchEvent& e)
{
	if(e.type == TouchEvent::kDown)
	{
		int itemIndex = ListViewPainter::getItemIndex (e.where);
		
		Rect itemRect;
		ListViewPainter::getItemRect (itemRect, itemIndex);
		Point whereInItem (e.where.x - itemRect.left, e.where.y - itemRect.top);
		
		if(RootView* root = getRootView ())
			root->setFocusView (this); // do this first, as MenuView::itemTouched () can delete itself...
		
		if(ListViewModel* model = listView.getModel ())
		{
			int maxIndex = model->getItemCount ()-1;
			if(itemIndex > maxIndex)
				itemIndex = -1;
			
			model->itemTouched (itemIndex, whereInItem, &listView);
		}
		
		selectItem (itemIndex, ListView::kTriggerTouch);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::Content::getHandledGestures (GestureVector& gestures, PointRef where)
{
	gestures.add (kGestureSwipe|kGesturePriorityNormal);
	gestures.add (kGestureSingleTap|kGesturePriorityNormal);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::Content::onGestureInput (const GestureEvent& e)
{
	if(e.getType () == kGestureSwipe)
	{
		if(e.getState () == kGestureBegin || e.getState () == kGesturePossible)
			e.userData = e.where.y + startIndex * listView.getRowHeight ();
		else
		{
			int index = (e.userData - e.where.y) / listView.getRowHeight (); 
			if(ListViewPainter::scrollTo (index))
				invalidate ();
		}
	}
	else if(e.getType () == kGestureSingleTap)
	{
		// note: overriden in MenuView
		onTouchInput (TouchEvent (TouchEvent::kDown, e.where));
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::Content::onWheelInput (const WheelEvent& e)
{
	if(listView.isWheelSelection ())
		selectBy (e.delta, ListView::kTriggerWheel);
	else
	{
		if(ListViewPainter::scrollBy (e.delta))
			invalidate ();
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::Content::onFocus (bool state)
{
	View::onFocus (state);
	listView.invalidate (); // allow the listview to draw the focus frame around its entire list
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::Content::modelChanged ()
{
	ListViewPainter::resetScrollPosition ();
	ListViewPainter::resetSelectedItem ();
	
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::Content::selectItem (int index, ListView::SelectionTrigger trigger)
{		
 	ListViewModel* model = listView.getModel ();				
	if(model && model->isSelectionHandler ())
	{
		int maxIndex = model->getItemCount ()-1;
		if(index > maxIndex)
			index = -1;

		bool state = true;
		if(listView.isDeselectAllowed () && trigger == ListView::kTriggerTouch) // we allow de-selection...
			state = !model->isItemSelected (index);

		model->selectListItem (index, state, &listView, trigger);
	}
	else
	{
		if(ListViewPainter::selectItem (index))
			invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::Content::selectBy (int delta, ListView::SelectionTrigger trigger)
{
	const ListViewModel* model = listView.getModel ();
	int maxIndex = model ? model->getItemCount () - 1 : -1;
	int newSelectIndex = bound (selectIndex + delta, 0, maxIndex);
	if(newSelectIndex != selectIndex)
	{
		selectItem (newSelectIndex, trigger);		
		makeItemVisible (selectIndex);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::Content::setSize (RectRef newSize)
{
	View::setSize (newSize);
	ListViewPainter::setClientWidth (newSize.getWidth ());
	ListViewPainter::setClientHeight (newSize.getHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::Content::draw (const DrawEvent& e)
{
	ListViewPainter::drawList (e, getStyle ());
}

//************************************************************************************************
// ListViewItem
//************************************************************************************************

ListViewItem::ListViewItem (CStringPtr title)
: title (title),
  userData (nullptr),
  enabled (true)
{}

//************************************************************************************************
// ListViewModel
//************************************************************************************************

ListViewModel::ListViewModel ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewModel::~ListViewModel ()
{
	ASSERT (views.isEmpty ())
	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::viewAttached (ListView* view)
{
	ASSERT (view && !views.contains (view))
	views.add (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::viewDetached (ListView* view)
{
	ASSERT (view && views.contains (view))
	views.remove (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::changed ()
{
	VectorForEach (views, ListView*, view)
		view->modelChanged ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::invalidate ()
{
	VectorForEach (views, ListView*, view)
		view->invalidate ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::addItem (ListViewItem* item)
{
	items.add (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModel::removeItem (ListViewItem* item)
{
	return items.remove (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewItem* ListViewModel::findItem (void* userData) const
{
	VectorForEach (items, ListViewItem*, item)
		if(item->getUserData () == userData)
			return item;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModel::removeAll ()
{
	VectorForEach (items, ListViewItem*, item)
		delete item;
	EndFor
	items.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListViewModel::getItemCount () const
{
	return items.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr ListViewModel::getItemTitle (int index) const
{
	ListViewItem* item = items.at (index);
	ASSERT (item != nullptr)
	return item ? item->getTitle ().str () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModel::isItemEnabled (int index) const
{
	ListViewItem* item = items.at (index);
	ASSERT (item != nullptr)
	return item ? item->isEnabled () : true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewModel::canSelectItem (int index) const
{
	return isItemEnabled (index);
}

//************************************************************************************************
// ListView
//************************************************************************************************

ListView::ListView (RectRef size, ListViewModel* _model)
: ContainerView (size),
  content (nullptr),
  model (nullptr)
{
	DpiSetting::instance ().scaleCoord (rowHeight);
	DpiSetting::instance ().scaleCoord (itemInset);
	DpiSetting::instance ().scaleCoord (scrollerSize);

	wantsTouch (true);
	wantsFocus (true); // we set this so we can forward focus to our content
	
	addView (content = NEW Content (Rect (0, 0, size.getWidth (), size.getHeight ()), *this));

	if(_model)
		setModel (_model);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListView::~ListView ()
{
	setModel (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr ListView::getConnectionType () const
{
	return kListViewModelType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::connect (void* object)
{
	setModel (reinterpret_cast<ListViewModel*> (object));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::setModel (ListViewModel* newModel)
{
	if(model != newModel)
	{
		if(model)
			model->viewDetached (this);
		model = newModel;
		content->setBaseModel (newModel); // ListViewPainter
		if(model)
			model->viewAttached (this);
		modelChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::modelChanged ()
{
	content->modelChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::setAttributes (const Attributes& a)
{
	ContainerView::setAttributes (a);

	options |= ViewAttributes::getOptions (a, Skin::Enumerations::listViewOptions);
 
	if(const Attributes* styleAttr = ViewAttributes::getStyleAttributes (a))
	{
		if(styleAttr->contains (ViewAttributes::kRowHeight))
		{
			rowHeight = (int)styleAttr->getInt (ViewAttributes::kRowHeight);
			DpiSetting::instance ().scaleCoord (rowHeight);
		}
		if(styleAttr->contains (ViewAttributes::kItemInset))
		{
			itemInset = (int)styleAttr->getInt (ViewAttributes::kItemInset);
			DpiSetting::instance ().scaleCoord (itemInset);
		}
		if(styleAttr->contains (ViewAttributes::kScrollerSize))
		{
			scrollerSize = (int)styleAttr->getInt (ViewAttributes::kScrollerSize);
			DpiSetting::instance ().scaleCoord (scrollerSize);
		}

		selectColor = ViewAttributes::getColor (*styleAttr, ViewAttributes::kSelectColor, selectColor);
		
		separatorColor = ViewAttributes::getColor (*styleAttr, ViewAttributes::kSeparatorColor, separatorColor);
		
		if(CStringPtr borderStyle = styleAttr->getString (ViewAttributes::kFocusBorder))
			focusBorder = EnumInfo::parseMultiple<ConstString> (borderStyle, Skin::Enumerations::border);
	}
	
	Rect contentSize = ViewAttributes::getSize (a, ViewAttributes::kContentSize);
	if(contentSize.isEmpty ())
	{
		contentSize = size;
		contentSize.moveTo (Point (0, 0));
	}
	if(contentSize.getWidth () < 0)
		contentSize.setWidth (getSize ().getWidth () - contentSize.left);
	if(contentSize.getHeight () < 0)
		contentSize.setHeight (getSize ().getHeight () - contentSize.top);

	content->setSize (contentSize);
	content->setStyle (NEW Style (getStyle ()));
	
	// if they explicitly passed a "backgroundstyle" use that for 'this' instead of "style"
	const Attributes* bgStyleAttributes = nullptr;
	if(const Attribute* attr = a.lookup ("backgroundstyle"))
	{
		if(attr->getType () == Attribute::kString) // name of shared style
			bgStyleAttributes = StyleManager::instance ().getStyle (attr->getString ());
		else
			bgStyleAttributes = attr->getAttributes ();
	}
	if(bgStyleAttributes)
	{	
		Style* bgStyle = NEW Style;
		ViewAttributes::getStyle (*bgStyle, *bgStyleAttributes);
		setStyle (bgStyle);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::getItemRect (Rect& rect, int index) const
{
	content->getItemRect (rect, index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::draw (const DrawEvent& e)
{
	bool focused = content->hasFocus ();
	
	// fill the background
	if(focused)
		e.graphics.fillRect (e.updateRect, getStyle ().getHiliteColor ());
	else
		e.graphics.fillRect (e.updateRect, getStyle ().getBackColor ());

	ContainerView::draw (e);
	
	if(focused && focusBorder != 0)
	{
		Rect clientRect;
		getClientRect (clientRect);		
		ThemePainter::instance ().drawFocusFrame (e.graphics, clientRect, focusBorder);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::setSize (RectRef newSize)
{
	ContainerView::setSize (newSize);

	Rect contentSize (0, 0, newSize.getWidth (), newSize.getHeight ());
	content->setSize (contentSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::scrollBy (int delta)
{
	if(content->scrollBy (delta))
		content->invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListView::getScrollPosition () const
{
	return content->getScrollPosition ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::scrollTo (int index)
{
	if(content->scrollTo (index))
		content->invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::selectItem (int index, SelectionTrigger trigger)
{
	content->selectItem (index, trigger);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::makeSelectedItemVisible ()
{
	if(content->makeSelectedItemVisible ())
		content->invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::makeItemVisible (int index)
{
	if(content->makeItemVisible (index))
		content->invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListView::onTouchInput (const TouchEvent& e)
{
	bool handled = ContainerView::onTouchInput (e);
	if(!handled && e.type == TouchEvent::kDown)
	{
		ASSERT (content->wantsTouch ())
		if(RootView* rootView = getRootView ())
			rootView->setFocusView (content);
		return true;
	}
	return handled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewModel* ListView::getModel () const
{
	return model;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListView::onFocus (bool state)
{
	// we don't accept focus, but we'll forward it to our content.
	if(state)
		if(RootView* root = getRootView ())
			root->setFocusView (content);
}
