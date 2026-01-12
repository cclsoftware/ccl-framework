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
// Filename    : ccl/base/object.h
// Description : Object class
//
//************************************************************************************************

#ifndef _ccl_object_h
#define _ccl_object_h

#include "ccl/base/initterm.h"
#include "ccl/base/metaclass.h"
#include "ccl/base/objectmacros.h"

#include "ccl/public/base/unknown.h"
#include "ccl/public/base/iobserver.h"

namespace CCL {

class Object;
class Storage;
class OutputStorage;

//************************************************************************************************
// Object type identification
//************************************************************************************************

/** Returns meta class of specified class. */
template <class T>
MetaClassRef ccl_typeid () 
{
	// A compiler error here means that you try to ccl_cast to a class with no DECLARE_CLASS macro
	// This can only be called if T has a DECLARE_CLASS macro (that grants friendship to this function)
	return T::__typeid ();
}

//************************************************************************************************
// IObjectCast
/** Get Object address from IUnknown. */
//************************************************************************************************

interface CCL_NOVTABLE IObjectCast: IUnknown
{
	/** Get address of Object, returns null if requested by foreign module. */
	virtual Object* CCL_API revealObject (const void* moduleAddress) = 0;

	DECLARE_IID (IObjectCast)
};

//************************************************************************************************
// Object
/** Object base class with RTTI and reference counting.  \ingroup ccl_base */
//************************************************************************************************

class Object: public Unknown,
			  public IObjectCast,
			  public ISubject,
			  public IObserver,
			  public IObject
{
public:
	// Type Information
	DECLARE_BASE_CLASS (Object)

	// IUnknown
	DECLARE_UNKNOWN

	// ISubject
	void CCL_API addObserver (IObserver* observer) override;
	void CCL_API removeObserver (IObserver* observer) override;
	void CCL_API signal (MessageRef msg) override;
	void CCL_API deferSignal (IMessage* msg) override;

	/** Defer kChanged message for this object. */
	virtual void deferChanged ();

	// IObserver
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// Comparison
	virtual bool equals (const Object& obj) const;
	virtual int compare (const Object& obj) const;

	// Storage
	virtual bool load (const Storage& storage);
	virtual bool save (const Storage& storage) const;
	virtual bool save (const OutputStorage& storage) const;

	// String Conversion
	virtual bool toString (String& string, int flags = 0) const;

	// Hashing
	virtual int getHashCode (int size) const;

	// IUnknown cast
	IUnknown* asUnknown () { return static_cast<IObject*> (this); }
	
	// Garbage Collection
	static void addGarbageCollected (Object* obj, bool globalScope = true);
	static void deferDestruction (Object* obj);

	/** Get module identification address. */
	static const void* getModuleAddress ();

protected:
	#if CCL_DEBUG_INTERNAL
	static constexpr int kDebugFlagObserver = 1<<1;
	static constexpr int kDebugFlagHasObserver = 1<<2;	
	~Object ();
	#endif
	
	/** Cancel deferred signals and posted messages. */
	void cancelSignals ();

	// IObjectCast
	Object* CCL_API revealObject (const void* module) override;

	// IObject
	const ITypeInfo& CCL_API getTypeInfo () const override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getPropertyNames (IPropertyCollector& collector) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// Type casting
//************************************************************************************************

/** Cast object pointer to specified class. */
template <class T>
inline const T* ccl_cast (const Object* obj)
{
	return obj && obj->canCast (ccl_typeid<T> ()) ? static_cast<const T*> (obj) : nullptr;
}

template <class T>
inline T* ccl_cast (Object* obj)
{
	return obj && obj->canCast (ccl_typeid<T> ()) ? static_cast<T*> (obj) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Cast object pointer to specified class if object is exactly of that class (not derived from). */
template <class T>
inline const T* ccl_strict_cast (const Object* obj)
{
	return obj && obj->isClass (ccl_typeid<T> ()) ? static_cast<const T*> (obj) : nullptr;
}

template <class T>
inline T* ccl_strict_cast (Object* obj)
{
	return obj && obj->isClass (ccl_typeid<T> ()) ? static_cast<T*> (obj) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Cast interface to object pointer. */
template <class Class>
inline Class* unknown_cast (const IUnknown* unk)
{
	IObjectCast* objCast = nullptr;
	if(unk) const_cast<IUnknown*> (unk)->queryInterface (ccl_iid<IObjectCast> (), (void**)&objCast);
	Object* obj = objCast ? objCast->revealObject (Object::getModuleAddress ()) : nullptr;
	return ccl_cast<Class> (obj);
}

/** Cast interface to object pointer. Specialization for CCL::Object (for performance) */
template <>
inline Object* unknown_cast<Object> (const IUnknown* unk)
{
	IObjectCast* objCast = nullptr;
	if(unk) const_cast<IUnknown*> (unk)->queryInterface (ccl_iid<IObjectCast> (), (void**)&objCast);
	return objCast ? objCast->revealObject (Object::getModuleAddress ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Cast object to IUnknown pointer. */
inline IUnknown* ccl_as_unknown (Object* object)
{ return object ? static_cast<IObject*> (object) : nullptr; }

inline IUnknown* ccl_as_unknown (const Object* object)
{ return object ? static_cast<IObject*> (const_cast<Object*> (object)) : nullptr; }

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Cast object reference to IUnknown pointer. */
inline IUnknown* ccl_as_unknown (Object& object)
{ return object.asUnknown (); }

inline IUnknown* ccl_as_unknown (const Object& object)
{ return const_cast<Object&> (object).asUnknown (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Cast object to interface pointer. */
template <class IFace>
inline IFace* ccl_as_unknown (Object* object)
{ return UnknownPtr<IFace> (ccl_as_unknown (object)); }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_object_h
