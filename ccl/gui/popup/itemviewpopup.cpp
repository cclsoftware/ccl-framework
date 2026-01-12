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
// Filename    : ccl/gui/popup/itemviewpopup.cpp
// Description : ItemView Popup Selector
//
//************************************************************************************************

#include "ccl/gui/popup/itemviewpopup.h"
#include "ccl/gui/itemviews/listview.h"
#include "ccl/gui/itemviews/treeview.h"
#include "ccl/gui/itemviews/treeitem.h"

#include "ccl/gui/gui.h"

#include "ccl/gui/layout/anchorlayout.h"
#include "ccl/gui/layout/boxlayout.h"
#include "ccl/gui/layout/layoutprimitives.h"
#include "ccl/gui/layout/directions.h"

using namespace CCL;

//************************************************************************************************
// ItemViewPopup
//************************************************************************************************

ItemViewPopup::ItemViewPopup ()
: itemView (nullptr),
  itemViewStyle (0, Styles::kItemViewBehaviorAutoSelect|
					Styles::kItemViewBehaviorSelectExclusive|
					Styles::kItemViewBehaviorSelectFullWidth|
					Styles::kItemViewBehaviorSimpleMouse|
					Styles::kItemViewBehaviorSwallowAlphaChars),
  scrollStyle (0, Styles::kScrollViewBehaviorAutoHideBoth),
  visualStyle (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemViewPopup::~ItemViewPopup ()
{
	safe_release (visualStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemModel* ItemViewPopup::getItemModel ()
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewPopup::setVisualStyle (VisualStyle* vs)
{
	take_shared (visualStyle, vs);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle* ItemViewPopup::getVisualStyle (Theme& theme)
{
	return visualStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewPopup::onItemViewCreated ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API ItemViewPopup::createPopupView (SizeLimit& limits)
{
	Rect size (0, 0, 100, 100);
	if(limits.isValid ())
		limits.makeValid (size);

	Coord borderSize = 0;
	AnchorLayoutView* layoutView = nullptr;

	VisualStyle* visualStyle = getVisualStyle (*ThemeSelector::currentTheme);
	if(visualStyle)
	{
		borderSize = visualStyle->getMetric ("border", -1);
		if(borderSize > 0)
		{
			AutoPtr<BoxLayout> layout (NEW BoxLayout);
			layout->setProperty (ATTR_SPACING, 0);
			layout->setProperty (ATTR_MARGIN, borderSize);

			StyleFlags layoutStyle (Styles::kHorizontal);
			layoutView = NEW AnchorLayoutView (size, layoutStyle, layout);
			layoutView->setSizeMode (View::kAttachAll|View::kFitSize);

			size.right -= 2 * borderSize;
			size.bottom -= 2 * borderSize;
		}
	}

	ItemControl* itemControl = createItemControl (size);
	itemControl->setTheme (ThemeSelector::currentTheme);
	itemControl->setVisualStyle (visualStyle);
	itemControl->setSizeMode (View::kAttachAll|View::kFitSize);
	IView* result = itemControl;

	SizeLimit controlLimits (limits);
	if(layoutView)
	{
		LayoutPrimitives::calcSizeLimitsFromParent<HorizontalDirection> (controlLimits, limits, 2 * borderSize);
		LayoutPrimitives::calcSizeLimitsFromParent<VerticalDirection> (controlLimits, limits, 2 * borderSize);
		layoutView->addView (itemControl);
		result = layoutView;
	}
	itemControl->setSizeLimits (controlLimits);

	itemView = UnknownPtr<IItemView> (itemControl->asUnknown ());
	if(ItemViewBase* itemViewBase = itemControl->getItemView ())
		itemViewBase->disableTouchHandler (true);
	
	onItemViewCreated ();

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemViewPopup::isInsideItemView (const MouseEvent& event, Point* clientPos) const
{
	if(View* view = unknown_cast<View> (itemView))
	{
		Point p (event.where);
		view->windowToClient (p);
		if(clientPos)
			*clientPos = p;
		return view->isInsideClient (p);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemViewPopup::attached (IWindow& popupWindow)
{
	PopupSelectorClient::attached (popupWindow);

	#if !IOS
	if(itemView)
	{
		// if mouse is already over an item, remember it
		Point p;
		GUI.getMousePosition (p);

		View* view = unknown_cast<View> (itemView);
		view->screenToClient (p);
		if(view->isInsideClient (p))
			itemView->findItem (initialMouseItem, p);
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemViewPopup::Result CCL_API ItemViewPopup::onMouseUp (const MouseEvent& event, IWindow& popupWindow)
{
	Point p;
	if(isInsideItemView (event, &p))
	{
		if(initialMouseItem.isValid ())
		{
			// ignore first mouseUp if still over initial item (avoid immediate close on simple click)
			ItemIndex item;
			if(itemView->findItem (item, p) && item == initialMouseItem)
			{
				initialMouseItem = ItemIndex ();
				return kIgnore;
			}
		}

		return PopupSelectorClient::onMouseUp (event, popupWindow);
	}
	else if(wantsMouseUpOutside ())
		return kCancel;

	return kIgnore;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemViewPopup::onPopupClosed (Result result)
{
	initialMouseItem = ItemIndex (); // reset
	PopupSelectorClient::onPopupClosed (result);
}

//************************************************************************************************
// TreeViewPopup
//************************************************************************************************

ItemControl* TreeViewPopup::createItemControl (RectRef size)
{
	itemViewStyle.custom |=
		Styles::kItemViewAppearanceNoFocusRect|
		Styles::kTreeViewAppearanceNoRoot|
		Styles::kTreeViewBehaviorExpandMouseItem|
		Styles::kTreeViewBehaviorAutoExpand;

	return NEW TreeControl (size, getItemModel (), itemViewStyle, scrollStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TreeViewPopup::onItemViewCreated ()
{
	TreeView* treeView = unknown_cast<TreeView> (itemView);
	treeView->getTree ().expand ();	// expand (hidden) root item
	treeView->autoSize ();
}

//************************************************************************************************
// ListViewPopup
//************************************************************************************************

ListViewPopup::ListViewPopup ()
: listViewType (Styles::kListViewList)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemControl* ListViewPopup::createItemControl (RectRef size)
{
	return NEW ListControl (size, getItemModel (), itemViewStyle, scrollStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewPopup::onItemViewCreated ()
{
	ListView* listView = unknown_cast<ListView> (itemView);
	listView->setViewType (listViewType);
}
