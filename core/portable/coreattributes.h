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
// Filename    : core/portable/coreattributes.h
// Description : Attribute list class
//
//************************************************************************************************

#ifndef _coreattributes_h
#define _coreattributes_h

#include "core/public/corevector.h"
#include "core/public/corestringbuffer.h"
#include "core/public/coremacros.h"
#include "core/public/corebuffer.h"

#include "core/text/coreattributehandler.h"

namespace Core {
namespace Portable {

class Attributes;
class AttributeQueue;
class AttributeAllocator;

typedef CStringPtr AttrID;

//************************************************************************************************
// AttributeValue
/** Class to hold a value of different types (int, float, string, etc.) \ingroup core_portable */
//************************************************************************************************

class AttributeValue
{
public:
	enum Types
	{
		kInt = 1,
		kFloat,
		kString,
		kQueue,			// array of (anonymous) values
		kAttributes,	// nested attributes list
		kTypeMask = 0x7F,

		kShared	= 1<<7,	// shared string (no copy)

		kUserFlag1 = 1<<8,
		kUserFlag2 = 1<<9,

		kLastUserFlag = 9,
		kUserFlagMask = kUserFlag1|kUserFlag2
	};

	AttributeValue ();
	virtual ~AttributeValue ();

	void copyFrom (const AttributeValue& other, AttributeAllocator& allocator);
	void makeReal ();

	short getType () const;
	PROPERTY_FLAG (type, kUserFlag1, isUserFlag1)
	PROPERTY_FLAG (type, kUserFlag2, isUserFlag2)

	void clear ();
	void set (int64 value);
	void set (double value);
	void set (CStringPtr value, bool shared = false);
	void set (const AttributeQueue& queue, AttributeAllocator& allocator);
	void set (const Attributes& attributes, AttributeAllocator& allocator);
	AttributeQueue* makeQueue ();
	Attributes* makeAttributes (AttributeAllocator& allocator);
	Attributes* detachAttributes ();

	int64 getInt () const;
	double getFloat () const;
	CStringPtr getString () const;
	const AttributeQueue* getQueue () const;
	Attributes* getAttributes () const;

	int getEstimatedSize (bool deep = true) const;

protected:
	short type;
	union
	{
		int64 lValue;
		double fValue;
		char* string;
		AttributeQueue* queue;
		Attributes* attributes;
	};

private:
	AttributeValue (const AttributeValue&) { ASSERT (0) }
	AttributeValue& operator = (const AttributeValue&) { ASSERT (0) return *this; }
};

//************************************************************************************************
// Attribute
/** Attribute value with key string \ingroup core_portable */
//************************************************************************************************

class Attribute: public AttributeValue
{
public:
	enum Flags
	{
		kShareID	= AttributeHandler::kInplace,		///< share attribute identifier (no copy)
		kShareValue	= AttributeHandler::kInplaceValue	///< share attribute string value (no copy)
	};

	Attribute (AttrID id = nullptr, int flags = 0);

	void copyFrom (const Attribute& other, AttributeAllocator& allocator);
	void makeReal ();

	const ConstString& getID () const;

protected:
	CString32 idBuffer;
	ConstString id;

private:
	Attribute (const Attribute&) : AttributeValue () { ASSERT (0) }
	Attribute& operator = (const Attribute&) { ASSERT (0) return *this; }
};

//************************************************************************************************
// AttributeAllocator
//************************************************************************************************

class AttributeAllocator
{
public:
	virtual ~AttributeAllocator () {}
	virtual Attribute* allocateAttribute (AttrID id = nullptr, int flags = 0) = 0;
	virtual void deallocateAttribute (Attribute* a) = 0;
	static AttributeAllocator& getDefault ();
};

//************************************************************************************************
// Attributes
/** Container for values of different types identified by a key string \ingroup core_portable */
//************************************************************************************************

class Attributes
{
public:
	Attributes (AttributeAllocator& allocator);
	Attributes (const Attributes& other);
	virtual ~Attributes ();

	AttributeAllocator& getAllocator () const;
	void copyFrom (const Attributes& other);
	void makeReal ();

