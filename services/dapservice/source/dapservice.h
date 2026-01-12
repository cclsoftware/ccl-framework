//************************************************************************************************
//
// DAP Service
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
// Filename    : dapservice.h
// Description : DAP Service
//
//************************************************************************************************

#ifndef _dapservice_h
#define _dapservice_h

#include "ccl/base/object.h"

#include "ccl/public/plugins/serviceplugin.h"
#include "ccl/public/plugins/idebugservice.h"
#include "ccl/public/system/ithreading.h"
#include "ccl/public/network/isocket.h"

namespace CCL {

//************************************************************************************************
// DAPService
//************************************************************************************************

class DAPService: public ServicePlugin,
                  public IDebugService,
			      public IObserver
{
public:
	DECLARE_STRINGID_MEMBER (kProtocolIdentifier)

	DAPService ();
	~DAPService ();

	// IDebugService
	tbool CCL_API startup (StringRef arg, IDebuggableManager* manager) override;
	tbool CCL_API shutdown () override;
	tbool CCL_API sendMessage (const IDebugMessage& message) override;
	IDebugMessage* CCL_API createMessage (StringRef rawData) override;

	CLASS_INTERFACE2 (IDebugService, IObserver, ServicePlugin)

protected:
	IDebuggableManager* debuggableManager;
	Net::ISocket* receiveSocket;
	Net::ISocket* sendSocket;
	Net::IPAddress address;
	CCL::Threading::IThread* receiveThread;
	int sequenceNumber;
	bool connected;

	bool sendMessage (StringRef message) const;
	void disconnect ();
	bool handleMessage (StringRef message);
	bool handleJSONMessage (StringRef message);

	static int CCL_API receiveThreadFunction (void* arg);

	// IObserver
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

} // namespace CCL

#endif // _dapservice_h
