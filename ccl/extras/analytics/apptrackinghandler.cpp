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
// Filename    : ccl/extras/analytics/apptrackinghandler.cpp
// Description : Application Activity Tracking Handler
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/extras/analytics/apptrackinghandler.h"

#include "ccl/app/documents/documentmanager.h"
#include "ccl/app/documents/document.h"

#include "ccl/base/storage/jsonarchive.h"
#include "ccl/base/storage/settings.h"
#include "ccl/base/singleton.h"

#include "ccl/public/app/signals.h"
#include "ccl/public/math/mathprimitives.h"

#include "ccl/public/gui/appanalytics.h"
#include "ccl/public/gui/iapplication.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/iworkspace.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/icolorscheme.h"
#include "ccl/public/gui/framework/iwin32specifics.h"
#include "ccl/public/gui/framework/imacosspecifics.h"
#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/base/streamer.h"

#include "ccl/public/system/cclanalytics.h"
#include "ccl/public/system/idiagnosticstore.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/cclversion.h"

#define SKIP_CACHE (0 && DEBUG) // write out each event immediately

using namespace CCL;

//************************************************************************************************
// AppTrackingHandler::DocumentsListener
//************************************************************************************************

class AppTrackingHandler::DocumentsListener: public Component,
											 public SharedSingleton<DocumentsListener>,
											 public AbstractDocumentEventHandler
{
public:
	DocumentsListener ()
	: Component ("DocumentsListener")
	{
		DocumentManager::instance ().addHandler (this);
	}

	// IDocumentEventHandler
	void CCL_API onDocumentManagerAvailable (tbool state) override
	{
		if(!state)
			DocumentManager::instance ().removeHandler (this);
	}

	void CCL_API onDocumentEvent (IDocument& document, int eventCode) override
	{
		switch(eventCode)
		{
		case IDocument::kCreated:
			{
				Attributes data;
				data.set (AnalyticsID::kType, document.getPath ().getFileType ().getExtension ());

				auto doc = unknown_cast<Document> (&document);
				if(doc && !doc->getSourceTemplateID ().isEmpty ())
					data.set (AnalyticsID::kTemplate, doc->getSourceTemplateID ());

				ccl_analytics_event (AnalyticsID::kDocumentCreated, &data);
			}
			break;

		case IDocument::kLoadFinished:
			{
				Attributes data;
				data.set (AnalyticsID::kType, document.getPath ().getFileType ().getExtension ());
				ccl_analytics_event (AnalyticsID::kDocumentOpened, &data);
			}
			break;
		}
	}

	void CCL_API onDocumentExported (IDocument& document, UrlRef exportPath) override
	{
		Attributes analyticsData;
		analyticsData.set (AnalyticsID::kFileExportContext, "ExportDocument");
		analyticsData.set (AnalyticsID::kFileExportType, exportPath.getFileType ().getExtension ());
		ccl_analytics_event (AnalyticsID::kFileExported, &analyticsData);
	}

	CLASS_INTERFACE (IDocumentEventHandler, Component)
};

DEFINE_SHARED_SINGLETON (AppTrackingHandler::DocumentsListener)

//************************************************************************************************
// AppTrackingHandler::PageUsageListener
//************************************************************************************************

class AppTrackingHandler::PageUsageListener: public Component,
											 public SharedSingleton<PageUsageListener>
{
public:
	PageUsageListener ()
	: Component ("PageUsageListener")
	{
		ISubject::addObserver (&System::GetGUI (), this);
	}

	PROPERTY_OBJECT (StringList, perspectiveIds, PerspectiveIds)

	tresult CCL_API terminate () override
	{
		if(IWorkspace* appWorkspace = System::GetWorkspaceManager ().getWorkspace (RootComponent::instance ().getApplicationID ()))
			ISubject::removeObserver (appWorkspace, this);

		return Component::terminate ();
	}

	// Component
	void CCL_API notify (ISubject* subject, MessageRef msg) override
	{
		if(msg == IWorkspace::kPerspectiveSelected)
		{
			// track duration when page was left
			bool state = msg[0];
			String perspectiveId (msg[1]);
			int64 lastActivated (msg[2]);
			if(!state && perspectiveIds.contains (perspectiveId))
			{
				int64 now = System::GetSystemTicks ();
				int64 duration = (now - lastActivated) / 1000;

				Attributes data;
				data.set (AnalyticsID::kType, perspectiveId);
				data.set (AnalyticsID::kDuration, duration);
				ccl_analytics_event (AnalyticsID::kPageClosed, &data);
			}
		}
		else if(msg == IApplication::kUIInitialized)
		{
			ISubject::removeObserver (&System::GetGUI (), this); // only needed to get notified when workspace is ready

			if(IWorkspace* appWorkspace = System::GetWorkspaceManager ().getWorkspace (RootComponent::instance ().getApplicationID ()))
				ISubject::addObserver (appWorkspace, this);
		}
	}
};

DEFINE_SHARED_SINGLETON (AppTrackingHandler::PageUsageListener)

//************************************************************************************************
// AppTrackingHandler::CommandListener
//************************************************************************************************

class AppTrackingHandler::CommandListener: public Component,
										   public SharedSingleton<CommandListener>,
										   public ICommandFilter
{
public:
	CommandListener ()
	: Component ("CommandListener")
	{
		System::GetCommandTable ().addFilter (this);
	}

	tresult CCL_API terminate () override
	{
		System::GetCommandTable ().removeFilter (this);
		return Component::terminate ();
	}

	MutableCString makeCommandID (const CommandMsg& msg)
	{
		return MutableCString (msg.category).append ('|').append (msg.name);
	}

	StringID classifyInvoker (const CommandMsg& msg)
	{
		if(UnknownPtr<IMenuItem> menuItem = msg.invoker)
		{
			UnknownPtr<IContextMenu> contextMenu;

			if(IMenu* menu = menuItem->getParentMenu ())
			{
				// up to root menu
				while(UnknownPtr<IMenu> parent = menu->getParentUnknown ())
					menu = parent;

				Variant menuData;
				menu->getMenuAttribute (menuData, IMenu::kMenuData);
				contextMenu = menuData;
			}

			if(contextMenu)
				return CSTR (AnalyticsID::kInvokerContextMenu);
			else
				return CSTR (AnalyticsID::kInvokerMainMenu);
		}
		else if(UnknownPtr<ICommand> command = msg.invoker)
		{
			return CSTR (AnalyticsID::kInvokerKeyboard); // TODO: could also be a deferred command...
		}
		return CSTR (AnalyticsID::kInvokerOther);
	}

	// ICommandFilter
	tbool CCL_API isCommandAllowed (const CommandMsg& msg) override
	{
		if(!msg.checkOnly ())
		{
			Attributes data;
			data.set (AnalyticsID::kCommand, makeCommandID (msg));
			data.set (AnalyticsID::kInvoker, classifyInvoker (msg));
			ccl_analytics_event (AnalyticsID::kCommandUsed, &data);
		}
		return true;
	}

	CLASS_INTERFACE (ICommandFilter, Component)
};

DEFINE_SHARED_SINGLETON (AppTrackingHandler::CommandListener)

//************************************************************************************************
// AppTrackingHandler::SystemReportFilter
//************************************************************************************************

class AppTrackingHandler::SystemReportFilter: public Unknown,
											  public AbstractAnalyticsEventFilter
{
public:
	SystemReportFilter (bool onlyWhenChanged)
	: onlyWhenChanged (onlyWhenChanged)
	{}

	// IAnalyticsEventFilter
	IAnalyticsEvent* CCL_API process (IAnalyticsEvent& e) override
	{
		if(e.getID () == AnalyticsID::kUserSystemReport)
		{
			DiagnosticStoreAccessor diagnostics (System::GetDiagnosticStore ());
			int64 hash = AnalyticsID::hashUserSystemReport (e);
			int64 previousHash = diagnostics.getPlainValue (kDiagnosticContext, kUserSystemHash);

			// only handle event if hash is different from previous one
			if(!onlyWhenChanged || previousHash != hash)
			{
				diagnostics.setPlainValue (kDiagnosticContext, kUserSystemHash, hash);
				return return_shared (&e);
			}
		}
		return nullptr;
	}

	CLASS_INTERFACE (IAnalyticsEventFilter, Unknown)

private:
	bool onlyWhenChanged;
};

