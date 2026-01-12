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
// Filename    : ccl/public/gui/framework/idialogbuilder.h
// Description : Dialog Builder Interface
//
//************************************************************************************************

#ifndef _ccl_idialogbuilder_h
#define _ccl_idialogbuilder_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"

#include "ccl/public/gui/framework/styleflags.h"
#include "ccl/public/gui/framework/controlstyles.h"

namespace CCL {

interface IView;
interface IWindow;
interface IParameter;
interface IController;
interface ITheme;
interface ITranslationTable;
interface IAsyncOperation;
interface IMenu;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** Dialog builder. */
	DEFINE_CID (DialogBuilder, 0x352f4422, 0x89bc, 0x437c, 0x99, 0x77, 0x82, 0xf9, 0xfc, 0xb0, 0x63, 0x5)
}

//************************************************************************************************
// IDialogBuilder
/** Dialog builder interface. Use DialogBox helper class in application code.
	\ingroup gui_dialog */
//************************************************************************************************

interface IDialogBuilder: IUnknown
{
	/** Assign theme. */
	virtual void CCL_API setTheme (ITheme* theme) = 0;

	/** Assign translation table. */
	virtual void CCL_API setStrings (ITranslationTable* table) = 0;

	/** Run modal dialog for given view (takes ownership of view). */
	virtual int CCL_API runDialog (IView* view, int dialogStyle = Styles::kWindowCombinedStyleDialog,
								   int buttons = 0, IWindow* parentWindow = nullptr) = 0;

	/** Run modal dialog for given view asynchronously (takes ownership of view). */
	virtual IAsyncOperation* CCL_API runDialogAsync (IView* view, int dialogStyle = Styles::kWindowCombinedStyleDialog,
													 int buttons = 0, IWindow* parentWindow = nullptr) = 0;

	/** Run modal dialog with parameter list. */
	virtual int CCL_API runWithParameters (StringRef name, IController& paramList, 
										   StringRef title, int dialogStyle = Styles::kWindowCombinedStyleDialog,
										   int buttons = Styles::kDialogOkCancel, IWindow* parentWindow = nullptr) = 0;
	
	/** Run modal dialog with parameter list asynchronously. */
	virtual IAsyncOperation* CCL_API runWithParametersAsync (StringRef name, IController& paramList,
															 StringRef title, int dialogStyle = Styles::kWindowCombinedStyleDialog,
															 int buttons = Styles::kDialogOkCancel, 
															 IWindow* parentWindow = nullptr) = 0;

	/** Run modal dialog asking for a string. */
	virtual tbool CCL_API askForString (String& string, StringID label,StringRef title, StringRef dialogName = nullptr) = 0;

	/** Run modal dialog asking for a string asynchronously. */
	virtual IAsyncOperation* CCL_API askForStringAsync (StringRef string, StringID label, StringRef title, StringRef dialogName = nullptr) = 0;

	/** Run dialog with menu. */
	virtual void CCL_API runWithMenu (IMenu* menu, StringRef title, StringRef text) = 0;

	/** Run dialog with menu asynchronously. */
	virtual IAsyncOperation* CCL_API runWithMenuAsync (IMenu* menu, StringRef title, StringRef text) = 0;

	/** Add a parameter for a custom dialog button. buttonRole specifies which button in the (platform specific) button order should be replaced by this button. */
	virtual void CCL_API addCustomButton (IParameter* param, StringRef title, int buttonRole = Styles::kOkayButton) = 0;

	/** Set dialog result code. */
	virtual void CCL_API setDialogResult (int dialogResult) = 0;

	/** Close modal dialog */
	virtual void CCL_API close () = 0;

	/** Exclude style flags, e.g. in order to override default styles */
	virtual void CCL_API excludeStyleFlags (StyleRef flags) = 0;

	DECLARE_IID (IDialogBuilder)
};

DEFINE_IID (IDialogBuilder, 0x63732896, 0xf562, 0x4acc, 0xa0, 0x83, 0xd6, 0x14, 0xe, 0x45, 0xbe, 0x17)

//************************************************************************************************
// IDialogButtonInterest
/** Callback interface to intercept dialog button presses.
	\ingroup gui_dialog */
//************************************************************************************************

interface IDialogButtonInterest: IUnknown
{
	/** Called when the dialog button is created. Needed to manage its enabled state. */
	virtual void CCL_API setDialogButton (IParameter* button, int which) = 0;

	/** Called when the dialog button is hit. Return true to avoid default processing. */
	virtual tbool CCL_API onDialogButtonHit (int which) = 0;

	DECLARE_IID (IDialogButtonInterest)
};

DEFINE_IID (IDialogButtonInterest, 0x61FDC184, 0x2A74, 0x4CBA, 0xB0, 0x6D, 0x69, 0x35, 0x38, 0x83, 0x4D, 0x57)

} // namespace CCL

#endif // _ccl_idialogbuilder_h
