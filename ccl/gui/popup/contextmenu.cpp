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
// Filename    : ccl/gui/popup/contextmenu.cpp
// Description : Context Menu
//
//************************************************************************************************

#include "ccl/gui/popup/contextmenu.h"
#include "ccl/gui/popup/extendedmenu.h"

#include "ccl/base/storage/configuration.h"
#include "ccl/base/asyncoperation.h"
#include "ccl/base/message.h"

#include "ccl/public/gui/framework/controlsignals.h"

using namespace CCL;

//************************************************************************************************
// ContextMenu
//************************************************************************************************

const Configuration::BoolValue useExtendedContextMenu ("GUI.ContextMenu", "UseExtendedMenu", false);
const Configuration::BoolValue useCompactContextMenu ("GUI.ContextMenu", "UseCompactMenu", false);
const Configuration::BoolValue useNonModalContextMenu ("GUI.ContextMenu", "UseNonModalContextMenu", false);

DEFINE_CLASS_ABSTRACT_HIDDEN (ContextMenu, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ContextMenu::setContextID (StringID id)
{
	contextID = id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API ContextMenu::getContextID () const
{
	return contextID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ContextMenu::setFocusItem (IUnknown* item)
{
	focusItem.share (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ContextMenu::getFocusItem () const
{
	return focusItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ContextMenu)
	DEFINE_METHOD_ARGR ("addCommandItem", "title, category, name", "MenuItem")
	DEFINE_METHOD_NAME ("addSeparatorItem")
END_METHOD_NAMES (ContextMenu)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ContextMenu::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "addCommandItem")
	{
		String title (msg[0].asString ());
		MutableCString category (msg[1].asString ());
		MutableCString name (msg[2].asString ());
		UnknownPtr<ICommandHandler> handler (msg[3].asUnknown ());

		returnValue = addCommandItem (title, category, name, handler);
		return true;
	}
	else if(msg == "addSeparatorItem")
	{
		returnValue = addSeparatorItem ();
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (ContextMenu)
	DEFINE_PROPERTY_NAME ("focusItem")
END_PROPERTY_NAMES (ContextMenu)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ContextMenu::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "focusItem")
	{
		var = getFocusItem ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ContextMenu::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "focusItem")
	{
		setFocusItem (var);
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//************************************************************************************************
// ContextPopupMenu
//************************************************************************************************

DEFINE_CLASS (ContextPopupMenu, ContextMenu)
DEFINE_CLASS_UID (ContextPopupMenu, 0x6161c214, 0x351a, 0x4378, 0x94, 0x5f, 0xf1, 0xf2, 0x1c, 0xa8, 0x63, 0x2f)

ContextPopupMenu* ContextPopupMenu::nonModalInstance = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

Menu* ContextPopupMenu::createMenu (StringID menuType)
{
	if((useExtendedContextMenu || useCompactContextMenu) && menuType != MenuPresentation::kNative)
		return NEW ExtendedMenu;
	else
		return PopupMenu::create ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ContextPopupMenu::ContextPopupMenu (StringID menuType)
: ContextPopupMenu (createMenu (menuType))
{
	menu->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ContextPopupMenu::ContextPopupMenu (Menu* menu)
: menu (menu),
  controlSink (CCL::Signals::kControls)
{
	ASSERT (menu)
	menu->retain ();

	menu->setMenuData (asUnknown ()); // must not be shared

	subContextMenus.objectCleanup (true);

	controlSink.setObserver (this);
	controlSink.enable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ContextPopupMenu::~ContextPopupMenu ()
{
	controlSink.enable (false);

	menu->setMenuData (0);
	menu->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Menu* ContextPopupMenu::getMenu () const
{
	return menu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContextPopupMenu::queryInterface (UIDRef iid, void** ptr)
{
	if(iid == ccl_iid<IMenu> () || iid == ccl_iid<IExtendedMenu> ())
		return menu->queryInterface (iid, ptr);

	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ContextPopupMenu::countItems () const
{
	return menu->countItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ContextPopupMenu::popup (const Point& where, View* view)
{
	if(nonModalInstance && nonModalInstance->popupSelector)
	{
		nonModalInstance->popupSelector->close ();
		nonModalInstance->popupSelector.release ();
		nonModalInstance = nullptr;
	}

	if(useNonModalContextMenu)
		nonModalInstance = this;

	popupSelector = NEW PopupSelector;
	popupSelector->setNonModal (useNonModalContextMenu);

	if(view)
	{
		VisualStyle* visualStyle = view->getTheme ().getStandardStyle (ThemePainter::kContextMenuStyle);
		if(!visualStyle)
			visualStyle = view->getTheme ().getStandardStyle (ThemePainter::kMenuControlStyle);

		popupSelector->setTheme (view->getTheme ());
		popupSelector->setVisualStyle (visualStyle);
	}

	PopupSizeInfo sizeInfo (where, view);
	Promise promise (popupSelector->popupAsync (menu, sizeInfo, useCompactContextMenu ? MenuPresentation::kCompact : ""));

	SharedPtr<ContextPopupMenu> contextMenu (this); // keep alive while non-modal window is open
	SignalSource (Signals::kControls).signal (Message (Signals::kContextMenuOpened, true));
	promise.then ([contextMenu, this] (IAsyncOperation& operation)
	{
		SignalSource (Signals::kControls).signal (Message (Signals::kContextMenuOpened, false));
		popupSelector.release ();

		if(nonModalInstance == contextMenu)
			nonModalInstance = nullptr;
	});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContextPopupMenu::addHeaderItem (StringRef title)
{
	ASSERT (!title.isEmpty ()) // title has to be translated!
	UnknownPtr<IExtendedMenu> extendedMenu (menu->asUnknown ());
	if(extendedMenu.isValid () == false)
		return kResultFalse;
	extendedMenu->addHeaderItem (title);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContextPopupMenu::addCommandItem (StringRef title, CStringRef category, CStringRef name, ICommandHandler* handler)
{
	ASSERT (!title.isEmpty ()) // title has to be translated!
	MenuItem* item = menu->addItem (String (name), title, handler);
	item->setCategory (String (category));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContextPopupMenu::removeCommandItem (CStringRef category, CStringRef name)
{
	if(MenuItem* item = menu->findCommandItem (category, name))
	{
		menu->removeItem (item);
		return kResultOk;
	}
	return kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContextPopupMenu::addSeparatorItem ()
{
	menu->addSeparatorItem ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ContextPopupMenu::hasCommandHandler (ICommandHandler* handler) const
{
	for(int i = 0; i < menu->countItems (); i++)
		if(menu->at (i)->getHandler () == handler)
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ContextPopupMenu::hasCommandItem (CStringRef category, CStringRef name) const
{
	return menu->findCommandItem (category, name) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IContextMenu* CCL_API ContextPopupMenu::addSubContextMenu (StringRef title)
{
	Menu* subMenu = unknown_cast<Menu> (static_cast<IMenu*> (menu)->createMenu ());
	subMenu->setTitle (title);
	MenuItem* subMenuItem = menu->addMenu (subMenu);

	ContextPopupMenu* subContextMenu = NEW ContextPopupMenu (subMenu);
	subContextMenus.add (subContextMenu);
	return subContextMenu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ContextPopupMenu::setInitialSubMenu (StringRef path)
{
	menu->setInitialSubMenuPath (path);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ContextPopupMenu::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kHideContextMenu)
	{
		if(popupSelector && this == nonModalInstance)
			if(auto window = unknown_cast<PopupWindow> (popupSelector->getCurrentWindow ()))
				window->hide ();
	}
	else if(msg == Signals::kRestoreContextMenu)
	{
		if(popupSelector && this == nonModalInstance)
			if(auto window = unknown_cast<PopupWindow> (popupSelector->getCurrentWindow ()))
			{
				if(msg.getArgCount () >= 2)
				{
					Point pos (msg[0].asInt (), msg[1].asInt ());
					Rect rect (window->getSize ());
					rect.moveTo (pos);
					window->setSize (rect);
				}
				window->show ();
			}
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ContextPopupMenu::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "popupMenu")
	{
		var.takeShared (ccl_as_unknown (menu));
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}
