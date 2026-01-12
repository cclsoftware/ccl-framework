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
// Filename    : core/extras/web/corewebserverloop.h
// Description : Embedded HTTP Server Run Loop
//
//************************************************************************************************

#ifndef _corewebserverloop_h
#define _corewebserverloop_h

#include "core/extras/web/corewebserver.h"

#include "core/public/coresocketaddress.h"

namespace Core {

namespace Sockets {
class Socket; }

namespace Portable {
namespace HTTP {

//************************************************************************************************
// HTTP::ServerRunLoop
//************************************************************************************************

class ServerRunLoop
{
public:
	ServerRunLoop ();
	~ServerRunLoop ();

	bool startup (IRequestHandler* requestHandler, Sockets::IPAddress& address);
	void quit ();

protected:
	IRequestHandler* requestHandler;
	Sockets::Socket* socket;
	class Thread;
	Thread* thread;
	bool quitRequested;

	void run ();
};

} // namespace HTTP
} // namespace Portable
} // namespace Core

#endif // _corewebserverloop_h
