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
// Filename    : ccl/base/objectmacros.h
// Description : Object class macros
//
//************************************************************************************************

#ifndef _ccl_objectmacros_h
#define _ccl_objectmacros_h

//************************************************************************************************
// Class macros
//************************************************************************************************

//////////////////////////////////////////////////////////////////////////////////////////////////
/*
	Example:

	class MyClass: public Object
	{
	public:
		DECLARE_CLASS (MyClass, Object)
	};

	DEFINE_CLASS (MyClass, Object)
*/
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Declare RTTI for a base class */
#define DECLARE_BASE_CLASS(Class)								\
static CCL::Object* __create () { return NEW Class; }			\
static const CCL::MetaClass __class;							\
private: static const CCL::MetaClass& __typeid ()				\
{ return __class; }												\
friend const CCL::MetaClass& CCL::ccl_typeid<Class> ();			\
public:															\
virtual const CCL::MetaClass& myClass () const					\
{ return __class; }												\
virtual bool isClass (const CCL::MetaClass& mc) const			\
{ return __class.isClass (mc); }								\
virtual bool canCast (const CCL::MetaClass& mc) const			\
{ return __class.canCast (mc); }								\
virtual CCL::Object* clone () const								\
{ return NEW Class (*this); }

/** Declare RTTI for a class */
#define DECLARE_CLASS(Class, Parent)							\
typedef Parent SuperClass;										\
static CCL::Object* __create () { return NEW Class; }			\
static const CCL::MetaClass __class;							\
private: static const CCL::MetaClass& __typeid ()				\
{ return __class; }												\
friend const CCL::MetaClass& CCL::ccl_typeid<Class> ();			\
public:															\
virtual const CCL::MetaClass& myClass () const override			\
{ return __class; }												\
virtual bool isClass (const CCL::MetaClass& mc) const override	\
{ return __class.isClass (mc); }								\
virtual bool canCast (const CCL::MetaClass& mc) const override	\
{ return __class.canCast (mc); }								\
virtual CCL::Object* clone () const override					\
{ return NEW Class (*this); }

