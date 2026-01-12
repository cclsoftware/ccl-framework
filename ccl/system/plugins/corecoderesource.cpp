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
// Filename    : ccl/system/plugins/corecoderesource.cpp
// Description : Core Plug-in Code Resource
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/plugins/corecoderesource.h"

#include "ccl/base/storage/propertyfile.h"

#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/plugins/pluginst.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// CoreClassFactory
//************************************************************************************************

class CoreClassFactory: public Object,
						public IClassFactory
{
public:
	CoreClassFactory (const Core::Plugins::ClassInfoBundle& classBundle);

	// IClassFactory
	void CCL_API getVersion (VersionDesc& version) const override;
	int CCL_API getNumClasses () const override;
	tbool CCL_API getClassDescription (ClassDesc& description, int index) const override;
	tbool CCL_API getClassAttributes (IAttributeList& attributes, UIDRef cid, StringID language) const override;
	tresult CCL_API createInstance (UIDRef cid, UIDRef iid, void** obj) override;

	CLASS_INTERFACE (IClassFactory, Object)

protected:
	const Core::Plugins::ClassInfoBundle& classBundle;
	VersionDesc versionInfo;

	const Core::Plugins::ClassInfo* findClass (UIDRef cid) const;
	void toDescription (ClassDesc& description, const Core::Plugins::ClassInfo& classInfo) const;
};

//************************************************************************************************
// CoreClass
//************************************************************************************************

class CoreClass: public Unknown,
				 public PluginInstance,
				 public ICoreClass
{
public:
	CoreClass (const Core::Plugins::ClassInfo& classInfo);

	// ICoreClass
	const Core::Plugins::ClassInfo& CCL_API getClassInfo () const override;
	tbool CCL_API getComponentClassID (UIDBytes& cid) const override;

	CLASS_INTERFACE2 (ICoreClass, IPluginInstance, Unknown)

protected:
	const Core::Plugins::ClassInfo& classInfo;
};

} // namespace CCL

using namespace CCL;
using namespace Core;
using namespace Plugins;

//************************************************************************************************
// CoreClass
//************************************************************************************************

