//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : sourcelocation.h
// Description : Shared "file:line" location formatting and C++-scope-qualified name helpers,
//               used by ClassScanner, TypeLibScanner, and SymbolResolver.
//
//************************************************************************************************

#ifndef _ccl_sourcelocation_h
#define _ccl_sourcelocation_h

#include "ccl/public/text/cstring.h"

namespace CCL {
namespace SourceLocation {

//////////////////////////////////////////////////////////////////////////////////////////////////
// SourceLocation inline helper functions
//////////////////////////////////////////////////////////////////////////////////////////////////

/** "file:line", for error messages. */
inline MutableCString makeLocation (CStringRef file, int line)
{
	MutableCString loc = file;
	loc += ":";
	loc.appendInteger (line);
	return loc;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** The 'file' part of a "file:line" string built by makeLocation(). */
inline MutableCString toLocationFile (CStringRef location)
{
	MutableCString loc (location);
	int pos = loc.lastIndex (':');
	return (pos >= 0) ? loc.subString (0, pos) : loc;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Last component of a (possibly C++-scope-qualified) reference, e.g. "CCL::Object" -> "Object". */
inline MutableCString toBareName (CStringRef raw)
{
	MutableCString s = raw;
	s.trimWhitespace ();
	int pos = s.lastIndex (':');
	if(pos > 0 && s.at (pos - 1) == ':')
		return s.subString (pos + 1);

	return s;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Join C++ scope qualifiers with '.', e.g. "Boxed::Point" -> "Boxed.Point". */
inline MutableCString toCanonicalName (CStringRef raw)
{
	MutableCString s = raw;
	s.trimWhitespace ();
	MutableCString result;
	CStringPtr text = s.str ();
	int length = s.length ();
	for(int i = 0; i < length; i++)
	{
		if(text[i] == ':' && i + 1 < length && text[i + 1] == ':')
		{
			result += '.';
			i++;
		}
		else
			result += text[i];
	}

	return result;
}

} // namespace SourceLocation
} // namespace CCL

#endif // _ccl_sourcelocation_h
