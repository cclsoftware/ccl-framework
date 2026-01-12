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
// Filename    : ccl/base/typelib.h
// Description : Type Library
//
//************************************************************************************************

#ifndef _ccl_typelib_h
#define _ccl_typelib_h

#include "ccl/base/object.h"

#include "ccl/public/base/itypelib.h"
#include "ccl/public/base/variant.h"

namespace CCL {

class Container;

//************************************************************************************************
// Model::AttributeDescription
//************************************************************************************************

namespace Model {

struct AttributeDescription
{
	MutableCString name;
	Variant value;

	AttributeDescription (StringID name = nullptr, VariantRef value = Variant ())
	: name (name),
	  value (value)
	{}
};

} // namespace Model

//************************************************************************************************
// TypeInfo
//************************************************************************************************

class TypeInfo: public Object,
				public AbstractTypeInfo
{
public:
	DECLARE_CLASS_ABSTRACT (TypeInfo, Object)

	TypeInfo (CStringPtr name, const TypeInfo* parentType);

	virtual bool getDetails (ITypeInfoDetails& details) const = 0;

	// ITypeInfo
	const ITypeInfo* CCL_API getParentType () const override;
	CStringPtr CCL_API getClassName () const override;
	CStringPtr CCL_API getClassNamespace () const override;

	CLASS_INTERFACE (ITypeInfo, Object)

protected:
	CStringPtr name;
	const TypeInfo* parentType;
};

//************************************************************************************************
// TypeInfoWithMembers
//************************************************************************************************

class TypeInfoWithMembers: public TypeInfo
{
public:
	DECLARE_CLASS_ABSTRACT (TypeInfoWithMembers, TypeInfo)

	TypeInfoWithMembers (CStringPtr name, const TypeInfo* parentType);

	struct MemberDescriptionModifier
	{
		MemberDescriptionModifier (TypeInfoWithMembers& This, Model::MemberDescription members[]);
	};

	struct AttributeModifier
	{
		AttributeModifier (TypeInfoWithMembers& This, Model::AttributeDescription attributes[]);
	};

	// TypeInfo
	bool getDetails (ITypeInfoDetails& details) const override;

protected:
	Model::MemberDescription* members;
	Model::AttributeDescription* attributes;
};

//************************************************************************************************
// EnumTypeInfo
//************************************************************************************************

class EnumTypeInfo: public Object,
					public IEnumTypeInfo
{
public:
	DECLARE_CLASS_ABSTRACT (EnumTypeInfo, Object)

	EnumTypeInfo (CStringPtr name, CStringPtr parentName = nullptr);

	// IEnumTypeInfo
	CStringPtr CCL_API getName () const override;
	CStringPtr CCL_API getParentName () const override;

	CLASS_INTERFACE (IEnumTypeInfo, Object)

protected:
	CStringPtr name;
	CString parentName;
};

//************************************************************************************************
// CStringEnumTypeInfo - define enum type with constant C-strings
//************************************************************************************************

class CStringEnumTypeInfo: public EnumTypeInfo
{
public:
	CStringEnumTypeInfo (CStringPtr name, CStringPtr enumerators[], int count);

	// EnumTypeInfo
	int CCL_API getEnumeratorCount () const override;
	tbool CCL_API getEnumerator (MutableCString& name, Variant& value, int index) const override;

protected:
	CStringPtr* enumerators;
	int count;
};

//************************************************************************************************
// MutableEnumTypeInfo - define dynamic enum type
//************************************************************************************************

class MutableEnumTypeInfo: public EnumTypeInfo
{
public:
	MutableEnumTypeInfo (CStringPtr name);
	~MutableEnumTypeInfo ();

	void addEnumerator (StringID name, VariantRef value);

	// EnumTypeInfo
	int CCL_API getEnumeratorCount () const override;
	tbool CCL_API getEnumerator (MutableCString& name, Variant& value, int index) const override;

protected:
	Container& enumerators;
};

//************************************************************************************************
// TEnumTypeInfo
//************************************************************************************************

template <class EnumDef>
class TEnumTypeInfo: public EnumTypeInfo
{
public:
	TEnumTypeInfo (CStringPtr name, const EnumDef enumerators[], int count)
	: EnumTypeInfo (name),
	  enumerators (enumerators),
	  count (count)
	{}

	// EnumTypeInfo
	int CCL_API getEnumeratorCount () const override { return count; }
	tbool CCL_API getEnumerator (MutableCString& name, Variant& value, int index) const override
	{
		ASSERT (index >= 0 && index < count)
		index = ccl_bound (index, 0, count-1);
		name = enumerators[index].getEnumName ();
		value = enumerators[index].getEnumValue ();
		return true;
	}

protected:
	const EnumDef* enumerators;
	int count;
};

//************************************************************************************************
// TypeLibrary
//************************************************************************************************

class TypeLibrary: public Object,
				   public ITypeLibrary
{
public:
	DECLARE_CLASS_ABSTRACT (TypeLibrary, Object)

	TypeLibrary (CStringPtr libraryName);
	~TypeLibrary ();

	void objectCleanup (bool state);

	bool addType (TypeInfo* typeInfo, bool checkExistence = false);
	bool addEnum (EnumTypeInfo* e, bool checkExistence = false);

	const TypeInfo* findType (StringID name, bool caseSensitive = true) const;
	const EnumTypeInfo* findEnum (StringID name, bool caseSensitive = true) const;

	// ITypeLibrary
	CStringPtr CCL_API getLibraryName () const override;
	IUnknownIterator* CCL_API newTypeIterator () const override;
	IUnknownIterator* CCL_API newEnumIterator () const override;
	tresult CCL_API getTypeDetails (ITypeInfoDetails& result, const ITypeInfo& typeInfo) const override;

	CLASS_INTERFACE (ITypeLibrary, Object)

protected:
	CStringPtr libraryName;
	Container& types;
	Container& enums;

	// ITypeLibrary
	const ITypeInfo* findTypeInfo (CStringPtr name) const override { return findType (name); }
	const IEnumTypeInfo* findEnumTypeInfo (CStringPtr name) const override { return findEnum (name); }
};

} // namespace CCL

#endif // _ccl_typelib_h