//************************************************************************************************
// AppTrackingHandler::AppLaunchReportFilter
//************************************************************************************************

class AppTrackingHandler::AppLaunchReportFilter: public Unknown,
												 public AbstractAnalyticsEventFilter
{
public:
	static void trackAppUsage (int64 activeSeconds)
	{
		if(activeSeconds > 0)
			System::GetDiagnosticStore ().submitValue (kDiagnosticContext, AnalyticsID::kTotalSessionTime, activeSeconds);
	}

	AppLaunchReportFilter (AppTrackingHandler& trackingHandler)
	: trackingHandler (trackingHandler)
	{}

	// IAnalyticsEventFilter
	IAnalyticsEvent* CCL_API process (IAnalyticsEvent& e) override
	{
		if(e.getID () == AnalyticsID::kAppStarted)
		{
			// submit time of app start, so that we can later calculate a frequency
			System::GetDiagnosticStore ().submitValue (kDiagnosticContext, e.getID (), UnixTime::getTime ());
		}
		else if(e.getID () == AnalyticsID::kAppLaunchReport)
		{
			return return_shared (&e);
		}
		return nullptr;
	}

	void CCL_API evaluateData () override
	{
		AutoPtr<IDiagnosticResult> appStartData (System::GetDiagnosticStore ().queryResult (kDiagnosticContext, AnalyticsID::kAppStarted));
		AutoPtr<IDiagnosticResult> totalSessionTimeData (System::GetDiagnosticStore ().queryResult (kDiagnosticContext, AnalyticsID::kTotalSessionTime));
		if(!appStartData && !totalSessionTimeData)
			return; // no data tracked since last evaluation

		// determine launch frequency and session time from data tracked in diagnostic
		double totalSessionTime = 0;
		double trackingDuration = 0;
		int numStarts = 0;

		if(appStartData)
		{
			trackingDuration = appStartData->getMaximum () - appStartData->getMinimum ();
			numStarts = appStartData->getCount ();
		}

		if(totalSessionTimeData)
			totalSessionTime = totalSessionTimeData->getSum ();

		triggerLaunchReport (trackingDuration, numStarts, totalSessionTime);
	}

	void CCL_API onWriteCompleted (StringID eventId) override
	{
		if(eventId == AnalyticsID::kAppLaunchReport)
		{
			// success: remove evaluated data from diagnostic store
			System::GetDiagnosticStore ().clearData (kDiagnosticContext, AnalyticsID::kAppStarted);
			System::GetDiagnosticStore ().clearData (kDiagnosticContext, AnalyticsID::kTotalSessionTime);
		}
	}

	CLASS_INTERFACE (IAnalyticsEventFilter, Unknown)

private:
	AppTrackingHandler& trackingHandler;
};

//************************************************************************************************
// AppTrackingHandler::DocumentUsageReportFilter
//************************************************************************************************

class AppTrackingHandler::DocumentUsageReportFilter: public Unknown,
													 public AbstractAnalyticsEventFilter
{
public:
	DocumentUsageReportFilter ()
	: documentsDiagnosticPrefix (MutableCString (kDiagnosticContext).append ("/Documents/")),
	  templatesDiagnosticPrefix (MutableCString (kDiagnosticContext).append ("/DocumentTemplates/"))
	{}

	MutableCString makeDiagnosticContext (const AttributeReader& data) { return MutableCString (documentsDiagnosticPrefix).append (data.getString (AnalyticsID::kType)); }
	MutableCString makeDiagnosticContext (const DocumentClass& docClass) { return MutableCString (documentsDiagnosticPrefix).append (docClass.getFileType ().getExtension ());	}

	// IAnalyticsEventFilter
	IAnalyticsEvent* CCL_API process (IAnalyticsEvent& e) override
	{
		if(e.getID () == AnalyticsID::kDocumentCreated)
		{
			AttributeReadAccessor data (e.getData ());
			System::GetDiagnosticStore ().submitValue (makeDiagnosticContext (data), AnalyticsID::kCreateCount, Variant ());

			String templateId;
			if(data.getString (templateId, AnalyticsID::kTemplate))
			{
				// track used template: "analytics/DocumentTemplates/templateID", "created"
				MutableCString context (templatesDiagnosticPrefix);
				context += templateId;
				System::GetDiagnosticStore ().submitValue (context, AnalyticsID::kCreateCount, Variant ());
			}
		}
		else if(e.getID () == AnalyticsID::kDocumentOpened)
		{
			AttributeReadAccessor data (e.getData ());
			System::GetDiagnosticStore ().submitValue (makeDiagnosticContext (data), AnalyticsID::kOpenCount, Variant ());
		}
		else if(e.getID () == AnalyticsID::kDocumentUsageReport)
			return return_shared (&e);

		return nullptr;
	}

	void CCL_API evaluateData () override
	{
		// create report from all filetypes
		Attributes data;

		for(auto docClass : iterate_as<DocumentClass> (DocumentManager::instance ().getDocumentClasses ()))
		{
			// query all keys (created, opened) for this document type
			Vector<CString> keys { AnalyticsID::kCreateCount, AnalyticsID::kOpenCount };
			AutoPtr<IDiagnosticResultSet> results = System::GetDiagnosticStore ().queryMultipleResults (makeDiagnosticContext (*docClass), keys, keys.count ());
			if(results)
			{
				auto docAttribs = NEW Attributes;
				docAttribs->set (AnalyticsID::kType, docClass->getFileType ().getExtension ());

				for(int i = 0; i < results->getCount (); i++)
					if(IDiagnosticResult* documentData = results->at (i))
						docAttribs->set (keys[i], documentData->getCount ());

				data.queue (AnalyticsID::kDocuments, docAttribs, Attributes::kOwns);
			}
		}

		// query "analytics/DocumentTemplates/*"
		MutableCString context (templatesDiagnosticPrefix);
		context.append ("*");

		AutoPtr<IDiagnosticResultSet> templateResults = System::GetDiagnosticStore ().queryResults (context, AnalyticsID::kCreateCount);
		if(templateResults)
		{
			for(int i = 0; i < templateResults->getCount (); i++)
			{
				IDiagnosticResult* templateData = templateResults->at (i);

				String templateId (templateData->getContext ().subString (templatesDiagnosticPrefix.length ()));

				auto templateAttribs = NEW Attributes;
				templateAttribs->set (AnalyticsID::kTemplateId, templateId);
				templateAttribs->set (AnalyticsID::kCreateCount, templateData->getCount ());

				data.queue (AnalyticsID::kTemplates, templateAttribs, Attributes::kOwns);
			}
		}

		if(!data.isEmpty ())
			ccl_analytics_event (AnalyticsID::kDocumentUsageReport, &data);
	}

	void CCL_API onWriteCompleted (StringID eventId) override
	{
		if(eventId == AnalyticsID::kDocumentUsageReport)
		{
			// success: remove evaluated data from diagnostic store
			MutableCString context (documentsDiagnosticPrefix);
			context.append ('*');
			System::GetDiagnosticStore ().clearData (context, CString::kEmpty);

			context = templatesDiagnosticPrefix;
			context.append ('*');
			System::GetDiagnosticStore ().clearData (context, CString::kEmpty);
		}
	}

	CLASS_INTERFACE (IAnalyticsEventFilter, Unknown)

private:
	MutableCString documentsDiagnosticPrefix;
	MutableCString templatesDiagnosticPrefix;
};

//************************************************************************************************
// AppTrackingHandler::PageUsageReportFilter
//************************************************************************************************

