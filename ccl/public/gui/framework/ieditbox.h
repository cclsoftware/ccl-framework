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
// Filename    : ccl/public/gui/framework/ieditbox.h
// Description : Edit Box Interface
//
//************************************************************************************************

#ifndef _ccl_ieditbox_h
#define _ccl_ieditbox_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IEditBox
/** 
	\ingroup gui_view */
//************************************************************************************************

interface IEditBox: IUnknown
{
	/** Insert text at caret */
	virtual void CCL_API insertText (StringRef text) = 0;

	/** Remove text starting at caret. Length may be negative to remove backwards. */
	virtual void CCL_API removeText (int length) = 0;

	/** Set caret to given index. */
	virtual void CCL_API setCaret (int textPosition) = 0;

	DECLARE_IID (IEditBox)
};

DEFINE_IID (IEditBox, 0xf8ea8f86, 0x0a78, 0x4c18, 0x83, 0xc5, 0xd8, 0x18, 0x38, 0x8a, 0x56, 0x5c)

} // namespace CCL

#endif // _ccl_ieditbox_h
