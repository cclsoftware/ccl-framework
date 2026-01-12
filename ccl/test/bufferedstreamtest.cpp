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
// Filename    : bufferedstreamtest.cpp
// Description : Unit test for BufferedStream
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/logging.h"
#include "ccl/public/text/cstring.h"
#include "ccl/base/initterm.h"

using namespace CCL;

namespace
{
	const char* originalText = "0123456789abcdefghijklmnopqrstuvwxyz";
}

#define FOR_BUFFER_PARAMS \
	for(int bufferSize = 0; bufferSize<=8; bufferSize++) \
		for(int portionSize = 1; portionSize<=bufferSize*2+2; portionSize++)

//************************************************************************************************
// BufferedStreamTest
//************************************************************************************************

class BufferedStreamTest: public Test
{
protected:
	void readChars (MutableCString& resultText, IStream* stream, int readSize, int numChars = 0xffffff)
	{
		Buffer buffer (readSize);
		int totalRead = 0;
		int bytesRead = 0;
		do
		{
			int toRead = ccl_min (readSize, numChars - totalRead);
			bytesRead = stream->read (buffer, toRead);
			CCL_TEST_ASSERT (bytesRead >= 0);
			CCL_TEST_ASSERT (bytesRead <= readSize);

			if(bytesRead > 0)
				resultText.append (buffer.as<char> (), bytesRead);
			totalRead += bytesRead;

		}while (bytesRead > 0 && totalRead < numChars);
	}
	
