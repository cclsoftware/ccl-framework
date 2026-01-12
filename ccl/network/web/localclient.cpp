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
// Filename    : ccl/network/web/localclient.cpp
// Description : Local Client
//
//************************************************************************************************

#include "ccl/network/web/localclient.h"
#include "ccl/network/web/webrequest.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/iprotocolhandler.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/network/web/iwebservice.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Web;

//************************************************************************************************
// LocalClient
//************************************************************************************************

LocalClient* LocalClient::create (StringID _protocol)
{
	String protocol (_protocol);
	UnknownPtr<IProtocolHandlerRegistry> registry (&System::GetFileSystem ());
	ASSERT (registry.isValid ())
	if(registry->getHandler (protocol) != nullptr)
		return NEW LocalClient (protocol);
	else
		return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (LocalClient, WebClient)

//////////////////////////////////////////////////////////////////////////////////////////////////

LocalClient::LocalClient (StringRef protocol)
: protocol (protocol)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocalClient::connect (StringRef hostname)
{
	getLastStatus (); // reset first error

	return SuperClass::connect (hostname);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API LocalClient::getLastStatus ()
{
	lastStatus = 0;
	System::GetFileSystem ().getFirstError (lastStatus);
	return lastStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocalClient::downloadData (StringRef _remotePath, IStream& localStream, IWebHeaderCollection* headers, IProgressNotify* progress)
{
	Url path;
	path.setProtocol (protocol);
	path.setHostName (hostname);

	String remotePath (_remotePath);
	if(remotePath.startsWith (CCLSTR ("/")))
		remotePath.remove (0, 1);

	// decode parameters
	int paramIndex = remotePath.lastIndex (CCLSTR ("?"));
	if(paramIndex > 0)
	{
		String params = remotePath.subString (paramIndex + 1);
		path.setParameters (params);
		remotePath.truncate (paramIndex);
	}

	path.setPath (remotePath, Url::kFile);

	AutoPtr<IStream> srcStream = System::GetFileSystem ().openStream (path);
	if(srcStream == nullptr)
		return kResultFailed;

	// notify content length
	if(UnknownPtr<IObserver> progressObserver = progress)
	{
		ASSERT (srcStream->isSeekable ())
		int64 contentLength = srcStream->seek (0, IStream::kSeekEnd);
		srcStream->rewind ();

		AutoPtr<WebHeaderCollection> headers = NEW WebHeaderCollection;
		progressObserver->notify (nullptr, Message (Meta::kContentLengthNotify, contentLength, headers->asUnknown ()));
	}

	tbool copied = System::GetFileUtilities ().copyStream (localStream, *srcStream, progress);
	return copied ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LocalClient::uploadData (IWebHeaderCollection* headers, IStream& localStream, StringRef remotePath,
										 IStream& responseStream, StringID method, IProgressNotify* progress)
{
	CCL_NOT_IMPL ("LocalClient::uploadData() not implemented!")
	return kResultNotImplemented;
}
