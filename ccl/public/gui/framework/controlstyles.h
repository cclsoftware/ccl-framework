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
// Filename    : ccl/public/gui/framework/controlstyles.h
// Description : Control Styles
//
//************************************************************************************************

#ifndef _ccl_controlstyles_h
#define _ccl_controlstyles_h

#include "ccl/public/base/platform.h"

#include "ccl/meta/generated/cpp/controlstyles-generated.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
/** Dialog Results 
	\ingroup gui_dialog */
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace DialogResult
{
	enum ResultCodes
	{
		kNone = -1,
		kCancel,
		kOkay,
		kClose,
		kApply,
		kNumDialogResults,

		kFirstCustomDialogResult
	};

	constexpr bool isCustomResult (int result ) { return result >= kFirstCustomDialogResult; }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/** Dialog Styles
	\ingroup gui_dialog */
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Styles
{
	enum DialogStyleFlags
	{
		kDialogButtonBase = 1 << (kImageViewLastFlag + 1),

		kCancelButton = kDialogButtonBase << DialogResult::kCancel, ///< user input is discarded, dialog is closed
		kOkayButton   = kDialogButtonBase << DialogResult::kOkay,	///< user input is confirmed, dialog is closed
		kCloseButton  = kDialogButtonBase << DialogResult::kClose,	///< user input is discarded, dialog is closed
		kApplyButton  = kDialogButtonBase << DialogResult::kApply,	///< user input is confirmed, dialog stays open

		kDialogOkCancel = kOkayButton|kCancelButton
	};

	inline int toDialogButton (int dialogResult)
	{
		return (kDialogButtonBase << dialogResult);
	}

	inline int toDialogResult (int dialogButton)
	{
		for(int i = 0; i < DialogResult::kNumDialogResults; i++)
			if(dialogButton == toDialogButton (i))
				return i;
		return DialogResult::kNone;
	}
}

} // namespace CCL

#endif // _ccl_controlstyles_h
