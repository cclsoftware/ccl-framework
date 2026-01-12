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
// Filename    : ccl/network/web/localclient.h
// Description : Local Client
//
//************************************************************************************************

#ifndef _ccl_localclient_h
#define _ccl_localclient_h

#include "ccl/network/web/webclient.h"

#include "ccl/public/text/cclstring.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// LocalClient
//************************************************************************************************

class LocalClient: public WebClient
{
public:
	DECLARE_CLASS (LocalClient, WebClient)

	static LocalClient* create (StringID protocol);

	// WebClient
	tresult CCL_API connect (StringRef hostname) override;
	int CCL_API getLastStatus () override;
	tresult CCL_API downloadData (StringRef remotePath, IStream& localStream, IWebHeaderCollection* headers = nullptr,
								  IProgressNotify* progress = nullptr) override;
	tresult CCL_API uploadData (IWebHeaderCollection* headers, IStream& localStream, StringRef remotePath, IStream& responseStream,
								StringID method = nullptr, IProgressNotify* progress = nullptr) override;

protected:
	LocalClient (StringRef protocol = nullptr);

	String protocol;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_localclient_h
