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
// Filename    : ccl/network/asyncdnsresolver.h
// Description : Bounded/cancelable DNS resolution
//
//************************************************************************************************

#ifndef _ccl_asyncdnsresolver_h
#define _ccl_asyncdnsresolver_h

#include "ccl/base/singleton.h"
#include "ccl/public/network/isocket.h"
#include "ccl/public/system/threadsync.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {
namespace Net {

//************************************************************************************************
// AsyncDNSResolver
/** Bounded, cancelable DNS resolution backed by c-ares, used in place of a plain blocking
	getaddrinfo() call. 
	Concurrent lookups for the same hostname are coalesced onto a single in-flight c-ares channel, 
	and a short	negative-result cache keeps a permanently unreachable host 
	(e.g. one blocked by network censorship) from spawning a fresh lookup on every retry. */
//************************************************************************************************

class AsyncDNSResolver: public StaticSingleton<AsyncDNSResolver>
{
public:
	~AsyncDNSResolver ();

	void initialize ();
	void terminate ();

	tresult resolve (SocketAddress& address, StringRef hostname, int timeout, IProgressNotify* progress = nullptr);

protected:
	static const int kNegativeCacheTimeout = 30000;
	
	struct NegativeCacheEntry
	{
		String hostname;
		int64 expiry; // System::GetSystemTicks() value at which this entry stops applying
	};

	class Session;

	Threading::CriticalSection lock;
	Vector<Session*> inFlight;
	Vector<NegativeCacheEntry> negativeCache;
};

} // namespace Net
} // namespace CCL

#endif // _ccl_asyncdnsresolver_h
