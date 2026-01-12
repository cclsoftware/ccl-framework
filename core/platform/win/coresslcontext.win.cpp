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
// Filename    : core/platform/win/coresslcontext.win.cpp
// Description : SSL context based on Win32 Schannel API
//
//************************************************************************************************

#include "coresslcontext.win.h"

#include "core/system/coredebug.h"

#define LOG_SECURITY_BUFFERS (0 && DEBUG_LOG)

#pragma comment (lib, "Secur32.lib")

/*
	http://www.codeproject.com/Articles/2642/SSL-TLS-client-server-for-NET-and-SSL-tunnelling
	http://www.codeproject.com/KB/IP/ssl_sockets.aspx
*/

using namespace Core;
using namespace Platform;

//////////////////////////////////////////////////////////////////////////////////////////////////

#if DEBUG_LOG
static void printSecurityStatus (CStringPtr functionName, SECURITY_STATUS status)
{
	DebugPrintf ("%s ", functionName);
	#define CASE_STATUS(kStatus) case kStatus : DebugPrint (#kStatus "\n"); break;
	switch(status)
	{
	CASE_STATUS (SEC_E_OK)
	CASE_STATUS (SEC_I_COMPLETE_AND_CONTINUE)
	CASE_STATUS (SEC_I_COMPLETE_NEEDED)
	CASE_STATUS (SEC_I_CONTINUE_NEEDED)
	CASE_STATUS (SEC_I_INCOMPLETE_CREDENTIALS)	// The server has requested client authentication
	CASE_STATUS (SEC_E_INCOMPLETE_MESSAGE)
	CASE_STATUS (SEC_E_INSUFFICIENT_MEMORY)
	CASE_STATUS (SEC_E_INTERNAL_ERROR)
	CASE_STATUS (SEC_E_INVALID_TOKEN)
	CASE_STATUS (SEC_E_ILLEGAL_MESSAGE)
	CASE_STATUS (SEC_E_DECRYPT_FAILURE)
	default :
		DebugPrintf ("Security Status %08X\n", status);
		break;
	}
	#undef CASE_STATUS
}
#define PRINT_STATUS(functionName, status) printSecurityStatus (functionName, status);
#else
#define PRINT_STATUS(functionName, status)
#endif

struct SecBufferList: SecBufferDesc
{
	SecBufferList (PSecBuffer buffers, int count)
	{
		ulVersion = SECBUFFER_VERSION;
		cBuffers = count;
		pBuffers = buffers;
	}

	SecBuffer* findBuffer (unsigned long bufferType)
	{
		for(unsigned long i = 0; i < cBuffers; i++)
			if(pBuffers[i].BufferType == bufferType)
				return &pBuffers[i];
		return nullptr;
	}

	#if DEBUG
	void dump (CStringPtr name)
	{
		DebugPrintf ("%s\n", name);
		for(unsigned long i = 0; i < cBuffers; i++)
			DebugPrintf (" - buffer[%d]: type = %d size = %d address = %p\n", i,
						 pBuffers[i].BufferType, pBuffers[i].cbBuffer, pBuffers[i].pvBuffer);			
	}
	#endif
};

//************************************************************************************************
// StreamBuffer
//************************************************************************************************

