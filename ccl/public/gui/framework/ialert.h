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
// Filename    : ccl/public/gui/framework/ialert.h
// Description : Alert Interface
//
//************************************************************************************************

#ifndef _ccl_ialert_h
#define _ccl_ialert_h

#include "ccl/public/system/alerttypes.h"

namespace CCL {

interface IErrorContext;
interface IProgressNotify;
interface IAttributeList;
interface IAsyncOperation;
interface IDialogInformation;
interface IMenu;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** Alert Box [IAlertBox] */
	DEFINE_CID (AlertBox, 0x9bf3ecb5, 0x5bb2, 0x4eb4, 0xaa, 0xac, 0x29, 0xaf, 0xf4, 0x66, 0x45, 0xa5)
	
	/** Dialog Information [IDialogInformation] */
	DEFINE_CID (DialogInformation, 0x7d2b332a, 0x107c, 0x4bb9, 0x88, 0x23, 0xcb, 0x5f, 0x31, 0x9c, 0xbf, 0x6)
}

//////////////////////////////////////////////////////////////////////////////////////////////////
/** Alert functions
\ingroup gui_dialog */
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Alert
{
	/** Question type. */
	DEFINE_ENUM (QuestionType)
	{
		kYesNo,				///< Yes/No
		kYesNoCancel,		///< Yes/No/Cancel
		kOkCancel,			///< Ok/Cancel
		kRetryCancel,		///< Retry/Cancel
		kNumQuestionTypes
	};

	/** Standard alert results. */
	DEFINE_ENUM (StandardResult)
	{
		kYes,		///< Yes
		kNo,		///< No
		kCancel,	///< Cancel
		kOk,		///< Ok
		kRetry		///< Retry
	};

	/** Results for customized buttons. */
	DEFINE_ENUM (CustomResult)
	{
		kFirstButton,	///< first button
		kSecondButton,	///< second button
		kThirdButton,	///< third button		
		kLastButton = kThirdButton,
		
		kEscapePressed	///< user pressed escape/close (none of the three buttons)
	};

	void warn (StringRef text);
	IAsyncOperation* warnAsync (StringRef text);

	void info (StringRef text);
	IAsyncOperation* infoAsync (StringRef text);

	void error (StringRef text);
	IAsyncOperation* errorAsync (StringRef text);

	int ask (StringRef text, int type = kYesNo);
	IAsyncOperation* askAsync (StringRef text, int type = kYesNo);

	int ask (StringRef text, StringRef firstButton, StringRef secondButton, StringRef thirdButton = nullptr);
	IAsyncOperation* askAsync (StringRef text, StringRef firstButton, StringRef secondButton, StringRef thirdButton = nullptr);

	bool notify (StringRef text, int type = kInformation);

	IReporter& getReporter (bool silent = false);

	void errorWithContext (StringRef text, bool forceDialog = false);
	IAsyncOperation* errorWithContextAsync (StringRef text, bool forceDialog = false);

	int askWithContext (StringRef text, int type = kYesNo);
	IAsyncOperation* askWithContextAsync (StringRef text, int type = kYesNo);

	StringRef button (StandardResult result);
}

//************************************************************************************************
// IAlertService
/** Alert Serice
\ingroup gui_dialog */
//************************************************************************************************

interface IAlertService: Alert::IReporter
{
	virtual void CCL_API setTitle (StringRef title) = 0;

	virtual void CCL_API setNotificationReporter (IReporter* notifier) = 0;

	virtual void CCL_API setProgressReporter (IProgressNotify* progress, tbool state) = 0;

	virtual tbool CCL_API showNotification (StringRef text, int type = Alert::kInformation) = 0;

	virtual IDialogInformation* CCL_API getCurrentDialog () const = 0;

	virtual StringRef CCL_API getButtonTitle (int standardResult) const = 0;

	virtual IProgressNotify* CCL_API getCurrentProgressDialog () const = 0;

	DECLARE_STRINGID_MEMBER (kBeginDialog)	///< args[0]: IDialogInformation
	DECLARE_STRINGID_MEMBER (kEndDialog)	///< args[0]: IDialogInformation

	DECLARE_IID (IAlertService)
};

DEFINE_IID (IAlertService, 0xef5e6c54, 0xb675, 0x48ff, 0x90, 0x3d, 0x32, 0xbf, 0x34, 0xd6, 0xf6, 0xdc)
DEFINE_STRINGID_MEMBER (IAlertService, kBeginDialog, "beginDialog")
DEFINE_STRINGID_MEMBER (IAlertService, kEndDialog, "endDialog")

//************************************************************************************************
// IAlertBox
/** Alert Box interface 
\ingroup gui_dialog */
//************************************************************************************************

interface IAlertBox: IUnknown
{
	virtual void CCL_API initWithType (StringRef text, int type = Alert::kWarning) = 0;
	
	virtual void CCL_API initWithQuestion (StringRef text, int type = Alert::kYesNo) = 0;

	virtual void CCL_API initWithButtons (StringRef text, StringRef firstButton, StringRef secondButton, StringRef thirdButton = nullptr) = 0;

	virtual void CCL_API initWithContext (StringRef text, IErrorContext* context, int question = -1) = 0;

	virtual int CCL_API run () = 0;

	virtual IAsyncOperation* CCL_API runAsync () = 0;

	DECLARE_IID (IAlertBox)
};

DEFINE_IID (IAlertBox, 0xfc819a81, 0x771f, 0x40bb, 0xb8, 0x89, 0x8d, 0xdf, 0xd6, 0xb5, 0x63, 0x58)

//************************************************************************************************
// IDialogInformation
/** Dialog Information
\ingroup gui_dialog */
//************************************************************************************************

interface IDialogInformation: IUnknown
{
	DEFINE_ENUM (DialogType)
	{
		kStandardAlert,
		kStandardDialog,
		kMenuDialog
		// TODO: kSystemDialog
	};

	virtual DialogType CCL_API getDialogType () const = 0;

	virtual StringRef CCL_API getDialogText () const = 0;

	virtual StringRef CCL_API getDialogTitle () const = 0;

	virtual StringRef CCL_API getButtonTitle (int index) const = 0;

	virtual IMenu* CCL_API getMenu () const = 0;

	virtual void CCL_API close (int buttonIndex) = 0;
	
	virtual tbool CCL_API getAttributes (IAttributeList& attributes) const = 0;

	virtual tbool CCL_API setAttributes (const IAttributeList& attributes) = 0;

	DECLARE_IID (IDialogInformation)
};

DEFINE_IID (IDialogInformation, 0xf32761ce, 0x57bf, 0x4544, 0xa7, 0xb9, 0x87, 0xd0, 0xdf, 0x91, 0x91, 0x7f)

} // namespace CCL

#endif // _ccl_ialert_h
