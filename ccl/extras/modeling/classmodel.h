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
// Filename    : ccl/extras/modeling/classmodel.h
// Description : Class Model
//
//************************************************************************************************

#ifndef _ccl_classmodel_h
#define _ccl_classmodel_h

#include "ccl/public/base/itypelib.h"
#include "ccl/public/base/variant.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/collections/objectarray.h"
#include "ccl/public/collections/treeset.h"

namespace CCL {

//************************************************************************************************
// TypeNames
//************************************************************************************************

namespace TypeNames
{
	// string conversion for data types defined in iobject.h
	CString toString (DataType type);
	DataType fromString (StringID string);

	DataType getDataType (VariantRef variant);
}

namespace Model {

class Member;
class Method;
class Enumerator;
class ClassRepository;

//************************************************************************************************
// Model::TypeDescription
//************************************************************************************************

class TypeDescription
{
public:
	PROPERTY_VARIABLE (DataType, type, Type)
	PROPERTY_MUTABLE_CSTRING (typeName, TypeName)

	TypeDescription ()
	: type (ITypeInfo::kVoid)
	{}

	void assign (const ITypeInfo::PropertyDefinition& propDef);

	int fromString (CStringRef string); // return number of composite types
};

//************************************************************************************************
// Model::Documentation
//************************************************************************************************

class Documentation: public Object
{
public:
	DECLARE_CLASS (Documentation, Object)

	Documentation ();

	PROPERTY_STRING (briefDescription, BriefDescription)
	PROPERTY_STRING (detailedDescription, DetailedDescription)
	PROPERTY_STRING (codeExample, CodeExample)
	PROPERTY_STRING (codeLanguage, CodeLanguage)
	PROPERTY_STRING (remarks, Remarks)

	bool isEmpty () const;
	Documentation& assign (const Documentation& other);
	
	typedef Vector<String> LinkList;

	void addLink (StringRef elementName);
	bool removeLink (int index);
	bool setLink (int index, StringRef elementName);
	const LinkList& getLinks () const;
	void setLinks (const LinkList& links);

	// Object
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	LinkList links;
};

//************************************************************************************************
// Model::Element
//************************************************************************************************

class Element: public Object
{
public:
	DECLARE_CLASS (Element, Object)

	Element (StringID name = nullptr);
	Element (const Element&);

	enum SaveMode { kNormal, kPrepareDoc };
	static void setSaveMode (SaveMode mode);
	static SaveMode getSaveMode ();
	static MutableCString toCanonicalName (CStringRef className);

	PROPERTY_POINTER (Element, enclosure, Enclosure)	///< enclosing element

	PROPERTY_MUTABLE_CSTRING (name, Name)
	PROPERTY_MUTABLE_CSTRING (namespaceName, NamespaceName)
	String getTitle () const { return String (name); }

	enum EditStates { kIsNew = 1<<0 };
	PROPERTY_VARIABLE (int, editState, EditState)	///< not saved!
	PROPERTY_FLAG (editState, kIsNew, isNew)

	Documentation& getDocumentation ();
	const Documentation& getDocumentation () const;

	virtual bool hasDocumentation () const;
	virtual void takeDocumentation (const Element& other);

	virtual String getEnclosedTitle () const;	///< e.g. "Enclosure.Element"
	virtual ClassRepository* findRepository () const;

	// Object
	bool equals (const Object& obj) const override;
	int compare (const Object& obj) const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	static SaveMode theSaveMode;

	Documentation documentation;
};

//************************************************************************************************
// Model::MainElement
/** Element on top level of class repository. */
//************************************************************************************************

class MainElement: public Element
{
public:
	DECLARE_CLASS (MainElement, Element)

	MainElement (StringID name = nullptr);
	MainElement (const MainElement& other);

	PROPERTY_POINTER (ClassRepository, repository, Repository)
	
	// Element
	ClassRepository* findRepository () const override;
};

//************************************************************************************************
// Model::Class
//************************************************************************************************

class Class: public MainElement,
			 public ITypeInfoDetails
{
public:
	DECLARE_CLASS (Class, MainElement)

	Class (StringID name = nullptr);
	Class (const Class&);

	PROPERTY_MUTABLE_CSTRING (parentName, ParentName)
	PROPERTY_MUTABLE_CSTRING (parentNamespace, ParentNamespace)

	PROPERTY_VARIABLE (int, flags, Flags)
	PROPERTY_FLAG (flags, ITypeInfo::kAbstract, isAbstract)
	PROPERTY_FLAG (flags, ITypeInfo::kScriptable, isScriptable)
	PROPERTY_FLAG (flags, ITypeInfo::kMutable, isMutable)

	const Attributes& getAttributes () const { return attributes; }
	String getGroupName () const { return attributes.getString (kClassDocGroup); }

	void addMember (Member* member);
	void getMembers (Container& result, bool includeInherited) const;
	Member* findMember (StringID name) const;

	void addMethod (Method* method);
	const Container& getMethods () const;
	Method* findMethod (StringID name) const;

	void assign (ITypeLibrary& typeLib, const ITypeInfo& typeInfo);

	// MainElement
	bool hasDocumentation () const override;
	void takeDocumentation (const Element& other) override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (ITypeInfoDetails, MainElement)

protected:
	ObjectArray members;
	ObjectArray methods;
	PersistentAttributes attributes;

	void getMembersInternal (Container& result, bool includeInherited, TreeSet<CString>& visitedParents) const;

	// ITypeInfoDetails
	void CCL_API addMember (const MemberDescription& member) override;
	void CCL_API setAttribute (StringID id, VariantRef value) override;
};

//************************************************************************************************
// Model::Variable
//************************************************************************************************

class Variable: public Element
{
public:
	DECLARE_CLASS (Variable, Element)

