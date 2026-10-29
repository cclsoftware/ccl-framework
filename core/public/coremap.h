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

#include "core/public/corevector.h"

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

template <typename TKey, typename TValue, typename TEntries = ConstVector<KeyValue<TKey, TValue>>>
class ConstMap
{
public:
	typedef KeyValue<TKey, TValue> Entry;
	typedef RangeIterator<TEntries, VectorIterator<Entry>, Entry&> Iterator;

	ConstMap (Entry* data, int length)
	: entries (data, length)
	{}

	/** Get number of entries. */
	int count () const
	{
		return entries.count ();
	}

	/** Check if container is empty. */
	bool isEmpty () const
	{
		return entries.isEmpty ();
	}

	/** Get value by key (operator). */
	TValue& operator [] (const TKey& key) const
	{
		return getValue (key);
	}

	/** Get value by key. */
	TValue& getValue (const TKey& key) const
	{
		Entry* result = entries.findIf ([&key] (const Entry& entry) { return entry.key == key; });
		return checked (result)->value;
	}

	/** Get value by key, returns false if key is not contained. */
	bool getValue (TValue& result, const TKey& key) const
	{
		Entry* found = entries.findIf ([&key] (const Entry& entry) { return entry.key == key; });
		if(found == nullptr)
			return false;

		result = found->value;
		return true;
	}
	
	/** Get key by value. */
	TKey& getKey (const TValue& value) const
	{
		Entry* result = entries.findIf ([&value] (const Entry& entry) { return entry.value == value; });
		return checked (result)->key;
	}

	/** Get key by value, returns false if value is not contained. */
	bool getKey (TKey& result, const TValue& value) const
	{
		Entry* found = entries.findIf ([&value] (const Entry& entry) { return entry.value == value; });
		if(found == nullptr)
			return false;

		result = found->key;
		return true;
	}
	
	/** Check if container holds given key. */
	bool contains (const TKey& key) const
	{
		Entry* result = entries.findIf ([&key] (const Entry& entry) { return entry.key == key; });
		return result != nullptr;
	}
	
	/** Check if container holds any key for given value. */
	bool containsValue (const TValue& value) const
	{
		Entry* result = entries.findIf ([&value] (const Entry& entry) { return entry.value == value; });
		return result != nullptr;
	}

	/** Iterate entries (range-for). */
	Iterator begin () const
	{
		return entries.begin ();
	}

	Iterator end () const
	{
		return entries.end ();
	}

protected:
	TEntries entries;

	ConstMap ()
	{}

	static Entry error;

	Entry* checked (Entry* result) const
	{
		ASSERT (result != nullptr)
		if(result == nullptr)
			return &error;
		
		return result;
	}
};

//************************************************************************************************
// Map
/** Mutable map container class.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template <typename TKey, typename TValue>
class Map: public ConstMap<TKey, TValue, Vector<KeyValue<TKey, TValue>>>
{
public:
	using typename ConstMap<TKey, TValue, Vector<KeyValue<TKey, TValue>>>::Entry;

	/** Construct with initial capacity and delta. */
	Map (int capacity = 0, int delta = 5)
	{
		this->entries.resize (capacity);
		this->entries.setDelta (delta);
	}

	/** Construct with initializer list. */
	Map (const InitializerList<Entry>& list)
	: Map ()
	{
		for(const Entry& entry : list)
			setValue (entry.key, entry.value);
	}

	/** Set value for key, replaces existing entry or appends a new one. */
	void setValue (const TKey& key, const TValue& value)
	{
		Entry* existing = this->entries.findIf ([&key] (const Entry& entry) { return entry.key == key; });
		if(existing)
			existing->value = value;
		else
			this->entries.add (Entry (key, value));
	}

	/** Remove entry by key. */
	bool remove (const TKey& key)
	{
		return this->entries.removeIf ([&key] (const Entry& entry) { return entry.key == key; }) > 0;
	}

	/** Remove all entries. */
	void removeAll ()
	{
		this->entries.removeAll ();
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template <typename TKey, typename TValue, typename TEntries> typename ConstMap<TKey, TValue, TEntries>::Entry ConstMap<TKey, TValue, TEntries>::error ({}, {});

} // namespace Core

#endif // _ccl_coremap_h
