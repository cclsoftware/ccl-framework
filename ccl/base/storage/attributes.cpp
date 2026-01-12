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
// Filename    : ccl/base/storage/attributes.cpp
// Description : Attribute List
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/base/storage/storage.h"

#if 1
// Attribute keys are often the same, should be more efficient to reuse string memory.
#define GetAttrIDString(id) System::GetConstantCString (id)
#else
#define GetAttrIDString(id) id
#endif

using namespace CCL;

//************************************************************************************************
// AttributeFilter
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AttributeFilter, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

AttributeFilter::AttributeFilter (const CString idList[], int idCount, FilterType type)
: idList (idList),
  idCount (idCount),
  type (type)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AttributeFilter::getCount () const
{
	return idCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID AttributeFilter::getId (int index) const
{
	ASSERT (index >= 0 && index < idCount)
	if(index >= 0 && index < idCount)
		 return idList[index];
	return CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AttributeFilter::matches (StringID id) const
{
	for(int i = 0; i < idCount; i++)
		if(idList[i] == id)
			return type == kInclude ? true : false;

	return type == kInclude ? false : true;
}

//************************************************************************************************
// AttributeQueue
//************************************************************************************************

DEFINE_CLASS (AttributeQueue, AttributeContainer)
DEFINE_CLASS_UID (AttributeQueue, 0xa0303add, 0x0e4f, 0x4557, 0x8c, 0xa4, 0xae, 0x3e, 0xcc, 0x42, 0xc4, 0x93)
DEFINE_CLASS_NAMESPACE (AttributeQueue, NAMESPACE_CCL)
DEFINE_CLASS_CATEGORY (AttributeQueue, "System")

//////////////////////////////////////////////////////////////////////////////////////////////////

AttributeQueue::AttributeQueue ()
: iter (nullptr)
{
	objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AttributeQueue::~AttributeQueue ()
{
	if(iter)
		delete iter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AttributeQueue::addValue (VariantRef value, int flags)
{
	Attribute* a = NEW Attribute;
	a->set (value, flags);
	add (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attribute* AttributeQueue::unqueueNext ()
{
	if(!iter)
		iter = newIterator ();

	Attribute* a = iter ? (Attribute*)iter->next () : nullptr;
	if(a)
	{
		remove (a);
		iter->previous (); // (problem with array iterator when removing items)
	}
	return a;
}

//************************************************************************************************
// AttributeQueue::ContentIterator
//************************************************************************************************

class AttributeQueue::ContentIterator: public IteratorDelegate
{
public:
	ContentIterator (Iterator* iter, MetaClassRef typeId)
	: IteratorDelegate (iter),
	  typeId (typeId)
	{}

	Object* resolve (Object* obj)
	{
		Attribute* attr = ccl_cast<Attribute> (obj);
		Object* valueObj = attr ? unknown_cast<Object> (attr->getValue ().asUnknown ()) : nullptr;
		return valueObj && valueObj->canCast (typeId) ? valueObj : nullptr;
	}

	// IteratorDelegate
	Object* next () override { return resolve (iterator->next ()); }
	Object* previous () override { return resolve (iterator->previous ()); }

protected:
	MetaClassRef typeId;
};

//************************************************************************************************
// Attribute
//************************************************************************************************

DEFINE_CLASS (Attribute, Object)
DEFINE_CLASS_NAMESPACE (Attribute, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

Attribute::Attribute (StringID id)
: id (GetAttrIDString (id))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attribute::Attribute (const Attribute& a)
: id (a.id)
{
	// force copy if owned by incoming attribute
	set (a.value, a.isOwner () ? Attributes::kTemp : 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Attribute::set (VariantRef _value, int flags)
{
	value = _value;

	if(value.getType () == Variant::kString)
		value.share ();
	else
	if(value.getType () == Variant::kObject)
	{
		if((flags & Attributes::kOwns) != 0)
			value.type |= Variant::kShared;
		else
		if((flags & Attributes::kTemp) != 0)
		{
			Object* obj = unknown_cast<Object> (value);
			if(obj)
			{
				obj = obj->clone ();
				ASSERT (obj != nullptr)
				value = ccl_as_unknown (obj);
				value.type |= Variant::kShared;
			}
			else
				value.share ();
		}
		else
		if((flags & Attributes::kShare) != 0)
			value.share ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VariantRef Attribute::detachValue ()
{
	if(value.getType () == Variant::kObject)
		value.type &= ~Variant::kShared;
	return value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Attribute::load (const Storage& storage)
{
	id = storage.getAttributes ().getCString ("id");
	Variant temp;
	storage.getAttributes ().getAttribute (temp, "value");
	set (temp, Attributes::kShare);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Attribute::save (const Storage& storage) const
{
	storage.getAttributes ().set ("id", id);
	storage.getAttributes ().setAttribute ("value", value);
	return true;
}

//************************************************************************************************
// Attributes
//************************************************************************************************

DEFINE_CLASS (Attributes, Object)
DEFINE_CLASS_UID (Attributes, 0x138ed4e7, 0x1786, 0x4a9d, 0xa0, 0xef, 0xd7, 0x6f, 0xba, 0x49, 0xb4, 0xda)
DEFINE_CLASS_NAMESPACE (Attributes, NAMESPACE_CCL)
DEFINE_CLASS_CATEGORY (Attributes, "System")

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes::Attributes ()
{
	list.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Attributes::makeValidKey (MutableCString& keyString) const
{
	CString inputKey (keyString);
	CStringWriter<512> writer (keyString);
	int length = inputKey.length ();
	for(int i = 0; i < length; i++)
		if(CString::isAlphaNumeric (inputKey[i]))
			writer.append (inputKey[i]);
		else
			writer.append ('_');
	writer.flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes::Attributes (const Attributes& other)
{
	list.objectCleanup ();
	copyFrom (other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Attributes::copyFrom (const Attributes& a)
{
	removeAll ();
	list.add (a.list, Container::kClone);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Attributes::copyFrom (const IAttributeList& list)
{
	Attributes* a = unknown_cast<Attributes> (&list);
	if(a)
		copyFrom (*a);
	else
	{
		// copy attributes from foreign module
		removeAll ();
		ForEachAttribute (list, name, value)
			if(value.isObject ())
				if(UnknownPtr<IAttributeList> nestedList = value.asUnknown ())
				{
					// recursion for nested attribute lists
					Attributes* listCopy = NEW Attributes;
					listCopy->copyFrom (*nestedList);
					set (name, listCopy, Attributes::kOwns);
					continue;
				}

			setAttribute (name, value, Attributes::kShare);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Attributes::addFrom (const IAttributeList& list, const IAttributeFilter* filter)
{
	ForEachAttribute (list, name, value)
		if(!filter || (filter && filter->matches (name)))
			setAttribute (name, value, Attributes::kShare);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Attributes::load (const Storage& storage)
{
	copyFrom (storage.getAttributes ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Attributes::save (const Storage& storage) const
{
	storage.getAttributes ().copyFrom (*this);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Attributes::equals (const Object& obj) const
{
	auto other = ccl_cast<Attributes> (&obj);
	if(!other)
		return false;
	
	int count = other->countAttributes ();
	if(countAttributes () != count)
		return false;

	for(int i = 0; i < count; i++)
	{
		MutableCString thisName, otherName;
		getAttributeName (thisName, i);
		other->getAttributeName (otherName, i);
		if(thisName != otherName)
			return false;

		Variant thisValue, otherValue;
		getAttributeValue (thisValue, i);
		other->getAttributeValue (otherValue, i);
		if(thisValue != otherValue)
			return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Attributes::setHexValue (StringID id, uint32 value)
{
	String temp;
	temp.appendHexValue ((int64)value, 8); // %08X
	return set (id, temp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

uint32 Attributes::getHexValue (StringID id) const
{
	int64 hexValue = 0;
	Variant var;
	if(getAttribute (var, id))
	{
		if(var.isString ())
			var.asString ().getHexValue (hexValue);
		else // read as integer for backwards compatibility
			hexValue = var.asInt ();
	}
	return (uint32)hexValue;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Attributes::set (StringID id, const Object& object, bool saveAsCopy, Archive* archive)
{
	Object* objToSave = nullptr;
	if(saveAsCopy)
	{
		objToSave = object.clone ();
		ASSERT (objToSave != nullptr)
	}

	if(!objToSave)
	{
		Attributes* attributes = NEW Attributes;
		bool result = object.save (Storage (*attributes, archive));
		ASSERT (result == true)
		if(!result)
		{
			attributes->release ();
			return false;
		}

		objToSave = attributes;
	}

	return set (id, objToSave, kOwns);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Attributes::get (Object& object, StringID id, Archive* archive) const
{
	Object* savedObj = getObject (id);
	if(!savedObj)
		return false;

	Attributes* attributes = ccl_cast<Attributes> (savedObj);
	if(attributes)
		return object.load (Storage (*attributes, archive));

	if(!savedObj->canCast (object.myClass ()))
		return false;

	Attributes a;
	bool result = savedObj->save (Storage (a));
	ASSERT (result == true)
	if(!result)
		return false;

	return object.load (Storage (a, archive));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Attributes::get (String& string, StringID id) const
{
	Variant value;
	if(getAttribute (value, id))
	{
		if(value.getType () == Variant::kObject)
		{
			Object* obj = unknown_cast<Object> (value);
			if(obj)
				return obj->toString (string, 0);
		}
		else if(value.getType () == Variant::kString)
		{
			string = value;
			return true;
		}
		else
		{
			value.toString (string);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Attributes::get (MutableCString& string, StringID id, TextEncoding encoding) const
{
	String temp;
	if(!get (temp, id))
		return false;

	string.empty ();
	string.append (temp, encoding);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Attributes::setAttribute (StringID id, VariantRef value, int flags)
{
	Attribute* a = lookup (id);
	if(!a)
	{
		a = NEW Attribute (id);
		list.add (a);
	}

	a->set (value, flags);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Attributes::appendAttribute (StringID id, VariantRef value, int flags)
{
	Attribute* a = NEW Attribute (id);
	list.add (a);

	a->set (value, flags);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Attributes::getAttribute (Variant& value, StringID id) const
{
	Attribute* a = lookup (id);
	if(a)
	{
		value = a->getValue ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* Attributes::getObject (StringID id, MetaClassRef typeId) const
{
	Object* obj = getObject (id);
	return obj && obj->canCast (typeId) ? obj : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Attributes::queueAttribute (StringID id, VariantRef value, int flags)
{
	AttributeQueue* list = ccl_cast<AttributeQueue> (getObject (id));
	if(!list)
	{
		list = NEW AttributeQueue;
		set (id, list, kOwns);
	}

	list->addValue (value, flags);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Attributes::unqueueAttribute (StringID id, Variant& value)
{
	AttributeQueue* list = ccl_cast<AttributeQueue> (getObject (id));
	if(list)
	{
		Attribute* a = list->unqueueNext ();
		if(a)
		{
			value = a->detachValue ();
			a->release ();
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* Attributes::unqueueObject (StringID id, MetaClassRef typeId)
{
	while(1)
	{
		Object* obj = unknown_cast<Object> (unqueueUnknown (id));
		if(!obj)
			return nullptr;

		if(obj->canCast (typeId))
			return obj;

		obj->release ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Attributes::queue (StringID id, const Container& container, int flags)
{
	AttributeQueue* list = ccl_cast<AttributeQueue> (getObject (id));
	if(!list)
	{
		list = NEW AttributeQueue;
		set (id, list, kOwns);
	}

	ForEach (container, Object, obj)
		list->addValue (static_cast<IObject*> (obj), flags);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Attributes::convertAndQueue (StringID id, const Container& container, Archive* archive)
{
	ObjectArray temp;
	temp.objectCleanup (true);
	ForEach (container, Object, obj)
		AutoPtr<Attributes> a = NEW Attributes;
		if(!obj->save (Storage (*a, archive)))
			return false;
		temp.add (a.detach ());
	EndFor

	return queue (id, temp, Attributes::kShare);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Container& Attributes::unqueue (Container& container, StringID id, MetaClassRef typeId)
{
	container.objectCleanup (true);
	Object* obj;
	while((obj = unqueueObject (id, typeId)) != nullptr)
		container.add (obj);
	return container;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* Attributes::newQueueIterator (StringID id, MetaClassRef typeId) const
{
	if(AttributeQueue* list = ccl_cast<AttributeQueue> (getObject (id)))
		if(Iterator* iter = list->newIterator ())
		{
			if(typeId == ccl_typeid<Attribute> ()) // allow direct access
				return iter;
			else
				return NEW AttributeQueue::ContentIterator (iter, typeId);
		}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Attributes::unqueueAndCreate (Container& container, StringID id, MetaClassRef typeId, Archive* archive)
{
	ObjectArray temp;
	unqueue (temp, id, ccl_typeid<Attributes> ());

	container.objectCleanup (true);
	ForEach (temp, Attributes, a)
		AutoPtr<Object> obj = typeId.createObject ();
		if(!obj->load (Storage (*a, archive)))
			return false;
		container.add (obj.detach ());
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Attributes::isEmpty () const
{
	return list.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Attributes::countAttributes () const
{
	return list.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Attributes::getAttributeName (MutableCString& name, int index) const
{
	Attribute* a = (Attribute*)list.at (index);
	if(a)
	{
		name = a->getID ();
		return true;
	}
	name.empty ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Attributes::getAttributeValue (Variant& value, int index) const
{
	Attribute* a = (Attribute*)list.at (index);
	if(a)
	{
		value = a->getValue ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Attributes::contains (StringID id) const
{
	return lookup (id) != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Attributes::remove (StringID id)
{
	Attribute* a = lookup (id);
	if(a)
	{
		list.remove (a);
		a->release ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Attributes::removeAll ()
{
	list.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Attributes::getAttributeIndex (StringID id) const
{
	Attribute* a = lookup (id);
	return a ? list.index (a) : -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Attributes::setAttributeIndex (StringID id, int index)
{
	Attribute* a = lookup (id);
	if(a)
	{
		list.remove (a);
		if(!list.insertAt (index, a))
			list.add (a);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Attributes::createInstance (UIDRef cid, UIDRef iid, void** obj)
{
	if(cid == ClassID::Attributes)
	{
		AutoPtr<Attributes> attr = NEW Attributes;
		return attr->queryInterface (iid, obj);
	}
	if(cid == ClassID::AttributeQueue)
	{
		AutoPtr<AttributeQueue> queue = NEW AttributeQueue;
		return queue->queryInterface (iid, obj);
	}
	if(cid == ClassID::PersistentAttributes)
	{
		AutoPtr<PersistentAttributes> attr = NEW PersistentAttributes;
		return attr->queryInterface (iid, obj);
	}

	CCL_DEBUGGER ("Unknown class!\n")
	*obj = nullptr;
	return kResultNoInterface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attribute* Attributes::lookup (StringID id) const
{
	AttrContainerForEach (list, Attribute, a)
		if(a->getID () == id)
			return a;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Attributes)
	DEFINE_METHOD_ARGR ("countAttributes", "", "int")
	DEFINE_METHOD_ARGR ("getAttributeName", "index: int", "string")
	DEFINE_METHOD_ARGR ("getAttributeValue", "index: int", "variant")
	DEFINE_METHOD_ARGR ("getAttribute", "name: string", "variant")
	DEFINE_METHOD_ARGR ("setAttribute", "name: string, value: variant", "bool")
	DEFINE_METHOD_ARGR ("queueAttribute", "id: string, value: variant", "bool")
	DEFINE_METHOD_ARGR ("newQueueIterator", "id: string", "Iterator")
	DEFINE_METHOD_ARGR ("contains", "id: string", "bool")
END_METHOD_NAMES (Attributes)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Attributes::getProperty (Variant& var, MemberID propertyId) const
{
	return getAttribute (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Attributes::setProperty (MemberID propertyId, VariantRef var)
{
	// implicit sharing for objects
	int flags = 0;
	if(var.getType () == Variant::kObject)
		flags = kShare;

	return setAttribute (propertyId, var, flags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Attributes::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "countAttributes")
	{
		returnValue = countAttributes ();
		return true;
	}
	else if(msg == "getAttributeName")
	{
		MutableCString name;
		getAttributeName (name, msg[0].asInt ());
		returnValue = Variant (String (name), true);
		return true;
	}
	else if(msg == "getAttributeValue")
	{
		getAttributeValue (returnValue, msg[0].asInt ());
		returnValue.share ();
		return true;
	}
	else if(msg == "getAttribute")
	{
		MutableCString id (msg[0].asString ());
		getAttribute (returnValue, id);
		return true;
	}
	else if(msg == "setAttribute")
	{
		MutableCString id (msg[0].asString ());
		returnValue = (setAttribute (id, msg[1], kShare) != 0);
		return true;
	}
	else if(msg == "queueAttribute")
	{
		MutableCString id (msg[0].asString ());
		Variant value = msg[1];
		returnValue = queueAttribute (id, value, kShare);
		return true;
	}
	else if(msg == "newQueueIterator")
	{
		MutableCString id (msg[0].asString ());
		AutoPtr<Iterator> iter;
		if(Iterator* it = newQueueIterator (id))
			iter = NEW HoldingIterator (this, it);
		returnValue.takeShared (ccl_as_unknown (iter));
		return true;
	}
	else if(msg == "contains")
	{
		MutableCString id (msg[0].asString ());
		returnValue = contains (id);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Attributes::dump (int inset) const
{
	dump (list, inset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Attributes::dump (const Container& list, int inset) const
{
	MutableCString indent;
	for(int i = 0; i < inset; i++)
		indent.append (" ");

	ForEach (list, Attribute, a)
		if(a->getValue ().getType () == Variant::kObject)
		{
			Attributes* attribs = unknown_cast<Attributes> (a->getValue ());
			if(attribs)
			{
				Debugger::print (indent);
				Debugger::print ("Attributes '");
				Debugger::print (a->getID ().isEmpty () ? "unnamed" : a->getID ().str ());
				Debugger::print ("':\n");

				attribs->dump (inset + 1);
				continue;
			}

			AttributeQueue* queue = unknown_cast<AttributeQueue> (a->getValue ());
			if(queue)
			{
				Debugger::print (indent);
				Debugger::print ("Queue '");
				Debugger::print (a->getID ().isEmpty () ? "unnamed" : a->getID ().str ());
				Debugger::print ("':");

				dump (*queue, inset + 1);
				continue;
			}

			Object* obj = unknown_cast<Object> (a->getValue ());
			if(obj)
			{
				Debugger::print (indent);
				Debugger::print ("Object '");
				Debugger::print (a->getID ());
				Debugger::print ("' = ");
				Debugger::println (obj->myClass ().getPersistentName ());
			}
		}
		else
		{
			String s;
			a->getValue ().toString (s);

			Debugger::print (indent);
			Debugger::print (a->getID ());
			Debugger::print (" = ");
			Debugger::println (s);
		}
	EndFor
}

//************************************************************************************************
// PersistentAttributes
//************************************************************************************************

DEFINE_CLASS (PersistentAttributes, Attributes)
DEFINE_CLASS_UID (PersistentAttributes, 0x9b8aa2e3, 0x36fd, 0x4fcb, 0xb1, 0xdf, 0xde, 0x71, 0xe1, 0xa5, 0x46, 0xb3)
DEFINE_CLASS_NAMESPACE (PersistentAttributes, NAMESPACE_CCL)
DEFINE_CLASS_CATEGORY (PersistentAttributes, "System")

//////////////////////////////////////////////////////////////////////////////////////////////////

void PersistentAttributes::makeValidKey (MutableCString& keyString) const
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PersistentAttributes::load (const Storage& storage)
{
	Attributes& storageAttributes = storage.getAttributes ();

	removeAll ();

	AutoPtr<Attribute> attr;
	while((attr = storageAttributes.unqueueObject<Attribute> (nullptr)) != nullptr)
		setAttribute (attr->getID (), attr->getValue (), kShare);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PersistentAttributes::save (const Storage& storage) const
{
	Attributes& storageAttributes = storage.getAttributes ();
	AttrContainerForEach (list, Attribute, attr)
		storageAttributes.queue (nullptr, attr, kShare);
	EndFor
	return true;
}
