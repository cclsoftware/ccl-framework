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
// Filename    : core/portable/coretypeinfo.h
// Description : Type Information
//
//************************************************************************************************

#ifndef _coretypeinfo_h
#define _coretypeinfo_h

#include "core/public/coretypes.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// Type Information Macros
//************************************************************************************************

#define DECLARE_CORE_CLASS(id, Class, Base) \
DECLARE_CORE_CLASS_ (id, Class) \
void* castTo (TypeID typeId) override { return typeId == kTypeID ? static_cast<Class*> (this) : Base::castTo (typeId); } \

#define BEGIN_CORE_CLASS_ \
void* castTo (TypeID typeId) override {

#define BEGIN_CORE_CLASS(id, Class) \
DECLARE_CORE_CLASS_ (id, Class) \
BEGIN_CORE_CLASS_ \
if(typeId == kTypeID) return static_cast<Class*> (this); \

#define ADD_CORE_CLASS(Class) \
if(void* obj = Class::castTo (typeId)) return obj; \

#define ADD_CORE_CLASS_(Class) \
if(typeId == Class::kTypeID) return static_cast<Class*> (this); \

#define END_CORE_CLASS(Base) \
return Base::castTo (typeId); }

#define DECLARE_CORE_CLASS_(id, Class) \
static const TypeID kTypeID = id; \
private: static TypeID __typeid () { return kTypeID; } \
friend TypeID Core::Portable::__TID<Class>::tid (); public: \

#define DELEGATE_CORE_CLASS(Base) \
void* castTo (TypeID typeId) { return Base::castTo (typeId); }

//************************************************************************************************
// ITypedObject
//************************************************************************************************

struct ITypedObject
{
	typedef int32 TypeID;
	virtual void* castTo (TypeID typeId) = 0;
};

//************************************************************************************************
// TypedObject
//************************************************************************************************

class TypedObject: public ITypedObject
{
public:
	virtual ~TypedObject() {}

	// ITypedObject
	void* castTo (TypeID typeId) override { return nullptr; }
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Type casting safeguard
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
struct __TID
{
	// A compiler error here means that you try to core_cast to a class with no DECLARE_CORE_CLASS macro
	static ITypedObject::TypeID tid () { return T::__typeid (); }
};

//************************************************************************************************
// Type casting
//************************************************************************************************

/** Get type identifier. */
template <class T>
inline TypedObject::TypeID core_typeid ()
{
	return __TID<T>::tid ();
}

/** Cast typed object. Usually faster than dynamic_cast, especially on embedded platforms. */
template <class T>
inline T* core_cast (TypedObject* c)
{
	return c ? static_cast<T*> (c->castTo (__TID<T>::tid ())) : nullptr;
}

template <class T>
inline const T* core_cast (const TypedObject* c)
{
	return c ? static_cast<T*> (const_cast<TypedObject*> (c)->castTo (__TID<T>::tid ())) : nullptr;
}

template <class T>
inline T* core_cast (ITypedObject* c)
{
	return c ? static_cast<T*> (c->castTo (__TID<T>::tid ())) : nullptr;
}

template <class T>
inline const T* core_cast (const ITypedObject* c)
{
	return c ? static_cast<T*> (const_cast<ITypedObject*> (c)->castTo (__TID<T>::tid ())) : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Portable
} // namespace Core

#endif // _coretypeinfo_h
