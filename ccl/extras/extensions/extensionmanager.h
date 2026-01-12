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
// Filename    : ccl/extras/extensions/extensionmanager.h
// Description : Extension Manager
//
//************************************************************************************************

#ifndef _ccl_extensionmanager_h
#define _ccl_extensionmanager_h

#include "ccl/app/component.h"
#include "ccl/base/singleton.h"

#include "ccl/extras/extensions/extensiondescription.h"

#include "ccl/public/system/idiagnosticdataprovider.h"
#include "ccl/public/gui/commanddispatch.h"

namespace CCL {

struct DragEvent;
interface IDragHandler;
interface IProgressNotify;
interface IAsyncOperation;
class XmlSettings;

namespace Install {

class ExtensionHandler;

//************************************************************************************************
// ExtensionFilter
//************************************************************************************************

class ExtensionFilter: public Object,
					   public StaticSingleton<ExtensionFilter>
{
public:
	void loadFilter ();

	bool isCompatible (StringRef id, const VersionNumber& version) const;

protected:
	struct Condition
	{
		enum Flags { kDeprecated = 1<<0 };

		String extensionId;
		VersionNumber minVersion;
		int flags = 0;

		PROPERTY_FLAG (flags, kDeprecated, isDeprecated)
	};

	Vector<Condition> conditions;
};

//************************************************************************************************
// ExtensionManager
//************************************************************************************************

class ExtensionManager: public Component,
						public IDiagnosticDataProvider,
						public ComponentSingleton<ExtensionManager>
{
public:
	DECLARE_CLASS (ExtensionManager, Component)

	ExtensionManager ();
	~ExtensionManager ();
	
	static const String kExtensionUpdateFolderName;

	PROPERTY_OBJECT (VersionNumber, appVersion, AppVersion)
	PROPERTY_STRING (appIdentity, AppIdentity)
	PROPERTY_SHARED_AUTO (IUrlFilter, signatureFilter, SignatureFilter)
	PROPERTY_SHARED_AUTO (Url, migrationSourceFolder, MigrationSourceFolder) // source folder for extension migration from older host version
	
	void setSharedLocation (UrlRef path);

	enum HandlerPriority { kFirstHandler = -1, kLastHandler = 0 };
	void addHandler (ExtensionHandler* handler, int priority = kLastHandler); ///< takes ownership!
	const Container& getHandlers () const;

	ExtensionDescription* scanExtension (StringRef id, StringRef shortId = nullptr); ///< try to scan given extension (without adding)

	void startup (IProgressNotify* progress = nullptr);
	bool isStarted () const;

	void checkAutomaticUpdates (bool& restartNeeded);
	void deferInstallWithUI (UrlRef path);
	void deferInstallFromServer ();
	IDragHandler* createDragHandler (const DragEvent& event, IView* view);

	int getExtensionCount () const;
	ExtensionDescription* getExtensionDescription (int index) const;
	ExtensionDescription* findExtension (StringRef id) const;

	enum ErrorCode { kAlreadyInstalled, kNotCompatible };
	String formatMessage (ErrorCode which, const ExtensionDescription& e, bool detailed = false) const;

	bool isInsideExtension (UrlRef path) const; ///< check if file is inside an extension
	bool isUserInstalled (const ExtensionDescription& e) const;

	// internal (used by ExtensionInstaller):
	bool checkSignature (UrlRef srcPath, IProgressNotify* progress = nullptr);
	bool checkCompatibility (ExtensionDescription& e) const;
	IAsyncOperation* checkUpdatesAsync (Container& extensions, bool silent = false, IProgressNotify* progress = nullptr);
	bool checkUpdates (Container& extensions, bool silent = false);
	bool downloadUpdate (Url& dstPath, ExtensionDescription& e, IProgressNotify* progress = nullptr);
	bool installFile (UrlRef srcPath, ExtensionDescription& e, IProgressNotify* progress = nullptr);
	void signalInstalled (ExtensionDescription& e, bool silent);
	bool uninstall (ExtensionDescription& e);
	bool updateFile (UrlRef srcPath, ExtensionDescription& e, IProgressNotify* progress = nullptr);
	bool enable (Container& extensions, bool state);
	void startupExtension (ExtensionDescription& e);
	void flushSettings ();

	// Component
	tresult CCL_API terminate () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	
	// IDiagnosticDataProvider
	int CCL_API countDiagnosticData () const override;
	tbool CCL_API getDiagnosticDescription (DiagnosticDescription& description, int index) const override;
	IStream* CCL_API createDiagnosticData (int index) override;
	
	CLASS_INTERFACE (IDiagnosticDataProvider, Component)

protected:
	ObjectArray handlers;
	ObjectArray extensions;
	XmlSettings& settings;
	bool restored;
	bool started;
	AutoPtr<IUnknown> credentials;

	void getUpdateLocation (Url& path) const;
	StringRef getParentProductID (ExtensionDescription& e) const;
	void updateEnabledState (ExtensionDescription& e);

	void migrateFiles (UrlRef folder);
	void scanFolder (UrlRef folder, ExtensionTypeID type);
	void installUpdates ();
};

} // namespace Install
} // namespace CCL

#endif // _ccl_extensionmanager_h
