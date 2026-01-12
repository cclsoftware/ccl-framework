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
// Filename    : ccl/base/storage/textfile.cpp
// Description : Text File
//
//************************************************************************************************

#include "ccl/base/storage/textfile.h"

#include "ccl/base/message.h"
#include "ccl/base/collections/stringlist.h"

#include "ccl/public/text/ihtmlwriter.h"
#include "ccl/public/text/itextbuilder.h"

#include "ccl/public/storage/filetype.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// TextUtils
//************************************************************************************************

TextEncoding TextUtils::getEncodingByName (StringRef name)
{
	if(name.contains (CCLSTR ("utf-8"), false))
		return Text::kUTF8;
	if(name.contains (CCLSTR ("utf-16"), false))
		return Text::kUTF16;
	if(name.contains (CCLSTR ("ascii"), false))
		return Text::kASCII;
	if(name.contains (CCLSTR ("iso-8859-1"), false))
		return Text::kISOLatin1;

	CCL_DEBUGGER ("Unknown text encoding!\n")
	return Text::kUnknownEncoding;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String TextUtils::loadRawString (UrlRef path)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path);
	if(stream)
		return loadRawString (*stream);

	return String ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String TextUtils::loadRawString (IStream& stream)
{
	String text;
	AutoPtr<ITextStreamer> reader = System::CreateTextStreamer (stream);

	StringWriter<256> writer (text);
	uchar c = 0;
	while(reader->readChar (c))
	{
		if(c == 0)
			break;

		writer.append (c);
	}
	writer.flush ();

	return text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String TextUtils::loadString (UrlRef path, String endline, TextEncoding encoding)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path);
	if(stream)
		return loadString (*stream, endline, encoding);
	return String ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String TextUtils::loadString (IStream& stream, String endline, TextEncoding encoding)
{
	String text;
	AutoPtr<ITextStreamer> reader = System::CreateTextStreamer (stream, {encoding});
	String line;
	while(reader->readLine (line))
		text << line << endline;
	return text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextUtils::loadStringList (StringList& stringList, UrlRef path, bool ignoreEmptyLines, TextEncoding encoding)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path);
	if(stream)
		return loadStringList (stringList, *stream, ignoreEmptyLines, encoding);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextUtils::loadStringList (StringList& stringList, IStream& stream, bool ignoreEmptyLines, TextEncoding encoding)
{
	AutoPtr<ITextStreamer> reader = System::CreateTextStreamer (stream, {encoding});
	String line;
	while(reader->readLine (line))
	{
		if(ignoreEmptyLines)
		{
			line.trimWhitespace ();
			if(line.isEmpty ())
				continue;
		}
		stringList.add (line);
	}
	return !stringList.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef TextUtils::getCSS ()
{
	static const String cssStyle (
		"body { color: #313131; font: 8pt \"Lucida Grande\", Lucida, Verdana, sans-serif; }" \
		"td { font: 8pt \"Lucida Grande\", Lucida, Verdana, sans-serif; width: 200px; vertical-align: top; border-bottom: 1px solid #e9ebeb; }" \
		"h2 { font: bold 10pt \"Lucida Grande\", Lucida, Verdana, sans-serif; margin-bottom: 4; }");

	return cssStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextUtils::saveTextBlock (UrlRef path, StringRef title, VariantRef data, const ITextPromise& textPromise)
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kCreateMode);
	if(stream == nullptr)
		return false;

	if(path.getFileType () == FileTypes::Html ())
	{
		AutoPtr<IHtmlWriter> writer = System::CreateTextWriter<IHtmlWriter> ();
		writer->setShouldIndent (false);
		if(writer->beginDocument (*stream, Text::kUTF8) != kResultOk)
			return false;
		
		writer->pushStyleElement (getCSS ());

		writer->startElement (String (HtmlTags::kHtml));
		writer->writeHead (title);
		writer->startElement (String (HtmlTags::kBody));

		AutoPtr<ITextBuilder> htmlBuilder = writer->createHtmlBuilder ();
		TextBlock block (htmlBuilder);
		textPromise.createText (block, title, data);
		writer->writeMarkup (block);

		writer->endElement (String (HtmlTags::kBody));
		writer->endElement (String (HtmlTags::kHtml));
		return writer->endDocument () == kResultOk;
	}
	else
	{
		CCL_DEBUGGER ("Unknown file type!")
		return false;
	}
}

