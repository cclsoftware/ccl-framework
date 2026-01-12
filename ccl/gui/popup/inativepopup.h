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
// Filename    : ccl/gui/popup/inativepopup.h
// Description : Native Popup Selector Window
//
//************************************************************************************************

#ifndef _ccl_inativepopup_h
#define _ccl_inativepopup_h

#include "ccl/public/gui/framework/ipopupselector.h"

namespace CCL {

//************************************************************************************************
// INativePopupSelectorWindow
/** Optional interface used on systems which have native support for arranging popup windows. */
//************************************************************************************************

interface INativePopupSelectorWindow: IUnknown
{
	virtual void setSizeInfo (const PopupSizeInfo& sizeInfo) = 0;
	
	DECLARE_IID (INativePopupSelectorWindow)
};

} // namespace CCL

#endif // _ccl_inativepopup_h
