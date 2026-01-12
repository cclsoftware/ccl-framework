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
// Filename    : ccl/public/base/itypelib.h
// Description : Type Library Interface
//
//************************************************************************************************

#ifndef _ccl_itypelib_h
#define _ccl_itypelib_h

#include "ccl/public/base/iobject.h"
#include "ccl/public/text/cstring.h"

namespace CCL {

interface IUnknownIterator;

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_is_base_of : check if type is derived from given base type
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool ccl_is_base_of (const ITypeInfo* baseType, const ITypeInfo* derivedType)
{
	if(baseType == nullptr || derivedType == nullptr)
		return false;
	if(derivedType == baseType)
		return true;
	if(derivedType->getParentType () == baseType)
		return true;
	return ccl_is_base_of (baseType, derivedType->getParentType ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Class Model Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Model {

//************************************************************************************************
// Model::Attributes
//************************************************************************************************

DEFINE_STRINGID (kClassDocGroup, "Class:DocGroup") ///< class documentation group

//************************************************************************************************
// Model::MemberDescription
//************************************************************************************************

struct MemberDescription
{
	MutableCString name;
	MutableCString typeName;
	DataType type;

	MemberDescription (StringID name = nullptr,
					   DataType type = ITypeInfo::kPrimitive,
					   StringID typeName = nullptr)
   : name (name),
     type (type),
	 typeName (typeName)
	{}
};

} // namespace Model

//************************************************************************************************
// ITypeInfoDetails
/** Type information details interface. 
	\ingroup ccl_base */
//************************************************************************************************

interface CCL_NOVTABLE ITypeInfoDetails: IUnknown
{
	/** Add member description. */
	virtual void CCL_API addMember (const Model::MemberDescription& member) = 0;

	/** Set additional attribute for this type. */
	virtual void CCL_API setAttribute (StringID id, VariantRef value) = 0;

	DECLARE_IID (ITypeInfoDetails)
};

DEFINE_IID (ITypeInfoDetails, 0xb1bd49f6, 0x81ed, 0x4a9c, 0x87, 0x15, 0x5c, 0x85, 0xc8, 0x7f, 0x6, 0x60)

//************************************************************************************************
// IEnumTypeInfo
/** Enumeration type information interface. 
	\ingroup ccl_base */
//************************************************************************************************

interface CCL_NOVTABLE IEnumTypeInfo: IUnknown
{
	/** Get name as null-terminated ASCII string. */
	virtual CStringPtr CCL_API getName () const = 0;

	/** Get optional parent name as null-terminated ASCII string. */
	virtual CStringPtr CCL_API getParentName () const = 0;
	
	/** Get number of enumerators. */
	virtual int CCL_API getEnumeratorCount () const = 0;
	
	/** Get enumerator name and value by index. */
	virtual tbool CCL_API getEnumerator (MutableCString& name, Variant& value, int index) const = 0;

	DECLARE_IID (IEnumTypeInfo)
};

DEFINE_IID (IEnumTypeInfo, 0xbdb450c0, 0xdc4e, 0x4097, 0x96, 0xb6, 0xc5, 0x1e, 0x31, 0xfe, 0x51, 0x93)

//************************************************************************************************
// ITypeLibrary
/** Type library interface. 
	\ingroup ccl_base */
//************************************************************************************************

interface CCL_NOVTABLE ITypeLibrary: IUnknown
{
	/** Get name of type library. */
	virtual CStringPtr CCL_API getLibraryName () const = 0;
	
	/** Create iterator of ITypeInfo objects. */
	virtual IUnknownIterator* CCL_API newTypeIterator () const = 0;
	
	/** Create iterator of IEnumTypeInfo objects. */
	virtual IUnknownIterator* CCL_API newEnumIterator () const = 0;

	/** Get details of given type (optional). */
	virtual tresult CCL_API getTypeDetails (ITypeInfoDetails& result, const ITypeInfo& typeInfo) const = 0;

	/** Find ITypeInfo object by name. */
	virtual const ITypeInfo* findTypeInfo (CStringPtr name) const = 0;

	/** Find IEnumTypeInfo object by name. */
	virtual const IEnumTypeInfo* findEnumTypeInfo (CStringPtr name) const = 0;

	DECLARE_IID (ITypeLibrary)
};

DEFINE_IID (ITypeLibrary, 0x25ca4b07, 0x72b2, 0x46d9, 0xa5, 0x9f, 0xc5, 0xb3, 0x98, 0xb4, 0xc3, 0xbf)

} // namespace CCL

#endif // _ccl_itypelib_h
