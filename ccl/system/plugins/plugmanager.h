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
// Filename    : ccl/system/plugins/plugmanager.h
// Description : Plugin Manager
//
//************************************************************************************************

#ifndef _ccl_plugmanager_h
#define _ccl_plugmanager_h

#include "ccl/base/singleton.h"
#include "ccl/base/boxedtypes.h"
#include "ccl/base/collections/objecthashtable.h"

#include "ccl/system/plugins/plugcollect.h"

#include "ccl/public/base/iextensible.h"
#include "ccl/public/system/idiagnosticdataprovider.h"
#include "ccl/public/system/threadsync.h"
#include "ccl/public/plugins/ipluginmanager.h"

namespace CCL {

struct ClassDesc;
struct VersionDesc;
interface ICodeResource;
interface ICodeResourceLoader;
interface ICodeResourceLoaderHook;
interface IObjectFilter;

class CodeModule;
class CodeResource;
class ClassCategory;
class VersionDescription;
class PersistentAttributes;

//************************************************************************************************
// ClassDescription
//************************************************************************************************

class ClassDescription: public Object,
						public IClassDescription
{
public:
	DECLARE_CLASS (ClassDescription, Object)
	DECLARE_METHOD_NAMES (ClassDescription)

	ClassDescription (StringRef name = nullptr);
	ClassDescription (UIDRef classID);
	ClassDescription (const ClassDescription&);
	ClassDescription (const ClassDesc& description);
	~ClassDescription ();

	void fromClassDesc (const ClassDesc& description);
	void toClassDesc (ClassDesc& description) const;
	bool isSingleton () const;

	bool loadAttributes (const IAttributeList& a);
	bool saveAttributes (IAttributeList& a) const;

	// IClassDescription
	UIDRef CCL_API getClassID () const override;
	StringRef CCL_API getCategory () const override;
	StringRef CCL_API getName () const override;
	StringRef CCL_API getSubCategory () const override;
	StringRef CCL_API getDescription () const override;
	void CCL_API clone (IClassDescription*&) const override;
	const IVersionDescription& CCL_API getModuleVersion () const override;
	tbool CCL_API getClassAttribute (Variant& value, StringID id) const override;
	tbool CCL_API getClassAttributes (IAttributeList& a) const override;
	void CCL_API getLocalizedName (String& name) const override;
	void CCL_API getLocalizedSubCategory (String& subCategory) const override;
	void CCL_API getLocalizedDescription (String& description) const override;
	void CCL_API getClassUrl (IUrl& url) const override;
	StringID CCL_API getCodeResourceType () const override;
	int CCL_API getClassFlags () const override;

	// Object
	bool equals (const Object& obj) const override;
	int getHashCode (int size) const override;
	bool toString (String& string, int flags = 0) const override;
	bool load (const Storage&) override;
	bool save (const Storage&) const override;

	CLASS_INTERFACES (Object)

protected:
	ICodeResource* resource;
	VersionDescription* version;
	PersistentAttributes* attributes;
	int classFlags;
	Boxed::UID classID;
	String category;
	String name;
	String subCategory;
	String description;

	friend class PlugInManager;
	friend class ClassCollection;
	void setResource (ICodeResource* resource);
	void setVersion (VersionDescription* version);
	PersistentAttributes& getAttributes ();
	tresult createInstance (UIDRef iid, void** obj);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// VersionDescription
//************************************************************************************************

class VersionDescription: public Object,
						  public IVersionDescription
{
public:
	DECLARE_CLASS (VersionDescription, Object)

	VersionDescription ();
	VersionDescription (const VersionDesc& description);

	void fromVersionDesc (const VersionDesc& description);
	void toVersionDesc (VersionDesc& description) const;

	bool loadAttributes (const IAttributeList& a);
	bool saveAttributes (IAttributeList& a) const;

	// IVersionDescription
	StringRef CCL_API getName () const override;
	StringRef CCL_API getVersion () const override;
	StringRef CCL_API getVendor () const override;
	StringRef CCL_API getCopyright () const override;
	StringRef CCL_API getUrl () const override;

	// Object
	bool load (const Storage&) override;
	bool save (const Storage&) const override;

	CLASS_INTERFACE (IVersionDescription, Object)

protected:
	String name;
	String version;
	String vendor;
	String copyright;
	String url;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

//************************************************************************************************
// PlugInManager
//************************************************************************************************

class PlugInManager: public PlugInCollection,
					 public IPlugInManager,
					 public IExtensible,
					 public IDiagnosticDataProvider,
					 public Singleton<PlugInManager>
{
public:
	DECLARE_CLASS (PlugInManager, PlugInCollection)
	DECLARE_METHOD_NAMES (PlugInManager)

	PlugInManager ();
	~PlugInManager ();

	typedef LinkedList<ICodeResourceLoaderHook*> HookList;
	const HookList& getHookList () const;
	void deferUnload (CodeModule* module);

	// IPlugInManager
	int CCL_API scanFolder (UrlRef url, StringID type, int options = PlugScanOption::kRecursive,
							IProgressNotify* progress = nullptr, IUrlFilter* filter = nullptr) override;
	int CCL_API scanFolder (UrlRef url, ICodeResourceLoader* loader, int options = PlugScanOption::kRecursive,
							IProgressNotify* progress = nullptr, IUrlFilter* filter = nullptr) override;
	tresult CCL_API unregisterLoader (ICodeResourceLoader* loader) override;
	tresult CCL_API registerFactory (IClassFactory* factory) override;
	tresult CCL_API unregisterFactory (IClassFactory* factory) override;
	tresult CCL_API updateFactory (IClassFactory* factory) override;
	tresult CCL_API storeFile (IAttributeList& classData, UrlRef url, ICodeResourceLoader* loader,
							   StringID language, IObjectFilter* classFilter = nullptr) override;
	tresult CCL_API restoreFile (UrlRef url, ICodeResourceLoader* loader, int options = 0,
								 IAttributeList* classData = nullptr, IObjectFilter* classFilter = nullptr,
								 tbool* fileIsOnBlocklist = nullptr) override;
	tbool CCL_API isDiscardable (UrlRef url) override;
	const IPlugInClassList& CCL_API getClassList (StringRef category) override;
	const IClassDescription* CCL_API getClassDescription (UIDRef cid) override;
	const IClassDescription* CCL_API getClassDescription (StringRef className) override;
	const IClassDescription* CCL_API getClassDescription (UrlRef url) override;
	const IClassDescription* CCL_API getMetaClassDescription (UIDRef cid) override;
	const IClassDescription* CCL_API getAlternativeClass (UIDRef cid) override;
	tresult CCL_API getLastModifiedTime (DateTime& lastModified, UrlRef url) override;
	ISearcher* CCL_API createSearcher (ISearchDescription& description) override;
	tresult CCL_API createInstance (UIDRef cid, UIDRef iid, void** obj) override; // IClassAllocator
	tresult CCL_API createInstance (StringRef className, UIDRef iid, void** obj) override;
	void CCL_API releaseInstance (IUnknown* obj) override;
	const IClassDescription* CCL_API getInstanceClass (IUnknown* obj) override;
	IPluginMetaClass* CCL_API createMetaClass (UIDRef cid) override;
	tresult CCL_API registerStubClass (UIDRef iid,  StringID name, StubConstructor constructor) override;
	tresult CCL_API unregisterStubClass (UIDRef iid, StubConstructor constructor) override;
	tresult CCL_API createStubInstance (UIDRef iid, IObject* object, void** stub) override;
	void CCL_API addHook (IUnknown* unknown) override;
	void CCL_API removeHook (IUnknown* unknown) override;
	tbool CCL_API enableBlocklist (tbool state) override;
	void CCL_API resetBlocklist () override;
	tresult CCL_API addToBlocklist (UrlRef url) override;
	tresult CCL_API removeFromBlocklist (UrlRef url) override;
	void CCL_API getBlocklistContent (IUnknownList& blocklist) override;
	void CCL_API saveSettings () override;
	void CCL_API removeSettings (tbool anyLanguage = true) override;
	tbool CCL_API getCurrentFolder (IUrl& currentFolder) override;
	void CCL_API setCurrentFolder (const IUrl* currentFolder) override;
	tresult CCL_API storeFileList (ICodeResourceLoader* loader) override;
	tresult CCL_API restoreFileList (ICodeResourceLoader* loader, int options = 0) override;
	tresult CCL_API setClassAttribute (const IClassDescription& description, StringID id, VariantRef value) override;
	tresult CCL_API unloadUnusedModules () override;
	void CCL_API terminate () override;

	// IExtensible
	IUnknown* CCL_API getExtension (StringID id) override;

	// IDiagnosticDataProvider
	int CCL_API countDiagnosticData () const override;
	tbool CCL_API getDiagnosticDescription (DiagnosticDescription& description, int index) const override;
	IStream* CCL_API createDiagnosticData (int index) override;
	
	// PlugInCollection
	using PlugInCollection::removeFromBlocklist;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	BEGIN_CLASS_INTERFACES
		QUERY_INTERFACE (IClassAllocator)
		QUERY_INTERFACE (IPlugInManager)
		QUERY_INTERFACE (IExtensible)
		QUERY_INTERFACE (IDiagnosticDataProvider)
	END_CLASS_INTERFACES (PlugInCollection)

protected:
	DECLARE_STRINGID_MEMBER (kVersionID)
	DECLARE_STRINGID_MEMBER (kClassesID)
	DECLARE_STRINGID_MEMBER (kNumClassesID)
	DECLARE_STRINGID_MEMBER (kDiscardableID)

	static const String kBlocklistProtocol;

	Threading::CriticalSection lock;
	ObjectList categories;
	ObjectHashTable classIdTable;
	ObjectList altClassMisses;
	ObjectHashTable altClassMissTable;
	ObjectHashTable instances;
	Settings* settings;
	ObjectList runtimeList;
	ICodeResourceLoader* currentLoader;
	IUrlFilter* currentPathFilter;
	bool keepDiscardable;
	HookList hookList;
	LinkedList<IObjectFilter*> filterList;
	ObjectList unloadList;

	bool addClass (ClassDescription* desc);
	ClassCategory* lookupCategory (StringRef category, bool create);
	ClassDescription* lookupClass (VariantRef var);
	ClassDescription* lookupClass (UIDRef cid);
	ClassDescription* lookupClass (StringRef className);
	tresult createInstance (ClassDescription& desc, UIDRef iid, void** obj);
	Object* getInstanceData (IUnknown* obj);
	CodeResource* findInRuntimeList (IClassFactory* factory) const;

	// PlugInCollection overrides:
	Settings& getSettings () override;
	bool isModule (UrlRef url) const override;
	Module* createModule (UrlRef url) const override;
	void getModuleTime (int64& modifiedTime, Module* module) override;
	bool restoreModuleInfo (StringRef settingsID, Module* module) override;
	bool registerModuleInfo (StringRef settingsID, Module* module) override;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

} // namespace CCL

#endif // _ccl_plugmanager_h
