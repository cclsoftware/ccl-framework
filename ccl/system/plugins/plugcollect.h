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
// Filename    : ccl/system/plugins/plugcollect.h
// Description : Plugin Collection
//
//************************************************************************************************

#ifndef _ccl_plugcollect_h
#define _ccl_plugcollect_h

#include "ccl/public/text/cclstring.h"
#include "ccl/base/collections/objectlist.h"
#include "ccl/public/base/datetime.h"

namespace CCL {

class Url;
class Module;
class Settings;
struct ModuleFilter;

interface IProgressNotify;

//************************************************************************************************
// PlugInCollection
//************************************************************************************************

class PlugInCollection: public Object
{
public:
	DECLARE_CLASS (PlugInCollection, Object)

	PlugInCollection (StringRef name = nullptr, StringRef blocklistName = nullptr);
	~PlugInCollection ();

	StringRef getName () const;

	void addSearchPath (UrlRef url);
	void addSearchPath (Url* url);
	void addSearchPaths (Container& paths);
	void addAppFolder (StringRef folderName);

	int scanFolders (IProgressNotify* progress = nullptr);
	int scanFolder (UrlRef url, bool recursive = true, IProgressNotify* progress = nullptr);

	bool scanFile (UrlRef url);
	bool scanFileInSearchPaths (StringRef fileName);

	void saveModules ();
	void restoreModules ();
	void resetBlocklist ();

protected:
	String name;
	ObjectList searchPaths;
	ObjectList modules;
	const IUrl* currentFolder;
	String blocklistName;
	Settings* blocklist;

	bool scanModule (Module* module);
	bool restoreModule (StringRef settingsID, const DateTime& moduleTime, Module* module);
	void storeModuleTime (StringRef settingsID, const DateTime& moduleTime);
	bool restoreModuleTime (DateTime& moduleTime, StringRef settingsID);

	String& getSettingsID (String& settingsID, UrlRef url) const;
	String& getSettingsID (String& settingsID, Module* module) const;

	// to be overwritten by subclass:
	virtual Settings& getSettings ();
	
	virtual bool isModule (UrlRef url) const;
	virtual Module* createModule (UrlRef url) const;

	virtual void getModuleTime (DateTime& modifiedTime, Module* module);
	virtual bool restoreModuleInfo (StringRef settingsID, Module* module);
	virtual bool registerModuleInfo (StringRef settingsID, Module* module);
	virtual void registerModuleFailed (StringRef settingsID, Module* module);
	virtual void unregisterModuleInfo (StringRef settingsID);

	void saveModules (StringRef settingsID, ModuleFilter* filter = nullptr);
	bool restoreModules (StringRef settingsID);

	// blocklist (can be used by subclass)
	Settings* createBlocklistCopy () const;
	bool isBlocklistEnabled () const;
	void enableBlocklist (bool state);
	bool checkBlocklist (StringRef settingsID, StringRef name);
	bool removeFromBlocklist (StringRef settingsID);
	bool addToBlocklist (StringRef settingsID);
};

} // namespace CCL

#endif // _ccl_plugcollect_h
