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
// Filename    : ccl/public/storage/filetype.cpp
// Description : File Type
//
//************************************************************************************************

#include "ccl/public/storage/filetype.h"

#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// Predefined File Types
//************************************************************************************************

const FileType& FileTypes::getDefault (int which)
{
	return System::GetFileTypeRegistry ().getDefaultFileType (which);
}

//************************************************************************************************
// FileType
//************************************************************************************************

bool FileType::isValid () const
{ 
	return !extension.isEmpty () || !mimeType.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileType::clear () 
{ 
	description.empty ();
	extension.empty (); 
	mimeType.empty (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileType::equals (const FileType& t) const
{ 
	return extension.compare (t.extension, false) == Text::kEqual; // case-insensitive
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileType::operator == (const FileType& t) const 
{ 
	return equals (t); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileType::operator != (const FileType& t) const 
{ 
	return !equals (t); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileType::isTextType () const
{
	static const String kText ("text/");
	return mimeType.startsWith (kText);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileType::isHumanReadable () const
{
	static const String kXmlSuffix ("+xml");
	static const String kJsonSuffix ("+json");
	return	isTextType () || // plain text, xml, html, etc.
			mimeType.endsWith (kXmlSuffix) ||
			mimeType.endsWith (kJsonSuffix) ||
			*this == FileTypes::Json ();
}
