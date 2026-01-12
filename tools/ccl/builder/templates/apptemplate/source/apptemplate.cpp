//************************************************************************************************
//
// Application Template
// (Application Copyright)
//
// Filename    : apptemplate.cpp
// Description : Application Template
//
//************************************************************************************************

#define SERVICES_ENABLED !CCL_STATIC_LINKAGE

#include "apptemplate.h"
#include "appversion.h"

#include "ccl/base/development.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/plugins/iservicemanager.h"
#include "ccl/public/plugservices.h"

using namespace AppNamespace;
using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

void ccl_app_init ()
{
	NEW AppTemplate;
}

//************************************************************************************************
// AppTemplate
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AppTemplate, Application)

//////////////////////////////////////////////////////////////////////////////////////////////////

AppTemplate::AppTemplate ()
: Application (APP_ID, APP_COMPANY, APP_NAME, APP_PACKAGE_ID, APP_VERSION)
{
	setWebsite (APP_WEBSITE);
	setBuildInformation (APP_FULL_NAME);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AppTemplate::startup ()
{
	if(!SuperClass::startup ())
		return false;

	// load theme
	Url skinFolder;
	GET_DEVELOPMENT_FOLDER_LOCATION (skinFolder, "applications", "apptemplate/skin")
	if(!loadTheme (skinFolder))
		return false;

	#if CCL_PLATFORM_DESKTOP
	// create main window
	createWindow ();
	#endif

	// scan plug-ins + start services
	#if SERVICES_ENABLED
	scanPlugIns ();
	System::GetServiceManager ().startup ();
	#endif

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AppTemplate::shutdown ()
{
	// stop services
	#if SERVICES_ENABLED
	System::GetServiceManager ().shutdown ();
	#endif

	return SuperClass::shutdown ();
}