class AppTrackingHandler::PageUsageReportFilter: public Object,
												 public AbstractAnalyticsEventFilter
{
public:
	PageUsageReportFilter ()
	{
		diagnosticContext += kDiagnosticContext;
		diagnosticContext += "/PageUsed";
	}

	// IAnalyticsEventFilter
	IAnalyticsEvent* CCL_API process (IAnalyticsEvent& e) override
	{
		if(e.getID () == AnalyticsID::kPageClosed)
		{
			AttributeReadAccessor data (e.getData ());
			MutableCString perspectiveId (data.getString (AnalyticsID::kType));
			int64 duration = data.getInt64 (AnalyticsID::kDuration);

			System::GetDiagnosticStore ().submitValue (diagnosticContext, perspectiveId, duration);
		}
		else if(e.getID () == AnalyticsID::kPageUsageReport)
			return return_shared (&e);

		return nullptr;
	}

	void CCL_API evaluateData () override
	{
		PageUsageListener* listener = PageUsageListener::peekInstance ();
		if(!listener)
			return;

		// create report for all registered perspective IDs
		Attributes data;

		for(const String* perspectiveId : listener->getPerspectiveIds ())
		{
			AutoPtr<IDiagnosticResult> perspectiveData (System::GetDiagnosticStore ().queryResult (diagnosticContext, MutableCString (*perspectiveId)));
			if(perspectiveData)
			{
				auto pageAttribs = NEW Attributes;
				pageAttribs->set (AnalyticsID::kType, *perspectiveId);
				pageAttribs->set (AnalyticsID::kDuration, perspectiveData->getSum ());
				data.queue (AnalyticsID::kPages, pageAttribs, Attributes::kOwns);
			}
		}

		if(!data.isEmpty ())
			ccl_analytics_event (AnalyticsID::kPageUsageReport, &data);
	}

	void CCL_API onWriteCompleted (StringID eventId) override
	{
		if(eventId == AnalyticsID::kPageUsageReport)
		{
			// success: remove evaluated data from diagnostic store
			System::GetDiagnosticStore ().clearData (diagnosticContext, CString::kEmpty);
		}
	}

	CLASS_INTERFACE (IAnalyticsEventFilter, Unknown)

private:
	MutableCString diagnosticContext;
};

//************************************************************************************************
// AppTrackingHandler::ViewReportFilter
//************************************************************************************************

class AppTrackingHandler::ViewReportFilter: public Object,
											public AbstractAnalyticsEventFilter
{
public:
	// IAnalyticsEventFilter
	IAnalyticsEvent* CCL_API process (IAnalyticsEvent& e) override
	{
		if(e.getID () == AnalyticsID::kViewOpened)
		{
			// track in diagnostic store: "analytics/Views/viewName"
			AttributeReadAccessor data (e.getData ());
			String viewName (data.getString (AnalyticsID::kViewName));

			MutableCString context (kDiagnosticsPrefix);
			context.append (viewName, Text::kUTF8);

			System::GetDiagnosticStore ().submitValue (context, AnalyticsID::kViewOpenCount, Variant ());
		}
		else if(e.getID () == AnalyticsID::KViewOpenReport)
			return return_shared (&e);

		return nullptr;
	}

	void CCL_API evaluateData () override
	{
		Attributes data;

		MutableCString context (kDiagnosticsPrefix);
		context.append ("*");

		AutoPtr<IDiagnosticResultSet> results = System::GetDiagnosticStore ().queryResults (context, AnalyticsID::kViewOpenCount);
		if(results)
		{
			for(int i = 0; i < results->getCount (); i++)
			{
				IDiagnosticResult* resultData = results->at (i);

				String viewName (resultData->getContext ().subString (kDiagnosticsPrefix.length ()));

				auto resultAttribs = NEW Attributes;
				resultAttribs->set (AnalyticsID::kViewName, viewName);
				resultAttribs->set (AnalyticsID::kViewOpenCount, resultData->getCount ());

				data.queue (AnalyticsID::kViews, resultAttribs, Attributes::kOwns);
			}
		}

		if(!data.isEmpty ())
			ccl_analytics_event (AnalyticsID::KViewOpenReport, &data);
	}

	void CCL_API onWriteCompleted (StringID eventId) override
	{
		if(eventId == AnalyticsID::KViewOpenReport)
		{
			// success: remove evaluated data from diagnostic store
			MutableCString context (kDiagnosticsPrefix);
			context.append ("*");
			System::GetDiagnosticStore ().clearData (context, CString::kEmpty);
		}
	}

	CLASS_INTERFACE (IAnalyticsEventFilter, Unknown)

private:
	static CString kDiagnosticsPrefix;
};

CString AppTrackingHandler::ViewReportFilter::kDiagnosticsPrefix ("analytics/View/");

//************************************************************************************************
// AppTrackingHandler::FileExportReportFilter
//************************************************************************************************

class AppTrackingHandler::FileExportReportFilter: public Unknown,
												  public AbstractAnalyticsEventFilter
{
public:
	// IAnalyticsEventFilter
	IAnalyticsEvent* CCL_API process (IAnalyticsEvent& e) override
	{
		if(e.getID () == AnalyticsID::kFileExported)
		{
			// track in diagnostic store: "analytics/Export/context/type"
			AttributeReadAccessor attributes (e.getData ());
			String exportContext (attributes.getString (AnalyticsID::kFileExportContext));
			String exportType (attributes.getString (AnalyticsID::kFileExportType));

			MutableCString context (kDiagnosticsPrefix);
			context.append (exportContext, Text::kUTF8);
			if(!exportType.isEmpty ())
				context.append ('/').append (exportType, Text::kUTF8);

			System::GetDiagnosticStore ().submitValue (context, AnalyticsID::kFileExportCount, Variant ());
		}
		else if(e.getID () == AnalyticsID::kFileExportReport)
			return return_shared (&e);

		return nullptr;
	}

	void CCL_API evaluateData () override
	{
		Attributes data;

		MutableCString context (kDiagnosticsPrefix);
		context.append ("*/*");

		AutoPtr<IDiagnosticResultSet> results = System::GetDiagnosticStore ().queryResults (context, AnalyticsID::kFileExportCount);
		if(results)
		{
			for(int i = 0; i < results->getCount (); i++)
			{
				IDiagnosticResult* exportData = results->at (i);

				String exportContext (exportData->getContext ().subString (kDiagnosticsPrefix.length ()));
				String exportType;
				int separatorIndex = exportContext.lastIndex ("/");
				if(separatorIndex >= 0)
				{
					exportType = exportContext.subString (separatorIndex + 1);
					exportContext.truncate (separatorIndex);
				}

				auto exportAttribs = NEW Attributes;
				exportAttribs->set (AnalyticsID::kFileExportContext, exportContext);
				exportAttribs->set (AnalyticsID::kFileExportType, exportType);
				exportAttribs->set (AnalyticsID::kFileExportCount, exportData->getCount ());

				data.queue (AnalyticsID::kFileExports, exportAttribs, Attributes::kOwns);
			}
		}

		if(!data.isEmpty ())
			ccl_analytics_event (AnalyticsID::kFileExportReport, &data);
	}

	void CCL_API onWriteCompleted (StringID eventId) override
	{
		if(eventId == AnalyticsID::kFileExportReport)
		{
			// success: remove evaluated data from diagnostic store
			MutableCString context (kDiagnosticsPrefix);
			context.append ("*");
			System::GetDiagnosticStore ().clearData (context, CString::kEmpty);
			context.append ("/*");
			System::GetDiagnosticStore ().clearData (context, CString::kEmpty);
		}
	}

	CLASS_INTERFACE (IAnalyticsEventFilter, Unknown)

private:
	static CString kDiagnosticsPrefix;
};

CString AppTrackingHandler::FileExportReportFilter::kDiagnosticsPrefix ("analytics/Export/");

//************************************************************************************************
// AppTrackingHandler::UserInputReportFilter
//************************************************************************************************

