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
// Filename    : ccl/text/strings/translationtable.cpp
// Description : Translation Table
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/text/strings/translationtable.h"

#include "ccl/text/xml/xmlentities.h"
#include "ccl/text/transform/textstreamer.h"

#include "ccl/public/base/buffer.h"
#include "ccl/public/base/streamer.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/text/translationformat.h"

namespace CCL {

//************************************************************************************************
// MachineObjectHeader
//************************************************************************************************
// see http://www.gnu.org/software/gettext/manual/gettext.html#MO-Files

struct MachineObjectHeader
{
	int32 magic;
	int32 version;
	int32 numStrings;
	int32 originalTableOffset;
	int32 translationTableOffset;
	int32 hashTableSize;
	int32 hashTableOffset;

	MachineObjectHeader ()
	{ ::memset (this, 0, sizeof(MachineObjectHeader)); }

	bool deserialize (Streamer& s)
	{
		s.setByteOrder (kLittleEndian);
		if(!s.read (magic))
			return false;
		if(magic != 0x950412de)
		{
			if(magic == 0xde120495)
				s.setByteOrder (kBigEndian);
			else
				return false;
		}

		s.read (version);
		s.read (numStrings);
		s.read (originalTableOffset);
		s.read (translationTableOffset);
		s.read (hashTableSize);
		return s.read (hashTableOffset);
	}
};

//************************************************************************************************
// MachineObjectParser
//************************************************************************************************

class MachineObjectParser
{
public:
	MachineObjectParser (TranslationTable& table, IStream& stream, ITranslationTableHook* hook);

	bool parse ();

protected:
	TranslationTable& table;
	Streamer stream;
	ITranslationTableHook* hook;

