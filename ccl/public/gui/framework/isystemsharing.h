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
// Filename    : ccl/public/gui/framework/isystemsharing.h
// Description : System Sharing Interface
//
//************************************************************************************************

#ifndef _ccl_isystemsharing_h
#define _ccl_isystemsharing_h

#include "ccl/public/text/cclstring.h"

namespace CCL {

interface IAsyncOperation;
interface IProgressNotify;
interface IWindow;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in classes (optional, not available on all platforms)
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (SystemSharingHandler, 0x3421790e, 0x33c8, 0x430a, 0xa4, 0x98, 0x97, 0x1f, 0x0d, 0xb2, 0x56, 0x22)
}

//************************************************************************************************
// ISystemSharingHandler
/** System sharing handler interface. Performs "Send to" funtionality for text and files on mobile devices.
	Created with ClassID::SystemSharingHandler
\ingroup gui_dialog */
//************************************************************************************************

interface ISystemSharingHandler: IUnknown
{
	/** Prompt the user to choose an app or contact to send this file to. */
	virtual IAsyncOperation* CCL_API shareFile (UrlRef url, IWindow* window = nullptr) = 0;

	/** Prompt the user to choose an app or contact to send this string to. */
	virtual IAsyncOperation* CCL_API shareText (StringRef text, IWindow* window = nullptr) = 0;

	DECLARE_IID (ISystemSharingHandler)
};

DEFINE_IID (ISystemSharingHandler, 0x88195d4a, 0x53b4, 0x414c, 0x8b, 0x52, 0x40, 0x7d, 0x6a, 0x12, 0x4f, 0x66)

} // namespace CCL

#endif // _ccl_isystemsharing_h