CoreClass::CoreClass (const ClassInfo& classInfo)
: classInfo (classInfo)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ClassInfo& CCL_API CoreClass::getClassInfo () const
{
	return classInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreClass::getComponentClassID (UIDBytes& cid) const
{
	CString attrString (classInfo.classAttributes);
	if(attrString.startsWith ("{"))
		return cid.fromCString (attrString);
	else
		return false;
}

//************************************************************************************************
// CoreCodeResource
//************************************************************************************************

CoreCodeResource::CoreCodeResource (IExecutableImage& image)
: image (image)
{
	GetClassInfoBundleProc getClassInfoBundle = (GetClassInfoBundleProc)image.getFunctionPointer ("CoreGetClassInfoBundle");

	ASSERT (getClassInfoBundle != nullptr)
	if(getClassInfoBundle == nullptr)
		return;

	const ClassInfoBundle* classBundle = (*getClassInfoBundle) (kAPIVersion);
	ASSERT (classBundle != nullptr)
	if(classBundle == nullptr)
		return;

	classFactory = NEW CoreClassFactory (*classBundle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CoreCodeResource::~CoreCodeResource ()
{
	safe_release (classFactory);
	image.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreCodeResource::queryInterface (UIDRef iid, void** ptr)
{
	// make IExecutableImage accessible
	if(iid == ccl_iid<IExecutableImage> ())
		return image.queryInterface (iid, ptr);

	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API CoreCodeResource::getType () const
{
	return CodeResourceType::kCore;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* CCL_API CoreCodeResource::getMetaInfo ()
{
	return const_cast<IAttributeList*> (image.getMetaInfo ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreCodeResource::getPath (IUrl& path) const
{
	return image.getPath (path);
}

//************************************************************************************************
// CoreClassFactory
//************************************************************************************************

CoreClassFactory::CoreClassFactory (const ClassInfoBundle& classBundle)
: classBundle (classBundle)
{
	String versionInfoString;
	versionInfoString.appendCString (Text::kUTF8, classBundle.versionInfo);

	// parse version information
	StringDictionary properties;
	Java::PropertyParser (properties).parse (versionInfoString);

	versionInfo.name = properties.lookupValue (CCLSTR ("name"));
	versionInfo.version = properties.lookupValue (CCLSTR ("version"));
	versionInfo.vendor = properties.lookupValue (CCLSTR ("vendor"));
	versionInfo.copyright = properties.lookupValue (CCLSTR ("copyright"));
	versionInfo.url = properties.lookupValue (CCLSTR ("url"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CoreClassFactory::getVersion (VersionDesc& version) const
{
	version = versionInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API CoreClassFactory::getNumClasses () const
{
	return classBundle.numClasses;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreClassFactory::getClassDescription (ClassDesc& description, int index) const
{
	if(index < 0 || index >= classBundle.numClasses)
		return false;

	const ClassInfo* classInfo = classBundle.classInfos[index];
	if(classInfo == nullptr)
		return false;

	toDescription (description, *classInfo);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CoreClassFactory::toDescription (ClassDesc& description, const Core::Plugins::ClassInfo& classInfo) const
{
	UID cid;
	cid.fromCString (classInfo.classID);
	description.classID = cid;

	// flags compatible with CCL
	static const int kCompatibleFlags = ClassInfo::kDiscardable;
	description.flags = (classInfo.flags & kCompatibleFlags);

	// check for subcategory
	StringRef classType = CCLSTR (classInfo.classType);
	int separatorIndex = classType.index (":");
	if(separatorIndex != -1)
	{
		description.category = classType.subString (0, separatorIndex);
		description.subCategory = classType.subString (separatorIndex+1);
	}
	else
		description.category = classType;

	description.name.appendCString (Text::kUTF8, classInfo.displayName);

	CoreCodeLoader::instance ().getDescription (description, classInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ClassInfo* CoreClassFactory::findClass (UIDRef cid) const
{
	MutableCString cidString;
	UID (cid).toCString (cidString);
	for(int i = 0; i < classBundle.numClasses; i++)
		if(const ClassInfo* classInfo = classBundle.classInfos[i])
			if(cidString.compare (classInfo->classID, false) == 0)
				return classInfo;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreClassFactory::getClassAttributes (IAttributeList& attributes, UIDRef cid, StringID language) const
{
	bool result = false;
	if(const ClassInfo* classInfo = findClass (cid))
	{
		ClassDesc description;
		toDescription (description, *classInfo);

		if(!description.subCategory.isEmpty ()) // implicitly use subcategory as folder (see ClassFactory::setSubCategoryAsFolder())
		{
			attributes.setAttribute (Meta::kClassFolder, description.subCategory);
			result = true;
		}

		int attrStartIndex = 0;
		CString attrString (classInfo->classAttributes);
		if(attrString.startsWith ("{"))
		{
			attrStartIndex =  attrString.index ("}") + 1;
			MutableCString idString = attrString.subString (0, attrStartIndex);
			AttributeAccessor (attributes).set (Meta::kComponentClassID, idString);
		}

		String attrUnicodeString;
		attrUnicodeString.appendCString (Text::kUTF8, attrString.subString (attrStartIndex));
		if(!attrUnicodeString.isEmpty ())
		{
			StringDictionary properties;
			Java::PropertyParser (properties).parse (attrUnicodeString);
			CCL_PRINTF ("Core class attributes for %s %s:\n", classInfo->classID, classInfo->displayName)
			for(int i = 0; i < properties.countEntries (); i++)
			{
				MutableCString key (properties.getKeyAt (i));
				String value (properties.getValueAt (i));
				CCL_PRINTF ("%s = %s\n", key.str (), MutableCString (value).str ())
				AttributeAccessor (attributes).set (key, value);
			}
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreClassFactory::createInstance (UIDRef cid, UIDRef iid, void** obj)
{
	if(const ClassInfo* classInfo = findClass (cid))
	{
		AutoPtr<IUnknown> unk = CoreCodeLoader::instance ().createInstance (*classInfo, iid);
		return unk->queryInterface (iid, obj);
	}
	*obj = nullptr;
	return kResultNoInterface;
}

//************************************************************************************************
// CoreCodeLoader
//************************************************************************************************

DEFINE_SINGLETON (CoreCodeLoader)

//////////////////////////////////////////////////////////////////////////////////////////////////

CoreCodeLoader::~CoreCodeLoader ()
{
	ASSERT (handlerList.isEmpty ())
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreCodeLoader::registerHandler (ICoreClassHandler* handler)
{
	handlerList.append (handler);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreCodeLoader::unregisterHandler (ICoreClassHandler* handler)
{
	handlerList.remove (handler);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IClassFactory* CCL_API CoreCodeLoader::createClassFactory (const Core::Plugins::ClassInfoBundle& classBundle)
{
	return NEW CoreClassFactory (classBundle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CoreCodeLoader::getDescription (ClassDesc& description, const Core::Plugins::ClassInfo& classInfo)
{
	ListForEach (handlerList, ICoreClassHandler*, handler)
		if(handler->getDescription (description, classInfo))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API CoreCodeLoader::createInstance (const ClassInfo& classInfo, UIDRef iid)
{
	if(iid != ccl_iid<ICoreClass> ())
		ListForEach (handlerList, ICoreClassHandler*, handler)
			if(IUnknown* instance = handler->createInstance (classInfo, iid))
				return instance;
		EndFor
	return static_cast<ICoreClass*> (NEW CoreClass (classInfo));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API CoreCodeLoader::getType () const
{
	return CodeResourceType::kCore;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CoreCodeLoader::loadCodeResource (ICodeResource*& codeResource, UrlRef path)
{
	IExecutableImage* nativeImage = nullptr;
	tresult result = System::GetExecutableLoader ().loadImage (nativeImage, path);
	if(result == kResultOk)
	{
		codeResource = NEW CoreCodeResource (*nativeImage);
		if(codeResource->getClassFactory ())
			return kResultOk;

		safe_release (codeResource);
		result = kResultFailed;
	}
	return result;
}
