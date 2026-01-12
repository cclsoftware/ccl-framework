//************************************************************************************************
//
// Plug-In Template
// (Plug-In Copyright)
//
// Filename    : plugintemplate.cpp
// Description : Plug-In Template
//
//************************************************************************************************

#include "plugversion.h"
#include "core/public/coreplugin.h"

namespace PlugNamespace {
	
//************************************************************************************************
// PlugTemplate
//************************************************************************************************

class PlugTemplate: public Core::IPropertyHandler
{
public:
	// IPropertyHandler
	void setProperty (const Core::Property& value) override;
	void getProperty (Core::Property& value) override;
	void release () override;
};

} // namespace PlugNamespace

using namespace PlugNamespace;
using namespace Core;

//************************************************************************************************
// PlugTemplate
//************************************************************************************************

void PlugTemplate::setProperty (const Property& value)
{
	// TODO
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugTemplate::getProperty (Property& value)
{
	// TODO
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlugTemplate::release ()
{
	// TODO
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Exports
//////////////////////////////////////////////////////////////////////////////////////////////////

typedef Plugins::ClassFactory<PlugTemplate, IPropertyHandler> PlugTemplateFactory;

DEFINE_CORE_CLASSINFO 
(
	PlugTemplateClass,
	0,
	PlugType,
	PLUG_NAME,
	"(ProjectGUID)",
	"",
	PlugTemplateFactory::createInstance
)

BEGIN_CORE_CLASSINFO_BUNDLE (DEFINE_CORE_VERSIONINFO (PLUG_NAME, PLUG_COMPANY, PLUG_VERSION, PLUG_COPYRIGHT, PLUG_WEBSITE))
	ADD_CORE_CLASSINFO (PlugTemplateClass)
END_CORE_CLASSINFO_BUNDLE
