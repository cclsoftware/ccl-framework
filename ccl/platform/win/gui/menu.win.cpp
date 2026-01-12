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
// Filename    : ccl/platform/win/gui/menu.win.cpp
// Description : Platform-specific Menu implementation
//
//************************************************************************************************

#include "ccl/gui/windows/window.h"

#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/bitmapfilter.h"
#include "ccl/gui/graphics/imaging/bitmappainter.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/base/asyncoperation.h"

#include "ccl/public/gui/framework/iwindow.h"

#include "ccl/platform/win/cclwindows.h"
#include "ccl/platform/win/gui/win32graphics.h"
#include "ccl/platform/win/gui/dpihelper.h"
#include "ccl/platform/win/gui/screenscaling.h"
#include "ccl/platform/win/gui/menu.win.h"

using namespace CCL;

//************************************************************************************************
// WindowsPopupMenu
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (WindowsPopupMenu, PopupMenu, "Menu")
DEFINE_CLASS_UID (WindowsPopupMenu, 0x1c1ff2c7, 0xeabe, 0x4b0c, 0xab, 0x94, 0xc2, 0x72, 0x8b, 0xfb, 0xc8, 0x12) // ClassID::Menu

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsPopupMenu* WindowsPopupMenu::fromSystemMenu (HANDLE menu)
{
	MENUINFO mi = {0};
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_MENUDATA;
	::GetMenuInfo ((HMENU)menu,  &mi);
	Object* obj = (Object*)mi.dwMenuData;
	return ccl_cast<WindowsPopupMenu> (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsPopupMenu::WindowsPopupMenu ()
: handle (::CreatePopupMenu ())
{
	MENUINFO mi = {0};
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_MENUDATA|MIM_STYLE;
	mi.dwMenuData = (ULONG_PTR)this;
	mi.dwStyle = MNS_NOTIFYBYPOS; // send WM_MENUCOMMAND instead of WM_COMMAND
	::SetMenuInfo (handle, &mi);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsPopupMenu::~WindowsPopupMenu ()
{
	if(!isAttached ())
		::DestroyMenu (handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenu* CCL_API WindowsPopupMenu::createMenu () const
{
	return NEW WindowsPopupMenu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsPopupMenu::realizeItem (MenuItem* item)
{
	int index = getItemIndex (item, true);
	ASSERT (index >= 0)

	if(item->isSeparator ())
		::InsertMenu (handle, index, MF_BYPOSITION|MF_SEPARATOR, 0, (LPCTSTR)item);
	else
	if(item->isSubMenu ())
	{
		WindowsPopupMenu* subMenu = ccl_cast<WindowsPopupMenu> (item->getSubMenu ());
		ASSERT (subMenu != nullptr)
		if(subMenu)
			::InsertMenu (handle, index, MF_BYPOSITION|MF_ENABLED|MF_POPUP, (UINT_PTR)subMenu->getHandle (), StringChars (subMenu->getTitle ()));
	}
	else
	{
		CCL_PRINTF ("realizeItem %d. (%d) %s\n", index, item->getItemID (), MutableCString (item->getTitle ()).str ())
		::InsertMenu (handle, index, MF_BYPOSITION|MF_STRING, item->getItemID (), StringChars (item->getTitle ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsPopupMenu::unrealizeItem (MenuItem* item)
{
	int index = getItemIndex (item, true);
	ASSERT (index >= 0)

	BOOL result = ::RemoveMenu (handle, index, MF_BYPOSITION);
	ASSERT (result != 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsPopupMenu::updateItem (MenuItem* item)
{
	MENUITEMINFO mi = {0};
	mi.cbSize = sizeof(MENUITEMINFO);

	mi.fMask = MIIM_STATE;
	if(!item->isEnabled ())
		mi.fState |= MFS_DISABLED|MFS_GRAYED;
	if(item->isChecked ())
		mi.fState |= MFS_CHECKED;

	String title;
	if(item->isSubMenu ())
		title = item->getSubMenu ()->getTitle ();
	else
	{
		title = item->getTitle ();
		if(const KeyEvent* key = item->getAssignedKey ())
		{
			String keyString;
			key->toString (keyString, true);
			title << "\t" << keyString;
		}
	}
	CCL_PRINTF ("updateItem (%d) %s\n", item->getItemID (), MutableCString (title).str ())

	mi.fMask |= MIIM_STRING;
	StringChars chars (title);
	mi.dwTypeData = (LPTSTR)(const uchar*)chars;
	mi.cch = title.length ();

	if(item->getIcon ())
	{
		const int kMenuIconSize = 20;

		// convert image to bitmap with unified menu icon size
		Bitmap* bitmap = ccl_cast<Bitmap> (item->getNativeIcon ());
		if(bitmap == nullptr)
		{
			item->getIcon ()->setCurrentFrame (0); // always draw first frame to get a consistent result

			if(Win32::gDpiInfo.isDpiAware ())
			{
				AutoPtr<Bitmap> newBitmap = NEW Bitmap (kMenuIconSize, kMenuIconSize, Bitmap::kRGBAlpha, Win32::gDpiInfo.getSystemDpiFactor ());

				Rect iconRect (0, 0, kMenuIconSize, kMenuIconSize);
				Rect src;
				item->getIcon ()->getSize (src);
				Rect dst (src);
				dst.center (iconRect);

				{
					BitmapGraphicsDevice device (newBitmap);
					device.drawImage (item->getIcon (), src, dst);
				}

				bitmap = newBitmap;
				item->keepNativeIcon (bitmap);
			}
			else // legacy code compatible with GDI where alpha behavior is inconsistent
			{
				BitmapProcessor processor;
				const Point menuIconSize (kMenuIconSize, kMenuIconSize);
				processor.setup (item->getIcon (), Colors::kWhite, 0, &menuIconSize);
				#if 0 // TBD: adjust color for template icons
				if(item->getIcon ()->getIsTemplate ())
				{
					BitmapFilters::RevertPremultipliedAlpha reverter;
					BitmapFilters::Colorizer colorizer;
					BitmapFilters::PremultipliedAlpha premultiplier;

					Color color = Win32::fromSystemColor (::GetSysColor (COLOR_MENUTEXT));
					colorizer.setColor (color);

					BitmapFilterList filterList;
					filterList.addFilter (&reverter, true);
					filterList.addFilter (&colorizer, true);
					filterList.addFilter (&premultiplier, true);
					processor.process (static_cast<IBitmapFilterList&> (filterList));
				}
				else
				#endif
				{
					BitmapFilterList copyFilter;
					processor.process (static_cast<IBitmapFilterList&> (copyFilter));
				}

				bitmap = unknown_cast<Bitmap> (processor.getOutput ());
				item->keepNativeIcon (bitmap);
			}
		}

		ASSERT (bitmap != nullptr)
		if(bitmap)
		{
			UnknownPtr<Win32::IWin32Bitmap> gdiBitmap = ccl_as_unknown (bitmap->getNativeBitmap ());
			ASSERT (gdiBitmap != nullptr)
			if(gdiBitmap)
			{
				mi.fMask |= MIIM_BITMAP;
				mi.hbmpItem = gdiBitmap->getHBITMAP ();
				ASSERT (mi.hbmpItem != nullptr)
			}
		}
	}


//	if(item->isSubMenu ())
	{
		// TO BE TESTED: use index to get sub menus working
		int itemPosition = items.index (item);
		ASSERT (itemPosition != -1)
		::SetMenuItemInfo (handle, itemPosition, TRUE, &mi);
	}
//	else
//		::SetMenuItemInfo (handle, item->getItemID (), FALSE, &mi);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* WindowsPopupMenu::popupPlatformMenu (const Point& where, IWindow* window)
{
	HWND hwnd = window ? (HWND)window->getSystemWindow () : nullptr;
	ASSERT (hwnd != nullptr)

	MENUINFO mi = {0};
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_STYLE;
	mi.dwStyle = 0;
	::SetMenuInfo (handle, &mi);

	Point p (where);
	Win32::gScreens->toPixelPoint (p);
	int result = ::TrackPopupMenu (handle, TPM_RETURNCMD, p.x, p.y, 0, hwnd, nullptr);

	// return an AsyncOperation (already completed, since we ran modally)
	return AsyncOperation::createCompleted ((MenuItemID)result);
}

//************************************************************************************************
// WindowsMenuBar
//************************************************************************************************

DEFINE_CLASS (WindowsMenuBar, MenuBar)
DEFINE_CLASS_UID (WindowsMenuBar, 0x32ac7729, 0x5ee3, 0x4273, 0xaf, 0x9d, 0xaf, 0x50, 0x1e, 0x7c, 0xe5, 0xb0) // ClassID::MenuBar

DEFINE_CLASS (WindowsVariantMenuBar, WindowsMenuBar)
DEFINE_CLASS_UID (WindowsVariantMenuBar, 0xd0d769c9, 0xe469, 0x445a, 0xb1, 0x9, 0x66, 0x7f, 0x55, 0xe1, 0xa0, 0xf5) // ClassID::VariantMenuBar

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsMenuBar* WindowsMenuBar::fromSystemMenu (HMENU menu)
{
	MENUINFO mi = {0};
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_MENUDATA;
	::GetMenuInfo ((HMENU)menu,  &mi);
	Object* obj = (Object*)mi.dwMenuData;
	return ccl_cast<WindowsMenuBar> (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsMenuBar::WindowsMenuBar ()
: handle (::CreateMenu ())
{
	MENUINFO mi = {0};
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_MENUDATA|MIM_STYLE;
	mi.dwMenuData = (ULONG_PTR)this;
	mi.dwStyle = MNS_NOTIFYBYPOS;
	::SetMenuInfo (handle, &mi);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowsMenuBar::~WindowsMenuBar ()
{
	if(!window)
		::DestroyMenu (handle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsMenuBar::insertPlatformMenu (PopupMenu* menu)
{
	WindowsPopupMenu* windowsMenu = ccl_cast<WindowsPopupMenu> (menu);
	int index = menus.index (menu);
	ASSERT (index >= 0)
	ASSERT (menu)

	BOOL result = ::InsertMenu (handle, index, MF_BYPOSITION|MF_ENABLED|MF_POPUP, (UINT_PTR)windowsMenu->getHandle (), StringChars (menu->getTitle ()));
	ASSERT (result != 0)

	HWND hwnd = window ? (HWND)window->getSystemWindow () : nullptr;
	if(hwnd)
		::DrawMenuBar (hwnd);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsMenuBar::removePlatformMenu (PopupMenu* menu)
{
	int index = menus.index (menu);
	ASSERT (index >= 0)

	BOOL result = ::RemoveMenu (handle, index, MF_BYPOSITION);
	ASSERT (result != 0)

	HWND hwnd = window ? (HWND)window->getSystemWindow () : nullptr;
	if(hwnd)
		::DrawMenuBar (hwnd);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowsMenuBar::updateMenu (Menu* menu)
{
	int index = menus.index (menu);
	ASSERT (index != -1)

	MENUITEMINFO mi = {0};
	mi.cbSize = sizeof(MENUITEMINFO);
	mi.fMask = MIIM_STRING;
	StringChars chars (menu->getTitle ());
	mi.dwTypeData = (LPTSTR)(const uchar*)chars;
	mi.cch = menu->getTitle ().length ();

	::SetMenuItemInfo (handle, index, TRUE, &mi);
}
