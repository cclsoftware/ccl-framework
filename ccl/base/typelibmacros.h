//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/base/typelibmacros.h
// Description : Type Library Class Macros
//
//************************************************************************************************

#ifndef _ccl_typelibmacros_h
#define _ccl_typelibmacros_h

//************************************************************************************************
// Type Library Construction Macros
// -- DEFINE_TYPELIB_ENTRY(_WITH_PARENT/_LOCAL) construct a plain library entry, instantiated via
//    TypeLibEntry. DEFINE_TYPELIB_CLASS(_WITH_PARENT) is the same, but additionally attaches a
//    MetaClass (registered via DECLARE_CLASS/MetaClass), enabling factory instantiation via
//    TypeLibClass. Everything further below (modifiers, members, attributes) applies uniformly to
//    entries constructed either way.
//************************************************************************************************

/** Declares an extern TypeLibEntry for 'UniqueName', so another translation unit can use it as a
 parent. Define with DEFINE_TYPELIB_ENTRY(_WITH_PARENT). */
#define DECLARE_TYPELIB_ENTRY(UniqueName) \
extern CCL::TypeLibEntry __typelibEntry##UniqueName;

/** Initializes a root 'UniqueName' TypeLibEntry (no parent), declared via DECLARE_TYPELIB_ENTRY.
 External linkage, unlike DEFINE_TYPELIB_ENTRY_LOCAL. */
#define DEFINE_TYPELIB_ENTRY(UniqueName, entryName, library) \
CCL::TypeLibEntry CCL::__typelibEntry##UniqueName (library, entryName);

/** Same as DEFINE_TYPELIB_ENTRY, but links to 'Parent's own TypeLibEntry (also declared via
 DECLARE_TYPELIB_ENTRY). */
#define DEFINE_TYPELIB_ENTRY_WITH_PARENT(UniqueName, Parent, entryName, library) \
CCL::TypeLibEntry CCL::__typelibEntry##UniqueName (library, entryName, &__typelibEntry##Parent);

/** Same as DEFINE_TYPELIB_ENTRY, but with internal linkage. Use DECLARE_TYPELIB_ENTRY /
 DEFINE_TYPELIB_ENTRY(_WITH_PARENT) instead if another translation unit needs to see or link to it. */
#define DEFINE_TYPELIB_ENTRY_LOCAL(UniqueName, entryName, library) \
static CCL::TypeLibEntry __typelibEntry##UniqueName (library, entryName);

/** Declares the class's TypeLibClass member and its ITypeLibReference::getReferencedType() override.
 Use alongside DECLARE_CLASS(_ABSTRACT); defined later via DEFINE_TYPELIB_CLASS(_WITH_PARENT). 'Class'
 must match the enclosing class, same as DECLARE_CLASS's own argument. */
#define DECLARE_TYPELIB_CLASS(Class) \
static CCL::TypeLibClass __typeLibClass; \
const CCL::ITypeInfo* CCL_API getReferencedType () const override { return &__typeLibClass; }

/** Same as DEFINE_TYPELIB_ENTRY, but attaches a MetaClass, enabling factory instantiation (see
 TypeLibClass). Pair with DECLARE_TYPELIB_CLASS. Mark abstract with DEFINE_TYPELIB_ABSTRACT. */
#define DEFINE_TYPELIB_CLASS(Class, entryName, library) \
CCL::TypeLibClass Class::__typeLibClass (library, entryName, ccl_typeid<Class> ()); \
static CCL::TypeLibEntry& __typelibEntry##Class = Class::__typeLibClass;

/** Same as DEFINE_TYPELIB_CLASS, but links to 'Parent's own TypeLibClass (also declared via
 DECLARE_TYPELIB_CLASS). */
#define DEFINE_TYPELIB_CLASS_WITH_PARENT(Class, Parent, entryName, library) \
CCL::TypeLibClass Class::__typeLibClass (library, entryName, ccl_typeid<Class> (), &Parent::__typeLibClass); \
static CCL::TypeLibEntry& __typelibEntry##Class = Class::__typeLibClass;

