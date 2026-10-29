//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/network/web/internal/wsclient.h
// Description : Internal WebSocket Client
//
//************************************************************************************************

#ifndef _ccl_internal_wsclient_h
#define _ccl_internal_wsclient_h

#include "ccl/network/web/webabstraction.h"

#include "ccl/base/collections/objectlist.h"

#include "ccl/public/system/threadsync.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// InternalWSClient
//************************************************************************************************

class InternalWSClient: public WebSocketClient
{
public:
	InternalWSClient (IObserver* owner);
	~InternalWSClient ();

	// WebSocketClient
	tresult connect (UrlRef url, VariantRef protocols, IProgressNotify* progress) override;
	tresult process () override;
	void queueMessage (WebSocketMessage* message) override;
	uint32 getBufferedAmount () const override;
	void flushAll () override;
	void disconnect () override;

protected:
	AutoPtr<IStream> stream;
	Threading::CriticalSection sendQueueLock;
	ObjectList sendQueue;
	int bufferedAmount;
	AutoPtr<WebSocketMessage> pendingMessage;

	WebSocketMessage* retrieveNextMessage ();
};

} // namespace Web
} // namespace CCL

#endif // _ccl_internal_wsclient_h
