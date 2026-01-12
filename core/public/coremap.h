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
// Filename    : core/public/coremap.h
// Description : Map class
//
//************************************************************************************************

#ifndef _ccl_coremap_h
#define _ccl_coremap_h

namespace Core {

//************************************************************************************************
// KeyValue
/** Template for key/value pairs.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template <typename TKey, typename TValue>
struct KeyValue
{
	TKey key;
	TValue value;
	
	KeyValue (const TKey& key = TKey (), const TValue& value = TValue ())
	: key (key),
	  value (value)
	{}

	bool operator == (const KeyValue& other) const
	{
		return other.key == key && other.value == value;
	}
};

//************************************************************************************************
// ConstMap
/** Constant map container class.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template <typename TKey, typename TValue>
class ConstMap
{
public:
	ConstMap (KeyValue<TKey, TValue>* data, int length)
	: entries (data, length)
	{}

	/** Get value by key (operator). */
	TValue& operator [] (const TKey& key) const
	{
		return at (key);
	}

	/** Get value by key. */
	TValue& at (const TKey& key) const
	{
		KeyValue<TKey, TValue>* result = entries.findIf ([&key] (const KeyValue<TKey, TValue>& entry) { return entry.key == key; });
		return checked (result)->value;
	}
	
	/** Find key by value. */
	TKey& findKey (const TValue& value) const
	{
		KeyValue<TKey, TValue>* result = entries.findIf ([&value] (const KeyValue<TKey, TValue>& entry) { return entry.value == value; });
		return checked (result)->key;
	}
	
	/** Check if container holds given key. */
	bool contains (const TKey& key) const
	{
		KeyValue<TKey, TValue>* result = entries.findIf ([&key] (const KeyValue<TKey, TValue>& entry) { return entry.key == key; });
		return result != nullptr;
	}
	
private:
	ConstVector<KeyValue<TKey, TValue>> entries;
	
	static KeyValue<TKey, TValue> error;
	
	KeyValue<TKey, TValue>* checked (KeyValue<TKey, TValue>* result) const
	{
		ASSERT (result != nullptr)
		if(result == nullptr)
			return &error;
		
		return result;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename TKey, typename TValue> KeyValue<TKey, TValue> ConstMap<TKey, TValue>::error ({}, {});

} // namespace Core

#endif // _ccl_coremap_h
