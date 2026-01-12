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
// Filename    : ccl/system/plugins/scriptcoderesource.cpp
// Description : Script Code Resource
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/plugins/scriptcoderesource.h"
#include "ccl/system/plugins/plugmanager.h"
#include "ccl/system/plugins/scriptingmanager.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/packageinfo.h"
#include "ccl/base/storage/archivehandler.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/text/itranslationtable.h"
#include "ccl/public/system/ifilesystem.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/plugins/pluginst.h"
#include "ccl/public/plugins/iclassfactory.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// ScriptUplink
//************************************************************************************************

class ScriptUplink: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (ScriptUplink, Object)

	ScriptUplink (UrlRef path);
	~ScriptUplink ();

	PROPERTY_POINTER (ScriptCodeResource, codeResource, CodeResource)
	PROPERTY_SHARED_AUTO (PackageInfo, packageInfo, PackageInfo)
	UrlRef getPath () const;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

protected:
	Url* path;
};

//************************************************************************************************
// ScriptClassResource
//************************************************************************************************

class ScriptClassResource: public Object
{
public:
	DECLARE_CLASS (ScriptClassResource, Object)

	PROPERTY_MUTABLE_CSTRING (id, ID)
	PROPERTY_MUTABLE_CSTRING (language, Language)
	PROPERTY_STRING (url, Url)

	// Object
	bool load (const Storage&) override;
	bool save (const Storage&) const override;
};

//************************************************************************************************
// ScriptClassFactory
//************************************************************************************************

