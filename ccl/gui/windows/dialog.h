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
// Filename    : ccl/gui/windows/dialog.h
// Description : Dialog Window
//
//************************************************************************************************

#ifndef _ccl_dialog_h
#define _ccl_dialog_h

#include "ccl/gui/windows/nativewindow.h"

namespace CCL {

interface IAsyncOperation;

//************************************************************************************************
// Dialog
//************************************************************************************************

class Dialog: public NativeDialog
{
public:
	DECLARE_CLASS (Dialog, NativeDialog)

	Dialog (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);

	DECLARE_STYLEDEF (dialogButtons)

	PROPERTY_VARIABLE (int, dialogResult, DialogResult)

	bool isCanceled () const;
	void setFirstFocusView (IView* v);
	void initFocusView ();

	virtual int showModal (IWindow* parentWindow = nullptr);
	virtual IAsyncOperation* showDialog (IWindow* parentWindow = nullptr);

	// NativeDialog
	bool onKeyDown (const KeyEvent& event) override;

protected:
	ObservedPtr<IView> firstFocusView;

	// platform-specific methods:
	virtual IAsyncOperation* showPlatformDialog (IWindow* parent);
};

} // namespace CCL

#endif // _ccl_dialog_h