class AppTrackingHandler::UserInputReportFilter: public Unknown,
												 public AbstractAnalyticsEventFilter
{
public:
	UserInputReportFilter ()
	: nextUpdate (UnixTime::getTime () + kUpdateDelay)
	{
		diagnosticContext += kDiagnosticContext;
		diagnosticContext += "/UserInput";

		System::GetGUI ().getInputStats (previousStats);
	}

	// IAnalyticsEventFilter
	IAnalyticsEvent* CCL_API process (IAnalyticsEvent& e) override
	{
		if(e.getID () == AnalyticsID::kUserInputReport)
			return return_shared (&e);

		return nullptr;
	}

	void updateStatistics ()
	{
		IUserInterface::InputStats currentStats;
		System::GetGUI ().getInputStats (currentStats);

		// add count difference since last seen stats
		IUserInterface::InputStats stats (currentStats);
		stats -= previousStats;

		DiagnosticStoreAccessor diagnostics (System::GetDiagnosticStore ());
		auto addCount = [&] (StringID key, int count)
		{
			int64 value = diagnostics.getPlainValue (diagnosticContext, key);
			diagnostics.setPlainValue (diagnosticContext, key, value + count);
		};

		addCount (AnalyticsID::kInputTypeMouse, stats.mouseCount);
		addCount (AnalyticsID::kInputTypeTouch, stats.touchCount);
		addCount (AnalyticsID::kInputTypePen, stats.penCount);
		addCount (AnalyticsID::kInputTypeDrop, stats.dropCount);
		addCount (AnalyticsID::kInputTypeContextMenu, stats.contextMenuCount);
		addCount (AnalyticsID::kInputTypeKeyCommand, stats.keyCommandCount);

		previousStats = currentStats;
	}

	void CCL_API evaluateData () override
	{
		Attributes data;
		DiagnosticStoreAccessor diagnostics (System::GetDiagnosticStore ());

		for(CStringRef inputType : inputTypes)
		{
			int count = diagnostics.getPlainValue (diagnosticContext, inputType).asInt ();
			if(count > 0)
			{
				auto inputAttribs = NEW Attributes;
				inputAttribs->set (AnalyticsID::kInputType, inputType);
				inputAttribs->set (AnalyticsID::kInputCount, count);
				data.queue (AnalyticsID::kInputEvents, inputAttribs, Attributes::kOwns);
			}
		}

		if(!data.isEmpty ())
			ccl_analytics_event (AnalyticsID::kUserInputReport, &data);
	}

	void CCL_API onWriteCompleted (StringID eventId) override
	{
		// success: remove evaluated data from diagnostic store
		if(eventId == AnalyticsID::kUserInputReport)
			System::GetDiagnosticStore ().clearData (diagnosticContext, CString::kEmpty);
	}

	void CCL_API onIdle () override
	{
		int64 now = UnixTime::getTime ();
		if(now >= nextUpdate)
		{
			updateStatistics ();
			nextUpdate = now + kUpdateDelay;
		}
	}

	void CCL_API terminate () override
	{
		if(System::GetAnalyticsManager ().isTrackingActive ())
			updateStatistics ();
	}

	CLASS_INTERFACE (IAnalyticsEventFilter, Unknown)

private:
	MutableCString diagnosticContext;
	IUserInterface::InputStats previousStats;
	int64 nextUpdate;
	static constexpr int64 kUpdateDelay = Time::kSecondsPerHour; ///< for querying stats from GUI

	Vector<CString> inputTypes {
		AnalyticsID::kInputTypeTouch,
		AnalyticsID::kInputTypePen,
		AnalyticsID::kInputTypeMouse,
		AnalyticsID::kInputTypeDrop,
		AnalyticsID::kInputTypeContextMenu,
		AnalyticsID::kInputTypeKeyCommand };
};

//************************************************************************************************
// AppTrackingHandler::CommandReportFilter
//************************************************************************************************

class AppTrackingHandler::CommandReportFilter: public Unknown,
											   public AbstractAnalyticsEventFilter
{
public:
	CommandReportFilter ()
	: diagnosticPrefix  (MutableCString (kDiagnosticContext).append ("/Commands/"))
	{}

	// IAnalyticsEventFilter
	IAnalyticsEvent* CCL_API process (IAnalyticsEvent& e) override
	{
		if(e.getID () == AnalyticsID::kCommandUsed)
		{
			// track in diagnostic store: "analytics/Commands/category|name", key: invoker
			AttributeReadAccessor attributes (e.getData ());
			MutableCString context (diagnosticPrefix);
			context += attributes.getCString (AnalyticsID::kCommand);
			MutableCString invoker (attributes.getCString (AnalyticsID::kInvoker));

			System::GetDiagnosticStore ().submitValue (context, invoker, Variant ());
		}
		else if(e.getID () == AnalyticsID::kCommandUsageReport)
			return return_shared (&e);

		return nullptr;
	}

	void CCL_API evaluateData () override
	{
		Attributes data;

		MutableCString context (diagnosticPrefix);
		context.append ("*");

		for(CStringRef invoker : invokerIds)
		{
			AutoPtr<IDiagnosticResultSet> results = System::GetDiagnosticStore ().queryResults (context, invoker);
			if(results)
			{
				int numResults = results->getCount ();
				if(numResults > 0)
				{
					auto* commandReport = NEW Attributes;

					for(int i = 0; i < numResults; i++)
					{
						IDiagnosticResult* commandData = results->at (i);
						MutableCString commandId (commandData->getContext ().subString (diagnosticPrefix.length ()));

						auto commandAttribs = NEW Attributes;
						commandAttribs->set (AnalyticsID::kInvoker, invoker);
						commandAttribs->set (AnalyticsID::kCommandCount, commandData->getCount ());

						commandReport->set (commandId, commandAttribs, Attributes::kOwns);
					}

					data.set (AnalyticsID::kCommands, commandReport, Attributes::kOwns);
				}
			}
		}

		if(!data.isEmpty ())
			ccl_analytics_event (AnalyticsID::kCommandUsageReport, &data);
	}

	void CCL_API onWriteCompleted (StringID eventId) override
	{
		if(eventId == AnalyticsID::kCommandUsageReport)
		{
			// success: remove evaluated data from diagnostic store
			MutableCString context (diagnosticPrefix);
			context.append ('*');
			System::GetDiagnosticStore ().clearData (context, CString::kEmpty);
		}
	}

	CLASS_INTERFACE (IAnalyticsEventFilter, Unknown)

private:
	MutableCString diagnosticPrefix;
	Vector<CString> invokerIds { AnalyticsID::kInvokerMainMenu, AnalyticsID::kInvokerContextMenu, AnalyticsID::kInvokerKeyboard };
};

//************************************************************************************************
// AppTrackingHandler::NavigationReportFilter
//************************************************************************************************

class AppTrackingHandler::NavigationReportFilter: public Unknown,
												  public AbstractAnalyticsEventFilter
{
public:
	// IAnalyticsEventFilter
	IAnalyticsEvent* CCL_API process (IAnalyticsEvent& e) override
	{
		if(e.getID () == AnalyticsID::kNavigation)
		{
			// track in diagnostic store: "analytics/Navigation/path"
			AttributeReadAccessor attributes (e.getData ());
			String path (attributes.getString (AnalyticsID::kNavigationPath));
			path.replace (Url::strPathChar, "|"); // avoid additional slashes in diagnostic context

			MutableCString context (kDiagnosticsPrefix);
			context.append (path, Text::kUTF8);

			System::GetDiagnosticStore ().submitValue (context, AnalyticsID::kNavigationCount, Variant ());
		}
		else if(e.getID () == AnalyticsID::kNavigationReport)
			return return_shared (&e);

		return nullptr;
	}

	void CCL_API evaluateData () override
	{
		Attributes data;

		MutableCString context (kDiagnosticsPrefix);
		context.append ("*");

		AutoPtr<IDiagnosticResultSet> results = System::GetDiagnosticStore ().queryResults (context, AnalyticsID::kNavigationCount);
		if(results)
		{
			for(int i = 0; i < results->getCount (); i++)
			{
				IDiagnosticResult* result = results->at (i);

				String navigationPath (result->getContext ().subString (kDiagnosticsPrefix.length ()));

				auto pathAttribs = NEW Attributes;
				pathAttribs->set (AnalyticsID::kNavigationPath, navigationPath);
				pathAttribs->set (AnalyticsID::kNavigationCount, result->getCount ());

				data.queue (AnalyticsID::kNavigationPaths, pathAttribs, Attributes::kOwns);
			}
		}

		if(!data.isEmpty ())
			ccl_analytics_event (AnalyticsID::kNavigationReport, &data);
	}

	void CCL_API onWriteCompleted (StringID eventId) override
	{
		if(eventId == AnalyticsID::kNavigationReport)
		{
			// success: remove evaluated data from diagnostic store
			MutableCString context (kDiagnosticsPrefix);
			context.append ("*");
			System::GetDiagnosticStore ().clearData (context, CString::kEmpty);
		}
	}

	CLASS_INTERFACE (IAnalyticsEventFilter, Unknown)

private:
	static CString kDiagnosticsPrefix;
};

