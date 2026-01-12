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
// Filename    : ccl/public/system/cclerror.cpp
// Description : Structured Error Handling Helpers
//
//************************************************************************************************

#include "ccl/public/system/cclerror.h"

#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Helper functions
////////////////////////////////////////////////////////////////////////////////////////////////////

void CCL::ccl_raise (AlertEventRef e)
{
	System::GetErrorHandler ().reportEvent (e);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CCL::ccl_raise (StringRef message, tresult errorCode)
{
	System::GetErrorHandler ().reportEvent (Alert::Event (message, errorCode, Alert::kError));
}

//************************************************************************************************
// ErrorContextGuard
//************************************************************************************************

tresult ErrorContextGuard::getResultCode (IErrorContext* context, bool deep)
{
	if(context)
	{
		if(context->getEventCount () > 0)
			return context->getEvent (0).resultCode;
		
		if(deep) for(int i = 0, count = context->getChildCount (); i < count; i++)
			if(IErrorContext* child = context->getChild (i))
			{
				tresult resultCode = getResultCode (child, true);
				if(resultCode != kResultOk)
					return resultCode;
			}
	}
	return kResultOk;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool ErrorContextGuard::hasErrors (IErrorContext* context, bool deep)
{
	if(context)
	{
		if(context->getEventCount () > 0)
			return true;
		
		if(deep) for(int i = 0, count = context->getChildCount (); i < count; i++)
			if(IErrorContext* child = context->getChild (i))
				if(hasErrors (child, true))
					return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ErrorContextGuard::ErrorContextGuard ()
: context (nullptr)
{
	beginContext ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ErrorContextGuard::~ErrorContextGuard ()
{
	endContext ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ErrorContextGuard::beginContext ()
{
	if(context == nullptr)
	{
		System::GetErrorHandler ().beginContext ();
		context = System::GetErrorHandler ().peekContext ();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ErrorContextGuard::endContext ()
{
	if(context)
	{
		System::GetErrorHandler ().endContext ();
		context = nullptr;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ErrorContextGuard::reset ()
{
	endContext ();
	beginContext ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ErrorContextGuard::removeAll ()
{
	if(context)
		context->removeAll ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IErrorContext* ErrorContextGuard::operator -> ()
{
	return context; 
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ErrorContextGuard::operator IErrorContext* ()
{
	return context;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool ErrorContextGuard::hasErrors (bool deep) const
{
	return hasErrors (context, deep);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tresult ErrorContextGuard::getResultCode (bool deep) const
{
	return getResultCode (context, deep);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ErrorContextGuard::pushToParent ()
{
	System::GetErrorHandler ().pushToParent (context);
}