/** Declare RTTI for abstract class */
#define DECLARE_CLASS_ABSTRACT(Class, Parent)					\
typedef Parent SuperClass;										\
static const CCL::MetaClass __class;							\
private: static const CCL::MetaClass& __typeid ()				\
{ return __class; }												\
friend const CCL::MetaClass& CCL::ccl_typeid<Class> ();			\
public:															\
virtual const CCL::MetaClass& myClass () const	override		\
{ return __class; }												\
virtual bool isClass (const CCL::MetaClass& mc) const override	\
{ return __class.isClass (mc); }								\
virtual bool canCast (const CCL::MetaClass& mc) const override	\
{ return __class.canCast (mc); }								\

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Define RTTI for a super class */
#define DEFINE_BASE_CLASS(Class)								\
const CCL::MetaClass Class::__class								\
(nullptr, #Class, Class::__create);

/** Define RTTI and add to class registry */
#define DEFINE_CLASS(Class, Parent)								\
const CCL::MetaClass Class::__class								\
(&Parent::__class, #Class,										\
 Class::__create);

/** Define RTTI *without* adding to class registry */
#define DEFINE_CLASS_HIDDEN(Class, Parent)						\
const CCL::MetaClass Class::__class								\
(&Parent::__class, #Class,										\
 Class::__create, nullptr, true);

/** Define RTTI with explicite storage name */
#define DEFINE_CLASS_PERSISTENT(Class, Parent, Name)			\
const CCL::MetaClass Class::__class								\
(&Parent::__class, #Class, Class::__create,	Name);

/** Define RTTI for abstract class */
#define DEFINE_CLASS_ABSTRACT(Class, Parent)					\
const CCL::MetaClass Class::__class								\
(&Parent::__class, #Class, nullptr, nullptr, false);

/** Define RTTI for abstract class without adding to class registry */
#define DEFINE_CLASS_ABSTRACT_HIDDEN(Class, Parent)				\
const CCL::MetaClass Class::__class								\
(&Parent::__class, #Class, nullptr, nullptr, true);

/** Define RTTI for singleton. */
#define DEFINE_SINGLETON_CLASS(Class, Parent)					\
const CCL::MetaClass Class::__class								\
(&Parent::__class, #Class, Class::__createSingleton);			\
DEFINE_CLASS_FLAGS (Class, CCL::ITypeInfo::kSingleton)

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Declare method names for IObject in class scope. */
#define DECLARE_METHOD_NAMES(ClassName) \
static const CCL::MetaClass::MethodDefinition __methodNames[];

/** Begin method name definition. */
#define BEGIN_METHOD_NAMES(ClassName) \
const CCL::MetaClass::MethodDefinition ClassName::__methodNames[] = {

/** End method name definition (null-terminated). */
#define END_METHOD_NAMES(ClassName) {nullptr,nullptr,nullptr}}; \
static const CCL::MetaClass::MethodNamesModifier UNIQUE_IDENT (MethodNamesModifier) \
(CCL::ccl_typeid<ClassName> (), ClassName::__methodNames);

/** Define method with given name (no arguments, return type is void). */
#define DEFINE_METHOD_NAME(name) \
{name, nullptr, nullptr},

/** Define method with given name and arguments (return type is void). */
#define DEFINE_METHOD_ARGS(name, args) \
{name, args, nullptr},

/** Define method with given name, arguments and return type. */
#define DEFINE_METHOD_ARGR(name, args, retval) \
{name, args, retval},

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Declare property names for IObject in class scope. */
#define DECLARE_PROPERTY_NAMES(ClassName) \
static const CCL::MetaClass::PropertyDefinition __propertyNames[];

/** Begin property name definition. */
#define BEGIN_PROPERTY_NAMES(ClassName) \
const CCL::MetaClass::PropertyDefinition ClassName::__propertyNames[] = {

/** End property name definition (null-terminated). */
#define END_PROPERTY_NAMES(ClassName) {nullptr}}; \
static const CCL::MetaClass::PropertyNamesModifier UNIQUE_IDENT (PropertyNamesModifier) \
(CCL::ccl_typeid<ClassName> (), ClassName::__propertyNames);

/** Define property with given name (without type information). */
#define DEFINE_PROPERTY_NAME(name) \
{name, CCL::ITypeInfo::kVoid, nullptr, nullptr},

/** Define property with given name and datatype. */
#define DEFINE_PROPERTY_TYPE(name,type) \
{name, type, nullptr, nullptr},

/** Define object property with given name and class name (string) of referenced object. */
#define DEFINE_PROPERTY_CLASS(name,className) \
{name, CCL::ITypeInfo::kObject, className, nullptr},

/** Define object property with given name and class name (string) of referenced object, with additional flags, e.g. ITypeInfo::kReadOnly. */
#define DEFINE_PROPERTY_CLASS_(name,className,flags) \
{name, CCL::ITypeInfo::kObject | flags, className, nullptr},

/** Define object property with given name and class of referenced object (stores MetaClass reference). */
#define DEFINE_PROPERTY_METACLASS(name,ClassName) \
{name, CCL::ITypeInfo::kObject, #ClassName, &CCL::ccl_typeid<ClassName> ()},

/** Define container property with given name and class of contained objects. */
#define DEFINE_PROPERTY_CONTAINER(name,ClassName) \
{name, CCL::ITypeInfo::kContainer, #ClassName, &CCL::ccl_typeid<ClassName> ()},

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Define class id for public classes (UID bytes). */
#define DEFINE_CLASS_UID(ClassName, data1, data2, data3, a, b, c, d, e, f, g, h) \
static const CCL::MetaClass::ClassIDModifier UNIQUE_IDENT (ClassIDModifier) \
(CCL::ccl_typeid<ClassName> (), CCL::UID (data1, data2, data3, a, b, c, d, e, f, g, h));

/** Define class id for public classes (string). */
#define DEFINE_CLASS_UID_STRING(ClassName, cidString) \
static const CCL::MetaClass::ClassIDModifier UNIQUE_IDENT (ClassIDModifier) \
(CCL::ccl_typeid<ClassName> (), cidString);

/** Define flags for class. */
#define DEFINE_CLASS_FLAGS(ClassName, flags) \
static const CCL::MetaClass::ClassFlagsModifier UNIQUE_IDENT (ClassFlagsModifier) \
(CCL::ccl_typeid<ClassName> (), flags);

/** Define namespace name for class. */
#define DEFINE_CLASS_NAMESPACE(ClassName, namespaceName) \
static const CCL::MetaClass::NamespaceModifier UNIQUE_IDENT (NamespaceModifier) \
(CCL::ccl_typeid<ClassName> (), namespaceName);

/** Define category name for class. */
#define DEFINE_CLASS_CATEGORY(ClassName, categoryName) \
static const CCL::MetaClass::CategoryModifier UNIQUE_IDENT (CategoryModifier) \
(CCL::ccl_typeid<ClassName> (), categoryName);

/** Define description for class. */
#define DEFINE_CLASS_DESCRIPTION(ClassName, description) \
static const CCL::MetaClass::DescriptionModifier UNIQUE_IDENT (DescriptionModifier) \
(CCL::ccl_typeid<ClassName> (), description);

/** Define attribute for class. */
#define DEFINE_CLASS_ATTRIBUTE(ClassName, name, value) \
static const CCL::MetaClass::AttributeModifier UNIQUE_IDENT (AttributeModifier) \
(CCL::ccl_typeid<ClassName> (), name, value);

/** Define constructor function for class, taken from another (e.g. derived) class. */
#define DEFINE_CLASS_CONSTRUCTOR(ClassName, ReplacementClassName) \
static const CCL::MetaClass::ConstructorModifier UNIQUE_IDENT (ConstructorModifier) \
(CCL::ccl_typeid<ClassName> (), CCL::ccl_typeid<ReplacementClassName> ());

//////////////////////////////////////////////////////////////////////////////////////////////////
// Helper macros for template classes derived from Object
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Helper to pass template class type names to macros as a single argument */
#define TEMPLATE_CLASS_TYPE_ARGUMENT(...) __VA_ARGS__

/** Declare RTTI for a class with one template argument */
#define DECLARE_TEMPLATE_CLASS(Class, Type, Parent) \
DECLARE_CLASS (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type>), Parent)

/** Declare RTTI for a class with two template arguments */
#define DECLARE_TEMPLATE_CLASS2(Class, Type1, Type2, Parent) \
DECLARE_CLASS (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type1, Type2>), Parent)

/** Declare RTTI for a class with three template arguments */
#define DECLARE_TEMPLATE_CLASS3(Class, Type1, Type2, Type3, Parent) \
DECLARE_CLASS (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type1, Type2, Type3>), Parent)

/** Declare RTTI for abstract class with one template argument */
#define DECLARE_TEMPLATE_CLASS_ABSTRACT(Class, Type, Parent) \
DECLARE_CLASS_ABSTRACT (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type>), Parent)

/** Declare RTTI for abstract class with two template arguments */
#define DECLARE_TEMPLATE_CLASS_ABSTRACT2(Class, Type1, Type2, Parent) \
DECLARE_CLASS_ABSTRACT (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type1, Type2>), Parent)

/** Declare RTTI for abstract class with three template arguments */
#define DECLARE_TEMPLATE_CLASS_ABSTRACT3(Class, Type1, Type2, Type3, Parent) \
DECLARE_CLASS_ABSTRACT (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type1, Type2, Type3>), Parent)

/** Define RTTI for class with one template argument and add to class registry */
#define DEFINE_TEMPLATE_CLASS(Class, Type, Parent) \
template<typename Type> \
DEFINE_CLASS (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type>), Parent);

/** Define RTTI for class with two template arguments and add to class registry */
#define DEFINE_TEMPLATE_CLASS2(Class, Type1, Type2, Parent) \
template<typename Type1, typename Type2> \
DEFINE_CLASS (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type1, Type2>), Parent);

/** Define RTTI for class with three template arguments and add to class registry */
#define DEFINE_TEMPLATE_CLASS3(Class, Type1, Type2, Type3, Parent) \
template<typename Type1, typename Type2, typename Type3> \
DEFINE_CLASS (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type1, Type2, Type3>), Parent);

