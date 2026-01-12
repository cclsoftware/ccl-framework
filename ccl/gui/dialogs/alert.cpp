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
// Filename    : ccl/gui/dialogs/alert.cpp
// Description : Alert Dialogs
//
//************************************************************************************************

#include "ccl/gui/dialogs/alert.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/popup/extendedmenu.h"
#include "ccl/gui/dialogs/progressdialog.h"

#include "ccl/base/kernel.h"
#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/ierrorhandler.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Alert")
	XSTRING (Yes, "Yes")
	XSTRING (No, "No")
	XSTRING (Okay, "OK")
	XSTRING (Cancel, "Cancel")
	XSTRING (Retry, "Retry")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IAlertService& CCL_API System::CCL_ISOLATED (GetAlertService) ()
{
	return AlertService::instance ();
}

//************************************************************************************************
// AlertService::DialogScope
//************************************************************************************************

AlertService::DialogScope::DialogScope (DialogInformation& information)
: information (information)
{
	AlertService::instance ().beginDialog (information);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AlertService::DialogScope::~DialogScope ()
{
	AlertService::instance ().endDialog (information);
}

//************************************************************************************************
// AlertService::ProgressList
//************************************************************************************************

void CCL_API AlertService::ProgressList::beginProgress ()
{
	VectorForEach (*this, IProgressNotify*, p)
		p->beginProgress ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AlertService::ProgressList::endProgress ()
{
	VectorForEach (*this, IProgressNotify*, p)
		p->endProgress ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AlertService::ProgressList::updateProgress (const State& state)
{
	VectorForEach (*this, IProgressNotify*, p)
		p->updateProgress (state);
	EndFor
}

//************************************************************************************************
// AlertService
//************************************************************************************************

void AlertService::printErrorMessagesDeep (String& text, IErrorContext& context)
{
	static const int kMaxLength = 1500;
	static String kMore ("...");

	// 1. try error events of this context
	int eventCount = context.getEventCount ();
	if(eventCount > 0)
	{
		for(int i = 0; i < eventCount; i++)
		{
			if(text.length () >= kMaxLength) // limit text displayed in alert box
			{
				if(!text.endsWith (kMore))
					text << kMore;
				return;
			}

			text << "\n" << context.getEvent (i).message;
		}
		return;
	}

	// 2. recursively try child contexts
	int i = 0;
	while(IErrorContext* child = context.getChild (i))
	{
		printErrorMessagesDeep (text, *child);
		i++;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AlertService& AlertService::instance ()
{
	static AlertService theAlertService;
	return theAlertService;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (AlertService, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AlertService::AlertService ()
: notifier (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AlertService::~AlertService ()
{
	ASSERT (notifier == nullptr)
	ASSERT (progressList.isEmpty ())
	ASSERT (dialogInformationStack.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlertService::beginDialog (DialogInformation& information)
{
	dialogInformationStack.push (&information);
	signal (Message (kBeginDialog, information.asUnknown ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlertService::endDialog (DialogInformation& information)
{
	signal (Message (kEndDialog, information.asUnknown ()));
	ccl_markGC (information.asUnknown ());

	ASSERT (dialogInformationStack.peek () == &information)
	dialogInformationStack.pop ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef AlertService::getTitle () const
{
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API AlertService::getButtonTitle (int standardResult) const
{
	switch(standardResult)
	{
	case Alert::kYes : return XSTR (Yes);
	case Alert::kNo : return XSTR (No);
	case Alert::kCancel : return XSTR (Cancel);
	case Alert::kOk : return XSTR (Okay);
	case Alert::kRetry : return XSTR (Retry);
	}
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AlertService::setTitle (StringRef title)
{
	this->title = title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AlertService::setNotificationReporter (IReporter* _notifier)
{
	this->notifier = _notifier; // share???
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IProgressNotify* AlertService::getProgressReporter ()
{
	return !progressList.isEmpty () ? &progressList : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AlertService::setProgressReporter (IProgressNotify* progress, tbool state)
{
	if(state)
		progressList.add (progress);
	else
		progressList.remove (progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlertService::showAlert (StringRef text, int type)
{
	AutoPtr<AlertBox> alert (AlertBox::create ());
	alert->initWithType (text, type);
	alert->run ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AlertService::showNotification (StringRef text, int type)
{
	if(notifier == nullptr)
		return false;

	notifier->reportEvent (Alert::Event (text, type));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AlertService::reportEvent (const Alert::Event& e)
{
	showAlert (e.message, e.type);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AlertService::setReportOptions (Severity minSeverity, int eventFormat)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDialogInformation* CCL_API AlertService::getCurrentDialog () const
{
	return dialogInformationStack.peek ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IProgressNotify* CCL_API AlertService::getCurrentProgressDialog () const
{
	return ProgressDialog::getFirstInstance ();
}

//************************************************************************************************
// DialogInformation
//************************************************************************************************

DEFINE_CLASS (DialogInformation, Object)
DEFINE_CLASS_UID (DialogInformation, 0x7d2b332a, 0x107c, 0x4bb9, 0x88, 0x23, 0xcb, 0x5f, 0x31, 0x9c, 0xbf, 0x6)

//////////////////////////////////////////////////////////////////////////////////////////////////

DialogInformation::DialogInformation (DialogType type, StringRef text, StringRef title)
: type (type),
  text (text),
  title (title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DialogInformation::setButtonTitle (int index, StringRef title)
{
	switch(index)
	{
	case 0 : firstButton = title; break;
	case 1 : secondButton = title; break;
	case 2 : thirdButton = title; break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API DialogInformation::getButtonTitle (int index) const
{
	switch(index)
	{
	case 0 : return firstButton;
	case 1 : return secondButton;
	case 2 : return thirdButton;
	}
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DialogInformation::setMenu (IMenu* _menu)
{
	menu = _menu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMenu* CCL_API DialogInformation::getMenu () const
{
	return menu;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DialogInformation::close (int buttonIndex)
{
	ASSERT (0)   // to be implemented by derived class!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DialogInformation::getAttributes (IAttributeList& attributes) const
{
	AttributeAccessor a (attributes);
	a.set ("dialogType", type);
	a.set ("dialogText", text);
	a.set ("dialogTitle", title);
	a.set ("firstButton", firstButton);
	if(!secondButton.isEmpty ())
		a.set ("secondButton", secondButton);
	if(!thirdButton.isEmpty ())
		a.set ("thirdButton", thirdButton);

	if(menu)
	{
		AutoPtr<IAttributeList> menuData = a.newAttributes ();
		menu->saveItems (*menuData);
		a.set ("menuData", menuData, IAttributeList::kShare);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DialogInformation::setAttributes (const IAttributeList& attributes)
{
	AttributeReadAccessor a (attributes);
	type = a.getInt ("dialogType");
	text = a.getString ("dialogText");
	title = a.getString ("dialogTitle");
	firstButton = a.getString ("firstButton");
	secondButton = a.getString ("secondButton");
	thirdButton = a.getString ("thirdButton");

	AutoPtr<IMenu> menu;
	if(UnknownPtr<IAttributeList> menuData = a.getUnknown ("menuData"))
	{
		menu = NEW ExtendedMenu;
		menu->loadItems (*menuData);
	}
	setMenu (menu);
	return true;
}

//************************************************************************************************
// AlertBox
//************************************************************************************************

DEFINE_CLASS (AlertBox, DialogInformation)

//////////////////////////////////////////////////////////////////////////////////////////////////

AlertBox* AlertBox::create ()
{
	// create derived platform specific class via class registry
	Object* object = Kernel::instance ().getClassRegistry ().createObject (ClassID::AlertBox);
	return ccl_cast<AlertBox> (object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AlertBox::AlertBox ()
: DialogInformation (IDialogInformation::kStandardAlert),
  alertType (Alert::kUndefined),
  questionType (Alert::kUndefined),
  firstResult (Alert::kUndefined),
  secondResult (Alert::kUndefined),
  thirdResult (Alert::kUndefined),
  closeResult (Alert::kUndefined),
  platformHandle (nullptr)
{
	setTitle (AlertService::instance ().getTitle ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlertBox::setButtonResult (int index, int result)
{
	switch(index)
	{
	case 0 : firstResult = result; break;
	case 1 : secondResult = result; break;
	case 2 : thirdResult = result; break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AlertBox::getButtonResult (int index) const
{
	switch(index)
	{
	case 0 : return firstResult;
	case 1 : return secondResult;
	case 2 : return thirdResult;
	}
	return Alert::kUndefined;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AlertBox::initWithType (StringRef text, int type)
{
	setText (text);
	setAlertType (type);
	setQuestionType (Alert::kUndefined);

	setFirstButton (XSTR (Okay));
	setFirstResult (Alert::kOk);
	setSecondButton (String::kEmpty);
	setSecondResult (Alert::kUndefined);
	setThirdButton (String::kEmpty);
	setThirdResult (Alert::kUndefined);

	setCloseResult (Alert::kUndefined);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AlertBox::initWithQuestion (StringRef text, int type)
{
	setText (text);
	setAlertType (Alert::kUndefined);
	setQuestionType (type);

	static const Alert::ButtonMapping questionButtons[Alert::kNumQuestionTypes] =
	{
		{Alert::kYes,   Alert::kNo,     Alert::kUndefined},	// kYesNo
		{Alert::kYes,   Alert::kNo,     Alert::kCancel},	// kYesNoCancel
		{Alert::kOk,    Alert::kCancel, Alert::kUndefined},	// kOkCancel
		{Alert::kRetry, Alert::kCancel, Alert::kUndefined}	// kRetryCancel
	};

	const Alert::ButtonMapping& mapping = questionButtons[ccl_bound (type, 0, Alert::kNumQuestionTypes-1)];
	for(int i = 0; i < 3; i++)
	{
		int result = mapping.getResultAtButtonIndex (i);
		setButtonTitle (i, AlertService::instance ().getButtonTitle (result));
		setButtonResult (i, result);
	}

	setCloseResult (Alert::kUndefined);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AlertBox::initWithButtons (StringRef text, StringRef firstButton, StringRef secondButton, StringRef thirdButton)
{
	setText (text);
	setAlertType (Alert::kUndefined);
	setQuestionType (Alert::kQuestionTypeCustom);

	const Alert::ButtonMapping mapping = {Alert::kFirstButton, Alert::kFirstButton+1, thirdButton.isEmpty () ? Alert::kUndefined : Alert::kFirstButton+2};
	for(int i = 0; i < 3; i++)
	{
		int result = mapping.getResultAtButtonIndex (i);
		switch(result)
		{
		case Alert::kFirstButton   : setButtonTitle (i, firstButton); break;
		case Alert::kFirstButton+1 : setButtonTitle (i, secondButton); break;
		case Alert::kFirstButton+2 : setButtonTitle (i, thirdButton); break;
		}
		setButtonResult (i, result);
	}

	setCloseResult (Alert::kUndefined);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AlertBox::initWithContext (StringRef _text, IErrorContext* context, int question)
{
	String text (_text);
	if(context)
	{
		String details;
		AlertService::printErrorMessagesDeep (details, *context);

		if(!details.isEmpty ())
		{
			if(!text.isEmpty ())
				text << "\n";
			text << details;
		}
	}

	if(question == Alert::kUndefined)
		initWithType (text, Alert::kError);
	else
		initWithQuestion (text, question);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AlertBox::close (int buttonIndex)
{
	setCloseResult (getButtonResult (buttonIndex));

	closePlatform ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AlertBox::run ()
{
#if DEBUG
	if(DragSession::getActiveSession ()) \
		Debugger::println ("WARNING: Drag'n'Drop still active when opening Alert. Should be deferred!");
#endif

	AlertService::DialogScope scope (*this);

	Promise promise (runAsyncPlatform ());
	while(promise->getState () == AsyncOperation::kStarted)
		GUI.flushUpdates ();

	if(closeResult != Alert::kUndefined) // override result if closed programmatically
		return closeResult;

	return promise->getResult ().asInt ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API AlertBox::runAsync ()
{
#if DEBUG
	if(DragSession::getActiveSession ()) \
		Debugger::println ("WARNING: Drag'n'Drop still active when opening Alert. Should be deferred!");
#endif

	AlertService::instance ().beginDialog (*this);

	retain ();

	Promise platformPromise (runAsyncPlatform ());
	return return_shared<IAsyncOperation> (platformPromise.then (this, &AlertBox::onAlertCompleted));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlertBox::onAlertCompleted (IAsyncOperation& operation)
{
	AlertService::instance ().endDialog (*this);

	// override result if closed programmatically
	if(closeResult != Alert::kUndefined)
		if(AsyncOperation* op = unknown_cast<AsyncOperation> (&operation))
			op->setResult (closeResult);

	release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AlertBox::isUsingCustomButtonResults () const
{
	return questionType == Alert::kQuestionTypeCustom;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AlertBox::closePlatform ()
{
	CCL_NOT_IMPL ("AlertBox::closePlatform")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* AlertBox::runAsyncPlatform ()
{
	CCL_NOT_IMPL ("AlertBox::runAsyncPlatform")
	return nullptr;
}
