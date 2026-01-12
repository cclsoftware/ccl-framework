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
// Filename    : ccl/extras/modeling/classrepository.cpp
// Description : Class Repository
//
//************************************************************************************************

#include "ccl/extras/modeling/classrepository.h"

#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/xmlarchive.h"
#include "ccl/base/collections/stringlist.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/storage/filetype.h"
#include "ccl/public/base/iobjectnode.h"
#include "ccl/public/system/isearcher.h"
#include "ccl/public/cclversion.h"

using namespace CCL;
using namespace Model;

//************************************************************************************************
// ClassRepositorySearcher
//************************************************************************************************

class ClassRepositorySearcher: public Object,
							   public AbstractSearcher
{
public:
	ClassRepositorySearcher (Model::ClassRepository* repository, ISearchDescription& searchDescription);

	// ISearcher
	tresult CCL_API find (ISearchResultSink& resultSink, IProgressNotify* progress) override;

	CLASS_INTERFACE (ISearcher, Object)

protected:
	SharedPtr<Model::ClassRepository> repository;
};

//************************************************************************************************
// ClassRepositorySearcher
//************************************************************************************************

ClassRepositorySearcher::ClassRepositorySearcher (Model::ClassRepository* repository, ISearchDescription& searchDescription)
: repository (repository),
  AbstractSearcher (searchDescription)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ClassRepositorySearcher::find (ISearchResultSink& resultSink, IProgressNotify* progress)
{
	// global search, ignoring SearchProvider::startPoint
	ForEach (repository->getClasses (), Model::Class, c)
		if(searchDescription.matchesName (String (c->getName ())))
			resultSink.addResult (ccl_as_unknown (Model::ElementUrl::createUrl (*repository, *c)));

		ObjectArray members;
		c->getMembers (members, true);
		ForEach (members, Model::Member, m)
			if(searchDescription.matchesName (String (m->getName ())) && !Model::ClassQualifier::isInheritedMember (*m))
				resultSink.addResult (ccl_as_unknown (Model::ElementUrl::createUrl (*repository, *m)));
		EndFor

		ForEach (c->getMethods (), Model::Method, m)
			if(searchDescription.matchesName (String (m->getName ())))
				resultSink.addResult (ccl_as_unknown (Model::ElementUrl::createUrl (*repository, *m)));
		EndFor
	EndFor

	ForEach (repository->getEnumerations (), Model::Enumeration, e)
		if(searchDescription.matchesName (String (e->getName ())))
			resultSink.addResult (ccl_as_unknown (Model::ElementUrl::createUrl (*repository, *e)));

		ObjectArray enumerators;
		e->getEnumerators (enumerators, true);
		ForEach (enumerators, Model::Enumerator, v)
			if(searchDescription.matchesName (String (v->getName ())))
				resultSink.addResult (ccl_as_unknown (Model::ElementUrl::createUrl (*repository, *v)));
		EndFor
	EndFor

	ForEach (repository->getObjects (), Model::ObjectElement, o)
		if(searchDescription.matchesName (String (o->getName ())))
			resultSink.addResult (ccl_as_unknown (Model::ElementUrl::createUrl (*repository, *o)));
	EndFor
	return kResultOk;
}

//************************************************************************************************
// Model::ClassRepository
//************************************************************************************************