int StreamBuffer::write (const void* srcBuffer, int bytesToWrite)
{
	void* dst = reserve (bytesToWrite);
	::memcpy (dst, srcBuffer, bytesToWrite);
	bytesFilled += bytesToWrite;
	return bytesToWrite;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

int StreamBuffer::read (void* dstBuffer, int bytesToRead)
{
	if(bytesToRead > bytesFilled)
		bytesToRead = bytesFilled;
			
	if(bytesToRead > 0)
	{
		::memcpy (dstBuffer, data.getAddress (), bytesToRead);
		adjustAfterRead (bytesToRead);
	}
	return bytesToRead;
}
		
//////////////////////////////////////////////////////////////////////////////////////////////////

void* StreamBuffer::reserve (int bytesToWrite)
{
	int bytesNeeded = bytesFilled + bytesToWrite;
	if(getSize () < bytesNeeded)
		data.resize (bytesNeeded);
	return data.as<char> () + bytesFilled;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void StreamBuffer::adjustAfterWrite (int bytesWritten)
{
	bytesFilled += bytesWritten;
	ASSERT (bytesFilled <= getSize ())
	if(bytesFilled > getSize ())
		bytesFilled = getSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void StreamBuffer::adjustAfterRead (int bytesRead)
{
	bytesFilled -= bytesRead;
	ASSERT (bytesFilled >= 0)
	if(bytesFilled < 0)
		bytesFilled = 0;
		
	if(bytesFilled > 0)
		::memmove (data.getAddress (), data.as<char> () + bytesRead, bytesFilled);
}

//************************************************************************************************
// SecureChannel
//************************************************************************************************

SecureChannel::SecureChannel ()
{
	SecInvalidateHandle (&hCredential);
	SecInvalidateHandle (&hContext);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SecureChannel::~SecureChannel ()
{
	if(SecIsValidHandle (&hContext))
	{
		SECURITY_STATUS status = ::DeleteSecurityContext (&hContext);
		ASSERT (status == SEC_E_OK)	
	}

	if(SecIsValidHandle (&hCredential))
	{
		SECURITY_STATUS status = ::FreeCredentialsHandle (&hCredential);
		ASSERT (status == SEC_E_OK)	
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SecureChannel::isInitialized () const
{
	return SecIsValidHandle (&hContext);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult SecureChannel::initialize (StreamBuffer& output, StreamBuffer& input)
{
	if(!SecIsValidHandle (&hCredential))
	{
		SCHANNEL_CRED credential = {0};
		credential.dwVersion = SCHANNEL_CRED_VERSION;
		credential.dwFlags = SCH_CRED_NO_DEFAULT_CREDS|SCH_CRED_NO_SYSTEM_MAPPER|SCH_CRED_REVOCATION_CHECK_CHAIN;

		SECURITY_STATUS status = ::AcquireCredentialsHandle (nullptr, SCHANNEL_NAME, SECPKG_CRED_OUTBOUND, nullptr,
									&credential, nullptr, nullptr, &hCredential, nullptr);
		if(FAILED (status))
			return kSSLFailed;
	}

	SecBuffer inputBuffers[2] = {0};
	inputBuffers[0].BufferType = SECBUFFER_TOKEN;
	inputBuffers[1].BufferType = SECBUFFER_EMPTY;

	SecBuffer outputBuffers[1] = {0};
	outputBuffers[0].BufferType = SECBUFFER_TOKEN;

	SecBufferList inputBufferList (inputBuffers, ARRAY_COUNT (inputBuffers));
	SecBufferList outputBufferList (outputBuffers, ARRAY_COUNT (outputBuffers));

	bool firstCall = !isInitialized ();
	if(firstCall == false)
	{
		inputBuffers[0].pvBuffer = const_cast<void*> (input.getData ());
		inputBuffers[0].cbBuffer = input.getBytesFilled ();
	}
		
	ULONG requestFlags =	ISC_REQ_SEQUENCE_DETECT |
							ISC_REQ_REPLAY_DETECT |
							ISC_REQ_CONFIDENTIALITY |
							ISC_RET_EXTENDED_ERROR |
							ISC_REQ_ALLOCATE_MEMORY |
							ISC_REQ_STREAM |
							ISC_REQ_MANUAL_CRED_VALIDATION;
	DWORD outFlags = 0;		
	SECURITY_STATUS status = ::InitializeSecurityContextA (&hCredential, firstCall ? nullptr : &hContext, 
									const_cast<char*> (peerName.str ()), requestFlags, 0, 0, firstCall ? nullptr : &inputBufferList, 0, 
									firstCall ? &hContext : nullptr, &outputBufferList, &outFlags, nullptr);
	PRINT_STATUS ("InitializeSecurityContext", status)
	#if LOG_SECURITY_BUFFERS
	if(firstCall == false)
		inputBufferList.dump ("InitializeSecurityContext input:");
	outputBufferList.dump ("InitializeSecurityContext output:");
	#endif

	if(outputBuffers[0].cbBuffer > 0)
	{
		output.write (outputBuffers[0].pvBuffer, outputBuffers[0].cbBuffer);
		::FreeContextBuffer (outputBuffers[0].pvBuffer);
	}

	if(status == SEC_E_INCOMPLETE_MESSAGE)
	{
		// SSPI didn't consume any of the input data
		return kSSLIncompleteMessage;
	}

	if(FAILED (status))
		return kSSLFailed;

	// Extra Buffers Returned by Schannel
	// see http://msdn.microsoft.com/en-us/library/windows/desktop/aa375412%28v=vs.85%29.aspx
	int bytesTotal = inputBuffers[0].cbBuffer;
	if(inputBuffers[1].BufferType == SECBUFFER_EXTRA)
	{
		int bytesUnprocessed = inputBuffers[1].cbBuffer;
		int bytesProcessed = bytesTotal - bytesUnprocessed;

		#if DEBUG_LOG
		DebugPrintf ("%d of %d bytes unprocessed by SSPI, processed only %d bytes\n", 
					bytesUnprocessed, bytesTotal, bytesProcessed);
		#endif
				
		if(bytesProcessed > 0)
			input.adjustAfterRead (bytesProcessed);
	}
	else
		input.adjustAfterRead (bytesTotal);
		
	return status == SEC_E_OK ? kSSLSuccess : kSSLWouldBlock;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult SecureChannel::shutdown (StreamBuffer& output)
{
	DWORD token = SCHANNEL_SHUTDOWN;
		
	SecBuffer buffers[1] = {0};
	buffers[0].BufferType = SECBUFFER_TOKEN;
	buffers[0].pvBuffer = &token;
	buffers[0].cbBuffer = sizeof(DWORD);
		
	SecBufferList bufferList (buffers, 1);
	SECURITY_STATUS status = ::ApplyControlToken (&hContext, &bufferList);
	PRINT_STATUS ("ApplyControlToken SCHANNEL_SHUTDOWN", status)
	if(FAILED (status))
		return kSSLFailed;

	buffers[0].cbBuffer = 0;
	buffers[0].pvBuffer = nullptr;

	ULONG requestFlags =	ISC_REQ_SEQUENCE_DETECT |
							ISC_REQ_REPLAY_DETECT |
							ISC_REQ_CONFIDENTIALITY |
							ISC_RET_EXTENDED_ERROR |
							ISC_REQ_ALLOCATE_MEMORY |
							ISC_REQ_STREAM;
	DWORD outFlags = 0;
	status = ::InitializeSecurityContext (&hCredential, &hContext, nullptr, 
					requestFlags, 0, 0, nullptr, 0, nullptr, &bufferList, &outFlags, nullptr);
	PRINT_STATUS ("InitializeSecurityContext (shutdown)", status)
	if(FAILED (status))
		return kSSLFailed;

	if(buffers[0].pvBuffer)
	{
		output.write (buffers[0].pvBuffer, buffers[0].cbBuffer);
		::FreeContextBuffer (buffers[0].pvBuffer);
	}

	return kSSLSuccess;		
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult SecureChannel::encryptMessage (StreamBuffer& output, const void* _message, int messageSize, int& bytesProcessed)
{
	SecPkgContext_StreamSizes streamSizes = {0};
	SECURITY_STATUS status = ::QueryContextAttributes (&hContext, SECPKG_ATTR_STREAM_SIZES, &streamSizes);
	if(FAILED (status))
		return kSSLFailed;

	int maxMessageSize = streamSizes.cbMaximumMessage;
	if(messageSize > maxMessageSize)
		messageSize = maxMessageSize;

	// allocate contiguous buffer for header + message + trailer
	int maxBufferSize = streamSizes.cbHeader + messageSize + streamSizes.cbTrailer;
	char* headerPointer = (char*)output.reserve (maxBufferSize);
	char* messagePointer = headerPointer + streamSizes.cbHeader;
	char* trailerPointer = messagePointer + messageSize;

	::memcpy (messagePointer, _message, messageSize);

	SecBuffer buffers[4] = {0};
	buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
	buffers[0].cbBuffer = streamSizes.cbHeader;
	buffers[0].pvBuffer = headerPointer;	
	buffers[1].BufferType = SECBUFFER_DATA;
	buffers[1].cbBuffer = messageSize;
	buffers[1].pvBuffer = messagePointer;
	buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
	buffers[2].cbBuffer = streamSizes.cbTrailer;
	buffers[2].pvBuffer = trailerPointer;
	buffers[3].BufferType = SECBUFFER_EMPTY;

	SecBufferList bufferList (buffers, ARRAY_COUNT (buffers));
	status = ::EncryptMessage (&hContext, 0, &bufferList, 0);
	PRINT_STATUS ("EncryptMessage", status)
	#if LOG_SECURITY_BUFFERS
	bufferList.dump ("EncryptMessage buffers:");
	#endif

	if(FAILED (status))
		return kSSLFailed;

	ASSERT (buffers[0].cbBuffer == streamSizes.cbHeader)
	ASSERT (buffers[1].cbBuffer == messageSize)

	// ATTENTION: trailer can be different from maximum reported by stream sizes!
	int encryptedMessageSize = streamSizes.cbHeader + messageSize + buffers[2].cbBuffer;
			
	bytesProcessed = messageSize;
	output.adjustAfterWrite (encryptedMessageSize); // incl. header + trailer
	return kSSLSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult SecureChannel::decryptMessage (StreamBuffer& output, const void* _message, int messageSize, int& bytesProcessed)
{
	void* messagePointer = output.reserve (messageSize);		
	::memcpy (messagePointer, _message, messageSize);
		
	SecBuffer buffers[4] = {0};
	buffers[0].BufferType = SECBUFFER_DATA;
	buffers[0].pvBuffer = messagePointer;
	buffers[0].cbBuffer = messageSize;
	buffers[1].BufferType = SECBUFFER_EMPTY;
	buffers[2].BufferType = SECBUFFER_EMPTY;
	buffers[3].BufferType = SECBUFFER_EMPTY;

	SecBufferList bufferList (buffers, ARRAY_COUNT (buffers));
	SECURITY_STATUS status = ::DecryptMessage (&hContext, &bufferList, 0, nullptr);
	PRINT_STATUS ("DecryptMessage", status)
	#if LOG_SECURITY_BUFFERS
	bufferList.dump ("DecryptMessage buffers:");
	#endif

	if(status == SEC_E_INCOMPLETE_MESSAGE)
	{
		// SSPI didn't consume any of the input data
		return kSSLIncompleteMessage;
	}
	
	if(FAILED (status))
		return kSSLFailed;

	// ATTENTION: decryption shuffles around the buffer list and returns header, data, trailer, and extra buffers!

	SecBuffer* outDataBuffer = bufferList.findBuffer (SECBUFFER_DATA);
	if(outDataBuffer)
	{
		::memmove (messagePointer, outDataBuffer->pvBuffer, outDataBuffer->cbBuffer);
		output.adjustAfterWrite (outDataBuffer->cbBuffer);
	}
	
	int bytesUnprocessed = 0;
	SecBuffer* outExtraBuffer = bufferList.findBuffer (SECBUFFER_EXTRA);
	if(outExtraBuffer)
	{
		bytesUnprocessed = outExtraBuffer->cbBuffer;

		#if DEBUG_LOG
		DebugPrintf ("%d of %d bytes unprocessed by SSPI, processed only %d bytes\n", 
					bytesUnprocessed, messageSize, messageSize - bytesUnprocessed);
		#endif
	}

	bytesProcessed = messageSize - bytesUnprocessed;

	if(status == SEC_I_RENEGOTIATE)
		return kSSLRenegotiate;

	// TODO: how to deal with SEC_I_CONTEXT_EXPIRED???

	return kSSLSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult SecureChannel::decryptMessage (StreamBuffer& output, StreamBuffer& input, int& bytesProcessed)
{
	SSLResult result = decryptMessage (output, input.getData (), input.getBytesFilled (), bytesProcessed);
	if(bytesProcessed > 0)
		input.adjustAfterRead (bytesProcessed);
	return result;		
}

//************************************************************************************************
// Win32SSLContext
//************************************************************************************************

const int Win32SSLContext::kMaxBytesBuffered = 1024 * 64; // max. 64KB buffered in memory

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32SSLContext::Win32SSLContext ()
: ioHandler (nullptr),
  renegotiatePending (false),
  wasIncompleteMessage (false),
  incompleteMessageTag (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32SSLContext::setIOHandler (ISSLContextIOHandler* _ioHandler)
{
	ioHandler = _ioHandler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Win32SSLContext::setPeerName (CStringPtr peerName)
{
	schannel.setPeerName (peerName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult Win32SSLContext::flushReadDirection (int* bytesReadOptional)
{
	static const int kBlockSize = 4096;
	char tempBuffer[kBlockSize];
	
	while(dataFromWire.getBytesFilled () < kMaxBytesBuffered)
	{	
		int bytesRead = 0;			
		SSLResult result = ioHandler->read (tempBuffer, kBlockSize, bytesRead);
		if(result != kSSLSuccess)
			return result;
			
		if(bytesRead == 0)
			break;
				
		dataFromWire.write (tempBuffer, bytesRead);
		dataFromWire.changed ();
			
		if(bytesReadOptional)
			*bytesReadOptional += bytesRead;

		#if DEBUG_LOG
		DebugPrintf ("<== %d bytes read from wire...\n", bytesRead);
		#endif
	}
	return kSSLSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult Win32SSLContext::flushWriteDirection (int* bytesWrittenOptional)
{
	while(!dataToWire.isEmpty ())
	{
		int bytesWritten = 0;
		SSLResult result = ioHandler->write (dataToWire.getData (), dataToWire.getBytesFilled (), bytesWritten);
		if(result != kSSLSuccess)
			return result;
			
		if(bytesWritten == 0)
			break;
				
		dataToWire.adjustAfterRead (bytesWritten);
		//dataToWire.changed (); not used
		
		if(bytesWrittenOptional)
			*bytesWrittenOptional += bytesWritten;

		#if DEBUG_LOG
		DebugPrintf ("==> %d bytes written to wire...\n", bytesWritten);
		#endif
	}
	return kSSLSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult Win32SSLContext::handshake ()
{
	SSLResult result = kSSLWouldBlock; // 'would block' means that handshake must be called again!
	while(1)
	{			
		if(schannel.isInitialized ())
			if(flushReadDirection () == kSSLFailed)
				return kSSLFailed;

		if(isMessageStillIncomplete ())
			break;

		result = schannel.initialize (dataToWire, dataFromWire);
		captureIncompleteMessage (result);
		if(result == kSSLFailed)
			break;
			
		int bytesWritten = 0;
		if(flushWriteDirection (&bytesWritten) == kSSLFailed)
			return kSSLFailed;
			
		if(result == kSSLSuccess)
			break;

		if(bytesWritten == 0 && result == kSSLWouldBlock)
			break;
	}

	if(result == kSSLSuccess)
		renegotiatePending = false;

	#if 0//DEBUG_LOG
	DebugPrint ("return from handshake()\n");
	#endif
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult Win32SSLContext::close ()
{
	// send shutdown token
	if(schannel.shutdown (dataToWire) == kSSLFailed)
		return kSSLFailed;

	return flushWriteDirection ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult Win32SSLContext::write (const void* buffer, int size, int& bytesWritten)
{
	// check if renegotiation pending
	if(renegotiatePending)
	{
		if(handshake () == kSSLFailed)
			return kSSLFailed;
		return kSSLWouldBlock;
	}

	if(dataToWire.getBytesFilled () >= kMaxBytesBuffered)
	{
		SSLResult flushResult = flushWriteDirection ();
		if(flushResult != kSSLSuccess)
			return flushResult;
		return kSSLWouldBlock;
	}

	if(schannel.encryptMessage (dataToWire, buffer, size, bytesWritten) == kSSLFailed)
		return kSSLFailed;

	if(flushWriteDirection () == kSSLFailed)
		return kSSLFailed;
			
	#if 0//DEBUG_LOG
	DebugPrint ("return from write()\n");
	#endif
	return kSSLSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult Win32SSLContext::read (void* buffer, int size, int& bytesRead)
{
	// check if renegotiation pending
	if(renegotiatePending)
	{
		if(handshake () == kSSLFailed)
			return kSSLFailed;
		return kSSLWouldBlock;
	}

	// flush outgoing data
	SSLResult result = flushWriteDirection ();
	if(result != kSSLSuccess)
		return result;
			
	while(1)
	{
		// return already decrypted data
		if(!dataForClient.isEmpty ())
		{
			bytesRead = dataForClient.read (buffer, size);
			break;
		}
		
		// read data from wire
		int bytesReceived = 0;
		if(flushReadDirection (&bytesReceived) == kSSLFailed)
			return kSSLFailed;
				
		// decrypt next message
		int bytesProcessed = 0;
		if(!dataFromWire.isEmpty () && !isMessageStillIncomplete ())
		{
			result = schannel.decryptMessage (dataForClient, dataFromWire, bytesProcessed);
			captureIncompleteMessage (result);
			if(result != kSSLSuccess)
			{
				if(result == kSSLRenegotiate)
				{
					#if DEBUG_LOG
					DebugPrint ("SSL renegotiate pending!\n");
					#endif
					renegotiatePending = true;
					return kSSLSuccess;
				}
				
				return result;
			}
		}
			
		if(bytesProcessed == 0 && bytesReceived == 0)
			break;
	}
		
	return kSSLSuccess;
}
