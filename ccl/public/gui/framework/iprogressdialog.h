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
// Filename    : ccl/public/gui/framework/iprogressdialog.h
// Description : Progress Dialog Interface
//
//************************************************************************************************

#ifndef _ccl_iprogressdialog_h
#define _ccl_iprogressdialog_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IWindow;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** Progress dialog (supports IProgressNotify, IProgressDialog, IProgressDetails). */
	DEFINE_CID (ProgressDialog, 0x70346f66, 0x3984, 0x45b3, 0xa5, 0x7c, 0xa6, 0x10, 0x0, 0xf2, 0x39, 0xc0)

	/** Modal progress dialog. */
	DEFINE_CID (ModalProgressDialog, 0x75bd62fa, 0xe314, 0x49a2, 0x87, 0xff, 0xfa, 0x7b, 0x3, 0xcd, 0xbd, 0x16)
};

//************************************************************************************************
// IProgressDialog
/** Progress dialog interface.
	\ingroup gui_dialog */
//************************************************************************************************

interface IProgressDialog: IUnknown
{
	/** Constrain the minimum and maximum number of visible progress bars. */
	virtual void CCL_API constrainLevels (int min, int max) = 0;

	/** Set delay for deferred opening of the dialog (0: open immediately). */
	virtual void CCL_API setOpenDelay (double seconds, tbool showWaitCursorBeforeOpen = false) = 0;

	/** Set translucent window appearance. */
	virtual void CCL_API setTranslucentAppearance (tbool state) = 0;

	/** Set parent window (optional). */
	virtual void CCL_API setParentWindow (IWindow* window) = 0;

	/** Cancel progress dialog programmatically. */
	virtual void CCL_API tryCancel () = 0;
	
	/** Hide progress dialog window */
	virtual void CCL_API hideWindow (tbool state) = 0;

	/** Sent once by dialog when cancel button is pressed. */
	DECLARE_STRINGID_MEMBER (kCancelButtonHit)

	DECLARE_IID (IProgressDialog)
};

DEFINE_IID (IProgressDialog, 0xF1D4A5CD, 0x17C5, 0x4A49, 0x9D, 0x44, 0x27, 0x7C, 0xEA, 0x35, 0xDF, 0xD1)
DEFINE_STRINGID_MEMBER (IProgressDialog, kCancelButtonHit, "cancelButtonHit")

//************************************************************************************************
// IModalProgressDialog
/**	Modal progress dialog interface. 
	Use when GUI should wait for an operation to be finished by another thread. 
	\ingroup gui_dialog */
//************************************************************************************************

interface IModalProgressDialog: IUnknown
{
	/** Run modal progress dialog.
		This method will not return until dialog is closed.
		Works on desktop platforms only. */
	virtual void CCL_API run () = 0;
	
	/** Close progress dialog. */
	virtual void CCL_API close () = 0;

	DECLARE_IID (IModalProgressDialog)
};

DEFINE_IID (IModalProgressDialog, 0x18e87d5b, 0xeb5, 0x4f68, 0xaa, 0xa7, 0xd, 0x2d, 0x1e, 0xbb, 0x8c, 0xcb)

//************************************************************************************************
// ProgressDialogHideScope
/** \ingroup gui_dialog  */
//************************************************************************************************

struct ProgressDialogHideScope
{
	IProgressDialog* progressDialog;
	
	ProgressDialogHideScope (IProgressDialog* progressDialog)
	: progressDialog (progressDialog)
	{
		if(progressDialog)
			progressDialog->hideWindow (true);
	}
	
	~ProgressDialogHideScope ()
	{
		if(progressDialog)
			progressDialog->hideWindow (false);
	}
};

} // namespace CCL

#endif // _ccl_iprogressdialog_h