	void writeChars (CStringRef sourceText, IStream* stream, int& pos, int writeSize, int numChars = 0xffffff)
	{
		Buffer buffer (writeSize);
		int totalWritten = 0;
		int bytesWritten = 0;
		do
		{
			int toWrite = ccl_min (writeSize, numChars - totalWritten);
			bytesWritten = stream->write (sourceText + pos, toWrite);
			CCL_TEST_ASSERT (bytesWritten >= 0);
			CCL_TEST_ASSERT (bytesWritten <= writeSize);

			totalWritten += bytesWritten;
			pos += bytesWritten;
		}while (bytesWritten > 0 && totalWritten < numChars);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (BufferedStreamTest, TestRead)
{
	Logging::debug ("Read from BufferedStream:");
	FOR_BUFFER_PARAMS
	{
		AutoPtr<CCL::IStream> sourceStream (System::GetFileUtilities ().createStringStream (originalText, Text::kASCII));
		CCL_TEST_ASSERT (sourceStream != nullptr);
		AutoPtr<CCL::IStream> bufferedStream (System::GetFileUtilities ().createBufferedStream (*sourceStream, bufferSize));
		CCL_TEST_ASSERT (bufferedStream != nullptr);

		MutableCString resultText;
		readChars (resultText, bufferedStream, portionSize);

		char msg[4096];
		snprintf (msg, 2096, "  bufferSize %d, portionSize %d:  %s", bufferSize, portionSize, resultText.str ());
		Logging::debug (msg);
		CCL_TEST_ASSERT (resultText == originalText);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (BufferedStreamTest, TestReadSeek)
{
	Logging::debug ("Read and seek:");
	FOR_BUFFER_PARAMS
	{
		AutoPtr<CCL::IStream> sourceStream (System::GetFileUtilities ().createStringStream (originalText, Text::kASCII));
		CCL_TEST_ASSERT (sourceStream != nullptr);
		AutoPtr<CCL::IStream> bufferedStream (System::GetFileUtilities ().createBufferedStream (*sourceStream, bufferSize));
		CCL_TEST_ASSERT (bufferedStream != nullptr);

		MutableCString resultText;

		Buffer buffer (portionSize);
		readChars (resultText, bufferedStream, portionSize, 4);
		resultText.append (" ");

		bufferedStream->seek (2, IStream::kSeekCur);
		readChars (resultText, bufferedStream, portionSize, 4);
		resultText.append (" ");

		bufferedStream->seek (2, IStream::kSeekCur);
		readChars (resultText, bufferedStream, portionSize, 4);
		resultText.append (" ");

		bufferedStream->seek (2, IStream::kSeekSet);
		readChars (resultText, bufferedStream, portionSize, 4);

		CCL_TEST_ASSERT (resultText == "0123 6789 cdef 2345");

		char msg[4096];
		snprintf (msg, 2096, "  bufferSize %d, portionSize %d:  %s", bufferSize, portionSize, resultText.str ());
		Logging::debug (msg);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (BufferedStreamTest, TestWrite)
{
	Logging::debug ("Write through BufferedStream:");
	FOR_BUFFER_PARAMS
	{
		AutoPtr<MemoryStream> targetStream (NEW MemoryStream);
		CCL_TEST_ASSERT (targetStream != nullptr);
		AutoPtr<CCL::IStream> bufferedStream (System::GetFileUtilities ().createBufferedStream (*targetStream, bufferSize));
		CCL_TEST_ASSERT (bufferedStream != nullptr);

		int pos = 0;
		writeChars (originalText, bufferedStream, pos, portionSize, (int)strlen (originalText) + 1);

		bufferedStream.release (); // to force flush...
		CString resultStr ((const char*)targetStream->getMemoryAddress ());

		char msg[4096];
		snprintf (msg, 2096, "  bufferSize %d, portionSize %d:  %s", bufferSize, portionSize, resultStr.str ());
		Logging::debug (msg);
		CCL_TEST_ASSERT (resultStr == originalText);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (BufferedStreamTest, TestWriteSeek)
{
	Logging::debug ("Write and seek:");
	FOR_BUFFER_PARAMS
	{
		AutoPtr<MemoryStream> targetStream (NEW MemoryStream);
		CCL_TEST_ASSERT (targetStream != nullptr);
		AutoPtr<CCL::IStream> bufferedStream (System::GetFileUtilities ().createBufferedStream (*targetStream, bufferSize));
		CCL_TEST_ASSERT (bufferedStream != nullptr);

		int pos = 0;

		// write 0-9
		writeChars (originalText, bufferedStream, pos, portionSize, 10);

		// back to 5
		bufferedStream->seek (-5, IStream::kSeekCur);

		// write a-z
		writeChars (originalText, bufferedStream, pos, portionSize, 27);

		bufferedStream.release (); // to force flush...
		CString resultStr ((const char*)targetStream->getMemoryAddress ());

		char msg[4096];
		snprintf (msg, 2096, "  bufferSize %d, portionSize %d:  %s", bufferSize, portionSize, resultStr.str ());
		Logging::debug (msg);
		CCL_TEST_ASSERT (resultStr == "01234abcdefghijklmnopqrstuvwxyz");
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (BufferedStreamTest, TestReadWrite)
{
	Logging::debug ("mixed read and write:");
	FOR_BUFFER_PARAMS
	{
		AutoPtr<MemoryStream> targetStream (NEW MemoryStream);
		CCL_TEST_ASSERT (targetStream != nullptr);
		AutoPtr<CCL::IStream> bufferedStream (System::GetFileUtilities ().createBufferedStream (*targetStream, bufferSize));
		CCL_TEST_ASSERT (bufferedStream != nullptr);

		MutableCString resultText;

		// write originalText and seek back
		int pos = 0;
		writeChars (originalText, bufferedStream, pos, portionSize, (int)strlen (originalText) + 1);
		bufferedStream->seek (0, IStream::kSeekSet);

		// read 10 chars
		readChars (resultText, bufferedStream, portionSize, 10);

		// write 5 upper letters
		pos = 0;
		writeChars ("ABCDE", bufferedStream, pos, portionSize, 5);

		// read 5 chars
		readChars (resultText, bufferedStream, portionSize, 5);

		// write 5 upper letters
		pos = 0;
		writeChars ("KLMNO", bufferedStream, pos, portionSize, 5);

		bufferedStream.release (); // to force flush...
		CString resultStr ((const char*)targetStream->getMemoryAddress ());

		char msg[4096];
		snprintf (msg, 2096, "  bufferSize %d, portionSize %d:  %s", bufferSize, portionSize, resultStr.str ());
		Logging::debug (msg);
		CCL_TEST_ASSERT (resultStr == "0123456789ABCDEfghijKLMNOpqrstuvwxyz");
	}
}
