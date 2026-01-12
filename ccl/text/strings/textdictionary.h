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
// Filename    : ccl/text/strings/textdictionary.h
// Description : Text Dictionary Template
//
//************************************************************************************************

#ifndef _ccl_textdictionary_h
#define _ccl_textdictionary_h

#include "ccl/public/collections/vector.h"

namespace CCL {

//************************************************************************************************
// TextDictionary
//************************************************************************************************

template <typename StringType, typename StringRefType>
class TextDictionary
{
public:
	TextDictionary ()
	: caseSensitive (true)
	{}

	~TextDictionary ()
	{
		empty ();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////

	bool caseSensitive;

	bool isEqualKey (StringRefType key1, StringRefType key2) const
	{
		return key1.compare (key2, caseSensitive) == 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////

	int count () const
	{
		return entries.count ();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////

	StringRefType CCL_API keyAt (int index) const
	{
		StringPair* pair = (StringPair*)entries.at (index);
		if(pair)
			return pair->key;
		return StringType::kEmpty;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////

	StringRefType valueAt (int index) const
	{
		StringPair* pair = (StringPair*)entries.at (index);
		if(pair)
			return pair->value;
		return StringType::kEmpty;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////

	StringRefType lookup (StringRefType key) const
	{
		VectorForEach (entries, StringPair*, pair)
			if(isEqualKey (pair->key, key))
				return pair->value;
		EndFor
		return StringType::kEmpty;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////

	void set (StringRefType key, StringRefType value)
	{
		VectorForEach (entries, StringPair*, pair)
			if(isEqualKey (pair->key, key))
			{
				pair->value = value;
				return;
			}
		EndFor

		append (key, value);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////

	void append (StringRefType key, StringRefType value)
	{
		entries.add (NEW StringPair (key, value));
	}

	//////////////////////////////////////////////////////////////////////////////////////////////

	void remove (StringRefType key)
	{
		VectorForEach (entries, StringPair*, pair)
			if(isEqualKey (pair->key, key))
			{
				entries.remove (pair);
				delete pair;
				return;
			}
		EndFor
	}

	//////////////////////////////////////////////////////////////////////////////////////////////

	void empty ()
	{
		VectorForEach (entries, StringPair*, pair)
			delete pair;
		EndFor
		entries.removeAll ();
	}

protected:
	struct StringPair
	{
		StringPair (StringRefType key, StringRefType value)
		: key (key), value (value)
		{}

		StringType key;
		StringType value;
	};

	Vector<StringPair*> entries;
};

} // namespace CCL

#endif // _ccl_textdictionary_h