CString AppTrackingHandler::NavigationReportFilter::kDiagnosticsPrefix ("analytics/Navigation/");

//************************************************************************************************
// AppTrackingHandler::BrowserReportFilter
//************************************************************************************************

class AppTrackingHandler::BrowserReportFilter: public Unknown,
											   public AbstractAnalyticsEventFilter
{
public:
	// IAnalyticsEventFilter
	IAnalyticsEvent* CCL_API process (IAnalyticsEvent& e) override
	{
		if(e.getID () == AnalyticsID::kBrowserInteraction)
		{
			// track in diagnostic store: "analytics/Browser/name"
			AttributeReadAccessor attributes (e.getData ());
			String name (attributes.getString (AnalyticsID::kBrowserName));

			MutableCString context (kDiagnosticsPrefix);
			context.append (name, Text::kUTF8);

			System::GetDiagnosticStore ().submitValue (context, AnalyticsID::kBrowserInteractionCount, Variant ());
		}
		else if(e.getID () == AnalyticsID::kBrowserInteractionReport)
			return return_shared (&e);

		return nullptr;
	}

	void CCL_API evaluateData () override
	{
		Attributes data;

		MutableCString context (kDiagnosticsPrefix);
		context.append ("*");

		AutoPtr<IDiagnosticResultSet> results = System::GetDiagnosticStore ().queryResults (context, AnalyticsID::kBrowserInteractionCount);
		if(results)
		{
			for(int i = 0; i < results->getCount (); i++)
			{
				IDiagnosticResult* result = results->at (i);

				String browserName (result->getContext ().subString (kDiagnosticsPrefix.length ()));

				auto browserAttribs = NEW Attributes;
				browserAttribs->set (AnalyticsID::kBrowserName, browserName);
				browserAttribs->set (AnalyticsID::kBrowserInteractionCount, result->getCount ());

				data.queue (AnalyticsID::kBrowsers, browserAttribs, Attributes::kOwns);
			}
		}

		if(!data.isEmpty ())
			ccl_analytics_event (AnalyticsID::kBrowserInteractionReport, &data);
	}

	void CCL_API onWriteCompleted (StringID eventId) override
	{
		if(eventId == AnalyticsID::kBrowserInteractionReport)
		{
			// success: remove evaluated data from diagnostic store
			MutableCString context (kDiagnosticsPrefix);
			context.append ("*");
			System::GetDiagnosticStore ().clearData (context, CString::kEmpty);
		}
	}

	CLASS_INTERFACE (IAnalyticsEventFilter, Unknown)

private:
	static CString kDiagnosticsPrefix;
};

CString AppTrackingHandler::BrowserReportFilter::kDiagnosticsPrefix ("analytics/Browser/");

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_COMMANDS (AppTrackingHandler)
	DEFINE_COMMAND ("Help", "Usage Data Settings", AppTrackingHandler::onUsageDataCommand)
END_COMMANDS (AppTrackingHandler)

//************************************************************************************************
// AnalyticsID
//************************************************************************************************

