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
// Filename    : ccl/text/xml/xmlwriter.cpp
// Description : XML Writer
//
//************************************************************************************************

#include "ccl/text/xml/xmlwriter.h"
#include "ccl/text/xml/xmlentities.h"

using namespace CCL;

//************************************************************************************************
// XmlWriter
//************************************************************************************************

XmlWriter::XmlWriter ()
: SuperClass (NEW XmlEntities)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlWriter::beginDocument (IStream& stream, TextEncoding encoding)
{
	if(encoding == Text::kUnknownEncoding)
		encoding = Text::kUTF16;

	const char* xmlEncoding = XmlEncodings::getEncoding (encoding);
	ASSERT (xmlEncoding != nullptr)
	if(xmlEncoding == nullptr)
		return kResultInvalidArgument; // encoding not supported!

	// let superclass create the streamer
	tresult tr = SuperClass::beginDocument (stream, encoding);
	if(tr != kResultOk)
		return tr;

	ASSERT (streamer != nullptr)

	String line;
	line << "<?xml version=\"1.0\" encoding=\"" << xmlEncoding << "\"?>";
	if(!streamer->writeString (line, true))
		return kResultFalse;

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XmlWriter::characterData (IStream& charData, TextEncoding encoding)
{
	ASSERT (streamer != nullptr)
	if(streamer == nullptr)
		return kResultUnexpected;

	if(!streamer->writeString (String () << "<![CDATA[", false))
		return kResultFalse;

	if(encoding != Text::kUnknownEncoding)
	{
		ASSERT (encoding == streamer->getEncoding ())
		if(encoding != streamer->getEncoding ())
			return kResultInvalidArgument;

		UnknownPtr<IMemoryStream> memStream (&charData);
		ASSERT (memStream != nullptr)
		if(memStream == nullptr)
			return kResultNotImplemented;

		streamer->flush ();

		int toWrite = memStream->getBytesWritten ();
		if(streamer->getStream ().write (memStream->getMemoryAddress (), toWrite) != toWrite)
			return kResultFalse;
	}
	else
	{	
		charData.rewind ();
		uchar c = 0;
		while(charData.read (&c, sizeof(uchar)) == sizeof(uchar))
		{
			if(!streamer->writeChar (c))
				return kResultFalse;
		}
	}

	return streamer->writeString (String () <<  "]]>", true) ? kResultOk : kResultFalse;
}
