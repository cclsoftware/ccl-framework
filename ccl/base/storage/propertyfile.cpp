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
// Filename    : ccl/base/storage/propertyfile.cpp
// Description : Java Property Files
//
//************************************************************************************************

/*
	http://en.wikipedia.org/wiki/.properties
	http://download.oracle.com/javase/6/docs/api/java/util/Properties.html#load%28java.io.Reader%29
*/

#include "ccl/base/storage/propertyfile.h"

#include "ccl/public/text/itextstreamer.h"
#include "ccl/public/base/istream.h"
#include "ccl/public/storage/filetype.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Java;

//************************************************************************************************
// Java::PropertyParser
//************************************************************************************************

PropertyParser::PropertyParser (StringDictionary& properties)
: properties (properties)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PropertyParser::parse (StringRef string)
{
	AutoPtr<IStream> stream = System::GetFileUtilities ().createStringStream (string, Text::kUTF16);
	return stream ? parse (*stream) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PropertyParser::parse (IStream& stream)
{
	properties.removeAll ();

	static const String commentChar1 ("#");
	static const String commentChar2 ("!");

	static const String separatorChar1 ("=");

	AutoPtr<ITextStreamer> reader = System::CreateTextStreamer (stream);
	while(1)
	{
		String line;
		if(!reader->readLine (line))
			break;

		line.trimWhitespace ();
		if(line.isEmpty ())
			continue;

		// ignore comments
		if(line.startsWith (commentChar1) || line.startsWith (commentChar2))
			continue;

		// TODO:
		// - handle values spread accross multiple lines via "\"
		// - handle escape sequences
		// - handle " " or ":" as separator

		String key, value;
		int index = line.index (separatorChar1);
		if(index != -1)
		{
			key = line.subString (0, index);
			value = line.subString (index + 1);
		}
		else
			key = line;
		
		key.trimWhitespace ();
		if(key.isEmpty ())
			continue;

		value.trimWhitespace ();

		properties.appendEntry (key, value);
	}
	return true;
}

//************************************************************************************************
// Java::PropertyWriter
//************************************************************************************************

PropertyWriter::PropertyWriter (const StringDictionary& properties)
: properties (properties)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PropertyWriter::write (IStream& stream)
{
	AutoPtr<ITextStreamer> writer = System::CreateTextStreamer (stream, {Text::kUTF8, Text::kSystemLineFormat});
	for(int i = 0; i < properties.countEntries (); i++)
	{
		StringRef key = properties.getKeyAt (i);
		StringRef value = properties.getValueAt (i);

		// TODO: escape special characters!
		
		String line;
		line << key << "=" << value;
		if(!writer->writeString (line, true))
			return false;
	}
	return true;
}

//************************************************************************************************
// Java::PropertyFile
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PropertyFile, StorableObject)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PropertyFile::getFormat (FileType& format) const
{
	format = FileTypes::Properties ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PropertyFile::save (IStream& stream) const
{
	return PropertyWriter (properties).write (stream);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PropertyFile::load (IStream& stream)
{
	return PropertyParser (properties).parse (stream);
}
