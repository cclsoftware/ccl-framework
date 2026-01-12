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
// Filename    : ccl/public/base/iobserver.cpp
// Description : Observer interface
//
//************************************************************************************************

#include "ccl/public/base/iobserver.h"
#include "ccl/public/text/cstring.h"

using namespace CCL;

//************************************************************************************************
// IMessage
//************************************************************************************************

bool IMessage::operator == (CStringPtr id) const
{
	return getID ().compare (id) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IMessage::operator != (CStringPtr id) const
{
	return getID ().compare (id) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IMessage::operator == (StringID id) const
{
	return getID () == id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IMessage::operator != (StringID id) const
{
	return getID () != id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Variant& IMessage::operator [] (int index) const
{
	return getArg (index);
}

//************************************************************************************************
// ISubject
//************************************************************************************************

void ISubject::addObserver (IUnknown* unknown, IObserver* observer)
{
	UnknownPtr<ISubject> subject (unknown);
	if(subject)
		subject->addObserver (observer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ISubject::removeObserver (IUnknown* unknown, IObserver* observer)
{
	UnknownPtr<ISubject> subject (unknown);
	if(subject)
		subject->removeObserver (observer);
}

//************************************************************************************************
// IObserver
//************************************************************************************************

void IObserver::notify (IUnknown* unknown, ISubject* subject, MessageRef msg)
{
	UnknownPtr<IObserver> observer (unknown);
	if(observer)
		observer->notify (subject, msg);
}
