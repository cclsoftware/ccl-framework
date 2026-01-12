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
// Filename    : ccl/text/strings/cstringbuffer.h
// Description : C-String Buffer
//
//************************************************************************************************

#ifndef _ccl_cstringbuffer_h
#define _ccl_cstringbuffer_h

#include "ccl/public/text/cstring.h"

#include "ccl/text/strings/stringtable.h"

namespace CCL {

//************************************************************************************************
// CStringEntry
//************************************************************************************************

struct CStringEntry: StringEntry
{
	MutableCString theCString;

	CStringEntry (const MutableCString& string)
	: StringEntry (string.str (), kNoCopy),
	  theCString (string)
	{}
};

//************************************************************************************************
// CStringBuffer
//************************************************************************************************

class CStringBuffer: public Unknown,
					 public ICString
{
public:
	CStringBuffer (const char* text = nullptr);
	CStringBuffer (const CStringBuffer&);
	~CStringBuffer ();

	// ICString
	tbool CCL_API resize (int newLength) override;
	char* CCL_API getText () override;
	ICString* CCL_API cloneString () const override;

	CLASS_INTERFACE (ICString, Unknown)

protected:
	char* text;
	int textByteSize;

	CStringBuffer& operator = (const CStringBuffer&)
	{ ASSERT (0) return *this; }

	bool assign (const char* text);
};

} // namespace CCL

#endif // _ccl_cstringbuffer_h
