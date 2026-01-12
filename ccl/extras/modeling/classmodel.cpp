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
// Filename    : ccl/extras/modeling/classmodel.cpp
// Description : Class Model
//
//************************************************************************************************

#include "ccl/extras/modeling/classmodel.h"

#include "ccl/extras/modeling/classrepository.h"

#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/url.h"

namespace CCL {
namespace Model {

//************************************************************************************************
// CompositeTypeParser
//************************************************************************************************

class CompositeTypeParser
{
public:
	CompositeTypeParser (CStringRef string)
	: tokenizer (string, "|")
	{}

	bool nextType (TypeDescription& result)
	{
		MutableCString typeName = tokenizer.next ();
		if(!typeName.isEmpty ())
		{
			typeName.trimWhitespace ();
			ASSERT (!typeName.isEmpty ())

			// check for container: "Type[]"
			static CStringRef kContainerSuffix = ("[]");
			static const int kContainerSuffixLength = kContainerSuffix.length ();

			bool isContainer = typeName.endsWith (kContainerSuffix);
			if(isContainer)
			{
				typeName.truncate (typeName.length () - kContainerSuffixLength);
				typeName.trimWhitespace ();
				ASSERT (!typeName.isEmpty ())
			}
			ASSERT (!typeName.contains ('['))
			ASSERT (!typeName.contains (']'))

			#if 0 // alternative syntax: "container<Type>"
			static const CStringRef kContainerPrefix = ("container<");
			static const int kContainerPrefixLength = kContainerPrefix.length ();
			if(!isContainer)
				isContainer = typeName.startsWith (kContainerPrefix);
			if(isContainer)
			{
				int typeLength = typeName.length () - kContainerPrefixLength;
				ASSERT (typeName.endsWith (">"))
				if(typeName.endsWith (">"))
					typeLength--;

				typeName = typeName.subString (kContainerPrefixLength, typeLength);
				typeName.trimWhitespace ();
				ASSERT (!typeName.isEmpty ())
			}
			#endif

			DataType type = TypeNames::fromString (typeName);
			if(isContainer)
				type = ITypeInfo::kContainer;
			else if(type == ITypeInfo::kVoid)
				type = ITypeInfo::kObject;

			result.setType (type);
			result.setTypeName (typeName);
			return true;
		}
		return false;
	}

private:
	Core::CStringTokenizer tokenizer;
};

} // namespace Model
} // namespace CCL

using namespace CCL;
using namespace Model;

//************************************************************************************************
// TypeNames
//************************************************************************************************

