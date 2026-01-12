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
// Filename    : ccl/public/base/iobject.h
// Description : Object Interface
//
//************************************************************************************************

#ifndef _ccl_iobject_h
#define _ccl_iobject_h

#include "ccl/public/base/imessage.h"

namespace CCL {

//************************************************************************************************
// DataType
//************************************************************************************************

typedef CCL::int32 DataType;

//************************************************************************************************
// TypeNames
/** Canonical type names corresponding to DataType. */
//************************************************************************************************

namespace TypeNames
{
	const CStringPtr kInt = "int";
	const CStringPtr kFloat = "float";
	const CStringPtr kString = "string";
	const CStringPtr kBool = "bool";
	const CStringPtr kEnum = "enum";
	const CStringPtr kVoid = "void";
	const CStringPtr kObject = "object";
	const CStringPtr kContainer = "container";
	const CStringPtr kVariant = "variant";
}

//************************************************************************************************
// ITypeInfo
/** Type information interface.
	\ingroup ccl_base */
//************************************************************************************************

interface CCL_NOVTABLE ITypeInfo: IUnknown
{
	/** Class flags. */
	enum Flags
	{
		kAbstract   = 1<<0,	///< class is abstract
		kScriptable = 1<<1,	///< class is scriptable
		kSingleton  = 1<<2,	///< class instance is a singleton
		kMutable    = 1<<3	///< class instance type is mutable at runtime
	};

	/** Data types. */
	enum DataTypes
	{
		kVoid      = 0,
		kPrimitive = 0x01,
		kObject	   = 0x02,
		kContainer = 0x03,
		kComposite = 0x04,	///< multiple alternative types
		kVariant   = 0x05,	///< can be int, float, string or object

		kInt    = kPrimitive | 0x0100,
		kFloat	= kPrimitive | 0x0200,
		kString = kPrimitive | 0x0300,
		kBool	= kPrimitive | 0x0400,
		kBlob	= kPrimitive | 0x0500,

		kReadOnly = 0x010000	///< flag for properties that can only be read
	};

	/** Method definition. */
	struct MethodDefinition
	{
		CStringPtr name;		///< method name
		CStringPtr args;		///< argument list
		CStringPtr retval;		///< return value
	};

	/** Property definition. */
	struct PropertyDefinition
	{
		CStringPtr name;			///< property name
		DataType type;				///< data type
		CStringPtr typeName;		///< type name
		const ITypeInfo* typeInfo;	///< type of referenced (kObject) or contained (kContainer) objects
	};

	/** Get type info of parent class (returns null for base class). */
	virtual const ITypeInfo* CCL_API getParentType () const = 0;

	/** Get class flags. */
	virtual int CCL_API getClassFlags () const = 0;

	/** Get class name as null-terminated ASCII string. */
	virtual CStringPtr CCL_API getClassName () const = 0;

	/** Get class namespace as null-terminated ASCII string, optional. */
	virtual CStringPtr CCL_API getClassNamespace () const = 0;

	/** Get unique class identifier if available, empty otherwise. */
	virtual UIDRef CCL_API getClassID () const = 0;

	/** Get null-terminated array of method definitions (optional). */
	virtual const MethodDefinition* CCL_API getMethodNames () const = 0;

	/** Get null-terminated array of property definitions (optional). */
	virtual const PropertyDefinition* CCL_API getPropertyNames () const = 0;

	/** Get reference of module this class resides in. */
	virtual ModuleRef CCL_API getModuleReference () const = 0;

	/** Create object instance of this class. */
	virtual IUnknown* CCL_API createInstance () const = 0;

	DECLARE_IID (ITypeInfo)
};

DEFINE_IID (ITypeInfo, 0xaa0ad2d0, 0x65da, 0x4d7e, 0xb0, 0x63, 0x6d, 0x29, 0x8f, 0xb3, 0xda, 0x4b)

//************************************************************************************************
// IPropertyCollector
/** Property collector interface.
	\ingroup ccl_base */
//************************************************************************************************

interface CCL_NOVTABLE IPropertyCollector: IUnknown
{
	/** Add property with given name. */
	virtual void CCL_API addPropertyName (CStringPtr name) = 0;

	/** Add array of property names (count = -1: array is null-terminated). */
	virtual void CCL_API addPropertyNames (CStringPtr names[], int count) = 0;

	/** Add property with given definition. */
	virtual void CCL_API addProperty (const ITypeInfo::PropertyDefinition& propDef) = 0;

	DECLARE_IID (IPropertyCollector)
};

DEFINE_IID (IPropertyCollector, 0xcc5c0b76, 0x27e2, 0x49bb, 0xb1, 0xff, 0xf4, 0x3c, 0x93, 0x3a, 0xe9, 0x65)

//************************************************************************************************
// IObject
/** Basic object interface.
	\ingroup ccl_base */
//************************************************************************************************

interface CCL_NOVTABLE IObject: IUnknown
{
	/** Property and method identifier. */
	typedef CStringRef MemberID;

	/** Get type information. */
	virtual const ITypeInfo& CCL_API getTypeInfo () const = 0;

	/** Get property value by name. */
	virtual tbool CCL_API getProperty (Variant& var, MemberID propertyId) const = 0;

	/** Set property value by name. */
	virtual tbool CCL_API setProperty (MemberID propertyId, const Variant& var) = 0;

	/** Get all property names of this object. */
	virtual tbool CCL_API getPropertyNames (IPropertyCollector& collector) const = 0;

	/** Call method. */
	virtual tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) = 0;

	DECLARE_IID (IObject)
};

DEFINE_IID (IObject, 0x2e726012, 0x522c, 0x4108, 0x98, 0x62, 0xc, 0xd5, 0x71, 0x79, 0x52, 0x49)

} // namespace CCL

#endif // _ccl_iobject_h
