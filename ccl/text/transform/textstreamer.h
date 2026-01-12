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
// Filename    : ccl/text/transform/textstreamer.h
// Description : Text Streamer
//
//************************************************************************************************

#ifndef _ccl_textstreamer_h
#define _ccl_textstreamer_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/text/itextstreamer.h"
#include "ccl/public/base/idatatransformer.h"

namespace CCL {

class TransformReader;
class TransformWriter;

//************************************************************************************************
// TextStreamer
/** A TextStreamer uses encoders and decoders for writing and reading and text in various encodings.
	The internal representation is always uchar32 (a 32 bit unicode character). A decoder converts 
	a specific encoding to a uchar32 sequence. An encoder converts a uchar32 sequence to a 
	specific encoding. */
//************************************************************************************************

class TextStreamer: public Unknown,
					public ITextStreamer
{
public:
	TextStreamer (IStream& stream, TextEncoding encoding, TextLineFormat format = Text::kUnknownLineFormat, int options = 0);
	~TextStreamer ();

	PROPERTY_VARIABLE (TextEncoding, encoding, Encoding)
	PROPERTY_VARIABLE (TextLineFormat, format, Format)
	PROPERTY_BOOL (writeByteOrder, WriteByteOrder)

	void flush ();
	IStream& getStream ();

	tbool writeChar32 (uchar32 c);

	// ITextStreamer
	TextEncoding CCL_API getTextEncoding () const override	{ return getEncoding (); }
	TextLineFormat CCL_API getLineFormat () const override	{ return getFormat (); }
	tbool CCL_API isEndOfStream () const override			{ return endOfStream; }
	tbool CCL_API readChar (uchar& c) override;
	tbool CCL_API readLine (String& string) override;
	tbool CCL_API writeChar (uchar c) override;
	tbool CCL_API writeString (StringRef string, tbool appendNewline = false) override;
	tbool CCL_API writeNewline () override;

	CLASS_INTERFACE (ITextStreamer, Unknown)

	/// convert a whole stream (to be moved elsewhere!!?)
	static bool convert (IStream& destStream, 
						 IStream& sourceStream, 
						 TextEncoding encoding = Text::kUnknownEncoding, 
						 TextLineFormat format = Text::kUnknownLineFormat);

private:
	IStream* targetStream;
	IStream* readStream;
	IStream* writeStream;
	TransformReader* decoderStream;
	TransformWriter* encoderStream;
	enum { kCharBufferSize = 512 };
	uchar32 charBuffer[kCharBufferSize];
	int buffered;
	uchar nextChar;
	bool endOfStream;
	bool flushNewline;

	IDataTransformer* createEncoder ();
	IDataTransformer* createDecoder ();
	void prepareReadStream ();
	void prepareWriteStream ();
	void writeBOM ();
	bool isNewlineCharacter (uchar32 c) const;
};

} // namespace CCL

#endif // _ccl_textstreamer_h
