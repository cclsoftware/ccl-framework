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
// Filename    : ccl/base/signalsource.cpp
// Description : Signal Source
//
//************************************************************************************************

#include "ccl/base/signalsource.h"

#include "ccl/public/system/iatomtable.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// SignalSource
//************************************************************************************************

void SignalSource::addObserver (StringID name, IObserver* observer)
{
	ASSERT (name.isEmpty () == false && observer != nullptr)
	AutoPtr<IAtom> atom = System::GetAtomTable ().createAtom (name);
	UnknownPtr<ISubject> subject (atom);
	ASSERT (subject != nullptr)
	subject->addObserver (observer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalSource::removeObserver (StringID name, IObserver* observer)
{
	ASSERT (name.isEmpty () == false && observer != nullptr)
	AutoPtr<IAtom> atom = System::GetAtomTable ().createAtom (name);
	UnknownPtr<ISubject> subject (atom);
	ASSERT (subject != nullptr)
	subject->removeObserver (observer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SignalSource::SignalSource (StringID name)
: name (name),
  atom (nullptr)
{
	ASSERT (name.isEmpty () == false)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SignalSource::~SignalSource ()
{
	if(atom)
	{
		//System::GetSignalHandler ().cancelSignals (atom); must not be done here, otherwise we can't construct it inplace and defer signals
		atom->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISubject* SignalSource::getAtom ()
{
	if(atom == nullptr)
	{
		IAtom* a = System::GetAtomTable ().createAtom (name);
		if(a)
		{
			a->queryInterface (ccl_iid<ISubject> (), (void**)&atom);
			ASSERT (atom != nullptr)
			a->release ();
		}
	}
	return atom;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalSource::signal (MessageRef msg)
{
	if(getAtom ())
		getAtom ()->signal (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalSource::deferSignal (IMessage* msg)
{
	if(getAtom ())
		getAtom ()->deferSignal (msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalSource::cancelSignals ()
{
	if(atom)
		System::GetSignalHandler ().cancelSignals (atom);
}

//************************************************************************************************
// SignalSink
//************************************************************************************************

SignalSink::SignalSink (StringID name)
: name (name),
  observer (nullptr),
  enabled (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SignalSink::~SignalSink ()
{
	ASSERT (isEnabled () == false)
	enable (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalSink::setName (StringID name)
{
	bool wasEnabled = isEnabled ();
	if(wasEnabled)
		enable (false);

	this->name = name;

	if(wasEnabled)
		enable (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalSink::setObserver (IObserver* _observer)
{
	ASSERT (isEnabled () == false)
	observer = _observer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SignalSink::isEnabled () const
{
	return enabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SignalSink::enable (bool state)
{
	if(state != enabled)
	{
		ASSERT (observer != nullptr)

		if(enabled)
			SignalSource::removeObserver (name, observer);

		enabled = state;
		
		if(enabled)
			SignalSource::addObserver (name, observer);
	}
}

//************************************************************************************************
// AutoSignalSink
//************************************************************************************************

AutoSignalSink::AutoSignalSink (StringID name)
: SignalSink (name),
  activator (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AutoSignalSink::~AutoSignalSink ()
{
	setActivator (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AutoSignalSink::setActivator (IActivatable* _activator)
{
	if(activator)
		ISubject::removeObserver (activator, this);
	activator = _activator;
	if(activator)
		ISubject::addObserver (activator, this);

	enable (activator && activator->isActive ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AutoSignalSink::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IActivatable::kActivate || msg == IActivatable::kDeactivate)
	{
		if(activator && isEqualUnknown (subject, activator))
			enable (msg == IActivatable::kActivate);
	}
}

//************************************************************************************************
// ActivationDelegate
//************************************************************************************************

ActivationDelegate::ActivationDelegate ()
: target (nullptr),
  source (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ActivationDelegate::~ActivationDelegate ()
{
	setSource (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActivationDelegate::setTarget (IActivatable* _target)
{
	target = _target;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ActivationDelegate::setSource (IActivatable* _source)
{
	if(source)
		ISubject::removeObserver (source, this);
	source = _source;
	if(source)
		ISubject::addObserver (source, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ActivationDelegate::isActive () const
{
	return source && source->isActive ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ActivationDelegate::notify (ISubject* subject, MessageRef msg)
{
	if(target == nullptr || source == nullptr)
		return;

	if(!isEqualUnknown (subject, source))
		return;

	if(msg == IActivatable::kActivate)
		target->activate ();
	else if(msg == IActivatable::kDeactivate)
		target->deactivate ();
}
