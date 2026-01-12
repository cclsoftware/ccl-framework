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
// Filename    : core/platform/cocoa/coresslcontext.cocoa.cpp
// Description : SSL session based on Apple Secure Transport API
//
//************************************************************************************************

#define DEBUG_SSL 0

#include "coresslcontext.cocoa.h"

#include "core/network/corenetwork.h"
#include "core/system/coredebug.h"

/*
 https://developer.apple.com/library/mac/documentation/Security/Reference/secureTransportRef/Reference/reference.html#//apple_ref/doc/uid/TP30000155
 */

using namespace Core;
using namespace Sockets;
using namespace Platform;

//************************************************************************************************
// CocoaSSLContext
//************************************************************************************************

CocoaSSLContext::CocoaSSLContext ()
: context (nullptr),
  ioHandler (nullptr)
{
	context = SSLCreateContext (NULL, kSSLClientSide, kSSLStreamType);
	if(context)
		SSLSetIOFuncs (context, readFunction, writeFunction);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaSSLContext::~CocoaSSLContext ()
{
	if(context)
		CFRelease (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaSSLContext::setIOHandler (ISSLContextIOHandler* _ioHandler)
{
	ioHandler = _ioHandler;
	OSStatus status = SSLSetConnection (context, this);
	ASSERT (status == noErr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaSSLContext::setPeerName (CStringPtr peerName)
{
	OSStatus status = SSLSetPeerDomainName (context, peerName, strnlen (peerName, 1024)); 
	ASSERT (status == noErr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult CocoaSSLContext::handshake ()
{
	OSStatus status = SSLHandshake (context);
	return sslResultFromStatus (status);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult CocoaSSLContext::close ()
{
	OSStatus status = SSLClose (context);
	return sslResultFromStatus (status);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult CocoaSSLContext::write (const void* buffer, int size, int& bytesWritten)
{
	size_t writeCount = 0;
	
	#if DEBUG_SSL
	SSLSessionState state;
	SSLGetSessionState (context, &state);
	if(state != kSSLConnected)
		DebugPrintf ("CocoaSSLContext::write SSLSessionState=%d\n", state);	
	#endif
	
	OSStatus status = SSLWrite (context, buffer, size, &writeCount);
	if(writeCount > 0)
		status = noErr;	
	bytesWritten = (int)writeCount;
	return sslResultFromStatus (status);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult CocoaSSLContext::read (void* buffer, int size, int& bytesRead)
{
	size_t readCount = 0;
	
	#if DEBUG_SSL
	SSLSessionState state;
	SSLGetSessionState (context, &state);
	if(state != kSSLConnected)
		DebugPrintf ("CocoaSSLContext::read SSLSessionState=%d\n", state);
	#endif
	
	OSStatus status = SSLRead (context, buffer, size, &readCount);
	if(readCount > 0)
		status = noErr;
	bytesRead = (int)readCount;
	return sslResultFromStatus (status);	
}	

//////////////////////////////////////////////////////////////////////////////////////////////////

OSStatus CocoaSSLContext::readFunction (SSLConnectionRef connection, void* data, size_t* dataLength)
{
	CocoaSSLContext* context = (CocoaSSLContext*)connection;
	if(context == nullptr)
		return SOCKET_ERROR;
	if(context->ioHandler == nullptr)
		return SOCKET_ERROR;
	
	int bytesRead = 0;
	SSLResult result = context->ioHandler->read (data, (int)*dataLength, bytesRead);
	
	#if DEBUG_SSL
	if(result != kSSLSuccess && result != kSSLWouldBlock)
		DebugPrintf ("CocoaSSLContext::readFunction result=%d\n", result);	
	#endif
	
	if(result == kSSLSuccess && static_cast<size_t> (bytesRead) < *dataLength)
	{
		*dataLength = static_cast<size_t> (bytesRead);
		return errSSLWouldBlock;
	}
	
	*dataLength = bytesRead;
	return statusFromSSLResult (result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

OSStatus CocoaSSLContext::writeFunction (SSLConnectionRef connection, const void* data, size_t* dataLength)
{
	CocoaSSLContext* context = (CocoaSSLContext*)connection;
	if(context == nullptr)
		return SOCKET_ERROR;
	if(context->ioHandler == nullptr)
		return SOCKET_ERROR;
	
	int bytesWritten = 0;
	SSLResult result = context->ioHandler->write (data, (int)*dataLength, bytesWritten);
	
	#if DEBUG_SSL
	if(result != kSSLSuccess && result != kSSLWouldBlock)
		DebugPrintf ("CocoaSSLContext::writeFunction result=%d\n", result);
	#endif		

	if(result == kSSLSuccess && static_cast<size_t> (bytesWritten) < *dataLength)
	{
		*dataLength = bytesWritten;
		return errSSLWouldBlock;
	}
	
	*dataLength = bytesWritten;
	return statusFromSSLResult (result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

OSStatus CocoaSSLContext::statusFromSSLResult (SSLResult result)
{
	switch(result)
	{
	case kSSLSuccess :
		return noErr;
	case kSSLWouldBlock :
		return errSSLWouldBlock;
	case kSSLFailed :
		return SOCKET_ERROR;
	default :
		return SOCKET_ERROR;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SSLResult CocoaSSLContext::sslResultFromStatus (OSStatus status)
{
	switch(status)
	{
	case noErr :
		return kSSLSuccess;
	case errSSLWouldBlock :
		return kSSLWouldBlock;
	default :
		return kSSLFailed;
	}
}
	
