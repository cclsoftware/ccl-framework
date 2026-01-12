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
// Filename    : ccl/base/metaclass.cpp
// Description : Runtime Type Information
//
//************************************************************************************************

#include "ccl/base/kernel.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// AbstractTypeInfo
//************************************************************************************************

const ITypeInfo* CCL_API AbstractTypeInfo::getParentType () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API AbstractTypeInfo::getClassFlags () const
{
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API AbstractTypeInfo::getClassName () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API AbstractTypeInfo::getClassNamespace () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDRef CCL_API AbstractTypeInfo::getClassID () const
{
	return kNullUID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ITypeInfo::MethodDefinition* CCL_API AbstractTypeInfo::getMethodNames () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ITypeInfo::PropertyDefinition* CCL_API AbstractTypeInfo::getPropertyNames () const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ModuleRef CCL_API AbstractTypeInfo::getModuleReference () const
{
	return System::GetCurrentModuleRef ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API AbstractTypeInfo::createInstance () const
{
	return nullptr;
}

//************************************************************************************************
// MetaClass
//************************************************************************************************

MetaClass::MethodNamesModifier::MethodNamesModifier (const MetaClass& This, const MethodDefinition* methodNames)
{
	const_cast<MetaClass&> (This).methodNames = methodNames;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClass::PropertyNamesModifier::PropertyNamesModifier (const MetaClass& This, const PropertyDefinition* propertyNames)
{
	const_cast<MetaClass&> (This).propertyNames = propertyNames;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClass::ClassIDModifier::ClassIDModifier (const MetaClass& This, UIDRef cid)
{
	const_cast<MetaClass&> (This).classID = cid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClass::ClassIDModifier::ClassIDModifier (const MetaClass& This, CStringPtr cidString)
{
	UID cid;
	cid.fromCString (cidString);
	const_cast<MetaClass&> (This).classID = cid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClass::ClassFlagsModifier::ClassFlagsModifier (const MetaClass& This, int flags)
{
	const_cast<MetaClass&> (This).flags = flags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClass::NamespaceModifier::NamespaceModifier (const MetaClass& This, CStringPtr namespaceName)
{
	const_cast<MetaClass&> (This).namespaceName = namespaceName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClass::CategoryModifier::CategoryModifier (const MetaClass& This, CStringPtr categoryName)
{
	const_cast<MetaClass&> (This).categoryName = categoryName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClass::DescriptionModifier::DescriptionModifier (const MetaClass& This, CStringPtr description)
{
	const_cast<MetaClass&> (This).description = description;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClass::AttributeModifier::AttributeModifier (const MetaClass& This, CStringPtr name, int value)
{
	const_cast<MetaClass&> (This).addAttribute (name, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClass::AttributeModifier::AttributeModifier (const MetaClass& This, CStringPtr name, CStringPtr value)
{
	const_cast<MetaClass&> (This).addAttribute (name, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClass::ConstructorModifier::ConstructorModifier (const MetaClass& This, const MetaClass& replacementClass)
{
	ccl_const_cast (This).constructor = replacementClass.constructor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* MetaClass::createInstance (UIDRef cid, void* userData)
{
	MetaClass* metaClass = (MetaClass*)userData;
	ASSERT (metaClass != nullptr)
	if(metaClass)
		return ccl_as_unknown (metaClass->createObject ());
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (MetaClass, 0x7988774a, 0x59c5, 0x47b0, 0xb8, 0x39, 0x94, 0x9c, 0x78, 0x38, 0x3a, 0xe6)

//////////////////////////////////////////////////////////////////////////////////////////////////

MetaClass::MetaClass (const MetaClass* parentClass, CStringPtr className, Object* (*constructor)(),
					  CStringPtr persistentName, bool hidden)
: parentClass (parentClass),
  constructor (constructor),
  className (className),
  persistentName (persistentName),
  methodNames (nullptr),
  propertyNames (nullptr),
  namespaceName (nullptr),
  categoryName (nullptr),
  description (nullptr),
  flags (0),
  attributeCount (0)
{
	ASSERT (parentClass != this) // sanity check
	if(!hidden)
		Kernel::instance ().getClassRegistry ().append (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaClass::isRegistered () const
{
	return Kernel::instance ().getClassRegistry ().getClasses ().contains (const_cast<MetaClass*> (this));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* MetaClass::createObject () const
{
	return constructor ? constructor () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaClass::isClass (const MetaClass& mc) const
{
	return this == &mc;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaClass::canCast (const MetaClass& mc) const
{
	if(isClass (mc))
		return true;
	return parentClass ? parentClass->canCast (mc) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr MetaClass::getPersistentName () const
{
	return persistentName ? persistentName : className;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetaClass::setPersistentName (CStringPtr name)
{
	persistentName = name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr MetaClass::getNamespaceName () const
{
	return namespaceName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr MetaClass::getCategoryName () const
{
	return categoryName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr MetaClass::getDescription () const
{
	return description;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API MetaClass::getClassFlags () const
{
	int result = this->flags;
	if(constructor == nullptr)
		result |= kAbstract;
	if(methodNames != nullptr || propertyNames != nullptr)
		result |= kScriptable;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDRef CCL_API MetaClass::getClassID () const
{
	return classID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MetaClass::operator == (const MetaClass& mc) const
{
	return isClass (mc);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const MetaClass* MetaClass::getParentClass () const
{ 
	return parentClass; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MetaClass::countAttributes () const
{
	return attributeCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr MetaClass::getAttributeName (int index) const
{
	ASSERT (index >= 0 && index < attributeCount)
	return attributes[index].name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetaClass::getAttributeValue (Variant& value, int index) const
{
	ASSERT (index >= 0 && index < attributeCount)
	const Attribute& a = attributes[index];
	switch(a.type)
	{
	case Attribute::kInt : value = a.intValue; break;
	case Attribute::kString : value = a.stringValue; break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetaClass::addAttribute (CStringPtr name, int value)
{
	ASSERT (attributeCount < kMaxAttributes)
	attributes[attributeCount++] = Attribute (name, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetaClass::addAttribute (CStringPtr name, CStringPtr value)
{
	ASSERT (attributeCount < kMaxAttributes)
	attributes[attributeCount++] = Attribute (name, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetaClass::removeAttribute (CStringPtr name)
{
	for(int i = 0; i < attributeCount; i++)
	{
		if(strcmp (attributes[i].name, name) == 0)
		{
			for(int j = i + 1; j < attributeCount; j++)
			{
				attributes[j - 1] = attributes[j];
			}	
			attributeCount--;
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MetaClass::queryInterface (UIDRef iid, void** ptr)
{
	QUERY_INTERFACE (MetaClass)
	QUERY_INTERFACE (ITypeInfo)
	QUERY_UNKNOWN (ITypeInfo)
	*ptr = nullptr;
	return kResultNoInterface;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API MetaClass::retain ()
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int CCL_API MetaClass::release ()
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ITypeInfo* CCL_API MetaClass::getParentType () const
{
	return parentClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API MetaClass::getClassName () const
{
	return getPersistentName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API MetaClass::getClassNamespace () const
{
	return namespaceName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const MetaClass::MethodDefinition* CCL_API MetaClass::getMethodNames () const
{
	return methodNames;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const MetaClass::PropertyDefinition* CCL_API MetaClass::getPropertyNames () const
{
	return propertyNames;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API MetaClass::createInstance () const
{
	return constructor ? static_cast<IObject*> (constructor ()) : nullptr;
}
