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
// Filename    : ccl/base/signalsource.h
// Description : Signal Source
//
//************************************************************************************************

#ifndef _ccl_signalsource_h
#define _ccl_signalsource_h

#include "ccl/public/base/iobserver.h"
#include "ccl/public/base/iactivatable.h"
#include "ccl/public/base/cclmacros.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

//************************************************************************************************
// SignalSource
/** Emits process-wide signal via named atom. */
//************************************************************************************************

class SignalSource
{
public:
	SignalSource (StringID name);
	~SignalSource ();

	ISubject* getAtom ();

	void signal (MessageRef msg);
	void deferSignal (IMessage* msg);
	void cancelSignals ();

	static void addObserver (StringID name, IObserver* observer);
	static void removeObserver (StringID name, IObserver* observer);

protected:
	ISubject* atom;
	MutableCString name;
};

//************************************************************************************************
// SignalSink
/** Handle connection between named atom and observer instance. */
//************************************************************************************************

class SignalSink
{
public:
	SignalSink (StringID name);
	~SignalSink ();

	void setName (StringID name);
	void setObserver (IObserver* observer);

	void enable (bool state);
	bool isEnabled () const;

protected:
	MutableCString name;
	IObserver* observer;
	bool enabled;
};

//************************************************************************************************
// AutoSignalSink
/** Sink is automatically enabled/disabled, based on state of its activator. */
//************************************************************************************************

class AutoSignalSink: public SignalSink,
					  public IObserver
{
public:
	AutoSignalSink (StringID name);
	~AutoSignalSink ();

	void setActivator (IActivatable* activator);

protected:
	IActivatable* activator;

	// IObserver
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	IMPLEMENT_DUMMY_UNKNOWN (IObserver)
};

//************************************************************************************************
// ActivationDelegate
/** Sync activation state between source and target IActivatable. */
//************************************************************************************************

class ActivationDelegate: public IObserver
{
public:
	ActivationDelegate ();
	~ActivationDelegate ();

	void setTarget (IActivatable* target);
	void setSource (IActivatable* source);

	bool isActive () const;

protected:
	IActivatable* target;
	IActivatable* source;

	// IObserver
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	IMPLEMENT_DUMMY_UNKNOWN (IObserver)
};

} // namespace CCL

#endif // _ccl_signalsource_h