	bool isEmpty () const;
	int countAttributes () const;
	AttrID getAttributeName (int index) const;
	const Attribute* getAttribute (int index) const;
	const Attribute* lookup (AttrID id) const;
	int getAttributeIndex (AttrID id) const;
	bool contains (AttrID id) const;
	bool remove (AttrID id);
	void removeAll ();

	// Write Attributes
	void set (AttrID id, int64 value);
	void set (AttrID id, int value);
	void set (AttrID id, double value);
	void set (AttrID id, CStringPtr value, bool shared = false);
	void set (AttrID id, const Attributes& attributes);
	AttributeQueue* makeQueue (AttrID id);
	Attributes* makeAttributes (AttrID id);

	void add (AttrID id, int64 value, int flags = 0);
	void add (AttrID id, int value, int flags = 0);
	void add (AttrID id, double value, int flags = 0);
	void add (AttrID id, CStringPtr value, int flags = 0);
	void add (AttrID id, const Attributes& attributes, int flags = 0);
	AttributeQueue* addQueue (AttrID id, int flags = 0);
	Attributes* addAttributes (AttrID id, int flags = 0);
	void addAttribute (const Attribute& a);

	// Read Attributes
	int64 getInt (AttrID id) const;
	double getFloat (AttrID id) const;
	CStringPtr getString (AttrID id) const;
	const AttributeQueue* getQueue (AttrID id) const;
	Attributes* getAttributes (AttrID id) const;

	// optional buffer for inplace parsing to be kept alive with this instance
	PROPERTY_POINTER (IO::Buffer, inplaceBuffer, InplaceBuffer)

	int getEstimatedSize (bool deep = true) const;

private:
	AttributeAllocator& allocator;
	Vector<Attribute*> list;

	friend class AttributesIterator;

	Attribute& lookupOrAdd (AttrID id);
	Attribute& addNew (AttrID id, int flags = 0);

	Attributes& operator = (const Attributes&) { ASSERT (0) return *this; }
};

//************************************************************************************************
// AttributeQueue
/** Array (or queue) of attribute values \ingroup core_portable */
//************************************************************************************************

class AttributeQueue
{
public:
	AttributeQueue ();
	virtual ~AttributeQueue ();

	void copyFrom (const AttributeQueue& other, AttributeAllocator& allocator);
	void moveTo (AttributeQueue& other);
	void makeReal ();
	void removeAll ();

	void append (int64 value);
	void append (int value);
	void append (double value);
	void append (CStringPtr value, bool shared = false);
	AttributeQueue* appendQueue ();
	Attributes* appendAttributes (AttributeAllocator& allocator);

	const ConstVector<AttributeValue*>& getValues () const;

protected:
	Vector<AttributeValue*> queue;

	AttributeValue& appendValue ();
};

//************************************************************************************************
// AttributePoolSuspender
/** Helper to temporarily disable memory pool for attributes. Disables pool for all threads, use caution! */
//************************************************************************************************

struct AttributePoolSuspender
{
	AttributePoolSuspender ();
	~AttributePoolSuspender ();

	static void dump (CStringPtr label = "");

private:
	bool wasDisabled;
};

//************************************************************************************************
// AttributesIterator
/** Fast Attributes iterator (like VectorForEachFast). Do not add/remove Attributes during iteration. */
//************************************************************************************************

class AttributesIterator
{
public:
	AttributesIterator (const Attributes& attributes);
	AttributesIterator (const AttributesIterator& iter); // iterate the remaining items of other iterator

	Attribute* next ();

private:
	Attribute** current;
	int numRemaining;
};

//************************************************************************************************
// AttributeBufferAllocator
/** Pre-allocates a  memory buffer that can hold the given number of attributes. 
	If more than capacity attributes are requested, the remaining ones will be allocated on the heap!
	Deallocating attributes does not reclaim space in the buffer! */
//************************************************************************************************

class AttributeBufferAllocator: public AttributeAllocator
{
public:
	AttributeBufferAllocator (int capacity);

	// AttributeAllocator
	Attribute* allocateAttribute (AttrID id, int flags) override;
	void deallocateAttribute (Attribute* a) override;

private:
	IO::Buffer attributeBuffer;
	int capacity;	// max number of attributes
	int total;		// current number of allocated attributes in buffer

