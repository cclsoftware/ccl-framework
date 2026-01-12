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
// Filename    : ccl/main/cclmain.cpp
// Description : GUI Application Main
//
//************************************************************************************************

#include "ccl/base/kernel.h"
#include "ccl/app/application.h"

#include "ccl/main/cclargs.h"
#include "ccl/main/cclterminate.h"
#if CCL_STATIC_LINKAGE
#include "ccl/main/cclstatic.h"
#else
#include "ccl/main/cclinit.h"
#endif

#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/iinterprocess.h"

extern void ccl_app_init (); // external!

namespace CCL {

const ArgumentList* g_ArgumentList = nullptr;

//************************************************************************************************
// ApplicationStartup
/** Helper for application startup/shutdown. */
//************************************************************************************************

class ApplicationStartup: public Object,
						  public IApplicationProvider
{
public:
	ApplicationStartup ()
	: arguments (nullptr) 
	{}

	bool beforeInit (ModuleRef module, const PlatformArgs& args)
	{
		#if CCL_STATIC_LINKAGE
		if(!frameworkInitializer.initializeFrameworkLevel ())
			return false;	
		#else
		frameworkInitializer.init ();
		#endif

		// now that ccltext has been initialized, MutableArgumentList can be used
		arguments = NEW MutableArgumentList (args);
		g_ArgumentList = arguments;

		if(!System::GetGUI ().startup (module, this))
			return false;

		ccl_app_init ();

		Application* application = Application::getApplication ();
		ASSERT (application != nullptr)
		if(application)
		{
			if(application->isSingleInstance ())
			{
				// check for existing instance...
				if(enterInstanceLock (application->getApplicationID ()) == false)
				{
					application->beforeInitialize (); // load translations before displaying message

					if(!System::GetGUI ().activateApplication (true, *arguments))
						Alert::warn (ApplicationStrings::AlreadyRunning ());

					Kernel::instance ().destroy ();
					destruct ();
					return false;
				}
			}

			application->beforeInitialize ();
		}

		return true;
	}

	void doExit ()
	{
		cleanup ();
		checkRestart ();
		destruct ();
	}

	// IApplicationProvider
	tbool CCL_API onInit () override
	{
		if(!Kernel::instance ().initialize ())
			return false;

		if(Application* application = Application::getApplication ())
			application->processCommandLine (*g_ArgumentList);
		
		return true;
	}

	void CCL_API onExit () override
	{
		cleanup ();
		
		destruct (); // control is not returnd to main function when process terminates via exit().
	}

	IApplication* CCL_API getApplication () override
	{
		return Application::getApplication ();
	}

	CLASS_INTERFACE (IApplicationProvider, Object)

protected:
	#if CCL_STATIC_LINKAGE
	FrameworkInitializerStatic frameworkInitializer;
	#else
	FrameworkInitializer frameworkInitializer;
	#endif
	AutoPtr<Threading::ISemaphore> instanceLock;
	MutableArgumentList* arguments;

	void cleanup ()
	{
		#if CCL_STATIC_LINKAGE
		frameworkInitializer.terminateApplicationLevel ();
		#else
		Kernel::instance ().terminate ();
		Kernel::instance ().destroy (); // make sure all application objects are gone before frameworks terminate
		#endif

		ccl_terminate ();
	}

	void checkRestart ()
	{
		if(RootComponent::instance ().isRestartRequested ())
		{
			exitInstanceLock ();

			String args[1] = {String ("/restart")};
			System::GetExecutableLoader ().relaunch (ArgumentList (1, args));
		}
	}

	void destruct ()
	{
		System::GetGUI ().shutdown ();

		exitInstanceLock ();

		delete arguments;
		arguments = nullptr;
		g_ArgumentList = nullptr;

		#if CCL_STATIC_LINKAGE
		frameworkInitializer.terminateFrameworkLevel ();
		#else
		frameworkInitializer.exit ();
		#endif
	}

	bool enterInstanceLock (StringID appId)
	{
		#if CCL_PLATFORM_MAC || CCL_PLATFORM_IOS || CCL_PLATFORM_ANDROID
		return true;
		#else
		instanceLock = System::CreateIPCSemaphore ();
		return instanceLock->create (appId) == kResultOk;
		#endif
	}

	void exitInstanceLock () 
	{
		if(instanceLock)
			instanceLock->close ();
		instanceLock.release (); 
	}
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Service APIs (linked locally)
//////////////////////////////////////////////////////////////////////////////////////////////////

static ModuleRef g_ModuleReference = nullptr;

ModuleRef System::GetCurrentModuleRef ()
{
	return g_ModuleReference;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_main_gui_init, ccl_main_gui_exit: separate init/exit phases
//////////////////////////////////////////////////////////////////////////////////////////////////

static ApplicationStartup appStartup;

bool ccl_main_gui_init (ModuleRef module, const PlatformArgs& args)
{
	g_ModuleReference = module;
	return appStartup.beforeInit (module, args);
}

void ccl_main_gui_exit ()
{
	appStartup.doExit ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_main_gui
//////////////////////////////////////////////////////////////////////////////////////////////////

int ccl_main_gui (ModuleRef module, const PlatformArgs& args)
{
	if(!ccl_main_gui_init (module, args))
		return 0;

	int exitCode = System::GetGUI ().runEventLoop ();

	ccl_main_gui_exit ();

	return exitCode;
}