	Variable (StringID name = nullptr, DataType type = ITypeInfo::kVoid, StringID typeName = nullptr);

	PROPERTY_VARIABLE (DataType, type, Type)
	PROPERTY_MUTABLE_CSTRING (typeName, TypeName)
	PROPERTY_BOOL (readOnly, ReadOnly)

	String getTypeDescription () const;

	// Element
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// Model::Member
//************************************************************************************************

class Member: public Variable
{
public:
	DECLARE_CLASS (Member, Variable)

	Member (StringID name = nullptr, DataType type = ITypeInfo::kVoid, StringID typeName = nullptr);
};

//************************************************************************************************
// Model::ReturnValue
//************************************************************************************************

class ReturnValue: public Variable
{
public:
	DECLARE_CLASS (ReturnValue, Variable)

	ReturnValue (StringID name = nullptr, DataType type = ITypeInfo::kVoid, StringID typeName = nullptr);

	// Variable
	String getEnclosedTitle () const override;
};

//************************************************************************************************
// Model::MethodArgument
//************************************************************************************************

class MethodArgument: public Variable
{
public:
	DECLARE_CLASS (MethodArgument, Variable)

	MethodArgument (StringID name = nullptr, DataType type = ITypeInfo::kVoid, StringID typeName = nullptr);

	PROPERTY_MUTABLE_CSTRING (defaultValue, DefaultValue)

	// Variable
	String getEnclosedTitle () const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// Model::Method
//************************************************************************************************

class Method: public Element
{
public:
	DECLARE_CLASS (Method, Element)

	Method (StringID name = nullptr);
	Method (const Method&);

	ReturnValue& getReturnValue ();
	const ReturnValue& getReturnValue () const;

	void addArgument (MethodArgument* arg);
	const Container& getArguments () const;

	void assign (const ITypeInfo::MethodDefinition& method);

	// Element
	bool hasDocumentation () const override;
	void takeDocumentation (const Element& other) override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ObjectArray arguments;
	ReturnValue returnValue;
};

//************************************************************************************************
// Model::Enumeration
//************************************************************************************************

class Enumeration: public MainElement
{
public:
	DECLARE_CLASS (Enumeration, MainElement)

	Enumeration (StringID name = nullptr, StringID parentName = nullptr);
	Enumeration (const Enumeration&);

	PROPERTY_MUTABLE_CSTRING (parentName, ParentName)

	void addEnumerator (Enumerator* e);
	void getEnumerators (Container& result, bool includeInherited) const;
	Enumerator* findEnumerator (StringID name) const;

	void assign (ITypeLibrary& typeLib, const IEnumTypeInfo& enumInfo);

	String asString () const; ///< compose string with "enumerator1, enumerator2,..."

	// Element
	bool hasDocumentation () const override;
	void takeDocumentation (const Element& other) override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ObjectArray enumerators;
};

//************************************************************************************************
// Model::Enumerator
//************************************************************************************************

class Enumerator: public Element
{
public:
	DECLARE_CLASS (Enumerator, Element)

	Enumerator (StringID name = nullptr);

	PROPERTY_VARIABLE (Variant, value, Value)

	// Element
	String getEnclosedTitle () const override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
};

//************************************************************************************************
// Model::Property
//************************************************************************************************

class Property: public Variable
{
public:
	DECLARE_CLASS (Property, Variable)

	Property (StringID name = nullptr, DataType type = ITypeInfo::kVoid, StringID typeName = nullptr);
};

//************************************************************************************************
// Model::ObjectElement
//************************************************************************************************

class ObjectElement: public MainElement
{
public:
	DECLARE_CLASS (ObjectElement, MainElement)

	ObjectElement (StringID name = nullptr);
	ObjectElement (const ObjectElement&);

	PROPERTY_MUTABLE_CSTRING (className, ClassName)
	PROPERTY_MUTABLE_CSTRING (classNamespace, ClassNamespace)
	PROPERTY_BOOL (readOnly, ReadOnly)
	PROPERTY_BOOL (dynamicType, DynamicType)

	ObjectElement* getParent () const;
	String getParentPathName () const;
	
	void addProperty (Property* p);
	const Container& getProperties () const;
	Property* findProperty (StringID name) const;
	
	void addMethod (Method* method);
	const Container& getMethods () const;
	Method* findMethod (StringID name) const;

	void addChild (ObjectElement* child);
	const Container& getChildren () const;
	ObjectElement* findChild (StringID name) const;

	// Element
	String getEnclosedTitle () const override;
	bool hasDocumentation () const override;
	void takeDocumentation (const Element& other) override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

protected:
	ObjectArray children;
	ObjectArray properties;
	ObjectArray methods;
};

//************************************************************************************************
// Model::PropertyCollection
//************************************************************************************************

class PropertyCollection: public Object,
						  public IPropertyCollector
{
public:
	PropertyCollection ();

	int count () const;
	Property* getProperty (int index) const;
	MutableCString at (int index) const;

	// IPropertyCollector
	void CCL_API addProperty (const ITypeInfo::PropertyDefinition& propDef) override;
	void CCL_API addPropertyName (CStringPtr name) override;
	void CCL_API addPropertyNames (CStringPtr names[], int count) override;

	CLASS_INTERFACE (IPropertyCollector, Object)

protected:
	ObjectArray properties;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Documentation inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline const Documentation::LinkList& Documentation::getLinks () const
{return links; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Element inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Documentation& Element::getDocumentation ()
{ return documentation; }

inline const Documentation& Element::getDocumentation () const
{ return documentation; }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Method inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline ReturnValue& Method::getReturnValue () 
{ return returnValue; }

inline const ReturnValue& Method::getReturnValue () const
{ return returnValue; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Model
} // namespace CCL

#endif // _ccl_classmodel_h
