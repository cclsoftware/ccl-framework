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
// Filename    : ccl/base/metaclass.h
// Description : Runtime Type Information
//
//************************************************************************************************

#ifndef _ccl_metaclass_h
#define _ccl_metaclass_h

#include "ccl/public/base/uid.h"
#include "ccl/public/base/iobject.h"
#include "ccl/public/base/cclmacros.h"

namespace CCL {

class Object;
class MetaClass;

/** Meta class reference. */
typedef const MetaClass& MetaClassRef;

//************************************************************************************************
// AbstractTypeInfo 
/** Abstract type information base class.  */
//************************************************************************************************

class AbstractTypeInfo: public ITypeInfo
{
public:
	// ITypeInfo
	const ITypeInfo* CCL_API getParentType () const override;
	int CCL_API getClassFlags () const override;
	CStringPtr CCL_API getClassName () const override;
	CStringPtr CCL_API getClassNamespace () const override;
	UIDRef CCL_API getClassID () const override;
	const MethodDefinition* CCL_API getMethodNames () const override;
	const PropertyDefinition* CCL_API getPropertyNames () const override;
	ModuleRef CCL_API getModuleReference () const override;
	IUnknown* CCL_API createInstance () const override;
};

//************************************************************************************************
// MetaClass
/** Runtime meta class for library objects.  \ingroup ccl_base  */
//************************************************************************************************

class MetaClass: public AbstractTypeInfo
{
public:
	MetaClass (const MetaClass* parentClass,
			   CStringPtr className,
			   Object* (*constructor) (),
			   CStringPtr persistentName = nullptr,
			   bool hidden = false);

	/** Check if meta class is registered. */
	bool isRegistered () const;

	/** Compare meta class. */
	bool isClass (const MetaClass&) const;

	/** Compare meta class for dynamic cast. */
	bool canCast (const MetaClass&) const;

	/** Compare meta class. */
	bool operator == (const MetaClass& mc) const;

	// Class flags
	PROPERTY_READONLY_FLAG (flags, kSingleton, isSingleton)
	
	/** Get persistent class name. */
	CStringPtr getPersistentName () const;

	/** Set persistent class name. */
	void setPersistentName (CStringPtr name);

	/** Get namespace as string (can be null). */
	CStringPtr getNamespaceName () const;

	/** Get category as string (can be null). */
	CStringPtr getCategoryName () const;

	/** Get description (can be null). */
	CStringPtr getDescription () const;

	/** Get unique class identifier [ITypeInfo]. */
	UIDRef CCL_API getClassID () const override;

	/** Create object instance of this class. */
	Object* createObject () const;

	/** Get meta class of parent class (returns null for base class). */
	const MetaClass* getParentClass () const;

	/** Get number of class attributes. */
	int countAttributes () const;

	/** Get attribute name at given index. */
	CStringPtr getAttributeName (int index) const;

	/** Get attribute value at given index. */
	void getAttributeValue (Variant& value, int index) const;

	/** Remove attribute with given name */
	void removeAttribute (CStringPtr name);

	/** Modifies method names in ctor. */
	struct MethodNamesModifier
	{
		MethodNamesModifier (const MetaClass& This, const MethodDefinition* methodNames);
	};

	/** Modifies property names in ctor. */
	struct PropertyNamesModifier
	{
		PropertyNamesModifier (const MetaClass& This, const PropertyDefinition* propertyNames);
	};

	/** Modifies class flags in ctor. */
	struct ClassFlagsModifier
	{
		ClassFlagsModifier (const MetaClass& This, int flags);
	};

	/** Modifies class id in ctor. */
	struct ClassIDModifier
	{
		ClassIDModifier (const MetaClass& This, UIDRef cid);
		ClassIDModifier (const MetaClass& This, CStringPtr cidString);
	};

	/** Modifies namespace name in ctor. */
	struct NamespaceModifier
	{
		NamespaceModifier (const MetaClass& This, CStringPtr namespaceName);
	};

	/** Modifies category name in ctor. */
	struct CategoryModifier
	{
		CategoryModifier (const MetaClass& This, CStringPtr categoryName);
	};

	/** Modifies description in ctor. */
	struct DescriptionModifier
	{
		DescriptionModifier (const MetaClass& This, CStringPtr description);
	};

	/** Adds class attribute in ctor. */
	struct AttributeModifier
	{
		AttributeModifier (const MetaClass& This, CStringPtr name, int value);
		AttributeModifier (const MetaClass& This, CStringPtr name, CStringPtr value);
	};

	/** Replaces the constructor function with the one of another class in ctor. */
	struct ConstructorModifier
	{
		ConstructorModifier (const MetaClass& This, const MetaClass& replacementClass);
	};

	DECLARE_UNKNOWN
	DECLARE_IID (MetaClass)

	/** Create function for use with factory and meta class in userData. */
	static IUnknown* createInstance (UIDRef cid, void* userData);

protected:
	const MetaClass* parentClass;
	Object* (*constructor) ();
	CStringPtr className;
	CStringPtr persistentName;
	const MethodDefinition* methodNames;
	const PropertyDefinition* propertyNames;
	CStringPtr namespaceName;
	CStringPtr categoryName;
	CStringPtr description;
	UID classID;
	int flags;

	struct Attribute
	{
		enum Type { kInt, kString };

		Type type;
		CStringPtr name;
		union 
		{
			int intValue;
			CStringPtr stringValue;
		};

		Attribute (CStringPtr name = nullptr, int value = 0): type (kInt), name (name), intValue (value) {}
		Attribute (CStringPtr name, CStringPtr value): type (kString), name (name), stringValue (value) {}
	};

	static constexpr int kMaxAttributes = 8;
	Attribute attributes[kMaxAttributes];
	int attributeCount;

	void addAttribute (CStringPtr name, int value);
	void addAttribute (CStringPtr name, CStringPtr value);

	// ITypeInfo
	const ITypeInfo* CCL_API getParentType () const override;
	int CCL_API getClassFlags () const override;
	CStringPtr CCL_API getClassName () const override;
	CStringPtr CCL_API getClassNamespace () const override;
	const MethodDefinition* CCL_API getMethodNames () const override;
	const PropertyDefinition* CCL_API getPropertyNames () const override;
	IUnknown* CCL_API createInstance () const override;
};

} // namespace CCL

#endif // _ccl_metaclass_h
