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
// Filename    : ccl/base/storage/attributes.h
// Description : Attribute List
//
//************************************************************************************************

#ifndef _ccl_attributes_h
#define _ccl_attributes_h

#include "ccl/public/storage/iattributelist.h"

#include "ccl/base/collections/objectarray.h"

namespace CCL {

class Archive;
class Attribute;

//************************************************************************************************
// AttributeContainer
//************************************************************************************************

typedef ObjectArray AttributeContainer;
#define AttrContainerForEach ArrayForEach

//************************************************************************************************
// Attributes
//************************************************************************************************

class Attributes: public Object,
				  public IAttributeList,
				  public AttributeReader,
				  public AttributeWriter
{
public:
	DECLARE_CLASS (Attributes, Object)
	DECLARE_METHOD_NAMES (Attributes)

	Attributes ();
	Attributes (const Attributes&);

	void copyFrom (const Attributes& attributes);

	virtual void makeValidKey (MutableCString& keyString) const;

	// IAttributeList
	tbool CCL_API isEmpty () const override;
	int CCL_API countAttributes () const override;
	tbool CCL_API getAttributeName (MutableCString& name, int index) const override;
	tbool CCL_API getAttributeValue (Variant& value, int index) const override;
	tbool CCL_API setAttribute (StringID id, VariantRef value, int flags = 0) override;
	tbool CCL_API appendAttribute (StringID id, VariantRef value, int flags = 0) override;
	tbool CCL_API getAttribute (Variant& value, StringID id) const override;
	tbool CCL_API queueAttribute (StringID id, VariantRef value, int flags = 0) override;
	tbool CCL_API unqueueAttribute (StringID id, Variant& value) override;
	tbool CCL_API contains (StringID id) const override;
	tbool CCL_API remove (StringID id) override;
	void CCL_API removeAll () override;
	void CCL_API copyFrom (const IAttributeList& list) override;
	void CCL_API addFrom (const IAttributeList& list, const IAttributeFilter* filter = nullptr) override;

	// IClassAllocator
	tresult CCL_API createInstance (UIDRef cid, UIDRef iid, void** obj) override;
	
	// Internal methods
	int getAttributeIndex (StringID id) const;
	bool setAttributeIndex (StringID id, int index); ///< move attribute to given index in list

	using AttributeWriter::set;
	bool setHexValue (StringID id, uint32 value); ///< set as ASCII-encoded hexadecimal string
	uint32 getHexValue (StringID id) const;

	bool set (StringID id, Object* o, int flags = 0);
	bool set (StringID id, const Object& object, bool saveAsCopy = false, Archive* archive = nullptr);

	bool get (String& string, StringID id) const; ///< always converted to string!
	bool get (MutableCString& string, StringID id, TextEncoding encoding = Text::kASCII) const; ///< always converted to string!
	bool get (Object& object, StringID id, Archive* archive = nullptr) const;

	Object* getObject (StringID id) const;
	template <class T> T* getObject (StringID id) const;
	Object* getObject (StringID id, MetaClassRef typeId) const;
	Attributes* getAttributes (StringID id) const;
	
	bool queue (StringID id, Object* o, int flags = 0);
	bool queue (StringID id, const Container& container, int flags = 0);
	bool convertAndQueue (StringID id, const Container& container, Archive* archive = nullptr); ///< converts objects to simple attributes

	Object* unqueueObject (StringID id);
	Object* unqueueObject (StringID id, MetaClassRef typeId);
	template <class T> T* unqueueObject (StringID id);
	Attributes* unqueueAttributes (StringID id = nullptr);

	Container& unqueue (Container& container, StringID id, MetaClassRef typeId = ccl_typeid<Object> ());
	Iterator* newQueueIterator (StringID id, MetaClassRef typeId = ccl_typeid<Object> ()) const;
	bool unqueueAndCreate (Container& container, StringID id, MetaClassRef typeId, Archive* archive = nullptr); ///< creates objects from simple attributes

	void dump (int inset = 0) const;
	void dump (const Container& list, int inset) const;

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
	bool equals (const Object& obj) const override;

	CLASS_INTERFACE2 (IAttributeList, IClassAllocator, Object)

protected:
	AttributeContainer list;

	Attribute* lookup (StringID id) const;

	IAttributeList& getList () override { return *this; }
	const IAttributeList& getList () const override { return *this; }

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, VariantRef var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// PersistentAttributes
/** Save attributes as objects instead of inline. */
//************************************************************************************************

class PersistentAttributes: public Attributes
{
public:
	DECLARE_CLASS (PersistentAttributes, Attributes)

	// Attributes
	void makeValidKey (MutableCString& keyString) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// AttributeFilter
//************************************************************************************************

class AttributeFilter: public Object,
					   public IAttributeFilter
{
public:
	DECLARE_CLASS (AttributeFilter, Object)

	enum FilterType { kInclude, kExclude };

	AttributeFilter (const CString idList[] = nullptr, int idCount = 0, FilterType type = kInclude);
	
	int getCount () const;
	StringID getId (int index) const;

	// IAttributeFilter
	tbool CCL_API matches (StringID id) const override;

	CLASS_INTERFACE (IAttributeFilter, Object)

protected:
	const CString* idList;
	int idCount;
	FilterType type;
};

//************************************************************************************************
// Attribute
//************************************************************************************************

class Attribute: public Object,
				 public IAttribute
{
public:
	DECLARE_CLASS (Attribute, Object)

	Attribute (StringID id = nullptr);
	Attribute (const Attribute&);

	bool isOwner () const { return value.isShared (); }
	void set (VariantRef value, int flags = 0);
	VariantRef detachValue ();

	// IAttribute
	StringID CCL_API getID () const override { return id; }
	VariantRef CCL_API getValue () const override { return value; }

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (IAttribute, Object)

protected:
	MutableCString id;
	Variant value;
};

//************************************************************************************************
// AttributeQueue
//************************************************************************************************

class AttributeQueue: public AttributeContainer,
					  public IAttributeQueue
{
public:
	DECLARE_CLASS (AttributeQueue, AttributeContainer)

	AttributeQueue ();
	~AttributeQueue ();

	Attribute* unqueueNext ();

	void addAttributes (Attributes* attr, int flags = 0);

	// IAttributeQueue
	void CCL_API addValue (VariantRef value, int flags = 0) override;

	CLASS_INTERFACE (IAttributeQueue, AttributeContainer)

	class ContentIterator;

protected:
	Iterator* iter;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Attributes inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool Attributes::set (StringID id, Object* o, int flags)
{ return setAttribute (id, Variant (static_cast<IObject*> (o)), flags) != 0; }

inline Object* Attributes::getObject (StringID id) const
{ return unknown_cast<Object> (getUnknown (id)); }

template <class T> T* Attributes::getObject (StringID id) const
{ return ccl_cast<T> (getObject (id)); }

inline Attributes* Attributes::getAttributes (StringID id) const
{ return getObject<Attributes> (id); }

inline bool Attributes::queue (StringID id, Object* o, int flags)
{ return queueAttribute (id, Variant (static_cast<IObject*> (o)), flags) != 0; }

inline Object* Attributes::unqueueObject (StringID id)
{ return unknown_cast<Object> (unqueueUnknown (id)); }

template <class T> T* Attributes::unqueueObject (StringID id)
{ return (T*)unqueueObject (id, ccl_typeid<T> ()); }

inline Attributes* Attributes::unqueueAttributes (StringID id)
{ return (Attributes*)unqueueObject (id, ccl_typeid<Attributes> ()); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// AttributeQueue inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void AttributeQueue::addAttributes (Attributes* attr, int flags) 
{ addValue (static_cast<IAttributeList*> (attr), flags); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_attributes_h
