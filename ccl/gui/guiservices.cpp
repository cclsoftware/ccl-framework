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
// Filename    : ccl/gui/guiservices.cpp
// Description : GUI Service APIs
//
//************************************************************************************************

#include "ccl/base/kernel.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/configuration.h"
#include "ccl/base/unittest.h"

#include "ccl/gui/scriptgui.h"
#include "ccl/gui/skin/skinelement.h" // for type library
#include "ccl/gui/theme/visualstyleclass.h"
#include "ccl/gui/skin/coreskinmodel.h"
#include "ccl/gui/graphics/graphicshelper.h"
#include "ccl/gui/windows/tooltip.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/plugins/classfactory.h"
#include "ccl/public/plugins/icoderesource.h"
#include "ccl/public/plugins/itypelibregistry.h"
#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/cclversion.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT Configuration::IRegistry& CCL_API System::CCL_ISOLATED (GetFrameworkConfiguration) ()
{
	return Configuration::Registry::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization
//////////////////////////////////////////////////////////////////////////////////////////////////

static bool _InitializeGUIFramework (bool state)
{
	if(state)
	{
		// load configuration of host process (optional)
		const String kConfigFileName ("cclgui.config");
		ResourceUrl configResourcePath (System::GetMainModuleRef (), kConfigFileName);
		bool succeeded = Configuration::Registry::instance ().loadFromFile (configResourcePath);

		// check for configuration file next to executable to overwrite for testing purposes
		Url appSupportFolder;
		System::GetSystem ().getLocation (appSupportFolder, System::kAppSupportFolder);
		Url userConfigPath (appSupportFolder);
		userConfigPath.descend (kConfigFileName);
		succeeded |= Configuration::Registry::instance ().loadFromFile (userConfigPath);

		#if !CCL_STATIC_LINKAGE
		// check for configuration next to framework DLL when used in non-CCL host (TODO: or use a built-in default file?)
		if(succeeded == false)
		{
			if(AutoPtr<IExecutableImage> dllImage = System::GetExecutableLoader ().createImage (System::GetCurrentModuleRef ()))
			{
				Url dllPath;
				if(dllImage->getPath (dllPath))
				{
					dllPath.ascend ();
					if(dllPath != appSupportFolder)
					{
						#if CCL_PLATFORM_MAC
						// look in resources folder of the bundle which contains this cclgui.framework
						dllPath.ascend ();
						dllPath.descend (CCLSTR ("Resources"), ResourceUrl::kFolder);
						#endif
						dllPath.descend (kConfigFileName);
						Configuration::Registry::instance ().loadFromFile (dllPath);
					}
				}
			}
		}
		#endif // !CCL_STATIC_LINKAGE
	}
	else
	{
		// nothing here
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Test Collection
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_ADD_TEST_COLLECTION (InternalGUITests)

//////////////////////////////////////////////////////////////////////////////////////////////////

#if CCL_STATIC_LINKAGE
tbool CCL_API System::InitializeGUIFramework (tbool state)
{
	#if CCL_PLATFORM_DESKTOP
	TooltipFactory::linkTooltipFactory ();
	#endif
	return _InitializeGUIFramework (state != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#else
CCL_KERNEL_INIT_LEVEL (GUIClasses, kFrameworkLevelFirst)
{
	_InitializeGUIFramework (true);

	System::GetExecutableLoader ().addNativeImage (System::GetCurrentModuleRef ());

	ClassFactory* classFactory = ClassFactory::instance ();
	VersionDesc version (CCL_PRODUCT_NAME,
						 CCL_VERSION_STRING,
						 CCL_AUTHOR_NAME,
						 CCL_AUTHOR_COPYRIGHT,
						 CCL_PRODUCT_WEBSITE);

	classFactory->setVersion (version);
	CCL_REGISTER_TEST_COLLECTION (classFactory,
								  UID (0xF227E81A, 0x200C, 0xC14F, 0x89, 0x91, 0x18, 0x3B, 0x30, 0x8B, 0x17, 0x67),
								  InternalGUITests)

	Kernel::instance ().registerPublicClasses (*classFactory);
	System::GetPlugInManager ().registerFactory (classFactory);
	classFactory->release ();

	System::GetObjectTable ().registerObject (Configuration::Registry::instance ().asUnknown (), kNullUID, "FrameworkConfiguration");

	System::GetScriptingManager ().startup (CCLGUI_PACKAGE_ID, System::GetCurrentModuleRef (), nullptr, false);
	System::GetScriptingManager ().getHost ().registerObject ("GUI", ScriptGUIHost::instance ());
	System::GetScriptingManager ().getHost ().registerObject ("Graphics", GraphicsHelper::instance ());
	System::GetPlugInManager ().addHook (ScriptGUIHost::instance ().getHook ());

	// register type libraries
	MetaClassRegistry& typeLib = Kernel::instance ().getClassRegistry ();
	typeLib.setLibName (CCLGUI_FILE_DESCRIPTION);
	System::GetTypeLibRegistry ().registerTypeLib (typeLib);
	System::GetTypeLibRegistry ().registerTypeLib (SkinElements::MetaElement::getTypeLibrary ());
	System::GetTypeLibRegistry ().registerTypeLib (VisualStyleClass::getTypeLibrary ());
	System::GetTypeLibRegistry ().registerTypeLib (CoreSkinModel::getTypeLibrary ());

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (GUIClasses2, kFrameworkLevelSecond + 1) // after translations have been loaded
{
	// register commands
	CommandRegistry::registerWithCommandTable ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_TERM_LEVEL (GUIClasses, kFrameworkLevelFirst)
{
	// unregister type libraries
	System::GetTypeLibRegistry ().unregisterTypeLib (CoreSkinModel::getTypeLibrary ());
	System::GetTypeLibRegistry ().unregisterTypeLib (SkinElements::MetaElement::getTypeLibrary ());
	System::GetTypeLibRegistry ().unregisterTypeLib (VisualStyleClass::getTypeLibrary ());
	System::GetTypeLibRegistry ().unregisterTypeLib (Kernel::instance ().getClassRegistry ());

	ClassFactory* classFactory = ClassFactory::instance ();
	System::GetPlugInManager ().unregisterFactory (classFactory);
	classFactory->release ();

	System::GetObjectTable ().unregisterObject (Configuration::Registry::instance ().asUnknown ());

	System::GetPlugInManager ().removeHook (ScriptGUIHost::instance ().getHook ());
	System::GetScriptingManager ().getHost ().unregisterObject (ScriptGUIHost::instance ());
	System::GetScriptingManager ().getHost ().unregisterObject (GraphicsHelper::instance ());
	System::GetScriptingManager ().shutdown (System::GetCurrentModuleRef (), false);

	System::GetExecutableLoader ().removeNativeImage (System::GetCurrentModuleRef ());

	_InitializeGUIFramework (false);
}
#endif // !CCL_STATIC_LINKAGE
