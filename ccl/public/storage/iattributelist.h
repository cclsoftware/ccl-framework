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
// Filename    : ccl/public/storage/iattributelist.h
// Description : Attribute List Interface
//
//************************************************************************************************

#ifndef _ccl_iattributelist_h
#define _ccl_iattributelist_h

#include "ccl/public/base/variant.h"
#include "ccl/public/text/cstring.h"

#include "ccl/public/collections/iunknownlist.h"

namespace CCL {

interface IAttributeFilter;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Attribute list classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (Attributes, 0x138ed4e7, 0x1786, 0x4a9d, 0xa0, 0xef, 0xd7, 0x6f, 0xba, 0x49, 0xb4, 0xda)
	DEFINE_CID (AttributeQueue, 0xa0303add, 0x0e4f, 0x4557, 0x8c, 0xa4, 0xae, 0x3e, 0xcc, 0x42, 0xc4, 0x93)
	DEFINE_CID (PersistentAttributes, 0x9b8aa2e3, 0x36fd, 0x4fcb, 0xb1, 0xdf, 0xde, 0x71, 0xe1, 0xa5, 0x46, 0xb3)
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Attribute list macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define ForEachAttribute(attribs, name, value) \
{ int __count = (attribs).countAttributes (); \
  if(__count > 0) for(int __idx = 0; __idx < __count;  __idx++) \
   { CCL::MutableCString name; (attribs).getAttributeName (name, __idx); \
     CCL::Variant value; (attribs).getAttributeValue (value, __idx);

//************************************************************************************************
// IAttributeList
/** An attribute list holds values of arbitrary type identified by a key string.
	\ingroup base_io */
//************************************************************************************************

interface IAttributeList: IClassAllocator
{
	/** Ownership flags. */
	enum Flags
	{
		kOwns = 1<<0,	///< list takes ownership of object
		kTemp = 1<<1,	///< list copies object
		kShare = 1<<2	///< list shares object
	};

	/** Check if attribute list is empty. */
	virtual tbool CCL_API isEmpty () const = 0;

	/** Get number of attributes in list. */
	virtual int CCL_API countAttributes () const = 0;

	/** Get attribute name at given index as C-String. */
	virtual tbool CCL_API getAttributeName (MutableCString& name, int index) const = 0;

	/** Get attribute value at given index. */
	virtual tbool CCL_API getAttributeValue (Variant& value, int index) const = 0;

	/** Set attribute value with given key and ownership. */
	virtual tbool CCL_API setAttribute (StringID id, VariantRef value, int flags = 0) = 0;

	/** Append attribute value with given key and ownership. This method does not check for duplicate keys. */
	virtual tbool CCL_API appendAttribute (StringID id, VariantRef value, int flags = 0) = 0;

	/** Get attribute value by key. */
	virtual tbool CCL_API getAttribute (Variant& value, StringID id) const = 0;

	/** Queue value to value list with given key. */
	virtual tbool CCL_API queueAttribute (StringID id, VariantRef value, int flags = 0) = 0;

	/** Unqueue value from value list by key. */
	virtual tbool CCL_API unqueueAttribute (StringID id, Variant& value) = 0;

	/** Check if list contains attribute with given key. */
	virtual tbool CCL_API contains (StringID id) const = 0;

	/** Remove attribute. */
	virtual tbool CCL_API remove (StringID id) = 0;

	/** Remove all attributes. */
	virtual void CCL_API removeAll () = 0;

	/** Copy all attributes from given list. */
	virtual void CCL_API copyFrom (const IAttributeList& list) = 0;

	/** Add attributes from list with optional filter. */
	virtual void CCL_API addFrom (const IAttributeList& list, const IAttributeFilter* filter = nullptr) = 0;

	DECLARE_IID (IAttributeList)
};

DEFINE_IID (IAttributeList, 0x6bb2c332, 0x137, 0x4820, 0xb1, 0x37, 0x47, 0x1, 0xb3, 0x90, 0xe5, 0x12)

//************************************************************************************************
// IAttributeQueue
/** Attribute queue interface, use IContainer to access contained IAttribute objects.
	\ingroup base_io */
//************************************************************************************************

interface IAttributeQueue: IUnknown
{
	/** Add a value to the queue. */
	virtual void CCL_API addValue (VariantRef value, int flags = 0) = 0;

	DECLARE_IID (IAttributeQueue)
};

DEFINE_IID (IAttributeQueue, 0x567820B2, 0xB8BC, 0x4A0C, 0xB5, 0x5D, 0x44, 0xE4, 0xC5, 0xBE, 0x50, 0x34)

//************************************************************************************************
// IAttribute
/** Attribute interface.
	\ingroup base_io */
//************************************************************************************************

interface IAttribute: IUnknown
{
	/** Get attribute identifier. */
	virtual StringID CCL_API getID () const = 0;

	/** Get attribute value. */
	virtual VariantRef CCL_API getValue () const = 0;

	DECLARE_IID (IAttribute)
};

DEFINE_IID (IAttribute, 0xe525580f, 0x477c, 0x44a6, 0x96, 0x53, 0x30, 0x37, 0xec, 0xda, 0x3b, 0x70)

//************************************************************************************************
// IAttributeFilter
/** Attribute filter interface.
	\ingroup base_io */
//************************************************************************************************

interface IAttributeFilter: IUnknown
{
	/** Tell if the attribute identifier matches a filter condition. */
	virtual tbool CCL_API matches (StringID id) const = 0;

	DECLARE_IID (IAttributeFilter)
};

DEFINE_IID (IAttributeFilter, 0xf8080968, 0x7c0b, 0x4761, 0xb2, 0xea, 0x69, 0xea, 0xe1, 0xe1, 0xd1, 0x47)

//************************************************************************************************
// AttributeReader
/** Helper class to read attributes by type.
	\ingroup base_io */
//************************************************************************************************

class AttributeReader
{
public:
	/** Get associated attribute list. */
	virtual const IAttributeList& getList () const = 0;

	/** Get variant value. */
	Variant getVariant (StringID id) const;

	/** Get integer value (64 bit). */
	int64 getInt64 (StringID id) const;

	/** Get integer value (32 bit). */
	int getInt (StringID id) const;

	/** Get boolean value. */
	bool getBool (StringID id) const;

	/** Get floating-point value (double precision). */
	double getFloat (StringID id) const;

	/** Get string value (possibly converted). */
	String getString (StringID id) const;

	/** Get C-string value (possibly converted). */
	MutableCString getCString (StringID id, TextEncoding encoding = Text::kASCII) const;

	/** Get object value. The attribute list maintains ownership. */
	IUnknown* getUnknown (StringID id) const;

	/** Create iterator for attribute queue, objects are of type IAttribute. */
	IUnknownIterator* newUnknownIterator (StringID id) const;

	/** Get tresult value. */
	tresult getResult (StringID id) const;

	/** Get integer value (64 bit). Returns true, if value has been set. */
	bool getInt64 (int64& value, StringID id) const;

	/** Get integer value (32 bit). Returns true, if value has been set. */
	bool getInt (int& value, StringID id) const;

	/** Get boolean value. Returns true, if value has been set. */
	bool getBool (bool& value, StringID id) const;

	/** Get floating-point value (double precision). Returns true, if value has been set. */
	bool getFloat (double& value, StringID id) const;

	/** Get floating-point value (single precision). Returns true, if value has been set. */
	bool getFloat (float& value, StringID id) const;

	/** Get string value (empty if type isn't string). Returns true, if value has been set. */
	bool getString (String& value, StringID id) const;

	/** Get C-string value (empty if type isn't string). Returns true, if value has been set. */
	bool getCString (MutableCString& value, StringID id, TextEncoding encoding = Text::kASCII) const;

	/** Get object value. The attribute list maintains ownership. Returns true, if value has been set. */
	bool getUnknown (IUnknown*& value, StringID id) const;
};

//************************************************************************************************
// AttributeClassFactory
/** Helper to allocate attribute classes.
	\ingroup base_io */
//************************************************************************************************

struct AttributeClassFactory
{
	IClassAllocator& allocator;

	AttributeClassFactory (IClassAllocator& allocator);

	IAttributeList* newAttributes ();
	IAttributeQueue* newAttributeQueue ();
	IAttributeList* newPersistentAttributes ();
};

//************************************************************************************************
// AttributeWriter
/** Helper class to write attributes by type.
	\ingroup base_io */
//************************************************************************************************

class AttributeWriter
{
public:
	/** Get associated attribute list. */
	virtual IAttributeList& getList () = 0;

	/** Create a new empty attribute list instance. */
	IAttributeList* newAttributes ();

	/** Create a new empty attribute queue instance. */
	IAttributeQueue* newAttributeQueue ();

	/** Create a new empty attribute list instance (alternative storage format). */
	IAttributeList* newPersistentAttributes ();

	/** Set integer value (64 bit). */
	bool set (StringID id, int64 v);

	/** Set integer value (32 bit). */
	bool set (StringID id, int v);

	/** Set tresult value */
	bool set (StringID id, tresult v);

	/** Set boolean value. */
	bool set (StringID id, bool b, int flags = 0);

	/** Set floating-point value (double precision). */
	bool set (StringID id, double v);

	/** Set string value. */
	bool set (StringID id, StringRef s);

	/** Set C-string value. */
	bool set (StringID id, CStringRef s, TextEncoding encoding = Text::kASCII);

	/** Set C-string value (by pointer). */
	bool set (StringID id, CStringPtr s, TextEncoding encoding = Text::kASCII);

	/** Set object with ownership flags. */
	bool set (StringID id, IUnknown* u, int flags = 0);

	/** Queue object value under given key. */
	bool queue (StringID id, IUnknown* u, int flags = 0);

	/** Unqueue (and remove) object. Caller takes over ownership of object. */
	IUnknown* unqueueUnknown (StringID id);
};

//************************************************************************************************
// AttributeReadAccessor
/**	\ingroup base_io */
//************************************************************************************************

class AttributeReadAccessor: public AttributeReader
{
public:
	AttributeReadAccessor (const IAttributeList& attributes)
	: attributes (attributes)
	{}

	// AttributeReader
	const IAttributeList& getList () const override { return attributes; }

protected:
	const IAttributeList& attributes;
};

//************************************************************************************************
// AttributeAccessor
/**	\ingroup base_io */
//************************************************************************************************

class AttributeAccessor: public AttributeReader,
						 public AttributeWriter
{
public:
	AttributeAccessor (IAttributeList& attributes)
	: attributes (attributes)
	{}

	// AttributeReader + AttributeWriter
	IAttributeList& getList () override { return attributes; }
	const IAttributeList& getList () const override { return attributes; }

protected:
	IAttributeList& attributes;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// AttributeClassFactory inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline AttributeClassFactory::AttributeClassFactory (IClassAllocator& allocator)
: allocator (allocator)
{}

inline IAttributeList* AttributeClassFactory::newAttributes ()
{
	IAttributeList* list = nullptr;
	allocator.createInstance (ClassID::Attributes, ccl_iid<IAttributeList> (), (void**)&list);
	return list;
}

inline IAttributeQueue* AttributeClassFactory::newAttributeQueue ()
{
	IAttributeQueue* queue = nullptr;
	allocator.createInstance (ClassID::AttributeQueue, ccl_iid<IAttributeQueue> (), (void**)&queue);
	return queue;
}

inline IAttributeList* AttributeClassFactory::newPersistentAttributes ()
{
	IAttributeList* list = nullptr;
	allocator.createInstance (ClassID::PersistentAttributes, ccl_iid<IAttributeList> (), (void**)&list);
	return list;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// AttributeWriter inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline IAttributeList* AttributeWriter::newAttributes ()
{ return AttributeClassFactory (getList ()).newAttributes (); }

inline IAttributeQueue* AttributeWriter::newAttributeQueue ()
{ return AttributeClassFactory (getList ()).newAttributeQueue (); }

inline IAttributeList* AttributeWriter::newPersistentAttributes ()
{ return AttributeClassFactory (getList ()).newPersistentAttributes (); }

inline bool AttributeWriter::set (StringID id, int64 v)
{ return getList ().setAttribute (id, Variant (v), 0) != 0; }

inline bool AttributeWriter::set (StringID id, int v)
{ return set (id, static_cast<int64> (v)); }

inline bool AttributeWriter::set (StringID id, tresult v)
{ return set (id,  static_cast<int64> (v)); }

inline bool AttributeWriter::set (StringID id, bool b, int flags)
{ return getList ().setAttribute (id, Variant (b, flags)); }

inline bool AttributeWriter::set (StringID id, double v)
{ return getList ().setAttribute (id, Variant (v), 0) != 0; }

inline bool AttributeWriter::set (StringID id, StringRef s)
{ return getList ().setAttribute (id, Variant (s), 0) != 0; }

inline bool AttributeWriter::set (StringID id, CStringRef s, TextEncoding encoding)
{ String _s; _s.appendCString (encoding, s); return getList ().setAttribute (id, Variant (_s), 0) != 0; }

inline bool AttributeWriter::set (StringID id, CStringPtr s, TextEncoding encoding)
{ return set (id, CString (s), encoding); }

inline bool AttributeWriter::set (StringID id, IUnknown* u, int flags)
{ return getList ().setAttribute (id, Variant (u), flags) != 0; }

inline bool AttributeWriter::queue (StringID id, IUnknown* u, int flags)
{ return getList ().queueAttribute (id, Variant (u), flags) != 0; }

inline IUnknown* AttributeWriter::unqueueUnknown (StringID id)
{ Variant v; getList ().unqueueAttribute (id, v); return v; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// AttributeReader inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Variant AttributeReader::getVariant (StringID id) const
{ Variant v; getList ().getAttribute (v, id); return v; }

inline int64 AttributeReader::getInt64 (StringID id) const
{ Variant v; getList ().getAttribute (v, id); return v; }

inline int AttributeReader::getInt (StringID id) const
{ return static_cast<int> (getInt64 (id)); }

inline bool AttributeReader::getBool (StringID id) const
{ return getInt (id) != 0; }

inline double AttributeReader::getFloat (StringID id) const
{ Variant v; getList ().getAttribute (v, id); return v; }

inline String AttributeReader::getString (StringID id) const
{ Variant v; getList ().getAttribute (v, id); return v.toString (); }

inline MutableCString AttributeReader::getCString (StringID id, TextEncoding encoding) const
{ return MutableCString (getString (id), encoding); }

inline IUnknown* AttributeReader::getUnknown (StringID id) const
{ Variant v; getList ().getAttribute (v, id); return v; }

inline IUnknownIterator* AttributeReader::newUnknownIterator (StringID id) const
{ UnknownPtr<IContainer> c = getUnknown (id); return c ? c->createIterator () :  nullptr; }

inline tresult AttributeReader::getResult (StringID id) const
{ return static_cast<tresult> (getInt64 (id)); }

inline bool AttributeReader::getInt64 (int64& _v, StringID id) const
{ Variant v; if(getList ().getAttribute (v, id)) { _v = v; return true; } return false; }

inline bool AttributeReader::getInt (int& _v, StringID id) const
{ Variant v; if(getList ().getAttribute (v, id)) { _v = v; return true; } return false; }

inline bool AttributeReader::getBool (bool& _v, StringID id) const
{ Variant v; if(getList ().getAttribute (v, id)) { _v = v; return true; } return false; }

inline bool AttributeReader::getFloat ( double& _v, StringID id) const
{ Variant v; if(getList ().getAttribute (v, id)) { _v = v; return true; } return false; }

inline bool AttributeReader::getFloat (float& _v, StringID id) const
{ Variant v; if(getList ().getAttribute (v, id)) { _v = (float)v; return true; } return false; }

inline bool AttributeReader::getString (String& _v, StringID id) const
{ Variant v; if(getList ().getAttribute (v, id)) { _v = v.toString (); return true; } return false; }

inline bool AttributeReader::getCString (MutableCString& _v, StringID id, TextEncoding encoding) const
{ Variant v; if(getList ().getAttribute (v, id)) { _v = MutableCString (v.toString (), encoding); return true; } return false; }

inline bool AttributeReader::getUnknown (IUnknown*& _v, StringID id) const
{ Variant v; if(getList ().getAttribute (v, id)) { _v = v; return true; } return false; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_iattributelist_h
