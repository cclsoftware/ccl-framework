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
// Filename    : ccl/network/netstream.cpp
// Description : Network Stream
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/network/netstream.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Net;

#define kDefaultTimeout 60000 // 1 minute

//************************************************************************************************
// NetworkStream
//************************************************************************************************

DEFINE_CLASS_HIDDEN (NetworkStream, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

NetworkStream::NetworkStream (ISocket* socket)
: socket (socket),
  byteCount (0),
  pseudoBlocking (false),
  timeout (kDefaultTimeout),
  cancelCallback (nullptr)
{
	if(socket)
		socket->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NetworkStream::~NetworkStream ()
{
	if(socket)
	{
		if(pseudoBlocking)
			socket->setOption (SocketOption::kNonBlocking, false);

		if(socket->isConnected ())
			socket->disconnect ();

		socket->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISocket* CCL_API NetworkStream::getSocket ()
{
	return socket;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NetworkStream::setPseudoBlocking (tbool state)
{
	ASSERT (socket != nullptr)
	if(socket)
		socket->setOption (SocketOption::kNonBlocking, state);

	pseudoBlocking = state != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NetworkStream::setTimeout (int timeout)
{
	this->timeout = timeout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API NetworkStream::setCancelCallback (IProgressNotify* callback)
{
	cancelCallback = callback;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API NetworkStream::read (void* buffer, int size)
{
	ASSERT (socket != nullptr)
	if(!socket)
		return -1;
	if(size <= 0)
		return 0;

	int bytesRead = 0;
	int64 timeToCancel = 0;
	while(bytesRead < size)
	{
		int result = socket->receive ((char*)buffer + bytesRead, size - bytesRead);
		if(result > 0)
		{
			bytesRead += result;
			this->byteCount += result;
		}
		else
		{
			// check for error
			if(!(pseudoBlocking && socket->wouldBlockOperation (false)))
				if(result < 0)
					return result;

			// check for cancelation
			if(cancelCallback && cancelCallback->isCanceled ())
			{
				bytesRead = -1; // signal error
				CCL_PRINTLN ("Network stream fast cancelation")
				break;
			}

			// check for timeout
			if(timeout > 0)
			{
				if(timeToCancel == 0)
					timeToCancel = System::GetSystemTicks () + timeout;
				else if(System::GetSystemTicks () > timeToCancel)
				{
					CCL_PRINTLN ("Network stream timed out!")
					bytesRead = -1; // signal error
					break;
				}
			}

			System::ThreadSleep (1);
		}
	}
	return bytesRead;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API NetworkStream::write (const void* buffer, int size)
{
	ASSERT (socket != nullptr)
	if(!socket)
		return -1;
	if(size <= 0)
		return 0;

	int bytesWritten = 0;
	int64 timeToCancel = 0;
	while(bytesWritten < size)
	{
		int result = socket->send ((char*)buffer + bytesWritten, size - bytesWritten);
		if(result > 0)
		{
			bytesWritten += result;
			byteCount += result;
		}
		else
		{
			// check for error
			if(!(pseudoBlocking && socket->wouldBlockOperation (true)))
				if(result < 0)
					return result;

			// check for cancelation
			if(cancelCallback && cancelCallback->isCanceled ())
			{
				bytesWritten = -1; // signal error
				CCL_PRINTLN ("Network stream fast cancelation")
				break;
			}

			// check for timeout
			if(timeout > 0)
			{
				if(timeToCancel == 0)
					timeToCancel = System::GetSystemTicks () + timeout;
				else if(System::GetSystemTicks () > timeToCancel)
				{
					CCL_PRINTLN ("Network stream timed out!")
					bytesWritten = -1; // signal error
					break;
				}
			}

			System::ThreadSleep (1);
		}
	}
	return bytesWritten;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NetworkStream::isSeekable () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API NetworkStream::seek (int64 pos, int mode)
{
	ASSERT (0)
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 CCL_API NetworkStream::tell ()
{
	return byteCount;
}
