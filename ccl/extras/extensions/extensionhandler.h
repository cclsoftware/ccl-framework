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
// Filename    : ccl/extras/extensions/extensionhandler.h
// Description : Extension Handler
//
//************************************************************************************************

#ifndef _ccl_extensionhandler_h
#define _ccl_extensionhandler_h

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/text/cclstring.h"
#include "ccl/public/extras/iextensionhandler.h"

#include "ccl/public/collections/linkedlist.h"

namespace CCL {

class Url;
interface ITheme;
interface ITranslationTable;

namespace Install {

class ExtensionDescription;

//************************************************************************************************
// ExtensionHandler
//************************************************************************************************

class ExtensionHandler: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (ExtensionHandler, Object)

	virtual void startup () {}
	virtual void shutdown () {}

	/** Do the work at program startup (register plug-ins, etc.). */
	virtual int startupExtension (ExtensionDescription& description) = 0;

	/** Revert changes done at program startup. */
	virtual void shutdownExtension (ExtensionDescription& description) {}

	/** Extension has been enabled/disabled/marked for uninstall/update, etc. */
	virtual void onExtensionChanged (ExtensionDescription& description) {}

	/** A new extension has been installed. */
	virtual void onExtensionInstalled (ExtensionDescription& description, bool silent) {}

	/** Begin/end notification for silent extension installation. */
	virtual void beginInstallation (bool state) {}
};

//************************************************************************************************
// IExtensionProductHandler
/** Private interface to detect products in extension. */
//************************************************************************************************

interface IExtensionProductHandler: IUnknown
{
	virtual tresult CCL_API detectProducts (ExtensionDescription& description) = 0;

	DECLARE_IID (IExtensionProductHandler)
};

//************************************************************************************************
// ExtensionNativePluginHandler
//************************************************************************************************

class ExtensionNativePluginHandler: public ExtensionHandler
{
public:
	// ExtensionHandler
	int startupExtension (ExtensionDescription& description) override;
};

//************************************************************************************************
// ExtensionCorePluginHandler
//************************************************************************************************

class ExtensionCorePluginHandler: public ExtensionHandler
{
public:
	// ExtensionHandler
	int startupExtension (ExtensionDescription& description) override;
};

//************************************************************************************************
// ExtensionScriptPluginHandler
//************************************************************************************************

class ExtensionScriptPluginHandler: public ExtensionHandler
{
public:
	// ExtensionHandler
	int startupExtension (ExtensionDescription& description) override;
};

//************************************************************************************************
// ExtensionLanguageHandler
//************************************************************************************************

class ExtensionLanguageHandler: public ExtensionHandler
{
public:
	ExtensionLanguageHandler ();
	
	// ExtensionHandler
	int startupExtension (ExtensionDescription& description) override;
	void onExtensionChanged (ExtensionDescription& description) override;
	void shutdownExtension (ExtensionDescription& description) override;

protected:
	class TableEntry: public Object
	{
	public:
		DECLARE_CLASS_ABSTRACT (TableEntry, Object)
		
		TableEntry (StringRef tableId, ITranslationTable* table)
		: tableId (tableId),
		  table (table)
		{}
		PROPERTY_STRING (tableId, TableID)
		PROPERTY_POINTER (ITranslationTable, table, Table)
	};
	ObjectArray tables;
};

//************************************************************************************************
// ExtensionHelpHandler
//************************************************************************************************

class ExtensionHelpHandler: public ExtensionHandler
{
public:
	// ExtensionHandler
	int startupExtension (ExtensionDescription& description) override;
};

//************************************************************************************************
// ExtensionPresetHandler
//************************************************************************************************

class ExtensionPresetHandler: public ExtensionHandler
{
public:
	// ExtensionHandler
	int startupExtension (ExtensionDescription& description) override;
	void onExtensionInstalled (ExtensionDescription& description, bool silent) override;
};

//************************************************************************************************
// ExtensionTemplateHandler
//************************************************************************************************

class ExtensionTemplateHandler: public ExtensionHandler
{
public:
	// ExtensionHandler
	int startupExtension (ExtensionDescription& description) override;
};

//************************************************************************************************
// ExtensionSkinHandler
//************************************************************************************************

class ExtensionSkinHandler: public ExtensionHandler
{
public:
	~ExtensionSkinHandler ();

	// ExtensionHandler
	void shutdown () override;
	int startupExtension (ExtensionDescription& description) override;

protected:
	LinkedList<ITheme*> themeList;

	bool findDefaultSkin (Url& result, UrlRef path) const;
};

//************************************************************************************************
// ExtensionSnapshotHandler
//************************************************************************************************

class ExtensionSnapshotHandler: public ExtensionHandler
{
public:
	// ExtensionHandler
	int startupExtension (ExtensionDescription& description) override;
	void shutdownExtension (ExtensionDescription& description) override;
};

//************************************************************************************************
// ExternalExtensionHandler
//************************************************************************************************

class ExternalExtensionHandler: public ExtensionHandler,
								public IExtensionCompatibilityHandler
{
public:
	~ExternalExtensionHandler ();

	// ExtensionHandler
	void startup () override;
	void shutdown () override;
	int startupExtension (ExtensionDescription& description) override;
	void shutdownExtension (ExtensionDescription& description) override;

	// IExtensionCompatibilityHandler
	tresult CCL_API checkCompatibility (IExtensionDescription& description) override;

	CLASS_INTERFACE (IExtensionCompatibilityHandler, ExtensionHandler)

protected:
	LinkedList<IExtensionHandler*> handlers;
};

} // namespace Install
} // namespace CCL

#endif // _ccl_extensionhandler_h
