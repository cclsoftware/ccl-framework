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
// Filename    : ccl/system/plugins/plugmanager.cpp
// Description : Plugin Manager
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/plugins/plugmanager.h"
#include "ccl/system/plugins/stubclasses.h"
#include "ccl/system/plugins/scriptcoderesource.h"
#include "ccl/system/plugins/corecoderesource.h"
#include "ccl/system/plugins/module.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/collections/stringlist.h"

#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/base/irecognizer.h"
#include "ccl/public/system/isearcher.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ipackagemetainfo.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/ilogger.h"
#include "ccl/public/plugins/iclassfactory.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// ClassCategory
//************************************************************************************************

class ClassCategory: public Object,
					 public IPlugInClassList
{
public:
	ClassCategory (StringRef name = nullptr)
	: name (name)
	{ classes.objectCleanup (); }

	StringRef getName () const						{ return name; }
	void addClass (ClassDescription* classDesc)		{ classes.add (classDesc); }
	void removeClass (ClassDescription* classDesc)	{ classes.remove (classDesc); }
	int count () const								{ return classes.count (); }
	ClassDescription* at (int idx) const			{ return (ClassDescription*)classes.at (idx); }
	Iterator* newIterator () const					{ return classes.newIterator (); }

	// IPlugInClassList
	int CCL_API getNumClasses () const override { return count (); }
	const IClassDescription& CCL_API getClass (int index) const override
	{
		ClassDescription* cd = at (index);
		if(cd)
			return *cd;

		static const ClassDescription emptyDescription;
		return emptyDescription;
	}

	// Object
	bool equals (const Object& obj) const override
	{ return name == ((const ClassCategory*)&obj)->getName (); }

	CLASS_INTERFACE (IPlugInClassList, Object)

protected:
	String name;
	ObjectArray classes;
};

//************************************************************************************************
// CodeModule
//************************************************************************************************

class CodeModule: public Module,
				  public ICodeResource
{
public:
	DECLARE_CLASS (CodeModule, Module)

	CodeModule (UrlRef path = Url (), ICodeResourceLoader* loader = nullptr);
	~CodeModule ();

	void addInstance (IUnknown* instance);
	int releaseInstance (IUnknown* instance);
	void checkUnload (bool force = false);
	void forceUnload ();
	int getNumInstances () const;

	bool matches (ICodeResourceLoader* loader) const;

	// ICodeResource
	StringID CCL_API getType () const override;
	IClassFactory* CCL_API getClassFactory () override;
	IAttributeList* CCL_API getMetaInfo () override;
	tbool CCL_API getPath (IUrl& path) const override;

	using Module::getPath;

	CLASS_INTERFACES (Module)

protected:
	ICodeResourceLoader* loader;
	ICodeResource* resource;
	int numInstances;

	ICodeResourceLoader& getLoader () const;

	// Module
	bool loadInternal () override;
	void unloadInternal () override;
	bool isLoadedInternal () const override;
};

//************************************************************************************************
// InstanceAssoc
//************************************************************************************************

struct InstanceAssoc: Object
{
	DECLARE_CLASS (InstanceAssoc, Object)

	void* instance;
	//SharedPtr<ClassDescription> desc; does not work as expected!
	ClassDescription* desc;

	InstanceAssoc (void* obj = nullptr, ClassDescription* desc = nullptr)
	: instance (obj),
	  desc (desc)
	{}

	// Object
	bool equals (const Object& obj) const override
	{ return instance == ((InstanceAssoc*)&obj)->instance; }
	int getHashCode (int size) const override
	{ return ccl_hash_pointer (instance, size); }
};

DEFINE_CLASS (InstanceAssoc, Object)
DEFINE_CLASS_NAMESPACE (InstanceAssoc, NAMESPACE_CCL)

//************************************************************************************************
// ClassCollection
//************************************************************************************************

class ClassCollection: public ObjectArray
{
public:
	ClassCollection (bool cleanup);

	PROPERTY_BOOL (discardable, Discardable)
	PROPERTY_SHARED_AUTO (VersionDescription, version, Version)

	void collect (IClassFactory& factory, StringID language = nullptr);
};

//************************************************************************************************
// ClassSearcher
//************************************************************************************************

