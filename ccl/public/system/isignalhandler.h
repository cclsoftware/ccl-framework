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
// Filename    : ccl/public/system/isignalhandler.h
// Description : Signal Handler Interface
//
//************************************************************************************************

#ifndef _ccl_isignalhandler_h
#define _ccl_isignalhandler_h

#include "ccl/public/base/iobserver.h"

namespace CCL {

//************************************************************************************************
// ISignalHandler
/**	Handler for signal protocol between ISubject and IObserver. 

	Threading Policy: 
	Everything except queueSignal() and postMessage() must be called from main thread only,
	otherwise the methods will fail with kResultWrongThread! 
	
	\ingroup ccl_system */
//************************************************************************************************

interface ISignalHandler: IUnknown
{
	/** Establish connection between subject and observer. */
	virtual tresult CCL_API advise (ISubject* subject, IObserver* observer) = 0;

	/** Break connection between subject and observer. */
	virtual tresult CCL_API unadvise (ISubject* subject, IObserver* observer) = 0;

	/** This will call IObserver::notify() on all dependent observers of given subject. */
	virtual tresult CCL_API performSignal (ISubject* subject, MessageRef msg) = 0;

	/**	Queue signal message of given subject. It is performed next time flush() is called. 
		If equal messages are queued for a subject, the signal is performed only once. */
	virtual tresult CCL_API queueSignal (ISubject* subject, IMessage* msg) = 0;

	/** Optimized version of queueSignal() for kChanged message without arguments. */
	virtual tresult CCL_API queueChanged (ISubject* subject) = 0;

	/** Discard any queued signal messages of given subject. */
	virtual tresult CCL_API cancelSignals (ISubject* subject) = 0;

	/** Post message directly to given observer with delay given in milliseconds. Delayed messages are delivered only once. */
	virtual tresult CCL_API postMessage (IObserver* observer, IMessage* msg, int delay = 0) = 0;

	/** Similar to postMessage(), but calling thread blocks until message is delivered or canceled. */
	virtual tresult CCL_API postMessageBlocking (IObserver* observer, IMessage* msg) = 0;

	/** Discard any messages posted to given observer which have not been delivered yet. */
	virtual tresult CCL_API cancelMessages (IObserver* observer) = 0;
	
	/** Flush queued signals. */
	virtual tresult CCL_API flush (IObserver* observer = nullptr) = 0;

	/** Return true, if it still has observers. */
	virtual tbool CCL_API hasObservers (ISubject* subject) = 0;

	/** Return true, if there are undelivered messages for the observer. */
	virtual tbool CCL_API messagesPending (IObserver* observer) = 0;

	DECLARE_IID (ISignalHandler)
};

DEFINE_IID (ISignalHandler, 0x677afe67, 0x5387, 0x49e4, 0xae, 0xd5, 0x56, 0xfd, 0xf9, 0x35, 0x47, 0x1)

} // namespace CCL

#endif // _ccl_isignalhandler_h
