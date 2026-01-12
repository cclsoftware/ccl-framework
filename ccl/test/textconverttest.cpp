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
// Filename    : textconverttest.cpp
// Description : Unit tests for Text Conversion
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/base/math/mathrange.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/base/idatatransformer.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/logging.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/text/ixmlwriter.h"
#include "ccl/public/text/itextstreamer.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/textservices.h"

using namespace CCL;

//************************************************************************************************
// TextConvertTest
//************************************************************************************************

CCL_TEST (TextConvertTest, TestBaseEncoding)
{
	const CString kTestString ("This is the Base Encoding test string.");

	Logging::debug (kTestString.str ());

	for(int run = 1; run <= 3; run++)
	{
		UID cid;
		switch(run)
		{
		case 1 : Logging::debug ("### Base 16 ###"); cid = ClassID::Base16Encoding; break;
		case 2 : Logging::debug ("### Base 32 ###"); cid = ClassID::Base32Encoding; break;
		case 3 : Logging::debug ("### Base 64 ###"); cid = ClassID::Base64Encoding; break;
		}

		// 1) Encoding
		AutoPtr<IDataTransformer> encoder;
		CCL_TEST_ASSERT ((encoder = System::CreateDataTransformer (cid, IDataTransformer::kEncode)) != nullptr);
		if(encoder == nullptr)
			break;

		MemoryStream encodedStream;
		AutoPtr<IStream> transformStream;
		CCL_TEST_ASSERT ((transformStream = System::CreateTransformStream (&encodedStream, encoder, true)) != nullptr);
		if(transformStream == nullptr)
			break;

		CCL_TEST_ASSERT (transformStream->write (kTestString, kTestString.length ()) == kTestString.length ());
		transformStream.release (); // force flush

		int baseEncodedLength = encodedStream.getBytesWritten ();
		char null = 0;
		encodedStream.write (&null, 1); // null terminator
		const char* baseEncodedData = (const char*)encodedStream.getMemoryAddress ();
		Logging::debug (baseEncodedData);

		// 2) Decoding
		encodedStream.rewind ();
		encodedStream.setBytesWritten (encodedStream.getBytesWritten () - 1);

		AutoPtr<IDataTransformer> decoder;
		CCL_TEST_ASSERT ((decoder = System::CreateDataTransformer (cid, IDataTransformer::kDecode)) != nullptr);
		if(decoder == nullptr)
			break;

		CCL_TEST_ASSERT ((transformStream = System::CreateTransformStream (&encodedStream, decoder, false)) != nullptr);
		if(transformStream == nullptr)
			break;

		MemoryStream decodedStream;
		Buffer buffer (256);
		while(1)
		{
			int numRead = transformStream->read (buffer, (int)buffer.getSize ());
			if(numRead <= 0)
				break;

			decodedStream.write (buffer, numRead);
		}

		transformStream.release ();

		int decodedLength = decodedStream.getBytesWritten ();
		decodedStream.write (&null, 1); // null terminator
		const char* decodedData = (const char*)decodedStream.getMemoryAddress ();

		CCL_TEST_ASSERT (kTestString.length () == decodedLength);
		CCL_TEST_ASSERT (kTestString == decodedData);
		Logging::debug (decodedData);
	}	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (TextConvertTest, TestXmlEncoding)
{
	AutoPtr<IXmlWriter> writer;
	CCL_TEST_ASSERT ((writer = System::CreateXmlWriter ()) != nullptr);
	if(writer == nullptr)
		return;

	const String kTestString ("&copy;&reg;&trade;Company 2008 &lt;---&gt;");//&#8364;
	Logging::debug ("Test String: %(1)", kTestString);
	
	String decodedString;
	CCL_TEST_ASSERT (writer->decode (decodedString, kTestString) == kResultOk);
	Logging::debug ("Decoded: %(1)", decodedString);

	String encodedString;
	CCL_TEST_ASSERT (writer->encode (encodedString, decodedString) == kResultOk);
	Logging::debug ("Encoded: %(1)", encodedString);

	CCL_TEST_ASSERT (encodedString == kTestString);

	String decodedString2;
	CCL_TEST_ASSERT (writer->decode (decodedString2, encodedString) == kResultOk);
	Logging::debug ("Decoded 2: %(1)", decodedString2);

	CCL_TEST_ASSERT (decodedString2 == decodedString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (TextConvertTest, TestXmlDecodingAmpersand)
{
	AutoPtr<IXmlWriter> writer;
	CCL_TEST_ASSERT ((writer = System::CreateXmlWriter ()) != nullptr);
	if(writer == nullptr)
		return;

	// Cover non-escaped w/o whitespaces, encoded by name or number, combinations of both, malformed encodings.
	const String kTestString ("A & B, A&B, A& B, A &B, A&amp;B, A&#38;B, &&amp;&#38;, A&ampB, A&##38;B, A&#38B");
	const String kExpectedString ("A & B, A&B, A& B, A &B, A&B, A&B, &&&, A&ampB, A&##38;B, A&#38B");

	String decodedString;
	CCL_TEST_ASSERT (writer->decode (decodedString, kTestString) == kResultOk);

	Logging::debug ("Input: %(1)", kTestString);
	Logging::debug ("Expected: %(1)", kExpectedString);
	Logging::debug ("Decoded: %(1)", decodedString);

	CCL_TEST_ASSERT_EQUAL (decodedString, kExpectedString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST (TextConvertTest, TestTextStreamer)
{
	Url outPath;
	System::GetSystem ().getLocation (outPath, System::kDesktopFolder);
	outPath.descend ("textstreamer.txt");

	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (outPath, IStream::kCreateMode);
	CCL_TEST_ASSERT (stream != nullptr);
	if(stream == nullptr)
		return;

	AutoPtr<ITextStreamer> streamer = System::CreateTextStreamer (*stream, {Text::kUTF8, Text::kSystemLineFormat});
	CCL_TEST_ASSERT (streamer != nullptr);
	if(streamer == nullptr)
		return;

	auto testString = [&] (uchar sequence[])
	{
		String string;
		string.append (sequence);

		tbool written = streamer->writeString (string, false);
		CCL_TEST_ASSERT (written);
		return written;
	};

	// In UTF-16, non-BMP characters (range U+10000-U+10FFFF) are stored as "surrogate pairs", two 16 bits units:
	// a high surrogate (in range U+D800-U+DBFF) followed by a low surrogate (in range U+DC00-U+DFFF)
	constexpr Math::Range<uchar> highSurrogates (0xD800, 0xDBFF);
	constexpr Math::Range<uchar> lowSurrogates  (0xDC00, 0xDFFF);
	const Math::Range<uchar> surrogateChars (Math::Range<uchar> (lowSurrogates).join (highSurrogates));

	uchar start = 1;
	uchar end = 0xFFFF;
	for(int i = start; i <= end; i++)
	{
		uchar c = (uchar)i;

		if(surrogateChars.isInsideClosed (c)) // building a string with only one "lone surrogate" character is invalid in UTF-16 (surrogate pairs are tested below)
			continue;

		uchar temp[2] = {c, 0};

		if(!testString (temp))
			break;
	}

	// test all possible surrogate pairs
	for(uchar h = highSurrogates.start; h <= highSurrogates.end; h++)
		for(uchar l = lowSurrogates.start; l <= lowSurrogates.end; l++)
		{
			uchar high = (uchar)h;
			uchar low = (uchar)l;
			uchar temp[3] = {high, low, 0};

			if(!testString (temp))
				break;
		}

	Logging::debug ("Done %(1)", ":-)");
}
