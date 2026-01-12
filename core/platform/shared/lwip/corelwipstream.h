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
// Filename    : core/platform/shared/lwip/corelwipstream.h
// Description : Network stream class using LwIP
//
//************************************************************************************************

#ifndef _corelwipstream_h
#define _corelwipstream_h

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "core/public/corestream.h"

namespace Core {
namespace Sockets {

//************************************************************************************************
// CoreLwipStream
//************************************************************************************************

class CoreLWIPStream : public IO::Stream
{
public:
	CoreLWIPStream (netconn* connection)
	: connection (connection),
	  netBuffer (0),
	  pointer (0),
	  bytesLeft (0),
	  totalBytesRead(0)
	{}

	~CoreLWIPStream ()
	{
		if(netBuffer != 0)
		{
			netbuf_delete (netBuffer);
			netBuffer = 0;
		}
	}

	int64 getPosition () override
	{
		return totalBytesRead;
	}

	int64 setPosition (int64 pos, int mode) override
	{
		return 0; // not implemented
	}

	int readBytes (void* buffer, int size) override
	{
		int bytesToRead = size;

		while(bytesToRead > 0)
		{
			if(bytesLeft == 0)
			{
				if(netBuffer != 0)
				{
					netbuf_delete (netBuffer);
					netBuffer = 0;
				}

				if(netconn_recv (connection, &netBuffer) != ERR_OK)
					return 0;

				netbuf_data (netBuffer, reinterpret_cast<void**>(&pointer), &bytesLeft);
			}

			if(bytesToRead > bytesLeft)
			{
				memcpy (buffer, pointer, bytesLeft);
				buffer = reinterpret_cast<u8_t*>(buffer) + bytesLeft;
				bytesToRead -= bytesLeft;
				bytesLeft = 0;
			}
			else
			{
				memcpy (buffer, pointer, bytesToRead);
				bytesLeft -= bytesToRead;
				pointer += bytesToRead;
				bytesToRead = 0;
			}
		}

		totalBytesRead += size;
		return size;
	}

	int writeBytes (const void* buffer, int size) override
	{
		size_t bytesToWrite = size;

		while(bytesToWrite > 0)
		{
			size_t bytesWritten;
			if(netconn_write_partly (connection, buffer, bytesToWrite, NETCONN_COPY, &bytesWritten) != ERR_OK)
				return 0;
            buffer = reinterpret_cast<const u8_t*>(buffer) + bytesWritten;
			bytesToWrite -= bytesWritten;
		}

		return size;
	}

private:
	netconn* connection;
	netbuf* netBuffer;
	u8_t* pointer;
	u16_t bytesLeft;
	int64_t totalBytesRead;
};

} // namespace Sockets
} // namespace Core

#endif // _corelwipstream_h