CString TypeNames::toString (DataType type)
{
	switch(type)
	{
	case ITypeInfo::kObject:	return kObject;
	case ITypeInfo::kContainer:	return kContainer;
	case ITypeInfo::kVariant:	return kVariant;
	case ITypeInfo::kInt:		return kInt;
	case ITypeInfo::kFloat:		return kFloat;
	case ITypeInfo::kString:	return kString;
	case ITypeInfo::kBool:		return kBool;
	}
	return kVoid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DataType TypeNames::fromString (StringID string)
{
	if(!string.isEmpty ())
	{
		if(string == kObject)	return ITypeInfo::kObject;
		if(string == kContainer)return ITypeInfo::kContainer;
		if(string == kVariant)	return ITypeInfo::kVariant;
		if(string == kInt)		return ITypeInfo::kInt;
		if(string == kFloat)	return ITypeInfo::kFloat;
		if(string == kString)	return ITypeInfo::kString;
		if(string == kBool)		return ITypeInfo::kBool;
	}
	return ITypeInfo::kVoid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DataType TypeNames::getDataType (VariantRef variant)
{
	switch(variant.getType ())
	{
	case Variant::kInt : return ITypeInfo::kInt;
	case Variant::kFloat : return ITypeInfo::kFloat;
	case Variant::kString : return ITypeInfo::kString;
	case Variant::kObject : return ITypeInfo::kObject;
	}
	return ITypeInfo::kVariant;
}

//************************************************************************************************
// Model::TypeDescription
//************************************************************************************************

void TypeDescription::assign (const ITypeInfo::PropertyDefinition& propDef)
{
	setType (propDef.type);

	// 1.) try typeName string first
	setTypeName (propDef.typeName);

	// 2.) try class name from typeInfo
	if(getTypeName ().isEmpty () && propDef.typeInfo)
		setTypeName (propDef.typeInfo->getClassName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TypeDescription::fromString (CStringRef string)
{
	int numTypes = 0;
	MutableCString resultString;

	TypeDescription type;
	CompositeTypeParser typeParser (string);
	while(typeParser.nextType (type))
	{
		if(numTypes++ > 0)
			resultString += " | ";

		resultString += type.getTypeName ();
	}

	setType (numTypes > 1 ? ITypeInfo::kComposite : type.getType ());
	setTypeName (resultString);	
	return numTypes;
}

//************************************************************************************************
// Model::Documentation
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Documentation, Object, "Model.Documentation")

//////////////////////////////////////////////////////////////////////////////////////////////////

Documentation::Documentation ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Documentation::addLink (StringRef elementName)
{
	links.add (elementName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Documentation::removeLink (int index)
{
	return links.removeAt (index);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Documentation::setLink (int index, StringRef elementName)
{
	if(index >= 0 && index < links.count ())
	{
		links.at (index) = elementName;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Documentation::setLinks (const LinkList& newLinks)
{
	if(&links != &newLinks)
		links.copyVector (newLinks);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Documentation::isEmpty () const
{
	return	briefDescription.isEmpty () && 
			detailedDescription.isEmpty () && 
			codeExample.isEmpty() &&
			remarks.isEmpty () &&
			links.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Documentation& Documentation::assign (const Documentation& other)
{
	briefDescription = other.briefDescription;
	detailedDescription = other.detailedDescription;
	codeExample = other.codeExample;
	remarks = other.remarks;
	links.copyVector (other.links);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline String getBoxedString (Attributes& a, StringID id)
{
	String result;
	Boxed::String* string = a.getObject<Boxed::String> (id);
	if(string)
		result = *string;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Documentation::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	briefDescription = getBoxedString (a, "brief");
	detailedDescription = getBoxedString (a, "detailed");
	codeExample = getBoxedString (a, "code");
	remarks = getBoxedString (a, "remarks");
	
	if(AttributeQueue* linkQueue = a.getObject<AttributeQueue> ("links"))
		while(AutoPtr<Attribute> attr = linkQueue->unqueueNext ())
		{
			if(Boxed::String* string = unknown_cast<Boxed::String> (attr->getValue ()))
				links.add (*string);
			else if(UrlWithTitle* url = unknown_cast<UrlWithTitle> (attr->getValue ()))
			{
				String link;
				url->getName (link);
				links.add (link);
			}
			else
				break;
		}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Documentation::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	if(Element::getSaveMode () == Element::kPrepareDoc || !briefDescription.isEmpty ())
		a.set ("brief", Boxed::String (briefDescription), true);
	if(!detailedDescription.isEmpty ())
		a.set ("detailed", Boxed::String (detailedDescription), true);
	if(!codeExample.isEmpty ())
		a.set ("code", Boxed::String (codeExample), true);
	if(!codeLanguage.isEmpty())
		a.set ("language", Boxed::String (codeLanguage), true);
	if(!remarks.isEmpty ())
		a.set ("remarks", Boxed::String (remarks), true);
	
	for(int i = 0; i < links.count (); i++)
		a.queueAttribute ("links", (NEW Boxed::String (links[i]))->asUnknown (), Attributes::kOwns);
	return true;
}

//************************************************************************************************
// Model::Element
//************************************************************************************************

Element::SaveMode Element::theSaveMode = Element::kNormal;
void Element::setSaveMode (SaveMode mode)
{ theSaveMode = mode; }
Element::SaveMode Element::getSaveMode ()
{ return theSaveMode; }

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString Element::toCanonicalName (CStringRef className)
{
	if(!className.contains ("::"))
		return className;

	MutableCString canonicalName;
	Core::CStringTokenizer tokenizer (className, "::");
	while(CStringPtr token = tokenizer.next ())
	{
		if(!canonicalName.isEmpty ())
			canonicalName.append (".");
		canonicalName.append (token);
	}
	return canonicalName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_PERSISTENT (Element, Object, "Model.Element")

//////////////////////////////////////////////////////////////////////////////////////////////////

Element::Element (StringID name)
: name (name),
  namespaceName (nullptr),
  enclosure (nullptr),
  editState (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Element::Element (const Element& e)
: name (e.name),
  namespaceName (e.namespaceName),
  enclosure (nullptr),
  editState (0),
  documentation (e.documentation)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Element::hasDocumentation () const
{
	return !documentation.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Element::takeDocumentation (const Element& other)
{
	documentation.assign (other.documentation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Element::getEnclosedTitle () const
{
	String title (name);
	if(enclosure)
	{
		title.prepend (".");
		title.prepend (String (enclosure->getName ()));
	}
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassRepository* Element::findRepository () const
{
	return enclosure ? enclosure->findRepository () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Element::equals (const Object& obj) const
{
	const Element* other = ccl_cast<Element> (&obj);
	return other ? other->name == name : SuperClass::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Element::compare (const Object& obj) const
{
	const Element* other = ccl_cast<Element> (&obj);
	return other ? name.compare (other->name) : SuperClass::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Element::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	name = a.getString ("name");
	namespaceName = a.getString ("namespace");
	a.get (documentation, "doc");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Element::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	if(!name.isEmpty ())
		a.set ("name", name);
	if(!namespaceName.isEmpty ())
		a.set ("namespace", namespaceName);
	if(theSaveMode == kPrepareDoc || !documentation.isEmpty ())
		a.set ("doc", documentation, true);
	return true;
}

//************************************************************************************************
// Model::MainElement
//************************************************************************************************

DEFINE_CLASS_HIDDEN (MainElement, Element)

//////////////////////////////////////////////////////////////////////////////////////////////////

MainElement::MainElement (StringID name)
: Element (name),
  repository (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MainElement::MainElement (const MainElement& other)
: Element (other),
  repository (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassRepository* MainElement::findRepository () const
{
	return repository;
}

//************************************************************************************************
// Model::Class
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Class, MainElement, "Model.Class")

//////////////////////////////////////////////////////////////////////////////////////////////////

Class::Class (StringID name)
: MainElement (name),
  flags (0)
{
	members.objectCleanup (true);
	methods.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Class::Class (const Class& other)
: MainElement (other),
  flags (other.flags),
  parentName (other.parentName),
  parentNamespace (other.parentNamespace)
{
	members.objectCleanup (true);
	members.add (other.members, Container::kClone);
	ForEach (members, Member, m)
		m->setEnclosure (this);
	EndFor

	methods.objectCleanup (true);
	methods.add (other.methods, Container::kClone);
	ForEach (methods, Method, m)
		m->setEnclosure (this);
	EndFor

	attributes.copyFrom (other.attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Class::addMember (Member* member)
{
	member->setEnclosure (this);
	members.add (member);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Class::getMembers (Container& result, bool includeInherited) const
{
	TreeSet<CString> parents;
	getMembersInternal (result, includeInherited, parents);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Class::getMembersInternal (Container& result, bool includeInherited, TreeSet<CString>& visitedParents) const
{

	ForEach (members, Member, member)
		if(!result.contains (*member))
			result.add (member);
	EndFor

	if(includeInherited && repository && !parentName.isEmpty ())
	{
		if(visitedParents.contains (name))
			return; // cyclic parent relationship

		visitedParents.add (name);

		if(const Class* parent = repository->findClass (parentName))
			parent->getMembersInternal (result, includeInherited, visitedParents);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Member* Class::findMember (StringID name) const
{
	return (Member*)members.findEqual (Member (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Class::addMethod (Method* method)
{
	method->setEnclosure (this);
	methods.add (method);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& Class::getMethods () const
{
	return methods;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Method* Class::findMethod (StringID name) const
{
	return (Method*)methods.findEqual (Method (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Class::assign (ITypeLibrary& typeLib, const ITypeInfo& typeInfo)
{
	ASSERT (getName () == Element::toCanonicalName (typeInfo.getClassName ()))

	CStringPtr classNamespaceName = typeInfo.getClassNamespace ();
	if(classNamespaceName != nullptr)
		setNamespaceName (classNamespaceName);

	setFlags (typeInfo.getClassFlags ());

	if(const ITypeInfo::MethodDefinition* methodNames = typeInfo.getMethodNames ())
		for(int i = 0; methodNames[i].name != nullptr; i++)
		{
			Method* method = NEW Method (methodNames[i].name);
			method->assign (methodNames[i]);
			addMethod (method);
		}

	typeLib.getTypeDetails (*this, typeInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Class::addMember (const MemberDescription& member)
{
	addMember (NEW Member (member.name, member.type, member.typeName));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Class::setAttribute (StringID id, VariantRef value)
{
	attributes.setAttribute (id, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Class::hasDocumentation () const
{
	if(SuperClass::hasDocumentation ())
		return true;

	ForEach (members, Member, member)
		if(member->hasDocumentation ())
			return true;
	EndFor

	ForEach (methods, Method, method)
		if(method->hasDocumentation ())
			return true;
	EndFor

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Class::takeDocumentation (const Element& other)
{
	SuperClass::takeDocumentation (other);

	if(const Class* docClass = ccl_cast<Class> (&other))
	{
		ForEach (members, Member, m)
			if(Member* docMember = docClass->findMember (m->getName ()))
				m->takeDocumentation (*docMember);
			else
				m->isNew (true);
		EndFor

		ForEach (methods, Method, m)
			if(Method* docMethod = docClass->findMethod (m->getName ()))
				m->takeDocumentation (*docMethod);
			else
				m->isNew (true);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Class::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	isAbstract (a.getBool ("abstract"));
	isScriptable (a.getBool ("scriptable"));
	isMutable (a.getBool ("mutable"));
	parentName = a.getString ("parent");
	parentNamespace = a.getString ("parentNamespace");

	a.unqueue (members, "members", ccl_typeid<Member> ());
	ForEach (members, Member, member)
		member->setEnclosure (this);
	EndFor

	a.unqueue (methods, "methods", ccl_typeid<Method> ());
	ForEach (methods, Method, method)
		method->setEnclosure (this);
	EndFor

	a.get (attributes, "attributes");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Class::save (const Storage& storage) const
{
	if(!SuperClass::save (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	if(isAbstract ())
		a.set ("abstract", isAbstract ());
	if(isScriptable ())
		a.set ("scriptable", isScriptable ());
	if(isMutable ())
		a.set ("mutable", isMutable ());
	if(!parentName.isEmpty ())
		a.set ("parent", parentName);
	if(!parentNamespace.isEmpty ())
		a.set ("parentNamespace", parentNamespace);
	a.queue ("members", members, Attributes::kShare);
	a.queue ("methods", methods, Attributes::kShare);

	if(!attributes.isEmpty ())
		a.set ("attributes", attributes);
	return true;
}

//************************************************************************************************
// Model::Variable
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Variable, Element, "Model.Variable")

//////////////////////////////////////////////////////////////////////////////////////////////////

Variable::Variable (StringID name, DataType type, StringID typeName)
: Element (name),
  type (type),
  typeName (typeName),
  readOnly (type & ITypeInfo::kReadOnly)
{
	if(readOnly)
		this->type &= ~ITypeInfo::kReadOnly;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Variable::getTypeDescription () const
{
	String result;
	if(!typeName.isEmpty ())
		result = String (typeName);
	else
		result = String (TypeNames::toString (type));
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Variable::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	type = TypeNames::fromString (a.getCString ("type"));
	typeName = a.getCString ("typeName");
	a.getBool (readOnly, "readOnly");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Variable::save (const Storage& storage) const
{
	if(!SuperClass::save (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	a.set ("type", TypeNames::toString (type));
	if(!typeName.isEmpty ())
		a.set ("typeName", typeName);
	if(isReadOnly ())
		a.set ("readOnly", true);
	return true;
}

//************************************************************************************************
// Model::Member
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Member, Variable, "Model.Member")

//////////////////////////////////////////////////////////////////////////////////////////////////

Member::Member (StringID name, DataType type, StringID typeName)
: Variable (name, type, typeName)
{}

//************************************************************************************************
// Model::ReturnValue
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (ReturnValue, Variable, "Model.ReturnValue")

//////////////////////////////////////////////////////////////////////////////////////////////////

ReturnValue::ReturnValue (StringID name, DataType type, StringID typeName)
: Variable (name, type, typeName)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ReturnValue::getEnclosedTitle () const
{
	String title;
	title << "@";
	if(enclosure)
		title << enclosure->getName ();
	return title;
}

//************************************************************************************************
// Model::MethodArgument
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (MethodArgument, Variable, "Model.MethodArg")

//////////////////////////////////////////////////////////////////////////////////////////////////

MethodArgument::MethodArgument (StringID name, DataType type, StringID typeName)
: Variable (name, type, typeName)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

String MethodArgument::getEnclosedTitle () const
{
	String title;
	title << "(" << getName () << ")";
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MethodArgument::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	defaultValue = a.getCString ("defaultValue");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MethodArgument::save (const Storage& storage) const
{
	if(!SuperClass::save (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	if(!defaultValue.isEmpty ())
		a.set ("defaultValue", defaultValue);
	return true;
}

//************************************************************************************************
// Model::Method
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Method, Element, "Model.Method")

//////////////////////////////////////////////////////////////////////////////////////////////////

Method::Method (StringID name)
: Element (name)
{
	returnValue.setEnclosure (this);
	arguments.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Method::Method (const Method& other)
: Element (other),
  returnValue (other.returnValue)
{
	returnValue.setEnclosure (this);

	arguments.objectCleanup (true);
	arguments.add (other.arguments, Container::kClone);
	ForEach (arguments, Variable, arg)
		arg->setEnclosure (this);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Method::addArgument (MethodArgument* arg)
{
	arg->setEnclosure (this);
	arguments.add (arg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& Method::getArguments () const
{
	return arguments;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Method::hasDocumentation () const
{
	if(SuperClass::hasDocumentation ())
		return true;

	if(returnValue.hasDocumentation ())
		return true;
	
	ForEach (arguments, Variable, arg)
		if(arg->hasDocumentation ())
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Method::takeDocumentation (const Element& other)
{
	SuperClass::takeDocumentation (other);
	
	if(const Method* docMethod = ccl_cast<Method> (&other))
	{
		returnValue.takeDocumentation (docMethod->getReturnValue ());
	
		/* TODO: how to merge argument documentation - index or name???
		ForEach (arguments, Variable, arg)
			//...
		EndFor
		*/
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Method::assign (const ITypeInfo::MethodDefinition& method)
{
	ASSERT (getName () == method.name)

	if(method.args)
	{
		ForEachStringToken (String (method.args), String (","), token)
			MutableCString argName (token);
			TypeDescription argType;
			MutableCString defaultValue;

			// "argName: type"
			// "argName: type = defaultValue"
			// "argName: type1 | type2 = defaultValue"

			// parse trailing "= defaultValue"
			int equalsIndex = argName.index ("=");
			if(equalsIndex >= 0)
			{
				defaultValue = argName.subString (equalsIndex + 1);
				defaultValue.trimWhitespace ();
				ASSERT (!defaultValue.isEmpty ())

				argName.truncate (equalsIndex);
			}

			// parse trailing ": type"
			int colonIndex = argName.index (":");
			if(colonIndex >= 0)
			{
				MutableCString typeString (argName.subString (colonIndex + 1));
				argType.fromString (typeString);

				argName.truncate (colonIndex);
			}

			argName.trimWhitespace ();

			MethodArgument* arg = NEW MethodArgument (argName, argType.getType (), argType.getTypeName ());
			arg->setDefaultValue (defaultValue);
			addArgument (arg);
		EndFor
	}

	if(method.retval)
	{
		DataType detectedType = TypeNames::fromString (method.retval);
		returnValue.setType (detectedType);
		if(detectedType == ITypeInfo::kVoid)
		{
			returnValue.setTypeName (method.retval);
			returnValue.setType (ITypeInfo::kObject);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Method::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	a.get (returnValue, "retval");

	a.unqueue (arguments, "args", ccl_typeid<MethodArgument> ());
	ForEach (arguments, Variable, arg)
		arg->setEnclosure (this);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Method::save (const Storage& storage) const
{
	if(!SuperClass::save (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	a.set ("retval", returnValue, true);
	a.queue ("args", arguments, Attributes::kShare);
	return true;
}

//************************************************************************************************
// Model::Enumeration
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Enumeration, MainElement, "Model.Enumeration")

//////////////////////////////////////////////////////////////////////////////////////////////////

Enumeration::Enumeration (StringID name, StringID parentName)
: MainElement (name),
  parentName (parentName)
{
	enumerators.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Enumeration::Enumeration (const Enumeration& other)
: MainElement (other)
{
	enumerators.objectCleanup (true);
	enumerators.add (other.enumerators, Container::kClone);
	ForEach (enumerators, Enumerator, e)
		e->setEnclosure (this);
	EndFor

	parentName = other.parentName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Enumeration::addEnumerator (Enumerator* e)
{
	e->setEnclosure (this);
	enumerators.add (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Enumeration::getEnumerators (Container& result, bool includeInherited) const
{
	ForEach (enumerators, Enumerator, enumerator)
		if(!result.contains (*enumerator))
			result.add (enumerator);
	EndFor

	if(includeInherited && repository)
	{
		if(!parentName.isEmpty ())
		{
			if(const Enumeration* parent = repository->findEnum (parentName))
				parent->getEnumerators (result, includeInherited);
		}

		int splitPoint = getName ().index (".");
		MutableCString className (getName ().subString (0, splitPoint));
		MutableCString enumName (getName ().subString (splitPoint));
		auto findParentClassName = [this] (CStringRef className) -> MutableCString
		{
			if(!className.isEmpty ())
			{
				if(const Class* currentClass = repository->findClass (className))
					return currentClass->getParentName ();
			}

			return "";
		};

		MutableCString parentClassName = findParentClassName (className);
		while(!parentClassName.isEmpty ())
		{
			MutableCString parentEnumName = parentClassName;
			parentEnumName.append (enumName);
			if(const Enumeration* parent = repository->findEnum (parentEnumName))
			{
				parent->getEnumerators (result, includeInherited);
				break;
			}

			parentClassName = findParentClassName (parentClassName);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Enumerator* Enumeration::findEnumerator (StringID name) const
{
	return (Enumerator*)enumerators.findEqual (Enumerator (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Enumeration::assign (ITypeLibrary& typeLib, const IEnumTypeInfo& enumInfo)
{
	ASSERT (getName () == enumInfo.getName ())

	int count = enumInfo.getEnumeratorCount ();
	for(int i = 0; i < count; i++)
	{
		Variant value;
		MutableCString name;
		enumInfo.getEnumerator (name, value, i);

		Enumerator* e = NEW Enumerator (name);
		e->setValue (value);
		addEnumerator (e);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Enumeration::asString () const
{
	String string;
	ForEach (enumerators, Enumerator, e)
		if(!string.isEmpty ())
			string << ", ";
		string << e->getName ();
	EndFor
	return string;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Enumeration::hasDocumentation () const
{
	if(SuperClass::hasDocumentation ())
		return true;

	ForEach (enumerators, Enumerator, e)
		if(e->hasDocumentation ())
			return true;
	EndFor

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Enumeration::takeDocumentation (const Element& other)
{
	SuperClass::takeDocumentation (other);

	if(const Enumeration* docEnum = ccl_cast<Enumeration> (&other))
	{
		ForEach (enumerators, Enumerator, e)
			if(Enumerator* docE = docEnum->findEnumerator (e->getName ()))
				e->takeDocumentation (*docE);
			else
				e->isNew (true);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Enumeration::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	parentName = a.getString ("parent");
	a.unqueue (enumerators, "enumerators", ccl_typeid<Enumerator> ());

	ForEach (enumerators, Enumerator, e)
		e->setEnclosure (this);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Enumeration::save (const Storage& storage) const
{
	if(!SuperClass::save (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	if(!parentName.isEmpty ())
		a.setAttribute ("parent", String (parentName));

	a.queue ("enumerators", enumerators, Attributes::kShare);
	return true;
}

//************************************************************************************************
// Model::Enumerator
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Enumerator, Element, "Model.Enumerator")

//////////////////////////////////////////////////////////////////////////////////////////////////

Enumerator::Enumerator (StringID name)
: Element (name)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

String Enumerator::getEnclosedTitle () const
{
	return String (getName ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Enumerator::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	a.getAttribute (value, "value");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Enumerator::save (const Storage& storage) const
{
	if(!SuperClass::save (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	a.setAttribute ("value", value, Attributes::kShare);
	return true;
}

//************************************************************************************************
// Model::Property
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Property, Variable, "Model.Property")

//////////////////////////////////////////////////////////////////////////////////////////////////

Property::Property (StringID name, DataType type, StringID typeName)
: Variable (name, type, typeName)
{}

//************************************************************************************************
// Model::ObjectElement
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (ObjectElement, MainElement, "Model.Object")

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectElement::ObjectElement (StringID name)
: MainElement (name),
  readOnly (false)
{
	children.objectCleanup (true);
	properties.objectCleanup (true);
	methods.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectElement::ObjectElement (const ObjectElement& other)
: MainElement (other),
  readOnly (other.readOnly)
{
	children.objectCleanup (true);
	children.add (other.children, Container::kClone);
	ForEach (children, ObjectElement, child)
		child->setEnclosure (this);
	EndFor

	properties.objectCleanup (true);
	properties.add (other.properties, Container::kClone);
	ForEach (properties, Property, p)
		p->setEnclosure (this);
	EndFor

	methods.objectCleanup (true);
	methods.add (other.methods, Container::kClone);
	ForEach (methods, Method, method)
		method->setEnclosure (this);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectElement* ObjectElement::getParent () const
{
	return ccl_cast<ObjectElement> (enclosure);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ObjectElement::getParentPathName () const
{
	String pathName;
	const ObjectElement* p = getParent ();
	while(p)
	{
		if(!pathName.isEmpty ())
			pathName.prepend (".");
		pathName.prepend (p->getTitle ());
		p = p->getParent ();
	}
	return pathName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String ObjectElement::getEnclosedTitle () const
{
	String title (getParentPathName ());
	if(!title.isEmpty ())
		title << ".";
	title << getTitle ();
	return title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectElement::addProperty (Property* p)
{
	p->setEnclosure (this);
	properties.add (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& ObjectElement::getProperties () const
{
	return properties;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Property* ObjectElement::findProperty (StringID name) const
{
	return (Property*)properties.findEqual (Property (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectElement::addMethod (Method* method)
{
	method->setEnclosure (this);
	methods.add (method);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& ObjectElement::getMethods () const
{
	return methods;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Method* ObjectElement::findMethod (StringID name) const
{
	return (Method*)methods.findEqual (Method (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectElement::addChild (ObjectElement* child)
{
	child->setEnclosure (this);
	children.add (child);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Container& ObjectElement::getChildren () const
{
	return children;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectElement* ObjectElement::findChild (StringID name) const
{
	return (ObjectElement*)children.findEqual (ObjectElement (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectElement::hasDocumentation () const
{
	if(SuperClass::hasDocumentation ())
		return true;

	ForEach (properties, Property, p)
		if(p->hasDocumentation ())
			return true;
	EndFor

	ForEach (methods, Method, method)
		if(method->hasDocumentation ())
			return true;
	EndFor

	ForEach (children, ObjectElement, child)
		if(child->hasDocumentation ())
			return true;
	EndFor

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectElement::takeDocumentation (const Element& other)
{
	SuperClass::takeDocumentation (other);
	if(const ObjectElement* docObject = ccl_cast<ObjectElement> (&other))
	{
		ForEach (properties, Property, p)
			if(Property* docP = docObject->findProperty (p->getName ()))
				p->takeDocumentation (*docP);
			else
				p->isNew (true);
		EndFor

		ForEach (methods, Method, m)
			if(Method* docM = docObject->findMethod (m->getName ()))
				m->takeDocumentation (*docM);
			else
				m->isNew (true);
		EndFor

		ForEach (children, ObjectElement, o)
			if(ObjectElement* docChild = docObject->findChild (o->getName ()))
				o->takeDocumentation (*docChild);
			else
				o->isNew (true);
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectElement::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	a.getCString (className, "class");
	a.getCString (classNamespace, "classNamespace");
	a.getBool (readOnly, "readOnly");
	a.getBool (dynamicType, "dynamicType");

	a.unqueue (properties, "properties", ccl_typeid<Property> ());
	ForEach (properties, Property, p)
		p->setEnclosure (this);
	EndFor

	a.unqueue (methods, "methods", ccl_typeid<Method> ());
	ForEach (methods, Method, method)
		method->setEnclosure (this);
	EndFor

	a.unqueue (children, "children", ccl_typeid<ObjectElement> ());
	ForEach (children, ObjectElement, child)
		child->setEnclosure (this);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectElement::save (const Storage& storage) const
{
	if(!SuperClass::save (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	a.set ("class", getClassName ());
	if(!getClassNamespace().isEmpty ())
		a.set ("classNamespace", getClassNamespace ());
	if(isReadOnly ())
		a.set ("readOnly", true);
	if(isDynamicType ())
		a.set ("dynamicType", true);
	a.queue ("properties", properties, Attributes::kShare);
	a.queue ("methods", methods, Attributes::kShare);
	a.queue ("children", children, Attributes::kShare);
	return true;
}

//************************************************************************************************
// Model::PropertyCollection
//************************************************************************************************

PropertyCollection::PropertyCollection ()
{
	properties.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PropertyCollection::count () const
{
	return properties.count (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Property* PropertyCollection::getProperty (int index) const
{
	return static_cast<Property*> (properties.at (index));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString PropertyCollection::at (int index) const
{
	Property* prop = getProperty (index);
	return prop ? prop->getName () : CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PropertyCollection::addProperty (const ITypeInfo::PropertyDefinition& propDef)
{
	TypeDescription type;
	type.assign (propDef);

	auto prop = NEW Property (propDef.name, type.getType (), type.getTypeName ());
	ASSERT (!prop->getName ().isEmpty ())
	properties.add (prop);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PropertyCollection::addPropertyName (CStringPtr name)
{
	auto prop = NEW Property (name); // no type info
	ASSERT (!prop->getName ().isEmpty ())
	properties.add (prop);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PropertyCollection::addPropertyNames (CStringPtr names[], int count)
{
	if(names == nullptr)
		return;

	if(count == -1)
		for(int i = 0; names[i] != nullptr; i++)
			addPropertyName (names[i]);
	else
		for(int i = 0; i < count; i++)
			addPropertyName (names[i]);
}
