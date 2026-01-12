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
// Filename    : ccl/app/safety/appsafetymanager.cpp
// Description : Application Safety Manager
//
//************************************************************************************************

#include "ccl/app/safety/appsafetymanager.h"
#include "ccl/app/controls/listviewmodel.h"
#include "ccl/app/documents/document.h"
#include "ccl/app/utilities/pluginclass.h"
#include "ccl/app/application.h"
#include "ccl/public/app/idocumentmetainfo.h"

#include "ccl/base/storage/archivehandler.h"
#include "ccl/base/storage/packageinfo.h"

#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/iprogressdialog.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/system/idiagnosticdataprovider.h"
#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/isafetymanager.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/cclversion.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Safety IDs
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace SafetyID
{
	const CStringPtr kBlockPluginSafetyOption = "blockPluginSafetyOption";
}
	
//************************************************************************************************
// SafetyOptionsDialog
//************************************************************************************************

class SafetyOptionsDialog: public Component
{
public:
	DECLARE_CLASS_ABSTRACT (SafetyOptionsDialog, Component)

	SafetyOptionsDialog (StringRef name = "SafetyOptions", StringID formName = "CCL/SafetyOptionsDialog", StringRef description = nullptr, ICrashReport* report = nullptr, IUnknownList* unstablePlugins = nullptr);

	PROPERTY_MUTABLE_CSTRING (formName, FormName)

	bool run ();
	void addOptionProvider (ISafetyOptionProvider& provider);
	int countOptions () const;
	
	// Component
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	struct OptionItem: SafetyOptionDescription
	{
		bool state;

		OptionItem (const SafetyOptionDescription& description)
		: SafetyOptionDescription (description),
		  state (false)
		{}
		
		OptionItem ()
		: SafetyOptionDescription (),
		  state (false)
		{}
	};

	Vector<OptionItem> options;
	String description;
	SharedPtr<ICrashReport> report;
	SharedPtr<IUnknownList> unstablePlugins;
	String crashingPlugin;

	IParameter* useOptionsParam;

	void findCrashingPlugin ();
};

//************************************************************************************************
// DiagnosticDialog
//************************************************************************************************

class DiagnosticDialog: public Component
{
public:
	DECLARE_CLASS_ABSTRACT (DiagnosticDialog, Component)

	DiagnosticDialog (DiagnosticDescription::DiagnosticCategory& categoryFlags, StringRef name = "DiagnosticsReport", StringID formName = "CCL/DiagnosticsDialog");

	PROPERTY_MUTABLE_CSTRING (formName, FormName)

	bool run ();

	// Component
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	DiagnosticDescription::DiagnosticCategory& categoryFlags;

	IParameter* errorInfoParam;
	IParameter* systemInfoParam;
	IParameter* plugInInfoParam;
	IParameter* applicationLogsParam;
	IParameter* applicationSettingsParam;
};

//************************************************************************************************
// AppSafetyManager::ServiceOptionsProvider
//************************************************************************************************

class AppSafetyManager::ServiceOptionsProvider: public Unknown,
												public ISafetyOptionProvider
{
public:
	tresult addService (const IClassDescription& description);

	static void getStartupOptionId (MutableCString& id, const IClassDescription& description);

	// ISafetOptionProvider
	tbool CCL_API checkContext (IUnknown* context) override;
	int CCL_API getOptionCount () const override;
	tbool CCL_API getOptionDescription (SafetyOptionDescription& description, int index) const override;

	CLASS_INTERFACE (ISafetyOptionProvider, Unknown)

protected:
	Vector<SharedPtr<IClassDescription>> classDescriptions;
};

//************************************************************************************************
// AppSafetyManager::PluginOptionsProvider
//************************************************************************************************

