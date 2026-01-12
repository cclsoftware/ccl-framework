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
// Filename    : ccl/public/plugins/ipluginmanager.h
// Description : Plug-in Manager Interface
//
//************************************************************************************************

#ifndef _ccl_ipluginmanager_h
#define _ccl_ipluginmanager_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/base/datetime.h"

namespace CCL {

interface IObject;
interface IStubObject;
interface IAttributeList;
interface IClassFactory;
interface IVersionDescription;
interface IPluginMetaClass;
interface ICodeResourceLoader;
interface ISearcher;
interface ISearchDescription;
interface IProgressNotify;
interface IUrlFilter;
interface IObjectFilter;
interface IUnknownList;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-In Manager Macros
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Iterate classes of given category. */
#define ForEachPlugInClass(category, var) \
{ const CCL::IPlugInClassList& __classList = CCL::System::GetPlugInManager ().getClassList (category); \
  for(int __classIndex = 0; __classIndex < __classList.getNumClasses (); __classIndex++) \
  { const CCL::IClassDescription& var = __classList.getClass (__classIndex);

//////////////////////////////////////////////////////////////////////////////////////////////////
// Plug-In Manager Signals
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Signals
{
	/** Signals related to Plug-In Manager */
	DEFINE_STRINGID (kPlugIns, "CCL.PlugIns")

		/** arg[0]: class category (String) */
		DEFINE_STRINGID (kClassCategoryChanged, "ClassCategoryChanged")

		/** arg[0]: IProgressNotify, args[1]: IUnknownList */
		DEFINE_STRINGID (kRescanPlugIns, "RescanPlugIns")

		/** no arguments */
		DEFINE_STRINGID (kResetBlocklist, "ResetBlocklist")

		/** no arguments */
		DEFINE_STRINGID (kResetBlocklistDone, "ResetBlocklistDone")

		/** no arguments */
		DEFINE_STRINGID (kTerminatePlugIns, "TerminatePlugIns")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Stub constructor. */
typedef IStubObject* (CCL_API *StubConstructor) (UIDRef iid, IObject* object, IUnknown* outerUnknown);

//************************************************************************************************
// IClassDescription
/** \ingroup base_plug */
//************************************************************************************************

interface IClassDescription: IUnknown
{
	/** Get unique class identifier. */
	virtual UIDRef CCL_API getClassID () const = 0;

	/** Get class flags. */
	virtual int CCL_API getClassFlags () const = 0;

	/** Get class category. */
	virtual StringRef CCL_API getCategory () const = 0;

	/** Get class name. */
	virtual StringRef CCL_API getName () const = 0;

	/** Get subcategory. */
	virtual StringRef CCL_API getSubCategory () const = 0;
	
	/** Get description. */
	virtual StringRef CCL_API getDescription () const = 0;

	/** Clone object. */
	virtual void CCL_API clone (IClassDescription*&) const = 0;

	/** Get version of module this class resides in. */
	virtual const IVersionDescription& CCL_API getModuleVersion () const = 0;

	/** Get class attribute by identifier (current language). */
	virtual tbool CCL_API getClassAttribute (Variant& value, StringID id) const = 0;

	/** Copy all class attributes (current language). */
	virtual tbool CCL_API getClassAttributes (IAttributeList& a) const = 0;
	
	/** Get a possibly localized class name. */
	virtual void CCL_API getLocalizedName (String& name) const = 0;

	/** Get a possibly localized class description. */
	virtual void CCL_API getLocalizedDescription (String& description) const = 0;

	/** Get a possibly localized subcategory. */
	virtual void CCL_API getLocalizedSubCategory (String& subCategory) const = 0;

	/** Make class URL for this class. */
	virtual void CCL_API getClassUrl (IUrl& url) const = 0;

	/** Get type of code resource (native, script). */
	virtual StringID CCL_API getCodeResourceType () const = 0;	

	DECLARE_IID (IClassDescription)
};

DEFINE_IID (IClassDescription, 0xe889006d, 0xd582, 0x4893, 0x9a, 0xa3, 0x8d, 0xf9, 0x3e, 0xcd, 0x2d, 0x8b)

//************************************************************************************************
// IVersionDescription
/** \ingroup base_plug */
//************************************************************************************************

interface IVersionDescription: IUnknown
{
	/** Get product name. */
	virtual StringRef CCL_API getName () const = 0;

	/** Get product version. */
	virtual StringRef CCL_API getVersion () const = 0;

	/** Get vendor string. */
	virtual StringRef CCL_API getVendor () const = 0;

	/** Get copyright string. */
	virtual StringRef CCL_API getCopyright () const = 0;

	/** Get URL (vendor or product website). */
	virtual StringRef CCL_API getUrl () const = 0;

	DECLARE_IID (IVersionDescription)
};

DEFINE_IID (IVersionDescription, 0x52e6ff91, 0x6721, 0x47f5, 0x94, 0xac, 0xc6, 0x27, 0x1f, 0x14, 0xbf, 0x2e)

//************************************************************************************************
// IPlugInClassList
/** \ingroup base_plug */
//************************************************************************************************

interface IPlugInClassList: IUnknown
{
	/** Get number of classes in list. */
	virtual int CCL_API getNumClasses () const = 0;

	/** Get class description at given index. */
	virtual const IClassDescription& CCL_API getClass (int index) const = 0;

	DECLARE_IID (IPlugInClassList)
};

DEFINE_IID (IPlugInClassList, 0x90f9f199, 0xa68e, 0x48e2, 0x84, 0xbf, 0x2d, 0xf7, 0x55, 0xfb, 0x36, 0x8b)

//************************************************************************************************
// PlugScanOption
//************************************************************************************************

namespace PlugScanOption
{
	enum Options
	{
		kRecursive = 1<<0,
		kKeepDiscardable = 1<<1,
		kValidityConfirmed = 1<<2 // file exports a valid class factory, used with restoreFile() only
	};
}

//************************************************************************************************
// IPlugInManager
/** \ingroup base_plug */
//************************************************************************************************

interface IPlugInManager: IClassAllocator
{
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Class registration
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Scan folder for plug-ins (built-in loader). */
	virtual int CCL_API scanFolder (UrlRef url, StringID type, int options = PlugScanOption::kRecursive, 
									IProgressNotify* progress = nullptr, IUrlFilter* filter = nullptr) = 0;

	/** Scan folder for plug-ins (custom loader). */
	virtual int CCL_API scanFolder (UrlRef url, ICodeResourceLoader* loader, int options = PlugScanOption::kRecursive,
									IProgressNotify* progress = nullptr, IUrlFilter* filter = nullptr) = 0;

	/** Unregister classes of given loader. */
	virtual tresult CCL_API unregisterLoader (ICodeResourceLoader* loader) = 0;

	/** Register class factory at runtime. */
	virtual tresult CCL_API registerFactory (IClassFactory* factory) = 0;

	/** Unregister class factory at runtime. */
	virtual tresult CCL_API unregisterFactory (IClassFactory* factory) = 0;

	/** Update class factory at runtime. */
	virtual tresult CCL_API updateFactory (IClassFactory* factory) = 0;

	/** Store class information of given file. */
	virtual tresult CCL_API storeFile (IAttributeList& classData, UrlRef url, ICodeResourceLoader* loader,
									   StringID language, IObjectFilter* classFilter = nullptr) = 0;

	/** Restore class information of given file. */
	virtual tresult CCL_API restoreFile (UrlRef url, ICodeResourceLoader* loader, int options = 0,
										 IAttributeList* classData = nullptr, IObjectFilter* classFilter = nullptr,
										 tbool* fileIsOnBlocklist = nullptr) = 0;

	/** Check if class information of given file is discardable. */
	virtual tbool CCL_API isDiscardable (UrlRef url) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Class enumeration
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Get class category. */
	virtual const IPlugInClassList& CCL_API getClassList (StringRef category) = 0;

	/** Get description by class identifier. */
	virtual const IClassDescription* CCL_API getClassDescription (UIDRef cid) = 0;

	/** Get description by class name ("Category:SubCategory:Name"). */
	virtual const IClassDescription* CCL_API getClassDescription (StringRef className) = 0;

	/** Get description by class URL. */
	virtual const IClassDescription* CCL_API getClassDescription (UrlRef url) = 0;

	/** Get description of associated meta class (optional). */
	virtual const IClassDescription* CCL_API getMetaClassDescription (UIDRef cid) = 0;

	/** Try to resolve class identifier to alternative class. */
	virtual const IClassDescription* CCL_API getAlternativeClass (UIDRef cid) = 0;

	/** Create searcher for plug-in classes. */
	virtual ISearcher* CCL_API createSearcher (ISearchDescription& description) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Instance creation
	//////////////////////////////////////////////////////////////////////////////////////////////

	using IClassAllocator::createInstance;

	/** Create instance by class name ("Category:SubCategory:Name"). */
	virtual tresult CCL_API createInstance (StringRef className, UIDRef iid, void** obj) = 0;

	/** Release class instance. */
	virtual void CCL_API releaseInstance (IUnknown* obj) = 0;

	/**	Get class of existing instance. 
		For package information, class description can be queried for ICodeResource. 
		Please note that package information is not available for classes registered
		at runtime via registerFactory(). */
	virtual const IClassDescription* CCL_API getInstanceClass (IUnknown* obj) = 0;

	/** Create meta class of given class (optional). */
	virtual IPluginMetaClass* CCL_API createMetaClass (UIDRef cid) = 0;
	
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Stub classes
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Register stub class for given interface. */
	virtual tresult CCL_API registerStubClass (UIDRef iid, StringID name, StubConstructor constructor) = 0;

	/** Unregister stub class for given interface. */
	virtual tresult CCL_API unregisterStubClass (UIDRef iid, StubConstructor constructor) = 0;

	/** Create stub class instance with given interface for an IObject. */
	virtual tresult CCL_API createStubInstance (UIDRef iid, IObject* object, void** stub) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Hooks
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Install hook (ICodeResourceLoaderHook or IObjectFilter). */
	virtual void CCL_API addHook (IUnknown* unknown) = 0;
	
	/** Uninstall hook. */
	virtual void CCL_API removeHook (IUnknown* unknown) = 0;
	
	//////////////////////////////////////////////////////////////////////////////////////////////
	// Blocklist
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Enable/disable blocklist, returns previous state. */
	virtual tbool CCL_API enableBlocklist (tbool state) = 0;

	/** Reset blocklist. */
	virtual void CCL_API resetBlocklist () = 0;

	/** Add file to blocklist. */
	virtual tresult CCL_API addToBlocklist (UrlRef url) = 0;

	/** Remove file from blocklist. */
	virtual tresult CCL_API removeFromBlocklist (UrlRef url) = 0;

	/** Get content of blocklist. */
	virtual void CCL_API getBlocklistContent (IUnknownList& blocklist) = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Other
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Save class settings. */
	virtual void CCL_API saveSettings () = 0;

	/** Remove all cached class settings. */
	virtual void CCL_API removeSettings (tbool anyLanguage = true) = 0;

	/** Get folder currently being scanned. */
	virtual tbool CCL_API getCurrentFolder (IUrl& currentFolder) = 0;

	/** Set (or reset) folder currently being scanned. */
	virtual void CCL_API setCurrentFolder (const IUrl* currentFolder) = 0;

	/** Store list of files handled by given loader. */
	virtual tresult CCL_API storeFileList (ICodeResourceLoader* loader) = 0;

	/** Restore files handled by given loader. */
	virtual tresult CCL_API restoreFileList (ICodeResourceLoader* loader, int options = 0) = 0;

	/** Get the modification date of a module that corresponds to a given module URL */
	virtual tresult CCL_API getLastModifiedTime (DateTime& lastModified, UrlRef url) = 0;

	/** Set attribute for registered class. */
	virtual tresult CCL_API setClassAttribute (const IClassDescription& description, StringID id, VariantRef value) = 0;

	/** Unload unused modules immediately. */
	virtual tresult CCL_API unloadUnusedModules () = 0;

	/** Unload all modules and cleanup. */
	virtual void CCL_API terminate () = 0;
	
	DECLARE_IID (IPlugInManager)
};

DEFINE_IID (IPlugInManager, 0x1c94bfc0, 0xeee2, 0x4096, 0xbb, 0x59, 0xb9, 0x85, 0xc4, 0xee, 0x55, 0xe5)

} // namespace CCL

#endif // _ccl_ipluginmanager_h
