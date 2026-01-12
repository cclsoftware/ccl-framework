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
// Filename    : core/platform/shared/openssl/coresslcontext.openssl.cpp
// Description : SSL context based on OpenSSL
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "core/platform/shared/openssl/coresslcontext.openssl.h"

#include "core/portable/coresingleton.h"

#include "core/system/coredebug.h"

#include <openssl/err.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// OpenSSLInitializer
//************************************************************************************************

class OpenSSLInitializer: public Portable::StaticSingleton<OpenSSLInitializer>
{
public:
	OpenSSLInitializer ()
	{
		SSL_library_init ();
		SSL_load_error_strings ();
		OpenSSL_add_all_algorithms();
	}
	
	~OpenSSLInitializer ()
	{
		EVP_cleanup ();
		SSL_COMP_free_compression_methods ();
		CRYPTO_cleanup_all_ex_data ();
	}
};

} // namespace Platform
} // namespace Core

using namespace Core;
using namespace Platform;

DEFINE_STATIC_SINGLETON (OpenSSLInitializer)

//************************************************************************************************
// OpenSSLContext
//************************************************************************************************

OpenSSLContext::OpenSSLContext ()
: ioHandler (nullptr),
  context (nullptr),
  readBIO (nullptr),
  writeBIO (nullptr),
  ssl (nullptr)
{
	OpenSSLInitializer::instance ();
	
	sslBuffer.resize (kBufferSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenSSLContext::setIOHandler (ISSLContextIOHandler* _ioHandler)
{
	ioHandler = _ioHandler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenSSLContext::setPeerName (CStringPtr peerName)
{
	ASSERT (context == nullptr)
	this->peerName = peerName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult OpenSSLContext::initialize ()
{
	ASSERT (context == nullptr)
	
	const SSL_METHOD* method = TLS_client_method ();
	if(method == nullptr)
		return kSSLFailed;
	
	context = SSL_CTX_new (method);
	if(context == nullptr)
		return kSSLFailed;
	
	if(!SSL_CTX_set_default_verify_paths (context))
	{
		cleanup ();
		return kSSLFailed;
	}
	
	ssl = SSL_new (context);
	if(ssl == nullptr)
	{
		cleanup ();
		return kSSLFailed;
	}

#if DEBUG
	SSL_CTX_set_msg_callback (context, messageCallback);
	SSL_set_info_callback (ssl, infoCallback);
#endif
	
	readBIO = BIO_new (BIO_s_mem ());
	if(readBIO == nullptr)
	{
		cleanup ();
		return kSSLFailed;
	}
	BIO_set_mem_eof_return (readBIO, -1);
	
	writeBIO = BIO_new (BIO_s_mem ());
	if(writeBIO == nullptr)
	{
		cleanup ();
		return kSSLFailed;
	}
	BIO_set_mem_eof_return (writeBIO, -1);
	
	SSL_set_mode (ssl, SSL_MODE_AUTO_RETRY);
	SSL_set_bio (ssl, writeBIO, readBIO);
	SSL_set_connect_state (ssl);
	
	CString128 hostName (peerName);
	for(int i = hostName.index ('.'); i != hostName.lastIndex ('.'); i = hostName.index ('.'))
	{
		hostName.subString (hostName, i + 1);
	}
	if(!SSL_set1_host(ssl, peerName.str ()))
	{
		cleanup ();
		return kSSLFailed;
	}
	if(!SSL_add1_host(ssl, hostName.str ()))
	{
		cleanup ();
		return kSSLFailed;
	}
	SSL_set_tlsext_host_name (ssl, peerName.str ());
	SSL_set_verify (ssl, SSL_VERIFY_PEER, nullptr);
	
	// Avoid SSLv2 and SSLv3
	SSL_CTX_set_options (context, SSL_OP_ALL|SSL_OP_NO_SSLv2|SSL_OP_NO_SSLv3);
	
	return kSSLSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult OpenSSLContext::handshake ()
{
	if(context == nullptr && initialize () != kSSLSuccess)
		return kSSLFailed;
	
	for(int result = SSL_do_handshake (ssl); result < 0; )
	{
		switch(SSL_get_error (ssl, result))
		{
		case SSL_ERROR_WANT_WRITE :
		case SSL_ERROR_WANT_READ :
			flushRead ();
			break;
		default :
			CORE_PRINTF ("%d: %s\n", SSL_get_error (ssl, result), ERR_reason_error_string (ERR_get_error ()));
			return kSSLFailed;
		}
		if(!SSL_in_connect_init (ssl))
			break;
		
		if(flushWrite () > 0)
			result = SSL_do_handshake (ssl);
	}
	
	return kSSLSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int OpenSSLContext::flushWrite ()
{
	int bytesReadFromSocket = 0;
	if(ioHandler->read (sslBuffer.getAddress (), sslBuffer.getSize (), bytesReadFromSocket) == kSSLSuccess && bytesReadFromSocket > 0)
		return BIO_write (writeBIO, sslBuffer.getAddress (), bytesReadFromSocket);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int OpenSSLContext::flushRead ()
{
	int bytesRead = 0;
	while(BIO_ctrl_pending (readBIO) > 0)
	{
		int bytesReadFromSSL = BIO_read (readBIO, sslBuffer.getAddress (), sslBuffer.getSize ());
		if(bytesReadFromSSL < 0)
			return -1;
		int offset = 0;
		while(offset < bytesReadFromSSL)
		{
			int bytesWrittenToSocket = 0;
			if(ioHandler->write (static_cast<const char*> (sslBuffer.getAddress ()) + offset, bytesReadFromSSL, bytesWrittenToSocket) != kSSLSuccess)
				return -1;
			offset += bytesWrittenToSocket;
		}
		bytesRead += offset;
	}
	return bytesRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenSSLContext::flush (int result)
{
	switch(SSL_get_error (ssl, result))
	{
	case SSL_ERROR_WANT_READ :
		flushRead ();
		return true;
	case SSL_ERROR_WANT_WRITE :
		flushWrite ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult OpenSSLContext::close ()
{
	cleanup ();
	return kSSLSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult OpenSSLContext::write (const void* buffer, int size, int& bytesWritten)
{
	bytesWritten = 0;
	if(context)
	{
		while(size > 0)
		{
			int bytesWrittenToSSL = 0;
			bool repeat = false;
			do 
			{				
				repeat = false;
				bytesWrittenToSSL = SSL_write (ssl, static_cast<const char*> (buffer) + bytesWritten, size);
				if(bytesWrittenToSSL < 0)
					repeat = flush (bytesWrittenToSSL);
				else
				{
					bytesWritten += bytesWrittenToSSL;
					size -= bytesWrittenToSSL;
				}			
			} while(repeat);
		}
		return kSSLSuccess;
	}
	return kSSLFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult OpenSSLContext::read (void* buffer, int size, int& bytesRead)
{
	bytesRead = 0;
	if(context)
	{
		while(size > 0)
		{
			int maxReadBytes = size;
			if(maxReadBytes > sslBuffer.getSize ())
				maxReadBytes = sslBuffer.getSize ();
			int bytesReadFromSSL = 0;
			bool repeat = false;
			do 
			{
				repeat = false;
				bytesReadFromSSL = SSL_read (ssl, static_cast<char*> (buffer) + bytesRead, maxReadBytes);
				if(bytesReadFromSSL < 0)
					repeat = flush (bytesReadFromSSL);
				flushWrite ();
			} while (repeat);
			
			if(bytesReadFromSSL < 0)
			{
				CORE_PRINTF ("%d: %s\n", SSL_get_error (ssl, bytesReadFromSSL), ERR_reason_error_string (ERR_get_error ()));
				return kSSLFailed;
			}
			bytesRead += bytesReadFromSSL;
			size -= bytesReadFromSSL;
		}
		return kSSLSuccess;
	}
	return kSSLFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenSSLContext::cleanup ()
{
	if(ssl)
		SSL_free (ssl); // also frees readBIO and writeBIO
	if(context)
		SSL_CTX_free (context);
	context = nullptr;
	readBIO = nullptr;
	writeBIO = nullptr;
	ssl = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG

void OpenSSLContext::infoCallback (const SSL* ssl, int where, int result)
{
	CORE_PRINTF ("SSL: %s - %s\n", SSL_state_string (ssl), SSL_state_string_long (ssl));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenSSLContext::messageCallback (int writeFlag, int version, int contentType, const void* buffer, size_t size, SSL* ssl, void* arg)
{
	CStringBuffer<1024> stringBuffer;
	if(size > 0)
	{
		stringBuffer.append (static_cast<const char*> (buffer), int (size));
		CORE_PRINTF ("SSL message: %s\n", stringBuffer.str ());
	}
}

#endif
