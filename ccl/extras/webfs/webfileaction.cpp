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
// Filename    : ccl/extras/webfs/webfileaction.cpp
// Description : Web File Action
//
//************************************************************************************************

#include "ccl/extras/webfs/webfileaction.h"

#include "ccl/base/message.h"

#include "ccl/public/network/web/iwebfileservice.h"
#include "ccl/public/netservices.h"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// FileAction
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (FileAction, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileAction::FileAction ()
: state (kNone)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileAction::~FileAction ()
{
	if(state == kPending)
		cancel ();

	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileAction::State FileAction::getState () const
{
	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileAction::isCompleted () const
{
	return state == kCompleted;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileAction::setState (State newState)
{
	if(state != newState)
	{
		state = newState;
		signal (Message (kChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileAction::cancel ()
{
	if(state == kPending)
	{
		System::GetWebFileService ().cancelOperation (this);
		setState (kFailed);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileAction::reset ()
{
	if(state == kPending)
		System::GetWebFileService ().cancelOperation (this);
	state = kNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileAction::restart ()
{
	reset ();
	start ();
}

//************************************************************************************************
// GetDirectoryAction
//************************************************************************************************

DEFINE_CLASS_HIDDEN (GetDirectoryAction, FileAction)

//////////////////////////////////////////////////////////////////////////////////////////////////

void GetDirectoryAction::start ()
{
	if(state == kNone)
	{
		ASSERT (!webfsUrl.isEmpty ())
		System::GetWebFileService ().requestDirectory (this, webfsUrl);
		setState (kPending);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API GetDirectoryAction::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Meta::kGetDirectoryCompleted)
	{
		tresult result = msg[0].asResult ();
		setState (result == kResultOk ? kCompleted : kFailed);
	}
	else
		SuperClass::notify (subject, msg);
}

//************************************************************************************************
// FileTaskAction
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FileTaskAction, FileAction)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileTaskAction::FileTaskAction ()
: tag (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTaskAction::start ()
{
	if(state == kNone)
	{
		ASSERT (task)
		if(!task)
		{
			setState (kFailed);
			return;
		}

		ASSERT (!webfsUrl.isEmpty ())
		System::GetWebFileService ().scheduleTask (this, webfsUrl, task);
		setState (kPending);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileTaskAction::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Meta::kFileTaskCompleted)
	{
		tresult result = msg[0].asResult ();
		setState (result == kResultOk ? kCompleted : kFailed);
	}
	else
		SuperClass::notify (subject, msg);
}

//************************************************************************************************
// FileTask
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (FileTask, Object)