//************************************************************************************************
// Type Library Entry Modifiers
// -- optional, one-line traits added after a DEFINE_TYPELIB_ENTRY/DEFINE_TYPELIB_CLASS line.
//************************************************************************************************

/** Flags the TypeLibEntry declared via DEFINE_TYPELIB_ENTRY(_LOCAL) or DEFINE_TYPELIB_CLASS(_WITH_PARENT)
 as abstract (for a class-backed entry: not directly instantiable). */
#define DEFINE_TYPELIB_ABSTRACT(UniqueName) \
static const CCL::TypeLibEntry::AbstractModifier UNIQUE_IDENT (AbstractModifier) (__typelibEntry##UniqueName);

/** Set the documentation group for the TypeLibEntry declared via DEFINE_TYPELIB_ENTRY(_LOCAL) or
 DEFINE_TYPELIB_CLASS(_WITH_PARENT). */
#define DEFINE_TYPELIB_DOCGROUP(UniqueName, groupName) \
static const CCL::TypeLibEntry::DocGroupModifier UNIQUE_IDENT (DocGroupModifier) \
(__typelibEntry##UniqueName, groupName);

/** Names a plain class whose own documentation should be used for the TypeLibEntry declared
 via DEFINE_TYPELIB_ENTRY(_LOCAL) or DEFINE_TYPELIB_CLASS(_WITH_PARENT). */
#define DEFINE_TYPELIB_RELATED_CLASS(UniqueName, RelatedClass) \
static const CCL::TypeLibEntry::RelatedClassModifier UNIQUE_IDENT (RelatedClassModifier) \
(__typelibEntry##UniqueName, #RelatedClass);

//************************************************************************************************
// Type Library Member Macros
// -- for a TypeLibEntry declared via DECLARE_TYPELIB_ENTRY or defined via
//    DEFINE_TYPELIB_ENTRY(_LOCAL)(_WITH_PARENT) or DEFINE_TYPELIB_CLASS(_WITH_PARENT).
//************************************************************************************************

/** Begin a block of CCL::Model::MemberDescription entries for 'UniqueName'. Close with
 END_TYPELIB_MEMBERS. */
#define BEGIN_TYPELIB_MEMBERS(UniqueName) \
static CCL::Model::MemberDescription __modelMemberDescription##UniqueName[] = {

/** Close a BEGIN_TYPELIB_MEMBERS block. */
#define END_TYPELIB_MEMBERS(UniqueName) \
CCL::Model::MemberDescription () }; \
static CCL::TypeLibEntry::MemberDescriptionModifier UNIQUE_IDENT (__modelMemberModifier) ( \
	__typelibEntry##UniqueName, __modelMemberDescription##UniqueName);

//************************************************************************************************
// Type Library Attribute Macros
//************************************************************************************************

/** Begin a block of CCL::Model::AttributeDescription entries for 'UniqueName'. Close with
 END_TYPELIB_ATTRIBUTES. */
#define BEGIN_TYPELIB_ATTRIBUTES(UniqueName) \
static CCL::Model::AttributeDescription __typelibAttributes##UniqueName[] = {

/** Add an attribute entry within a BEGIN/END_TYPELIB_ATTRIBUTES block. */
#define ADD_TYPELIB_ATTRIBUTE(name, value) \
CCL::Model::AttributeDescription (name, value),

/** Close a BEGIN_TYPELIB_ATTRIBUTES block. */
#define END_TYPELIB_ATTRIBUTES(UniqueName) \
CCL::Model::AttributeDescription () }; \
static CCL::TypeLibEntry::AttributeModifier UNIQUE_IDENT (__typelibAttributeModifier) ( \
	__typelibEntry##UniqueName, __typelibAttributes##UniqueName);

/** Adds 'val' as a schema group this entry belongs to (inherited from parents; an entry is also
 implicitly its own schema group). Use within BEGIN/END_TYPELIB_ATTRIBUTES. */
#define ADD_TYPELIB_SCHEMAGROUP_ATTRIBUTE(val) ADD_TYPELIB_ATTRIBUTE (CCL::Model::kClassSchemaGroups, val)

/** Adds 'val' as the schema group allowed as this entry's children (inherited from parents). Use
 within BEGIN/END_TYPELIB_ATTRIBUTES. */
#define ADD_TYPELIB_CHILDGROUP_ATTRIBUTE(val) ADD_TYPELIB_ATTRIBUTE (CCL::Model::kClassChildGroup, val)

 //************************************************************************************************
 // Type Library Enumeration Macros
 // -- shared by both TypeLibEntry and TypeLibClass declarations.
 //************************************************************************************************

 /** Declares and initializes a TypeLibEnumeration keyed by "entryName.attrName". */
#define DEFINE_TYPELIB_ENUMERATION(entryName, attrName, enumDef, library) \
static CCL::TypeLibEnumeration UNIQUE_IDENT (__modelEnumeration) (library, entryName "." attrName, enumDef);

/** Same as DEFINE_TYPELIB_ENUMERATION, but chained to a parent enum keyed
 "parentEntryName.parentAttrName" (see EnumTypeInfo::parentName). */
#define DEFINE_TYPELIB_ENUMERATION_WITH_PARENT(entryName, attrName, enumDef, parentEntryName, parentAttrName, library) \
static CCL::TypeLibEnumeration UNIQUE_IDENT (__modelEnumeration) \
(library, entryName "." attrName, enumDef, parentEntryName "." parentAttrName);

//************************************************************************************************
// Type Library Member Macros
// -- shared by both TypeLibEntry and TypeLibClass declarations.
//************************************************************************************************

/** Add a string-typed member entry within a BEGIN_TYPELIB_MEMBERS block. */
#define ADD_TYPELIB_MEMBER(name, typeName) \
CCL::Model::MemberDescription (name, CCL::ITypeInfo::kString, typeName),

/** Same as ADD_TYPELIB_MEMBER, but with an explicit ITypeInfo::DataType instead of kString. */
#define ADD_TYPELIB_MEMBER_WITH_TYPE(name, dataType, typeName) \
CCL::Model::MemberDescription (name, dataType, typeName),

/** Same as ADD_TYPELIB_MEMBER_WITH_TYPE, but with a fixed ITypeInfo::kBool data type and "bool" type name. */
#define ADD_TYPELIB_MEMBER_BOOL(name) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kBool, CCL::TypeNames::kBool)

/** Same as ADD_TYPELIB_MEMBER_WITH_TYPE, but with a fixed ITypeInfo::kInt data type and "enum" type name. */
#define ADD_TYPELIB_MEMBER_ENUM(name) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kInt, CCL::TypeNames::kEnum)

/** Same as ADD_TYPELIB_MEMBER_WITH_TYPE, but with a fixed ITypeInfo::kFloat data type and "float" type name. */
#define ADD_TYPELIB_MEMBER_FLOAT(name) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kFloat, CCL::TypeNames::kFloat)

/** Same as ADD_TYPELIB_MEMBER_WITH_TYPE, but with a fixed ITypeInfo::kInt data type and "int" type name. */
#define ADD_TYPELIB_MEMBER_INT(name) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kInt, CCL::TypeNames::kInt)

/** Same as ADD_TYPELIB_MEMBER_WITH_TYPE, but with a fixed ITypeInfo::kObject data type. */
#define ADD_TYPELIB_MEMBER_OBJECT(name, typeName) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kObject, typeName)

/** Same as ADD_TYPELIB_MEMBER_WITH_TYPE, but with a fixed ITypeInfo::kString data type and "string" type name. */
#define ADD_TYPELIB_MEMBER_STRING(name) \
ADD_TYPELIB_MEMBER_WITH_TYPE (name, CCL::ITypeInfo::kString, CCL::TypeNames::kString)

#endif // _ccl_typelibmacros_h
