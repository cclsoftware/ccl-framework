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
// Filename    : ccl/public/gui/icontextmenu.h
// Description : Context Menu Interface
//
//************************************************************************************************

#ifndef _ccl_icontextmenu_h
#define _ccl_icontextmenu_h

#include "ccl/public/gui/icommandhandler.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (ContextMenu, 0x6161c214, 0x351a, 0x4378, 0x94, 0x5f, 0xf1, 0xf2, 0x1c, 0xa8, 0x63, 0x2f);
}

//************************************************************************************************
// IContextMenu
/**
	\ingroup gui_menu */
//************************************************************************************************

interface IContextMenu: IUnknown
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Building the menu
	//////////////////////////////////////////////////////////////////////////////////////////////
	
	/** Add a header. */
	virtual tresult CCL_API addHeaderItem (StringRef title) = 0;

	/** Add an item that fires a command. */
	virtual tresult CCL_API addCommandItem (StringRef title, CStringRef category, CStringRef name, ICommandHandler* handler) = 0;
	
	/** Remove an item. */
	virtual tresult CCL_API removeCommandItem (CStringRef category, CStringRef name) = 0;

	/** Add a separator. Separators are managed internally to avoid consecutive occurances. */
	virtual tresult CCL_API addSeparatorItem () = 0;
	
	/** Check if command handler is already in context menu. */
	virtual tbool CCL_API hasCommandHandler (ICommandHandler* handler) const = 0;

	/** Check if command is already in context menu. */
	virtual tbool CCL_API hasCommandItem (CStringRef category, CStringRef name) const = 0;

	/** Add sub menu. Caller does _not_ own the menu. */
	virtual IContextMenu* CCL_API addSubContextMenu (StringRef title) = 0;

	/** Specify a sub menu that should be opened initially (path of IMenu::kMenuName, separated by '/'). */
	virtual tresult CCL_API setInitialSubMenu (StringRef path) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Passing information from a view to a component
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Set context ID. This ID can help a component to distinguish different views. */
	virtual void CCL_API setContextID (StringID id) = 0;

	/** Get context ID. */
	virtual StringID CCL_API getContextID () const = 0;

	/** Set focus item (item that was mouse-clicked). */
	virtual void CCL_API setFocusItem (IUnknown* item) = 0;

	/** Get focus item. */
	virtual IUnknown* CCL_API getFocusItem () const = 0;

	DECLARE_IID (IContextMenu)

	//////////////////////////////////////////////////////////////////////////////////////////////

	inline tresult addCommandItem (const CommandWithTitle& cwt, ICommandHandler* handler, bool followIndicator = false)
	{
		if(followIndicator == false)
			return addCommandItem (cwt.title, cwt.category, cwt.name, handler);
		else // same as IMenu::strFollowIndicator
			return addCommandItem (String () << cwt.title << CCLSTR ("..."), cwt.category, cwt.name, handler);
	}
};

DEFINE_IID (IContextMenu, 0xebf7a928, 0x8dad, 0x4858, 0x87, 0xbd, 0x92, 0x31, 0x4d, 0xb, 0xca, 0xd2)

//************************************************************************************************
// IContextMenuHandler
/**
	\ingroup gui_menu */
//************************************************************************************************

interface IContextMenuHandler: IUnknown
{
	virtual tresult CCL_API appendContextMenu (IContextMenu& contextMenu) = 0;

	DECLARE_IID (IContextMenuHandler)
};

DEFINE_IID (IContextMenuHandler, 0xbae9709a, 0xaf57, 0x439b, 0x98, 0x9, 0x8e, 0x98, 0x8, 0x46, 0x8d, 0xa2)

} // namespace CCL

#endif // _ccl_icontextmenu_h
