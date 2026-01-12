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
// Filename    : ccl/app/components/asynchandler.cpp
// Description : Asynchronous Operation Handler
//
//************************************************************************************************

#include "ccl/app/components/asynchandler.h"

#include "ccl/base/message.h"

#include "ccl/public/gui/framework/iprogressdialog.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// AsyncCallHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AsyncCallHandler, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

AsyncCallHandler::AsyncCallHandler (StringRef name)
: Component (name),
  currentProgressDialog (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AsyncCallHandler::~AsyncCallHandler ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* AsyncCallHandler::performAsync (IAsyncCall* call, StringRef description, StringRef title)
{
#if CCL_PLATFORM_DESKTOP
	// run as a modal operation on desktop platforms for now
	Variant result = performModal (call, description, title);
	return AsyncOperation::createCompleted (result);
#else
	IProgressNotify* progress = ccl_new<IProgressNotify> (CCL::ClassID::ProgressDialog);
	ASSERT (progress != 0)
	if(!title.isEmpty ())
		progress->setTitle (title);
	progress->setProgressText (description);
	progress->setCancelEnabled (true);
	progress->beginProgress ();
	progress->updateAnimated ();

	ISubject::addObserver (progress, this);

	pendingOperation = call->call ();
	Promise p = Promise (pendingOperation).then ([progress, this] (IAsyncOperation& operation)
	{
		ISubject::removeObserver (progress, this);

		progress->endProgress ();
		progress->release ();

		pendingOperation.release ();
	});
	return return_shared<IAsyncOperation> (p);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Variant AsyncCallHandler::performModal (IAsyncCall* call, StringRef description, StringRef title)
{
	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (CCL::ClassID::ModalProgressDialog);
	ASSERT (progress != nullptr)
	if(!title.isEmpty ())
		progress->setTitle (title);
	progress->setProgressText (description);
	progress->setCancelEnabled (true);
	ISubject::addObserver (progress, this);

	(NEW Message ("start", call))->post (this, 1);

	ScopedVar<IModalProgressDialog*> dialogScope (currentProgressDialog, UnknownPtr<IModalProgressDialog> (progress));
	currentProgressDialog->run ();
	
	ISubject::removeObserver (progress, this);

	Promise p = pendingOperation.detach ();
	Variant result = p->getResult ();
	result.share ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AsyncCallHandler::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "start")
	{
		UnknownPtr<IAsyncCall> call (msg[0].asUnknown ());
		ASSERT (call.isValid ())
		Promise p = call->call ();
		pendingOperation = p;
		p.then<AsyncCallHandler> (this, &AsyncCallHandler::onCallCompleted);
	}
	else if(msg == IProgressDialog::kCancelButtonHit)
	{
		if(pendingOperation)
			pendingOperation->cancel ();
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncCallHandler::onCallCompleted (IAsyncOperation& operation)
{
	if(currentProgressDialog)
		currentProgressDialog->close ();
}
