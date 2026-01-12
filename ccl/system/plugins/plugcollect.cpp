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
// Filename    : ccl/system/plugins/plugcollect.cpp
// Description : Plugin Collection
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/plugins/plugcollect.h"
#include "ccl/system/plugins/module.h"

#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/settings.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ilogger.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/text/translation.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("PlugIns")
	XSTRING (BlockListed, "%(1) has been blocked.")
END_XSTRINGS

//************************************************************************************************
// PlugInCollection
//************************************************************************************************

DEFINE_CLASS (PlugInCollection, Object)
DEFINE_CLASS_NAMESPACE (PlugInCollection, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInCollection::PlugInCollection (StringRef name, StringRef blocklistName)
: name (name),
  currentFolder (nullptr),
  blocklistName (blocklistName),
  blocklist (nullptr)
{
	searchPaths.objectCleanup ();
	modules.objectCleanup (); // this is a very central place, please keep module ownership here!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlugInCollection::~PlugInCollection ()
{
	safe_release (blocklist);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef PlugInCollection::getName () const 
{ 
	return name; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCollection::addSearchPath (Url* url)
{
	if(!searchPaths.contains (*url))
		searchPaths.add (url);
	else
		url->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCollection::addSearchPath (UrlRef url)
{
	addSearchPath (NEW Url (url));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCollection::addSearchPaths (Container& paths)
{
	ForEach (paths, Url, path)
		addSearchPath (path);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCollection::addAppFolder (StringRef folderName)
{
	Url* path = NEW Url;
	System::GetSystem ().getLocation (*path, System::kAppSupportFolder);
	path->descend (folderName, Url::kFolder);

	addSearchPath (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PlugInCollection::scanFolders (IProgressNotify* progress)
{
	int total = 0;
	ForEach (searchPaths, Url, path)
		total += scanFolder (*path, true, progress);
	EndFor
	return total;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PlugInCollection::scanFolder (UrlRef baseUrl, bool recursive, IProgressNotify* progress)
{
	if(!currentFolder)
		currentFolder = &baseUrl;
	
	int total = 0;
	int mode = recursive ? (IFileIterator::kAll | IFileIterator::kBundlesAsFiles) : IFileIterator::kFiles;

	if(progress)
		progress->updateAnimated (UrlDisplayString (baseUrl));

	ForEachFile (System::GetFileSystem ().newIterator (baseUrl, mode), url)		
		if(url->isFile () || isModule (*url)) // could be a file or a folder
		{
			if(scanFile (*url))
				total++;
		}
		else if(url->isFolder ())
		{
			if(recursive)
				total += scanFolder (*url, true);
		}

		if(progress)
		{
			progress->updateAnimated ();
			if(progress->isCanceled ())
				break;
		}
	EndFor
	
	if(currentFolder == &baseUrl)
		currentFolder = nullptr;

	return total;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCollection::scanFile (UrlRef url)
{
	bool result = false;
	if(isModule (url))
	{
		CCL_PRINT ("Found module: ")
		CCL_PRINTLN (url.getPath ())

		AutoPtr<Module> module = createModule (url);
		ASSERT (module != nullptr)
		if(!modules.contains (*module))
		{
			if(scanModule (module))
			{
				module->retain ();
				modules.add (module);
				result = true;
			}			
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCollection::scanFileInSearchPaths (StringRef fileName)
{
	ForEach (searchPaths, Url, path)
		Url fullUrl (*path);
		fullUrl.descend (fileName);
		if(System::GetFileSystem ().fileExists (fullUrl))
		{
			if(scanFile (fullUrl))
				return true;
		}
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCollection::isModule (UrlRef url) const
{
	// could be a file or a folder
	String ext;
	url.getExtension (ext);
	return ext.compare (FileTypes::Module ().getExtension (), false) == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Module* PlugInCollection::createModule (UrlRef url) const
{
	CCL_DEBUGGER ("PlugInCollection::createModule must be implemented by derived class!")
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings& PlugInCollection::getSettings ()
{
	return Settings::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& PlugInCollection::getSettingsID (String& settingsID, Module* module) const
{
	return getSettingsID (settingsID, module->getPath ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String& PlugInCollection::getSettingsID (String& settingsID, UrlRef url) const
{
	settingsID.empty ();

	String pathName, fileName;
	url.getPathName (pathName);
	url.getName (fileName);

	unsigned int hash = pathName.getHashCode ();
	settingsID.appendHexValue ((int64)hash, 8); // %08X
	settingsID.append (Url::strPathChar);
	settingsID.append (fileName);
	return settingsID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCollection::getModuleTime (DateTime& modifiedTime, Module* module)
{
	FileInfo info;
	System::GetFileSystem ().getFileInfo (info, module->getPath ());
	modifiedTime = info.modifiedTime;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCollection::restoreModule (StringRef settingsID, const DateTime& moduleTime, Module* module)
{
	DateTime savedTime;
	if(restoreModuleTime (savedTime, settingsID) && savedTime == moduleTime)
	{
		if(restoreModuleInfo (settingsID, module))
			return true;
	}	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCollection::storeModuleTime (StringRef settingsID, const DateTime& moduleTime)
{
	getSettings ().getSection (settingsID)->setObject ("modifiedTime", NEW Boxed::DateTime (moduleTime));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCollection::restoreModuleTime (DateTime& moduleTime, StringRef settingsID)
{
	Boxed::DateTime* time = getSettings ().getSection (settingsID)->getObject<Boxed::DateTime> ("modifiedTime");
	if(time == nullptr)
		return false;
	moduleTime = *time;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCollection::scanModule (Module* module)
{
	String settingsID;
	getSettingsID (settingsID, module);

	DateTime moduleTime;
	getModuleTime (moduleTime, module);

	// try to restore module information if not modified...

	if(restoreModule (settingsID, moduleTime, module))
		return true;

	//...or register module and keep time stamp...

	storeModuleTime (settingsID, moduleTime);

	bool result = registerModuleInfo (settingsID, module);

	// let subclass decide if settings should really be removed!
	if(!result) 
		registerModuleFailed (settingsID, module);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCollection::restoreModuleInfo (StringRef settingsID, Module* module)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCollection::registerModuleInfo (StringRef settingsID, Module* module)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCollection::registerModuleFailed (StringRef settingsID, Module* module)
{
	unregisterModuleInfo (settingsID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCollection::unregisterModuleInfo (StringRef settingsID)
{
	getSettings ().remove (settingsID); // cleanup settings
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCollection::saveModules ()
{
	String settingsID;
	settingsID = name;
	settingsID.append (CCLSTR (" Paths"));

	saveModules (settingsID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCollection::restoreModules ()
{
	String settingsID;
	settingsID = name;
	settingsID.append (CCLSTR (" Paths"));

	restoreModules (settingsID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCollection::saveModules (StringRef settingsID, ModuleFilter* filter)
{
	getSettings ().remove (settingsID); // remove old paths
	Attributes& attributes = getSettings ().getAttributes (settingsID);

	ForEach (modules, Module, module)
		if(filter && !filter->matches (module))
			continue;
		attributes.queue (nullptr, module->getPath ().clone (), Attributes::kOwns);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCollection::restoreModules (StringRef settingsID)
{
	if(getSettings ().getSection (settingsID, false) == nullptr) // check if section exists
		return false;

	Attributes& attributes = getSettings ().getAttributes (settingsID);
	IterForEach (attributes.newQueueIterator (nullptr, ccl_typeid<Url> ()), Url, url)
		AutoPtr<Module> module = createModule (*url);
		ASSERT (module != nullptr)
		if(module)
		{
			String str;
			getSettingsID (str, module);
			if(restoreModuleInfo (str, module))
			{
				module->retain ();
				modules.add (module);
			}
		}
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings* PlugInCollection::createBlocklistCopy () const
{
	Settings* blocklistCopy = NEW XmlSettings (blocklistName);
	blocklistCopy->isPlatformSpecific (true);
	blocklistCopy->restore ();
	return blocklistCopy;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCollection::enableBlocklist (bool state)
{
	if(state)
	{
		if(blocklist == nullptr)
			blocklist = createBlocklistCopy ();
	}
	else
		safe_release (blocklist);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCollection::isBlocklistEnabled () const
{
	return blocklist != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugInCollection::resetBlocklist ()
{
	ASSERT (!blocklistName.isEmpty ())
	XmlSettings::removeSettings (blocklistName, false, true);
	if(blocklist)
		blocklist->removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCollection::removeFromBlocklist (StringRef settingsID)
{
	bool removed = false;
	if(blocklist)
	{
		removed = blocklist->remove (settingsID);
		blocklist->flush ();
	}
	return removed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCollection::addToBlocklist (StringRef settingsID)
{
	unregisterModuleInfo (settingsID);

	if(blocklist)
	{
		Attributes& blockInfo = blocklist->getAttributes (settingsID);
		blockInfo.set ("clean", false);
		blocklist->flush ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PlugInCollection::checkBlocklist (StringRef settingsID, StringRef name)
{
	if(blocklist)
	{
		Attributes& blockInfo = blocklist->getAttributes (settingsID);
		bool clean = true;
		blockInfo.getBool (clean, "clean");
		if(!clean)
		{
			System::GetLogger ().reportEvent (Alert::Event (String ().appendFormat (XSTR (BlockListed), name), Alert::kWarning));
			return false;
		}
		blockInfo.set ("clean", false);
		blocklist->flush ();
	}
	return true;
}
