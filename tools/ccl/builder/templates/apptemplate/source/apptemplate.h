//************************************************************************************************
//
// Application Template
// (Application Copyright)
//
// Filename    : apptemplate.h
// Description : Application Template
//
//************************************************************************************************

#ifndef _apptemplate_h
#define _apptemplate_h

#include "ccl/app/application.h"

namespace AppNamespace {

//************************************************************************************************
// AppTemplate
//************************************************************************************************

class AppTemplate: public CCL::Application
{
public:
	DECLARE_CLASS (AppTemplate, Application)

	AppTemplate ();

	// Application
	bool startup () override;
	bool shutdown () override;
};

} // namespace AppNamespace

#endif // _apptemplate_h
