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
// Filename    : ccl/base/typelib.cpp
// Description : Type Library
//
//************************************************************************************************

#include "ccl/base/typelib.h"

#include "ccl/base/boxedtypes.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/base/storage/attributes.h"

using namespace CCL;

//************************************************************************************************
// TypeInfo
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TypeInfo, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

TypeInfo::TypeInfo (CStringPtr name, const TypeInfo* parentType)
: name (name),
  parentType (parentType)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ITypeInfo* CCL_API TypeInfo::getParentType () const
{
	return parentType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API TypeInfo::getClassName () const
{
	return name;
}

//************************************************************************************************
// EnumTypeInfo
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (EnumTypeInfo, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

EnumTypeInfo::EnumTypeInfo (CStringPtr name, CStringPtr parentName)
: name (name),
  parentName (parentName)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API EnumTypeInfo::getName () const
{
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API EnumTypeInfo::getParentName () const
{
	return parentName;
}

//************************************************************************************************
// CStringEnumTypeInfo
//************************************************************************************************

CStringEnumTypeInfo::CStringEnumTypeInfo (CStringPtr name, CStringPtr enumerators[], int count)
: EnumTypeInfo (name),
  enumerators (enumerators),
  count (count)
{
	ASSERT (count > 0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API CStringEnumTypeInfo::getEnumeratorCount () const
{
	return count;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CStringEnumTypeInfo::getEnumerator (MutableCString& name, Variant& value, int index) const
{
	name = enumerators[ccl_bound (index, 0, count-1)];
	value = index;
	return true;
}

//************************************************************************************************
// MutableEnumTypeInfo
//************************************************************************************************

MutableEnumTypeInfo::MutableEnumTypeInfo (CStringPtr name)
: EnumTypeInfo (name),
  enumerators (*NEW ObjectArray)
{
	enumerators.objectCleanup (true);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

MutableEnumTypeInfo::~MutableEnumTypeInfo ()
{
	enumerators.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MutableEnumTypeInfo::addEnumerator (StringID name, VariantRef value)
{
	enumerators.add (NEW Boxed::VariantWithName (value, String (name)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MutableEnumTypeInfo::getEnumeratorCount () const
{
	return enumerators.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MutableEnumTypeInfo::getEnumerator (MutableCString& name, Variant& value, int index) const
{
	Boxed::VariantWithName* e = (Boxed::VariantWithName*)enumerators.at (index);
	ASSERT (e != nullptr)
	if(e == nullptr)
		return false;

	name = e->getName ();
	value = static_cast<const Variant&> (*e);
	value.share ();
	return true;
}

//************************************************************************************************
// TypeLibrary
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TypeLibrary, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

TypeLibrary::TypeLibrary (CStringPtr libraryName)
: libraryName (libraryName),
  types (*NEW ObjectList),
  enums (*NEW ObjectList)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

TypeLibrary::~TypeLibrary ()
{
	types.release ();
	enums.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TypeLibrary::objectCleanup (bool state)
{
	types.objectCleanup (state);
	enums.objectCleanup (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibrary::addType (TypeInfo* typeInfo, bool checkExistence)
{
	if(checkExistence && findType (typeInfo->getClassName ()))
		return false;

	return types.add (typeInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibrary::addEnum (EnumTypeInfo* e, bool checkExistence)
{
	if(checkExistence && findEnum (e->getName ()))
		return false;

	return enums.add (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const TypeInfo* TypeLibrary::findType (StringID name, bool caseSensitive) const
{
	ForEach (types, TypeInfo, ti)
		if(name.compare (ti->getClassName (), caseSensitive) == 0)
			return ti;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const EnumTypeInfo* TypeLibrary::findEnum (StringID name, bool caseSensitive) const
{
	ForEach (enums, EnumTypeInfo, ei)
		if(name.compare (ei->getName (), caseSensitive) == 0)
			return ei;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr TypeLibrary::getLibraryName () const
{
	return libraryName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API TypeLibrary::newTypeIterator () const
{
	return types.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknownIterator* CCL_API TypeLibrary::newEnumIterator () const
{
	return enums.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TypeLibrary::getTypeDetails (ITypeInfoDetails& result, const ITypeInfo& typeInfo) const
{
	TypeInfo* ti = unknown_cast<TypeInfo> ((IUnknown*)&typeInfo);
	ASSERT (ti != nullptr)
	if(ti == nullptr)
		return kResultInvalidArgument;

	return ti->getDetails (result) ? kResultOk : kResultFalse;
}

//************************************************************************************************
// TypeLibEntry::MemberDescriptionModifier
//************************************************************************************************

TypeLibEntry::MemberDescriptionModifier::MemberDescriptionModifier (TypeLibEntry& This, Model::MemberDescription members[])
{
	This.members = members;
}

//************************************************************************************************
// TypeLibEntry::AttributeModifier
//************************************************************************************************

TypeLibEntry::AttributeModifier::AttributeModifier (TypeLibEntry& This, Model::AttributeDescription attributes[])
{
	This.attributes = attributes;
}

//************************************************************************************************
// TypeLibEntry::DocGroupModifier
//************************************************************************************************

TypeLibEntry::DocGroupModifier::DocGroupModifier (TypeLibEntry& This, CStringPtr docGroup)
{
	This.docGroup = docGroup;
};

//************************************************************************************************
// TypeLibEntry::RelatedClassModifier
//************************************************************************************************

TypeLibEntry::RelatedClassModifier::RelatedClassModifier (TypeLibEntry& This, CStringPtr relatedClass)
{
	This.relatedClass = relatedClass;
};

//************************************************************************************************
// TypeLibEntry::AbstractModifier
//************************************************************************************************

TypeLibEntry::AbstractModifier::AbstractModifier (TypeLibEntry& This)
{
	This.abstract = true;
}

//************************************************************************************************
// TypeLibEntry
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TypeLibEntry, TypeInfo)

//////////////////////////////////////////////////////////////////////////////////////////////////

TypeLibEntry::TypeLibEntry (TypeLibrary& library, CStringPtr entryName, const TypeLibEntry* parentType)
: TypeInfo (entryName, parentType),
  docGroup (nullptr),
  relatedClass (nullptr),
  members (nullptr),
  attributes (nullptr),
  abstract (false)
{
	library.addType (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TypeLibEntry::getClassFlags () const
{
	return abstract ? kAbstract : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeLibEntry::getDetails (ITypeInfoDetails& details) const
{
	if(attributes)
	{
		Attributes resultAttributes;
		for(Model::AttributeDescription* a = attributes; !a->name.isEmpty (); a++)
		{
			Variant value (a->value);
			if(value.isString () && resultAttributes.getAttribute (value, a->name))
			{
				String string = value.asString ().append (" ").append (a->value.asString ());
				value = string;
				value.share ();
			}

			resultAttributes.setAttribute (a->name, value);
		}

		ForEachAttribute (resultAttributes, name, value)
			details.setAttribute (name, value);
		EndFor
	}

	if(docGroup)
		details.setAttribute (Model::kClassDocGroup, docGroup);

	if(members)
	{
		for(Model::MemberDescription* m = members; !m->name.isEmpty (); m++)
			details.addMember (*m);
	}

	return true;
}

//************************************************************************************************
// TypeLibClass
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TypeLibClass, TypeLibEntry)

//////////////////////////////////////////////////////////////////////////////////////////////////

TypeLibClass::TypeLibClass (TypeLibrary& library, CStringPtr entryName, MetaClassRef _metaClass, const TypeLibEntry* parentType)
: TypeLibEntry (library, entryName, parentType),
  metaClass (_metaClass)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClassRef TypeLibClass::getMetaClass () const
{
	return this->metaClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* TypeLibClass::createObject () const
{
	return metaClass.createObject ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API TypeLibClass::getClassFlags () const
{
	return TypeLibEntry::getClassFlags () | static_cast<const ITypeInfo&> (metaClass).getClassFlags ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API TypeLibClass::createInstance () const
{
	return static_cast<const ITypeInfo&> (metaClass).createInstance ();
}

//************************************************************************************************
// TypeLibEnumeration
//************************************************************************************************

TypeLibEnumeration::TypeLibEnumeration (TypeLibrary& library, CStringPtr canonicalName, const EnumInfo info[], CStringPtr parentName)
: TEnumTypeInfo<EnumInfo> (canonicalName, info, info ? EnumInfo::getCount (info) : 0, parentName)
{
	library.addEnum (this);
}
