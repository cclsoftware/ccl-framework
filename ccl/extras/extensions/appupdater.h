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
// Filename    : ccl/extras/extensions/appupdater.h
// Description : Application Updater
//
//************************************************************************************************

#ifndef _ccl_appupdater_h
#define _ccl_appupdater_h

#include "ccl/app/component.h"

#include "ccl/extras/extensions/icontentserver.h"

#include "ccl/public/plugins/versionnumber.h"
#include "ccl/public/gui/commanddispatch.h"

namespace CCL {
interface ITriggerAction;
interface IProgressNotify;

namespace Install {

//************************************************************************************************
// IUpdateCheckResult
//************************************************************************************************

interface IUpdateCheckResult: IUnknown
{
	virtual const ContentDefinition& getDefinition () const = 0;

	virtual const VersionNumber& getCurrentVersion () const = 0;

	virtual StringRef getErrorText () const = 0;
	
	virtual CCL::tbool CCL_API getAction (CCL::IUrl& url, CCL::String& title) const = 0;

	DECLARE_IID (IUpdateCheckResult)

	static IUpdateCheckResult* getFirst (VariantRef result);
};

//************************************************************************************************
// ApplicationUpdater
//************************************************************************************************

class ApplicationUpdater: public Component,
						  public CommandDispatcher<ApplicationUpdater>,
						  public ComponentSingleton<ApplicationUpdater>
{
public:
	DECLARE_CLASS (ApplicationUpdater, Component)

	ApplicationUpdater ();

	PROPERTY_POINTER (IContentServer, contentServer, ContentServer) ///< not owned!

	void setAppDefinition (const ContentDefinition& appDefinition);
	const ContentDefinition& getAppDefinition () const;
	
	PROPERTY_STRING (releaseNotesId, ReleaseNotesID)
	PROPERTY_OBJECT (VersionNumber, appServerVersion, AppServerVersion)

	bool isUpdateAvailable () const;
	void checkAppUpdatesInBackground ();
	void setUpdateAvailable (bool state);

	/** Async result: IContainer with IUpdateCheckResults. */
	IAsyncOperation* checkUpdates (const ContentDefinition definitions[], int count, IUnknown* credentials, IProgressNotify* progress = nullptr);
	IAsyncOperation* checkUpdates (const ContentDefinition definitions[], int count); ///< prompts for credentials
	
	/** Start asynchronous download, prompts for credentials. */
	bool startDownload (const ContentDefinition& definition, ITriggerAction* finalizer, IUrl* localPath = nullptr, IUnknown* userData = nullptr);
	
	/** Synchronous download blocking main thread. */
	bool downloadFile (IUrl& dstPath, const ContentDefinition& definition, IUnknown* credentials, IProgressNotify* progress);
	bool downloadFile (IUrl& dstPath, const ContentDefinition& definition, IProgressNotify* progress); ///< prompts for credentials

	// Commands
	DECLARE_COMMANDS (ApplicationUpdater)
	DECLARE_COMMAND_CATEGORY ("Help", Component)
	bool onCheckUpdates (CCL::CmdArgs args);
	bool onUpdateNow (CCL::CmdArgs args);
	bool onViewReleaseNotes (CCL::CmdArgs args);

	// Component
	tresult CCL_API terminate () override;

protected:
	class UpdateTask;
	class UpdateFinalizer;
	class ReleaseNotesFinalizer;

	SharedPtr<IAsyncOperation> appUpdateOperation;
	ContentDefinition appDefinition;
	bool updateAvailable;

	void onAppCheckCompleted (CCL::IAsyncOperation& operation);
	bool verifyResult (String& text, const IUpdateCheckResult& result) const;

	void loadUpdaterConfiguration ();

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

} // namespace Install
} // namespace CCL

#endif // _ccl_appupdater_h
