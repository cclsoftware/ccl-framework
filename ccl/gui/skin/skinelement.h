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
// Filename    : ccl/gui/skin/skinelement.h
// Description : Skin Element class
//
//************************************************************************************************

#ifndef _ccl_skinelement_h
#define _ccl_skinelement_h

#include "ccl/base/typelib.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/gui/framework/iskinmodel.h"
#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

class Url;
class Theme;
class SkinAttributes;

interface IFileSystem;
interface IPackageFile;
interface ITranslationTable;

namespace SkinElements {
class Element; }

//************************************************************************************************
// Skin Warnings
//************************************************************************************************

extern void SkinWarning (const SkinElements::Element* element, CStringRef warning);

#define SKIN_WARNING(e, s, ...) \
{ CCL::MutableCString __warning; __warning.appendFormat (s, __VA_ARGS__); CCL::SkinWarning (e, __warning); }

//************************************************************************************************
// Skin Element Macros
//************************************************************************************************

#define DECLARE_SKIN_ELEMENT(Class, Parent) \
DECLARE_CLASS (Class, Parent) \
static SkinElements::MetaElement __skinClass; \
const ITypeInfo* CCL_API getElementClass () const override { return &__skinClass; }

#define DECLARE_SKIN_ELEMENT_ABSTRACT(Class, Parent) \
DECLARE_CLASS_ABSTRACT (Class, Parent) \
static SkinElements::MetaElement __skinClass;

#define DECLARE_SKIN_ELEMENT_CLASS(Class, Parent) \
class Class: public Parent \
{ public: Class () {sorted = true;} DECLARE_SKIN_ELEMENT (Class, Parent) };

#define DEFINE_SKIN_ENUMERATION(TagName, AttrName, styleDef) \
static SkinElements::Enumeration UNIQUE_IDENT (__skinEnum) (TagName "." AttrName, nullptr, styleDef);

#define DEFINE_SKIN_ENUMERATION_PARENT(TagName, AttrName, styleDef, ParentTagName, ParentAttrName) \
static SkinElements::Enumeration UNIQUE_IDENT (__skinEnum) (TagName "." AttrName, ParentTagName "." ParentAttrName, styleDef);

#define ADD_SKIN_ELEMENT_MEMBER(name, typeName) \
Model::MemberDescription (name, ITypeInfo::kString, typeName),

#define BEGIN_SKIN_ELEMENT_ATTRIBUTES(Class) \
static Model::AttributeDescription __skinAttributes##Class[] = {

#define ADD_SKIN_ELEMENT_ATTRIBUTE(Name, Value) \
Model::AttributeDescription (Name, Value),