	bool seekTo (int64 offset);
	bool readString (Buffer& buffer, int32& length);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

static void resolveTranslationEntities (String& text) // allow XML entities in translations
{
	String decodedText = XmlEntities ().decode (text);
	#if (0 && DEBUG)
	if(decodedText != text)
	{
		Debugger::print ("Replaced XML entities in translation: ");
		Debugger::println (text);
	}
	#endif
	text = decodedText;
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// TranslationEntry
//************************************************************************************************

void TranslationEntry::addText (const TranslatedText& text)
{
	bool added = false;
	if(!translations.isEmpty ())
	{
		for(int i = 0; i < translations.count (); i++)
		{
			if(translations[i].scope == text.scope)
			{
				CCL_PRINT ("Replacing translated text \"")
				CCL_PRINT (translations[i].text);
				CCL_PRINT ("\" with \"")
				CCL_PRINT (text.text)
				CCL_PRINTF ("\" in scope \"%s\"\n", text.scope.str ())

				translations[i].text = text.text;
				added = true;
				break;
			}
		}
	}

	if(added == false)
		translations.add (text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef TranslationEntry::getText (StringID scope) const
{
	int count = translations.count ();
	if(count == 0)
	{
		CCL_DEBUGGER ("Empty translation entry!")
		return String::kEmpty;
	}

	if(!scope.isEmpty () && count > 1)
	{
		for(int i = 0; i < count; i++)
			if(translations[i].scope == scope)
				return translations[i].text;
	}

	return translations[0].text;
}

//************************************************************************************************
// TranslationTable
//************************************************************************************************

tresult CCL_API TranslationTable::addString (StringID scope, StringID key, StringRef _text)
{
	String text;
	resolveVariables (text, _text);

	ASSERT (!key.isEmpty ())
	TranslationEntry* te = (TranslationEntry*)strings.lookup (key);
	if(te == nullptr)
	{
		te = NEW TranslationEntry (key, TranslationEntry::kCopy);
		strings.add (te);
	}
	#if (0 && DEBUG_LOG)
	else
		CCL_PRINTF ("Reusing translation entry \"%s\" for scope \"%s\"\n", key.str (), scope.str ())
	#endif

	te->addText (TranslatedText (scope, text));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TranslationTable::addStringWithUnicodeKey (StringID scope, StringRef unicodeKey, StringRef text)
{
	MutableCString asciiKey = XmlEntities ().encodeToASCII (unicodeKey);
	return addString (scope, asciiKey, text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TranslationTable::loadStrings (IStream& stream, ITranslationTableHook* hook)
{
	ASSERT (stream.isSeekable ())
	if(!stream.isSeekable ())
		return kResultInvalidArgument;

	bool result = MachineObjectParser (*this, stream, hook).parse ();
	return result ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TranslationTable::getString (String& result, StringID scope, StringID key) const
{
	if(key.isEmpty ())
	{
		result.empty ();
		return kResultOk;
	}

	TranslationEntry* te = (TranslationEntry*)strings.lookup (key);
	if(te)
	{
		result = te->getText (scope);
		return kResultOk;
	}
	else
	{
		// use key if not found
		String text (key);
		resolveTranslationEntities (text);
		resolveVariables (result, text);
		return kResultFalse;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TranslationTable::getStringWithUnicodeKey (String& result, StringID scope, StringRef unicodeKey) const
{
	if(unicodeKey.isEmpty ())
	{
		result.empty ();
		return kResultOk;
	}

	MutableCString asciiKey = XmlEntities ().encodeToASCII (unicodeKey);

	TranslationEntry* te = (TranslationEntry*)strings.lookup (asciiKey);
	if(te)
		result = te->getText (scope);
	else
	{
		// use key if not found
		String text (unicodeKey);
		resolveTranslationEntities (text);
		resolveVariables (result, text);
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static bool isValidVariableChar (uchar c)
{
	if(c > 127) // allow ASCII characters only!
		return false;
	return CString::isAlpha ((char)c);
}

void TranslationTable::resolveVariables (String& result, StringRef text) const
{
	static const String strVariablePrefix = CCLSTR ("$");

	if(text.isEmpty () || variables.isEmpty () || !text.contains (strVariablePrefix)) // nothing to do ;-)
	{
		result = text;
		return;
	}

	StringWriter<512> resultWriter (result, true);

	StringChars chars (text);
	int length = text.length ();
	for(int i = 0; i < length; i++)
	{
		if(chars[i] == '$')
		{
			i++;
			if(chars[i] == '$')
				resultWriter.append (chars[i]);
			else
			{
				MutableCString varName;
				CStringWriter<256> varNameWriter (varName);
				while(i < length)
				{
					if(!isValidVariableChar (chars[i]))
					{
						i--;
						break;
					}

					varNameWriter.append (chars[i]);
					i++;
				}
				varNameWriter.flush ();

				String varString = getVariable (varName);

				resultWriter.flush ();
				result.append (varString);

				#if (0 && DEBUG_LOG)
				String varNameString (varName);
				CCL_PRINT (String ().appendFormat (CCLSTR ("Replaced variable %(1) with \"%(2)\"\n"), varNameString, varString))
				#endif
			}
		}
		else
			resultWriter.append (chars[i]);
	}

	resultWriter.flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TranslationTable::addVariable (StringID name, StringRef text)
{
	variables.add (NEW UnicodeStringEntry (name, text, UnicodeStringEntry::kCopy));
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String TranslationTable::getVariable (StringID name) const
{
	UnicodeStringEntry* e = (UnicodeStringEntry*)variables.lookup (name);
	if(e)
		return e->theString;
	else
		return String (CCLSTR ("$")) << name; // keep unresolved variable
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TranslationTable::saveStrings (IStream& stream, tbool isTemplate) const
{
	TextStreamer streamer (stream, Text::kUTF8, Text::kLFLineFormat, TextStreamer::kSuppressByteOrderMark);
	PortableObjectFormat::FormatWriter formatWriter (&streamer);
	if(!isTemplate) // .pot doesn't have a header
	{
		if(!formatWriter.writeHeader ())
			return kResultFailed;
	}

	for(int tableIndex = 0; tableIndex < strings.getSize (); tableIndex++)
	{
		const StringEntryList& list = strings.getList (tableIndex);
		if(!list.isEmpty ())
			ListForEach (list, StringEntry*, entry)
				TranslationEntry* te = (TranslationEntry*)entry;
				if(!te->translations.isEmpty ())
				{
					for(int i = 0; i < te->translations.count (); i++)
					{
						const TranslatedText& text = te->translations[i];
						if(!formatWriter.writeMessage (String (text.scope), String (te->cString), text.text))
							return kResultFailed;
					}					
				}
				else
				{
					if(!formatWriter.writeMessage (String::kEmpty, String (te->cString), String::kEmpty))
						return kResultFailed;
				}
			EndFor
	}
	return kResultOk;
}

//************************************************************************************************
// MachineObjectParser
//************************************************************************************************

MachineObjectParser::MachineObjectParser (TranslationTable& table, IStream& stream, ITranslationTableHook* hook)
: table (table),
  stream (stream),
  hook (hook)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MachineObjectParser::seekTo (int64 offset)
{
	return stream->seek (offset, IStream::kSeekSet) == offset;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MachineObjectParser::readString (Buffer& buffer, int32& length)
{
	length = 0;
	if(!stream.read (length))
		return false;
	int32 offset = 0;
	if(!stream.read (offset))
		return false;

	int64 oldPos = stream->tell ();
	if(!seekTo (offset))
		return false;

	if(length >= (int32)buffer.getSize ())
		buffer.resize (length + 1);

	length = stream.read (buffer, length + 1) - 1; // read with NULL

	return seekTo (oldPos);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MachineObjectParser::parse ()
{
	// *** Read Header ***
	MachineObjectHeader header;
	if(!header.deserialize (stream))
		return false;
		
	// *** Read Original Strings ***
	Vector<MutableCString> originalTable;
	if(!seekTo (header.originalTableOffset))
		return false;

	Buffer buffer;
	for(int i = 0; i < header.numStrings; i++)
	{
		int32 length = 0;
		if(!readString (buffer, length))
			return false;

		const char* charBuffer = buffer.as<char> ();

		MutableCString key;
		key.append (charBuffer, length);
		originalTable.add (key);
	}

	// *** Read Translated Strings ***
	Vector<String> translationTable;
	if(!seekTo (header.translationTableOffset))
		return false;

	for(int i = 0; i < header.numStrings; i++)
	{
		int32 length = 0;
		if(!readString (buffer, length))
			return false;

		const char* charBuffer = buffer.as<char> ();

		String text;
		text.appendCString (Text::kUTF8, charBuffer, length);		
		resolveTranslationEntities (text);

		translationTable.add (text);
	}

	// *** Add to Table ***
	for(int i = 0; i < header.numStrings; i++)
	{
		MutableCString& original = originalTable[i];
		if(original.isEmpty ())
			continue;

		MutableCString scope;
		const char* start = original.str ();

		// ASCII char 4 [EOT, End Of Transmission]
		#define GETTEXT_EOT '\004'
		int eot = original.index (GETTEXT_EOT);
		if(eot != -1)
		{
			// PO conform format: scope results from msgctxt,
			// separated from msgid via GETTEXT_EOT.
			start += eot + 1;
			scope.append (original.str (), eot);
		}
		else
		{
			// Legacy format: msgid contains scope as
			// leading, [] enclosed substring.
			const char* ptr = start;
			if(*ptr == '[')
			{
				while(*ptr && *ptr != ']')
					ptr++;

				ASSERT (*ptr == ']')
				if(*ptr == ']')
				{
					start = ptr + 1;
					int count = (int)(start - original.str () - 2);
					scope.append (original.str () + 1, count);
				}
			}
		}

		String text = translationTable[i];
		if(text.isEmpty ())
			 continue;

		MutableCString key (start);
		table.addString (scope, key, text);

		if(hook)
			hook->translationAdded (scope, key, text);
	}

	return true;
}