/** Define RTTI for class with one template argument *without* adding to class registry */
#define DEFINE_TEMPLATE_CLASS_HIDDEN(Class, Type, Parent) \
template<typename Type> \
DEFINE_CLASS_HIDDEN (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type>), Parent);

/** Define RTTI for class with two template arguments *without* adding to class registry */
#define DEFINE_TEMPLATE_CLASS_HIDDEN2(Class, Type1, Type2, Parent) \
template<typename Type1, typename Type2> \
DEFINE_CLASS_HIDDEN (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type1, Type2>), Parent);

/** Define RTTI for class with three template arguments *without* adding to class registry */
#define DEFINE_TEMPLATE_CLASS_HIDDEN3(Class, Type1, Type2, Type3, Parent) \
template<typename Type1, typename Type2, typename Type3> \
DEFINE_CLASS_HIDDEN (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type1, Type2, Type3>), Parent);

/** Define RTTI for abstract class with one template argument */
#define DEFINE_TEMPLATE_CLASS_ABSTRACT(Class, Type, Parent) \
template<typename Type> \
DEFINE_CLASS_ABSTRACT (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type>), Parent);

/** Define RTTI for abstract class with two template arguments */
#define DEFINE_TEMPLATE_CLASS_ABSTRACT2(Class, Type1, Type2, Parent) \
template<typename Type1, typename Type2> \
DEFINE_CLASS_ABSTRACT (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type1, Type2>), Parent);

/** Define RTTI for abstract class with three template arguments */
#define DEFINE_TEMPLATE_CLASS_ABSTRACT3(Class, Type1, Type2, Type3, Parent) \
template<typename Type1, typename Type2, typename Type3> \
DEFINE_CLASS_ABSTRACT (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type1, Type2, Type3>), Parent);

/** Define RTTI for abstract class with one template argument without adding to class registry */
#define DEFINE_TEMPLATE_CLASS_ABSTRACT_HIDDEN(Class, Type, Parent) \
template<typename Type> \
DEFINE_CLASS_ABSTRACT_HIDDEN (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type>), Parent);

/** Define RTTI for abstract class with two template arguments without adding to class registry */
#define DEFINE_TEMPLATE_CLASS_ABSTRACT_HIDDEN2(Class, Type1, Type2, Parent) \
template<typename Type1, typename Type2> \
DEFINE_CLASS_ABSTRACT_HIDDEN (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type1, Type2>), Parent);

/** Define RTTI for abstract class with three template arguments without adding to class registry */
#define DEFINE_TEMPLATE_CLASS_ABSTRACT_HIDDEN3(Class, Type1, Type2, Type3, Parent) \
template<typename Type1, typename Type2, typename Type3> \
DEFINE_CLASS_ABSTRACT_HIDDEN (TEMPLATE_CLASS_TYPE_ARGUMENT (Class<Type1, Type2, Type3>), Parent);

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif // _ccl_objectmacros_h
