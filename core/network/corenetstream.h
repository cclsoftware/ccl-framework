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
// Filename    : core/network/corenetstream.h
// Description : Network stream class
//
//************************************************************************************************

#ifndef _corenetstream_h
#define _corenetstream_h

#include "core/network/coresocket.h"
#include "core/public/corestream.h"

namespace Core {
namespace Sockets {

//************************************************************************************************
// NetworkStream
//************************************************************************************************

class NetworkStream: public IO::Stream
{
public:
	NetworkStream (Socket& socket)
	: socket (socket),
	  byteCount (0)
	{}

	// IO::Stream
	int64 getPosition () override
	{
		return byteCount;
	}

	int64 setPosition (int64 pos, int mode) override
	{
		ASSERT (0)
		return -1;
	}

	int readBytes (void* buffer, int size) override
	{
		int bytesRead = 0;
		while(bytesRead < size)
		{
			int result = socket.receive ((char*)buffer + bytesRead, size - bytesRead);
			if(result > 0)
			{
				bytesRead += result;
				this->byteCount += result;
			}
			else
				return bytesRead;
		}
		return bytesRead;
	}

	int writeBytes (const void* buffer, int size) override
	{
		int bytesWritten = 0;
		while(bytesWritten < size)
		{
			int result = socket.send ((char*)buffer + bytesWritten, size - bytesWritten);
			if(result > 0)
			{
				bytesWritten += result;
				byteCount += result;
			}
			else
				return bytesWritten;
		}
		return bytesWritten;
	}

protected:
	Socket& socket;
	int64 byteCount;
};

} // namespace Sockets
} // namespace Core

#endif // _coresocket_h
