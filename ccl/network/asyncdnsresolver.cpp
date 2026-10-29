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
// Filename    : ccl/network/asyncdnsresolver.cpp
// Description : Bounded/cancelable DNS resolution
//
//************************************************************************************************

#include "ccl/network/asyncdnsresolver.h"
#include "ccl/network/netsocket.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/text/cstring.h"

#include "ares.h"

namespace CCL {
namespace Net {

//************************************************************************************************
// AsyncDNSResolver::Session
/** One c-ares channel + in-flight ares_getaddrinfo() query for a single hostname. */
//************************************************************************************************

class AsyncDNSResolver::Session: public Unknown
{
public:
	Session (StringRef hostname);

	PROPERTY_STRING (hostname, HostName)
	PROPERTY_OBJECT (IPAddress, resolvedAddress, ResolvedAddress)

	bool start ();
	bool wait (int slice);
	void cancel ();
	void destroy ();
	bool succeeded () const;

private:
	bool success;
	Threading::Signal done;
	Threading::CriticalSection channelLock;
	ares_channel channel;

	static void addrinfoCallback (void* arg, int status, int timeouts, ares_addrinfo* result);
	static bool fillAddress (IPAddress& address, const struct sockaddr* sa, ares_socklen_t saLen);
};

} // namespace Net
} // namespace CCL

using namespace CCL;
using namespace Net;

//************************************************************************************************
// AsyncDNSResolver
//************************************************************************************************