class ScriptClassFactory: public Object,
						  public IClassFactory
{
public:
	DECLARE_CLASS (ScriptClassFactory, Object)

	ScriptClassFactory ();

	PROPERTY_SHARED_AUTO (IFileSystem, fileSystem, FileSystem)
	PROPERTY_SHARED_AUTO (ScriptUplink, uplink, Uplink)

	// IClassFactory
	void CCL_API getVersion (VersionDesc& version) const override;
	int CCL_API getNumClasses () const override;
	tbool CCL_API getClassDescription (ClassDesc& description, int index) const override;
	tbool CCL_API getClassAttributes (IAttributeList& attributes, UIDRef cid, StringID language) const override;
	tresult CCL_API createInstance (UIDRef cid, UIDRef iid, void** obj) override;

	// Object
	bool load (const Storage&) override;
	bool save (const Storage&) const override;

	CLASS_INTERFACE (IClassFactory, Object)

protected:
	class ScriptClass: public ClassDescription
	{
	public:
		DECLARE_CLASS (ScriptClass, ClassDescription)

		PROPERTY_STRING (sourceFile, SourceFile)
		PROPERTY_STRING (functionName, FunctionName)

		Attributes& getAttributes () { return attributes; }

		virtual IUnknown* createInstance (ScriptClassFactory& factory);

		// ClassDescription
		bool load (const Storage&) override;
		bool save (const Storage&) const override;

	protected:
		PersistentAttributes attributes;
	};

	class ScriptMetaClass: public ScriptClass
	{
	public:
		DECLARE_CLASS (ScriptMetaClass, ScriptClass)

		ScriptMetaClass ();

		Iterator* getResources () const;

		// ScriptClass
		IUnknown* createInstance (ScriptClassFactory& factory) override;
		bool load (const Storage&) override;

	protected:
		ObjectArray resources;
	};

	class MetaInstance: public Object,
						public PluginInstance,
						public IPluginMetaClass
	{
	public:
		MetaInstance (ScriptMetaClass& metaClass, ScriptClassFactory& factory);
		~MetaInstance ();

		// IPluginMetaClass
		tresult CCL_API getResourceLocation (IUrl& url, StringID id, StringID language) override;

		CLASS_INTERFACE2 (IPluginMetaClass, IPluginInstance, Object)

	protected:
		ScriptMetaClass& metaClass;
		Url packageUrl;
	};

	class ScriptFile: public Object
	{
	public:
		ScriptFile (StringRef fileName = nullptr)
		: fileName (fileName)
		{}

		PROPERTY_STRING (fileName, FileName)
		PROPERTY_SHARED_AUTO (IObject, executable, Executable)
	};

	ObjectArray classes;
	ObjectList files;
	AutoPtr<VersionDescription> version;

	ScriptClass* getClass (UIDRef cid) const;
	bool remove (UIDRef cid);

	friend class ScriptClass;
	ScriptFile* getFile (StringRef fileName);

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// ScriptHelper
//************************************************************************************************

class ScriptHelper
{
public:
	static IClassFactory* loadFactory (IFileSystem& fileSystem, ScriptUplink* uplink);
	static IObject* loadScript (IFileSystem& fileSystem, StringRef fileName, StringRef packageID);
	static IClassFactory* createFactory (IObject& executable, ScriptUplink* uplink);
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// ScriptUplink
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ScriptUplink, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptUplink::ScriptUplink (UrlRef _path)
: codeResource (nullptr),
  path (NEW Url (_path))
{
	String fileName;
	path->getName (fileName);
	if(fileName == PackageInfo::kFileName)
		path->ascend ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptUplink::~ScriptUplink ()
{
	path->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef ScriptUplink::getPath () const
{
	return *path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptUplink::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "path")
	{
		var = ccl_as_unknown (path);
		return true;
	}
	else if(propertyId == "packageInfo")
	{
		var = ccl_as_unknown (packageInfo);
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// ScriptClassResource
//************************************************************************************************

DEFINE_CLASS (ScriptClassResource, Object)
DEFINE_CLASS_NAMESPACE (ScriptClassResource, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptClassResource::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	id = a.getCString ("id");
	language = a.getCString ("language");
	url = a.getString ("url");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptClassResource::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	a.set ("id", id);
	a.set ("language", language);
	a.set ("url", url);
	return true;
}

//************************************************************************************************
// ScriptClassFactory::ScriptClass
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (ScriptClassFactory::ScriptClass, ClassDescription, "ScriptClass")

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ScriptClassFactory::ScriptClass::createInstance (ScriptClassFactory& factory)
{
	Variant returnValue;
	ScriptFile* file = factory.getFile (getSourceFile ());
	if(file && file->getExecutable ())
	{
		MutableCString functionName (getFunctionName ());
		file->getExecutable ()->invokeMethod (returnValue, Message (functionName, static_cast<IObject*> (factory.getUplink ())));
	}
	IUnknown* unknown = returnValue.asUnknown ();
	return return_shared (unknown);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptClassFactory::ScriptClass::load (const Storage& storage)
{
	if(!SuperClass::load (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	sourceFile = a.getString ("sourceFile");
	functionName = a.getString ("functionName");

	attributes.load (storage);

	// check for inline meta class identifier
	String metaIdString = a.getString ("metaClassID");
	if(!metaIdString.isEmpty ())
		attributes.set (Meta::kMetaClassID, metaIdString);

	#if DEBUG_LOG
	if(!attributes.isEmpty ())
	{
		CCL_PRINTLN ("Script class attributes:")
		attributes.dump ();
	}
	#endif
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptClassFactory::ScriptClass::save (const Storage& storage) const
{
	if(!SuperClass::save (storage))
		return false;

	Attributes& a = storage.getAttributes ();
	a.set ("sourceFile", sourceFile);
	a.set ("functionName", functionName);

	attributes.save (storage);
	return true;
}

//************************************************************************************************
// ScriptClassFactory::ScriptMetaClass
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (ScriptClassFactory::ScriptMetaClass, ScriptClassFactory::ScriptClass, "ScriptMetaClass")

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptClassFactory::ScriptMetaClass::ScriptMetaClass ()
{
	resources.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* ScriptClassFactory::ScriptMetaClass::getResources () const
{
	return resources.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ScriptClassFactory::ScriptMetaClass::createInstance (ScriptClassFactory& factory)
{
	return static_cast<IPluginMetaClass*> (NEW MetaInstance (*this, factory));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptClassFactory::ScriptMetaClass::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	String idString (a.getString ("classID"));
	if(!classID.fromString (idString))
		return false;

	name = idString;
	category = PLUG_CATEGORY_METACLASS;

	// load resources + attributes
	Object* obj = nullptr;
	while((obj = a.unqueueObject (nullptr)) != nullptr)
	{
		if(ScriptClassResource* r = ccl_cast<ScriptClassResource> (obj))
			resources.add (r);
		else if(Attribute* attr = ccl_cast<Attribute> (obj))
			attributes.setAttribute (attr->getID (), attr->getValue (), Attributes::kShare);
		else
			obj->release ();
	}
	return true;
}

//************************************************************************************************
// ScriptClassFactory::MetaInstance
//************************************************************************************************

ScriptClassFactory::MetaInstance::MetaInstance (ScriptMetaClass& metaClass, ScriptClassFactory& factory)
: metaClass (metaClass)
{
	metaClass.retain ();

	String packageID;
	if(ScriptUplink* uplink = factory.getUplink ())
		if(PackageInfo* packageInfo = uplink->getPackageInfo ())
			packageID = packageInfo->getPackageID ();

	ASSERT (!packageID.isEmpty ())
	packageUrl = PackageUrl (packageID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptClassFactory::MetaInstance::~MetaInstance ()
{
	metaClass.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ScriptClassFactory::MetaInstance::getResourceLocation (IUrl& url, StringID id, StringID language)
{
	IterForEach (metaClass.getResources (), ScriptClassResource, r)
		if(r->getID () == id)
		{
			if(r->getLanguage ().isEmpty () || r->getLanguage () == language)
			{
				if(r->getUrl ().contains (CCLSTR ("://")))
				{
					url.setUrl (r->getUrl ());

					static const String kPackagePlaceholder ("$package");
					if(url.getHostName () == kPackagePlaceholder) // replace variable with package identifier
						url.setHostName (packageUrl.getHostName ());
				}
				else
				{
					url.assign (packageUrl);
					url.descend (r->getUrl ());
				}
				return kResultOk;
			}
			break;
		}
	EndFor
	return kResultFalse;
}

//************************************************************************************************
// ScriptClassFactory
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ScriptClassFactory, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptClassFactory::ScriptClassFactory ()
{
	classes.objectCleanup (true);
	files.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptClassFactory::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();
	Object* obj;
	while((obj = a.unqueueObject (nullptr)) != nullptr)
	{
		ScriptClass* scriptClass = ccl_cast<ScriptClass> (obj);
		if(scriptClass)
			classes.add (scriptClass);
		else
		{
			VersionDescription* scriptVersion = ccl_cast<VersionDescription> (obj);
			if(scriptVersion)
				version = scriptVersion;
			else
				obj->release ();
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptClassFactory::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();
	ForEach (classes, ScriptClass, scriptClass)
		a.queue (nullptr, scriptClass);
	EndFor

	if(version)
		a.queue (nullptr, version);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptClassFactory::ScriptClass* ScriptClassFactory::getClass (UIDRef cid) const
{
	ForEach (classes, ScriptClass, scriptClass)
		if(scriptClass->getClassID () == cid)
			return scriptClass;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ScriptClassFactory::remove (UIDRef cid)
{
	if(ScriptClass* scriptClass = getClass (cid))
	{
		classes.remove (scriptClass);
		scriptClass->release ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptClassFactory::ScriptFile* ScriptClassFactory::getFile (StringRef fileName)
{
	ScriptFile* file = nullptr;
	ForEach (files, ScriptFile, f)
		if(f->getFileName () == fileName)
		{
			file = f;
			break;
		}
	EndFor

	if(!file)
		files.add (file = NEW ScriptFile (fileName));

	// try to load executable if not done yet...
	if(!file->getExecutable () && fileSystem)
	{
		ASSERT (uplink && uplink->getPackageInfo ())
		AutoPtr<IObject> executable = ScriptHelper::loadScript (*fileSystem, fileName, uplink->getPackageInfo ()->getPackageID ());
		file->setExecutable (executable);
	}

	return file;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptClassFactory::getVersion (VersionDesc& _version) const
{
	if(version)
		version->toVersionDesc (_version);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ScriptClassFactory::getNumClasses () const
{
	return classes.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptClassFactory::getClassDescription (ClassDesc& description, int index) const
{
	ScriptClass* scriptClass = (ScriptClass*)classes.at (index);
	if(scriptClass)
	{
		scriptClass->toClassDesc (description);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptClassFactory::getClassAttributes (IAttributeList& attributes, UIDRef cid, StringID language) const
{
	bool result = false;
	if(ScriptClass* scriptClass = getClass (cid))
	{
		if(!scriptClass->getAttributes ().isEmpty ())
		{
			attributes.copyFrom (scriptClass->getAttributes ());
			result = true;
		}

		// try to provide a localized class name...
		if(ScriptCodeResource* resource = uplink->getCodeResource ())
			if(ITranslationTable* stringTable = resource->getStrings ())
			{
				String localizedName;
				stringTable->getString (localizedName, nullptr, MutableCString (scriptClass->getName ()));
				if(localizedName != scriptClass->getName ())
				{
					attributes.setAttribute (Meta::kClassLocalizedName, localizedName);
					result = true;
				}

				if(!scriptClass->getDescription ().isEmpty ())
				{
					String localizedDescription;
					stringTable->getString (localizedDescription, nullptr, MutableCString (scriptClass->getDescription ()));
					if(localizedDescription != scriptClass->getDescription ())
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

tresult CCL_API ScriptClassFactory::createInstance (UIDRef cid, UIDRef iid, void** obj)
{
	if(ScriptClass* scriptClass = getClass (cid))
	{
		AutoPtr<IUnknown> unknown = scriptClass->createInstance (*this);
		if(unknown)
			return unknown->queryInterface (iid, obj);
	}

	*obj = nullptr;
	return kResultClassNotFound;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptClassFactory::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "remove")
	{
		UID cid;
		cid.fromString (msg[0].asString ());
		returnValue = remove (cid);
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ScriptHelper
//************************************************************************************************

IClassFactory* ScriptHelper::loadFactory (IFileSystem& fileSystem, ScriptUplink* uplink)
{
	AutoPtr<ScriptClassFactory> classFactory = NEW ScriptClassFactory;
	classFactory->setFileSystem (&fileSystem);
	classFactory->setUplink (uplink);
	ArchiveHandler handler (fileSystem);
	if(handler.loadItem (CCLSTR ("classfactory.xml"), "ClassFactory", *classFactory))
		return classFactory.detach ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* ScriptHelper::loadScript (IFileSystem& fileSystem, StringRef fileName, StringRef packageID)
{
	Url path;
	path.setPath (fileName);
	AutoPtr<IStream> stream = fileSystem.openStream (path, IStream::kOpenMode);
	if(stream)
	{
		AutoPtr<Scripting::IScript> script = ScriptingManager::instance ().createScript (*stream, fileName, packageID, &fileSystem);
		if(script)
			return ScriptingManager::instance ().compileScript (*script);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IClassFactory* ScriptHelper::createFactory (IObject& executable, ScriptUplink* uplink)
{
	IClassFactory* classFactory = nullptr;
	Variant returnValue;
	executable.invokeMethod (returnValue, Message ("CCLGetClassFactory", static_cast<IObject*> (uplink)));
	IUnknown* unknown = returnValue.asUnknown ();
	if(unknown)
		unknown->queryInterface (ccl_iid<IClassFactory> (), (void**)&classFactory);
	return classFactory;
}

//************************************************************************************************
// ScriptCodeResource
//************************************************************************************************

ScriptCodeResource::ScriptCodeResource (IPackageFile* package, IClassFactory* classFactory,
										ScriptUplink* uplink, IObject* executable)
: CodeResource (classFactory),
  package (package),
  uplink (uplink),
  executable (executable),
  stringTable (nullptr)
{
	if(package)
		package->retain ();

	if(uplink)
		uplink->retain (),
		uplink->setCodeResource (this);

	if(executable)
		executable->retain ();

	// try to load translation table
	if(IAttributeList* metaInfo = getMetaInfo ())
	{
		AttributeAccessor accessor (*metaInfo);
		MutableCString packageID = accessor.getCString (Meta::kPackageID);
		String translationFile = accessor.getString ("Package:TranslationFile");
		if(!packageID.isEmpty () && !translationFile.isEmpty ())
		{
			PackageUrl path (String (packageID), translationFile, IUrl::kDetect); // detect type!

			tresult result = System::GetLocaleManager ().loadStrings (stringTable, path, packageID);
			//ASSERT (result == kResultOk)
			if(result != kResultOk)
				CCL_WARN ("Failed to load Translation Table of %s!\n", packageID.str ())
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptCodeResource::~ScriptCodeResource ()
{
	bool started = ScriptingManager::instance ().isStarted ();
	ASSERT (started == true)
	if(!started)
	{
		CCL_WARN ("Detected Scripting Leak!", 0)

		// We have a leak here, but we do not want to crash on exit
		classFactory = nullptr;
		executable = nullptr;
	}

	if(classFactory)
		classFactory->release (),
		classFactory = nullptr;

	if(executable)
		executable->release ();

	if(uplink)
		uplink->setCodeResource (nullptr),
		uplink->release ();

	if(stringTable)
		System::GetLocaleManager ().unloadStrings (stringTable),
		stringTable = nullptr;

	if(package)
	{
		System::GetPackageHandler ().unmountPackageVolume (package);
		package->release ();
	}

	// hmm???
	ScriptingManager::instance ().garbageCollect (); // i.e. ccl_forceGC ()
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITranslationTable* ScriptCodeResource::getStrings ()
{
	return stringTable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API ScriptCodeResource::getType () const
{
	return CodeResourceType::kScript;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* CCL_API ScriptCodeResource::getMetaInfo ()
{
	ASSERT (uplink != nullptr)
	return uplink ? uplink->getPackageInfo () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptCodeResource::getPath (IUrl& path) const
{
	ASSERT (uplink != nullptr)
	if(uplink)
		path.assign (uplink->getPath ());
	return uplink != nullptr;
}

//************************************************************************************************
// ScriptCodeLoader
//************************************************************************************************

DEFINE_SINGLETON (ScriptCodeLoader)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptCodeLoader::~ScriptCodeLoader ()
{
	#if DEBUG
	keyProvider.release (); // place for breakpoint
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ScriptCodeLoader::setKeyProvider (IEncryptionKeyProvider* _keyProvider)
{
	keyProvider = _keyProvider;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API ScriptCodeLoader::getType () const
{
	return CodeResourceType::kScript;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptCodeLoader::isCodeResource (UrlRef path)
{
	return System::GetPackageHandler ().isPackage (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ScriptCodeLoader::loadCodeResource (ICodeResource*& codeResource, UrlRef path)
{
	codeResource = nullptr;

	AutoPtr<IPackageFile> package = System::GetPackageHandler ().openPackage (path);
	if(!package)
		return kResultFailed;

	AutoPtr<PackageInfo> info = NEW PackageInfo;
	if(!info->loadFromPackage (*package))
		return kResultFailed;

	String packageID;
	packageID = info->getPackageID ();
	if(packageID.isEmpty ())
		return kResultFailed;

	// check for encryption
	if(info->getBool (Meta::kPackageExternalKeyRequired))
	{
		String key;
		ASSERT (keyProvider)
		if(keyProvider)
			keyProvider->getEncryptionKey (key, packageID);
		if(key.isEmpty ())
			return kResultFailed;

		package->setOption (PackageOption::kExternalEncryptionKey, key);
	}

	AutoPtr<ScriptUplink> uplink = NEW ScriptUplink (path);
	uplink->setPackageInfo (info);

	AutoPtr<IObject> executable;
	AutoPtr<IClassFactory> factory;

	String sourceFile = info->getString ("CodeResource:Executable");
	if(!sourceFile.isEmpty ())
	{
		executable = ScriptHelper::loadScript (*package->getFileSystem (), sourceFile, packageID);
		if(executable)
			factory = ScriptHelper::createFactory (*executable, uplink);
	}

	// try to load static class factory...
	if(!executable && !factory)
		factory = ScriptHelper::loadFactory (*package->getFileSystem (), uplink);

	if(!factory)
		return kResultFailed;

	// try to mount package...
	if(System::GetPackageHandler ().mountPackageVolume (package, packageID, IPackageVolume::kHidden) != kResultOk)
		return kResultFailed;

	codeResource = NEW ScriptCodeResource (package, factory, uplink, executable);
	return kResultOk;
}
