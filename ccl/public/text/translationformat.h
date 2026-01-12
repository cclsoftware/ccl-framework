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
// Filename    : ccl/public/text/translationformat.h
// Description : Translation File Format
//
//************************************************************************************************

#ifndef _ccl_translationformat_h
#define _ccl_translationformat_h

#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/itextstreamer.h"
#include "ccl/public/base/cclmacros.h"

namespace CCL {
namespace PortableObjectFormat {

//////////////////////////////////////////////////////////////////////////////////////////////////
/*
	The Format of PO Files
	http://www.gnu.org/software/hello/manual/gettext/PO-Files.html
*/
//////////////////////////////////////////////////////////////////////////////////////////////////

static const String keyword_msgctxt ("msgctxt");
static const String keyword_msgid ("msgid");
static const String keyword_msgstr ("msgstr");
static const String quote_char ("\"");
static const String hash_char ("#");
static const String space_char (" ");
static const String keyword_reference ("#:");
static const String blank_literal ("\"\"");
static const String orphaned_prefix ("#~");
	
//************************************************************************************************
// FormatWriter
//************************************************************************************************

class FormatWriter
{
public:
	FormatWriter (ITextStreamer* streamer);

	PROPERTY_VARIABLE (int, lineNumber, LineNumber)
	PROPERTY_VARIABLE (int, lastMessageLineNumber, LastMessageLineNumber)

	bool writeHeader ();
	bool writeReference (StringRef reference);
	bool writeMessage (StringRef scope, StringRef key, StringRef translation, bool orphaned = false);

protected:
	ITextStreamer* streamer;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// FormatWriter inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline FormatWriter::FormatWriter (ITextStreamer* streamer)
: streamer (streamer),
  lineNumber (1),
  lastMessageLineNumber (-1)
{
	ASSERT (streamer)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool FormatWriter::writeHeader ()
{
	// Header does not have a msgctxt.

	if(!streamer->writeString (String () << keyword_msgid << space_char << blank_literal, true)) // msgid ""
		return false;
	lineNumber++;
	if(!streamer->writeString (String () << keyword_msgstr << space_char << blank_literal, true)) // msgstr ""
		return false;
	lineNumber++;

	static CStringPtr headers[] =
	{
		//"Project-Id-Version: \\n", // TODO
		//"POT-Creation-Date: \\n",
		//"PO-Revision-Date: \\n",
		//"Last-Translator: \\n", // TODO
		//"Language-Team: \\n", // TODO
		"MIME-Version: 1.0\\n",
		"Content-Type: text/plain; charset=utf-8\\n",
		"Content-Transfer-Encoding: 8bit\\n"
	};

	for(int i = 0; i < ARRAY_COUNT (headers); i++)
	{
		String header;
		header << quote_char;
		header << headers[i];
		header << quote_char;

		if(!streamer->writeString (header , true))
			return false;
		lineNumber++;
	}

	if(!streamer->writeNewline ()) // nl
		return false;
	lineNumber++;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool FormatWriter::writeReference (StringRef reference)
{
	if(!streamer->writeString (String () << keyword_reference << space_char << reference, true)) // #. ref
		return false;
	lineNumber++;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool FormatWriter::writeMessage (StringRef scope, StringRef key, StringRef translation, bool orphaned)
{
	/* Write 'value' as 'keyword' item to stream, optionally tagged as orphaned. */
	auto writeStream = [this] (StringRef keyword, StringRef value, bool orphaned) -> bool
	{
		String str = String () << quote_char << value << quote_char;
		String line = String () << keyword << space_char << str;
		if(orphaned)
			line = String () << orphaned_prefix << " " << line;

		return streamer->writeString (line, true);
	};

	// msgctxt, optional. 'No value' may be implied by no
	// export or export as empty string, use no export here.
	if(!scope.isEmpty ())
	{
		if(!writeStream (keyword_msgctxt, scope, orphaned))
			return false;
		lineNumber++;
	}

	// msgid
	if(!writeStream (keyword_msgid, key, orphaned))
		return false;
	lineNumber++;

	// msgstr
	if(!writeStream (keyword_msgstr, translation, orphaned))
		return false;
	lastMessageLineNumber = lineNumber;
	lineNumber++;

	if(!streamer->writeNewline ()) // nl
		return false;
	lineNumber++;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace PortableObjectFormat
} // namespace CCL

#endif // _ccl_translationformat_h
