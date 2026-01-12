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
// Filename    : ccl/extras/analytics/apptrackinghandler.h
// Description : Application Activity Tracking Handler
//
//************************************************************************************************

#ifndef _ccl_apptrackinghandler_h
#define _ccl_apptrackinghandler_h

#include "ccl/app/component.h"

#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/framework/idleclient.h"
#include "ccl/public/base/datetime.h"

#include "ccl/extras/analytics/analyticsevent.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/collections/stringlist.h"

namespace CCL {

class DialogBox;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Analytics Identifier
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace AnalyticsID
{
	// Events
	const CStringPtr kAppStarted = "AppStarted";

	const CStringPtr kAppLaunchReport = "AppLaunchReport";
		const CStringPtr kLaunchFrequency = "launchFrequency";
		const CStringPtr kTotalSessionTime = "totalSessionTime";

	const CStringPtr kDocumentCreated = "DocumentCreated";
	const CStringPtr kDocumentOpened = "DocumentOpened";
	const CStringPtr kDocumentUsageReport = "DocumentUsageReport";
		const CStringPtr kDocuments = "documents";
		const CStringPtr kCreateCount = "created";
		const CStringPtr kOpenCount = "opened";
		const CStringPtr kTemplates = "templates";
		const CStringPtr kTemplate = "template";
		const CStringPtr kTemplateId = "id";

	const CStringPtr kPageClosed = "PageClosed";
	const CStringPtr kPageUsageReport = "PageUsageReport";
		const CStringPtr kPage = "page";
		const CStringPtr kPages = "pages";
		const CStringPtr kType = "type";
		const CStringPtr kDuration = "duration";

	const CStringPtr kCommandUsed = "CommandUsed";
	const CStringPtr kCommandUsageReport = "CommandUsageReport";
		const CStringPtr kCommands = "commands";
		const CStringPtr kCommand = "command";
		const CStringPtr kCommandCount = "count";
		const CStringPtr kInvoker = "invoker";
		const CStringPtr kInvokerMainMenu = "mainMenu";
		const CStringPtr kInvokerContextMenu = "contextMenu";
		const CStringPtr kInvokerKeyboard = "key";
		const CStringPtr kInvokerOther = "-";

	const CStringPtr kUserSystemReport = "UserSystemReport";
		const CStringPtr kOsName = "osName";
		const CStringPtr kOsVersion = "osVersion";
		const CStringPtr kCpuType = "cpuType";
		const CStringPtr kCpuCores = "cpuCores";		
		const CStringPtr kRamAmount = "ramAmount";
		const CStringPtr kDiskSize = "diskSize";
		const CStringPtr kMonitorCount = "monitorCount";
		const CStringPtr kMonitors = "monitors";
		const CStringPtr kMonitorX = "x";
		const CStringPtr kMonitorY = "y";
		const CStringPtr kScaling = "scaling";
		const CStringPtr kWinHighDpiEnabled = "win_highdpi";
		const CStringPtr kMacMetalEnabled = "mac_metal";
		const CStringPtr kColorSchemes = "colorSchemes";
		const CStringPtr kLuminance = "luminance";

	// Common Properties
	const CStringPtr kApplicationFingerprint = "appFingerprint";
	const CStringPtr kApplicationFlavor = "appFlavor";
	const CStringPtr kApplicationBranding = "appBranding";

	enum AppPlan { kPerpetual, kSubscription };

	/** Make user group identifier with format "{Age bracket}{Plan}{Region}{Language}". */
	String makeUserGroupID (int userAge, AppPlan plan);

	/** Get age in years from birthday. */
	int getUserAge (const Date& birthday);

	/** Compute hash for user system report. */
	int64 hashUserSystemReport (const IAnalyticsEvent& e);

	/** Quantizes given Unix seconds according to internal tracking resolution. */
	int64 quantizeTimeStamp (int64 unixTime);
}

//************************************************************************************************
// AppTrackingHandler
//************************************************************************************************

class AppTrackingHandler: public Component,
						  public IAnalyticsEventSink,
						  public CommandDispatcher<AppTrackingHandler>,
						  public IdleClient
{
public:
	DECLARE_CLASS (AppTrackingHandler, Component)

	AppTrackingHandler ();
	~AppTrackingHandler ();

	// Tracking configuration
	PROPERTY_BOOL (autoOptInEnabled, AutoOptInEnabled) ///< automatically opt-in (default is off)
	PROPERTY_STRING (applicationName, ApplicationName)
	PROPERTY_STRING (applicationVersion, ApplicationVersion)
	PROPERTY_VARIABLE (int, buildNumber, BuildNumber)
	PROPERTY_STRING (platform, Platform)
	PROPERTY_STRING (architecture, Architecture)
	PROPERTY_STRING (appFlavor, AppFlavor);
	PROPERTY_STRING (appBranding, AppBranding);
	PROPERTY_OBJECT (Date, userBirthday, UserBirthday);
	PROPERTY_STRING (userTrackingId, UserTrackingID)
	PROPERTY_STRING (appFingerprint, AppFingerprint)
	PROPERTY_VARIABLE (int64, autoFlushPeriod, AutoFlushPeriod)   ///< seconds before cached events are automatically flushed (even before kFlushAt is reached); scheduled when an event is added to the cache
	PROPERTY_VARIABLE (int64, retryFlushPeriod, RetryFlushPeriod) ///< seconds before retrying when writing cached events was denied via canWriteEventsNow ()
	PROPERTY_VARIABLE (int64, retryWritePeriod, RetryWritePeriod) ///< seconds before retrying after failed write (for directly written events)
	PROPERTY_VARIABLE (int64, retryEvaluationPeriod, RetryEvaluationPeriod)	///< seconds before retrying evaluation after failed write (for events that are evaluated)
	PROPERTY_OBJECT (StringList, colorSchemeNames, ColorSchemeNames) ///< name of color schemes to be included in system report (luminance only)

	// register reports
	void registerUserSystemReport (bool onlyWhenChanged = true);
	void registerAppLaunchReport (int64 evaluationPeriod);
	void registerDocumentUsageReport (int64 evaluationPeriod);
	void registerPageUsageReport (int64 evaluationPeriod, const StringList& perspectiveIds);
	void registerViewOpenReport (int64 evaluationPeriod);
	void registerFileExportReport (int64 evaluationPeriod);
	void registerUserInputReport (int64 evaluationPeriod);
	void registerCommandUsageReport (int64 evaluationPeriod);
	void registerNavigationReport (int64 evaluationPeriod);
	void registerBrowserInteractionReport (int64 evaluationPeriod);

	// register direct events
	void registerDocumentUsageEvents ();
	void registerPageClosedEvent (const StringList& perspectiveIds);
	void registerCommandUsedEvent ();

	virtual void startup ();
	virtual void shutdown ();
	virtual void runDialog (bool startupMode = false);
	bool wasTrackingDecided () const;

	class DocumentsListener;
	class PageUsageListener;
	class CommandListener;

	// IAnalyticsEventSink
	void CCL_API addEvent (IAnalyticsEvent& e) override;

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

	// Commands
	DECLARE_COMMANDS (AppTrackingHandler)
	DECLARE_COMMAND_CATEGORY ("Help", Component)
	bool onUsageDataCommand (CmdArgs);

	CLASS_INTERFACE2 (IAnalyticsEventSink, ITimerTask, Component)

protected:
	static constexpr int kFlushAt = 20;
	static constexpr int kMaxEventCount = 1000;
	static constexpr int kIdlePeriod = 60;
	static const Date kBirthdayInvalid;
	static const String kSettingsId;
	static StringID kDiagnosticContext;
	static StringID kTrackingAccepted;
	static StringID kUserBirthday;
	static StringID kUserSystemHash;

	int64 lastAppActivation;
	bool trackingStarted;
	bool trackingEnabled;
	bool trackingDecided;
	AnalyticsTrackingPlan trackingPlan;
	AnalyticsEventCache eventCache;
	AutoPtr<IAnalyticsOutput> trackingOutput;
	AutoPtr<IAsyncOperation> pendingWriteOperation;
	AnalyticsEventCache pendingEvents;
	AnalyticsEventCache eventsToRetry;
	int64 nextFlush;
	int64 nextRetryWrite;
	DialogBox* trackingDialog;
	bool startupMode;

	virtual void addCommonFields (Attributes& data, StringID eventId);

	void restoreSettings ();
	void storeSettings ();
	void applyTrackingDecision (bool state);
	void setTrackingEnabled (bool enabled);
	void enableTrackingInternal (bool enable);
	virtual void onTrackingDecisionChanged (bool state);
	virtual bool canWriteEventsNow () const;

	void onAppActivated (bool active);
	void triggerSystemReport ();
	static void triggerLaunchReport (double trackingDuration, int numStarts, double totalSessionTime);
	void flushEventCache ();
	void onWriteCompleted (IAsyncOperation& op);
	void tryFlush (int64 now);
	void retryWrite (int64 now);
	void removeAllData ();

	// IdleClient
	void onIdleTimer () override;

	class AppLaunchReportFilter;
	class DocumentUsageReportFilter;
	class PageUsageReportFilter;
	class ViewReportFilter;
	class SystemReportFilter;
	class FileExportReportFilter;
	class UserInputReportFilter;
	class CommandReportFilter;
	class NavigationReportFilter;
	class BrowserReportFilter;
};

//************************************************************************************************
// NullAnalyticsOutput
/** Swallows the received events. */
//************************************************************************************************

class NullAnalyticsOutput: public Object,
						   public IAnalyticsOutput
{
public:
	// IAnalyticsOutput
	IAsyncOperation* CCL_API writeEvents (IAnalyticsEvent* events[], int count) override;

	CLASS_INTERFACE (IAnalyticsOutput, Object)
};

} // namespace CCL

#endif // _ccl_apptrackinghandler_h
