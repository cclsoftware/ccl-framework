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
// Filename    : ccl/app/safety/appsafetymanager.h
// Description : Application Safety Manager 
//
//************************************************************************************************

#ifndef _ccl_appsafetymanager_h
#define _ccl_appsafetymanager_h

#include "ccl/base/singleton.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/app/isafetyoption.h"
#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/plugins/iservicemanager.h"

namespace CCL {
	
class Document;
interface ICrashReport;
interface IDiagnosticDataProvider;

//************************************************************************************************
// AppSafetyManager
//************************************************************************************************

class AppSafetyManager: public Object,
						public IdleClient,
						public IServiceNotification,
						public Singleton<AppSafetyManager>
{
public:
	AppSafetyManager ();
	
	class ServiceOptionsProvider;
	class PluginOptionsProvider;

	bool startup (bool forceDialog = false);
	void shutdown ();

	tresult runDiagnosticsUI () const;

	/** Add a provider. Takes ownership. */
	void addOptionProvider (ISafetyOptionProvider* provider);
	void addDiagnosticProvider (IDiagnosticDataProvider* provider);
	bool addDiagnosticProvider (IUnknown* provider);

	bool showDocumentSafetyOptions (Document* document = nullptr);
	void resetDocumentSafetyOptions (Document* document = nullptr);

	void getActiveSafetyOptions (Vector<SafetyOptionDescription>& options, IUnknown* context) const;
	void getActiveSafetyOptionsText (String& options, IUnknown* context) const;

	void getDiagnosticsFolder (IUrl& folder) const;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IdleClient
	void onIdleTimer () override;
	
	// IServiceNotification
	tresult CCL_API onServiceNotification (const IServiceDescription& description, int eventCode) override;

	CLASS_INTERFACE2 (ITimerTask, IServiceNotification, Object)

protected:
	friend class SafetyOptionsDialog;

	static const String kDiagnosticFolder;

	static const int kIdleDelay = 10000;
	
	SignalSink safetySink;
	
	UnknownList optionProviders;
	ServiceOptionsProvider* serviceOptionsProvider;
	PluginOptionsProvider* pluginOptionsProvider;
	
	UnknownList diagnosticProviders;

	void registerServiceStartupOptions (StringRef category);
	bool showAppSafetyOptions (StringRef description, ICrashReport* report = nullptr, IUnknownList* unstablePlugins = nullptr);
	void applyAppSafetyOptions ();
	void reportUnstablePlugins (IUnknownList& unstablePlugins) const;
};

} // namespace CCL

#endif // _ccl_appsafetymanager_h
