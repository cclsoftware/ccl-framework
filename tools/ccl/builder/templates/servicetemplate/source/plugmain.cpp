//************************************************************************************************
//
// (Service Template)
// (Service Copyright)
//
// Filename    : plugmain.cpp
// Description : (Service Template) Plug-in Entry
//
//************************************************************************************************

#include "servicetemplate.h"
#include "plugversion.h"

#include "ccl/app/modulecomponent.h"
#include "ccl/public/plugins/classfactory.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Version
//////////////////////////////////////////////////////////////////////////////////////////////////

static VersionDesc version 
(
	PLUG_NAME, 
	PLUG_VERSION, 
	VENDOR_NAME,
	VENDOR_COPYRIGHT,
	VENDOR_WEBSITE
);

//////////////////////////////////////////////////////////////////////////////////////////////////
// Exported classes
//////////////////////////////////////////////////////////////////////////////////////////////////

static ClassDesc serviceClass
(
	UID (1),
	PLUG_CATEGORY_PROGRAMSERVICE,
	PLUG_NAME
);

//////////////////////////////////////////////////////////////////////////////////////////////////
// ccl_module_main
//////////////////////////////////////////////////////////////////////////////////////////////////

bool ccl_module_main (int reason)
{
	if(reason == kModuleInit)
	{
		NEW ModuleComponent (PLUG_ID, VENDOR_NAME, PLUG_NAME);
		return true;
	}
	else
		return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// CCLGetClassFactory
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IClassFactory* CCL_API CCLGetClassFactory ()
{
	ClassFactory* factory = ClassFactory::instance ();
	if(factory->isEmpty ())
	{
		factory->setVersion (version);
		factory->setLocalizationEnabled (true);
		factory->registerClass (serviceClass, PluginConstructor<ServiceNamespace::ServiceTemplate, IComponent>::createInstance);
	}	
	return factory;
}