class ClassSearcher: public Object,
					 public AbstractSearcher
{
public:
	ClassSearcher (ISearchDescription& searchDescription);

	// ISearcher
	tresult CCL_API find (ISearchResultSink& resultSink, IProgressNotify* progress) override;

	CLASS_INTERFACE (ISearcher, Object)
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("PlugInManager")
	XSTRING (DeletedScriptObjectIsStillReferenced, "Deleted script object from \"%(1)\" is still referenced.")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-in Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IPlugInManager& CCL_API System::CCL_ISOLATED (GetPlugInManager) ()
{
	return PlugInManager::instance ();
}

//************************************************************************************************
// ClassDescription
//************************************************************************************************

DEFINE_CLASS (ClassDescription, Object)
DEFINE_CLASS_NAMESPACE (ClassDescription, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassDescription::ClassDescription (StringRef name)
: resource (nullptr),
  version (nullptr),
  name (name),
  attributes (nullptr),
  classFlags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassDescription::ClassDescription (UIDRef classID)
: resource (nullptr),
  version (nullptr),
  classID (classID),
  attributes (nullptr),
  classFlags (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassDescription::ClassDescription (const ClassDescription& cd)
: classID (cd.classID),
  category (cd.category),
  name (cd.name),
  subCategory (cd.subCategory),
  description (cd.description),
  classFlags (cd.classFlags),
  resource (nullptr), // <-- do not copy code resource!
  version (nullptr),
  attributes (nullptr)
{
	if(cd.version)
		setVersion (cd.version);
	if(cd.attributes)
		attributes = (PersistentAttributes*)cd.attributes->clone ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassDescription::ClassDescription (const ClassDesc& description)
: resource (nullptr),
  version (nullptr),
  attributes (nullptr),
  classFlags (0)
{
	fromClassDesc (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassDescription::~ClassDescription ()
{
	if(resource)
		resource->release ();
	if(version)
		version->release ();
	if(attributes)
		attributes->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ClassDescription::queryInterface (UIDRef iid, void** ptr)
{
	// make code resource accessible
	if(iid == ccl_iid<ICodeResource> () && resource)
		return resource->queryInterface (iid, ptr);

	QUERY_INTERFACE (IClassDescription)
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDRef CCL_API ClassDescription::getClassID () const
{
	return classID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API ClassDescription::getCategory () const
{
	return category;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API ClassDescription::getName () const
{
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API ClassDescription::getSubCategory () const
{
	return subCategory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API ClassDescription::getDescription () const
{
	return description;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ClassDescription::clone (IClassDescription*& desc) const
{
	desc = NEW ClassDescription (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IVersionDescription& CCL_API ClassDescription::getModuleVersion () const
{
	ASSERT (version != nullptr)
	if(version)
		return *version;

	static const VersionDescription emptyVersion;
	return emptyVersion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ClassDescription::getClassAttribute (Variant& value, StringID id) const
{
	if(attributes)
		return attributes->getAttribute (value, id);
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ClassDescription::getClassAttributes (IAttributeList& a) const
{
	if(attributes)
	{
		a.copyFrom (*attributes);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ClassDescription::getLocalizedName (String& name) const
{
	Variant value;
	getClassAttribute (value, Meta::kClassLocalizedName);
	name = value.asString ();
	if(name.isEmpty ())
		name = getName ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ClassDescription::getLocalizedSubCategory (String& subCategory) const
{
	Variant value;
	getClassAttribute (value, Meta::kClassLocalizedSubCategory);
	subCategory = value.asString ();
	if(subCategory.isEmpty ())
		subCategory = getSubCategory ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ClassDescription::getLocalizedDescription (String& description) const
{
	Variant value;
	getClassAttribute (value, Meta::kClassLocalizedDescription);
	description = value.asString ();
	if(description.isEmpty ())
		description = getDescription ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ClassDescription::getClassUrl (IUrl& url) const
{
	url.setProtocol (CCLSTR ("class"));

	String idString;
	classID.toString (idString);
	url.setHostName (idString);

	String pathString;
	pathString << getCategory ();
	pathString << Url::strPathChar;
	pathString << getName ();
	url.setPath (pathString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API ClassDescription::getCodeResourceType () const
{
	return resource ? resource->getType () : CodeResourceType::kNative;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ClassDescription::getClassFlags () const
{
	return classFlags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassDescription::toString (String& string, int flags) const
{
	string = getName ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ClassDescription::getHashCode (int size) const
{
	int result = classID.getHashCode (size);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassDescription::equals (const Object& obj) const
{
	const ClassDescription* desc = ccl_cast<ClassDescription> (&obj);
	return desc ? classID == desc->classID : Object::equals (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassDescription::setResource (ICodeResource* _resource)
{
	take_shared<ICodeResource> (resource, _resource);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassDescription::setVersion (VersionDescription* _version)
{
	take_shared<VersionDescription> (version, _version);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PersistentAttributes& ClassDescription::getAttributes ()
{
	if(!attributes)
		attributes = NEW PersistentAttributes;
	return *attributes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassDescription::fromClassDesc (const ClassDesc& description)
{
	classFlags = description.flags;
	classID = description.classID;
	category = description.category;
	name = description.name;
	subCategory = description.subCategory;
	this->description = description.description;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassDescription::toClassDesc (ClassDesc& description) const
{
	description.flags = classFlags;
	description.classID = classID;
	description.category = category;
	description.name = name;
	description.subCategory = subCategory;
	description.description = this->description;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassDescription::isSingleton () const
{
	return get_flag<int> (classFlags, ClassDesc::kSingleton);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ClassDescription::createInstance (UIDRef iid, void** obj)
{
	tresult result = kResultClassNotFound;
	IClassFactory* factory = resource ? resource->getClassFactory () : nullptr;
	if(factory)
		result = factory->createInstance (classID, iid, obj);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassDescription::loadAttributes (const IAttributeList& a)
{
	AttributeReadAccessor reader (a);

	classFlags = reader.getInt ("classFlags");
	
	String cidString = reader.getString ("classID");
	if(!classID.fromString (cidString))
		return false;

	name = reader.getString ("name");
	category = reader.getString ("category");
	if(name.isEmpty () || category.isEmpty ())
		return false;

	subCategory = reader.getString ("subCategory");
	description = reader.getString ("description");

	safe_release (attributes);
	if(IUnknown* unk = reader.getUnknown ("attributes"))
	{
		// cast works if loaded within this module
		if(PersistentAttributes* internalAttr = unknown_cast<PersistentAttributes> (unk))
			take_shared (attributes, internalAttr);
		// import from foreign module otherwise
		else if(UnknownPtr<IAttributeList> externalAttr = unk)
		{
			attributes = NEW PersistentAttributes;
			attributes->copyFrom (*externalAttr);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassDescription::saveAttributes (IAttributeList& a) const
{
	AttributeAccessor writer (a);

	int flagsToSave = classFlags & ~ClassDesc::kDiscardable;
	if(flagsToSave != 0)
		writer.set ("classFlags", flagsToSave);
	
	String cidString;
	classID.toString (cidString);
	writer.set ("classID", cidString);

	writer.set ("category", category);
	writer.set ("name", name);

	if(!subCategory.isEmpty ())
		writer.set ("subCategory", subCategory);
	if(!description.isEmpty ())
		writer.set ("description", description);

	if(attributes && !attributes->isEmpty ())
	{
		// cast works if saved within this module
		if(Attributes* internalAttr = unknown_cast<Attributes> (&a))
			internalAttr->set ("attributes", attributes, Attributes::kShare);
		// export to foreign module otherwise
		else
		{
			AutoPtr<IAttributeList> externalAttr = writer.newPersistentAttributes ();
			externalAttr->copyFrom (*attributes);
			writer.set ("attributes", externalAttr, IAttributeList::kShare);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassDescription::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	return loadAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ClassDescription::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	return saveAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ClassDescription::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "classID")
	{
		String s;
		classID.toString (s);
		var = s;
		var.share ();
		return true;
	}
	else if(propertyId == "category")
	{
		var = category;
		return true;
	}
	else if(propertyId == "name")
	{
		var = name;
		return true;
	}
	else if(propertyId == "subCategory")
	{
		var = subCategory;
		return true;
	}
	else if(propertyId == "description")
	{
		var = description;
		return true;
	}
	else if(propertyId == "localizedName" || propertyId == "localizedDescription")
	{
		String localizedString;
		if(propertyId == "localizedName")
			getLocalizedName (localizedString);
		else
			getLocalizedDescription (localizedString);
		var = localizedString;
		var.share ();
		return true;
	}
	else if(propertyId.startsWith ("module"))
	{
		String string;
		if(propertyId == "moduleName")
			string = getModuleVersion ().getName ();
		else if(propertyId == "moduleVersion")
			string = getModuleVersion ().getVersion ();
		else if(propertyId == "moduleVendor")
			string = getModuleVersion ().getVendor ();
		else if(propertyId == "moduleCopyright")
			string = getModuleVersion ().getCopyright ();
		else if(propertyId == "moduleUrl")
			string = getModuleVersion ().getUrl ();

		var = string;
		var.share ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ClassDescription::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "classID")
	{
		Boxed::UID* interfaceUid = unknown_cast<Boxed::UID> (var.asUnknown ());
		if(!interfaceUid)
			return false;
		classID = *interfaceUid;
		return true;
	}
	else if(propertyId == "category")
	{
		category = var.asString ();
		return true;
	}
	else if(propertyId == "name")
	{
		name = var.asString ();
		return true;
	}
	else if(propertyId == "subCategory")
	{
		subCategory = var.asString ();
		return true;
	}
	else if(propertyId == "description")
	{
		description = var.asString ();
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ClassDescription)
	DEFINE_METHOD_NAME ("getClassAttribute")
END_METHOD_NAMES (ClassDescription)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ClassDescription::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "getClassAttribute")
	{
		MutableCString id (msg[0].asString ());
		getClassAttribute (returnValue, id);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// VersionDescription
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (VersionDescription, Object, "Version")
DEFINE_CLASS_NAMESPACE (VersionDescription, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

VersionDescription::VersionDescription ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

VersionDescription::VersionDescription (const VersionDesc& description)
{
	fromVersionDesc (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VersionDescription::fromVersionDesc (const VersionDesc& description)
{
	name = description.name;
	version = description.version;
	vendor = description.vendor;
	copyright = description.copyright;
	url = description.url;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VersionDescription::toVersionDesc (VersionDesc& description) const
{
	description.name = name;
	description.version = version;
	description.vendor = vendor;
	description.copyright = copyright;
	description.url = url;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API VersionDescription::getName () const
{
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API VersionDescription::getVersion () const
{
	return version;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API VersionDescription::getVendor () const
{
	return vendor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API VersionDescription::getCopyright () const
{
	return copyright;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API VersionDescription::getUrl () const
{
	return url;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VersionDescription::loadAttributes (const IAttributeList& a)
{
	AttributeReadAccessor reader (a);
	name = reader.getString ("name");
	version = reader.getString ("version");
	vendor = reader.getString ("vendor");
	copyright = reader.getString ("copyright");
	url = reader.getString ("url");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VersionDescription::saveAttributes (IAttributeList& a) const
{
	AttributeAccessor writer (a);
	if(!name.isEmpty ())
		writer.set ("name", name);
	if(!version.isEmpty ())
		writer.set ("version", version);
	if(!vendor.isEmpty ())
		writer.set ("vendor", vendor);
	if(!copyright.isEmpty ())
		writer.set ("copyright", copyright);
	if(!url.isEmpty ())
		writer.set ("url", url);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VersionDescription::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	return loadAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool VersionDescription::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	return saveAttributes (a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VersionDescription::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "name")
	{
		var = name;
		return true;
	}
	else if(propertyId == "version")
	{
		var = version;
		return true;
	}
	else if(propertyId == "vendor")
	{
		var = vendor;
		return true;
	}
	else if(propertyId == "copyright")
	{
		var = copyright;
		return true;
	}
	else if(propertyId == "url")
	{
		var = url;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API VersionDescription::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "name")
	{
		name = var.asString ();
		return true;
	}
	else if(propertyId == "version")
	{
		version = var.asString ();
		return true;
	}
	else if(propertyId == "vendor")
	{
		vendor = var.asString ();
		return true;
	}
	else if(propertyId == "copyright")
	{
		copyright = var.asString ();
		return true;
	}
	else if(propertyId == "url")
	{
		url = var.asString ();
		return true;
	}
	return SuperClass::setProperty (propertyId, var);
}

//************************************************************************************************
// ClassCollection
//************************************************************************************************

ClassCollection::ClassCollection (bool cleanup)
: discardable (false)
{
	objectCleanup (cleanup);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ClassCollection::collect (IClassFactory& factory, StringID _language)
{
	if(UnknownPtr<IClassFactoryUpdate> factoryUpdate = &factory)
	{
		factoryUpdate->updateClasses ();

		// preserve discardable flag when currently no classes exported
		if(factoryUpdate->hasDiscardableClasses ())
			discardable = true;
	}

	VersionDesc versionDesc;
	factory.getVersion (versionDesc);
	version = NEW VersionDescription (versionDesc);

	StringID language = !_language.isEmpty () ? _language : System::GetLocaleManager ().getLanguage ();

	int numClasses = factory.getNumClasses ();
	for(int i = 0; i < numClasses; i++)
	{
		ClassDesc classDesc;
		if(!factory.getClassDescription (classDesc, i))
			continue;
		if(!classDesc.classID.isValid ())
			continue;

		if(classDesc.flags & ClassDesc::kDiscardable)
			discardable = true;

		ClassDescription* desc = NEW ClassDescription (classDesc);
		desc->setVersion (version);
		factory.getClassAttributes (desc->getAttributes (), classDesc.classID, language);

		add (desc);
	}
}

//************************************************************************************************
// PlugInManager
//************************************************************************************************

static const CString kDeferUnload (CSTR ("deferUnload"));
DEFINE_STRINGID_MEMBER_ (PlugInManager, kVersionID, "version")
DEFINE_STRINGID_MEMBER_ (PlugInManager, kClassesID, "Classes")
DEFINE_STRINGID_MEMBER_ (PlugInManager, kNumClassesID, "numClasses")
DEFINE_STRINGID_MEMBER_ (PlugInManager, kDiscardableID, "discardable")

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (PlugInManager, PlugInCollection)
DEFINE_CLASS_NAMESPACE (PlugInManager, NAMESPACE_CCL)
DEFINE_SINGLETON (PlugInManager)
const String PlugInManager::kBlocklistProtocol ("blocklist");

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInManager::PlugInManager ()
: PlugInCollection (CCLSTR ("Plugins"), CCLSTR ("PluginBlocklist")),
  settings (nullptr),
  currentLoader (nullptr),
  currentPathFilter (nullptr),
  keepDiscardable (false)
{
	categories.objectCleanup ();
	altClassMisses.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInManager::~PlugInManager ()
{
	cancelSignals ();

	ASSERT (instances.isEmpty () == true)
	ASSERT (runtimeList.isEmpty () == true)
	ASSERT (hookList.isEmpty () == true)
	ASSERT (filterList.isEmpty () == true)

	safe_release (settings);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API PlugInManager::getExtension (StringID id)
{
	if(id == ICoreCodeLoader::kExtensionID)
		return ccl_as_unknown (CoreCodeLoader::instance ());
	if(id == IScriptCodeLoader::kExtensionID)
		return ccl_as_unknown (ScriptCodeLoader::instance ());
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PlugInManager::HookList& PlugInManager::getHookList () const
{
	return hookList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManager::deferUnload (CodeModule* module)
{
	{
		Threading::ScopedLock scopedLock (lock);
		if(!unloadList.contains (module))
			unloadList.add (module);
	}

	(NEW Message (kDeferUnload))->post (this, 1000);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::unloadUnusedModules ()
{
	ASSERT (System::IsInMainThread ())
	if(!System::IsInMainThread ())
		return kResultWrongThread;

	Threading::ScopedLock scopedLock (lock);
	if(unloadList.isEmpty ())
		return kResultOk;

	ForEach (unloadList, CodeModule, module)
		CCL_PRINTLN (String () << "Unloading Module deferred: " << module->getPath ().getPath ())
		module->checkUnload (true);
	EndFor
	unloadList.removeAll ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInManager::terminate ()
{
	SignalSource (Signals::kPlugIns).signal (Message (Signals::kTerminatePlugIns));

	ASSERT (System::IsInMainThread ())
	unloadUnusedModules ();

	#if RELEASE
	modules.objectCleanup (false); // avoid crash on exit
	#endif

	ForEach (modules, CodeModule, module)
		module->forceUnload ();
	EndFor

	modules.removeAll ();

	if(settings)
		settings->flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInManager::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kDeferUnload)
	{
		unloadUnusedModules ();
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassCategory* PlugInManager::lookupCategory (StringRef name, bool create)
{
	ClassCategory* c = (ClassCategory*)categories.findEqual (ClassCategory (name));
	if(!c && create)
	{
		c = NEW ClassCategory (name);
		categories.add (c);
	}
	return c;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassDescription* PlugInManager::lookupClass (VariantRef var)
{
	// 1) try UID directly
	if(UnknownPtr<IBoxedUID> boxedID = var.asUnknown ())
	{
		UID cid;
		boxedID->copyTo (cid);
		return lookupClass (cid);
	}

	String string = var.asString ();
	if(string.isEmpty ())
		return nullptr;

	// 2) try UID as string
	UID cid;
	if(cid.fromString (string))
		return lookupClass (cid);

	// 2) try class name
	return lookupClass (string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassDescription* PlugInManager::lookupClass (UIDRef cid)
{
#if 1
	// hash table lookup
	ClassDescription temp (cid);
	return (ClassDescription*)classIdTable.lookup (temp);
#else
	// linear search
	ForEach (categories, ClassCategory, c)
		IterForEach (c->newIterator (), ClassDescription, desc)
			if(desc->getClassID () == cid)
				return desc;
		EndFor
	EndFor
	return 0;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ClassDescription* PlugInManager::lookupClass (StringRef className)
{
	// split category and name
	static const String colon (CCLSTR (":"));
	int index = className.lastIndex (colon);
	ASSERT (index != -1)
	String name = className.subString (index+1);
	String category = className.subString (0, index);

	// check if subcategory present
	String subCategory;
	index = category.index (colon);
	if(index != -1)
	{
		subCategory = category.subString (index+1);
		category.truncate (index);
	}

	ClassCategory* c = lookupCategory (category, false);
	if(c)
	{
		IterForEach (c->newIterator (), ClassDescription, desc)
			if(desc->getName () == name)
			{
				if(!subCategory.isEmpty () && desc->getSubCategory () != subCategory)
					continue;
				return desc;
			}
		EndFor
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInManager::addClass (ClassDescription* desc)
{
	CCL_PRINTLN (String () << "class: " << desc->getName ())

	// check if class already exists
	ClassDescription* existingClass = lookupClass (desc->getClassID ());
	if(existingClass)
	{
		if(desc->getName () != existingClass->getName ()) // don't warn when overwriting classes
		{
			CCL_WARN ("UID conflict on class registration: %s (%s %s) => used by: %s (%s %s)\n",
																		  MutableCString (desc->getName ()).str (),
																		  MutableCString (desc->getCategory ()).str (),
																		  MutableCString (desc->getSubCategory ()).str (),
																		  MutableCString (existingClass->getName ()).str (),
																		  MutableCString (existingClass->getCategory ()).str (),
																		  MutableCString (existingClass->getSubCategory ()).str ())
		}

		desc->release ();
		return false;
	}

	// check filters
	ListForEach (filterList, IObjectFilter*, filter)
		if(!filter->matches (desc->asUnknown ()))
		{
			desc->release ();
			return false;
		}
	EndFor

	lookupCategory (desc->getCategory (), true)->addClass (desc);
	classIdTable.add (desc);
	altClassMisses.removeAll ();
	altClassMissTable.removeAll ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static ICodeResourceLoader* getBuiltInLoader (StringID type)
{
	if(type == CodeResourceType::kScript)
		return &ScriptCodeLoader::instance ();
	if(type == CodeResourceType::kCore)
		return &CoreCodeLoader::instance ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PlugInManager::scanFolder (UrlRef url, StringID type, int options, IProgressNotify* progress, IUrlFilter* filter)
{
	ScopedVar<ICodeResourceLoader*> ls (currentLoader, getBuiltInLoader (type));
	ScopedVar<IUrlFilter*> fs (currentPathFilter, filter);
	ScopedVar<bool> ds (keepDiscardable, (options & PlugScanOption::kKeepDiscardable) != 0);

	bool recursive = (options & PlugScanOption::kRecursive) != 0;
	return SuperClass::scanFolder (url, recursive, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PlugInManager::scanFolder (UrlRef url, ICodeResourceLoader* loader, int options, IProgressNotify* progress, IUrlFilter* filter)
{
	ASSERT (loader != nullptr)
	ScopedVar<ICodeResourceLoader*> ls (currentLoader, loader);
	ScopedVar<IUrlFilter*> fs (currentPathFilter, filter);
	ScopedVar<bool> ds (keepDiscardable, (options & PlugScanOption::kKeepDiscardable) != 0);

	bool recursive = (options & PlugScanOption::kRecursive) != 0;
	return SuperClass::scanFolder (url, recursive, progress);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::unregisterLoader (ICodeResourceLoader* loader)
{
	ASSERT (loader != nullptr)
	if(loader == nullptr)
		return kResultInvalidArgument;

	ObjectList affectedModules;

	ForEach (categories, ClassCategory, c)
		Iterator* iter = c->newIterator ();
		IterForEach (iter, ClassDescription, desc)
			CodeModule* module = unknown_cast<CodeModule> (desc->resource);
			if(module && module->matches (loader))
			{
				if(!affectedModules.contains (module))
					affectedModules.add (module);

				c->removeClass (desc);
				classIdTable.remove (desc);
				desc->setResource (nullptr); // unlink from resource

				uint32 retainCount = desc->release ();
				ASSERT (retainCount == 0)

				iter->previous (); // (problem with array iterator when removing items)
			}
		EndFor
	EndFor

	ForEach (modules, CodeModule, module)
		if(module->matches (loader))
		{
			// there might be still modules in the list not catched by the loop above!
			if(!affectedModules.contains (module))
				affectedModules.add (module);
		}
	EndFor

	{
		// cleanup unload list
		Threading::ScopedLock scopedLock (lock);
		ForEach (unloadList, CodeModule, module)
			if(affectedModules.contains (module))
				unloadList.remove (module);
		EndFor
	}

	ForEach (affectedModules, CodeModule, module)
		modules.remove (module);
		uint32 retainCount = module->release ();
		ASSERT (retainCount == 0)
	EndFor
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CodeResource* PlugInManager::findInRuntimeList (IClassFactory* factory) const
{
	ForEach (runtimeList, CodeResource, r)
		if(r->getClassFactory () == factory)
			return r;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::registerFactory (IClassFactory* factory)
{
	ASSERT (factory != nullptr)
	if(factory == nullptr)
		return kResultInvalidArgument;

	ASSERT (findInRuntimeList (factory) == nullptr)
	CodeResource* resource = NEW CodeResource (factory);
	runtimeList.add (resource);

	ClassCollection classList (false);
	classList.collect (*factory);

	// Note: Classes are registered temporarily at runtime,
	// no need to add descriptions to Settings!

	ForEach (classList, ClassDescription, desc)
		desc->setResource (resource);
		addClass (desc);
	EndFor
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::unregisterFactory (IClassFactory* factory)
{
	CodeResource* resource = findInRuntimeList (factory);
	ASSERT (resource != nullptr)
	if(!resource)
		return kResultInvalidArgument;

	ForEach (categories, ClassCategory, c)
		Iterator* iter = c->newIterator ();
		IterForEach (iter, ClassDescription, desc)
			if(desc->resource == resource)
			{
				c->removeClass (desc);
				classIdTable.remove (desc);
				desc->setResource (nullptr); // unlink from resource

				uint32 retainCount = desc->release ();
				ASSERT (retainCount == 0)

				iter->previous (); // (problem with array iterator when removing items)
			}
		EndFor
	EndFor

	runtimeList.remove (resource);
	resource->release ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::updateFactory (IClassFactory* factory)
{
	CodeResource* resource = findInRuntimeList (factory);
	ASSERT (resource != nullptr)
	if(!resource)
		return kResultInvalidArgument;

	StringList dirtyCategories;

	// collect new classes from factory
	ClassCollection classList (false);
	classList.collect (*factory);

	// check previously registered classes
	ForEach (categories, ClassCategory, c)
		Iterator* iter = c->newIterator ();
		IterForEach (iter, ClassDescription, desc)
			if(desc->resource == resource)
			{
				ClassDescription* newDesc = (ClassDescription*)classList.findEqual (*desc);
				if(newDesc == nullptr)
				{
					// 1) class has been removed
					dirtyCategories.addOnce (desc->getCategory ());

					c->removeClass (desc);
					classIdTable.remove (desc);
					desc->setResource (nullptr); // unlink from resource

					uint32 retainCount = desc->release ();
					ASSERT (retainCount == 0) // if reference count is not zero here, instances of this class might still exist!

					iter->previous (); // (problem with array iterator when removing items)
				}
				else
				{
					// 2) class is already registered
					classList.remove (newDesc);
					newDesc->release ();
				}
			}
		EndFor
	EndFor

	// register new classes
	ForEach (classList, ClassDescription, desc)
		dirtyCategories.addOnce (desc->getCategory ());
		desc->setResource (resource);
		addClass (desc);
	EndFor

	ForEach (dirtyCategories, Boxed::String, str)
		String category (*str);
		SignalSource (Signals::kPlugIns).signal (Message (Signals::kClassCategoryChanged, category));
	EndFor

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::storeFile (IAttributeList& classData, UrlRef url, ICodeResourceLoader* loader,
										  StringID language, IObjectFilter* classFilter)
{
	ScopedVar<ICodeResourceLoader*> ls (currentLoader, loader);

	AutoPtr<CodeModule> module = (CodeModule*)createModule (url);
	ASSERT (module != nullptr)
	Module::Unloader unloader (*module);

	IClassFactory* factory = module->getClassFactory ();
	ASSERT (factory != nullptr)
	if(factory == nullptr)
		return kResultFailed;

	ClassCollection classList (true);
	classList.collect (*factory, language);

	AttributeAccessor writer (classData);
	if(classList.isDiscardable ())
		writer.set (kDiscardableID, true);

	AutoPtr<IAttributeList> versionAttr = writer.newAttributes ();
	classList.getVersion ()->saveAttributes (*versionAttr);
	writer.set (kVersionID, versionAttr, IAttributeList::kShare);

	ArrayForEach (classList, ClassDescription, description)
		if(classFilter && !classFilter->matches (description->asUnknown ()))
			continue;

		AutoPtr<IAttributeList> classAttr = writer.newAttributes ();
		description->saveAttributes (*classAttr);
		writer.queue (kClassesID, classAttr, IAttributeList::kShare);
	EndFor
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::restoreFile (UrlRef url, ICodeResourceLoader* loader, int options,
											IAttributeList* classData, IObjectFilter* classFilter,
											tbool* fileIsOnBlocklist)
{
	ScopedVar<ICodeResourceLoader*> ls (currentLoader, loader);
	ScopedVar<bool> ds (keepDiscardable, (options & PlugScanOption::kKeepDiscardable) != 0);

	if(!isModule (url))
		return kResultInvalidArgument;

	AutoPtr<Module> module = createModule (url);
	ASSERT (module != nullptr)

	String settingsID;
	getSettingsID (settingsID, module);
	DateTime moduleTime;
	getModuleTime (moduleTime, module);

	if(modules.contains (*module))
	{
		DateTime savedTime;
		if(restoreModuleTime (savedTime, settingsID) && savedTime == moduleTime)
			return kResultAlreadyExists;
	}

	bool overrideDiscardable = false;
	if(classData != nullptr)
	{
		// inject class information scanned externally
		// note that this is always like keepDiscardable = true
		Attributes& a = getSettings ().getAttributes (settingsID);
		a.removeAll ();
		storeModuleTime (settingsID, moduleTime);

		AttributeReadAccessor reader (*classData);
		AutoPtr<VersionDescription> version = NEW VersionDescription;
		if(UnknownPtr<IAttributeList> versionAttr = reader.getUnknown (kVersionID))
			version->loadAttributes (*versionAttr);
		a.set (kVersionID, version, Attributes::kShare);

		int classCount = 0;
		if(UnknownPtr<IContainer> classQueue = reader.getUnknown (kClassesID))
		{
			ForEachUnknown (*classQueue, unk)
				if(UnknownPtr<IAttribute> queueItem = unk)
					if(UnknownPtr<IAttributeList> classAttr = queueItem->getValue ().asUnknown ())
					{
						AutoPtr<ClassDescription> description = NEW ClassDescription;
						if(description->loadAttributes (*classAttr))
						{
							if(classFilter && !classFilter->matches (description->asUnknown ()))
								continue;

							classCount++;
							a.queue (kClassesID, description, Attributes::kShare);
						}
					}
			EndFor
		}
		a.set (kNumClassesID, classCount);

		// Assume that plug-ins who's validity is confirmed but who do not export classes at this time
		// might do so sometime later -> mark as discardable.
		if(classCount == 0)
		{
			bool validityConfirmed = (options & PlugScanOption::kValidityConfirmed) != 0;
			if(validityConfirmed == true)
				overrideDiscardable = true;
		}

		// remove from blocklist (if enabled)
		ASSERT (fileIsOnBlocklist == nullptr)
		removeFromBlocklist (settingsID);
	}
	#if DEBUG
	else
		ASSERT (classFilter == nullptr) // no class filter expected without data
	#endif

	if(restoreModule (settingsID, moduleTime, module) == false)
	{
		// check if caller wants to use the blocklist
		if(fileIsOnBlocklist != nullptr)
		{
			String name;
			module->getPath ().getName (name, false);
			*fileIsOnBlocklist = checkBlocklist (settingsID, name) == false;
		}

		return kResultClassNotFound;
	}
	else
	{
		// preserve discardable flag
		if((classData && AttributeReadAccessor (*classData).getBool (kDiscardableID)) || overrideDiscardable)
			getSettings ().getAttributes (settingsID).set (kDiscardableID, true);
	}

	if(!modules.contains (module))
		modules.add (module.detach ());
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInManager::isDiscardable (UrlRef url)
{
	String settingsID;
	getSettingsID (settingsID, url);

	Attributes& a = getSettings ().getAttributes (settingsID);
	return a.getBool (kDiscardableID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IPlugInClassList& CCL_API PlugInManager::getClassList (StringRef category)
{
	ClassCategory* c = lookupCategory (category, false);
	if(c)
		return *c;

	static const ClassCategory emptyCategory;
	return emptyCategory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IClassDescription* CCL_API PlugInManager::getClassDescription (UIDRef cid)
{
	return lookupClass (cid);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IClassDescription* CCL_API PlugInManager::getClassDescription (StringRef className)
{
	return lookupClass (className);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IClassDescription* CCL_API PlugInManager::getClassDescription (UrlRef url)
{
	if(url.getProtocol () == CCLSTR ("class"))
	{
		UID cid;
		if(cid.fromString (url.getHostName ()))
			return getClassDescription (cid);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IClassDescription* CCL_API PlugInManager::getMetaClassDescription (UIDRef cid)
{
	ClassDescription* desc = lookupClass (cid);
	if(desc)
	{
		Variant v;
		UID metaClassID;
		if(desc->getClassAttribute (v, Meta::kMetaClassID) && metaClassID.fromString (v.asString ()))
			return lookupClass (metaClassID);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IClassDescription* CCL_API PlugInManager::getAlternativeClass (UIDRef cid)
{
	#if DEBUG
	auto logAltClass = [this] (ClassDescription* altClass, UIDRef cid)
	{
		if(altClass)
			if(auto* origClass = lookupClass (cid))
			{
				Debugger::println (String ("getAlternativeClass: ") << 
						UIDString (origClass->getClassID ()) << " -> " << 
						UIDString (altClass->getClassID ()) << "  " << 
						origClass->getName () << " -> " << 
						altClass->getName ());
			}
	};
	#endif

	// 1) check if we already know that there are no alternative classes
	ClassDescription temp (cid);
	if(Object* result = altClassMissTable.lookup (temp))
		return nullptr;

	StringID language = System::GetLocaleManager ().getLanguage ();

	// 2) try to obtain alternative class from factories registered at runtime
	ForEach (runtimeList, CodeResource, r)
		IClassFactory* factory = r->getClassFactory ();
		Attributes attr;
		if(factory->getClassAttributes (attr, cid, language))
		{
			String altString = attr.getString (Meta::kAlternativeClassID);
			if(!altString.isEmpty ())
			{
				UID altClassId;
				altClassId.fromString (altString);
				
				ClassDescription* altClass = lookupClass (altClassId);
				#if DEBUG_LOG
				logAltClass (altClass, cid);
				#endif
				return altClass;
			}
		}
	EndFor
	
	// 3) search all registered classes for compatibility ids
	ForEach (categories, ClassCategory, c)
		IterForEach (c->newIterator (), ClassDescription, desc)
			Variant classAttr;
			if(!desc->getClassAttribute (classAttr, Meta::kAlternativeClassID))
				continue;

			String compatibilityString = classAttr.asString ();
			if(!compatibilityString.isEmpty ())
			{
				// some classes can have multiple compatibility ids separated by ","
				ForEachStringToken (compatibilityString, ",", compatibilitySubString)
					UID compatibilityId;
					compatibilityId.fromString (compatibilitySubString);
					if(compatibilityId.equals (cid))
					{
						#if DEBUG_LOG
						logAltClass (desc, cid);
						#endif
						return desc;
					}
				EndFor
			}
		EndFor
	EndFor

	ClassDescription* desc = NEW ClassDescription (cid);
	altClassMisses.add (desc);
	altClassMissTable.add (desc);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::getLastModifiedTime (DateTime& lastModified, UrlRef url)
{
	String settingsID;
	getSettingsID (settingsID, url);

	if(!restoreModuleTime (lastModified, settingsID))
		return kResultFailed;
	if(lastModified == DateTime ())
		return kResultFailed;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ISearcher* CCL_API PlugInManager::createSearcher (ISearchDescription& description)
{
	return NEW ClassSearcher (description);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::createInstance (UIDRef cid, UIDRef iid, void** obj)
{
	tresult result = kResultClassNotFound;
	ClassDescription* desc = lookupClass (cid);
	if(desc)
		result = createInstance (*desc, iid, obj);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::createInstance (StringRef className, UIDRef iid, void** obj)
{
	tresult result = kResultClassNotFound;
	ClassDescription* desc = lookupClass (className);
	if(desc)
		result = createInstance (*desc, iid, obj);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult PlugInManager::createInstance (ClassDescription& desc, UIDRef iid, void** obj)
{
	Threading::ScopedLock scopedLock (lock);

	tresult result = desc.createInstance (iid, obj);
	if(result == kResultOk)
	{
		IUnknown* u = (IUnknown*)(*obj); // this cast must work!

		CodeModule* module = unknown_cast<CodeModule> (desc.resource);
		if(module)
			module->addInstance (u); // increment instance counter

		// store factory token
		UnknownPtr<IPluginInstance> instance (u);
		if(instance)
			instance->setFactoryToken (desc.asUnknown ());
		else
		{
			UnknownPtr<IObject> iObject (u);
			if(!iObject || (iObject && !iObject->getTypeInfo ().getClassID ().isValid ()))
				instances.add (NEW InstanceAssoc (*obj, &desc));
		}
	}
	else
	{
		// unload module if no instances exist...
		CodeModule* module = unknown_cast<CodeModule> (desc.resource);
		if(module)
			module->checkUnload ();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInManager::releaseInstance (IUnknown* obj)
{
	if(!obj)
		return;

	Threading::ScopedLock scopedLock (lock);

	InstanceAssoc* assoc = nullptr;
	SharedPtr<ClassDescription> desc; // if description is shared by instance, we have to keep it alive here

	Object* data = getInstanceData (obj);
	desc = ccl_cast<ClassDescription> (data);
	if(!desc)
	{
		assoc = ccl_cast<InstanceAssoc> (data);
		desc = assoc ? assoc->desc : nullptr;
	}

	ASSERT (desc != nullptr)
	
	int refCount = 0;
	CodeModule* module = desc ? unknown_cast<CodeModule> (desc->resource) : nullptr;
	if(module)
	{
		refCount = module->releaseInstance (obj);

		if(refCount > 0 && module->getType () == CodeResourceType::kScript)
		{
			String fileName;
			module->getPath ().getName (fileName);
			String warning;
			warning.appendFormat (XSTR (DeletedScriptObjectIsStillReferenced), fileName);
			System::GetLogger ().reportEvent (Alert::Event (warning, Alert::kWarning));
		}
	}
	else
		refCount = obj->release ();

	ASSERT (refCount == 0 || (desc && desc->isSingleton () && refCount == 1))

	if(assoc && refCount == 0) // hmm???
	{
		instances.remove (assoc);
		assoc->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IClassDescription* CCL_API PlugInManager::getInstanceClass (IUnknown* obj)
{
	Object* data = getInstanceData (obj);

	ClassDescription* desc = ccl_cast<ClassDescription> (data);
	if(desc)
		return desc;

	InstanceAssoc* assoc = ccl_cast<InstanceAssoc> (data);
	if(assoc)
		return assoc->desc;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* PlugInManager::getInstanceData (IUnknown* obj)
{
	Object* data = nullptr;
	UnknownPtr<IPluginInstance> instance (obj);
	if(instance)
		data = unknown_cast<ClassDescription> (instance->getFactoryToken ());
	else
	{
		UnknownPtr<IObject> iObject (obj);
		if(iObject && iObject->getTypeInfo ().getClassID ().isValid ())
			data = lookupClass (iObject->getTypeInfo ().getClassID ());
		else
			data = instances.lookup (InstanceAssoc (obj));
	}
	return data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPluginMetaClass* CCL_API PlugInManager::createMetaClass (UIDRef cid)
{
	IPluginMetaClass* metaClass = nullptr;
	ClassDescription* desc = unknown_cast<ClassDescription> (getMetaClassDescription (cid));
	if(desc)
		createInstance (*desc, ccl_iid<IPluginMetaClass> (), (void**)&metaClass);
	return metaClass;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings& PlugInManager::getSettings ()
{
	if(settings == nullptr)
	{
		// Note: Class attributes are language-dependent!
		settings = NEW XmlSettings (XmlSettings::getNameWithLanguage (name));
		settings->isPlatformSpecific (true);
		settings->isAutoSaveEnabled (true);
		settings->enableSignals (true);
		settings->restore ();
	}
	return *settings;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInManager::saveSettings ()
{
	getSettings ().flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInManager::removeSettings (tbool anyLanguage)
{
	ASSERT (settings == nullptr) // must be called before classes are scanned!
	if(anyLanguage)
		XmlSettings::removeSettings (name, true, true);
	else
		XmlSettings::removeSettings (XmlSettings::getNameWithLanguage (name), false, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInManager::getCurrentFolder (IUrl& _currentFolder)
{
	if(currentFolder)
	{
		_currentFolder.assign (*currentFolder);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInManager::setCurrentFolder (const IUrl* _currentFolder)
{
	currentFolder = _currentFolder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::storeFileList (ICodeResourceLoader* loader)
{
	if(loader == nullptr)
		return kResultInvalidArgument;

	String id (loader->getType ());
	id << ".FileList";

	struct LoaderFilter: ModuleFilter
	{
		ICodeResourceLoader* loader;
		LoaderFilter (ICodeResourceLoader* loader): loader (loader) {}
		bool matches (const Module* module) const override { return ((CodeModule*)module)->matches (loader); }
	} filter (loader);

	saveModules (id, &filter);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::restoreFileList (ICodeResourceLoader* loader,  int options)
{
	if(loader == nullptr)
		return kResultInvalidArgument;

	ScopedVar<ICodeResourceLoader*> ls (currentLoader, loader);
	ScopedVar<bool> ds (keepDiscardable, (options & PlugScanOption::kKeepDiscardable) != 0);

	String id (loader->getType ());
	id << ".FileList";

	if(restoreModules (id) == false)
		return kResultClassNotFound;
	else
		return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInManager::isModule (UrlRef url) const
{
	if(currentPathFilter && !currentPathFilter->matches (url))
		return false;

	if(currentLoader)
		return currentLoader->isCodeResource (url) != 0;
	else
		return SuperClass::isModule (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Module* PlugInManager::createModule (UrlRef url) const
{
	return NEW CodeModule (url, currentLoader);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInManager::getModuleTime (DateTime& modifiedTime, Module* _module)
{
	CodeModule* module = (CodeModule*)_module;

	// force reload of unpacked scripts
	if(module->getType () == CodeResourceType::kScript)
		if(module->getPath ().isFolder ())
		{
			DateTime now;
			System::GetSystem ().getLocalTime (now);
			modifiedTime = now;
			return;
		}

	SuperClass::getModuleTime (modifiedTime, module);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInManager::restoreModuleInfo (StringRef settingsID, Module* _module)
{
	CodeModule* module = (CodeModule*)_module;

	Attributes& a = getSettings ().getAttributes (settingsID);

	// check if classes are discardable (have to be always rescanned)
	if(keepDiscardable == false)
	{
		bool discardable = a.getBool (kDiscardableID);
		if(discardable)
			return false;
	}

	VersionDescription* version = a.getObject<VersionDescription> (kVersionID);
	if(!version)
		return false;

	ObjectList restoredClasses;
	restoredClasses.objectCleanup (true);
	ClassDescription* desc;
	while((desc = a.unqueueObject<ClassDescription> (kClassesID)) != nullptr)
		restoredClasses.add (desc);

	// our only safety check...
	if(!a.contains (kNumClassesID))
		return false;

	int numSavedClasses = a.getInt (kNumClassesID);
	if(restoredClasses.count () != numSavedClasses)
		return false;

	ForEach (restoredClasses, ClassDescription, desc)
		ClassDescription* newDesc = NEW ClassDescription (*desc);
		newDesc->setResource (module);
		newDesc->setVersion (version);

		addClass (newDesc); // this call might fail, but we still want to save the class information!

		a.queue (kClassesID, desc, Attributes::kShare);	// push back to attributes
	EndFor

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInManager::registerModuleInfo (StringRef settingsID, Module* _module)
{
	CodeModule* module = (CodeModule*)_module;
	Module::Unloader unloader (*module);

	// check blocklist before loading module
	String name;
	module->getPath ().getName (name, false);
	if(checkBlocklist (settingsID, name) == false)
		return false;

	IClassFactory* factory = module->getClassFactory ();
	if(!factory)
		return false;

	ClassCollection classList (false);
	classList.collect (*factory);

	Attributes& a = getSettings ().getAttributes (settingsID);
	a.set (kVersionID, classList.getVersion (), Attributes::kShare);
	a.remove (kClassesID); // remove old classes

	ArrayForEach (classList, ClassDescription, desc)
		desc->setResource (module);

		SharedPtr<ClassDescription> keeper (desc);
		addClass (desc); // this call might fail, but we still want to save the class information!

		if(keepDiscardable == true || !classList.isDiscardable ())
			a.queue (kClassesID, desc->clone (), Attributes::kOwns);
	EndFor

	a.set (kNumClassesID, classList.count ());

	if(classList.isDiscardable ())
		a.set (kDiscardableID, true);

	removeFromBlocklist (settingsID);

	return !classList.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::registerStubClass (UIDRef iid,  StringID name, StubConstructor constructor)
{
	bool result = StubFactory::instance ().addClass (iid, name, constructor);
	return result ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::unregisterStubClass (UIDRef iid, StubConstructor constructor)
{
	bool result = StubFactory::instance ().removeClass (iid);
	return result ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::createStubInstance (UIDRef iid, IObject* object, void** stub)
{
	AutoPtr<GenericStub> genericStub = NEW GenericStub (object);
	return genericStub->queryInterface (iid, stub);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInManager::addHook (IUnknown* unknown)
{
	UnknownPtr<ICodeResourceLoaderHook> hook (unknown);
	if(hook.isValid ())
		hookList.append (hook);

	UnknownPtr<IObjectFilter> filter (unknown);
	if(filter.isValid ())
		filterList.append (filter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInManager::removeHook (IUnknown* unknown)
{
	UnknownPtr<ICodeResourceLoaderHook> hook (unknown);
	if(hook.isValid ())
		hookList.remove (hook);

	UnknownPtr<IObjectFilter> filter (unknown);
	if(filter.isValid ())
		filterList.remove (filter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInManager::enableBlocklist (tbool state)
{
	bool wasEnabled = isBlocklistEnabled ();
	SuperClass::enableBlocklist (state != 0);
	return wasEnabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInManager::resetBlocklist ()
{
	SuperClass::resetBlocklist ();
	SignalSource (Signals::kPlugIns).signal (Message (Signals::kResetBlocklistDone));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::addToBlocklist (UrlRef url)
{
	if(!isBlocklistEnabled ())
		return kResultUnexpected;
	
	String settingsID;
	if(url.getProtocol () == kBlocklistProtocol)
		settingsID = url.getPath (); // already hashed, see getBlocklistContent()
	else
		getSettingsID (settingsID, url);
	
	ForEach (categories, ClassCategory, c)
		Iterator* iter = c->newIterator ();
		IterForEach (iter, ClassDescription, desc)
			CodeModule* module = unknown_cast<CodeModule> (desc->resource);
			if(module && module->getPath () == url)
			{
				c->removeClass (desc);
				classIdTable.remove (desc);
				desc->release ();

				iter->previous ();
			}
		EndFor
	EndFor
	
	return SuperClass::addToBlocklist (settingsID) ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::removeFromBlocklist (UrlRef url)
{
	if(!isBlocklistEnabled ())
		return kResultUnexpected;

	String settingsID;
	if(url.getProtocol () == kBlocklistProtocol)
		settingsID = url.getPath (); // already hashed, see getBlocklistContent()
	else
		getSettingsID (settingsID, url);

	return SuperClass::removeFromBlocklist (settingsID) ? kResultOk : kResultFalse;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PlugInManager::getBlocklistContent (IUnknownList& blocklist)
{
	AutoPtr<Settings> settings = createBlocklistCopy ();
	IterForEach (settings->getSections (), Settings::Section, section)
		Url* path = NEW Url;
		path->setProtocol (kBlocklistProtocol);
		path->setPath (section->getPath ());
		blocklist.add (path->asUnknown ());
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PlugInManager::setClassAttribute (const IClassDescription& description, StringID id, VariantRef value)
{
	auto* classDescription = unknown_cast<ClassDescription> (&description);
	ASSERT (classDescription && classIdTable.contains (classDescription))
	if(!classDescription)
		return kResultInvalidArgument;

	classDescription->getAttributes ().setAttribute (id, value);

	if(classDescription->resource)
	{
		// to make this change persistent, we also have to reflect it in the settings
		Url moduleUrl;
		classDescription->resource->getPath (moduleUrl);

		String settingsId;
		getSettingsID (settingsId, moduleUrl);

		Attributes& a = getSettings ().getAttributes (settingsId);
		AttributeQueue* classes = a.getObject<AttributeQueue> (kClassesID);
		if(classes)
		{
			for(auto attrib : iterate_as<Attribute> (*classes))
			{
				ClassDescription* cd = unknown_cast<ClassDescription> (attrib->getValue ());
				if(cd && cd->getClassID () == classDescription->getClassID ())
				{
					cd->getAttributes ().setAttribute (id, value);
					break;
				}
			}
		}
	}
	return kResultOk;		
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PlugInManager::countDiagnosticData () const
{
	return 2;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInManager::getDiagnosticDescription (DiagnosticDescription& description, int index) const
{
	if(index == 0)
	{
		description.categoryFlags = DiagnosticDescription::kPlugInInformation;
		description.fileName = name;
		description.fileType = FileTypes::Xml ();
		return true;
	}
	else if(index == 1)
	{
		description.categoryFlags = DiagnosticDescription::kPlugInInformation;
		description.fileName = blocklistName;
		description.fileType = FileTypes::Xml ();
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API PlugInManager::createDiagnosticData (int index)
{
	if(index == 0)
	{
		XmlSettings settings (XmlSettings::getNameWithLanguage (name));
		settings.isPlatformSpecific (true);
		return System::GetFileSystem ().openStream (settings.getPath (), IStream::kOpenMode | IStream::kShareWrite);
	}
	else if(index == 1)
	{
		XmlSettings blockList (blocklistName);
		blockList.isPlatformSpecific (true);
		return System::GetFileSystem ().openStream (blockList.getPath (), IStream::kOpenMode | IStream::kShareWrite);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (PlugInManager)
	DEFINE_METHOD_ARGR ("createInstance", "className_or_cid: UID | string", "Object")
	DEFINE_METHOD_ARGR ("getClassDescription", "className_or_cid: UID | string", "ClassDescription")
	DEFINE_METHOD_ARGR ("newIterator", "category", "Iterator")
END_METHOD_NAMES (PlugInManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PlugInManager::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "createInstance")
	{
		// TODO: releaseInstance() is not called by scripts!!!!

		AutoPtr<IObject> object;
		if(ClassDescription* desc = lookupClass (msg[0]))
			createInstance (*desc, ccl_iid<IObject> (), object.as_ppv ());
		returnValue.takeShared (object);
		return true;
	}
	else if(msg == "getClassDescription")
	{
		AutoPtr<ClassDescription> descCopy;
		if(ClassDescription* desc = lookupClass (msg[0]))
			descCopy = (ClassDescription*)desc->clone ();
		returnValue.takeShared (static_cast<IObject*> (descCopy));
		return true;
	}
	else if(msg == "newIterator")
	{
		String category = msg[0].asString ();
		AutoPtr<ObjectArray> descriptions = NEW ObjectArray;
		descriptions->objectCleanup (true);
		if(ClassCategory* c = lookupCategory (category, false))
		{
			ForEach (*c, ClassDescription, desc)
				descriptions->add (desc->clone ());
			EndFor
		}
		returnValue.takeShared (AutoPtr<IObject> (NEW HoldingIterator (descriptions, descriptions->newIterator ())));
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ClassSearcher
//************************************************************************************************

ClassSearcher::ClassSearcher (ISearchDescription& searchDescription)
: AbstractSearcher (searchDescription)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ClassSearcher::find (ISearchResultSink& resultSink, IProgressNotify* progress)
{
	String category (searchDescription.getStartPoint ().getPath ());
	ASSERT (!category.isEmpty ())

	struct ClassChecker
	{
		ISearchDescription& searchDescription;

		inline ClassChecker (ISearchDescription& searchDescription)
		: searchDescription (searchDescription)
		{}

		inline bool matches (const IClassDescription& classDescription) const
		{
			// class name
			String name;
			classDescription.getLocalizedName (name);
			if(searchDescription.matchesName (name))
				return true;

			// class vendor
			Variant classsVendor;
			if(classDescription.getClassAttribute (classsVendor, Meta::kClassVendor) && searchDescription.matchesName (classsVendor))
				return true;

			// module vendor
			if(searchDescription.matchesName (classDescription.getModuleVersion ().getVendor ()))
				return true;

			// alternative class name
			Variant alternativeName;
			if(classDescription.getClassAttribute (alternativeName, Meta::kAlternativeClassName) && searchDescription.matchesName (alternativeName))
				return true;

			// (sub)category: only match at beginning, only if at least 3 letters entered (avoid suprising matches)
			if(searchDescription.getSearchTerms ().length () >= 3)
			{
				String subCategory;
				classDescription.getLocalizedSubCategory (subCategory);

				// try to match each part of a category path (e.g. in "(Native)/Modulation")
				bool caseSensitive = searchDescription.getOptions () & ISearchDescription::kMatchCase;
				ForEachStringToken (subCategory, Url::strPathChar, categoryToken)
					if(categoryToken.startsWith (searchDescription.getSearchTerms (), caseSensitive))
						return true;
				EndFor
			}
			return false;
		}
	};

	ClassChecker classChecker (searchDescription);

	ForEachPlugInClass (category, classDescription)
		if(classChecker.matches (classDescription))
		{
			Url* classUrl = NEW Url;
			classDescription.getClassUrl (*classUrl);
			resultSink.addResult (ccl_as_unknown (classUrl));
		}
	EndFor
	return kResultOk;
}

//************************************************************************************************
// CodeModule
//************************************************************************************************

DEFINE_CLASS (CodeModule, Module)
DEFINE_CLASS_NAMESPACE (CodeModule, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

CodeModule::CodeModule (UrlRef path, ICodeResourceLoader* loader)
: Module (path),
  loader (loader),
  resource (nullptr),
  numInstances (0)
{
	if(loader)
		loader->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CodeModule::~CodeModule ()
{
	ASSERT (numInstances == 0)
	destruct ();
	ASSERT (resource == nullptr)

	if(loader)
		loader->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CodeModule::queryInterface (UIDRef iid, void** ptr)
{
	// make IExecutableImage accessible
	if(iid == ccl_iid<IExecutableImage> () && resource)
		return resource->queryInterface (iid, ptr);

	QUERY_INTERFACE (ICodeResource)
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ICodeResourceLoader& CodeModule::getLoader () const
{
	if(loader)
		return *loader;
	return NativeCodeLoader::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CodeModule::matches (ICodeResourceLoader* loader) const
{
	return this->loader == loader;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API CodeModule::getType () const
{
	return getLoader ().getType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IClassFactory* CCL_API CodeModule::getClassFactory ()
{
	if(!isLoaded ())
		load ();

	return resource ? resource->getClassFactory () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* CCL_API CodeModule::getMetaInfo ()
{
	//ASSERT (resource != 0)
	return resource ? resource->getMetaInfo () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API CodeModule::getPath (IUrl& path) const
{
	if(resource == nullptr || !resource->getPath (path))
		path = getPath ();
	return !path.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CodeModule::loadInternal ()
{
	ASSERT (resource == nullptr)
	getLoader ().loadCodeResource (resource, getPath ());
	if(resource != nullptr)
	{
		ListForEach (PlugInManager::instance ().getHookList (), ICodeResourceLoaderHook*, hook)
			hook->onLoad (*resource);
		EndFor
	}
	return resource != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CodeModule::unloadInternal ()
{
	if(resource)
	{
		ListForEach (PlugInManager::instance ().getHookList (), ICodeResourceLoaderHook*, hook)
			hook->onUnload (*resource);
		EndFor

		resource->release (),
		resource = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CodeModule::isLoadedInternal () const
{
	return resource != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CodeModule::addInstance (IUnknown* instance)
{
	AtomicAddInline (numInstances, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CodeModule::releaseInstance (IUnknown* instance)
{
	int refCount = 0;
	if(instance)
		refCount = (int)instance->release ();

	AtomicAddInline (numInstances, -1);
	checkUnload ();

	return refCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CodeModule::checkUnload (bool force)
{
	if(numInstances == 0)
	{
		if(force || getType () == CodeResourceType::kScript)
		{
			ASSERT (System::IsInMainThread ())
			unload ();
		}
		else
			PlugInManager::instance ().deferUnload (this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CodeModule::forceUnload ()
{
	ASSERT (numInstances == 0)
	if(numInstances != 0)
	{
		String name;
		path.getName (name);
		MutableCString cname (name);

		CCL_WARN ("Forcing unload of %s, %d instances left\n", cname.str (), numInstances)
		unload ();
		numInstances = 0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CodeModule::getNumInstances () const
{
	return numInstances;
}
