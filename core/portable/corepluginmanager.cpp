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
// Filename    : core/portable/corepluginmanager.cpp
// Description : Plug-in Management
//
//************************************************************************************************

#include "core/portable/corepluginmanager.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// BuiltInCodeResource
//************************************************************************************************

BuiltInCodeResource::BuiltInCodeResource (CStringPtr name, Plugins::GetClassInfoBundleProc entryPoint)
: name (name),
  classInfoBundle (nullptr)
{
	classInfoBundle = (*entryPoint)(Plugins::kAPIVersion);
	ASSERT (classInfoBundle != nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr BuiltInCodeResource::getResourceName () const
{
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Plugins::ClassInfoBundle* BuiltInCodeResource::getClassInfoBundle ()
{
	return classInfoBundle;
}

//************************************************************************************************
// PluginManager
//************************************************************************************************

DEFINE_STATIC_SINGLETON (PluginManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

PluginManager::~PluginManager ()
{
	VectorForEachFast (codeResources, CodeResource*, codeResource)
		delete codeResource;
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PluginManager::addCodeResource (CodeResource* codeResource)
{
	codeResources.add (codeResource);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ForEachPlugInClass(codeResources, classInfo) \
  VectorForEachFast (codeResources, CodeResource*, codeResource) \
    if(const Plugins::ClassInfoBundle* classInfoBundle = codeResource->getClassInfoBundle ()) \
	  for(int i = 0; i < classInfoBundle->numClasses; i++) \
        if(const Plugins::ClassInfo* classInfo = classInfoBundle->classInfos[i])

const Plugins::ClassInfo* PluginManager::findClass (UIDRef classId) const
{
	CString64 idString;
	classId.toCString (idString.getBuffer (), idString.getSize ());

	// TODO: use hash table with class identifiers...
	ForEachPlugInClass (codeResources, classInfo)
		if(idString == classInfo->classID)
			return classInfo;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PluginManager::collectClasses (ClassList& classList, CStringPtr _classType) const
{
	ConstString classType (_classType);
	ForEachPlugInClass (codeResources, classInfo)
		if(classType == classInfo->classType)
			classList.add (classInfo);
	EndFor
}

//************************************************************************************************
// ClassAttributeReader
//************************************************************************************************

ClassAttributeReader::ClassAttributeReader (const Plugins::ClassInfo& classInfo)
: classInfo (classInfo)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassAttributeReader::getValue (CString64& value, CStringPtr _key) const
{
	value.empty ();
	CString64 key (_key);
	key += '=';

	ConstString attrString (classInfo.classAttributes);
	int index = attrString.index (key);
	if(index != -1)
	{
		index += key.length ();
		int maxLength = attrString.length ();
		while(index < maxLength)
		{
			char c = attrString[index++];
			if(c == '\n')
				break;
			value += c;
		}
		return true;
	}
	return false;
}

//************************************************************************************************
// AuthorizationPolicy
//************************************************************************************************

const Attributes* AuthorizationPolicy::findItemOfType (const Attributes& parent, CStringPtr _sid, CStringPtr _typeId)
{
	ConstString sid (_sid);
	ConstString typeId (_typeId);
	if(const AttributeQueue* itemArray = parent.getQueue (kChildren))
		VectorForEachFast (itemArray->getValues (), AttributeValue*, value)
			if(const Attributes* itemAttr = value->getAttributes ())
			{
				if(typeId == itemAttr->getString (kTypeID) && sid == itemAttr->getString (kSID))
					return itemAttr;
			}
		EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Attributes* AuthorizationPolicy::findMatchingItem (const Attributes& parent, CStringPtr _sid)
{
	ConstString sid (_sid);
	ConstString any (kAny);
	if(const AttributeQueue* children = parent.getQueue (kChildren))
		VectorForEachFast (children->getValues (), AttributeValue*, value)
			if(const Attributes* itemAttr = value->getAttributes ())
			{
				ConstString itemSid (itemAttr->getString (kSID));
				if(itemSid == sid || itemSid == any)
					return itemAttr;
			}
		EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AuthorizationPolicy::checkAccess (const Attributes& parent, CStringPtr _sid)
{
	ConstString sid (_sid);
	ConstString any (kAny);
	if(const AttributeQueue* children = parent.getQueue (kChildren))
		VectorForEachFast (children->getValues (), AttributeValue*, value)
			if(const Attributes* itemAttr = value->getAttributes ())
			{
				ConstString itemSid (itemAttr->getString (kSID));
				if(itemSid == sid || itemSid == any)
				{
					ConstString itemType (itemAttr->getString (kTypeID));
					if(itemType == kAccessDenied)
						return false;
					if(itemType == kAccessAllowed)
						return true;
				}
			}
		EndFor
	return false;
}
