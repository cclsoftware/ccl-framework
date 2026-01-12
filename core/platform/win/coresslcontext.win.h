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
// Filename    : core/platform/win/coresslcontext.win.h
// Description : SSL context based on Win32 Schannel API
//
//************************************************************************************************

#ifndef _coresslcontext_win_h
#define _coresslcontext_win_h

#include "core/platform/shared/coreplatformsslcontext.h"

#include "core/public/corebuffer.h"
#include "core/public/coremacros.h"
#include "core/public/corestringbuffer.h"

#include <windows.h>

#define SECURITY_WIN32
#include <security.h>
#include <schnlsp.h>

namespace Core {
namespace Platform {

//************************************************************************************************
// StreamBuffer
//************************************************************************************************

class StreamBuffer
{
public:
	StreamBuffer ()
	: bytesFilled (0),
	  tag (0)
	{}

	int getTag () const	{ return tag; }
	void changed ()		{ tag++; }
	
	bool isEmpty () const			{ return bytesFilled == 0; }
	int getBytesFilled () const		{ return bytesFilled; }	
	const void* getData () const	{ return data; }

	int write (const void* srcBuffer, int bytesToWrite);
	int read (void* dstBuffer, int bytesToRead);		
	
	void* reserve (int bytesToWrite);
	void adjustAfterWrite (int bytesWritten);
	void adjustAfterRead (int bytesRead);
		
protected:
	IO::Buffer data;
	int bytesFilled;
	int tag;

	int getSize () const { return data.getSize (); }
};

//************************************************************************************************
// SecureChannel
//************************************************************************************************

class SecureChannel
{
public:
	SecureChannel ();
	~SecureChannel ();

	PROPERTY_CSTRING_BUFFER (128, peerName, PeerName)

	bool isInitialized () const;
	SSLResult initialize (StreamBuffer& output, StreamBuffer& input);
	SSLResult shutdown (StreamBuffer& output);

	SSLResult encryptMessage (StreamBuffer& output, const void* message, int messageSize, int& bytesProcessed);
	SSLResult decryptMessage (StreamBuffer& output, const void* message, int messageSize, int& bytesProcessed);
	SSLResult decryptMessage (StreamBuffer& output, StreamBuffer& input, int& bytesProcessed);	

protected:
	::CredHandle hCredential;
	::CtxtHandle hContext;
};

//************************************************************************************************
// Win32SSLContext
//************************************************************************************************

class Win32SSLContext: public ISSLContext
{
public:
	Win32SSLContext ();

	// ISSLContext
	void setIOHandler (ISSLContextIOHandler* ioHandler) override;
	void setPeerName (CStringPtr peerName) override;
	SSLResult handshake () override;
	SSLResult close () override;
	SSLResult write (const void* buffer, int size, int& bytesWritten) override;
	SSLResult read (void* buffer, int size, int& bytesRead) override;

protected:
	static const int kMaxBytesBuffered;

	ISSLContextIOHandler* ioHandler;
	SecureChannel schannel;
	StreamBuffer dataFromWire;
	StreamBuffer dataToWire;	
	StreamBuffer dataForClient;
	bool renegotiatePending;
	bool wasIncompleteMessage;
	int incompleteMessageTag;

	void captureIncompleteMessage (SSLResult& result)
	{
		wasIncompleteMessage = result == kSSLIncompleteMessage;
		if(wasIncompleteMessage)
		{
			result = kSSLWouldBlock;
			incompleteMessageTag = dataFromWire.getTag ();
		}
	}

	bool isMessageStillIncomplete () const
	{
		return wasIncompleteMessage == true && dataFromWire.getTag () == incompleteMessageTag;
	}

	SSLResult flushReadDirection (int* bytesReadOptional = nullptr);
	SSLResult flushWriteDirection (int* bytesWrittenOptional = nullptr);
};

typedef Win32SSLContext SSLContext;

} // namespace Platform
} // namespace Core

#endif // _coresslcontext_win_h
