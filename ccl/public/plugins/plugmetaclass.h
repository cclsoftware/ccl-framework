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
// Filename    : ccl/public/plugins/plugmetaclass.h
// Description : Plug-in Meta Class
//
//************************************************************************************************

#ifndef _ccl_plugmetaclass_h
#define _ccl_plugmetaclass_h

#include "ccl/public/base/uid.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/plugins/classfactory.h"
#include "ccl/public/plugins/pluginst.h"

namespace CCL {

//************************************************************************************************
// Plug-in Meta Class macros
//************************************************************************************************

/*
	Example:

	DEFINE_PLUGIN_METACLASS (MyMetaClass, UID (...), "MyMetaClass")
	{ addResource (Meta::kClassImageResource, ...); }

	factory->registerClass (MyMetaClass::getDescription (), MyMetaClass::createInstance);
*/

#define DEFINE_PLUGIN_METACLASS(ClassName, cid, Name) \
class ClassName: public CCL::PluginMetaClass \
{ public: \
	static CCL::IUnknown* createInstance (CCL::UIDRef, void*) \
	{ return static_cast<CCL::IPluginMetaClass*> (NEW ClassName); } \
	static const CCL::ClassDesc& getDescription () \
	{ static const CCL::ClassDesc description (cid, PLUG_CATEGORY_METACLASS, Name); \
	  return description; } \
	static CCL::String getIdentifier () \
	{ CCL::String id; CCL::UID (getDescription ().classID).toString (id); return id; } \
	ClassName (); \
}; \
ClassName::ClassName ()

//************************************************************************************************
// PluginMetaClass
/** \ingroup base_plug */
//************************************************************************************************

class PluginMetaClass: public Unknown,
					   public PluginInstance,
					   public IPluginMetaClass
{
public:
	~PluginMetaClass ();
	
	void addResource (StringID id, UrlRef url, StringID language = nullptr);
	
	// IPluginMetaClass
	tresult CCL_API getResourceLocation (IUrl& url, StringID id, StringID language) override;
	
	CLASS_INTERFACE2 (IPluginMetaClass, IPluginInstance, Unknown)
	
protected:
	struct ResourceEntry; 
	Vector<ResourceEntry*> resources;
};

} // namespace CCL

#endif // _ccl_plugmetaclass_h
