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
// Filename    : ccl/public/gui/framework/iclipboard.h
// Description : Clipboard Interface
//
//************************************************************************************************

#ifndef _ccl_iclipboard_h
#define _ccl_iclipboard_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IConvertFilter;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Clipboard Formats
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClipboardFormat
{
	/** UTF-16 Text [IMemoryStream] */
	DEFINE_CID (UnicodeText, 0x8c89995d, 0x5cd0, 0x47f9, 0xaa, 0x63, 0x74, 0x71, 0xf3, 0xd7, 0xe3, 0xc3);
}

//************************************************************************************************
// IClipboard
/** Clipboard interface.
\ingroup gui_data  */
//************************************************************************************************

interface IClipboard: IUnknown
{
	/** Check if clipboard is empty. */
	virtual tbool CCL_API isEmpty () const = 0;

	/** Get current content. */
	virtual IUnknown* CCL_API getContent () const = 0;

	/** Set current content (clipboard takes ownership!). */
	virtual tbool CCL_API setContent (IUnknown* object) = 0;

	/** Empty clipboard. */
	virtual tbool CCL_API empty () = 0;

	/** Copy text to clipboard. */
	virtual tbool CCL_API setText (StringRef text) = 0;

	/** Paste text from clipboard. */
	virtual tbool CCL_API getText (String& text) const = 0;

	/** Register conversion filter. */
	virtual void CCL_API registerFilter (IConvertFilter* filter) = 0;

	/** Unregister conversion filter. */
	virtual void CCL_API unregisterFilter (IConvertFilter* filter) = 0;

	DECLARE_IID (IClipboard)
};

DEFINE_IID (IClipboard, 0xf53fafef, 0xead8, 0x41d5, 0x9d, 0x44, 0xfc, 0xcc, 0x5d, 0xd9, 0x99, 0x85)

} // namespace CCL

#endif // _ccl_iclipboard_h
