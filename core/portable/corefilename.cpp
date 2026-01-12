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
// Filename    : core/portable/corefilename.cpp
// Description : Filename class
//
//************************************************************************************************

#include "core/portable/corefilename.h"

#include "core/public/corebasicmacros.h"

using namespace Core;
using namespace Portable;

#if CORE_PLATFORM_WINDOWS
#define PATH_CHAR "\\"
#else
#define PATH_CHAR "/"
#endif

//************************************************************************************************
// FileName
//************************************************************************************************

CStringPtr FileName::kPathDelimiter = PATH_CHAR;

//////////////////////////////////////////////////////////////////////////////////////////////////

FileName::FileName (CStringPtr filename)
: CString256 (filename)
{
	// ATTENTION: Must not change path delimiter here!
	// Use adjustPathDelimiters() explicitly if needed.
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileName& FileName::ascend ()
{
	int index = lastIndex (kPathDelimiter[0]);
	if(index != -1)
		truncate (index);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileName& FileName::descend (CStringPtr name)
{
	if(lastChar () != kPathDelimiter[0])
		*this += kPathDelimiter;
	*this += name;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileName& FileName::makeValid ()
{
	static const CStringPtr kInvalidFileNameChars = "?*/\\<>|:\"\t\r\n";
	
	for(int i = 0, count = length (); i < count; i++)
		if(::strchr (kInvalidFileNameChars, buffer[i]))
			buffer[i] = '_';

	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileName& FileName::adjustPathDelimiters (PathDelimiterType type)
{
	switch(type)
	{
	case kPathChar :
		if(kPathDelimiter[0] != '/')
		{
			replace ('/', kPathDelimiter[0]);
			break;
		} // fall trough on other platforms

	case kForwardSlash :
		replace ('\\', '/');
		break;
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileName::isRelative () const
{
	static const CStringPtr kThisPrefix = "." PATH_CHAR;
	static const CStringPtr kParentPrefix = ".." PATH_CHAR;
	
	return	isEmpty () || 
			*this == "." ||
			*this == ".." ||
			startsWith (kThisPrefix) ||
			startsWith (kParentPrefix);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileName& FileName::makeAbsolute (CStringPtr baseFolder)
{
	FileName relativePath = *this;
	*this = baseFolder;
	// let OS resolve relative paths
	// TODO: use string tokenizer...
	descend (relativePath);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileName& FileName::setExtension (CStringPtr ext, bool replace)
{
	if(replace == true)
	{
		int index = lastIndex ('.');
		if(index != -1)
			truncate (index);
	}

	if(lastChar () != '.')
		append ('.');
	append (ext);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileName& FileName::removeExtension ()
{
	int index = lastIndex ('.');
	if(index > 0)
		truncate (index);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileName::getExtension (FileName& extension) const
{
	extension.empty ();
	int index = lastIndex ('.');
	if(index != -1)
		subString (extension, index+1);
	return !extension.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileName::getName (FileName& name) const
{
	int index = lastIndex (kPathDelimiter[0]);
	if(index != -1)
		subString (name, index + 1);
	else
		name = *this;
}
