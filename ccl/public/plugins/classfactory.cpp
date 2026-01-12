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
// Filename    : ccl/public/plugins/classfactory.cpp
// Description : Class Factory
//
//************************************************************************************************

#include "ccl/public/plugins/classfactory.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/system/ipackagemetainfo.h"

using namespace CCL;

//************************************************************************************************
// ClassFactory::ClassEntry
//************************************************************************************************

ClassFactory::ClassEntry::ClassEntry (const ClassDesc& description, UnknownCreateFunc createFunc, 
									  void* userData, IAttributeList* attributes)
: description (description),
  createFunc (createFunc),
  userData (userData),
  attributes (attributes)
{
	if(attributes)
		attributes->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassFactory::ClassEntry::~ClassEntry ()
{
	if(attributes)
		attributes->release ();
}

//************************************************************************************************
// ClassFactory
//************************************************************************************************

ClassFactory* ClassFactory::theInstance = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

int ClassFactory::hashClassEntry (const ClassHashEntry& entry, int size)
{
	uint32 uidHashCode = UID (entry.cid).hash ();
	return ((int)uidHashCode & 0x7FFFFFFF) % size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassFactory* ClassFactory::instance ()
{
	if(theInstance)
		theInstance->retain ();
	else
		theInstance = NEW ClassFactory;
	return theInstance;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassFactory::ClassFactory ()
: classIdTable (256, hashClassEntry),
  subCategoryAsFolder (false),
  localizationEnabled (false),
  attributeProvider (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassFactory::~ClassFactory ()
{
	VectorForEach (classes, ClassEntry*, e)
		delete e;
	EndFor
	classes.removeAll ();

	if(theInstance == this)
		theInstance = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassFactory::isEmpty () const
{ 
	return classes.isEmpty (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassFactory::registerClass (const ClassDesc& description, UnknownCreateFunc createFunc, void* userData, IAttributeList* attributes)
{
	ClassEntry* existingEntry = lookup (description.classID);
	if(existingEntry)
	{
		CCL_WARN ("UID conflict on class registration: %s (%s %s)\n", MutableCString (description.name).str (), 
																	  MutableCString (description.category).str (),
																	  MutableCString (description.subCategory).str ())
		return false;
	}

	ClassEntry* entry = NEW ClassEntry (description, createFunc, userData, attributes);
	classes.add (entry);
	classIdTable.add (ClassHashEntry (description.classID, entry));
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassFactory::unregisterClass (UIDRef cid)
{
	ClassEntry* entry = lookup (cid);
	if(entry == nullptr)
		return false;

	classes.remove (entry);
	classIdTable.remove (ClassHashEntry (cid, entry));
	delete entry;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassFactory::unregisterAll ()
{
	VectorForEach (classes, ClassEntry*, e)
		classIdTable.remove (ClassHashEntry (e->description.classID, e));
		delete e;
	EndFor
	classes.removeAll ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassFactory::setVersion (const VersionDesc& _version)
{ 
	version = _version; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ClassFactory::getVersion (VersionDesc& _version) const
{
	_version = version;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ClassFactory::getNumClasses () const
{
	return classes.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ClassFactory::getClassDescription (ClassDesc& description, int index) const
{
	ClassEntry* e = classes.at (index);
	if(e)
	{
		description = e->description;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ClassFactory::getClassAttributes (IAttributeList& attributes, UIDRef cid, StringID language) const
{
	bool result = false;

	if(attributeProvider)
		result = attributeProvider->getClassAttributes (attributes, cid, language);

	ClassEntry* entry = lookup (cid);
	if(entry)
	{
		if(entry->attributes)
		{
			attributes.addFrom (*entry->attributes);
			result = true;
		}

		if(subCategoryAsFolder && !entry->description.subCategory.isEmpty ())
		{
			attributes.setAttribute (Meta::kClassFolder, entry->description.subCategory);
			result = true;
		}

		if(localizationEnabled)
		{
			String localizedName = TRANSLATE (entry->description.name);
			if(localizedName != entry->description.name)
			{
				attributes.setAttribute (Meta::kClassLocalizedName, localizedName);
				result = true;
			}

			if(!entry->description.subCategory.isEmpty ())
			{
				String localizedSubCategory = TRANSLATE (entry->description.subCategory);
				if(localizedSubCategory != entry->description.subCategory)
				{
					attributes.setAttribute (Meta::kClassLocalizedSubCategory, localizedSubCategory);
					result = true;
				}
			}

			if(!entry->description.description.isEmpty ())
			{
				String localizedDescription = TRANSLATE (entry->description.description);
				if(localizedDescription != entry->description.description)
				{
					attributes.setAttribute (Meta::kClassLocalizedDescription, localizedDescription);
					result = true;
				}
			}
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassFactory::ClassEntry* ClassFactory::lookup (UIDRef cid) const
{
	ClassEntry* entry = nullptr;
	#if 0
	VectorForEach (classes, ClassEntry*, e)
		if(e->desc.classID.equals (cid))
		{
			entry = e;
			break;
		}
	EndFor
	#else
	entry = classIdTable.lookup (ClassHashEntry (cid)).entry;
	#endif
	return entry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ClassDesc* ClassFactory::findClass (UIDRef cid) const
{
	if(ClassEntry* entry = lookup (cid))
		return &entry->description;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ClassFactory::createInstance (UIDRef cid, UIDRef iid, void** obj)
{
	tresult result = kResultClassNotFound;
	ClassEntry* entry = lookup (cid);
	if(entry && entry->createFunc)
	{
		AutoPtr<IUnknown> newObj = entry->createFunc (cid, entry->userData);
		if(newObj)
			result = newObj->queryInterface (iid, obj);
	}
	return result;
}
