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
// Filename    : ccl/extras/web/webprotocol.cpp
// Description : Web Protocol
//
//************************************************************************************************

#include "ccl/extras/web/webprotocol.h"

#include "ccl/public/network/web/iwebservice.h"
#include "ccl/public/netservices.h"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// WebClientProtocol
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (WebClientProtocol, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebClientProtocol::WebClientProtocol (StringID protocol)
: protocol (protocol),
  registered (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WebClientProtocol::~WebClientProtocol ()
{
	ASSERT (registered == false)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebClientProtocol::registerHandler ()
{
	UnknownPtr<IWebProtocolRegistrar> registrar (&System::GetWebService ());
	ASSERT (registrar.isValid ())
	registrar->registerProtocol (this);
	registered = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WebClientProtocol::unregisterHandler ()
{
	UnknownPtr<IWebProtocolRegistrar> registrar (&System::GetWebService ());
	ASSERT (registrar.isValid ())
	registrar->unregisterProtocol (this);
	registered = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API WebClientProtocol::getProtocol () const
{
	return protocol;
}

//************************************************************************************************
// WebClient
//************************************************************************************************

DEFINE_CLASS_HIDDEN (WebClient, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

WebClient::WebClient ()
: connected (false),
  lastStatus (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WebClient::setCredentials (IWebCredentials* credentials)
{
	this->credentials = credentials;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebClient::connect (StringRef hostname)
{
	lastStatus = 0;
	connected = true;
	this->hostname = hostname;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebClient::disconnect ()
{
	connected = false;
	hostname.empty ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WebClient::isConnected ()
{
	return connected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API WebClient::getLastStatus ()
{
	return lastStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebClient::downloadData (StringRef remotePath, IStream& localStream, IWebHeaderCollection* headers, IProgressNotify* progress)
{
	CCL_NOT_IMPL ("WebClient::downloadData not implemented!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebClient::uploadData (IWebHeaderCollection* headers, IStream& localStream, StringRef remotePath, 
									   IStream& responseStream, StringID method, IProgressNotify* progress)
{
	CCL_NOT_IMPL ("WebClient::uploadData not implemented!")
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WebClient::setOption (StringID optionId, VariantRef value)
{
	return kResultNotImplemented;
}

//************************************************************************************************
// SimpleFileClient::DirIterator
//************************************************************************************************

void SimpleFileClient::DirIterator::add (const DirEntry& entry, IUnknown* object)
{
	Entry e;
	e.base = entry;
	e.object = object;
	entries.add (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWebFileClient::DirEntry& SimpleFileClient::DirIterator::addDirectory (StringRef name, int flags)
{
	Entry e;
	e.base.name = name;
	e.base.directory = true;
	e.base.flags = flags;
	int index = entries.count ();
	entries.add (e);
	return entries[index].base;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IWebFileClient::DirEntry* CCL_API SimpleFileClient::DirIterator::getEntry (int index) const
{
	return index >= 0 && index < entries.count () ? &entries.at (index).base : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API SimpleFileClient::DirIterator::getObject (int index) const
{
	return index >= 0 && index < entries.count () ? entries.at (index).object : nullptr;
}

//************************************************************************************************
// SimpleFileClient
//************************************************************************************************

DEFINE_CLASS_HIDDEN (SimpleFileClient, WebClient)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API SimpleFileClient::getServerInfo (StringRef remotePath, ServerInfo& info)
{
	info = ServerInfo ();
	return kResultOk;
}
