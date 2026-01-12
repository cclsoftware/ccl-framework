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
// Filename    : ccl/text/transform/textstreamer.cpp
// Description : Text Streamer
//
//************************************************************************************************

#include "ccl/text/transform/textstreamer.h"
#include "ccl/text/transform/transformstreams.h"

#include "ccl/text/transform/encodings/cstringencoding.h"
#include "ccl/text/transform/encodings/utfencoding.h"

#include "ccl/public/text/cclstring.h"

#include "core/text/coreutfcodec.h"

namespace CCL {

enum ControlCharacters
{
	kCarriageReturn = 0x0D,
	kLineFeed = 0x0A
};

// byte order marks
const unsigned char kBomUTF32BigEndian[]	= {0x00, 0x00, 0xFE, 0xFF};
const unsigned char kBomUTF32LittleEndian[]	= {0xFF, 0xFE, 0x00, 0x00};
const unsigned char kBomUTF16BigEndian[]	= {0xFE, 0xFF};
const unsigned char kBomUTF16LittleEndian[] = {0xFF, 0xFE};
const unsigned char kBomUTF8[]				= {0xEF, 0xBB, 0xBF};

//************************************************************************************************
// TextStreamer
//************************************************************************************************

TextStreamer::TextStreamer (IStream& stream, TextEncoding encoding, TextLineFormat format, int options)
: encoding (encoding),
  format (format),
  targetStream (nullptr),
  readStream (nullptr),
  writeStream (nullptr),
  decoderStream (nullptr),
  encoderStream (nullptr),
  nextChar (0),
  buffered (0),
  endOfStream (false),
  writeByteOrder ((options & kSuppressByteOrderMark) == 0),
  flushNewline ((options & kFlushLineEnd) != 0)
{
	take_shared (targetStream, &stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextStreamer::~TextStreamer ()
{
	flush ();

	safe_release (targetStream);
	safe_release (decoderStream);
	safe_release (encoderStream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextStreamer::flush ()
{
	if(buffered > 0)
	{
		int numBytes = buffered * sizeof(charBuffer[0]);
		writeStream->write (charBuffer, numBytes);
		buffered = 0;

		if(UnknownPtr<ITransformStream> transformWriter = writeStream)
			transformWriter->flush ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IStream& TextStreamer::getStream ()
{
	return *targetStream;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDataTransformer* TextStreamer::createEncoder ()
{
	switch(encoding)
	{
	case Text::kASCII     : return NEW ASCIIEncoder;
	case Text::kISOLatin1 :	return NEW Latin1Encoder;
	case Text::kUTF8      :	return NEW UTF8Encoder;
	case Text::kUTF16LE   :	return NEW UTF16Encoder (kLittleEndian);
	case Text::kUTF16BE   :	return NEW UTF16Encoder (kBigEndian);
	}

	CCL_DEBUGGER ("Unknown Text Encoding!!!")
	return NEW ASCIIEncoder; // fall back to ASCII
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDataTransformer* TextStreamer::createDecoder ()
{
	switch(encoding)
	{
	case Text::kASCII      : return NEW ASCIIDecoder;
	case Text::kISOLatin1  : return NEW Latin1Decoder;
	case Text::kUTF8       : return NEW UTF8Decoder;
	case Text::kUTF16LE    : return NEW UTF16Decoder (kLittleEndian);
	case Text::kUTF16BE    : return NEW UTF16Decoder (kBigEndian);
	}

	CCL_DEBUGGER ("Unknown Text Encoding!!!")
	return NEW ASCIIDecoder; // fall back to ASCII
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextStreamer::prepareReadStream ()
{
	if(!readStream && targetStream)
	{
		unsigned char buffer[3];
		int bytesRead = 0;
		int bytesUsed = 0;

		if(1)//was: if(encoding == Text::kUnknownEncoding)
		{
			bytesRead = targetStream->read (buffer, sizeof(buffer));
			if(bytesRead >= 2)
			{
				if(buffer[0] == 0xFE && buffer[1] == 0xFF)
				{
					encoding = Text::kUTF16BE;
					bytesUsed = 2;
				}
				else if(buffer[0] == 0xFF && buffer[1] == 0xFE)
				{
					encoding = Text::kUTF16LE;
					bytesUsed = 2;
				}
				else if(bytesRead > 2 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF)
				{
					encoding = Text::kUTF8;
					bytesUsed = 3;
				}

				// TODO: try to detect encoding from first few bytes!
			}

			if(encoding == Text::kUnknownEncoding)
				encoding = Text::kUTF8; // default, if no BOM found
		}

		AutoPtr<IDataTransformer> decoder (createDecoder ());
		if(decoder)
		{
			readStream = decoderStream = NEW TransformReader ();
			decoderStream->open (decoder, targetStream);

			// feed the unused data we already read
			if(bytesUsed < bytesRead)
				decoderStream->preloadSourceData (buffer + bytesUsed, bytesRead - bytesUsed);
		}
		else
			readStream = targetStream;

		uchar c;
		readChar (c);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextStreamer::prepareWriteStream ()
{
	if(!writeStream)
	{
		AutoPtr<IDataTransformer> encoder (createEncoder ());
		if(encoder)
		{
			writeStream = encoderStream = NEW TransformWriter ();
			encoderStream->open (encoder, targetStream);
		}
		else
			writeStream = targetStream;

		if(isWriteByteOrder ())
			writeBOM ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextStreamer::readChar (uchar& c)
{
	if(endOfStream)
		return false;

	prepareReadStream ();

	if(readStream == nullptr)
		return false;

	c = nextChar; // read 1 char ahead

	uchar32 uc;
	int bytesRead = readStream->read (&uc, sizeof(uc));
	if(bytesRead == sizeof(uchar32))
		nextChar = (uchar)uc;
	else
		endOfStream = true;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextStreamer::readLine (String& string)
{
	StringWriter<512> stringWriter (string, true);

	// read chars until end of line
	bool result = false;
	uchar c;
	while(readChar (c))
	{
		result = true;
		if(c == kCarriageReturn)
		{
			if(nextChar == kLineFeed)
			{
				readChar (c);
				format = Text::kCRLFLineFormat;
			}
			else
				format = Text::kCRLineFormat;
			break;
		}
		else if(c == kLineFeed)
		{
			format = Text::kLFLineFormat;
			break;
		}
		else
			stringWriter.append (c);
	}

	stringWriter.flush ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool TextStreamer::writeChar32 (uchar32 c)
{
	bool result = true;

	prepareWriteStream ();

	charBuffer[buffered++] = c;

	bool isNewline = flushNewline ? isNewlineCharacter (c) : false;
	if(buffered == kCharBufferSize || isNewline)
	{
		int numBytes = buffered * sizeof(charBuffer[0]);
		int bytesWritten = writeStream->write (charBuffer, numBytes);
		result &= (bytesWritten == numBytes);

		buffered = 0;

		if(isNewline && encoderStream != nullptr)
			encoderStream->flush ();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextStreamer::writeChar (uchar c)
{
	return writeChar32 ((uchar32)c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextStreamer::writeString (StringRef string, tbool appendNewline)
{
	StringChars chars (string);
	int length = string.length ();
	for(int i = 0; i < length; ++i)
	{
		ASSERT (!Core::Text::UTFCodec::isLowSurrogateUTF16 (chars[i]))

		if(Core::Text::UTFCodec::isHighSurrogateUTF16 (chars[i]))
		{
			ASSERT (i + 1 < length)
			if(i + 1 >= length)
				return false;

			uint32 c = Core::Text::UTFCodec::makeSurrogatePairUTF16 (chars[i], chars[i + 1]);
			if(!writeChar32 (c))
				return false;

			i++;
		}
		else if(!writeChar (chars[i]))
			return false;
	}

	if(appendNewline)
	{
		if(!writeNewline ())
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextStreamer::writeNewline ()
{
	switch(format)
	{
	case Text::kCRLFLineFormat :
		writeChar (kCarriageReturn);
		writeChar (kLineFeed);
		break;

	case Text::kCRLineFormat :
		writeChar (kCarriageReturn);
		break;

	case Text::kLFLineFormat :
		writeChar (kLineFeed);
		break;

	default :
		CCL_DEBUGGER ("TextStreamer: Unknown line format!")
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextStreamer::isNewlineCharacter (uchar32 c) const
{
	switch(format)
	{
	case Text::kLFLineFormat :
	case Text::kCRLFLineFormat :
		return c == kLineFeed;

	case Text::kCRLineFormat :
		return c == kCarriageReturn;

	default :
		CCL_DEBUGGER ("TextStreamer: Unknown line format!")
		break;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextStreamer::writeBOM ()
{
	if(targetStream == nullptr)
		return;

	switch(encoding)
	{
	case Text::kUTF16BE :
		targetStream->write (kBomUTF16BigEndian, sizeof(kBomUTF16BigEndian));
		break;

	case Text::kUTF16LE :
		targetStream->write (kBomUTF16LittleEndian, sizeof(kBomUTF16LittleEndian));
		break;

	case Text::kUTF8 :
		targetStream->write (kBomUTF8, sizeof(kBomUTF8));
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextStreamer::convert (IStream& destStream, IStream& sourceStream, TextEncoding encoding, TextLineFormat format)
{
	String line;
	TextStreamer reader (sourceStream, Text::kUnknownEncoding);
	if(reader.readLine (line))
	{
		if(encoding == Text::kUnknownEncoding)
			encoding = reader.getEncoding ();

		if(format == Text::kUnknownLineFormat)
			format = reader.getFormat ();

		TextStreamer writer (destStream, encoding);
		writer.setFormat (format);
		do
		{
			writer.writeString (line);
		}
		while(reader.readLine (line));
	}
	return true;
}

} // namespace CCL