const FileType& ClassRepository::getFileType ()
{
	static const FileType fileType ("Class Model", "classModel", CCL_MIME_TYPE "-classmodel+xml");
	return fileType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_PERSISTENT (ClassRepository, StorableObject, "Model.ClassRepository")

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassRepository::ClassRepository ()
{
	classes.objectCleanup (true);
	enumerations.objectCleanup (true);
	objects.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassRepository::addClass (Class* c)
{
	c->setRepository (this);
	classes.addSorted (c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassRepository::addEnumeration (Enumeration* e)
{
	e->setRepository (this);
	enumerations.addSorted (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassRepository::addObject (ObjectElement* o)
{
	o->setRepository (this);
	objects.addSorted (o);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassRepository::setRepositoryLink (ObjectArray& elements, bool state)
{
	ArrayForEach (elements, MainElement, e)
		e->setRepository (state ? this : nullptr);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassRepository::removeAll ()
{
	setRepositoryLink (classes, false);
	setRepositoryLink (enumerations, false);
	setRepositoryLink (objects, false);

	classes.removeAll ();
	enumerations.removeAll ();
	objects.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassRepository::takeAll (ClassRepository& other)
{
	classes.add (other.classes, Container::kShare);
	other.classes.removeAll ();
	setRepositoryLink (classes, true);
	
	enumerations.add (other.enumerations, Container::kShare);
	other.enumerations.removeAll ();
	setRepositoryLink (enumerations, true);

	objects.add (other.classes, Container::kShare);
	other.objects.removeAll ();
	setRepositoryLink (objects, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassRepository::collectGroupNames (StringList& result) const
{
	ArrayForEach (classes, Class, c)
		String groupName = c->getGroupName ();
		if(!result.contains (groupName))
			result.addSorted (groupName);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
const T* ClassRepository::findElement (const ObjectArray& elements, StringID name) const
{
	if(name.isEmpty ())
		return nullptr;
	ArrayForEach (elements, Element, e)
		if(e->getName () == name)
			return (T*)e;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Class* ClassRepository::findClass (StringID name) const
{
	return findElement<Class> (classes, name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Class* ClassRepository::getSuperClass (const Class* c) const
{
	return c ? findClass (c->getParentName ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassRepository::collectSuperClasses (Container& result, const Class* c) const
{
	if(const Class* parent = getSuperClass (c))
		while(parent)
		{
			result.add (const_cast<Class*> (parent));
			parent = getSuperClass (parent);
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassRepository::collectDerivedClasses (Container& result, const Class* candidate) const
{
	if(!candidate)
		return;
	ArrayForEach (classes, Class, c)
		if(c == candidate)
			continue;
		
		#if 0
		ObjectArray temp;
		collectSuperClasses (temp, c);
		if(temp.contains (candidate))
			result.add (c);
		#else
		if(getSuperClass (c) == candidate)
			result.add (c);
		#endif
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassRepository::collectGroupClasses (Container& result, StringRef groupName) const
{
	ArrayForEach (classes, Class, c)
		if(c->getGroupName () == groupName)
			result.add (c);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Enumeration* ClassRepository::findEnum (StringID name) const
{
	return findElement<Enumeration> (enumerations, name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ObjectElement* ClassRepository::findObject (StringID name) const
{
	return findElement<ObjectElement> (objects, name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassRepository::collectObjectsFlat (Container& result, ObjectElement* parent) const
{
	const Container& nodes = parent == nullptr ? objects : parent->getChildren ();

	ForEach (nodes, ObjectElement, obj)
		result.add (obj);
		collectObjectsFlat (result, obj);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* ClassRepository::createSearcher (ISearchDescription& description)
{
	return NEW ClassRepositorySearcher (this, description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassRepository::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	
	name = a.getCString ("name");
	
	a.unqueue (classes, "classes", ccl_typeid<Class> ());
	setRepositoryLink (classes, true);
	classes.sort ();
	
	a.unqueue (enumerations, "enums", ccl_typeid<Enumeration> ());
	setRepositoryLink (enumerations, true);
	enumerations.sort ();

	a.unqueue (objects, "objects", ccl_typeid<ObjectElement> ());
	setRepositoryLink (objects, true);
	objects.sort ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassRepository::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("name", name);
	a.queue ("classes", classes);
	a.queue ("enums", enumerations);
	a.queue ("objects", objects);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ClassRepository::save (IStream& stream) const
{
	return StorableObject::saveToStream (*this, stream, XmlArchive::kDefineNamespace);
}

//************************************************************************************************
// Model::TypeInfoFilter
//************************************************************************************************

TypeInfoFilter::TypeInfoFilter (bool scriptableOnly)
: scriptableOnly (scriptableOnly)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeInfoFilter::matches (ITypeInfo& typeInfo) const
{
	bool scriptable = (typeInfo.getClassFlags () & ITypeInfo::kScriptable) != 0;
	if(scriptableOnly && !scriptable)
		return false;
	return true; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TypeInfoFilter::matches (IEnumTypeInfo& enumInfo) const
{
	return true; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TypeInfoFilter::matches (IUnknown* object) const
{
	UnknownPtr<ITypeInfo> typeInfo (object);
	if(typeInfo)
		return matches (*typeInfo);
	
	UnknownPtr<IEnumTypeInfo> enumInfo (object);
	if(enumInfo)
		return matches (*enumInfo);

	CCL_DEBUGGER ("Unknown object!\n")
	return false;
}

//************************************************************************************************
// Model::ClassRepositoryBuilder
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ClassRepositoryBuilder, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassRepositoryBuilder::ClassRepositoryBuilder (ClassRepository& repository)
: repository (repository)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassRepositoryBuilder::build (ITypeLibrary& typeLib, IObjectFilter* filter)
{
	repository.setName (typeLib.getLibraryName ());

	// *** 1) Classes ***
	IterForEachUnknown (typeLib.newTypeIterator (), unk)
		UnknownPtr<ITypeInfo> typeInfo (unk);
		ASSERT (typeInfo.isValid ())
		if(!typeInfo)
			continue;
		if(filter && !filter->matches (typeInfo))
			continue;

		Class* c = NEW Class (Element::toCanonicalName (typeInfo->getClassName ()));
		c->assign (typeLib, *typeInfo);

		MutableCString parentName;
		MutableCString parentNamespace;
		if(const ITypeInfo* parentType = typeInfo->getParentType ())
		{
			parentName = parentType->getClassName ();
			parentNamespace = parentType->getClassNamespace ();
		}
		c->setParentName (Element::toCanonicalName (parentName));
		c->setParentNamespace (parentNamespace);

		repository.addClass (c);
	EndFor

	// *** 2) Enumerations ***
	IterForEachUnknown (typeLib.newEnumIterator (), unk)
		UnknownPtr<IEnumTypeInfo> enumInfo (unk);
		ASSERT (enumInfo.isValid ())
		if(!enumInfo)
			continue;
		if(filter && !filter->matches (enumInfo))
			continue;

		Enumeration* e = NEW Enumeration (enumInfo->getName (), enumInfo->getParentName ());
		e->assign (typeLib, *enumInfo);

		repository.addEnumeration (e);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassRepositoryBuilder::build (StringID name, IObjectNode& object, const IExtractor& extractor, bool deep, ObjectElement* parent)
{
	ObjectElement* element = NEW ObjectElement (name);
	extractor.extract (*element, object);
	
	if(parent == nullptr)
	{
		repository.setName (name);
		repository.addObject (element);
	}
	else
		parent->addChild (element);

	if(deep)
		for(int i = 0, count = object.countChildren (); i < count; i++)
			if(IObjectNode* child = object.getChild (i))
			{
				MutableCString name (child->getObjectID ());
				ASSERT (!name.isEmpty ())
				if(name.isEmpty ())
					continue;

				build (name, *child, extractor, true, element);
			}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassRepositoryBuilder::build (StringID name, IObject& object, bool deep, ObjectElement* parent, Property* propertyInfo)
{
	ObjectElement* element = NEW ObjectElement (name);

	if(CStringPtr className = object.getTypeInfo ().getClassName ())
		element->setClassName (Element::toCanonicalName (className));

	if(CStringPtr classNamespace = object.getTypeInfo ().getClassNamespace ())
		element->setClassNamespace (classNamespace);

	int classFlags = object.getTypeInfo ().getClassFlags ();
	element->setDynamicType (get_flag<int> (classFlags, ITypeInfo::kMutable));

	if(propertyInfo)
	{
		// if object is a property of a parent object, add infos we got from property definition macros
		if(element->getClassName ().isEmpty ())
			element->setClassName (propertyInfo->getTypeName ());
		element->setReadOnly (propertyInfo->isReadOnly ());
	}

	if(parent == nullptr)
	{
		repository.setName (name);
		repository.addObject (element);
	}
	else
		parent->addChild (element);

	// Methods
	for(const ITypeInfo* typeInfo = &object.getTypeInfo (); typeInfo; typeInfo = typeInfo->getParentType ())
		if(const ITypeInfo::MethodDefinition* methodNames = typeInfo->getMethodNames ())
			for(int i = 0; methodNames[i].name != nullptr; i++)
			{
				Method* method = NEW Method (methodNames[i].name);
				method->assign (methodNames[i]);
				element->addMethod (method);
			}

	// Properties
	PropertyCollection props;
	object.getPropertyNames (props);
	
	for(int i = 0; i < props.count (); i++)
	{
		Property* property = props.getProperty (i);
		StringID name = property->getName ();

		Variant value;
		object.getProperty (value, name);

		UnknownPtr<IObject> child (value.asUnknown ());
		if(child)
		{
			if(deep)
				build (name, *child, true, element, property);
		}
		else
			element->addProperty (NEW Property (name, TypeNames::getDataType (value)));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassRepositoryBuilder::update (ClassRepository& documented, ClassRepository& prototype)
{
	ForEach (prototype.getClasses (), Class, protoClass)
		Class* newClass = (Class*)protoClass->clone ();
		if(const Class* docClass = documented.findClass (newClass->getName ()))
			newClass->takeDocumentation (*docClass);
		else
			newClass->isNew (true);
		repository.addClass (newClass);
	EndFor

	ForEach (prototype.getEnumerations (), Enumeration, protoEnum)
		Enumeration* newEnum = (Enumeration*)protoEnum->clone ();
		if(const Enumeration* docEnum = documented.findEnum (newEnum->getName ()))
			newEnum->takeDocumentation (*docEnum);
		else
			newEnum->isNew (true);
		repository.addEnumeration (newEnum);
	EndFor

	ForEach (prototype.getObjects (), ObjectElement, protoObject)
		ObjectElement* newObject = (ObjectElement*)protoObject->clone ();
		if(const ObjectElement* docObject = documented.findObject (newObject->getName ()))
			newObject->takeDocumentation (*docObject);
		else
			newObject->isNew (true);
		repository.addObject (newObject);
	EndFor
}

//************************************************************************************************
// Model::ClassQualifier
//************************************************************************************************

ClassQualifier::ClassQualifier (const Member& member)
: repository (nullptr),
  currentClass (nullptr)
{
	currentClass = ccl_cast<Class> (member.getEnclosure ());
	repository = currentClass ? currentClass->getRepository () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Class* ClassQualifier::next ()
{
	const Class* result = currentClass;
	currentClass = currentClass && repository ? repository->getSuperClass (currentClass) : nullptr;
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassQualifier::isInheritedMember (const Member& member, const Class** fromClass)
{
	ClassQualifier q (member);
	q.next (); // start at superclass
	while(const Class* c = q.next ())
		if(c->findMember (member.getName ()) != nullptr)
		{
			if(fromClass)
				*fromClass = c;
			return true;
		}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Element* ClassQualifier::findTypeForMember (const Member& member)
{
	ClassQualifier q (member);
	while(const Class* c = q.next ())
	{
		// try to find enum with "class.member" name used for XML models
		// LATER TODO: search for class/enum via member type name!

		MutableCString enumName;
		enumName = c->getName ();
		enumName += ".";
		enumName += member.getName ();

		if(const Enumeration* result = c->getRepository ()->findEnum (enumName))
			return result;
	}
	return nullptr;
}

//************************************************************************************************
//  Model::ElementUrl
//************************************************************************************************

Url* ElementUrl::createUrl (ClassRepository& repository, Class& c)
{
	return createUrl (CCLSTR ("class"), repository, c);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url* ElementUrl::createUrl (ClassRepository& repository, Member& m)
{
	return createChildUrl (CCLSTR ("member"), repository, m);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url* ElementUrl::createUrl (ClassRepository& repository, Method& m)
{
	return createChildUrl (CCLSTR ("method"), repository, m);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url* ElementUrl::createUrl (ClassRepository& repository, Enumeration& e)
{
	return createUrl (CCLSTR ("enum"), repository, e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url* ElementUrl::createUrl (ClassRepository& repository, Enumerator& e)
{
	return createChildUrl (CCLSTR ("enumerator"), repository, e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url* ElementUrl::createUrl (ClassRepository& repository, ObjectElement& o)
{
	return createUrl (CCLSTR ("object"), repository, o);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Element* ElementUrl::findElement (ClassRepository& repository, UrlRef url)
{
	if(MutableCString (url.getHostName ()) == repository.getName ())
	{
		if(url.getProtocol () == "class")
			return repository.findClass (MutableCString (url.getPath ()));

		else if(url.getProtocol () == "member")
		{
			String className, memberName;
			url.getPathName (className);
			url.getName (memberName);
			if(const Class* c = repository.findClass (MutableCString (className)))
				return c->findMember (MutableCString (memberName));
		}
		else if(url.getProtocol () == "method")
		{
			String className, methodName;
			url.getPathName (className);
			url.getName (methodName);
			if(const Class* c = repository.findClass (MutableCString (className)))
				return c->findMethod (MutableCString (methodName));
		}
		else if(url.getProtocol () == "enum")
			return repository.findEnum (MutableCString (url.getPath ()));

		else if(url.getProtocol () == "enumerator")
		{
			String enumName, valueName;
			url.getPathName (enumName);
			url.getName (valueName);
			if(const Enumeration* e = repository.findEnum (MutableCString (enumName)))
				return e->findEnumerator (MutableCString (valueName));
		}

		else if(url.getProtocol () == "object")
			return repository.findObject (MutableCString (url.getPath ()));
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url* ElementUrl::createUrl (StringRef protocol, ClassRepository& repository, Element& element)
{
	return NEW Url (protocol, String (repository.getName ()), String (element.getName ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url* ElementUrl::createChildUrl (StringRef protocol, ClassRepository& repository, Element& element)
{
	if(Element* enclosure = element.getEnclosure ())
	{
		Url* url = createUrl (protocol, repository, *enclosure);
		url->descend (String (element.getName ()));
		return url;
	}
	return nullptr;
}
