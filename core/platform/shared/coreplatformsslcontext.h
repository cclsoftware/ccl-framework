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
// Filename    : core/platform/shared/coreplatformsslcontext.h
// Description : SSL Context platform implementation base
//
//************************************************************************************************

#ifndef _coreplatformsslcontext_h
#define _coreplatformsslcontext_h

#include "core/platform/corefeatures.h"

#include "core/public/coretypes.h"

namespace Core {
namespace Platform {
	
namespace SSLTypes {

//************************************************************************************************
// SSLResult
//************************************************************************************************

enum SSLResult
{
	kSSLSuccess = 0,
	kSSLWouldBlock,
	kSSLFailed,

	// private
	kSSLRenegotiate,
	kSSLIncompleteMessage
};

} // namespace SSLTypes

using namespace SSLTypes;

//************************************************************************************************
// ISSLContextIOHandler
//************************************************************************************************

struct ISSLContextIOHandler
{
	virtual ~ISSLContextIOHandler () {}

	virtual SSLResult write (const void* buffer, int size, int& bytesWritten) = 0;
	
	virtual SSLResult read (void* buffer, int size, int& bytesRead) = 0;	
};

//************************************************************************************************
// ISSLContext
//************************************************************************************************

struct ISSLContext: public ISSLContextIOHandler
{
	virtual void setIOHandler (ISSLContextIOHandler* ioHandler) = 0;
	
	virtual void setPeerName (CStringPtr peerName) = 0;

	virtual SSLResult handshake () = 0;
	
	virtual SSLResult close () = 0;
	
	virtual SSLResult write (const void* buffer, int size, int& bytesWritten) override = 0;
	
	virtual SSLResult read (void* buffer, int size, int& bytesRead) override = 0;
};

#if CORE_SSL_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED

//************************************************************************************************
// SSLContextStub
//************************************************************************************************

class SSLContextStub: public ISSLContext
{
public:
	// ISSLContext
	void setIOHandler (ISSLContextIOHandler* ioHandler) override {}
	void setPeerName (CStringPtr peerName) override {}
	SSLResult handshake () override { return kSSLFailed; }
	SSLResult close () override { return kSSLFailed; }
	SSLResult write (const void* buffer, int size, int& bytesWritten) override { return kSSLFailed; }
	SSLResult read (void* buffer, int size, int& bytesRead) override { return kSSLFailed; }
};

typedef SSLContextStub SSLContext;
#endif

} // namespace Sockets
} // namespace Core

#endif // _coreplatformsslcontext_h
