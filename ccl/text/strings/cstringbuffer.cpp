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
// Filename    : ccl/text/strings/cstringbuffer.cpp
// Description : C-String Buffer
//
//************************************************************************************************

#define OPTIMIZE_STRING 1 // enable string optimizations

#include "ccl/text/strings/cstringbuffer.h"
#include "ccl/text/strings/stringstats.h"

using namespace CCL;

#if STRING_STATS
static StringStatistics<char> theStats ("C-String Statistics");
#endif

//************************************************************************************************
// CStringBuffer
//************************************************************************************************

CStringBuffer::CStringBuffer (const char* text)
: text (nullptr),
  textByteSize (0)
{
	STRING_ADDED

	if(text && text[0])
		assign (text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringBuffer::CStringBuffer (const CStringBuffer& other)
: text (nullptr),
  textByteSize (0)
{
	STRING_ADDED

	if(other.text && other.text[0])
		assign (other.text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringBuffer::~CStringBuffer ()
{
	resize (0);

	STRING_REMOVED
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CStringBuffer::assign (const char* _text)
{
	ASSERT (_text != nullptr)
	int length = (int)::strlen (_text);
	if(!resize (length))
		return false;
	::strcpy (text, _text);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CStringBuffer::resize (int newLength)
{
	unsigned int byteSize = newLength > 0 ? newLength + 1 : 0;
	
	#if OPTIMIZE_STRING
	if(byteSize > 0)
	{
		static const unsigned int delta = 16;

		byteSize = (byteSize / delta + 1) * delta;
		if(byteSize == textByteSize)
			return true;
	}
	#endif
	
	if(byteSize == 0)
	{
		if(text)
			string_free ((void*)text);
		text = nullptr;
	}
	else
	{
		void* temp = text ? string_realloc ((void*)text, byteSize) : string_malloc (byteSize);
		if(temp == nullptr)
			return false;

		text = (char*)temp;
	}
	
	STRING_RESIZED (textByteSize, byteSize)

	textByteSize = byteSize;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

char* CCL_API CStringBuffer::getText ()
{
	return text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICString* CCL_API CStringBuffer::cloneString () const
{
	return NEW CStringBuffer (*this);
}
