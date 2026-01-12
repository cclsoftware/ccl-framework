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
// Filename    : ccl/extras/modeling/classrepository.h
// Description : Class Repository
//
//************************************************************************************************

#ifndef _ccl_classrepository_h
#define _ccl_classrepository_h

#include "ccl/extras/modeling/classmodel.h"

#include "ccl/base/storage/storableobject.h"

#include "ccl/public/base/irecognizer.h"

namespace CCL {

interface IObjectNode;
interface ISearcher;
interface ISearchDescription;

class Url;
class StringList;

namespace Model {

//************************************************************************************************
// Model::ClassRepository
//************************************************************************************************

class ClassRepository: public StorableObject
{
public:
	DECLARE_CLASS (ClassRepository, StorableObject)

	ClassRepository ();

	static const FileType& getFileType ();

	PROPERTY_MUTABLE_CSTRING (name, Name)
	String getTitle () const { return String (name); }

	void addClass (Class* c);
	void addEnumeration (Enumeration* e);
	void addObject (ObjectElement* o);

	void removeAll ();
	void takeAll (ClassRepository& other);

	const Container& getClasses () const { return classes; }
	const Container& getEnumerations () const { return enumerations; }
	const Container& getObjects () const { return objects; }
	void collectGroupNames (StringList& result) const;

	const Class* findClass (StringID name) const;
	const Class* getSuperClass (const Class* c) const;
	void collectSuperClasses (Container& result, const Class* c) const;
	void collectDerivedClasses (Container& result, const Class* c) const;
	void collectGroupClasses (Container& result, StringRef groupName) const;

	const Enumeration* findEnum (StringID name) const;

	const ObjectElement* findObject (StringID name) const;
	void collectObjectsFlat (Container& result, ObjectElement* parent = nullptr) const;

	ISearcher* createSearcher (ISearchDescription& description);

	// StorableObject
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;
	tbool CCL_API save (IStream& stream) const override;

protected:
	ObjectArray classes;
	ObjectArray enumerations;
	ObjectArray objects;

	void setRepositoryLink (ObjectArray& elements, bool state);

	template<class T>
	const T* findElement (const ObjectArray& elements, StringID name) const;
};

//************************************************************************************************
// Model::TypeInfoFilter
//************************************************************************************************

class TypeInfoFilter: public ObjectFilter
{
public:
	TypeInfoFilter (bool scriptableOnly = false);

	PROPERTY_BOOL (scriptableOnly, ScriptableOnly)

	virtual bool matches (ITypeInfo& typeInfo) const;
	virtual bool matches (IEnumTypeInfo& enumInfo) const;

	// IObjectFilter
	tbool CCL_API matches (IUnknown* object) const override;
};

//************************************************************************************************
// Model::ClassRepositoryBuilder
//************************************************************************************************

class ClassRepositoryBuilder: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (ClassRepositoryBuilder, Object)

	ClassRepositoryBuilder (ClassRepository& repository);

	interface IExtractor
	{
		virtual void extract (ObjectElement& element, IObjectNode& object) const = 0;
	};

	bool build (ITypeLibrary& typeLib, IObjectFilter* filter = nullptr);
	bool build (StringID name, IObjectNode& object, const IExtractor& extractor, bool deep, ObjectElement* parent = nullptr);
	bool build (StringID name, IObject& object, bool deep, ObjectElement* parent = nullptr, Property* propertyInfo = nullptr);

	void update (ClassRepository& documented, ClassRepository& prototype);

protected:
	ClassRepository& repository;
};

//************************************************************************************************
// Model::ClassQualifier
//************************************************************************************************

class ClassQualifier
{
public:
	ClassQualifier (const Member& member);

	const Class* next (); ///< get next class in inheritance tree

	static bool isInheritedMember (const Member& member, const Class** fromClass = nullptr);
	static const Element* findTypeForMember (const Member& member);

protected:
	ClassRepository* repository;
	const Class* currentClass;
};

//************************************************************************************************
//  Model::ElementUrl
/** Urls for model elements:  protocol://repository/elementName
	protocol can be class, enum, object */
//************************************************************************************************

class ElementUrl
{
public:
	static Url* createUrl (ClassRepository& repository, Class& c);
	static Url* createUrl (ClassRepository& repository, Member& m);
	static Url* createUrl (ClassRepository& repository, Method& m);
	static Url* createUrl (ClassRepository& repository, Enumeration& e);
	static Url* createUrl (ClassRepository& repository, Enumerator& e);
	static Url* createUrl (ClassRepository& repository, ObjectElement& o);

	static const Element* findElement (ClassRepository& repository, UrlRef url);

private:
	static Url* createUrl (StringRef protocol, ClassRepository& repository, Element& element);
	static Url* createChildUrl (StringRef protocol, ClassRepository& repository, Element& element);
};

} // namespace Model
} // namespace CCL

#endif // _ccl_classrepository_h
