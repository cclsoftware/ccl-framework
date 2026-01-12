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
// Filename    : core/portable/coreattributes.cpp
// Description : Attribute list class
//
//************************************************************************************************

#include "coreattributes.h"
#include "corepool.h"

#include "core/system/coredebug.h"

using namespace Core;
using namespace Portable;

//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CORE_ATTRIBUTE_POOL_DISABLED // can be disabled in project settings
	#define ATTRIBUTE_POOL_ENABLED 1
#endif

static const int kAttrVectorSize = 0; // do not allocate memory first (when attributes are not used, it does not have to be freed again)
static const int kAttrVectorDelta = 20; 

//************************************************************************************************
// Attribute classes using memory pools
//************************************************************************************************
#if ATTRIBUTE_POOL_ENABLED

class AttributeValue2: public AttributeValue,
					   public PooledObject<AttributeValue2, CoreMemoryPool>
{
public:
	AttributeValue2 ()
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

class Attribute2: public Attribute,
				  public PooledObject<Attribute2, CoreMemoryPool>
{
public:
	Attribute2 (AttrID id = nullptr, int flags = 0)
	: Attribute (id, flags)
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

class Attributes2: public Attributes,
				   public PooledObject<Attributes2, CoreMemoryPool>
{
public:
	#if 0
	Attributes2 (AttributeAllocator& allocator)
	: Attributes (allocator)
	{}
	#else // workaround template issue with reference
	Attributes2 (AttributeAllocator* allocator)
	: Attributes (*allocator)
	{}
	#endif
};

//////////////////////////////////////////////////////////////////////////////////////////////////

class AttributeQueue2: public AttributeQueue,
					   public PooledObject<AttributeQueue2, CoreMemoryPool>
{
public:
};

static const int kAttrPoolSize = 1000;
static const int kAttrMediumPoolSize = 4000;
static const int kAttrLargePoolSize = 10000;
static const int kAttrSmallStringSize = 32;

DEFINE_OBJECTPOOL_SIZE (Attribute2, CoreMemoryPool, kAttrLargePoolSize)
DEFINE_OBJECTPOOL_SIZE (AttributeValue2, CoreMemoryPool, kAttrPoolSize)
DEFINE_OBJECTPOOL_SIZE (Attributes2, CoreMemoryPool, kAttrMediumPoolSize)
DEFINE_OBJECTPOOL_SIZE (AttributeQueue2, CoreMemoryPool, kAttrPoolSize)
static CoreMemoryPool gSmallStringPool (kAttrSmallStringSize, kAttrPoolSize, "SmallStringPool");

#endif // ATTRIBUTE_POOL_ENABLED

//************************************************************************************************
// AttributeFactory
//************************************************************************************************

namespace AttributeFactory
{
	static bool poolDisabled = false;

	#if ATTRIBUTE_POOL_ENABLED
	template <class NonPoolClass, class PoolClass>
	NonPoolClass* allocate ()
	{
		if(poolDisabled == false)
			if(NonPoolClass* obj = PoolClass::pool_new ())
				return obj;
		return NEW NonPoolClass;	
	}

	template <class NonPoolClass, class PoolClass, typename ArgType>
	NonPoolClass* allocate (const ArgType& arg)
	{
		if(poolDisabled == false)
			if(NonPoolClass* obj = PoolClass::pool_new (arg))
				return obj;
		return NEW NonPoolClass (arg);	
	}

	template <class NonPoolClass, class PoolClass, typename ArgType1, typename ArgType2>
	NonPoolClass* allocate (const ArgType1& arg1, const ArgType2& arg2)
	{
		if(poolDisabled == false)
			if(NonPoolClass* obj = PoolClass::pool_new (arg1, arg2))
				return obj;
		return NEW NonPoolClass (arg1, arg2);
	}
	#endif
	
	static INLINE AttributeValue* newAttributeValue ()
	{ 
		#if ATTRIBUTE_POOL_ENABLED
		return allocate<AttributeValue, AttributeValue2> (); 
		#else
		return NEW AttributeValue;
		#endif
	}

	static INLINE Attributes* newAttributes (AttributeAllocator& allocator)
	{
		#if ATTRIBUTE_POOL_ENABLED
		// compiler complains when using reference here :-/
		//return allocate<Attributes, Attributes2> (allocator);
		if(poolDisabled == false)
			if(Attributes* obj = Attributes2::pool_new (&allocator))
				return obj;
		return NEW Attributes (allocator);
		#else
		return NEW Attributes (allocator);
		#endif
	}

	static INLINE AttributeQueue* newAttributeQueue ()
	{
		#if ATTRIBUTE_POOL_ENABLED
		return allocate<AttributeQueue, AttributeQueue2> ();
		#else
		return NEW AttributeQueue;
		#endif
	}

	enum Flags
	{
		kIsPoolString = 1<<(AttributeValue::kLastUserFlag + 1)
	};

	static char* newString (int size, short& type)
	{
		#if ATTRIBUTE_POOL_ENABLED
		if(poolDisabled == false && size <= kAttrSmallStringSize)
			if(void* ptr = gSmallStringPool.newBlock ())
			{
				type |= kIsPoolString;
				return (char*)ptr;
			}
		#endif

		return NEW char[size];
	}

	static void freeString (char* string, short type)
	{
		#if ATTRIBUTE_POOL_ENABLED
		if(type & kIsPoolString)
			gSmallStringPool.deleteBlock (string);
		else
		#endif
			delete [] string;
	}
}

//************************************************************************************************
// AttributePoolSuspender
//************************************************************************************************

AttributePoolSuspender::AttributePoolSuspender ()
: wasDisabled (AttributeFactory::poolDisabled)
{
	if(wasDisabled == false)
		AttributeFactory::poolDisabled = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AttributePoolSuspender::~AttributePoolSuspender ()
{
	if(wasDisabled == false)
		AttributeFactory::poolDisabled = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributePoolSuspender::dump (CStringPtr label)
{
	#if ATTRIBUTE_POOL_ENABLED
	DebugPrintf ("Attribute pool utilization %s: %.2f %.2f %.2f %.2f (strings %.2f)\n",
				 label,
				 Attribute2::getPool ().getBlockUtilization (),
				 AttributeValue2::getPool ().getBlockUtilization (),
				 Attributes2::getPool ().getBlockUtilization (),
				 AttributeQueue2::getPool ().getBlockUtilization (),
				 gSmallStringPool.getBlockUtilization ());
	#endif
}

//************************************************************************************************
// AttributeAllocator
//************************************************************************************************

AttributeAllocator& AttributeAllocator::getDefault ()
{
	class DefaultAllocator: public AttributeAllocator
	{
		Attribute* allocateAttribute (AttrID id, int flags) override
		{
			#if ATTRIBUTE_POOL_ENABLED
			return AttributeFactory::allocate<Attribute, Attribute2> (id, flags); 
			#else
			return NEW Attribute (id, flags);
			#endif
		}

		void deallocateAttribute (Attribute* a) override
		{
			delete a;
		}
	};

	static DefaultAllocator defaultAllocator;
	return defaultAllocator;
}

//************************************************************************************************
// AttributeBufferAllocator
//************************************************************************************************

class AttributeBufferAllocator::BufferAttribute: public Attribute
{
public:
	BufferAttribute (AttrID id = nullptr, int flags = 0)
	: Attribute (id, flags)
	{}

	void* operator new (size_t size, void* slot)
	{
		return slot;
	}

	void operator delete (void* ptr)
	{}

	void operator delete (void* ptr, void* slot)
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

AttributeBufferAllocator::AttributeBufferAllocator (int capacity)
: attributeBuffer (capacity * sizeof(Attribute), false),
  capacity (capacity),
  total (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attribute* AttributeBufferAllocator::allocateAttribute (AttrID id, int flags)
{
	if(total < capacity)
	{
		void* slot = (void*)getSlot (++total);
		return new (slot) BufferAttribute (id, flags);
	}
	else
	{
		// fall back to heap allocation (slow)
		#if DEBUG
		Core::DebugPrintf ("BufferAttributeAllocator: capacity %d exceeded!\n", capacity);
		#endif
		return NEW Attribute (id, flags);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeBufferAllocator::deallocateAttribute (Attribute* a)
{
	if(a >= getSlot (0) && a <= getSlot (capacity - 1))
		a->clear (); // inside buffer (no dtor): release sub attributes, queues, strings
	else
		delete a; // outside of buffer: must be on heap
}

//************************************************************************************************
// AttributeQueue
//************************************************************************************************

AttributeQueue::AttributeQueue ()
: queue (kAttrVectorSize, kAttrVectorDelta)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AttributeQueue::~AttributeQueue ()
{
	removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeQueue::removeAll ()
{
	VectorForEachFast (queue, AttributeValue*, a)
		delete a;
	EndFor

	queue.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeQueue::copyFrom (const AttributeQueue& other, AttributeAllocator& allocator)
{
	removeAll ();
	VectorForEachFast (other.queue, AttributeValue*, a)
		AttributeValue* a2 = AttributeFactory::newAttributeValue ();
		a2->copyFrom (*a, allocator);
		queue.add (a2);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeQueue::moveTo (AttributeQueue& other)
{
	other.queue.resize (other.queue.count () + queue.count ());
	VectorForEachFast (queue, AttributeValue*, a)
		other.queue.add (a);
	EndFor
	queue.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeQueue::makeReal ()
{
	VectorForEachFast (queue, AttributeValue*, a)
		a->makeReal ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AttributeValue& AttributeQueue::appendValue ()
{
	AttributeValue* a = AttributeFactory::newAttributeValue ();
	queue.add (a);
	return *a;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeQueue::append (int64 value)
{
	appendValue ().set (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeQueue::append (int value)
{
	appendValue ().set ((int64)value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeQueue::append (double value)
{
	appendValue ().set (value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeQueue::append (CStringPtr value, bool shared)
{
	appendValue ().set (value, shared);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AttributeQueue* AttributeQueue::appendQueue ()
{
	return appendValue ().makeQueue ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes* AttributeQueue::appendAttributes (AttributeAllocator& allocator)
{
	return appendValue ().makeAttributes (allocator);
}

//************************************************************************************************
// AttributeValue
//************************************************************************************************

AttributeValue::AttributeValue ()
: type (0), lValue (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AttributeValue::~AttributeValue ()
{
	clear ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeValue::copyFrom (const AttributeValue& other, AttributeAllocator& allocator)
{
	switch(other.getType ())
	{
	case kFloat:
		set (other.fValue);
		break;

	case kString:
		set (other.string, (other.type & kShared) != 0);
		break;

	case kQueue:
		if(other.queue)
			set (*other.queue, allocator);
		else
		{
			clear ();
			type = kQueue;
		}
		break;

	case kAttributes:
		if(other.attributes)
			set (*other.attributes, allocator);
		else
		{
			clear ();
			type = kAttributes;
		}
		break;

	default :
		set (other.lValue);
		break;
	}

	type |= (other.type & kUserFlagMask);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeValue::makeReal ()
{
	switch(getType ())
	{
	case kString:
		if(type & kShared) // make real copy of string
		{
			int size = ConstString (string).length ();
			if(size > 0)
			{
				size++;
				CStringPtr oldString = string;
				string = AttributeFactory::newString (size, type);
				::memcpy (string, oldString, size);
			}
			else
				string = nullptr;
			type &= ~kShared;
		}
		break;

	case kQueue:
		if(queue)
			queue->makeReal ();
		break;

	case kAttributes:
		if(attributes)
			attributes->makeReal ();
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeValue::clear ()
{
	switch(type & (kTypeMask|kShared))
	{
	case kString:
		AttributeFactory::freeString (string, type);
		break;

	case kString|kShared:
		break;

	case kQueue:
		delete queue;
		break;

	case kAttributes:
		delete attributes;
		break;
	}

	type &= kUserFlagMask;
	lValue = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION
void AttributeValue::set (int64 value)
{
	clear ();

	type |= kInt;
	lValue = value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION
void AttributeValue::set (double value)
{
	clear ();

	type |= kFloat;
	fValue = value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION
void AttributeValue::set (CStringPtr value, bool shared)
{
	clear ();

	type |= kString;
	if(shared)
	{
		string = const_cast<char*> (value);
		type |= kShared;
	}
	else
	{
		int size = ConstString (value).length ();
		if(size > 0)
		{
			size++;
			string = AttributeFactory::newString (size, type);
			::memcpy (string, value, size);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeValue::set (const AttributeQueue& queue, AttributeAllocator& allocator)
{
	AttributeQueue* newQueue = makeQueue ();
	if(newQueue)
		newQueue->copyFrom (queue, allocator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AttributeValue::set (const Attributes& attributes, AttributeAllocator& allocator)
{
	Attributes* a2 = makeAttributes (allocator);
	if(a2)
		a2->copyFrom (attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes* AttributeValue::makeAttributes (AttributeAllocator& allocator)
{
	clear ();

	type |= kAttributes;
	attributes = AttributeFactory::newAttributes (allocator);
	return attributes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes* AttributeValue::detachAttributes ()
{
	Attributes* a = getAttributes ();
	attributes = nullptr;
	return a;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AttributeQueue* AttributeValue::makeQueue ()
{
	clear ();

	type |= kQueue;
	queue = AttributeFactory::newAttributeQueue ();
	return queue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AttributeValue::getEstimatedSize (bool deep) const
{
	int size = 0;
	switch(getType ())
	{
	case kInt :
	case kFloat :
		size = 8;
		break;
	case kString :
		size = ConstString (string).length () + 2;
		break;
	case kQueue:
		if(queue)
			VectorForEachFast (queue->getValues (), AttributeValue*, value)
				size += value->getEstimatedSize (deep);
			EndFor
		break;
	case kAttributes :
		if(attributes && deep == true)
			size += attributes->getEstimatedSize (deep);
		break;
	}
	return size;
}

//************************************************************************************************
// Attribute
//************************************************************************************************

Attribute::Attribute (AttrID _id, int flags)
{
	if(flags & kShareID)
		id = _id;
	else
	{
		// copy string
		idBuffer = _id;
		id = idBuffer.str ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Attribute::copyFrom (const Attribute& other, AttributeAllocator& allocator)
{
	AttributeValue::copyFrom (other, allocator);

	if(!other.id.isEmpty ())
	{
		if(other.idBuffer.isEmpty ()) // not copied
			id = other.id;
		else
		{
			idBuffer = other.id;
			id = idBuffer.str ();
		}
	}
	else
	{
		static const ConstString emptyId;
		id = emptyId;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Attribute::makeReal ()
{
	AttributeValue::makeReal ();

	if(idBuffer.isEmpty ()) // make real copy of id
	{
		idBuffer = id;
		id = idBuffer.str ();
	}
}

//************************************************************************************************
// Attributes
//************************************************************************************************

Attributes::Attributes (AttributeAllocator& _allocator)
: allocator (_allocator),
  list (kAttrVectorSize, kAttrVectorDelta),
  inplaceBuffer (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes::Attributes (const Attributes& other)
: allocator (other.allocator),
  list (kAttrVectorSize, kAttrVectorDelta),
  inplaceBuffer (nullptr)
{
	copyFrom (other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes::~Attributes ()
{
	removeAll ();

	if(inplaceBuffer)
		delete inplaceBuffer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Attributes::copyFrom (const Attributes& other)
{
	removeAll ();
	VectorForEachFast (other.list, Attribute*, a)
		addAttribute (*a);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Attributes::makeReal ()
{
	VectorForEachFast (list, Attribute*, a)
		a->makeReal ();
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AttrID Attributes::getAttributeName (int index) const
{
	if(Attribute* a = list.at (index))
		return a->getID ();

	return "";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 Attributes::getInt (AttrID id) const
{
	const Attribute* a = lookup (id);
	return a ? a->getInt () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

double Attributes::getFloat (AttrID id) const
{
	const Attribute* a = lookup (id);
	return a ? a->getFloat () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION
CStringPtr Attributes::getString (AttrID id) const
{
	const Attribute* a = lookup (id);
	return a ? a->getString () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const AttributeQueue* Attributes::getQueue (AttrID id) const
{
	const Attribute* a = lookup (id);
	return a ? a->getQueue () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes* Attributes::getAttributes (AttrID id) const
{
	const Attribute* a = lookup (id);
	return a ? a->getAttributes () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Attributes::remove (AttrID id)
{
	int idx = getAttributeIndex (id);
	if(idx >= 0)
	{
		Attribute* a = list[idx];
		list.removeAt (idx);
		allocator.deallocateAttribute (a);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Attributes::removeAll ()
{
	VectorForEachFast (list, Attribute*, a)
		allocator.deallocateAttribute (a);
	EndFor

	list.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION
const Attribute* Attributes::lookup (AttrID _id) const
{
	ConstString id (_id); // check for 0 once, not for every attribute

	VectorForEachFast (list, Attribute*, a)
		if(a->getID () == id)
			return a;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Attributes::getAttributeIndex (AttrID _id) const
{
	ConstString id (_id); // check for 0 once, not for every attribute

	int idx = 0;
	VectorForEachFast (list, Attribute*, a)
		if(a->getID () == id)
			return idx;
		++idx;
	EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attribute& Attributes::lookupOrAdd (AttrID _id)
{
	ConstString id (_id); // check for 0 once, not for every attribute

	VectorForEachFast (list, Attribute*, a)
		if(a->getID () == id)
			return *a;
	EndFor

	return addNew (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION
Attribute& Attributes::addNew (AttrID id, int flags)
{
	Attribute* a = allocator.allocateAttribute (id, flags);
	list.add (a);
	return *a;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CORE_HOT_FUNCTION
void Attributes::addAttribute (const Attribute& a)
{
	Attribute* a2 = allocator.allocateAttribute ();
	a2->copyFrom (a, allocator);
	list.add (a2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Attributes::getEstimatedSize (bool deep) const
{
	int size = 0;
	VectorForEachFast (list, Attribute*, a)
		size += a->getID ().length () + 4;
		size += a->getEstimatedSize (deep);
	EndFor
	return size;
}
