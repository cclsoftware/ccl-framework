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
// Filename    : core/platform/cocoa/coresslcontext.cocoa.h
// Description : SSL session based on Apple Secure Transport API
//
//************************************************************************************************

#ifndef _coresslcontext_cocoa_h
#define _coresslcontext_cocoa_h

#include "core/platform/shared/coreplatformsslcontext.h"

#include "Security/Security.h"
#if CORE_PLATFORM_IOS
#include "Security/SecureTransport.h"
#endif

namespace Core {
namespace Platform {

//************************************************************************************************
// CocoaSSLContext
//************************************************************************************************

class CocoaSSLContext: public ISSLContext
{
public:
	CocoaSSLContext ();
	~CocoaSSLContext ();

	// ISSLContext
	void setIOHandler (ISSLContextIOHandler* ioHandler) override;
	void setPeerName (CStringPtr peerName) override;
	SSLResult handshake () override;
	SSLResult close () override;
	SSLResult write (const void* buffer, int size, int& bytesWritten) override;
	SSLResult read (void* buffer, int size, int& bytesRead) override;
				
protected:
	static OSStatus readFunction (SSLConnectionRef connection, void* data, size_t* dataLength);
	static OSStatus writeFunction (SSLConnectionRef connection, const void* data, size_t* dataLength);
	static OSStatus statusFromSSLResult (SSLResult result);
	static SSLResult sslResultFromStatus (OSStatus result);
	
	SSLContextRef context;
	ISSLContextIOHandler* ioHandler; 
};

typedef CocoaSSLContext SSLContext;

} // namespace Platform
} // namespace Core

#endif // _coresslcontext_cocoa_h
