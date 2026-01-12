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
// Filename    : ccl/public/gui/framework/imousecursor.h
// Description : Mouse Cursor Interface
//
//************************************************************************************************

#ifndef _ccl_imousecursor_h
#define _ccl_imousecursor_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IImage;

//************************************************************************************************
// IMouseCursor
/** Mouse cursor interface. 
	\ingroup gui */
//************************************************************************************************

interface IMouseCursor: IUnknown
{
	/** Create image representing the cursor shape (might return null). */
	virtual IImage* CCL_API createImage () const = 0;
		
	DECLARE_IID (IMouseCursor)
};

DEFINE_IID (IMouseCursor, 0xfbf93850, 0x22cb, 0x4c29, 0x88, 0xf1, 0xf, 0x1b, 0x69, 0xa3, 0x8e, 0xf0)

} // namespace CCL

#endif // _ccl_imousecursor_h
