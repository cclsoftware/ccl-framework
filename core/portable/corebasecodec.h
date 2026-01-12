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
// Filename    : core/portable/corebasecodec.h
// Description : Base Encoding
//
//************************************************************************************************

#ifndef _corebasecodec_h
#define _corebasecodec_h

#include "core/public/corememstream.h"

//************************************************************************************************
// Include our modified version of libb64
// LATER TODO: refactor and use Base-16/32/64 encoding classes from CCL!
#include "core/portable/libb64.h"
//************************************************************************************************

namespace Core {
namespace Portable {

//************************************************************************************************
// Base64Decoder
//************************************************************************************************

class Base64Decoder
{
public:
	int decodeBlock (char* outputBuffer, int outputBufferSize, const char inputBuffer[], int inputLength)
	{
		ASSERT (outputBufferSize >= inputLength)
		if(outputBufferSize < inputLength)
			return 0;
		return libb64::base64_decode_block (inputBuffer, inputLength, outputBuffer, &state);
	}

	bool decodeStream (IO::MemoryStream& outStream, const IO::MemoryStream& inStream)
	{
		const char* inputBuffer = inStream.getBuffer ().as<char> ();
		uint32 inputLength = inStream.getBytesWritten ();
		uint32 outputBufferSize = inputLength;
		if(!outStream.allocateMemory (outputBufferSize))
			return false;

		char* outputBuffer = const_cast<char*> (outStream.getBuffer ().as<char> ());
		int bytesDecoded = decodeBlock (outputBuffer, static_cast<int> (outputBufferSize), inputBuffer, static_cast<int> (inputLength));
		if(bytesDecoded > 0)
			outStream.setBytesWritten (bytesDecoded);
		return true;
	}

private:
	libb64::base64_decodestate state;
};

//************************************************************************************************
// Base64URLDecoder
//************************************************************************************************

class Base64URLDecoder
{
public:
	bool decodeStream (IO::MemoryStream& outStream, const IO::MemoryStream& _inStream)
	{
		// create mutable copy of input stream here :-/
		IO::MemoryStream inStream;
		inStream.writeBytes (_inStream.getBuffer ().getAddress (), _inStream.getBytesWritten ());
		
		return decodeMutableInputStream (outStream, inStream);
	}

	bool decodeBuffer (IO::MemoryStream& outStream, const IO::Buffer& inBuffer)
	{
		IO::MemoryStream inStream;
		inStream.writeBytes (inBuffer.getAddress (), inBuffer.getSize ());
		
		return decodeMutableInputStream (outStream, inStream);
	}

	bool decodeMutableInputStream (IO::MemoryStream& outStream, IO::MemoryStream& inStream)
	{
		// https://tools.ietf.org/html/rfc4648#section-5
		// https://tools.ietf.org/html/rfc7515#appendix-C

		uint32 inputLength = inStream.getBytesWritten ();
		switch(inputLength % 4) // Pad with trailing '='s
		{
		case 0: break; // No pad chars in this case
		case 2: inStream.writeBytes ("==", 2); break; // Two pad chars
		case 3: inStream.writeBytes ("=", 1); break; // One pad char
		default : /* Illegal base64url string! */ break;
		}

		char* inputBuffer = const_cast<char*> (inStream.getBuffer ().as<char> ());
		for(uint32 i = 0; i < inputLength; i++)
			switch(inputBuffer[i])
			{
			case '-' : inputBuffer[i] = '+'; break; // 62nd char of encoding
			case '_' : inputBuffer[i] = '/'; break; // 63rd char of encoding
			}

		return base64decoder.decodeStream (outStream, inStream);
	}

private:
	Base64Decoder base64decoder;
};

} // namespace Portable
} // namespace Core

#endif // _corebasecodec_h
