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
// Filename    : ccl/public/gui/framework/inotifyicon.h
// Description : Notification Icon Interface
//
//************************************************************************************************

#ifndef _ccl_inotifyicon_h
#define _ccl_inotifyicon_h

#include "ccl/public/system/alerttypes.h"

namespace CCL {

interface IImage;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (NotifyIcon, 0x6d51b752, 0xb1c9, 0x44c2, 0xb5, 0xb4, 0x88, 0x6c, 0x61, 0x10, 0xc, 0xe4);
}

//************************************************************************************************
// INotifyIcon
/**
	\ingroup gui_dialog */
//************************************************************************************************

interface INotifyIcon: Alert::IReporter
{
	DECLARE_STRINGID_MEMBER (kContextID)			///< identifier for context menu [IContextMenuHandler]
	DECLARE_STRINGID_MEMBER (kIconClicked)			///< icon click message [IObserver]
	DECLARE_STRINGID_MEMBER (kIconDoubleClicked)	///< icon double-click message [IObserver]
	
	/** Set notification icon image. */
	virtual tresult CCL_API setImage (IImage* image) = 0;

	/** Set notification icon title. */
	virtual tresult CCL_API setTitle (StringRef title) = 0;

	/** Set handler for context menu, etc. */
	virtual tresult CCL_API setHandler (IUnknown* handler) = 0;

	/** Show/hide the notification icon. */
	virtual tresult CCL_API setVisible (tbool state) = 0;

	/** In auto-show mode, the icon will appear only while an event is reported. */
	virtual tresult CCL_API setAutoShow (tbool state) = 0;

	DECLARE_IID (INotifyIcon)
};

DEFINE_IID (INotifyIcon, 0x8e9d2252, 0xe18f, 0x4487, 0xa6, 0xee, 0x18, 0xdb, 0x4b, 0x3f, 0x21, 0x4c)
DEFINE_STRINGID_MEMBER (INotifyIcon, kContextID, "NotifyIconContextMenu")
DEFINE_STRINGID_MEMBER (INotifyIcon, kIconClicked, "NotifyIconClicked")
DEFINE_STRINGID_MEMBER (INotifyIcon, kIconDoubleClicked, "NotifyIconDoubleClicked")

} // namespace CCL

#endif // _ccl_inotifyicon_h
