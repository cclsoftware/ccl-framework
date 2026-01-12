//************************************************************************************************
//
// (Service Template)
// (Service Copyright)
//
// Filename    : servicetemplate.cpp
// Description : (Service Template)
//
//************************************************************************************************

#include "servicetemplate.h"

using namespace ServiceNamespace;
using namespace CCL;

//************************************************************************************************
// ServiceTemplate
//************************************************************************************************

tresult CCL_API ServiceTemplate::initialize (IUnknown* context)
{
	return ServicePlugin::initialize (context);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ServiceTemplate::terminate ()
{
	return ServicePlugin::terminate ();
}