AsyncDNSResolver::~AsyncDNSResolver ()
{
	ASSERT (inFlight.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncDNSResolver::initialize ()
{
	if(ares_library_init (ARES_LIB_INIT_ALL) != ARES_SUCCESS)
		CCL_WARN ("AsyncDNSResolver initialization failed, DNS resolution will not work\n", 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncDNSResolver::terminate ()
{
	ares_library_cleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AsyncDNSResolver::resolve (SocketAddress& address, StringRef hostname, int timeout, IProgressNotify* progress)
{
	String key (hostname);
	int64 now = System::GetSystemTicks ();

	AutoPtr<Session> session;
	bool owner = false;
	{
		Threading::ScopedLock scopedLock (lock);

		for(int i = 0; i < negativeCache.count (); i++)
		{
			if(negativeCache[i].hostname == key)
			{
				bool stillNegative = negativeCache[i].expiry > now;
				negativeCache.removeAt (i);
				if(stillNegative)
					return kResultFailed; // still within the negative-cache TTL. Fail fast, spawn nothing
				break;
			}
		}

		Session* existing = nullptr;
		for(int i = 0; i < inFlight.count (); i++)
			if(inFlight[i]->getHostName () == key)
			{
				existing = inFlight[i];
				break;
			}

		if(existing)
			session.share (existing); // coalesce onto the already-running lookup
		else
		{
			session = NEW Session (key); // AutoPtr adopts the construction reference

			owner = true;
			session->retain (); // extra reference held by the inFlight entry itself
			inFlight.add (session.as_plain ());

			if(!session->start ())
			{
				inFlight.remove (session.as_plain ());
				session->release (); // undo the list's reference; insertion is being reverted
				return kResultFailed;
			}
		}
	}

	static const int kPollSlice = 200; // bounds cancellation latency without busy-waiting
	int waited = 0;
	bool done = false;
	while(true)
	{
		int slice = (timeout - waited) < kPollSlice ? (timeout - waited) : kPollSlice;
		done = session->wait (slice);
		if(done)
			break;
		if(progress && progress->isCanceled ())
			break;

		waited += slice;
		if(waited >= timeout)
			break;
	}

	if(!done && owner)
	{
		// Only the owner (the caller that actually created this lookup) may cancel it.
		// A coalesced caller giving up must not cut the query short for everyone else still waiting.
		// ares_cancel synchronously invokes the still-pending callback with ARES_ECANCELLED,
		// which signals 'done' immediately. The query is genuinely interrupted, never abandoned.
		session->cancel ();
		done = true;
	}

	tresult result = kResultFailed;
	if(done)
	{
		if(session->succeeded ())
		{
			static_cast<IPAddress&> (address) = session->getResolvedAddress ();
			result = kResultOk;
		}
		else
			result = kResultFailed;
	}
	else
		result = kResultAborted; // this (non-owning) caller gave up; the query keeps running for others

	if(owner)
	{
		session->destroy ();

		Threading::ScopedLock scopedLock (lock);

		inFlight.remove (session.as_plain ()); // no-op if a differently-owned entry already replaced it

		if(!session->succeeded ())
		{
			NegativeCacheEntry entry;
			entry.hostname = key;
			entry.expiry = now + kNegativeCacheTimeout;
			negativeCache.add (entry);
		}

		session->release ();
	}

	return result;
}

//************************************************************************************************
// AsyncDNSResolver::Session
//************************************************************************************************

void AsyncDNSResolver::Session::addrinfoCallback (void* arg, int status, int timeouts, ares_addrinfo* result)
{
	Session* session = static_cast<Session*> (arg);

	if(status == ARES_SUCCESS && result)
	{
		bool converted = false;
		for(int pass = 0; pass < 2 && !converted; pass++)
		{
			for(ares_addrinfo_node* node = result->nodes; node != nullptr; node = node->ai_next)
			{
				if(pass == 0 && node->ai_family != AF_INET) // prefer an IPv4 result first, matching
					continue;                                // getAddressByHost()'s existing behavior

				if(fillAddress (session->resolvedAddress, node->ai_addr, node->ai_addrlen))
				{
					converted = true;
					break;
				}
			}
		}
		session->success = converted;
		ares_freeaddrinfo (result);
	}
	else
		session->success = false;

	session->done.signal ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AsyncDNSResolver::Session::fillAddress (IPAddress& address, const struct sockaddr* sa, ares_socklen_t saLen)
{
	if(sa->sa_family == AF_INET && size_t(saLen) >= sizeof(sockaddr_in))
	{
		const sockaddr_in* sin = reinterpret_cast<const sockaddr_in*> (sa);
		address.family = kInternet;
		address.port = ntohs (sin->sin_port);
		::memcpy (address.ip.address, &sin->sin_addr, 4);
		return true;
	}
	if(sa->sa_family == AF_INET6 && size_t(saLen) >= sizeof(sockaddr_in6))
	{
		const sockaddr_in6* sin6 = reinterpret_cast<const sockaddr_in6*> (sa);
		address.family = kInternetV6;
		address.port = ntohs (sin6->sin6_port);
		::memcpy (address.ipv6.address, &sin6->sin6_addr, 16);
		address.ipv6.flowinfo = ntohl (sin6->sin6_flowinfo);
		address.ipv6.scopeid = sin6->sin6_scope_id;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AsyncDNSResolver::Session::Session (StringRef hostname)
: hostname (hostname),
  channel (nullptr),
  success (false),
  done (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AsyncDNSResolver::Session::start ()
{
	ares_options options = {};
	options.evsys = ARES_EVSYS_DEFAULT;

	// ARES_OPT_EVENT_THREAD makes c-ares run its own internal thread pumping the resolver's
	// event loop. No worker thread of our own is needed, and ares_cancel() genuinely interrupts 
	// an in-flight query instead of merely bounding our own wait for it.
	if(ares_init_options (&channel, &options, ARES_OPT_EVENT_THREAD) != ARES_SUCCESS)
	{
		channel = nullptr;
		return false;
	}

	MutableCString hostNameString (hostname, kNetworkTextEncoding);

	ares_addrinfo_hints hints = {};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	ares_getaddrinfo (channel, hostNameString, nullptr, &hints, &Session::addrinfoCallback, this);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AsyncDNSResolver::Session::wait (int slice)
{
	return done.wait (uint32(slice));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncDNSResolver::Session::cancel ()
{
	Threading::ScopedLock channelScopedLock (channelLock);
	if(channel)
		ares_cancel (channel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AsyncDNSResolver::Session::destroy ()
{
	if(channel)
	{
		ares_destroy (channel);
		channel = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AsyncDNSResolver::Session::succeeded () const
{
	return success;
}