//************************************************************************************************
// TextFile
//************************************************************************************************

DEFINE_CLASS (TextFile, Object)
DEFINE_CLASS_NAMESPACE (TextFile, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

TextFile::TextFile ()
: streamer (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextFile::TextFile (UrlRef path, TextEncoding encoding, TextLineFormat lineFormat, int options)
: streamer (nullptr)
{
	create (path, encoding, lineFormat, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextFile::TextFile (UrlRef path, Mode mode, TextEncoding encoding, int options)
: streamer (nullptr)
{
	ASSERT (mode == kOpen)
	open (path, encoding, options);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextFile::~TextFile ()
{
	close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextFile::create (UrlRef filePath, TextEncoding encoding, TextLineFormat lineFormat, int options)
{
	ASSERT (streamer == nullptr)

	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (filePath, IStream::kCreateMode|IStream::kShareRead);
	ASSERT (stream != nullptr)
	if(stream == nullptr)
		return;

	path = filePath;

	streamer = System::CreateTextStreamer (*stream, {encoding, lineFormat, options});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextFile::open (UrlRef filePath, TextEncoding encoding, int options)
{
	ASSERT (streamer == nullptr)

	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (filePath, IStream::kOpenMode);
	ASSERT (stream != nullptr)
	if(stream == nullptr)
		return;

	path = filePath;

	streamer = System::CreateTextStreamer (*stream, {encoding, options});
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextFile::close ()
{
	safe_release (streamer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (TextFile)
	DEFINE_PROPERTY_NAME ("endOfStream")
END_PROPERTY_NAMES (TextFile)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextFile::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "endOfStream")
	{
		var = !streamer || streamer->isEndOfStream ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (TextFile)
	DEFINE_METHOD_NAME ("readLine")
	DEFINE_METHOD_NAME ("writeLine")
	DEFINE_METHOD_NAME ("writeString")
	DEFINE_METHOD_NAME ("close")
END_METHOD_NAMES (TextFile)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextFile::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "readLine")
	{
		ASSERT (isValid ())
		String string;
		if(streamer)
			streamer->readLine (string);
		returnValue = string;
		returnValue.share ();
		return true;
	}
	else if(msg == "writeLine" || msg == "writeString")
	{
		ASSERT (isValid ())
		String string (msg[0].asString ());
		bool appendNewline = msg == "writeLine";
		returnValue = streamer && streamer->writeString (string, appendNewline);
		return true;
	}
	else if(msg == "close")
	{
		close ();
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// TextResource
//************************************************************************************************

DEFINE_CLASS_HIDDEN (TextResource, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

TextResource::TextResource (StringRef content, TextEncoding encoding)
: content (content),
  encoding (encoding),
  suppressByteOrderMark (false),
  suppressFinalLineEnd (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextResource::getFormat (FileType& format) const
{
	format = FileTypes::Text ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextResource::save (IStream& stream) const
{
	auto isByteOrderMarkNeeded = [&] ()
	{
		return encoding == Text::kUTF8 || encoding == Text::kUTF16;
	};

	// suppress BOM for non-Unicode encodings
	int options = isByteOrderMarkNeeded () && !suppressByteOrderMark ? 0 : ITextStreamer::kSuppressByteOrderMark;
	AutoPtr<ITextStreamer> writer = System::CreateTextStreamer (stream, {encoding, Text::kSystemLineFormat, options});
	return writer->writeString (content);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextResource::load (IStream& stream)
{
	content.empty ();

	String endline = String::getLineEnd ();
	AutoPtr<ITextStreamer> reader = System::CreateTextStreamer (stream, {encoding});
	String line;
	while(reader->readLine (line))
	{
		content << line;
		if(!suppressFinalLineEnd || !reader->isEndOfStream ())
			content << endline;
	}
	return true;
}
