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
// Filename    : ccl/network/network.cpp
// Description : Network class
//
//************************************************************************************************

#include "ccl/network/network.h"

#include "ccl/network/netsslsocket.h"
#include "ccl/network/netstream.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/collections/iunknownlist.h"

using namespace CCL;
using namespace Net;

//************************************************************************************************
// Network
//************************************************************************************************

bool Network::startup ()
{
	bool result = Core::Sockets::Network::startup ();

	#if (0 && DEBUG)
	Core::CString32 macAddress;
	Core::Sockets::Network::getLocalMACAddress (macAddress);
	IPAddress ip;
	Core::Sockets::Network::getLocalIPAddress (ip);
	Core::Vector<IPAddress> ipList;
	Core::Sockets::Network::getLocalIPAddressList (ipList);
	#endif

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Network::shutdown ()
{
	Core::Sockets::Network::shutdown ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Network::getLocalHostname (String& hostname)
{
	hostname.empty ();

	Core::CString256 cname;
	if(!Core::Sockets::Network::getLocalHostname (cname))
		return kResultFailed;

	hostname.appendCString (kNetworkTextEncoding, cname);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Network::getLocalIPAddress (IPAddress& address)
{
	if(!Core::Sockets::Network::getLocalIPAddress (address))
		return kResultFailed;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Network::getAddressByHost (SocketAddress& address, StringRef hostname)
{
	Core::CString256 cname;
	hostname.toCString (kNetworkTextEncoding, cname.getBuffer (), cname.getSize ());

	if(!Core::Sockets::Network::getAddressByHost (address, cname))
		return kResultFailed;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Network::getHostByAddress (String& hostname, const SocketAddress& address)
{
	hostname.empty ();

	Core::CString256 cname;
	if(!Core::Sockets::Network::getHostByAddress (cname, address))
		return kResultFailed;

	hostname.appendCString (kNetworkTextEncoding, cname);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Network::getAddressString (String& string, const SocketAddress& address)
{
	string.empty ();

	Core::CString256 dst;
	if(!Core::Sockets::Network::getAddressString (dst, address))
		return kResultFailed;

	string.appendASCII (dst);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Network::getAddressFromString (SocketAddress& address, StringRef string)
{
	Core::CString256 src;
	string.toCString (kNetworkTextEncoding, src.getBuffer (), src.getSize ());

	if(!Core::Sockets::Network::getAddressFromString (address, src))
		return kResultFailed;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISocket* CCL_API Network::createSocket (AddressFamily addressFamily, SocketType type, ProtocolType protocol)
{
	return Socket::createSocket (addressFamily, type, protocol);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IStream* CCL_API Network::openStream (const SocketAddress& address, ProtocolType protocol)
{
	AutoPtr<Socket> s = Socket::createSocket (address.family, kStream, protocol);
	if(s)
	{
		tresult result = s->connect (address);
		if(result == kResultOk)
			return NEW NetworkStream (s);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::IStream* CCL_API Network::openSSLStream (const IPAddress& address, StringRef peerName, CCL::IProgressNotify* progress)
{
	AutoPtr<SSLSocket> s = SSLSocket::createSocket (address.family);
	if(s)
	{
		s->setPeerName (peerName);
		tresult result = s->connect (address, progress);
		if(result == kResultOk)
			return NEW NetworkStream (s);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static SocketIDSet* toDescriptorList (SocketID& highestSocket, SocketIDSet* descriptorList, const IUnknownList* unknownList)
{
	if(!unknownList)
		return nullptr;
	ForEachUnknown (*unknownList, unk)
		SocketID descriptor = UnknownPtr<Socket> (unk)->getDescriptor ();
		descriptorList->set (descriptor);
		if(descriptor > highestSocket)
			highestSocket = descriptor;
	EndFor
	return descriptorList;
}

static bool fromDescriptorList (IUnknownList* unknownList, SocketIDSet* descriptorList)
{
	bool state = false;
	if(unknownList)
		ForEachUnknown (*unknownList, unk)
			SocketID descriptor = UnknownPtr<Socket> (unk)->getDescriptor ();
			if(descriptorList->isSet (descriptor))
				state = true;
			else
			{
				unknownList->remove (unk);
				unk->release (); // we expect the list to have ownership
			}
		EndFor
	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Network::selectSockets (IUnknownList* readList, IUnknownList* writeList, IUnknownList* errorList, int timeout)
{
	SocketID highestSocket = 0;
	SocketIDSet readFds = { {0} }, writeFds = { {0} }, errorFds = { {0} };
	
	SocketIDSet* readArg = toDescriptorList (highestSocket, &readFds, readList);
	SocketIDSet* writeArg = toDescriptorList (highestSocket, &writeFds, writeList);
	SocketIDSet* errorArg = toDescriptorList (highestSocket, &errorFds, errorList);

	int result = Core::Sockets::Socket::select (highestSocket, readArg, writeArg, errorArg, timeout);
	if(result < 0)
		return kResultSocketError;

	tresult state = kResultFailed;
	if(fromDescriptorList (readList, &readFds))
		state = kResultOk;
	if(fromDescriptorList (writeList, &writeFds))
		state = kResultOk;
	if(fromDescriptorList (errorList, &errorFds))
		state = kResultOk;
	return state;
}
