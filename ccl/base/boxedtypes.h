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
// Filename    : ccl/base/boxedtypes.h
// Description : Basic "boxed" types
//
//************************************************************************************************

#ifndef _ccl_boxedtypes_h
#define _ccl_boxedtypes_h

#include "ccl/base/object.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/base/datetime.h"
#include "ccl/public/base/iformatter.h"

namespace CCL {

//************************************************************************************************
// IBoxedUID
//************************************************************************************************

interface IBoxedUID: IUnknown
{
	virtual void CCL_API assign (UIDRef uid) = 0;

	virtual void CCL_API copyTo (UIDBytes& uid) const = 0;

	DECLARE_IID (IBoxedUID)
};

namespace Boxed {

//************************************************************************************************
// Boxing Macros
//************************************************************************************************

#ifdef CCL_BOX_STATIC
	#define CCL_BOX(BoxedType, boxedName, plainName) \
	static BoxedType::Value boxedName##Value; \
	BoxedType* boxedName = boxedName##Value (plainName);
#else
	#define CCL_BOX(BoxedType, boxedName, plainName) \
	CCL::AutoPtr<BoxedType> boxedName = NEW BoxedType (plainName);
#endif

//************************************************************************************************
// Boxed::ValueHelper
//************************************************************************************************

template <class BoxedType, class PlainType>
class ValueHelper
{
public:
	ValueHelper ()
	: boxedValue (NEW BoxedType)
	{}

	~ValueHelper ()
	{
		boxedValue->release ();
	}

	ValueHelper& operator = (const PlainType& type)
	{
		*boxedValue = type;
		return *this;
	}

	ValueHelper& operator () (const PlainType& type)
	{
		*boxedValue = type;
		return *this;
	}

	operator BoxedType* ()    { return boxedValue; }
	BoxedType* operator -> () { return boxedValue; }

protected:
	BoxedType* boxedValue;
};

//************************************************************************************************
// Boxed::UID
//************************************************************************************************

class UID: public Object,
		   public CCL::UID,
		   public IBoxedUID

{
public:
	DECLARE_CLASS (UID, Object)
	DECLARE_METHOD_NAMES (UID)

	UID (CCL::UIDRef uid = kNullUID);

	static UIDBytes fromVariant (CCL::VariantRef var);

	typedef ValueHelper<Boxed::UID, CCL::UID> Value;

	UID& operator = (CCL::UIDRef uid);

	String asString () const;
	MutableCString asCString () const;

	// IBoxedUID
	void CCL_API assign (CCL::UIDRef uid) override;
	void CCL_API copyTo (UIDBytes& uid) const override;

	// Object
	int getHashCode (int size) const override;
	bool equals (const Object& obj) const override;
	bool load (const Storage&) override;
	bool save (const Storage&) const override;
	bool toString (CCL::String& string, int flags = 0) const override;

	CLASS_INTERFACE (IBoxedUID, Object)

protected:
	// IObject
	tbool CCL_API invokeMethod (CCL::Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// Boxed::Variant
//************************************************************************************************

class Variant: public Object,
			   public IVariant
{
public:
	DECLARE_CLASS (Variant, Object)

	Variant (CCL::VariantRef v = 0);

	typedef ValueHelper<Boxed::Variant, CCL::Variant> Value;

	operator CCL::VariantRef () const;
	Variant& operator = (CCL::VariantRef v);

	CCL::VariantRef asVariant () const;

	// IVariant
	void CCL_API assign (CCL::VariantRef variant) override;
	void CCL_API copyTo (CCL::Variant& variant) const override;

	// Object
	// TODO: load/save!
	bool equals (const Object& obj) const override;
	int compare (const Object& obj) const override;
	bool toString (CCL::String& string, int flags = 0) const override;

	CLASS_INTERFACE (IVariant, Object)

protected:
	CCL::Variant v;
};

//************************************************************************************************
// Boxed::VariantWithName
//************************************************************************************************

class VariantWithName: public Boxed::Variant
{
public:
	VariantWithName (VariantRef value = 0, CCL::StringRef name = nullptr);

	// Object
	bool toString (CCL::String& string, int flags = 0) const override;

	PROPERTY_STRING (name, Name)
};

//************************************************************************************************
// Boxed::String
//************************************************************************************************

class String: public Object,
			  public CCL::String
{
public:
	DECLARE_CLASS (String, Object)

	String (CCL::StringRef string = nullptr);

	typedef ValueHelper<Boxed::String, CCL::String> Value;

	String& operator = (CCL::StringRef string);

	using CCL::String::compare;

	// Object
	bool equals (const Object& obj) const override;
	int compare (const Object& obj) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
	bool toString (CCL::String& string, int flags = 0) const override;
	int getHashCode (int size) const override;
};

//************************************************************************************************
// Boxed::DateTime
//************************************************************************************************

class DateTime: public Object,
				public IDateTime,
				public CCL::DateTime
{
public:
	DECLARE_CLASS (DateTime, Object)
	DECLARE_METHOD_NAMES (DateTime)

	DateTime (const CCL::DateTime& dt = CCL::DateTime ());

	typedef ValueHelper<Boxed::DateTime, CCL::DateTime> Value;

	// IDateTime
	void CCL_API assign (const CCL::DateTime& dateTime) override;
	void CCL_API copyTo (CCL::DateTime& dateTime) const override;

	// Object
	bool equals (const Object& obj) const override;
	int compare (const Object& obj) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (IDateTime, Object)

protected:
	// IObject
	tbool CCL_API getProperty (CCL::Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const CCL::Variant& var) override;
	tbool CCL_API invokeMethod (CCL::Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// Boxed::Formatter
//************************************************************************************************

class Formatter: public Object,
				 public IFormatter
{
public:
	DECLARE_CLASS (Formatter, Object)
	DECLARE_METHOD_NAMES (Formatter)

	Formatter (IFormatter* formatter = nullptr);

	// IFormatter
	int CCL_API getFlags () const override;
	tbool CCL_API printString (CCL::String& string, CCL::VariantRef value) const override;
	tbool CCL_API scanString (CCL::Variant& value, CCL::StringRef string) const override;
	CStringPtr CCL_API getFactoryName () const override;

	CLASS_INTERFACES (Object)

protected:
	SharedPtr<IFormatter> formatter;

	// IObject
	tbool CCL_API invokeMethod (CCL::Variant& returnValue, MessageRef msg) override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Boxed::UID& Boxed::UID::operator = (CCL::UIDRef uid) { UIDBytes::assign (uid); return *this; }
inline CCL::VariantRef Boxed::Variant::asVariant () const { return v; }
inline Boxed::Variant::operator CCL::VariantRef () const { return v; }
inline Boxed::Variant& Boxed::Variant::operator = (CCL::VariantRef _v) { v = _v; return *this; }
inline Boxed::String& Boxed::String::operator = (CCL::StringRef string) { *((CCL::String*)this) = string; return *this; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Boxed
} // namespace CCL

#endif // _ccl_boxedtypes_h
