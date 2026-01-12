//************************************************************************************************
//
// (Service Template)
// (Service Copyright)
//
// Filename    : servicetemplate.h
// Description : (Service Template)
//
//************************************************************************************************

#ifndef _servicetemplate_h
#define _servicetemplate_h

#include "ccl/public/plugins/serviceplugin.h"

namespace ServiceNamespace {

//************************************************************************************************
// ServiceTemplate
//************************************************************************************************

class ServiceTemplate: public CCL::ServicePlugin
{
public:
	// ServicePlugin
	CCL::tresult CCL_API initialize (IUnknown* context = 0) override;
	CCL::tresult CCL_API terminate () override;
};

} // ServiceNamespace

#endif // _servicetemplate_h