String AnalyticsID::makeUserGroupID (int userAge, AppPlan plan)
{
	auto getAgeBracket = [] (int userAge)
	{
		// array contains start years of each age bracket, starting with index 1 ...
		// an age below the first range is treated as "unknown" (0)
		static const int startAges[] = { 13, 18, 25, 35, 45, 55, 65, NumericLimits::kMaxInt };

		int bracketIndex = 0;
		for(auto nextStart : startAges)
		{
			if(userAge < nextStart)
				break;

			bracketIndex++;
		}
		ASSERT (bracketIndex >= 0 && bracketIndex < ARRAY_COUNT (startAges))
		return bracketIndex;
	};

	String id;
	id << getAgeBracket (userAge);
	id << (plan == kSubscription ? "s" : "p");
	id << System::GetLocaleManager ().getSystemRegion ();
	id << System::GetLocaleManager ().getLanguage ();
	return id;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AnalyticsID::getUserAge (const Date& birthday)
{
	// check if birthday is valid
	if(birthday.getYear () <= 1 // (server not asked yet or date not entered)
		|| birthday.getYear () == Date ().getYear ())
		return 0;

	DateTime now;
	System::GetSystem ().getLocalTime (now);

	int months = (now.getDate ().getYear () * 12 + now.getDate ().getMonth ()) - (birthday.getYear () * 12 + birthday.getMonth ());
	return months / 12;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 AnalyticsID::hashUserSystemReport (const IAnalyticsEvent& e)
{
	int64 hash = 0;

	AttributeReadAccessor data (e.getData ());
	hash += data.getString (AnalyticsID::kOsName).getHashCode ();
	hash += data.getString (AnalyticsID::kOsVersion).getHashCode ();
	hash += data.getString (AnalyticsID::kCpuType).getHashCode ();
	hash += data.getInt64 (AnalyticsID::kCpuCores);
	hash += data.getInt64 (AnalyticsID::kRamAmount);
	hash += data.getInt64 (AnalyticsID::kDiskSize);
	hash += data.getInt64 (AnalyticsID::kMonitorCount);

	IterForEachUnknown (data.newUnknownIterator (AnalyticsID::kMonitors), m)
		UnknownPtr<IAttribute> a (m);
		if(a)
		{
			UnknownPtr<IAttributeList> monitorAttribs (a->getValue ());
			if(monitorAttribs)
			{
				AttributeReadAccessor monitorData (*monitorAttribs);
				hash += monitorData.getInt (AnalyticsID::kMonitorX);
				hash += monitorData.getInt (AnalyticsID::kMonitorY);
				hash += (int64)(monitorData.getFloat (AnalyticsID::kScaling) * 1000);
			}
		}
	EndFor

	IterForEachUnknown (data.newUnknownIterator (AnalyticsID::kColorSchemes), s)
		UnknownPtr<IAttribute> a (s);
		if(a)
		{
			UnknownPtr<IAttributeList> schemeAttribs (a->getValue ());
			if(schemeAttribs)
			{
				AttributeReadAccessor monitorData (*schemeAttribs);
				hash += int64 (monitorData.getFloat (AnalyticsID::kLuminance) * 100);
			}
		}
	EndFor

	hash += data.getInt (AnalyticsID::kWinHighDpiEnabled) * 7;
	hash += data.getInt (AnalyticsID::kMacMetalEnabled) * 7;
	return hash;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int64 AnalyticsID::quantizeTimeStamp (int64 unixTime)
{
	constexpr int64 kResolution = Time::kSecondsPerHour;
	return int64 (unixTime / kResolution + 0.5) * kResolution;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag 
{
	enum AppTrackingHandlerTags
	{
		kTrackingDecision = 100,
		kAcceptTracking,
		kDenyTracking
	};
}

//************************************************************************************************
// AppTrackingHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AppTrackingHandler, Component)
IMPLEMENT_COMMANDS (AppTrackingHandler, Component)

const String AppTrackingHandler::kSettingsId ("Analytics");
StringID AppTrackingHandler::kDiagnosticContext = CSTR ("analytics");
StringID AppTrackingHandler::kTrackingAccepted = CSTR ("accepted");
StringID AppTrackingHandler::kUserBirthday = CSTR ("datetimestamp");
StringID AppTrackingHandler::kUserSystemHash = CSTR ("systemHash");
const Date AppTrackingHandler::kBirthdayInvalid = Date (1, 1, -1);

//////////////////////////////////////////////////////////////////////////////////////////////////

AppTrackingHandler::AppTrackingHandler ()
: Component ("AppTracking"),
  autoOptInEnabled (false),
  trackingDialog (nullptr),
  startupMode (false),
  buildNumber (0),
  lastAppActivation (-1),
  nextFlush (NumericLimits::kMaxInt64),
  nextRetryWrite (NumericLimits::kMaxInt64),
  autoFlushPeriod (60 * 10),
  retryFlushPeriod (60),
  retryWritePeriod (Time::kSecondsPerHour),
  retryEvaluationPeriod (Time::kSecondsPerHour),
  trackingStarted (false),
  trackingEnabled (false),
  trackingDecided (false),
  userBirthday (kBirthdayInvalid)
{
	paramList.addParam ("trackingDecision", Tag::kTrackingDecision);
	paramList.addParam ("acceptTracking", Tag::kAcceptTracking);
	paramList.addParam ("denyTracking", Tag::kDenyTracking);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AppTrackingHandler::~AppTrackingHandler ()
{
	CCL_PRINTLN ("AppTrackingHandler dtor")
	ASSERT (trackingStarted == false)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AppTrackingHandler::initialize (IUnknown* context)
{
	restoreSettings ();
	startup ();
	return SuperClass::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AppTrackingHandler::terminate ()
{
	storeSettings ();
	shutdown ();
	return SuperClass::terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::startup ()
{
	if(trackingStarted)
		return;

	trackingStarted = true;
	
	// default configuration
	if(userTrackingId.isEmpty ())
		userTrackingId = AnalyticsID::makeUserGroupID (0, AnalyticsID::kPerpetual);
	if(applicationName.isEmpty ())
		applicationName = RootComponent::instance ().getApplicationTitle ();
	if(applicationVersion.isEmpty ())
		applicationVersion = RootComponent::instance ().getApplicationVersion ();
	if(platform.isEmpty ())
		platform = CCL_PLATFORM_ID_CURRENT;
	if(architecture.isEmpty ())
		architecture = CCL_PLATFORM_ARCH;

	System::GetAnalyticsManager ().setEventAllocator (AutoPtr<IClassAllocator> (NEW AnalyticsEventFactory));
	
	ISubject::addObserver (&System::GetGUI (), this);

	trackingPlan.restoreSettings ();

	if(!trackingDecided)
		trackingEnabled = true; // enable collecting data (not sending) while user hasn't decided
	enableTrackingInternal (trackingEnabled);

	if(trackingEnabled)
		trackingPlan.initializeLastTimeStamps (UnixTime::getTime ());

	onAppActivated (true);
	ccl_analytics_event (AnalyticsID::kAppStarted);

	triggerSystemReport ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::shutdown ()
{
	if(!trackingStarted)
		return;

	onAppActivated (false);
	trackingStarted = false;

	if(pendingWriteOperation)
		pendingWriteOperation->cancel ();

	ISubject::removeObserver (&System::GetGUI (), this);	

	System::GetAnalyticsManager ().setEventAllocator (nullptr);

	if(trackingEnabled)
	{
		enableTrackingInternal (false);
		trackingPlan.storeSettings ();
	}
	trackingPlan.terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::setTrackingEnabled (bool enabled)
{
	if(enabled != trackingEnabled)
	{
		trackingEnabled = enabled;
		enableTrackingInternal (trackingEnabled);

		if(trackingEnabled)
			trackingPlan.initializeLastTimeStamps (UnixTime::getTime (), true);

		onTrackingDecisionChanged (enabled);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::enableTrackingInternal (bool enable)
{
	if(enable)
	{
		System::GetAnalyticsManager ().addEventSink (this);

		// determine required timer period from all periodic tasks
		int64 period = kIdlePeriod;
		ccl_upper_limit (period, trackingPlan.getEvaluationPeriod ());
		ccl_upper_limit (period, getRetryFlushPeriod ());
		ccl_upper_limit (period, getRetryWritePeriod ());
		ccl_upper_limit (period, getRetryEvaluationPeriod ());

		startTimer (period * 1000 / 2, true);	
		CCL_PRINTF ("AppTrackingHandler::enableTrackingInternal: timer period: %d seconds\n", (int)delay / 1000)
	}
	else
	{
		// note: AnalyticsManager::isTrackingActive () reports false when it has no sinks
		System::GetAnalyticsManager ().removeEventSink (this);
		stopTimer ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::restoreSettings ()
{
	const Attributes& attribs (Settings::instance ().getAttributes (kSettingsId));

	// tracking consent
	int accepted = autoOptInEnabled ? 1 : -1;
	attribs.getInt (accepted, kTrackingAccepted);
	trackingDecided = accepted >= 0;
	trackingEnabled = accepted == 1;

	// user birthday
	String dateString;
	if(attribs.getString (dateString, kUserBirthday))
	{
		int64 timestamp = -1;
		if(dateString.getHexValue (timestamp))
			userBirthday = UnixTime::toUTC (timestamp).getDate ();
		else
			userBirthday (0, 0, 0); // empty string: unknown
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::storeSettings ()
{
	Attributes& attribs (Settings::instance ().getAttributes (kSettingsId));

	// tracking consent
	attribs.set (kTrackingAccepted, trackingDecided ? trackingEnabled : -1);

	// user birthday
	if(userBirthday != kBirthdayInvalid)
	{
		String dateString;
		if(userBirthday.getYear () != 0) // store empty string for unknown (not entered) date
		{
			int64 timestamp = UnixTime::fromUTC (DateTime (userBirthday));
			dateString.appendHexValue (timestamp);
		}
		attribs.set (kUserBirthday, dateString);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::runDialog (bool _startupMode)
{
	startupMode = _startupMode;
	paramList.byTag (Tag::kTrackingDecision)->setValue (trackingEnabled);

	if(auto view = getTheme ()->createView ("AppTrackingDialog", this->asUnknown ()))
	{
		ASSERT (!trackingDialog)
		trackingDialog = NEW DialogBox;
		Promise ((*trackingDialog)->runDialogAsync (view)).then ([this] (IAsyncOperation& op)
		{
			ASSERT (op.getState () == IAsyncInfo::kCompleted)

			// Always apply tracking decision, regardless of close or cancel
			// but not when already applied in paramChanged() indicated by 'apply' result
			int dialogResult = op.getResult ();
			if(dialogResult != DialogResult::kApply)
			{
				bool state = paramList.byTag (Tag::kTrackingDecision)->getValue ();
				applyTrackingDecision (state);
			}

			ASSERT (trackingDialog)
			if(trackingDialog)
			{
				delete trackingDialog;
				trackingDialog = nullptr;
			}

			startupMode = false;
		});
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::onTrackingDecisionChanged (bool state)
{
	// hook for subclass
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AppTrackingHandler::canWriteEventsNow () const
{
	// hook for subclass
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AppTrackingHandler::wasTrackingDecided () const
{
	return trackingDecided; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AppTrackingHandler::onUsageDataCommand (CmdArgs args)
{
	if(!args.checkOnly ())
		runDialog ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::applyTrackingDecision (bool state)
{
	setTrackingEnabled (state);
	trackingDecided = true;

	if(!trackingEnabled)
		removeAllData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AppTrackingHandler::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kAcceptTracking :
	case Tag::kDenyTracking :
		{
			applyTrackingDecision (param->getTag () == Tag::kAcceptTracking);

			if(trackingDialog)
			{
				(*trackingDialog)->setDialogResult (DialogResult::kApply); // see result handling in runDialog()
				(*trackingDialog)->close ();
			}
		}
		break;
	}
	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::onAppActivated (bool active)
{
	if(active)
	{
		if(lastAppActivation < 0)
			lastAppActivation = System::GetSystemTicks ();
	}
	else
	{
		if(trackingEnabled && lastAppActivation >= 0)
		{
			// track duration between activation and deactivation
			AppLaunchReportFilter::trackAppUsage ((System::GetSystemTicks () - lastAppActivation) / 1000);
			lastAppActivation = -1;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AppTrackingHandler::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IApplication::kAppActivated)
	{
		onAppActivated (true);
	}
	else if(msg == IApplication::kAppDeactivated)
	{
		onAppActivated (false);
	}
	else if(msg == IApplication::kAppSuspended)
	{
		//...
	}
	else if(msg == IApplication::kAppResumed)
	{
		//...
	}
	else if(msg == IApplication::kAppTerminates)
	{
		//...
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API AppTrackingHandler::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "trackingEnabled")
	{
		var = trackingEnabled;
		return true;
	}
	else if(propertyId == "startupMode")
	{
		var = startupMode;
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::addCommonFields (Attributes& data, StringID eventId)
{
	auto setConditional = [&] (StringID key, StringRef value)
	{
		if(!value.isEmpty ())
			data.set (key, value);
	};

	if(AnalyticsTrackingPlan::BatchGroup* batch = trackingPlan.findBatchGroup (eventId))
	{
		data.set (AnalyticsID::kBatchID, batch->getBatchID (eventId));
		data.set (AnalyticsID::kTimestamp, batch->getCurrentTimeStamp ());
		data.set (AnalyticsID::kLastTimestamp, batch->getLastTimeStamp ());
	}
	else
	{
		data.set (AnalyticsID::kTimestamp, UnixTime::getTime ());
		data.set (AnalyticsID::kLastTimestamp, AnalyticsID::quantizeTimeStamp (trackingPlan.getLastEventTimestamp (eventId)));
	}

	data.set (AnalyticsID::kApplicationName, getApplicationName ());
	data.set (AnalyticsID::kApplicationVersion, getApplicationVersion ());
	data.set (AnalyticsID::kBuildNumber, getBuildNumber ());
	data.set (AnalyticsID::kPlatform, getPlatform ());
	data.set (AnalyticsID::kArchitecture, getArchitecture ());

	data.set (AnalyticsID::kUserID, userTrackingId);

	setConditional (AnalyticsID::kApplicationFingerprint, appFingerprint);
	setConditional (AnalyticsID::kApplicationFlavor, appFlavor);
	setConditional (AnalyticsID::kApplicationBranding, appBranding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AppTrackingHandler::addEvent (IAnalyticsEvent& e)
{
	if(!trackingEnabled || !trackingOutput)
		return;

	ASSERT (eventCache.getCount () < kMaxEventCount)
	if(eventCache.getCount () >= kMaxEventCount)
		return;

	if(auto filter = trackingPlan.findFilter (e.getID ()))
	{
		AutoPtr<IAnalyticsEvent> outputEvent = filter->process (e);
		if(outputEvent)
		{
			eventCache.addShared (outputEvent);

			if((eventCache.getCount () >= kFlushAt || SKIP_CACHE) && !pendingWriteOperation)
				flushEventCache ();
			else
			{
				// schedule automatic flush (if not already scheduled earlier)
				if(getAutoFlushPeriod () >= 0)
					ccl_upper_limit (nextFlush, UnixTime::getTime () + getAutoFlushPeriod ());
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::flushEventCache ()
{
	if(canWriteEventsNow ())
		nextFlush = NumericLimits::kMaxInt64;
	else
	{
		nextFlush = UnixTime::getTime () + getRetryFlushPeriod ();
		return;
	}

	if(!trackingDecided && !isAutoOptInEnabled ())
	{
		eventCache.removeAll ();
		return;
	}

	Vector<IAnalyticsEvent*> rawEvents (eventCache.getCount ());
	for(auto event : iterate_as<AnalyticsEvent> (eventCache.getEvents ()))
	{
		#if DEBUG_LOG
		// log without common attributes
		MemoryStream jsonStream;
		JsonArchive a (jsonStream);
		a.isSuppressWhitespace (true);
		a.saveAttributes (0, event->getMutableData ());
		Streamer (jsonStream).writeChar (0);

		Time t (UnixTime::toUTC (UnixTime::getTime ()).getTime ());
		CCL_PRINTF ("%02d:%02d:%02d write event: %s %s\n", t.getHour (), t.getMinute (), t.getSecond (), event->getID ().str (), jsonStream.getBuffer ().getAddress ());
		#endif

		addCommonFields (event->getMutableData (), event->getID ());

		rawEvents.add (event);
		pendingEvents.addShared (event);
	}
	eventCache.removeAll ();

	pendingWriteOperation = trackingOutput->writeEvents (rawEvents.getItems (), rawEvents.count ());
	pendingWriteOperation->retain ();

	Promise p = static_cast<IAsyncOperation*> (pendingWriteOperation);
	p.then (this, &AppTrackingHandler::onWriteCompleted);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::onWriteCompleted (IAsyncOperation& op)
{
	if(op.getState () == IAsyncInfo::kCompleted)
	{
		// success: store last time stamp per event
		for(auto e : iterate_as<AnalyticsEvent> (pendingEvents.getEvents ()))
		{
			// store last event timestamp
			int64 timestamp = e->getTimestamp ();
			trackingPlan.setLastEventTimestamp (e->getID (), timestamp);

			if(IAnalyticsEventFilter* filter = trackingPlan.findFilter (e->getID ()))
				filter->onWriteCompleted (e->getID ());
		}
	}
	else // write failed
	{
		if(!pendingEvents.getEvents ().isEmpty ())
		{
			int64 now = UnixTime::getTime ();

			for(auto e : iterate_as<AnalyticsEvent> (pendingEvents.getEvents ()))
			{
				// find original eventId to be evaluated
				StringID inEventid = trackingPlan.getEvaluationEventId (e->getID ());
				if(!inEventid.isEmpty ())
				{
					// schedule new evaluation
					trackingPlan.setNextEvaluationTime (inEventid, now + getRetryEvaluationPeriod ());
				}
				else
				{
					// no evaluation for this event: keep event + schedule write retry
					eventsToRetry.addShared (e);
					nextRetryWrite = now + getRetryWritePeriod ();
				}
			}
		}
	}

	pendingEvents.removeAll ();
	AsyncOperation::deferDestruction (pendingWriteOperation.detach ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::tryFlush (int64 now)
{
	if(now >= nextFlush)
		flushEventCache ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::retryWrite (int64 now)
{
	if(eventsToRetry.getCount () > 0)
	{
		if(now >= nextRetryWrite)
		{
			eventCache.addAllFrom (eventsToRetry);
			eventsToRetry.removeAll ();
			nextRetryWrite = NumericLimits::kMaxInt64;

			flushEventCache ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::removeAllData ()
{
	// remove all analytics data ("analytics" and sub paths)
	MutableCString context (kDiagnosticContext);
	for(int depth = 0; depth < 3; depth++)
	{
		System::GetDiagnosticStore ().clearData (context, CString::kEmpty);
		context += "/*";
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::registerUserSystemReport (bool onlyWhenChanged)
{
	trackingPlan.addFilter (AnalyticsID::kUserSystemReport, NEW SystemReportFilter (onlyWhenChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::registerAppLaunchReport (int64 evaluationPeriod)
{
	auto filter = NEW AppLaunchReportFilter (*this);
	trackingPlan.addFilter (AnalyticsID::kAppStarted, AnalyticsID::kAppLaunchReport, filter, evaluationPeriod);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::registerDocumentUsageReport (int64 evaluationPeriod)
{
	if(!DocumentsListener::peekInstance ())
		addComponent (DocumentsListener::instance ());

	trackingPlan.addFilter ({ AnalyticsID::kDocumentCreated, AnalyticsID::kDocumentOpened}, AnalyticsID::kDocumentUsageReport, NEW DocumentUsageReportFilter, evaluationPeriod);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::registerDocumentUsageEvents ()
{
	if(!DocumentsListener::peekInstance ())
		addComponent (DocumentsListener::instance ());

	trackingPlan.addPassThroughFilter (AnalyticsID::kDocumentCreated);
	trackingPlan.addPassThroughFilter (AnalyticsID::kDocumentOpened);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::registerPageUsageReport (int64 evaluationPeriod, const StringList& perspectiveIds)
{
	if(!PageUsageListener::peekInstance ())
	{
		PageUsageListener* listener = PageUsageListener::instance ();
		listener->setPerspectiveIds (perspectiveIds);
		addComponent (listener);
	}

	trackingPlan.addFilter (AnalyticsID::kPageClosed, AnalyticsID::kPageUsageReport, NEW PageUsageReportFilter, evaluationPeriod);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::registerPageClosedEvent (const StringList& perspectiveIds)
{
	if(!PageUsageListener::peekInstance ())
	{
		PageUsageListener* listener = PageUsageListener::instance ();
		listener->setPerspectiveIds (perspectiveIds);
		addComponent (listener);
	}

	trackingPlan.addPassThroughFilter (AnalyticsID::kCommandUsed);
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::registerViewOpenReport (int64 evaluationPeriod)
{
	trackingPlan.addFilter (AnalyticsID::kViewOpened, AnalyticsID::KViewOpenReport, NEW ViewReportFilter, evaluationPeriod);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::registerFileExportReport (int64 evaluationPeriod)
{
	trackingPlan.addFilter (AnalyticsID::kFileExported, AnalyticsID::kFileExportReport, NEW FileExportReportFilter, evaluationPeriod);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::registerUserInputReport (int64 evaluationPeriod)
{
	trackingPlan.addFilter (AnalyticsID::kUserInputReport, NEW UserInputReportFilter, evaluationPeriod);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::registerCommandUsageReport (int64 evaluationPeriod)
{
	if(!CommandListener::peekInstance ())
		addComponent (CommandListener::instance ());

	trackingPlan.addFilter (AnalyticsID::kCommandUsed, AnalyticsID::kCommandUsageReport, NEW CommandReportFilter, evaluationPeriod);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::registerCommandUsedEvent ()
{
	if(!CommandListener::peekInstance ())
		addComponent (CommandListener::instance ());

	trackingPlan.addPassThroughFilter (AnalyticsID::kCommandUsed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::registerNavigationReport (int64 evaluationPeriod)
{
	trackingPlan.addFilter (AnalyticsID::kNavigation, AnalyticsID::kNavigationReport, NEW NavigationReportFilter, evaluationPeriod);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::registerBrowserInteractionReport (int64 evaluationPeriod)
{
	trackingPlan.addFilter (AnalyticsID::kBrowserInteraction, AnalyticsID::kBrowserInteractionReport, NEW BrowserReportFilter, evaluationPeriod);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::triggerLaunchReport (double trackingDuration, int numStarts, double totalSessionTime)
{
	// determine launch frequency 
	double durationDays = trackingDuration / DateTime::kSecondsInDay;
	ccl_lower_limit (durationDays, 1.); // avoid huge frequency values when the measured timeframe is very short (only expected in debug builds with DEBUG_IMPATIENTLY)

	double launchFrequency = numStarts / durationDays;
	launchFrequency = ccl_round<1> (launchFrequency);

	CCL_PRINTF ("launchFrequency: %f (%d launches in %f days)\n", launchFrequency, numStarts, durationDays)

	Attributes data;
	data.set (AnalyticsID::kLaunchFrequency, launchFrequency);
	data.set (AnalyticsID::kTotalSessionTime, totalSessionTime);

	ccl_analytics_event (AnalyticsID::kAppLaunchReport, &data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::triggerSystemReport ()
{
	Attributes computerInfo;
	System::GetSystem ().getComputerInfo (computerInfo, System::kQueryExtendedComputerInfo);

	System::MemoryInfo memoryInfo;
	System::GetSystem ().getMemoryInfo (memoryInfo);

	Attributes data;
	data.set (AnalyticsID::kOsName, computerInfo.getString (System::kOSName));
	data.set (AnalyticsID::kOsVersion, computerInfo.getString (System::kOSVersion));
	data.set (AnalyticsID::kCpuType, computerInfo.getString (System::kCPUIdentifier));
	data.set (AnalyticsID::kCpuCores, System::GetSystem ().getNumberOfCores ());
	data.set (AnalyticsID::kRamAmount, (int64)memoryInfo.physicalRAMSize);

	// size of system volume
	VolumeInfo volumeInfo;
	Url systemFolder;
	if(System::GetSystem ().getLocation (systemFolder, System::kSystemFolder))
		if(System::GetFileSystem ().getVolumeInfo (volumeInfo, systemFolder) && volumeInfo.bytesTotal > 0)
			data.set (AnalyticsID::kDiskSize, (int64)volumeInfo.bytesTotal);

	// monitor info
	int numMonitors = System::GetDesktop ().countMonitors ();
	data.set (AnalyticsID::kMonitorCount, numMonitors);
	for(auto m = 0; m < numMonitors; m++)
	{
		Rect size;
		System::GetDesktop ().getMonitorSize (size, m, false);
		float scaleFactor = System::GetDesktop ().getMonitorScaleFactor (m);
		PixelPoint pixelSize (size.getSize (), scaleFactor);

		auto monitorAttribs = NEW Attributes;
		monitorAttribs->set (AnalyticsID::kMonitorX, pixelSize.x);
		monitorAttribs->set (AnalyticsID::kMonitorY, pixelSize.y);
		monitorAttribs->set (AnalyticsID::kScaling, scaleFactor);
		data.queue (AnalyticsID::kMonitors, monitorAttribs, Attributes::kOwns);
	}

	// luminance of selected colorschemes
	if(!colorSchemeNames.isEmpty ())
	{
		if(AutoPtr<IColorSchemes> colorSchemes = ccl_new<IColorSchemes> (ClassID::ColorSchemes))
		{
			for(const String* name : colorSchemeNames)
				if(IColorScheme* colorScheme = colorSchemes->getScheme (MutableCString (*name), true))
				{
					auto schemeAttribs = NEW Attributes;
					schemeAttribs->set (AnalyticsID::kType, name);
					schemeAttribs->set (AnalyticsID::kLuminance, colorScheme->getLevel (IColorScheme::kLuminanceLevel));
					data.queue (AnalyticsID::kColorSchemes, schemeAttribs, Attributes::kOwns);
				}
		}
	}

	// platform specific settings
	#if CCL_PLATFORM_WINDOWS
	AutoPtr<Win32::IDpiInfo> dpiInfo (ccl_new<Win32::IDpiInfo> (Win32::ClassID::DpiInfo));
	if(dpiInfo)
		data.set (AnalyticsID::kWinHighDpiEnabled, dpiInfo->isDpiAwarenessEnabled ());
	#elif CCL_PLATFORM_MAC
	AutoPtr<MacOS::IMetalGraphicsInfo> metalInfo (ccl_new<MacOS::IMetalGraphicsInfo> (MacOS::ClassID::MetalGraphicsInfo));
	if(metalInfo)
		data.set (AnalyticsID::kMacMetalEnabled, metalInfo->isMetalEnabled ());
	#endif

	ccl_analytics_event (AnalyticsID::kUserSystemReport, &data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppTrackingHandler::onIdleTimer ()
{
	if(trackingEnabled)
	{
		int64 now = UnixTime::getTime ();

		trackingPlan.onTimer (now);
		retryWrite (now);
		tryFlush (now);
	}
}

//************************************************************************************************
// NullAnalyticsOutput
//************************************************************************************************

IAsyncOperation* CCL_API NullAnalyticsOutput::writeEvents (IAnalyticsEvent* events[], int count)
{
	return AsyncOperation::createCompleted ();
}
