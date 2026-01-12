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
// Filename    : ccl/gui/dialogs/alert.h
// Description : Alert Dialogs
//
//************************************************************************************************

#ifndef _ccl_alert_h
#define _ccl_alert_h

#include "ccl/base/object.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/collections/stack.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/imenu.h"

namespace CCL {

class AlertBox;
class DialogInformation;

//************************************************************************************************
// AlertService
//************************************************************************************************

class AlertService: public Object,
					public IAlertService
{
public:
	DECLARE_CLASS_ABSTRACT (AlertService, Object)

	AlertService ();
	~AlertService ();

	static AlertService& instance ();
	static void printErrorMessagesDeep (String& text, IErrorContext& context);

	struct DialogScope
	{
		DialogInformation& information;

		DialogScope (DialogInformation& information);
		~DialogScope ();
	};

	void beginDialog (DialogInformation& information);
	void endDialog (DialogInformation& information);
	
	StringRef getTitle () const;
	IProgressNotify* getProgressReporter ();

	// IAlertService
	void CCL_API setTitle (StringRef title) override;
	void CCL_API setNotificationReporter (IReporter* notifier) override;
	void CCL_API setProgressReporter (IProgressNotify* progress, tbool state) override;
	tbool CCL_API showNotification (StringRef text, int type = Alert::kInformation) override;
	IDialogInformation* CCL_API getCurrentDialog () const override;
	StringRef CCL_API getButtonTitle (int standardResult) const override;
	IProgressNotify* CCL_API getCurrentProgressDialog () const override;

	// Alert::IReporter
	void CCL_API reportEvent (const Alert::Event& e) override;
	void CCL_API setReportOptions (Severity minSeverity, int eventFormat) override;

	CLASS_INTERFACE2 (IAlertService, IReporter, Object)

protected:
	class ProgressList: public Object,
						public AbstractProgressNotify,
						public Vector<IProgressNotify*>
	{
	public:
		// IProgressNotify
		void CCL_API beginProgress () override;
		void CCL_API endProgress () override;
		void CCL_API updateProgress (const State& state) override;

		CLASS_INTERFACE (IProgressNotify, Object)
	};

	String title;
	IReporter* notifier;
	ProgressList progressList;
	Stack<DialogInformation*> dialogInformationStack;

	void showAlert (StringRef text, int type);
};

//************************************************************************************************
// DialogInformation
//************************************************************************************************

class DialogInformation: public Object,
						 public IDialogInformation
{
public:
	DECLARE_CLASS (DialogInformation, Object)

	DialogInformation (DialogType type = kStandardAlert, StringRef text = nullptr, StringRef title = nullptr);

	PROPERTY_VARIABLE (DialogType, type, Type)
	PROPERTY_STRING (text, Text)
	PROPERTY_STRING (title, Title)

	PROPERTY_STRING (firstButton, FirstButton)
	PROPERTY_STRING (secondButton, SecondButton)
	PROPERTY_STRING (thirdButton, ThirdButton)

	void setButtonTitle (int index, StringRef title);
	void setMenu (IMenu* menu);

	// IDialogInformation
	DialogType CCL_API getDialogType () const override	{ return getType (); }
	StringRef CCL_API getDialogText () const override	{ return getText (); }
	StringRef CCL_API getDialogTitle () const override	{ return getTitle (); }
	StringRef CCL_API getButtonTitle (int index) const override;
	IMenu* CCL_API getMenu () const override;
	void CCL_API close (int buttonIndex) override;
	tbool CCL_API getAttributes (IAttributeList& attributes) const override;
	tbool CCL_API setAttributes (const IAttributeList& attributes) override;

	CLASS_INTERFACE (IDialogInformation, Object)

protected:
	SharedPtr<IMenu> menu;
};

//************************************************************************************************
// AlertBox
//************************************************************************************************

class AlertBox: public DialogInformation,
				public IAlertBox
{
public:
	DECLARE_CLASS (AlertBox, DialogInformation)
	
	static AlertBox* create ();

	PROPERTY_VARIABLE (int, alertType, AlertType)
	PROPERTY_VARIABLE (int, questionType, QuestionType)

	PROPERTY_VARIABLE (int, firstResult, FirstResult)
	PROPERTY_VARIABLE (int, secondResult, SecondResult)
	PROPERTY_VARIABLE (int, thirdResult, ThirdResult)

	void setButtonResult (int index, int result);
	int getButtonResult (int index) const;
	bool isUsingCustomButtonResults () const; ///< caller semantic is kFirstButton instead of kYes

	PROPERTY_POINTER (void, platformHandle, PlatformHandle)
	PROPERTY_VARIABLE (int, closeResult, CloseResult)

	// IAlertBox
	void CCL_API initWithType (StringRef text, int type = Alert::kWarning) override;
	void CCL_API initWithQuestion (StringRef text, int = Alert::kYesNo) override;
	void CCL_API initWithButtons (StringRef text, StringRef firstButton, StringRef secondButton, StringRef thirdButton = nullptr) override;
	void CCL_API initWithContext (StringRef text, IErrorContext* context, int question = -1) override;
	int CCL_API run () override;
	IAsyncOperation* CCL_API runAsync () override;

	// DialogInformation
	void CCL_API close (int buttonIndex) override;

	CLASS_INTERFACE (IAlertBox, DialogInformation)
	
protected:
	AlertBox ();
	void onAlertCompleted (IAsyncOperation& operation);

	// platform-specific:
	virtual void closePlatform ();
	virtual IAsyncOperation* runAsyncPlatform ();
};

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Private alert definitions. */
namespace Alert
{
	static const int kUndefined = -1;
	static const int kQuestionTypeCustom = kNumQuestionTypes;

	struct ButtonMapping
	{
		int defaultResult;
		int alternateResult;
		int otherResult;

		int getResultAtButtonIndex (int buttonIndex) const; ///< platform-specific!
	};
}

} // namespace CCL

#endif // _ccl_alert_h