#define END_SKIN_ELEMENT_ATTRIBUTES(Class) Model::AttributeDescription () }; \
SkinElements::MetaElement::AttributeModifier UNIQUE_IDENT (__skinattr) (Class::__skinClass, __skinAttributes##Class);

// Elements can be members of one or more schemagroups and can have a childgroup.
// All members of an element's childgroup are allowed as children of the element.
// Schemagroups and childgroups are inherited from parent elements.
// Every skin element is implicitly part of a schema group only containing the element itself
// allowing the use of an element tag as childgroup.
#define ADD_SKIN_SCHEMAGROUP_ATTRIBUTE(val) ADD_SKIN_ELEMENT_ATTRIBUTE("Class:SchemaGroups", val)
#define ADD_SKIN_CHILDGROUP_ATTRIBUTE(val) ADD_SKIN_ELEMENT_ATTRIBUTE("Class:ChildGroup", val)

#define BEGIN_SKIN_ELEMENT_WITH_MEMBERS(Class, Parent, TagName, GroupName, RelatedClass) \
DEFINE_CLASS_HIDDEN (Class, Parent) \
SkinElements::MetaElement Class::__skinClass (TagName, Class::__create, &Parent::__skinClass, false, GroupName); \
static Model::MemberDescription __skinMembers##Class[] = {

#define BEGIN_SKIN_ELEMENT_ABSTRACT_WITH_MEMBERS(Class, Parent, TagName, GroupName, RelatedClass) \
DEFINE_CLASS_HIDDEN (Class, Parent) \
SkinElements::MetaElement Class::__skinClass (TagName, Class::__create, &Parent::__skinClass, true, GroupName); \
static Model::MemberDescription __skinMembers##Class[] = {

#define BEGIN_SKIN_ELEMENT_BASE_WITH_MEMBERS(Class, Parent, TagName, GroupName) \
DEFINE_CLASS_HIDDEN (Class, Parent) \
SkinElements::MetaElement Class::__skinClass (TagName, Class::__create, 0, true, GroupName); \
static Model::MemberDescription __skinMembers##Class[] = {

#define END_SKIN_ELEMENT_WITH_MEMBERS(Class) Model::MemberDescription () }; \
static SkinElements::MetaElement::MemberDescriptionModifier UNIQUE_IDENT (__skinmod) (Class::__skinClass, __skinMembers##Class);

#define DEFINE_SKIN_ELEMENT(Class, Parent, TagName, GroupName, RelatedClass) \
BEGIN_SKIN_ELEMENT_WITH_MEMBERS(Class, Parent, TagName, GroupName, RelatedClass) \
END_SKIN_ELEMENT_WITH_MEMBERS(Class)

#define DEFINE_SKIN_ELEMENT_ABSTRACT(Class, Parent, TagName, GroupName, RelatedClass) \
BEGIN_SKIN_ELEMENT_ABSTRACT_WITH_MEMBERS(Class, Parent, TagName, GroupName, RelatedClass) \
END_SKIN_ELEMENT_WITH_MEMBERS(Class)

#define TYPE_BOOL CCL::TypeNames::kBool
#define TYPE_STRING CCL::TypeNames::kString
#define TYPE_FLOAT CCL::TypeNames::kFloat
#define TYPE_INT CCL::TypeNames::kInt
#define TYPE_ENUM CCL::TypeNames::kEnum
#define TYPE_METRIC ("metric")
#define TYPE_COLOR ("color")
#define TYPE_RECT ("rect")
#define TYPE_SIZE ("size")
#define TYPE_POINT ("point")
#define TYPE_POINT3D ("point3d")

//************************************************************************************************
// ISkinContext
//************************************************************************************************

class ISkinContext
{
public:
	virtual StringID getSkinID () const = 0;
	virtual IFileSystem* getFileSystem () const = 0;
	virtual ITranslationTable* getStringTable () const = 0;
	virtual Theme* getTheme () const = 0;
	virtual IPackageFile* getPackage () const = 0;

	DECLARE_STRINGID_MEMBER (kImportID)
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// SkinElements namespace starts here
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace SkinElements {
	
class Element;

//************************************************************************************************
// MetaElement
/** Represents a tag class in Skin XML. */
//************************************************************************************************

class MetaElement: public TypeInfoWithMembers
{
public:
	MetaElement (CStringPtr name, Object* (*creator)(), const MetaElement* parentClass,
				 bool isAbstract = false, CStringPtr groupName = nullptr);

	constexpr static bool kTagsCaseSensitive = false; ///< case sensitivity of skin tags

	static TypeLibrary& getTypeLibrary ();
	static Element* createElement (CStringRef name);

	// TypeInfoWithMembers
	int CCL_API getClassFlags () const override;
	bool getDetails (ITypeInfoDetails& details) const override;

protected:
	Object* (*creator)();
	bool isAbstract;
	CStringPtr groupName;

	Element* createElement () const;
};

//************************************************************************************************
// Enumeration
/** Represents an enumeration in Skin XML. */
//************************************************************************************************

class Enumeration: public EnumTypeInfo
{
public:
	Enumeration (CStringPtr name, CStringPtr parentName, const StyleDef* def);

	static const StyleDef* getStyleDef (CStringRef name);

	// EnumTypeInfo
	int CCL_API getEnumeratorCount () const override;
	tbool CCL_API getEnumerator (MutableCString& name, Variant& value, int index) const override;

protected:
	friend class SkinElementLibrary;
	const StyleDef* def;
	mutable int count;
};

//************************************************************************************************
// Element
/** The base class of all skin elements.
This class is not used directly, but all other skin element classes inherit from it. */
//************************************************************************************************

class Element: public ObjectArray,
			   public ISkinElement,
			   public ISkinElementChildren
{
public:
	DECLARE_SKIN_ELEMENT (Element, ObjectArray)
	
	Element (CStringRef name = nullptr);

	static bool isSkinWarningsEnabled ();

	PROPERTY_MUTABLE_CSTRING (fileName, FileName)
	PROPERTY_VARIABLE (int, lineNumber, LineNumber)

	StringID CCL_API getName () const override { return name; } // ISkinElement
	void CCL_API setName (StringID _name) override { name = _name; } // ISkinElement
	void setName (StringRef name);

	virtual void addChild (Element* e, int index = -1);
	virtual void removeChild (Element* e);

	/** Called when a sibling with the same name is about to be added;
		Returns true if a custom action was performed, otherwise the existing element is removed (default). */
	virtual bool mergeElements (Element& other); 

	void takeElements (Element& source); ///< move all children from source to this

	Element* getParent () const;
	void setParent (Element* parent);
	Element* getParent (MetaClassRef typeId) const;

	Element* findElement (CStringRef name) const;
	Element* findElement (CStringRef name, MetaClassRef typeId) const;
	Element* findElement (MetaClassRef typeId) const;
	Element* getElement (int index) const;

	template <class T> T* findElement () const					{ return (T*)findElement (ccl_typeid<T> ()); }
	template <class T> T* findElement (CStringRef name) const	{ return (T*)findElement (name, ccl_typeid<T> ()); }

	virtual bool setAttributes (const SkinAttributes& a);
	virtual bool getAttributes (SkinAttributes& a) const;
	virtual void loadFinished (); ///< when element tag closes (after all childs are loaded)
	virtual bool isOverrideEnabled () const; ///< silence warnings for intentional overrides 
	
	// Context information
	virtual ISkinContext* getSkinContext () const;
	IFileSystem* getFileSystem () const;
	Url& makeSkinUrl (Url& url, StringRef path, bool isFolder = false) const;
	virtual const Element* getTranslationScope () const;
	String translate (StringRef text) const;
	String translateWithScope (StringID scopeName, StringRef text) const;
	Theme* getTheme () const;
	tbool CCL_API getSourceInfo (String& fileName, int32& lineNumber, IUrl* packageUrl = nullptr) const override; // ISkinElement
	void CCL_API setSourceFile (StringRef fileName) override; // ISkinElement

	CLASS_INTERFACE2 (ISkinElement, ISkinElementChildren, ObjectArray)

	class SortingSuspender;

protected:
	MutableCString name;
	String comment;
	Element* parent;
	bool sorted;

	// ObjectArray
	int compare (const Object& obj) const override;

	// ISkinElement
	void CCL_API getComment (String& _comment) const override { _comment = comment; }
	void CCL_API setComment (StringRef _comment) override { comment = _comment; }
	void CCL_API getAttributes (IAttributeList& attributes) const override;
	void CCL_API setAttributes (const IAttributeList& attributes) override;
	tbool CCL_API getAttributeValue (Variant& value, StringID name) const override;
	void CCL_API setAttributeValue (StringID name, VariantRef value, int index = -1) override;
	tbool CCL_API removeAttribute (StringID name, int* oldIndex = nullptr) override;
	void CCL_API clone (ISkinElement*& element) const override;

	// ISkinElementChildren
	tbool CCL_API addChildElement (ISkinElement* childElement, int index = -1) override;
	tbool CCL_API removeChildElement (ISkinElement* childElement, int* oldIndex = nullptr) override;
};

//************************************************************************************************
// Element::SortingSuspender
//************************************************************************************************

class Element::SortingSuspender
{
public:
	SortingSuspender (Element& element)
	: element (element),
	  wasSorted (element.sorted),
	  oldCount (element.count ())
	{
		element.sorted = false;
	}
	~SortingSuspender ()
	{
		if(wasSorted && element.count () != oldCount)
			element.sort ();
		element.sorted = wasSorted;
	}
private:
	Element& element;
	bool wasSorted;
	int oldCount;
};

} // namespace SkinElements
} // namespace CCL

#endif // _ccl_skinelement_h
