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
// Filename    : ccl/public/plugins/classfactory.h
// Description : Class Factory
//
//************************************************************************************************

#ifndef _ccl_classfactory_h
#define _ccl_classfactory_h

#include "ccl/public/base/unknown.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/collections/hashtable.h"
#include "ccl/public/plugins/iclassfactory.h"

namespace CCL {

//************************************************************************************************
// ClassAttributeProvider
/** \ingroup base_plug */
//************************************************************************************************

class ClassAttributeProvider
{
public:
	virtual bool getClassAttributes (IAttributeList& attributes, UIDRef cid, StringID language) const = 0;
};

//************************************************************************************************
// ClassFilter
/** \ingroup base_plug */
//************************************************************************************************

class ClassFilter
{
public:
	virtual bool matches (const ClassDesc& description) const = 0;
};

//************************************************************************************************
// ClassFactory
/** \ingroup base_plug */
//************************************************************************************************

class ClassFactory: public Unknown,
					public IClassFactory
{
public:
	ClassFactory ();
	~ClassFactory ();

	typedef IUnknown* (*UnknownCreateFunc) (UIDRef cid, void* userData);

	/** Global class factory instance, must be released by caller! */
	static ClassFactory* instance ();
	
	/** Set factory version. */
	void setVersion (const VersionDesc& version);

	/** Check if factory is empty */
	bool isEmpty () const;

	/** Find registered class. */
	const ClassDesc* findClass (UIDRef cid) const;

	/** Register class. */
	bool registerClass (const ClassDesc& description, 
						UnknownCreateFunc createFunc, 
						void* userData = nullptr,
						IAttributeList* attributes = nullptr);
	
	/** Unregister class. */
	bool unregisterClass (UIDRef cid);
	
	/** Unregister all classes. */
	bool unregisterAll ();

	/** Use subcategory as class folder attribute. */
	PROPERTY_BOOL (subCategoryAsFolder, SubCategoryAsFolder)

	/** Enable localization of class attributes. */
	PROPERTY_BOOL (localizationEnabled, LocalizationEnabled)

	/** Class attribute provider (optional). */
	PROPERTY_POINTER (ClassAttributeProvider, attributeProvider, AttributeProvider)

	// IClassFactory
	void CCL_API getVersion (VersionDesc& version) const override;
	int CCL_API getNumClasses () const override;
	tbool CCL_API getClassDescription (ClassDesc& description, int index) const override;
	tbool CCL_API getClassAttributes (IAttributeList& attributes, UIDRef cid, StringID language) const override;

	// IClassAllocator
	tresult CCL_API createInstance (UIDRef cid, UIDRef iid, void** obj) override;

	CLASS_INTERFACE2 (IClassFactory, IClassAllocator, Unknown)

protected:
	struct ClassEntry
	{
		ClassDesc description;
		UnknownCreateFunc createFunc;
		void* userData;
		IAttributeList* attributes;

		ClassEntry (const ClassDesc& description,
					UnknownCreateFunc createFunc,
					void* userData,
					IAttributeList* attributes);
		~ClassEntry ();
	};

	struct ClassHashEntry
	{
		UIDBytes cid;
		ClassEntry* entry;

		ClassHashEntry (UIDRef cid = kNullUID, 
						ClassEntry* entry = nullptr)
		: cid (cid),
		  entry (entry)
		{}

		bool operator == (const ClassHashEntry& entry)
		{ return cid == entry.cid; }
	};

	VersionDesc version;
	Vector<ClassEntry*> classes;
	HashTable<ClassHashEntry, LinkedList<ClassHashEntry> > classIdTable;

	ClassEntry* lookup (UIDRef cid) const;

	static ClassFactory* theInstance;
	static int hashClassEntry (const ClassHashEntry& entry, int size);
};

} // namespace CCL

#endif // _ccl_classfactory_h
