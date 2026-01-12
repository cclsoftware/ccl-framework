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
// Filename    : ccl/public/text/istringdict.h
// Description : String Dictionary Interface
//
//************************************************************************************************

#ifndef _ccl_istringdict_h
#define _ccl_istringdict_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/text/textencoding.h"

namespace CCL {

interface ICStringDictionary;

//************************************************************************************************
// ITStringDictionary
/** String dictionary interface template. 
	\ingroup ccl_text */
//************************************************************************************************

template<typename StringRefType>
interface IStringDictionaryT: IUnknown
{
	/** Check if dictionary keys are case-sensitive. */
	virtual tbool CCL_API isCaseSensitive () const = 0;

	/** Set case-sensitivity of dictionary keys. */
	virtual void CCL_API setCaseSensitive (tbool state) = 0;

	/** Get number of entries. */
	virtual int CCL_API countEntries () const = 0;

	/** Get key of entry at given index. */
	virtual StringRefType CCL_API getKeyAt (int index) const = 0;
	
	/** Get value of entry at given index. */
	virtual StringRefType CCL_API getValueAt (int index) const = 0;

	/** Lookup value by key, returns empty string if not found. */
	virtual StringRefType CCL_API lookupValue (StringRefType key) const = 0;

	/** Set entry overwriting existing. */
	virtual void CCL_API setEntry (StringRefType key, StringRefType value) = 0;
	
	/** Append entry not overwriting existing. */
	virtual void CCL_API appendEntry (StringRefType key, StringRefType value) = 0;

	/** Remove entry with given key. */
	virtual void CCL_API removeEntry (StringRefType key) = 0;

	/** Remove all entries. */
	virtual void CCL_API removeAll () = 0;
};

//************************************************************************************************
// IStringDictionary
/** Unicode string dictionary interface. */
//************************************************************************************************

interface IStringDictionary: IStringDictionaryT<StringRef>
{
	/** Copy all entries from other dictionary. */
	virtual void CCL_API copyFrom (const IStringDictionary& dictionary) = 0;

	/** Convert to C-string dictionary with given text encoding. */
	virtual void CCL_API convertTo (ICStringDictionary& dst, TextEncoding encoding) const = 0;
	
	DECLARE_IID (IStringDictionary)
};

DEFINE_IID (IStringDictionary, 0xea3b3f22, 0x9f08, 0x45d2, 0xa7, 0x80, 0x4b, 0xb, 0xf6, 0x1e, 0x7, 0x87)

//************************************************************************************************
// ICStringDictionary
/** C-string dictionary interface. */
//************************************************************************************************

interface ICStringDictionary: IStringDictionaryT<CStringRef>
{
	/** Copy all entries from other dictionary. */
	virtual void CCL_API copyFrom (const ICStringDictionary& dictionary) = 0;

	/** Convert to Unicode string dictionary with given text encoding. */
	virtual void CCL_API convertTo (IStringDictionary& dst, TextEncoding encoding) const = 0;
	
	DECLARE_IID (ICStringDictionary)
};

DEFINE_IID (ICStringDictionary, 0xf60d4b7b, 0x5ff3, 0x4213, 0x8b, 0xdf, 0xd9, 0x65, 0x60, 0x64, 0xb8, 0x92)

} // namespace CCL

#endif // _ccl_istringdict_h
