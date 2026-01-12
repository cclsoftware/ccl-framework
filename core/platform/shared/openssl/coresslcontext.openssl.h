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
// Filename    : core/platform/shared/openssl/coresslcontext.openssl.h
// Description : SSL context based on OpenSSL
//
//************************************************************************************************

#ifndef _coresslcontext_openssl_h
#define _coresslcontext_openssl_h

#include "core/platform/shared/coreplatformsslcontext.h"

#include "core/public/corestringbuffer.h"
#include "core/public/corebuffer.h"

#include <openssl/bio.h>
#include <openssl/ssl.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// OpenSSLContext
//************************************************************************************************

class OpenSSLContext: public ISSLContext
{
public:
	OpenSSLContext ();

	// ISSLContext
	void setIOHandler (ISSLContextIOHandler* ioHandler) override;
	void setPeerName (CStringPtr peerName) override;
	SSLResult handshake () override;
	SSLResult close () override;
	SSLResult write (const void* buffer, int size, int& bytesWritten) override;
	SSLResult read (void* buffer, int size, int& bytesRead) override;

	#if DEBUG
	static void infoCallback (const SSL* ssl, int where, int result);
	static void messageCallback (int writeFlag, int version, int contentType, const void* buffer, size_t size, SSL* ssl, void* arg);
	#endif

protected:
	static const int kBufferSize = 4096;
	
	ISSLContextIOHandler* ioHandler;
	
	SSL* ssl;
	SSL_CTX* context;
	BIO* readBIO; //< used to read encrypted data from SSL engine
	BIO* writeBIO; //< used to write encrypted data from socket to SSL engine
	
	CString128 peerName;
	IO::Buffer sslBuffer;
	
	SSLResult initialize ();
	void cleanup ();
	bool flush (int result);
	int flushWrite ();
	int flushRead ();
};

typedef OpenSSLContext SSLContext;

} // namespace Platform
} // namespace Core

#endif // _coresslcontext_openssl_h