	class BufferAttribute;
	Attribute* getSlot (int index);
};

//************************************************************************************************
// PreAllocatedAttributes
/** Attributes that own an AttributeBufferAllocator of given capacity. */
//************************************************************************************************

class PreAllocatedAttributes: public Attributes
{
public:
	PreAllocatedAttributes (int capacity)
	: Attributes (allocator),
	  allocator (capacity)
	{}
	
	~PreAllocatedAttributes ()
	{
		removeAll (); // remove attributes before buffer is deallocated
	}

	AttributeBufferAllocator allocator;
};

//************************************************************************************************
// DefaultAllocatedAttributes
//************************************************************************************************

class DefaultAllocatedAttributes: public Attributes
{
public:
	DefaultAllocatedAttributes ()
	: Attributes (AttributeAllocator::getDefault ())
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline AttributeAllocator& Attributes::getAllocator () const { return allocator; }

inline bool Attributes::isEmpty () const { return list.isEmpty (); }
inline int Attributes::countAttributes () const	{ return list.count (); }
inline bool Attributes::contains (AttrID id) const { return lookup (id) != nullptr; }
inline const Attribute* Attributes::getAttribute (int i) const { return (Attribute*)list.at (i); }

inline void Attributes::set (AttrID id, int64 value) { lookupOrAdd (id).set (value); }
inline void Attributes::set (AttrID id, int value) { lookupOrAdd (id).set ((int64)value); }
inline void Attributes::set (AttrID id, double value) { lookupOrAdd (id).set (value); }
inline void Attributes::set (AttrID id, CStringPtr value, bool shared) { lookupOrAdd (id).set (value, shared); }
inline void Attributes::set (AttrID id, const Attributes& attributes) { lookupOrAdd (id).set (attributes, allocator); }
inline AttributeQueue* Attributes::makeQueue (AttrID id) { return lookupOrAdd (id).makeQueue (); }
inline Attributes* Attributes::makeAttributes (AttrID id) { return lookupOrAdd (id).makeAttributes (allocator); }

inline void Attributes::add (AttrID id, int64 value, int flags) { addNew (id, flags).set (value); }
inline void Attributes::add (AttrID id, int value, int flags) { addNew (id, flags).set ((int64)value); }
inline void Attributes::add (AttrID id, double value, int flags) { addNew (id, flags).set (value); }
inline void Attributes::add (AttrID id, CStringPtr value, int flags) { addNew (id, flags).set (value, (flags & Attribute::kShareValue) != 0); }
inline void Attributes::add (AttrID id, const Attributes& attributes, int flags) { addNew (id, flags).set (attributes, allocator); }
inline AttributeQueue* Attributes::addQueue (AttrID id, int flags) { return addNew (id, flags).makeQueue (); }
inline Attributes* Attributes::addAttributes (AttrID id, int flags) { return addNew (id, flags).makeAttributes (allocator); }

inline short AttributeValue::getType () const { return (type & kTypeMask); }
inline int64 AttributeValue::getInt () const { return getType () == kInt ? lValue : getType () == kFloat ? (int64)fValue : 0; }
inline double AttributeValue::getFloat () const { return getType () == kFloat ? fValue : getType () == kInt ? (double)lValue : 0.0; }
inline CStringPtr AttributeValue::getString () const { return getType () == kString ? string : nullptr; }
inline Attributes* AttributeValue::getAttributes () const { return getType () == kAttributes ? attributes : nullptr; }
inline const AttributeQueue* AttributeValue::getQueue () const { return getType () == kQueue ? queue : nullptr; }

inline const ConstString& Attribute::getID () const { return id; }

inline const ConstVector<AttributeValue*>& AttributeQueue::getValues () const { return queue; }

inline AttributesIterator::AttributesIterator (const Attributes& attributes)
: current (attributes.list.getItems ()), numRemaining (attributes.list.count ())
{}

inline AttributesIterator::AttributesIterator (const AttributesIterator& iter)
: current (iter.current), numRemaining (iter.numRemaining)
{}

inline Attribute* AttributesIterator::next ()
{
	if(numRemaining)
	{
		numRemaining--;
		return *current++;
	}
	return nullptr;
}

inline Attribute* AttributeBufferAllocator::getSlot (int index) { return attributeBuffer.as<Attribute> () + index; }

} // namespace Portable
} // namespace Core

#endif // _coreattributes_h
