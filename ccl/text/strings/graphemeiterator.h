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
// Filename    : ccl/text/strings/graphemeiterator.h
// Description : String Grapheme Iterator
//
//************************************************************************************************

#ifndef _ccl_graphemeiterator_h
#define _ccl_graphemeiterator_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/text/cclstring.h"

#include "core/text/coregraphemeiterator.h"

namespace CCL {

//************************************************************************************************
// GraphemeIterator
//************************************************************************************************

class GraphemeIterator: public Unknown,
						public IGraphemeIterator
{
public:
	GraphemeIterator (StringRef _string, int position)
	: string (_string), // make sure we hold a reference to the IString
	  stringChars (string),
	  iter (stringChars, string.length (), position)
	{}

	// IGraphemeIterator
	int CCL_API getPosition () const override { return iter.getPosition (); }
	void CCL_API setPosition (int position) override { iter.setPosition (position); }
	int CCL_API next () override { return iter.next (); }
	int CCL_API previous () override { return iter.previous (); }

	CLASS_INTERFACE (IGraphemeIterator, Unknown)

protected:
	String string;
	StringChars stringChars;
	Core::Text::GraphemeIterator iter;
};

} // namespace CCL

#endif // _ccl_graphemeiterator_h
