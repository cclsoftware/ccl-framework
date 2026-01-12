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
// Filename    : ccl/gui/layout/idockpanel.h
// Description : Docking Panel Interface
//
//************************************************************************************************

#ifndef _ccl_idockpanel_h
#define _ccl_idockpanel_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IViewFactory;

//************************************************************************************************
// IDockPanelItem
/** Additional interfaces: IObjectNode, IController. */
//************************************************************************************************

interface IDockPanelItem: IUnknown
{
	/** Item state. */
	enum States
	{
		kVisible = 1<<0,	///< item is visible
		kHidable = 1<<1		///< item can be hidden
	};

	/** Initialize item. */
	virtual void CCL_API init (StringRef name, IUnknown* controller, int state, IParameter* titleParam) = 0;

	/** Set view factory (optional). */
	virtual void CCL_API setViewFactory (IViewFactory* factory) = 0;

	/** Show item. */
	virtual void CCL_API show () = 0;

	/** Hide item. */
	virtual void CCL_API hide () = 0;

	/** Hide, remove from parent and release. */
	virtual void CCL_API kill () = 0;

	/** Add sub item (group only). */
	virtual tbool CCL_API addItem (IDockPanelItem* item) = 0;

	/** Remove all sub items (group only). */
	virtual void CCL_API removeItems () = 0;

	/** Find item by controller (group only). */
	virtual IDockPanelItem* CCL_API findItem (IUnknown* controller, tbool deep = false) = 0;

	DECLARE_IID (IDockPanelItem)
};

DEFINE_IID (IDockPanelItem, 0xa9a4b86f, 0x796b, 0x40c1, 0x87, 0xd0, 0xf6, 0xc6, 0xbf, 0x61, 0xfb, 0x92)

//************************************************************************************************
// IDockPanelView
//************************************************************************************************

interface IDockPanelView: IUnknown
{
	/** Assign docking panel tree to view. */
	virtual void CCL_API setItems (IDockPanelItem* items) = 0;

	/** Get associated docking panel tree. */
	virtual IDockPanelItem* CCL_API getItems () = 0;

	DECLARE_IID (IDockPanelView)
};

DEFINE_IID (IDockPanelView, 0x4f802007, 0x7e3a, 0x4aad, 0xb2, 0x6c, 0xf2, 0x3b, 0x1e, 0x51, 0x3d, 0x6a)

} // namespace CCL

#endif // _ccl_idockpanel_h