class AppSafetyManager::PluginOptionsProvider: public Unknown,
											   public ISafetyOptionProvider
{
public:
	tresult addModule (UrlRef modulePath);
	tresult addModules (const IUnknownList& modulePaths);

	static void getBlockPluginOptionId (MutableCString& id, UrlRef modulePath);
	void applyOptions ();

	// ISafetOptionProvider
	tbool CCL_API checkContext (IUnknown* context) override;
	int CCL_API getOptionCount () const override;
	tbool CCL_API getOptionDescription (SafetyOptionDescription& description, int index) const override;

	CLASS_INTERFACE (ISafetyOptionProvider, Unknown)

protected:
	Vector<Url> modules;
};

//************************************************************************************************
// ServiceFilter
//************************************************************************************************

class ServiceFilter: public Unknown,
					 public IObjectFilter
{
public:
	// IObjectFilter
	tbool CCL_API matches (IUnknown* object) const override;

	CLASS_INTERFACE (IObjectFilter, Unknown)
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("AppSafetyManager")
	XSTRING (ServiceStartupOption, "Disable %(1)")
	XSTRING (ServiceStartupExplanation, "Disable service until next time you start $APPNAME.")
	XSTRING (BlockPluginOption, "Block %(1)")
	XSTRING (BlockPluginExplanation, "Add plug-in to the Blocklist.")
	XSTRING (ApplicationCrashed, "We're sorry. $APPNAME quit unexpectedly.")
	XSTRING (ApplicationUnstable, "We're sorry. $APPNAME noticed a problem.")
	XSTRING (UnstableModules, "The following plug-ins didn't work as expected:")
	XSTRING (RestartAdvice, "Please save your work and restart $APPNAME.")
	XSTRING (OpenDocument, "Open \"%(1)\" with safety options")
	XSTRING (DiagnosticsReport, "Diagnostics Report")
END_XSTRINGS

//************************************************************************************************
// SafetyOptionsDialog
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (SafetyOptionsDialog, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

SafetyOptionsDialog::SafetyOptionsDialog (StringRef name, StringID formName, StringRef description, ICrashReport* report, IUnknownList* unstablePlugins)
: Component (name),
  formName (formName),
  description (description),
  useOptionsParam (nullptr),
  report (report),
  unstablePlugins (unstablePlugins)
{
	paramList.addParam ("openDumpFolder");
	paramList.addParam ("createDiagnosticsReport");
	useOptionsParam = paramList.addParam ("useOptions");
	useOptionsParam->setValue (true);

	findCrashingPlugin ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SafetyOptionsDialog::run ()
{
	int result = DialogResult::kCancel;
	AutoPtr<IView> view = getTheme ()->createView (formName, this->asUnknown ());
	if(view)
	{
		DialogBox dialog;
		result = dialog->runDialog (return_shared<IView> (view), Styles::kWindowCombinedStyleDialog);
	}
	if(result == DialogResult::kOkay && useOptionsParam->getValue ().asBool ())
	{
		for(OptionItem& option : options)
			System::GetSafetyManager ().setValue (option.id, option.state);
	}

	return result == DialogResult::kOkay;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyOptionsDialog::addOptionProvider (ISafetyOptionProvider& provider)
{
	SafetyOptionDescription description;
	for(int i = 0; provider.getOptionDescription (description, i); i++)
	{
		MutableCString paramId;
		paramId.appendFormat ("state[%d]", options.count ());
		paramList.addParam (paramId);
		int index = options.count ();
		for(; index > 0; index--)
			if(options[index - 1].displayPriority <= description.displayPriority)
				break;
		options.insertAt (index, description);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SafetyOptionsDialog::countOptions () const
{
	return options.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SafetyOptionsDialog::findCrashingPlugin ()
{
	crashingPlugin.empty ();
	if(report && System::GetFileSystem ().fileExists (report->getModuleCausingCrash ()))
	{
		// Check if the crashing module is a third party plug-in...
		Url crashingModule (report->getModuleCausingCrash ());
		if(PlugIn::findModulePath (crashingModule))
			crashingModule.getName (crashingPlugin);
		// ...or try to find a third-party plug-in in the call stack.
		else if(unstablePlugins)
		{
			ForEachUnknown (*unstablePlugins, unk)
				UnknownPtr<IUrl> pluginPath = unk;
				if(pluginPath)
				{
					ForEachUnknown (report->getCallingModules (), unk)
						UnknownPtr<IUrl> callingModulePath (unk);
						if(callingModulePath && pluginPath)
						{
							if(callingModulePath->isEqualUrl (*pluginPath))
							{
								callingModulePath->getName (crashingPlugin);
								break;
							}
						}
					EndFor
					if(!crashingPlugin.isEmpty ())
						break;
				}
			EndFor
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SafetyOptionsDialog::getProperty (Variant& var, MemberID propertyId) const
{
	int index = -1;
	int start = propertyId.index ("[") + 1;
	int end = propertyId.index ("]");
	CStringRef postfix = propertyId.subString (start, end);
	index = String (postfix).scanInt ();

	// description

	if(propertyId == "description")
	{
		var.fromString (description);
		return true;
	}

	// crash information
	
	else if(propertyId == "hasCrashingModule")
	{
		var = !crashingPlugin.isEmpty ();
		return true;
	}
	else if(propertyId == "crashingModule")
	{
		var.fromString (crashingPlugin);
		return true;
	}
	else if(propertyId == "hasPendingActions")
	{
		if(report == nullptr)
			return false;
		const IArrayObject& pendingActions = report->getLastActionsBeforeCrash ();
		var = pendingActions.getArrayLength () > 0;
		return true;
	}
	else if(propertyId == "numPendingActions")
	{
		if(report == nullptr)
			return false;
		const IArrayObject& pendingActions = report->getLastActionsBeforeCrash ();
		var = pendingActions.getArrayLength ();
		return true;
	}
	else if(propertyId.startsWith ("pendingAction["))
	{
		if(report == nullptr)
			return false;
		const IArrayObject& pendingActions = report->getLastActionsBeforeCrash ();
		
		if(index < 0 || index >= pendingActions.getArrayLength ())
			return false;

		return pendingActions.getArrayElement (var, index);
	}
	else if(propertyId == "hasDumpFile")
	{
		if(report == nullptr)
			return false;
		Url path = report->getSystemDumpPath ();
		path.ascend ();
		var = System::GetFileSystem ().fileExists (path);
		return true;
	}
	else if(propertyId == "dumpFolder")
	{
		if(report)
		{
			Url path = report->getSystemDumpPath ();
			path.ascend ();
			var.fromString (UrlDisplayString (path));
		}
		return true;
	}
	else if(propertyId == "hasUnstableModules")
	{
		var = unstablePlugins && !unstablePlugins->isEmpty ();
		return true;
	}
	else if(propertyId == "numUnstableModules")
	{
		var = unstablePlugins ? IterCountData (unstablePlugins->createIterator ()) : 0;
		return true;
	}
	else if(propertyId.startsWith ("unstableModule["))
	{
		if(unstablePlugins == nullptr)
			return false;
		int i = 0;
		String name;
		ForEachUnknown (*unstablePlugins, unk)
			if(i == index)
			{
				UnknownPtr<IUrl> url = unk;
				if(url)
					url->getName (name);
			}
			i++;
		EndFor
		var.fromString (name);
		return true;
	}

	// safety options

	else if(propertyId == "numOptions")
	{
		var = countOptions ();
		return true;
	}
	else if(propertyId.startsWith ("state["))
	{
		if(index < 0 || index >= options.count ())
			return false;

		var = options[index].state;
		return true;
	}
	else if(propertyId.startsWith ("title["))
	{
		if(index < 0 || index >= options.count ())
			return false;

		var.fromString (options[index].title);
		return true;
	}
	else if(propertyId.startsWith ("explanation["))
	{
		if(index < 0 || index >= options.count ())
			return false;

		var.fromString (options[index].explanationText);
		return true;
	}
	else if(propertyId.startsWith ("id["))
	{
		if(index < 0 || index >= options.count ())
			return false;

		var.fromString (String (options[index].id));
		return true;
	}

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API SafetyOptionsDialog::createView (StringID name, VariantRef data, const Rect& bounds)
{
	IView* view = getTheme ()->createView (name, this->asUnknown ());
	if(view == nullptr && name.endsWith (".safetyoption"))
		view = getTheme ()->createView ("default.safetyoption", this->asUnknown ());
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SafetyOptionsDialog::paramChanged (IParameter* param)
{
	CStringRef name = param->getName ();
	int index = -1;
	int start = name.index ("[") + 1;
	int end = name.index ("]");
	CStringRef postfix = name.subString (start, end);
	index = String (postfix).scanInt ();

	if(name.startsWith ("state["))
	{
		if(index < 0 || index >= options.count ())
			return false;

		options[index].state = param->getValue ().asBool ();
		
		if(param->getValue ().asBool ())
			if(useOptionsParam->getValue ().asBool () == false)
				useOptionsParam->setValue (true);
		
		return true;
	}
	else if(name == "openDumpFolder")
	{
		if(report)
		{
			Url path = report->getSystemDumpPath ();
			path.ascend ();
			System::GetSystemShell ().openUrl (path);
		}
	}
	else if(name == "createDiagnosticsReport")
	{
		AppSafetyManager::instance ().runDiagnosticsUI ();
	}

	return SuperClass::paramChanged (param);
}

//************************************************************************************************
// DiagnosticDialog
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DiagnosticDialog, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

DiagnosticDialog::DiagnosticDialog (DiagnosticDescription::DiagnosticCategory& categoryFlags, StringRef name, StringID formName)
: Component (name),
  formName (formName),
  categoryFlags (categoryFlags)
{
	errorInfoParam = paramList.addParam ("enableErrorInfo");
	systemInfoParam = paramList.addParam ("enableSystemInfo");
	plugInInfoParam = paramList.addParam ("enablePlugInInfo");
	applicationLogsParam = paramList.addParam ("enableApplicationLogs");
	applicationSettingsParam = paramList.addParam ("enableApplicationSettings");
	
	categoryFlags = 0;

	errorInfoParam->setValue (true, true);
	systemInfoParam->setValue (true, true);
	plugInInfoParam->setValue (true, true);
	applicationLogsParam->setValue (true, true);
	applicationSettingsParam->setValue (true, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DiagnosticDialog::run ()
{
	int result = DialogResult::kCancel;
	AutoPtr<IView> view = getTheme ()->createView (formName, this->asUnknown ());
	if(view)
		result = DialogBox ()->runDialog (view.detach (), Styles::kWindowCombinedStyleDialog);
	return result == DialogResult::kOkay;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API DiagnosticDialog::createView (StringID name, VariantRef data, const Rect& bounds)
{
	return getTheme ()->createView (name, this->asUnknown ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DiagnosticDialog::paramChanged (IParameter* param)
{
	DiagnosticDescription::DiagnosticCategory flags = 0;
	if(param == errorInfoParam)
		flags = DiagnosticDescription::kErrorInformation;
	else if(param == systemInfoParam)
		flags = DiagnosticDescription::kSystemInformation;
	else if(param == plugInInfoParam)
		flags = DiagnosticDescription::kPlugInInformation;
	else if(param == applicationLogsParam)
		flags = DiagnosticDescription::kApplicationLogs;
	else if(param == applicationSettingsParam)
		flags = DiagnosticDescription::kApplicationSettings;

	if(flags != 0)
	{
		if(param->getValue ().asBool ())
			categoryFlags |= flags;
		else
			categoryFlags &= ~flags;
	}

	return SuperClass::paramChanged (param);
}

//************************************************************************************************
// AppSafetyManager
//************************************************************************************************

DEFINE_SINGLETON (AppSafetyManager)
const String AppSafetyManager::kDiagnosticFolder ("Diagnostics");

//////////////////////////////////////////////////////////////////////////////////////////////////

AppSafetyManager::AppSafetyManager ()
: serviceOptionsProvider (NEW ServiceOptionsProvider),
  pluginOptionsProvider (NEW PluginOptionsProvider),
  safetySink (Signals::kSafetyManagement)
{
	addOptionProvider (serviceOptionsProvider);
	addOptionProvider (pluginOptionsProvider);
	System::GetSafetyManager ().addFilter (NEW ServiceFilter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AppSafetyManager::startup (bool forceDialog)
{
	startTimer (kIdleDelay, true);
	
	safetySink.setObserver (this);
	safetySink.enable (true);
	
	System::GetServiceManager ().registerNotification (this);

	registerServiceStartupOptions (PLUG_CATEGORY_USERSERVICE);
	registerServiceStartupOptions (PLUG_CATEGORY_PROGRAMSERVICE);
	registerServiceStartupOptions (PLUG_CATEGORY_FRAMEWORKSERVICE);

	ISafetyManager& manager = System::GetSafetyManager ();
	AutoPtr<ICrashReport> report = manager.detectCrash ();
	
	AutoPtr<UnknownList> unstablePlugins;
		
	if(report)
	{ 
		unstablePlugins = NEW UnknownList;

		// Try to add modules from the call stack first. 
		// For a plug-in shell, we want to block the shell, not subsequently loaded libraries.
		PlugIn::findModulePaths (*unstablePlugins, report->getCallingModules ());
		
		// If we could not find a plug-in library in the call stack, add the crashing module directly.
		Url crashingModule (report->getModuleCausingCrash ());
		if(unstablePlugins->isEmpty () && PlugIn::findModulePath (crashingModule))
			pluginOptionsProvider->addModule (crashingModule);
		
		// Also add unstable modules.
		PlugIn::findModulePaths (*unstablePlugins, report->getUnstableModules ());

		pluginOptionsProvider->addModules (*unstablePlugins);
	}

	if(forceDialog || report != nullptr)
	{
		String description;
		if(report)
		{
			if(report->didShutdownCleanly ())
				description.append (XSTR (ApplicationUnstable));
			else
				description.append (XSTR (ApplicationCrashed));
		}
		return showAppSafetyOptions (description, report, unstablePlugins);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppSafetyManager::shutdown ()
{
	stopTimer ();
	System::GetServiceManager ().unregisterNotification (this);
	safetySink.enable (false);
	optionProviders.removeAll ();
	diagnosticProviders.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppSafetyManager::addOptionProvider (ISafetyOptionProvider* provider)
{
#if DEBUG
	ForEachUnknown (optionProviders, unk)
		ASSERT (provider != unk)
	EndFor
#endif
	optionProviders.add (provider, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppSafetyManager::addDiagnosticProvider (IDiagnosticDataProvider* provider)
{
#if DEBUG
	ForEachUnknown (diagnosticProviders, unk)
		ASSERT (provider != unk)
	EndFor
#endif
	diagnosticProviders.add (provider, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AppSafetyManager::addDiagnosticProvider (IUnknown* unknown)
{
	UnknownPtr<IDiagnosticDataProvider> provider (unknown);
	if(!provider.isValid ())
		return false;

#if DEBUG
	ForEachUnknown (diagnosticProviders, unk)
		ASSERT (provider != unk)
	EndFor
#endif
	return diagnosticProviders.add (provider.detach (), false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AppSafetyManager::showDocumentSafetyOptions (Document* document)
{
	SafetyOptionsDialog dialog ("SafetyOptions", "CCL/SafetyOptionsDialog", document ? String ().appendFormat (XSTR (OpenDocument), document->getTitle ()) : nullptr);
	
	ForEachUnknown (optionProviders, unk)
		UnknownPtr<ISafetyOptionProvider> provider = unk;
		if(provider)
			if(document == nullptr || provider->checkContext (ccl_as_unknown (document)))
				dialog.addOptionProvider (*provider);
	EndFor

	if(dialog.countOptions () > 0)
		return dialog.run ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppSafetyManager::resetDocumentSafetyOptions (Document* document)
{
	ISafetyManager& manager = System::GetSafetyManager ();

	ForEachUnknown (optionProviders, unk)
		UnknownPtr<ISafetyOptionProvider> provider = unk;
		if(provider)
		{
			if(document == nullptr || provider->checkContext (ccl_as_unknown (document)))
			{
				SafetyOptionDescription description;
				for(int i = 0; provider->getOptionDescription (description, i); i++)
					manager.setValue (description.id, false);
			}
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppSafetyManager::getActiveSafetyOptions (Vector<SafetyOptionDescription>& options, IUnknown* context) const
{
	ISafetyManager& manager = System::GetSafetyManager ();

	ForEachUnknown (optionProviders, unk)
		UnknownPtr<ISafetyOptionProvider> provider = unk;
		if(provider)
		{
			if(provider->checkContext (context))
			{
				SafetyOptionDescription description;
				for(int i = 0; provider->getOptionDescription (description, i); i++)
					if(manager.getValue (description.id))
						options.add (description);
			}
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppSafetyManager::getActiveSafetyOptionsText (String& safetyOptions, IUnknown* context) const
{
	Vector<SafetyOptionDescription> options;
	getActiveSafetyOptions (options, context);
	for(const SafetyOptionDescription& description : options)
		safetyOptions.appendFormat (" - %(1)%(2)", description.title, String::getLineEnd ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppSafetyManager::getDiagnosticsFolder (IUrl& folder) const
{
	System::GetSystem ().getLocation (folder, System::kUserContentFolder);
	folder.descend (String (kDiagnosticFolder));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AppSafetyManager::runDiagnosticsUI () const
{	
	DiagnosticDescription::DiagnosticCategory categoryFlags = 0;
	DiagnosticDialog diagnosticDialog (categoryFlags);
	if(!diagnosticDialog.run ())
		return kResultAborted;

	if(categoryFlags == 0)
		return kResultOk;

	AutoPtr<IProgressNotify> progress = ccl_new<IProgressNotify> (ClassID::ProgressDialog);
	progress->setTitle (XSTR (DiagnosticsReport));
	UnknownPtr<IProgressDialog> dialog (progress);
	if(dialog)
		dialog->setOpenDelay (1.); // do not open immediately

	ProgressNotifyScope scope (progress);
	
	LegalFileName fileName (String () << RootComponent::instance ().getApplicationTitle () << " " << XSTR (DiagnosticsReport));
	System::GetFileUtilities ().appendDateTime (fileName);

	Url path;
	getDiagnosticsFolder (path);
	path.descend (fileName, IUrl::kFile);
	path.setExtension (FileTypes::Zip ().getExtension ());
	
	AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().createPackage (path, ClassID::ZipFile);
	if(packageFile == nullptr)
		return kResultFailed;

	packageFile->setOption (PackageOption::kCompressed, true);
	if(!packageFile->create (IStream::kCreateMode))
		return kResultFailed;

	IFileSystem* fileSystem = packageFile->getFileSystem ();
	if(fileSystem == nullptr)
		return kResultFailed;
	
	int totalCount = IterCountData (diagnosticProviders.createIterator ());
	int count = 0;

	ArchiveHandler archiveHandler (*fileSystem);	
	PackageInfo metaInfo;
	metaInfo.set (Meta::kDocumentMimeType, CCL_MIME_TYPE "-diagnostics-report");
	metaInfo.set (Meta::kDocumentGenerator, RootComponent::instance ().getGeneratorName ());
	metaInfo.saveWithHandler (archiveHandler);
		
	ForEachUnknown (diagnosticProviders, unk)
		UnknownPtr<IDiagnosticDataProvider> provider = unk;
		if(provider)
		{
			for(int i = 0; i < provider->countDiagnosticData (); i++)
			{
				DiagnosticDescription description;
				provider->getDiagnosticDescription (description, i);

				if((description.categoryFlags & categoryFlags) == 0)
					continue;

				AutoPtr<IStream> storable = provider->createDiagnosticData (i);
				if(storable)
				{
					String filePath (description.fileName);
					if(!description.subFolder.isEmpty ())
					{
						description.subFolder.append ("/");
						filePath.prepend (description.subFolder);
					}
					if(description.fileType.isValid ())
					{
						filePath.append (".");
						filePath.append (description.fileType.getExtension ());
					}
					archiveHandler.addSaveTask (filePath, *storable, nullptr);
				}
			}
		}
		count++;
		progress->updateProgress ((double)count / totalCount);
		if(progress->isCanceled ())
			break;
	EndFor
		
	if(progress->isCanceled ())
		return kResultAborted;

	AutoPtr<IProgressNotify> subProgress = progress->createSubProgress ();
	packageFile->flush (subProgress);
	
	System::GetSystemShell ().showFile (path);

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppSafetyManager::reportUnstablePlugins (IUnknownList& unstablePlugins) const
{
	String message (XSTR (UnstableModules));
			
	bool anyPlugInFound = false;
	ForEachUnknown (unstablePlugins, unk)
		UnknownPtr<IUrl> url (unk);
		if(url && PlugIn::findModulePath (*url))
		{
			String moduleName;
			url->getName (moduleName);
			message.appendFormat (" %(1)", moduleName);
			anyPlugInFound = true;
		}
	EndFor
				
	message.append (String::getLineEnd ());
	message.append (XSTR (RestartAdvice));

	if(anyPlugInFound)
		Alert::notify (message, Alert::kWarning);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AppSafetyManager::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kModuleException && msg.getArgCount () > 0)
	{
		UnknownPtr<IUnknownList> list (msg[0]);
		if(list)
			reportUnstablePlugins (*list);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppSafetyManager::onIdleTimer ()
{
	System::GetSafetyManager ().checkStability ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AppSafetyManager::onServiceNotification (const IServiceDescription& description, int eventCode)
{
	UnknownPtr<ISafetyOptionProvider> provider = description.getServiceInstance ();
	if(provider)
	{
		if(eventCode == kServiceStarted)
			optionProviders.add (provider, true);
		else if(eventCode == kServiceStopped)
		{
			optionProviders.remove (provider);
			provider.release ();
		}
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppSafetyManager::registerServiceStartupOptions (StringRef category)
{
	ForEachPlugInClass (category, desc)
		serviceOptionsProvider->addService (desc);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AppSafetyManager::showAppSafetyOptions (StringRef description, ICrashReport* report, IUnknownList* unstablePlugins)
{
	IApplication* app = Application::getApplication ();
	
	SafetyOptionsDialog dialog ("SafetyOptions", "CCL/AppSafetyOptionsDialog", description, report, unstablePlugins);
	
	ForEachUnknown (optionProviders, unk)
		UnknownPtr<ISafetyOptionProvider> provider = unk;
		if(provider)
			if(app == nullptr || provider->checkContext (app))
				dialog.addOptionProvider (*provider);
	EndFor

	bool succeeded = true;

	if(dialog.countOptions () > 0)
	{
		succeeded = dialog.run ();
		if(succeeded)
			applyAppSafetyOptions ();
	}
	return succeeded;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppSafetyManager::applyAppSafetyOptions ()
{
	pluginOptionsProvider->applyOptions ();
}

//************************************************************************************************
// AppSafetyManager::ServiceOptionsProvider
//************************************************************************************************

tresult AppSafetyManager::ServiceOptionsProvider::addService (const IClassDescription& description)
{
	Variant value;
	if(!description.getClassAttribute (value, SafetyID::kStartupSafetyOption))
		return kResultInvalidArgument;
	
	for(IClassDescription* classDescription : classDescriptions)
		if(classDescription == &description)
			return kResultAlreadyExists;

	if(classDescriptions.add (const_cast<IClassDescription*> (&description)))
		return kResultOk;

	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppSafetyManager::ServiceOptionsProvider::getStartupOptionId (MutableCString& id, const IClassDescription& description)
{ 
	id = SafetyID::kStartupSafetyOption;
	char cidString[128] = { 0 };
	description.getClassID ().toCString (cidString, sizeof(cidString));
	id.appendFormat (".%s", cidString);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool AppSafetyManager::ServiceOptionsProvider::checkContext (IUnknown* context)
{
	UnknownPtr<IApplication> app = context;
	return app != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AppSafetyManager::ServiceOptionsProvider::getOptionCount () const
{
	return classDescriptions.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool AppSafetyManager::ServiceOptionsProvider::getOptionDescription (SafetyOptionDescription& description, int index) const
{
	int currentIndex = 0;
	
	for(IClassDescription* classDescription : classDescriptions)
	{
		if(currentIndex == index)
		{
			getStartupOptionId (description.id, *classDescription);

			String title;
			classDescription->getLocalizedName (title);

			String descriptionText;
			classDescription->getLocalizedDescription (descriptionText);

			description.title = String ().appendFormat (XSTR (ServiceStartupOption), title);
			description.explanationText = XSTR (ServiceStartupExplanation);
			description.explanationText.append (String::getLineEnd ());
			description.explanationText.append (descriptionText);
			return true;
		}

		currentIndex++;
	}

	return false;
}

//************************************************************************************************
// AppSafetyManager::PluginOptionsProvider
//************************************************************************************************

tresult AppSafetyManager::PluginOptionsProvider::addModule (UrlRef modulePath)
{
	if(modules.contains (modulePath))
		return kResultAlreadyExists;

	modules.add (modulePath);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult AppSafetyManager::PluginOptionsProvider::addModules (const IUnknownList& modulePaths)
{
	tresult result = kResultFailed;

	ForEachUnknown (modulePaths, unk)
		UnknownPtr<IUrl> modulePath = unk;
		if(modulePath)
		{
			if(modules.contains (*modulePath))
				continue;
			modules.add (*modulePath);
			result = kResultOk;
		}
	EndFor

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AppSafetyManager::PluginOptionsProvider::getBlockPluginOptionId (MutableCString& id, UrlRef modulePath)
{ 
	id = SafetyID::kBlockPluginSafetyOption;
	String pathName, fileName;
	modulePath.getPathName (pathName);
	modulePath.getName (fileName);
	id.appendFormat (".%d/%s", pathName.getHashCode (), MutableCString (fileName, Text::kUTF8).str ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL::AppSafetyManager::PluginOptionsProvider::applyOptions ()
{
	bool wantBlocklistEnabled = System::GetPlugInManager ().enableBlocklist (true);
	MutableCString optionId;
	for(UrlRef modulePath : modules)
	{
		getBlockPluginOptionId (optionId, modulePath);
		if(System::GetSafetyManager ().getValue (optionId))
		{
			System::GetPlugInManager ().addToBlocklist (modulePath);
			wantBlocklistEnabled = true;
		}
	}
	System::GetPlugInManager ().enableBlocklist (wantBlocklistEnabled);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool AppSafetyManager::PluginOptionsProvider::checkContext (IUnknown* context)
{
	UnknownPtr<IApplication> app = context;
	return app != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AppSafetyManager::PluginOptionsProvider::getOptionCount () const
{
	return modules.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool AppSafetyManager::PluginOptionsProvider::getOptionDescription (SafetyOptionDescription& description, int index) const
{
	int currentIndex = 0;
	
	for(UrlRef module : modules)
	{
		if(currentIndex == index)
		{
			getBlockPluginOptionId (description.id, module);

			String name;
			module.getName (name);

			description.title = String ().appendFormat (XSTR (BlockPluginOption), name);
			description.explanationText = XSTR (BlockPluginExplanation);
			description.displayPriority = 20;
			return true;
		}

		currentIndex++;
	}

	return false;
}

//************************************************************************************************
// ServiceFilter
//************************************************************************************************

tbool CCL_API ServiceFilter::matches (IUnknown* object) const
{
	UnknownPtr<IClassDescription> classDescription = object;
	if(classDescription)
	{
		MutableCString optionId;
		AppSafetyManager::ServiceOptionsProvider::getStartupOptionId (optionId, *classDescription);

		return System::GetSafetyManager ().getValue (optionId);
	}

	return false;
}
