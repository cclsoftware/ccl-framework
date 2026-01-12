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
// Filename    : ccl/gui/popup/menupopupselector.cpp
// Description : Menu Popup Selector
//
//************************************************************************************************

#include "ccl/gui/popup/menupopupselector.h"
#include "ccl/gui/popup/parametermenubuilder.h"
#include "ccl/gui/popup/menu.h"

#include "ccl/gui/itemviews/treeview.h"
#include "ccl/gui/itemviews/treeitem.h"

#include "ccl/gui/graphics/shapes/shapeimage.h"
#include "ccl/gui/graphics/shapes/shapes.h"

#include "ccl/base/message.h"

#include "ccl/public/collections/unknownlist.h"

using namespace CCL;

//************************************************************************************************
// MenuPopupSelector
//************************************************************************************************

MenuPopupSelector::MenuPopupSelector (Menu* _menu, bool _shouldSelectCheckedItems)
: menu (nullptr),
  mustCloseOnSelect (false),
  condensedSeparators (false),
  shouldSelectCheckedItems (_shouldSelectCheckedItems),
  hasCheckedItems (false)
{
	itemViewStyle.custom |= Styles::kItemViewBehaviorFocusSelectable;

	take_shared (menu, _menu);

	if(menu)
	{
		// the parameter can adjust the behavior through a secret property
		// TODO: could be done via IParameterMenuCustomize!
		IParameter* parameter = ParameterMenuBuilder::extractParameter (*menu);
		if(shouldSelectCheckedItems == false && parameter && parameter->isOutOfRange () == false)
			shouldSelectCheckedItems = true;

		if(UnknownPtr<IObject> paramObject = parameter)
		{
			Variant var;
			if(paramObject->getProperty (var, MenuPopupSelectorBehavior::kMustCloseMenuOnSelect) && var.asBool ())
				mustCloseOnSelect = true;

			var.clear ();
			if(paramObject->getProperty (var, MenuPopupSelectorBehavior::kCondensedMenuSeparators) && var.asBool ())
				condensedSeparators = true;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuPopupSelector::~MenuPopupSelector ()
{
	safe_release (menu);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle* MenuPopupSelector::getVisualStyle (Theme& theme)
{
	if(visualStyle)
		return visualStyle;

	if(menu && menu->getVariant ().contains (Menu::strLargeVariant, false))
		return theme.getStandardStyle (ThemePainter::kPopupMenuLargeStyle);
	else
		return theme.getStandardStyle (ThemePainter::kPopupMenuStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemControl* MenuPopupSelector::createItemControl (RectRef size)
{
	if(hideHScroll ())
		scrollStyle.setCustomStyle (Styles::kScrollViewBehaviorVScrollSpace);
	return SuperClass::createItemControl (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MenuPopupSelector::onItemViewCreated ()
{
	ItemView* itemView = unknown_cast<ItemView> (this->itemView);
	ASSERT (itemView != nullptr)
	if(itemView->getVisualStyle ().getMetric (MenuPopupSelectorBehavior::kCondensedMenuSeparators, false))
		condensedSeparators = true;

	TreeViewPopup::onItemViewCreated ();

	if(menu)
	{
		menu->init (); // update checked/enabled state

		hasCheckedItems = false;
		selectCheckedMenuItems (*menu);

		if(!checkMarkIcon)
		{
			noCheckMarkIcon = nullptr;
			checkMarkIcon = itemView->getVisualStyle ().getImage ("checkmarkicon");
			if(checkMarkIcon)
			{
				AutoPtr<ComplexShape> shape = NEW ComplexShape;
				shape->setSize (Rect (0, 0, checkMarkIcon->getWidth (), checkMarkIcon->getHeight ()));
				noCheckMarkIcon = NEW ShapeImage (shape);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MenuPopupSelector::attached (IWindow& popupWindow)
{
	// focus the first selected item (scrolls to make it visible)
	ForEachItem (itemView->getSelection (), item)
		itemView->setFocusItem (item, false);
		break;
	EndFor

	TreeViewPopup::attached (popupWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static inline Menu* getMenu (ItemIndexRef index)
{
	IUnknown* obj = index.getObject ();
	if(MenuItem* menuItem = unknown_cast<MenuItem> (obj))
		return menuItem->getSubMenu ();
	else
		return unknown_cast<Menu> (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static inline bool isSeparator (MenuItem* item)
{
	return item->isSeparator () || item->isHeader ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuPopupSelector::selectCheckedMenuItems (Menu& menu)
{
	Variant attrib;
	int numItems = menu.countItems ();
	bool isSelectExclusive = itemViewStyle.isCustomStyle (Styles::kItemViewBehaviorSelectExclusive);

	for(int i = 0; i < numItems; i++)
	{
		if(MenuItem* item = menu.at (i))
		{
			if(item->isChecked ())
			{
				hasCheckedItems = true;
				if(shouldSelectCheckedItems)
				{
					bool found = itemView->selectItem (item->asUnknown (), true);
					if(found == false && isSelectExclusive)
					{
						if(TreeView* treeView = unknown_cast<TreeView> (itemView))
						{
							auto& tree = treeView->getTree ();
							tree.expand (true, true);  // create all tree items
							tree.expand (false, true); // collapse all items
							tree.expand (true, false); // re-expand only root
							itemView->setFocusItem (item->asUnknown (), true); // focus and select
						}						
					}
					else
						itemView->setFocusItem (item->asUnknown (), false);
				}
				else
					return true;

				if(isSelectExclusive)
					return true;
			}
			if(Menu* subMenu = item->getSubMenu ())
				if(selectCheckedMenuItems (*subMenu))
					if(isSelectExclusive)
						return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuPopupSelector::selectMatchingItem (Menu& menu, const MenuItem& referenceItem)
{
	for(int i = 0, numItems = menu.countItems (); i < numItems; i++)
	{
		if(MenuItem* item = menu.at (i))
		{
			if(item->isEnabled () && !isSeparator (item)
				&& referenceItem.getCategory () == item->getCategory ()
				&& referenceItem.getName () == item->getName ())
				//&& referenceItem.getTitle () == item->getTitle ()) title can change!
				{
					itemView->selectItem (item->asUnknown (), true);
					itemView->setFocusItem (item->asUnknown (), false);
					return true;
				}

			if(Menu* subMenu = item->getSubMenu ())
				if(selectMatchingItem (*subMenu, referenceItem))
					return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MenuItem* MenuPopupSelector::getCurrentMenuItem ()
{
	if(itemView)
	{
		ForEachItem (itemView->getSelection (), index)
			MenuItem* menuItem = unknown_cast<MenuItem> (index.getObject ());
			if(menuItem && menuItem->isEnabled () && !isSeparator (menuItem))
				return menuItem;
		EndFor
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MenuPopupSelector::hasPopupResult ()
{
	MenuItem* item = getCurrentMenuItem ();
	return item && !item->getSubMenu ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MenuPopupSelector::getRootItem (ItemIndex& index)
{
	index = ItemIndex (ccl_as_unknown (menu));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MenuPopupSelector::canExpandItem (ItemIndexRef index)
{
	return getMenu (index) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MenuPopupSelector::canSelectItem (ItemIndexRef index)
{
	MenuItem* menuItem = unknown_cast<MenuItem> (index.getObject ());
	return menuItem && menuItem->isEnabled () && !isSeparator (menuItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API MenuPopupSelector::getItemBackground (ItemIndexRef index)
{
	MenuItem* menuItem = unknown_cast<MenuItem> (index.getObject ());
	if(menuItem && menuItem->isSeparator ()) // (not for header)
		return CSTR ("separator");
	else
		return CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MenuPopupSelector::getSubItems (IUnknownList& items, ItemIndexRef index)
{
	if(Menu* menu = getMenu (index))
	{
		int numItems = menu->countItems ();
		for(int i = 0; i < numItems; i++)
			if(MenuItem* subItem = menu->at (i))
			{
				if(condensedSeparators && isSeparator (subItem))
					continue;

				items.add (subItem->asUnknown (), true);
			}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MenuPopupSelector::getItemTitle (String& title, ItemIndexRef index)
{
	IUnknown* obj = index.getObject ();
	if(MenuItem* menuItem = unknown_cast<MenuItem> (obj))
	{
		if(menuItem->isSubMenu ())
		{
			if(Menu* menu = menuItem->getSubMenu ())
			{
				title = menu->getTitle ();
				return true;
			}
		}

		title = menuItem->getTitle ();
		return true;
	}
	else if(Menu* menu = unknown_cast<Menu> (obj))
	{
		title = menu->getTitle ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MenuPopupSelector::getItemTooltip (String& tooltip, ItemIndexRef index, int column)
{
	tooltip.empty ();
	IUnknown* obj = index.getObject ();
	if(MenuItem* menuItem = unknown_cast<MenuItem> (obj))
	{
		// display command key as tooltip
		if(const KeyEvent* key = menuItem->getAssignedKey ())
		{
			String keyString;
			key->toString (keyString, true);
			tooltip = String () << menuItem->getTitle () << " (" << keyString << ")";
		}
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API MenuPopupSelector::getItemIcon (ItemIndexRef index)
{
	IImage* icon = nullptr;

	IUnknown* obj = index.getObject ();
	if(MenuItem* menuItem = unknown_cast<MenuItem> (obj))
	{
		icon = menuItem->getIcon ();
		if(!icon && menuItem->isChecked ())
			icon = checkMarkIcon;
	}
	else if(Menu* menu = unknown_cast<Menu> (obj))
		icon = menu->getIcon ();

	if(!icon && hasCheckedItems)
		icon = noCheckMarkIcon;

	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MenuPopupSelector::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	MenuItem* menuItem = unknown_cast<MenuItem> (index.getObject ());
	ASSERT (menuItem != nullptr)
	if(menuItem == nullptr)
		return false;

	ItemView* itemView = unknown_cast<ItemView> (this->itemView);
	ASSERT (itemView != nullptr)

	bool result = false;

	if(menuItem->isSeparator ())
	{
		ASSERT (condensedSeparators == false)
		Point p1 (info.rect.left, (info.rect.top + info.rect.bottom) / 2);
		Point p2 (info.rect.right, p1.y);
		const IVisualStyle& vs = itemView->getVisualStyle ();
		Pen pen (vs.getColor ("menu.separatorcolor", vs.getColor ("separatorcolor", info.style.getTextBrush (false).getColor ())));
		info.graphics.drawLine (p1, p2, pen);
		result = true;
	}
	else if(menuItem->isItalic () || (!menuItem->isEnabled () && !menuItem->isHeader ()) || menuItem->isChecked ())
	{
		Font font (info.style.font);
		if(menuItem->isItalic ())
			font.isItalic (true);
		// obsolete with proper checkmark icon
		//if(menuItem->isChecked ())
		//	font.isBold (true);

		SolidBrush brush (info.style.textBrush);
		if(!menuItem->isEnabled ())
			brush = info.style.getTextBrush (false);

		info.graphics.drawString (info.rect, menuItem->getTitle (), font, brush, Alignment::kLeftCenter);
		result = true;
	}

	// draw separator
	if(condensedSeparators)
	{
		// make sure the extra separator is visible when separators are drawn between all items
		Coord left (info.rect.left);
		const IVisualStyle& vs = itemView->getVisualStyle ();
		Pen pen (vs.getColor ("menu.separatorcolor", vs.getColor ("separatorcolor", Colors::kTransparentBlack)));
		if(pen.getColor ().getAlphaF () != 0)
		{
			left = 0;
			pen.setWidth (2);
		}

		if(MenuItem* prevItem = menuItem->getPreviousItem ())
			if(isSeparator (prevItem))
				info.graphics.drawLine (Point (left, info.rect.top), info.rect.getRightTop (), pen);

		if(MenuItem* nextItem = menuItem->getNextItem ())
			if(isSeparator (nextItem))
				info.graphics.drawLine (Point (left, info.rect.bottom), info.rect.getRightBottom (), pen);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API MenuPopupSelector::onKeyDown (const KeyEvent& event)
{
	if(menu)
	{
		// let parameter intercept keys via IParameterMenuCustomize
		UnknownPtr<IParameterMenuCustomize> customizer (ParameterMenuBuilder::extractParameter (*menu));
		if(customizer && customizer->onMenuKeyDown (event))
			return kSwallow;
	}

	if(event.vKey == VKey::kSpace)
	{
		if(mustCloseOnSelect)
			return kOkay;

		// select current menu item
		MenuItem* item = getCurrentMenuItem ();
		if(item && !item->isSubMenu ())
			item->select ();

		return kSwallow; // stay open, event consumed
	}

	return SuperClass::onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PopupSelectorClient::Result CCL_API MenuPopupSelector::onMouseDown (const MouseEvent& event, IWindow& popupWindow)
{
	Result result = TreeViewPopup::onMouseDown (event, popupWindow);

	if(result == kIgnore && acceptOnDoubleClick () && isInsideItemView (event))
	{
		// select current menu item
		MenuItem* item = getCurrentMenuItem ();
		if(item && !item->isSubMenu ())
			item->select ();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MenuPopupSelector::onPopupClosed (Result result)
{
	SuperClass::onPopupClosed (result);

	if(result == IPopupSelectorClient::kOkay)
		if(MenuItem* item = getCurrentMenuItem ())
			item->select ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MenuPopupSelector::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IParameter::kUpdateMenu)
	{
		Menu* newMenu = nullptr;
		if(msg.getArgCount () > 0)
			newMenu = unknown_cast<Menu> (msg [0]);
		else if(menu)
		{
			if(ParameterMenuBuilder* builder = ParameterMenuBuilder::extractBuilder (*menu))
				if(IParameter* param = ParameterMenuBuilder::extractParameter (*menu))
				{
					IMenu* m = menu;
					m->removeAll ();
					builder->buildMenu (menu);
					signal (Message (kNewRootItem));

					hasCheckedItems = false;
					selectCheckedMenuItems (*menu);
				}
		}

		if(newMenu)
		{
			SharedPtr<MenuItem> oldSelectedItem = getCurrentMenuItem ();

			take_shared (menu, newMenu);
			signal (Message (kNewRootItem));

			// try to select a new item matching the previously selected item
			if(oldSelectedItem)
				selectMatchingItem (*newMenu, *oldSelectedItem);
		}
	}
	SuperClass::notify (subject, msg);
}
