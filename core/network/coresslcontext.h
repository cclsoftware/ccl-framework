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
// Filename    : core/network/coresslcontext.h
// Description : SSL context
//
//************************************************************************************************

#ifndef _coresslcontext_h
#define _coresslcontext_h

#include "core/platform/corefeatures.h"

#if CORE_SSL_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
	#include CORE_PLATFORM_IMPLEMENTATION_HEADER (coresslcontext)
#elif CORE_SSL_IMPLEMENTATION == CORE_EXTERNAL_PLATFORM_IMPLEMENTATION
	#include CORE_EXTERNAL_PLATFORM_IMPLEMENTATION_HEADER (coresslcontext)
#elif CORE_SSL_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_OPENSSL
	#include "core/platform/shared/openssl/coresslcontext.openssl.h"
#elif CORE_SSL_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED
	#include "core/platform/shared/coreplatformsslcontext.h"
#endif

namespace Core {
namespace Sockets {

namespace SSLTypes {
using namespace Platform::SSLTypes; }
using namespace SSLTypes;

//************************************************************************************************
// SSLContextIOHandler
//************************************************************************************************

class SSLContextIOHandler: public Platform::ISSLContextIOHandler
{
public:
	virtual SSLResult write (const void* buffer, int size, int& bytesWritten) override = 0;
	virtual SSLResult read (void* buffer, int size, int& bytesRead) override = 0;
};

//************************************************************************************************
// SSLContext
//************************************************************************************************

class SSLContext
{
public:
	void setIOHandler (SSLContextIOHandler* ioHandler);
	void setPeerName (CStringPtr peerName);

	SSLResult handshake ();
	SSLResult close ();
	SSLResult write (const void* buffer, int size, int& bytesWritten);
	SSLResult read (void* buffer, int size, int& bytesRead);
	
protected:
	Platform::SSLContext platformContext;
};

//************************************************************************************************
// SSLContext implementation
//************************************************************************************************

inline void SSLContext::setIOHandler (SSLContextIOHandler* ioHandler)
{ platformContext.setIOHandler (ioHandler); }

inline void SSLContext::setPeerName (CStringPtr peerName)
{ platformContext.setPeerName (peerName); }

inline SSLResult SSLContext::handshake ()
{ return platformContext.handshake (); }

inline SSLResult SSLContext::close ()
{ return platformContext.close (); }

inline SSLResult SSLContext::write (const void* buffer, int size, int& bytesWritten)
{ return platformContext.write (buffer, size, bytesWritten); }

inline SSLResult SSLContext::read (void* buffer, int size, int& bytesRead)
{ return platformContext.read (buffer, size, bytesRead); }

} // namespace Sockets
} // namespace Core

#endif // _coresslcontext_h
