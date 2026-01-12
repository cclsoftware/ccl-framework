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
// Filename    : ccl/public/gui/framework/idropbox.h
// Description : Drop Box Interface
//
//************************************************************************************************

#ifndef _ccl_idropbox_h
#define _ccl_idropbox_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IDropBox
/** 
	\ingroup gui_view */
//************************************************************************************************

interface IDropBox: IUnknown
{
	/** Get the view created for a given item. */
	virtual IView* CCL_API getViewItem (ItemIndexRef index) = 0;

	DECLARE_STRINGID_MEMBER (kItemSuffix) ///< suffix for item views

	DECLARE_IID (IDropBox)
};

DEFINE_IID (IDropBox, 0x504D11EA, 0x7EA3, 0x4F61, 0xBC, 0x4F, 0xDC, 0x34, 0x40, 0xFF, 0xE8, 0x2F)
DEFINE_STRINGID_MEMBER (IDropBox, kItemSuffix, "Item")

} // namespace CCL

#endif // _ccl_idropbox_h
